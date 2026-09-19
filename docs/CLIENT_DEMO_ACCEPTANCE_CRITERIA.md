# PROJECT MANHUNT — Client Demo Acceptance Criteria (Unreal Engine 5 Vertical Slice)

**Purpose:** the objective checklist that decides whether the UE5 vertical slice is ready to show the client. This is the exit gate for `UNREAL_MIGRATION_PLAN.md` §9 Phase 5 and the "definition of done" from `VERTICAL_SLICE_SCOPE.md` §5, expanded into a pass/fail rubric.

**Standard being held to, verbatim from direction received:** *the presentation must communicate "this game is entering production." It must not communicate "this is an experimental browser game."*

---

## 1. How to use this document

Before scheduling a client presentation, someone who did **not** work on the build day-to-day should walk this checklist against the actual running build, on the actual demo hardware, cold. Every unchecked item is a blocker, not a "nice to have" — this list only contains things that were explicitly scoped in `VERTICAL_SLICE_SCOPE.md`.

---

## 2. Visual bar

- [ ] All five characters (player, Hunter, 3 crew) are final art — no Mannequin/gray-box placeholders visible anywhere in the demo path.
- [ ] Characters read as real people at both close (over-the-shoulder) and mid-distance (chase) camera ranges — skin shading, hair, and clothing hold up under the Hunter's flashlight and under streetlight/practical lighting, not just in flat ambient light.
- [ ] Every building on the demo path uses final modular-kit art with Nanite detail — no flat, untextured, or obviously repeating/tiling surfaces at typical camera distance.
- [ ] Lumen GI is active and producing the intended look: warm window/storefront light visibly bouncing onto nearby surfaces, moonlight producing soft directional shadow, no obvious light leaks or black/unlit geometry.
- [ ] Rain is active with a visible wet-surface response on street/sidewalk materials (specular streaking, not just particles falling with no surface interaction).
- [ ] At least one puddle with a visible reflection is present on the demo path.
- [ ] Fog/volumetric fog is present and interacting visibly with streetlights and the Hunter's flashlight beam.
- [ ] Storefront signage, awnings, and roll gates are present, legible, and lit on at least 2 storefronts.
- [ ] Steam is visible from at least one source on the demo path.
- [ ] No console/debug UI, editor viewport chrome, or placeholder text (e.g. "TODO", default UE5 asset names) is visible anywhere in the captured/demoed path.

## 3. Motion & animation bar

- [ ] Locomotion (idle/walk/jog/sprint/crouch/crouch-walk) shows no foot sliding at any speed tier.
- [ ] Starting and stopping movement shows a real transition, not an instant velocity snap.
- [ ] Turning to face a new direction while stationary uses a turn-in-place animation, not an instant capsule rotation.
- [ ] The fire-escape climb is a real animated traversal (hand/foot contact visible, believable timing) — not a floating/sliding position interpolation.
- [ ] Vaulting a low obstacle (if included on the demo path) shows correct hand-plant contact via Motion Warping, adapted to the actual obstacle, not a generic clipped-through motion.
- [ ] The capture moment has a dedicated camera cut and a synced two-character animation (Hunter grab + target reaction) — this must **not** play as two independent, unsynced animations or a simple pose-swap.
- [ ] A captured crew member visibly struggles while jailed (not simply standing idle behind bars).
- [ ] The rescue moment has a visible, distinct reaction animation on the freed crew member(s).
- [ ] The Hunter's flashlight tracks/aims believably (upper-body layer) rather than staying rigidly fixed relative to the body during locomotion.

## 4. Gameplay loop — parity with the proven browser prototype

Reference: play `index.html` (this repo's browser build) side-by-side if there is any doubt about intended behavior.

- [ ] Player can walk, sprint (stamina-gated), and crouch, with visibly different movement speed and character posture per state.
- [ ] Sprinting drains stamina and prevents further sprinting below the minimum threshold; stamina regenerates when not sprinting.
- [ ] Sprinting has an audible/AI-perceptible chance of generating Hunter-detectable noise.
- [ ] The decoy ability generates a noise event the Hunter can investigate.
- [ ] Crouching measurably reduces the Hunter's effective detection range against the player.
- [ ] The Hunter visibly patrols when no target is perceived.
- [ ] On first spotting a target, the Hunter shows a brief "suspicious" beat (not an instant full chase) before committing to chase — this delay should be perceptible to a demo audience, not just present in code.
- [ ] Once chasing, breaking line of sight (a building corner, a parked car, an alley) causes the Hunter to lose the player and path to the last-seen location, then eventually give up and return to patrol if not re-spotted.
- [ ] A crew member the Hunter catches is captured and visibly placed in the jail.
- [ ] The player reaching the jail while crew are held releases them, with a clear visual/audio confirmation.
- [ ] A player caught by the Hunter ends the round with a clear loss state.
- [ ] Surviving the full round duration ends the round with a clear win state.
- [ ] A player who climbs to a rooftop is genuinely undetectable by the Hunter from the street — demonstrable live, not just asserted.
- [ ] The round timer, stamina meter, detection/alert indicator, and crew free/captured count are all visible and accurate throughout.

## 5. Camera bar

- [ ] The over-the-shoulder camera tracks smoothly with no jitter, no clipping through walls/geometry, and a believable collision pull-in when close to a wall.
- [ ] Sprinting visibly widens the field of view.
- [ ] Being spotted/chased produces a perceptible camera response (shake/intensity change) that reads as tension, not as a bug.
- [ ] The round opens with a directed cinematic camera beat, not an instant cut to gameplay.
- [ ] The capture moment cuts to its dedicated camera angle (§3).

## 6. Presentation & framing

This section is about how the build is *shown*, not just what it contains — equally important to the "entering production, not an experiment" standard.

- [ ] A branded intro/title screen is the first thing seen — no bare gameplay view or engine default UI on launch.
- [ ] The demo is run either as a **rehearsed, directed playthrough** of a known-good path, or from a **recorded trailer-quality capture** if a live demo carries risk in the pitch setting — a decision made deliberately in advance, not improvised in the room.
- [ ] The rehearsed/captured path visibly demonstrates, in sequence: street-level stealth movement, a spotted-and-chased beat, a successful break-line-of-sight escape using real cover (car, corner, or alley), a rooftop escape via fire escape, a capture-and-rescue beat, and a survive-to-win finish. (Matches `VERTICAL_SLICE_SCOPE.md` §2.4.)
- [ ] Frame rate is stable throughout the demo path on the confirmed target hardware — no visible stutter, especially not during the chase or capture beats, which are the presentation's emotional high points.
- [ ] No loading stalls, hitches, or visible streaming pop-in occur during the demo path.
- [ ] If presented live, a tested fallback (the recorded capture) is available in case of a live technical issue — the presentation should never be one dropped frame away from failure.

## 7. What is explicitly **not** evaluated here

Per `VERTICAL_SLICE_SCOPE.md` §3, the following are out of scope for this build and must not be held against it in a client review: additional levels/neighborhoods, multiplayer, difficulty progression, character customization systems, console certification, save/settings/accessibility/localization systems, and a fully mixed/QA'd audio pass beyond the demo path's needs. If the client raises these, the answer is "scoped for the next production phase," not a same-day fix.

## 8. Sign-off

This build is ready to present when **every checkbox above is checked**, on the confirmed target hardware, by someone other than its primary builder. A single unchecked item in §2–§5 is grounds to delay the presentation rather than show a build that risks landing as "an experiment" instead of "a game entering production" — which is the entire point of this migration effort.
