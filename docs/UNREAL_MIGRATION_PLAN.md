# PROJECT MANHUNT — Unreal Engine 5 Migration Plan

**Status:** Planning document. No Unreal project exists in this repository yet.
**Audience:** the studio/contractor team that will build the UE5 vertical slice, and the client evaluating the production plan.
**Companion documents:** `VERTICAL_SLICE_SCOPE.md`, `ASSET_MANIFEST.md`, `ANIMATION_SPEC.md`, `HUNTER_AI_SPEC.md`, `NYC_LEVEL_DESIGN.md`, `CLIENT_DEMO_ACCEPTANCE_CRITERIA.md`.

---

## 1. Why this document exists

The browser build in `src/` (Three.js) proved the gameplay: a Hunter with real field-of-view and line-of-sight detection, a player who can sprint/crouch/distract/hide, three crew who can be captured and rescued, a 90-second survival round, and a dense-enough city block to support cover-based movement. The client has correctly rejected its visual fidelity for a commercial pitch — procedurally-built box-and-capsule characters and canvas-texture buildings were never going to clear a "this looks like a real game" bar, and they were never intended to.

**What carries forward is the design, not the code.** Every number, state machine, and rule below was extracted from the working browser implementation, not invented fresh — the Unreal build is a faithful re-implementation of proven gameplay in a AAA-capable engine, not a redesign.

**What does not carry forward is any Three.js code, procedural geometry, or synthesized audio.** Unreal Engine 5 is a different engine, a different language (C++/Blueprint), a different content pipeline, and a different asset economy. There is no code-level migration path — this is a from-scratch UE5 build that targets parity with the browser prototype's *rules*, built with real art.

---

## 2. Target engine & project setup

| Decision | Choice | Rationale |
|---|---|---|
| Engine version | **UE 5.4 LTS** (or the latest 5.4.x patch at project start) | Mature Lumen/Nanite, MetaSounds, Motion Matching experimental plugin available, well-supported by the Marketplace/Fab asset ecosystem. Avoid bleeding-edge point releases for a client-facing deadline. |
| Rendering path | **Lumen (software or hardware, scalability-dependent) + Nanite** for static environment meshes | This is a nighttime, light-driven scene (sodium streetlights, neon signage, moonlight, practicals). Lumen GI is the single highest-leverage visual investment for "this looks real" — see §6. |
| Project template | Third Person template, stripped to a custom `AManhuntCharacter` | Fastest path to a working camera + movement component we then heavily customize; the template's input/camera scaffolding is not worth reinventing. |
| Source control | **Git + Git LFS** for all binary content (`.uasset`, `.fbx`, textures, audio) | The web repo is plain Git; Unreal content is binary and merge-unfriendly. A **separate repository** is recommended (see §3) rather than nesting a multi-GB UE5 project inside this one. |
| Primary scripting | **C++ for core gameplay systems** (character movement extensions, AI controller, detection system, game mode/round timer), **Blueprint for content-facing logic** (level dressing triggers, UMG HUD wiring, VFX cues, montage notifies) | Matches how the browser code is already split: `PlayerController.js`/`HunterAI.js`/`DetectionSystem.js` are systems (→ C++); `HUD.js`/prop placement (→ Blueprint/level). Keeps performance-critical AI/detection code fast and testable, keeps iteration-heavy content work in the hands of designers without a C++ compile cycle. |

### 3. Repository structure recommendation

Do **not** commit a multi-gigabyte UE5 project into this repository. Recommended structure:

```
project-manhunt1/                  (this repo — unchanged, stays the browser reference build)
project-manhunt-unreal/            (new repo, Git LFS enabled from commit 1)
  ProjectManhunt.uproject
  Source/
    ProjectManhunt/
      Characters/
      AI/
      Detection/
      Camera/
      GameModes/
      UI/
  Content/
    Characters/
    Environment/
    Animation/
    Audio/
    VFX/
    UI/
  Config/
  docs/  → symlink or copy of this repo's /docs for on-site reference
```

This document package (`docs/`) is written to live in *either* repo; keep a copy in both if the Unreal repo is created, since it is the design source of truth for both builds.

---

## 4. System-by-system mapping

