# PROJECT MANHUNT — Hunter AI Specification (Unreal Engine 5)

**Source of truth this spec translates:** `src/game/HunterAI.js` and `src/game/DetectionSystem.js`, tuned by `src/config/gameConfig.js` (`hunter`, `detection` blocks). Play the browser build to feel the reference behavior before implementing — it is the regression target (`UNREAL_MIGRATION_PLAN.md` §11).

---

## 1. Architecture overview

| Component | Purpose |
|---|---|
| `AManhuntHunterCharacter` | The Hunter's pawn: mesh, movement component, flashlight spotlight attachment. |
| `AManhuntHunterController` (`AAIController`) | Owns the Behavior Tree, Blackboard, and `UAIPerceptionComponent`. |
| `BT_Hunter` (Behavior Tree) | Encodes the state machine in §3. |
| `BB_Hunter` (Blackboard) | State data — see §2. |
| `UManhuntDetectionComponent` | Custom component wrapping `AIPerceptionComponent` with the crouch/elevation/patrol-alertness rules AIPerception doesn't support natively (§4). |
| NavMesh (`RecastNavMesh`) | Street-level walkable surface. Rooftops are **not** on the Hunter's NavMesh (§6). |

This mirrors the browser split cleanly: `HunterAI.js`'s `state` field → Blackboard `EHunterState` enum key; its `_findVisibleTarget` → `UManhuntDetectionComponent`; its per-state `update()` branches → Behavior Tree state nodes; its `_move()` → NavMesh `MoveTo` tasks with per-state speed.

---

## 2. Blackboard keys (`BB_Hunter`)

| Key | Type | Source in browser code |
|---|---|---|
| `SelfActor` | Object | — |
| `CurrentState` | Enum (`EHunterState`: Patrol, Suspicious, Investigate, Chase, Search, Return, Guard) | `this.state` |
| `TargetActor` | Object (nullable) | `this.currentTargetRef` |
| `TargetLocation` | Vector | `this.targetX/targetZ` |
| `LastKnownLocation` | Vector | `this.lastSeen` |
| `SuspicionMeter` | Float (0–550, ms) | `this.suspiciousMeter` |
| `SearchTimer` | Float (ms) | `this.searchTimer` |
| `ReturnTimer` | Float (ms) | `this.returnTimer` |
| `InvestigateTimer` | Float (ms) | `this.investigateTimer` |
| `GuardTimer` | Float (ms) | `this.guardTimer` |
| `HeardNoiseLocation` | Vector (nullable) | `detection.latestNoiseWithin(...)` result |
| `IsChasing` | Bool | `this.isChasing` getter |

## 3. State machine

