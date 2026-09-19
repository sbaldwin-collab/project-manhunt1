# PROJECT MANHUNT — Animation Specification (Unreal Engine 5)

**Source of truth this spec translates:** `src/game/CharacterFactory.js` (the browser's procedural locomotion driver — idle/walk/run/crouch/climb states, capture/rescue one-shot reactions) and the movement rules in `src/game/PlayerController.js`, `CrewAI.js`, `HunterAI.js`.

**Read this alongside `ASSET_MANIFEST.md`** — this document specifies *what* the animation system must do; the manifest specifies *where each clip comes from* (Marketplace/Mixamo vs. custom mocap) and its cost/risk profile. This document does not itself supply animation clips.

---

## 1. Why this matters as much as the character model

Stated plainly, per direction received: a photoreal character model on a stiff or sliding locomotion cycle still reads as a tech demo, not a game. The client's rejection was about *believability*, and motion is at least as responsible for believability as the mesh/material is. This is not a polish pass to defer — it is core to clearing the visual bar.

## 2. Skeleton

- Target the **UE5 Manny/Quinn (UE5 Mannequin) skeleton hierarchy**, which is also what **Metahuman** characters are retargeted to via UE5's IK Retargeter. This maximizes compatibility with Marketplace animation packs (the large majority target the UE4/UE5 Mannequin skeleton or are one retarget away from it) and with Metahuman if that path is chosen for character art (`ASSET_MANIFEST.md`).
- All five characters (player, Hunter, 3 crew) share **one skeleton and one Animation Blueprint** where possible; only the Hunter needs an animation-layer addition (flashlight-aim upper body, see §7). Shared-skeleton, shared-ABP is a direct efficiency win over the browser build, which had to hand-build a separate procedural rig per character.

## 3. Locomotion states (full list, mapped from the browser + expanded per direction received)

| State | Browser equivalent | UE5 implementation |
|---|---|---|
| Idle | `CharacterFactory` `'idle'` — subtle breathing bob | Idle animation, looping, with a secondary "long idle" variant (weight shift / look-around) triggered after ~8–10s stationary to avoid a robotic loop-read |
| Walk | `'walk'` — moderate stride | 2D Blend Space (see §4) at low speed input |
| Jog | *(new — the browser only had walk/run; direction requests jog as a distinct speed tier)* | Blend Space mid-speed tier. Recommended addition even though the browser skipped it: a walk→sprint jump reads as a game-y speed pop in a "believable" third-person camera; a jog tier smooths it. |
| Sprint | `'run'` — larger stride, forward lean | Blend Space top-speed tier, matches `player.sprintSpeed` (6.3 vs `walkSpeed` 3.6 — roughly 1.75× walk, informs blend space speed thresholds) |
| Crouch (idle) | `'crouch'` state, reduced hip height | Separate crouch idle animation (not just a scaled standing idle — crouch changes the capsule height and silhouette, needs its own pose) |
| Crouch-walk | `'crouch'` + moving, reduced stride | Crouch Blend Space, low speed cap matching `player.crouchSpeed` (1.75, roughly half walk speed) |
| Directional strafing | *(new — the browser's camera-relative movement let the character face any direction, but had no distinct strafe animation, just a rotated walk cycle)* | 2D Blend Space (Speed × Direction, -180° to 180°) so sideways/backward movement while camera-locked reads correctly rather than a walk cycle awkwardly rotated in world space |
| Start movement | *(new)* | Short directional start animation blended in via the Locomotion State Machine's Idle→Move transition, avoiding an instant-velocity "teleport into stride" pop |
| Stop movement | *(new)* | Foot-plant stop animation (ideally with a Distance Matching–style adjustment, see §6) so the character doesn't slide to a halt |
| 90° turn | *(new)* | In-place turn animation, blended by Aim Offset or a dedicated turn-in-place state when speed ≈ 0 and desired facing changes sharply — prevents foot-sliding pivots |
| 180° turn | *(new)* | Same system, larger-angle clip; also covers a "quick check behind you" beat that sells tension well in a stealth game |
| Vault | *(new — low obstacles: curbs, low fences, car hoods)* | Root-motion vault montage + Motion Warping (§6) to align hand/foot contact with the obstacle regardless of exact approach angle/height |
| Climb / Mantle | Browser's `'climb'` state — a scripted alternating-arm procedural pose over a fixed 1.5s position tween | Root-motion climb/mantle montage set for the fire escape, replacing the browser's procedural tween entirely — see §6 |
| Roof access (transition) | Browser: instant elevation change at climb completion | A short "step onto rooftop" mantle-out clip at the top of the climb montage, and a matching "step onto fire escape platform" clip at the top of the ladder-to-roof-edge transition, so the street↔roof transition has a real physical beat instead of a teleport |
| Landing | *(new — implied by roof access/vault/mantle needing a landing counterpart)* | Landing pose + a light landing-noise trigger (Anim Notify → audio + a brief Hunter-audible noise event if landing hard while sprint-descending, consistent with the hearing rules in `HUNTER_AI_SPEC.md`) |
| Capture (reaction) | Browser's `playOneShot('capture', 700ms)` — torso pitches forward, arms flail, hip drops | A real capture montage: Hunter grab animation + player/crew capture-reaction animation, played in sync (two-actor montage pairing) with the dedicated capture camera cut (`UNREAL_MIGRATION_PLAN.md` §7) |
| Struggle | *(new — direction explicitly requests this; the browser had no equivalent)* | A short looping struggle animation for a captured crew member while held in the jail cage, before the rescue reaction plays — sells "these are real captured people," not just a state flag |
| Rescue (reaction) | Browser's `playOneShot('rescue', 800ms)` — arms raised, small bounce | Rescue reaction montage — relief/celebration beat — for the crew member(s) freed, timed with the jail cage open animation/VFX |

## 4. Blend Spaces

- **`BS_Locomotion_Stand`** — 2D (Speed 0→SprintSpeed on one axis, Movement Direction -180°→180° on the other), covering idle/walk/jog/sprint/strafe from a single space, sampled by the Animation Blueprint's core locomotion state.
- **`BS_Locomotion_Crouch`** — same shape, capped at `player.crouchSpeed`, used while the crouch Blackboard-equivalent flag is set.
- Speed thresholds seeded from the browser's actual values (walk 3.6, sprint 6.3, crouch 1.75) — **use these as the starting sample points**, then refine by feel once real mocap is in, since a 2D blend space's perceived quality depends heavily on how the sampled clips' foot-plant timing lines up, which the browser's procedural sine-wave gait had no equivalent constraint on.

## 5. Animation Blueprint / State Machine structure

- **Locomotion state machine**: `Idle ↔ Move` (Blend Space–driven), with `Start`/`Stop` transition animations (§3) inserted on the edges rather than relying on the Blend Space alone to absorb acceleration.
- **Turn-in-place sub-state**: active when `Speed < threshold` and `|DesiredFacing - ActorFacing| > ~60°`, playing the 90°/180° turn clips from §3 instead of letting the capsule instantly snap-rotate (the single biggest "this is a real person" vs. "this is a game character" tell in third-person games).
- **Upper-body layer** (slot-based, `FAnimSlotGroup`): drives the flashlight-aim pose on the Hunter and any future one-handed-carry poses, layered over the base locomotion state machine so aiming/looking doesn't require a whole-body animation set.
- **Full-body montage layer**: capture, struggle, rescue, vault, climb/mantle — all played as montages that temporarily own the full body (or full body via root motion), per §6.

## 6. Root motion, IK, and Motion Warping

| Technique | Where it's required | Why |
|---|---|---|
| **Root motion** | Vault, climb/mantle, capture, rescue montages | These are all *displacement-defining* actions (the animation itself determines how far/where the character ends up), unlike locomotion (which is *velocity-defining*, driven by the Character Movement Component and best left in-place/non-root-motion for responsiveness). This exactly matches the split already implicit in the browser build: `PlayerController.js` drives x/z every frame for normal movement (velocity-defining), but the climb was a **hard-coded position tween** (`_updateClimb`, an eased lerp from ground point to roof point) — i.e., the browser already treats climbing as displacement-defining, just without real animation. Root motion is the correct real-animation equivalent of that tween. |
| **Motion Warping** | Vault (align to obstacle height/angle), climb/mantle (align start hand-plant to the actual fire escape geometry, align end position to the actual roof-edge point) | The browser's climb points (`world.roofAccess[].ground` / `.roof`) are exact, hand-placed coordinates the character teleport-interpolates between. Motion Warping is what lets a *fixed* animation clip adapt to those *exact* level-specific contact points without needing a unique animation per fire escape — warp targets = the same `ground`/`roof` access-point transforms already defined in the level (`NYC_LEVEL_DESIGN.md`). |
| **Foot IK (per-foot ground trace + pelvis offset)** | All locomotion, standing idle, crouch idle | Nighttime NYC streets are not flat — curbs, cracked pavement, the road/sidewalk height transition. Foot IK is required for characters to plant correctly on this geometry rather than clipping into or floating above it; the browser build had zero foot placement logic (fixed Y=0 for all characters, explicitly noted as a known limitation — see `src/game/PlayerController.js`'s flat elevation model). |
| **Hand IK** | Vault (hand-plant on obstacle top), climb (hand-plant on fire escape rails/ladder rungs) | Sells the physical contact Motion Warping's positioning alone won't. |
| **Aim Offset** | Hunter's flashlight-aim upper body, and the "look around" beat during Investigate (`HUNTER_AI_SPEC.md` §3) | Lets the AI's head/torso track a point of interest without a full-body animation per look direction. |

## 7. Per-character animation set scope

Not every character needs every clip — matching the browser, which gave crew members a smaller behavior set (`wander`/`flee` only, no crouch/climb/decoy) than the player:

| Character | Needs |
|---|---|
| **Player** | Full set: every row in §3. |
| **Hunter** | Full locomotion set (§3, minus vault/climb/mantle/roof-access — per `HUNTER_AI_SPEC.md` §6, the Hunter never leaves street level) **plus** the flashlight-aim upper-body layer (§5) and the capture "grab" half of the capture montage pairing. |
| **Crew (×3)** | Reduced set: idle, walk, jog/flee-sprint, capture reaction (the "being grabbed" half, distinct from the Hunter's "grabbing" half), struggle (while jailed), rescue reaction. No crouch, no vault/climb/mantle (crew never use rooftops — `VERTICAL_SLICE_SCOPE.md` §3), no strafing needed (their AI never strafes, per `CrewAI.js`). |

## 8. Anim Notifies tying back into gameplay systems

Direct ports of moments the browser build handled as timer-based side effects, now correctly tied to actual animation frames:

| Notify | Placed on | Triggers |
|---|---|---|
| `AN_Footstep` | Every locomotion clip, at each foot-plant frame | Footstep SFX (surface-aware if budget allows) **and** — critically — the sprint noise-event roll from `HUNTER_AI_SPEC.md` §4 should fire from this notify rather than a flat per-second dice roll, once real animation exists; it's a more accurate (and more art-directable) source of "how often does a footstep actually land" than the browser's `Math.random() < chancePerSec * dt` approximation. |
| `AN_HandContact` | Vault/climb/mantle montages | Hand IK contact point activation, and the corresponding traversal audio (fabric/metal-rail sounds). |
| `AN_CaptureGrab` | Capture montage (Hunter half) | Triggers the capture camera cut (`UNREAL_MIGRATION_PLAN.md` §7), capture SFX, and the `OnPlayerCaptured`/`OnCrewCaptured` gameplay event (`HUNTER_AI_SPEC.md` §5) — i.e., the *animation* is what fires the gameplay consequence, not a distance check resolving instantly. This is an intentional improvement over the browser, where capture is a same-frame distance check with the reaction pose played after the fact. |
| `AN_JailRelease` | Rescue montage | Un-jails the crew member, matching `RescueSystem.tryRescue`'s effect but keyed to the animation's release beat. |
| `AN_LandingImpact` | Landing clip | Landing SFX + hard-landing noise event (§3 "Landing" row). |

## 9. What this spec does not cover

- Specific clip counts, mocap session planning, or a shot list for a mocap shoot — that is a production-costing exercise for whichever acquisition path `ASSET_MANIFEST.md` selects (licensed pack vs. custom mocap), not a design document task.
- Facial animation / lip sync — not required for this vertical slice (no dialogue is specified in `VERTICAL_SLICE_SCOPE.md`); flag as a future-production item if voiced characters are added later.