This is the core of the migration: for every module in the browser build, what it becomes in UE5, and what stays true vs. what necessarily changes because the engines work differently.

| Browser module | UE5 equivalent | Notes |
|---|---|---|
| `src/game/Game.js` (orchestrator, round timer, win/lose) | `AManhuntGameMode` + `AManhuntGameState` | Round duration (90s), win/lose conditions, and the capture-ends-round rule move to `GameState` so they're replication-ready if multiplayer is ever pursued (not in scope now, but GameMode/GameState is the correct place regardless). |
| `src/game/PlayerController.js` | `AManhuntCharacter` (extends `ACharacter`) + `UCharacterMovementComponent` overrides + `APlayerController` for input | Speeds, stamina drain/regen, and crouch behavior are direct numeric ports (see §5). Rooftop climb becomes a **root-motion montage + Motion Warping** interaction, not a scripted position tween (see `ANIMATION_SPEC.md`). |
| `src/game/CrewAI.js` | `AManhuntCrewCharacter` + a lightweight `AAIController` (Blackboard-only, no Behavior Tree needed — wander/flee is two states) | Flee logic ports directly; "flee when the Hunter can see me" uses the same `AIPerception`-driven query the Hunter itself uses (see `HUNTER_AI_SPEC.md` §5). |
| `src/game/HunterAI.js` | `AManhuntHunterController` (`AAIController`) + Behavior Tree + Blackboard | Full state-by-state translation in `HUNTER_AI_SPEC.md`. This is the single largest engineering task in the migration. |
| `src/game/DetectionSystem.js` | `UAIPerceptionComponent` (Sight + Hearing senses) on the Hunter, **plus a custom line-of-sight/elevation gate** since stock AIPerception sight is cone+range+LOS only | The crouch-visibility-reduction and rooftop-elevation-gate rules have no stock UE5 equivalent and must be custom logic layered on top of AIPerception (detailed in `HUNTER_AI_SPEC.md` §4). |
| `src/game/RescueSystem.js` | `AJailVolume` (Actor with a trigger volume + jail cage mesh) + a `URescueComponent` on the GameState | Capture/jail/rescue radius and jail position are direct ports. |
| `src/world/CollisionSystem.js` | Native UE5 **collision channels + Nav Mesh** | This is one of the few systems the engine already does *better* than the browser build — Unreal's physics/collision and NavMesh generation replace the hand-written AABB + Liang-Barsky segment test entirely. No custom collision system needed. |
| `src/world/World.js`, `src/world/NYCBlock.js` | **Level (persistent + streaming sublevels)** built from a modular Kitbash/ModKit or custom BSP-blocked-then-art-passed geometry | The *layout* (block arrangement, street widths, jail plaza position) is a direct design port — see `NYC_LEVEL_DESIGN.md`. The *construction method* is completely different: hand-placed/kitbashed static meshes with Nanite, not procedural box geometry. |
| `src/world/WindowInstancer.js` | **Instanced Static Mesh Components / HISM** for repeated window trim, or simply baked into modular building meshes with emissive window materials | Unreal's Nanite + HISM systems make this trivial compared to the manual InstancedMesh bookkeeping in the browser build. |
| `src/camera/ThirdPersonCamera.js` | `USpringArmComponent` + `UCameraComponent`, with a custom `UManhuntCameraComponent` for shake/cinematic-intro/capture-camera logic | Direct behavioral port; spring arm collision replaces the manual raycast-against-occluders code. See §7. |
| `src/audio/AudioManager.js` | **MetaSounds** + `UAudioComponent`s + a `USoundConcurrency`-managed cue system | Synthesized placeholder cues are replaced by MetaSound graphs (which *can* still procedurally generate some ambience, e.g. rain/wind, but character/impact/UI sounds need real recorded/designed SFX — see `ASSET_MANIFEST.md`). |
| `src/ui/HUD.js`, `styles/main.css` | **UMG (Unreal Motion Graphics)** widgets | Direct 1:1 port of every HUD element (stamina bar, detection meter, timer, crew count, rescue prompt, pause/win/lose screens) — this is the lowest-risk part of the migration. |
| `src/config/gameConfig.js` | A `UManhuntGameSettings` `UDataAsset` (or `.ini`-backed `UDeveloperSettings` class) | Every tunable number below is ported verbatim as the starting point; see §5. Keeping one designer-facing data asset (instead of scattering constants through C++) preserves the "one file to tune the whole game" property the browser build has today. |
| `src/game/CharacterFactory.js` (procedural rig) | **Not ported.** Replaced entirely by real Skeletal Mesh characters (Metahuman or licensed/custom) rigged to a standard UE5 humanoid skeleton. | See `ASSET_MANIFEST.md` and `ANIMATION_SPEC.md`. |
| `src/world/textures.js` (canvas textures) | **Not ported.** Replaced by authored/licensed PBR materials and decals. | See `ASSET_MANIFEST.md`. |