Each row: browser behavior → Behavior Tree implementation. States are implemented as **BT Selector children guarded by Decorators reading `CurrentState`**, with a top-level Service ticking every frame to run the perception/transition logic (mirroring `HunterAI.update()`'s single per-frame pass) and write the new `CurrentState` before the state's action node runs.

### Patrol
- **Browser:** wander to random walkable points; if a noise ping lands within `hearingRadius` (20m), switch to Investigate.
- **UE5:** `BTTask_MoveTo` a random point from a pre-placed `PatrolPointsVolume` (or `NavMesh::GetRandomReachablePointInRadius`) at `Speed = HunterSpeeds.Patrol` (1.8 m/s equivalent). A `BTService` checks `UAIPerceptionComponent` hearing stimuli each tick; on a qualifying stimulus within 20m, set `HeardNoiseLocation` and transition to Investigate.
- View-distance modifier while in this state: **patrol alertness multiplier ×0.72** applied to sight range (browser: `viewDistance.patrolMultiplier`) — implemented in `UManhuntDetectionComponent`, not in stock AIPerception (see §4).

### Suspicious
- **Browser:** entered the moment a target is first seen from any non-Chase state; `suspiciousMeter` accumulates while sight is maintained (real time, ms) and decays at ~1.56× rate when sight is lost; reaching `suspiciousToSpotMs` (550ms) promotes to Chase; hitting zero demotes to Patrol.
- **UE5:** on first sight stimulus, transition to Suspicious and start `BTTask_RunBehavior` that increments `SuspicionMeter` by `DeltaTime * 1000` while `TargetActor` perceived, decrements by `DeltaTime * 1400` while not perceived (asymmetric rates ported exactly). Movement during this state: Hunter turns to face/track the target but does not path toward them yet (matches the browser, which doesn't change `targetX/targetZ` behavior distinctly for Suspicious beyond tracking — implement as "turn toward `TargetActor`, hold position or continue current path at `Speed = HunterSpeeds.Suspicious`" = 1.4 m/s).
- Threshold crossing → **Chase**, fire the `OnSpot` event (§5) which drives the chase-camera-shake and HUD "HUNTER SPOTTED YOU" beat on the player side.
- Meter hits zero → **Patrol**.

### Investigate (sound)
- **Browser:** triggered only from Patrol, by a noise ping within hearing radius. Heads to the noise location; if a target is spotted en route, promotes normally (Suspicious→Chase path). On arrival, `investigateTimer` accelerates (×3 effectively — `+= dt*1000*2` on top of the normal `+= dt*1000` increment while within 1.2m of the target point) to simulate a brisk look-around, then transitions out after 2200ms total.
- **UE5:** `BTTask_MoveTo(HeardNoiseLocation)` at `Speed = HunterSpeeds.Investigate` (2.7 m/s). On arrival, play a short head-swivel animation/look-around Aim Offset while the accelerated timer runs. Timer expiry → **Return**.

### Chase (Acquire target → Chase)
- **Browser:** full speed toward `TargetActor`'s live position, re-updated every frame the target is visible; resets `searchTimer` to 0 continuously while visible.
- **UE5:** `BTTask_MoveTo(TargetActor)` re-issued each tick perception updates the target's location (or a `BTTask` using `bReachTestIncludesAgentRadius` + continuous re-plan), `Speed = HunterSpeeds.Chase` (4.9 m/s — the fastest state, matching the browser). This is the only state where `TargetActor` (not just a location) drives movement.

### Lose line of sight → Search last known position
- **Browser:** the instant `CurrentState == Chase` and no target is visible this frame, transition to Search, target = `lastSeen`, fire `onLoseSight`.
- **UE5:** a Decorator on the Chase branch that fails when the perception stimulus for `TargetActor` becomes "not currently sensed" (AIPerception's `LastSensedStimulus.WasSuccessfullySensed() == false`) drops out of Chase into **Search**, capturing `LastKnownLocation` from the last successful sight stimulus location. Fire a Blueprint-assignable event here for camera/HUD hooks (§5).

### Search last known position
- **Browser:** paths to `lastSeen`; considers itself "arrived" within 1.3m; times out after `searchDurationMs` (4500ms) regardless of arrival. Either condition → Return.
- **UE5:** `BTTask_MoveTo(LastKnownLocation)`, `Speed = HunterSpeeds.Search` (3.0 m/s), with a parallel timer Decorator capping the state at 4.5s even if `MoveTo` hasn't reported success (mirrors the browser's "arrival OR timeout" OR-condition exactly — do not make it AND). Re-spotting the target during Search jumps straight back to **Chase** (not through Suspicious again — matches `HunterAI.update()`, which only re-arms `suspiciousMeter` accumulation from a non-Chase state).

### Return to patrol
- **Browser:** a fixed 1200ms settle beat (picks a new patrol target immediately but doesn't act "alert" during it) before formally resuming Patrol.
- **UE5:** `BTTask_Wait` 1.2s (optionally with an idle-look-around animation) using `Speed = HunterSpeeds.Patrol`, then transition to **Patrol**. Used as the common exit ramp from Search, Investigate, and Guard, exactly as in the browser (`_setState(HunterState.RETURN)` is called from all three).

### Guard (captured crew)
- **Browser:** entered probabilistically (35% chance, `guardCrewChance`) on a capture event, *from any state* (not part of the normal perception loop — the browser explicitly skips `_findVisibleTarget` while `state === GUARD`). Orbits the jail plaza at radius `jailRadius + 1.6` for a fixed 9000ms, then → Return.
- **UE5:** on the `RescueSystem`'s capture event (§ below / `HUNTER_AI_SPEC` interacts with `RescueSystem` Actor), roll `FMath::FRand() < 0.35`; if true, force-transition the Blackboard `CurrentState` to Guard regardless of current state, suppressing the perception Service for this state's duration. `BTTask` orbits the jail location (simple sine/cosine offset `MoveTo` loop, or a pre-placed `SplineComponent` ring around the jail) at `Speed = HunterSpeeds.Suspicious * 0.7` (browser value) for 9s, then **Return**.
- **Design intent, preserved:** this is a deliberate risk/reward window — a bold rescue attempt is riskier right after a capture. Do not tune away the probability without a design reason; it's a proven tension beat from the browser build.

---

## 4. Detection: vision, occlusion, hearing, crouch, verticality

`UManhuntDetectionComponent` wraps and augments `UAIPerceptionComponent` because several browser-proven rules have no stock AIPerception equivalent:

| Rule | Browser source | UE5 implementation |
|---|---|---|
| Field of view | 100° (`fovDegrees`) | `AISenseConfig_Sight::PeripheralVisionAngleDegrees = 50` (half-angle). |
| Base sight range | 15m (`viewDistance.base`) | `AISenseConfig_Sight::SightRadius = 1500` (cm). |
| **Patrol alertness reduces sight range ×0.72** | `viewDistance.patrolMultiplier` — applied only while `state ∈ {Patrol, Return}` | No stock equivalent. `UManhuntDetectionComponent` re-checks distance against `BaseRange * (CurrentState is Patrol/Return ? 0.72 : 1.0)` **after** AIPerception reports a sensed actor, rejecting the stimulus if it exceeds the state-adjusted range. |
| **Crouching reduces how visible the *target* is**, ×0.58 | `viewDistance.crouchTargetMultiplier` — applied per-candidate based on *their* crouch state, not the Hunter's | Same pattern: reject the AIPerception stimulus if `Distance > BaseRange * 0.58` and the sensed actor's `UStaminaComponent`/movement state reports crouched. |
| **Line-of-sight occlusion by geometry** | `CollisionSystem.hasLineOfSight` — a segment/AABB test against every registered building/prop collider | AIPerception Sight already performs a line trace by default (`AISenseConfig_Sight` uses the visibility trace channel) — **this is a case where UE5's stock behavior is sufficient and should be used as-is**, provided every building, parked car, dumpster, and fence is set to block the `Visibility` (or a dedicated `AISight`) trace channel. Verify explicitly during Phase 2 (`UNREAL_MIGRATION_PLAN.md`) that cover objects (cars, dumpsters, fences) are *not* accidentally set to ignore that channel — a common modular-kit import mistake. |
| **Elevation gate (rooftop is unseeable from the street)** | `Math.abs(observer.y - target.y) > 3.2` short-circuits `canSee()` before the LOS raycast even runs | No stock AIPerception equivalent — a Z-height check must be added. `UManhuntDetectionComponent` rejects any stimulus where `abs(HunterZ - TargetZ) > ~320cm` (tune against real building floor heights; the browser's 3.2m constant was tuned against its own building geometry, not a physical law — re-derive against real story heights in `NYC_LEVEL_DESIGN.md`). This is the mechanism that makes "climb the fire escape to be safe" work, and it must survive the port. |
| Hearing / sprint noise | `sprintNoiseChancePerSec: 6.5` — a per-second dice roll while sprinting registers a noise ping the Hunter can hear (not see) through walls, within `hearingRadius` 20m | Player's sprint state feeds a `ReportNoiseEvent(Location, Loudness)` call at the same probability into `UAISense_Hearing::ReportNoiseEvent`, radius 2000cm. Hearing in UE5 is already "through walls" by default (it's a radius+location report, not a traced sense) — matches the browser's intentional design (sound isn't blocked by walls, sight is). |
| Decoy noise | Player's `F` decoy action registers a louder, farther-reaching one-shot ping | Same `ReportNoiseEvent` call, higher `Loudness`/effective radius, on decoy activation. |
| Capture distance | 0.95m, checked independently of AI state every frame (§ below) | A capsule overlap query in `AManhuntHunterController` tick (or a `UCapsuleComponent::OnComponentBeginOverlap` on the Hunter), **not gated by `CurrentState`** — matches the browser exactly: capture in `Game.js` is a raw distance check run every frame regardless of what state the Hunter's AI is in, so a Hunter that is technically "Investigating" a decoy can still capture the player if it happens to close within range. This is a deliberate, already-tested rule — preserve it. |

### Suspicion vs. instant chase

Note the browser does **not** implement instant Patrol→Chase — every first sighting must pass through the 550ms Suspicious accumulation. This is an important feel property (the player gets a brief, fair warning beat — the HUD's "ALERT" state — before the Hunter fully commits) and must not be simplified away as "just use AIPerception's default sensed/forgotten latency," which has different semantics (a stimulus-age timer, not an accumulating suspicion meter that also *decays* while briefly unsighted). Implement `SuspicionMeter` as bespoke Blackboard float logic per §3, not as an AIPerception setting.

---

## 5. Events other systems hook into

Ported from the browser's `hunter.onSpot`, `hunter.onLoseSight`, `hunter.onStateChange` callback hooks (`Game.js` wires these to HUD log lines and camera shake):

| Event | Browser trigger | UE5 dispatch point | Consumers |
|---|---|---|---|
| `OnHunterSpottedPlayer` | Suspicious meter crosses threshold, target is the player specifically | Fired from the Suspicious→Chase BT transition, guarded by `TargetActor == PlayerCharacter` | Chase camera shake trigger, HUD "HUNTER SPOTTED YOU" line, chase audio cue (MetaSound chase layer fade-in) |
| `OnHunterLostPlayer` | Chase→Search transition while target was the player | Chase→Search BT transition | Chase audio fade-out, HUD state text |
| `OnCrewCaptured` | Capture overlap resolves against a crew member | `RescueSystem`/`AJailVolume` capture handling | Triggers the Guard-state roll (§3), jail placement, capture montage + capture camera cut (`ANIMATION_SPEC.md`), capture SFX |
| `OnPlayerCaptured` | Capture overlap resolves against the player | `AManhuntGameMode` | Ends the round (loss), capture montage/camera, HUD lose screen |

---

## 6. NavMesh & verticality

- Street-level `RecastNavMesh` covers all sidewalk/street/alley/interior-lot surfaces.
- **Rooftops are explicitly excluded from the Hunter's NavMesh** (either a separate `NavArea` the Hunter's `NavAgent` doesn't traverse, or physically disconnected nav geometry) — this is the direct, intentional port of the browser's `HunterAI._move()` which only ever operates on `GROUND_LAYER` collision and never queries rooftop colliders. The player-only fire-escape climb (a Motion-Warped montage, not NavMesh pathing — see `ANIMATION_SPEC.md`) is what makes rooftops a legitimate escape route specifically *because* the Hunter cannot follow.
- This is flagged as a named limitation in `UNREAL_MIGRATION_PLAN.md` and `VERTICAL_SLICE_SCOPE.md` §3 (out of scope: rooftop AI pursuit) — do not silently "fix" this during implementation without a design conversation; it's load-bearing for the level's escape-route design (`NYC_LEVEL_DESIGN.md`).

## 7. What to build vs. what to validate against the browser build

**Build fresh in UE5** (no direct code reuse possible): the Behavior Tree graph itself, the AIPerception configuration, the NavMesh, the `UManhuntDetectionComponent` C++ class.

**Validate against the browser build, side by side:** every timing constant in §3, every range/multiplier in §4, and the overall *feel* of the suspicious→chase ramp and the search-then-give-up behavior. The fastest way to catch a translation error is to run both builds and compare: spawn the Hunter, walk in and out of its cone at the same relative distances, and confirm the alert meter and state transitions happen at matching moments.