---

## 5. Gameplay constants — direct numeric ports

These values are extracted verbatim from `src/config/gameConfig.js` and are the tuning starting point in UE5. They should be treated as **the design baseline to feel-match**, not as untouchable physics — Unreal's movement component, camera, and animation blending will feel different enough from a browser lerp-based controller that some retuning during playtesting is expected. But they are the numbers the browser prototype was balanced against, so the Hunter's difficulty and the player's "just barely got away" tension should carry over.

| System | Value | Unreal home |
|---|---|---|
| Round duration | 90 seconds | `GameState::RoundDuration` |
| Cinematic intro length | 2.6s | Level Sequence at round start |
| Player walk / sprint / crouch speed | 3.6 / 6.3 / 1.75 (m/s equivalent) | `CharacterMovementComponent::MaxWalkSpeed` per-state (via a Gameplay Tag–driven speed table) |
| Stamina | max 100, sprint drain 24/s, walk regen 14/s, idle regen 20/s, min-to-sprint 3 | `UStaminaComponent` |
| Sprint noise chance | 6.5% per second while sprinting | Feeds `AIPerception` hearing stimulus generation |
| Crew count / wander speed / flee speed / flee trigger distance | 3 / 1.25 / 3.6 / 9.5m | `AManhuntCrewCharacter` defaults |
| Hunter patrol/suspicious/investigate/search/chase speeds | 1.8 / 1.4 / 2.7 / 3.0 / 4.9 (m/s) | Per-BT-state speed set via Blackboard-driven `MaxWalkSpeed` |
| Hunter field of view | 100° | `AIPerceptionComponent` Sight config `PeripheralVisionAngle` = 50° half-angle |
| Hunter view distance (base / crouch target multiplier / patrol multiplier) | 15m / ×0.58 / ×0.72 | Custom gate on top of AIPerception sight radius (AIPerception itself has no per-target-state range multiplier) |
| Hearing radius | 20m | `AIPerception` Hearing sense range |
| Capture distance | 0.95m | Capsule-overlap check in `AManhuntHunterController` |
| Suspicious→Chase threshold | 550ms sustained sight | Blackboard float `SuspicionMeter`, BT Decorator gate |
| Search duration | 4.5s | Blackboard timer |
| Return-to-patrol grace | 1.2s | Blackboard timer |
| Guard-the-jail chance after capture | 35% | `FMath::RandBool` weighted, on capture event |
| Detection meter rise/decay rate, chase floor | 130/s rise, 85/s decay, floor 78 | HUD-facing value only; drives the on-screen alert meter, not AI logic itself |
| Camera distance / shoulder offset / FOV base / FOV sprint | 4.6m / 0.68m / 56° / 62° | `SpringArmComponent::TargetArmLength`, socket offset, `CameraComponent::FieldOfView` |

---

## 6. Visual bar: what "photorealistic" requires from the engine

Being explicit, as instructed, about what code cannot do:

- **Lumen** (dynamic global illumination) is what will sell the "warm window light spilling onto wet asphalt, sodium streetlight glow, moonlit brick" look the client wants. This needs to be planned for from level-blockout day one — retrofitting Lumen-friendly lighting onto a level built for baked lighting is expensive.
- **Nanite** lets the environment use real film/AAA-density meshes (individually sculpted brick, weathered concrete, detailed fire escapes) without the manual polygon-budgeting that a browser WebGL build requires. This is a major advantage over the Three.js build, not a migration risk.
- **Real characters require real character art.** No amount of Unreal engine feature use substitutes for a licensed/custom character model, PBR clothing materials with correct roughness/subsurface skin shading, and groomed hair (Unreal Hair strands or card-based). This is the single largest budget line in `ASSET_MANIFEST.md` and the single largest gap between "prototype" and "commercial" — flag this to the client early and explicitly, it is not a rendering-settings problem.
- **Motion sells character believability as much as the model does.** A photoreal Metahuman standing on a stiff, foot-sliding locomotion cycle will still read as a game demo. `ANIMATION_SPEC.md` is not optional polish — budget real animation time/licensing for it.
- **Rain/wet-street/fog** in UE5 (Niagara rain VFX, a wet-surface material function driving contact-puddles/reflections, exponential height fog + volumetric fog) will look categorically better than the browser build's CSS rain overlay and flat puddle circles, at a real but manageable performance cost (see `NYC_LEVEL_DESIGN.md` §6 for scalability guidance).

---

## 7. Camera — headline UE5 approach

Full behavioral spec lives inline here since it's short; asset/animation dependencies are cross-referenced.

- `USpringArmComponent` with `bDoCollisionTest = true`, `bEnableCameraLag = true` / `bEnableCameraRotationLag = true` for smooth damping (replaces the browser's manual exponential lerp).
- Socket offset for the over-the-shoulder framing (replaces `shoulderOffset: 0.68`); tune against the real character's shoulder height, not the browser capsule's.
- Sprint FOV widen: `CameraComponent::FieldOfView` interpolated 56°→62° over ~0.3s (`FInterpTo`), matching `fovBase`/`fovSprint`.
- Chase shake: a `UCameraShakeBase` asset (`BP_ChaseCamShake`), triggered on Hunter-spots-player and intensified while in Chase state — a subtler, sustained version rather than a one-shot, replacing the browser's decaying shake impulse.
- Capture camera: a short dedicated Level Sequence / Camera Actor cut that frames the capture grab (see `ANIMATION_SPEC.md` capture montage) — the browser build only had a shake + red vignette; the UE5 build should cut to a purpose-built angle, since this is a key "feels like a real game" moment for the client demo.
- Cinematic round-opening: a Level Sequence (not hand-coded orbit math) — art-directable, matches `cinematicIntroMs: 2600` as a starting duration.
- Contextual framing: slight FOV/arm-length pull-in while crouched-and-stationary (stealth framing), matching the "this is a stealth game" read the client is expecting.

---

## 8. Team & roles required

This is not a one-person or code-only effort. Minimum viable team for the vertical slice scope defined in `VERTICAL_SLICE_SCOPE.md`:

| Role | Why |
|---|---|
| **Technical Designer / Gameplay Programmer (C++)** | Character movement, AI controller/Behavior Tree, detection system, game mode/round logic. This is the role most directly continuing the work in this repository. |
| **Environment Artist** (1–2) | Modular NYC kit construction/kitbashing, material authoring, level dressing. |
| **Character Artist / Technical Animator** | Character import/rigging (or Metahuman customization), clothing, skin/hair shading. |
| **Animator** (or a licensed mocap library + one technical animator to integrate it) | The full locomotion + traversal + capture/rescue animation set in `ANIMATION_SPEC.md`. |
| **Lighting/VFX Artist** | Lumen lighting pass, rain/fog/steam Niagara systems, neon/practical light dressing. |
| **UI Artist** | UMG HUD visual pass (the browser HUD's layout/logic ports directly; it needs a real art pass, not a rebuild). |
| **Sound Designer** | Real footstep/ambience/chase/capture/rescue audio via MetaSounds, replacing synthesized placeholders. |
| **Producer/PM** | Coordinating the above against the milestone plan in `VERTICAL_SLICE_SCOPE.md`. |

A single generalist could technically attempt this, but the timeline and quality bar implied by a $500K commercial engagement assumes a small team working in parallel tracks (level, character, AI/gameplay, animation, audio) rather than serial single-person execution.

---

## 9. Phased plan

| Phase | Deliverable | Depends on |
|---|---|---|
| **0. Pre-production** (this document package) | Migration plan, scope doc, asset manifest, animation spec, AI spec, level design doc, acceptance criteria — all committed to this repo. | Nothing (delivered now). |
| **1. Engine bring-up** | New UE5 repo from Third Person template; `AManhuntCharacter`, `AManhuntGameMode`, `UStaminaComponent` stubs; gray-box level matching `NYC_LEVEL_DESIGN.md` block layout; NavMesh baked. | Phase 0. |
| **2. Core gameplay parity** | Sprint/crouch/stamina/decoy/rescue working on the gray-box level; Hunter Behavior Tree implementing every state in `HUNTER_AI_SPEC.md` on placeholder (Mannequin) characters; capture/jail/rescue loop functional; HUD ported to UMG. **This phase's exit criterion is "the gray-box build plays exactly like the browser prototype."** | Phase 1. |
| **3. Art integration** | Real characters (rigged, clothed) replace Mannequins; modular NYC kit replaces gray-box geometry; Lumen lighting pass; MetaSounds audio pass. | Phase 2 (gameplay must be locked before art integration to avoid re-tuning against a moving target), `ASSET_MANIFEST.md` acquisition/production complete. |
| **4. Animation & camera polish** | Full blend space / state machine locomotion, motion warping for vault/mantle/turns, capture/rescue montages with dedicated camera cuts, chase camera shake tuning. | Phase 3 characters in place. |
| **5. Vertical slice polish & capture** | Bug fixing, performance pass against target hardware (see `NYC_LEVEL_DESIGN.md` §6), scripted client-demo playthrough rehearsed, trailer/capture footage recorded. | Phases 2–4 complete. Exit criteria = `CLIENT_DEMO_ACCEPTANCE_CRITERIA.md`. |

Phases 2 and 3 should overlap where possible (art production doesn't need to wait for AI to be 100% complete), but the **exit criterion of Phase 2 as a gray-box milestone is important to protect**: it is the point where the team can prove the game is fun and correct before spending art budget, and it is the natural internal demo to show the client that the underlying game (already proven once in the browser) survived the engine transition intact.

---

## 10. Risk register

| Risk | Mitigation |
|---|---|
| Character art/animation is underestimated in cost or time (most common AAA-adjacent budget failure) | `ASSET_MANIFEST.md` explicitly flags every custom-art and custom-animation line; get quotes/timelines for those specific lines before committing to a client delivery date. |
| Team tunes gameplay against placeholder Mannequin movement feel, then it changes once real character + animation is integrated | Protect the Phase 2 gray-box milestone (above) as a locked gameplay reference; re-validate against it after Phase 3/4, don't silently let animation blending change the game's difficulty. |
| Lumen/Nanite performance on the target hardware tier is unknown until tested | Establish target hardware and scalability tier at Phase 1 kickoff (see `NYC_LEVEL_DESIGN.md` §6); do not defer performance validation to the end of Phase 5. |
| Scope creep past "2–4 blocks, one vertical slice" | `VERTICAL_SLICE_SCOPE.md` is the explicit scope contract; any addition is a change order against it, not an assumed inclusion. |
| Client expects the UE5 build "any day now" because a playable browser build already exists | Set expectations explicitly: the browser build proved the *design*; the UE5 build is new production work on a different technology stack with a real art/animation budget and timeline, per this plan. |

---

## 11. Relationship to the existing browser build

Per direction: **the browser build in this repository is not removed, deprecated, or further visually polished.** Its role from this point forward is:

- The **interactive gameplay/systems reference** — anyone on the UE5 team can play it in a browser with zero setup to understand exactly how detection, chase, capture, and rescue are supposed to feel and behave, without needing the Unreal project open.
- The **regression reference** for the Phase 2 gray-box milestone above — "does the UE5 gray-box feel like this" is a concrete, playable A/B, not a written spec alone.
- The **living documentation of every gameplay constant** — `src/config/gameConfig.js` is the ground truth this document's §5 table was extracted from; if gameplay numbers need to change during UE5 development, update both.

No further work is planned on the browser build's visual presentation. Bug fixes to keep it functional as a reference are in scope; visual polish is not.
