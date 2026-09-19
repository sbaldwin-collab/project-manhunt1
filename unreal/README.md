# PROJECT MANHUNT — Unreal Engine 5 Project (Scaffold)

**Status: unopened, uncompiled scaffold.** Nothing in this directory has been run through the Unreal Editor or a C++ compiler — it was authored as plain text against documented UE5 5.4 APIs and the design package in `/docs` (one level up from this directory). Treat every claim in this README's "should work" language as exactly that, not a verified fact, until someone opens `ProjectManhunt.uproject` in the real editor and works through Section 3 below.

This exists because scaffolding real, structurally-correct starting code is far more useful to a team picking this up than a written spec alone — but it is still a first draft that needs a first compile pass, not a finished, validated codebase.

---

## 1. What this is

A from-scratch Unreal Engine 5 C++ project implementing the systems specified in `/docs/UNREAL_MIGRATION_PLAN.md` and translated line-by-line from the working browser reference build in `/src` (one level up). See that migration plan's Section 4 ("System-by-system mapping") for the authoritative browser-file-to-UE5-class table; the short version:

| Area | Classes |
|---|---|
| Player | `AManhuntPlayerCharacter`, `UStaminaComponent`, `UManhuntCameraComponent` |
| Hunter AI | `AManhuntHunterCharacter`, `AManhuntHunterController`, `UManhuntDetectionComponent` |
| Crew | `AManhuntCrewCharacter`, `AManhuntCrewController` |
| Round rules | `AManhuntGameMode`, `AManhuntGameState`, `AManhuntPlayerController` |
| Jail / rescue | `AJailVolume`, `URescueComponent` |
| Rooftop traversal | `AManhuntClimbVolume` |
| Audio | `UManhuntAudioManager` |
| Tuning | `UManhuntGameSettings` (Project Settings > Game > Manhunt Game Settings) |
| Behavior-Tree migration starters | `UBTTask_FindPatrolPoint`, `UBTService_UpdateSuspicion` (see Section 4) |

## 2. What this is **not**

- **Not art, not animation, not a level.** `Content/ProjectManhunt/` is an empty folder structure (tracked via `.gitkeep`) matching `/docs/ASSET_MANIFEST.md`'s categories, ready to receive real content. There is no map/level in this scaffold — `Content/ProjectManhunt/Maps/VerticalSlice/` is an empty destination folder, not a populated level.
- **Not a Behavior Tree implementation of the Hunter AI**, despite `/docs/HUNTER_AI_SPEC.md` specifying one as the target architecture. `AManhuntHunterController` implements the full state machine natively in C++ (Tick-driven) instead — **read the long comment at the top of `Source/ProjectManhunt/AI/ManhuntHunterController.h` before changing this**; it explains why, and names `UBTTask_FindPatrolPoint` / `UBTService_UpdateSuspicion` as working starter templates for the BT migration if the team wants designer-facing iteration speed instead.
- **Not validated against a real skeleton/animation set.** Anywhere this scaffold references a bone socket (e.g. the Hunter's flashlight attaching to `"hand_r"`) or a montage/root-motion action (vault, climb, capture), it is a documented hand-off point, not a working implementation — see `/docs/ANIMATION_SPEC.md`.
- **Not performance-tested, not platform-tested, not a complete game.** It is the Phase 1 "engine bring-up" deliverable described in `/docs/UNREAL_MIGRATION_PLAN.md` Section 9 — the starting skeleton for that phase's work, not its output.

## 3. Before you trust this compiles: validate these in the editor

In rough order of "how likely this is to need a fix on first open":

1. **Generate project files and do a first compile before anything else.** Right-click `ProjectManhunt.uproject` → "Generate Visual Studio project files" (or the Mac/Linux equivalent), then build. Fix whatever the compiler finds — this scaffold has never been compiled.
2. **`UManhuntGameSettings::AudioCues`** (`TMap<FName, TSoftObjectPtr<USoundBase>>`) and **`UManhuntDetectionComponent::GatedDetectionState`** (`TMap<TObjectPtr<AActor>, bool>`) — both use container UPROPERTYs with object-type keys/values. This is supported in modern UE5 but is exactly the kind of declaration worth a first-compile check.
3. **`UAISense_Hearing::ReportNoiseEvent`'s exact parameter list** (called from `AManhuntPlayerCharacter::ThrowDecoy`/`UpdateLocomotionAndSpeed`) and **`UGameplayStatics::SpawnSound2D`'s exact parameter list** (called from `UManhuntAudioManager::GetOrStartLoop`) — both have had default-parameter changes across UE5 point releases; confirm the calls here match your installed engine version's header.
4. **`FBlackboardKeySelector::AddVectorFilter` / `AddFloatFilter` / `AddObjectFilter`** calls in `AI/Tasks/BTTask_FindPatrolPoint.cpp` and `BTService_UpdateSuspicion.cpp` — standard pattern, but Blackboard key filter APIs are a common source of small per-version signature drift.
5. **The Hunter's flashlight socket name (`"hand_r"`)** in `AManhuntHunterCharacter`'s constructor — will silently attach to the component root if the socket doesn't exist on whatever skeleton ends up assigned. Fix once a real/Mannequin skeleton is in place.
6. **`AManhuntHunterController::SetPerceptionComponent` / `UAISenseConfig_Sight::GetSenseImplementation`** — correct as of the UE5 API this was written against; a quick compile will confirm for your exact engine build.
7. **Everything marked `TODO` or "hand-off point" in code comments** — these are deliberate stubs (montage playback in `AManhuntPlayerCharacter::BeginClimb`, the Level Sequence hookup in `UManhuntCameraComponent::PlayCinematicIntro_Implementation`, per-map GameMode/spawn-point wiring) documented at the point they're needed, not oversights.

None of the above are reasons to distrust the *design* — every gameplay rule, timing value, and state transition was ported deliberately from the working, tested browser build (see `/docs/UNREAL_MIGRATION_PLAN.md` Section 5 for the full constant-by-constant citation). They're a first-compile checklist for the *Unreal API surface*, which cannot be verified without the engine.

## 4. Recommended first tasks (maps to `/docs/UNREAL_MIGRATION_PLAN.md` Section 9, Phase 1)

1. Compile (Section 3 above).
2. Build a gray-box level (`Content/ProjectManhunt/Maps/VerticalSlice/`) matching `/docs/NYC_LEVEL_DESIGN.md`'s layout using BSP/placeholder blocks — enough to bake a NavMesh and place one `AManhuntHunterCharacter` + `AManhuntPlayerCharacter` start + three `AManhuntCrewCharacter` + one `AJailVolume` + two `AManhuntClimbVolume` pairs.
3. Assign a `UBehaviorTree`/skip it per the note in Section 2 above; either way, confirm the Hunter patrols, spots, chases, loses, searches, and returns correctly against the same beats the browser build (`/index.html` at the repo root) demonstrates.
4. Author the UMG HUD widget (port of `/src/ui/HUD.js` + `/styles/main.css`) and assign it to `AManhuntPlayerController::HudWidgetClass`.
5. Once the above is solid, this is the Phase 1 exit gate — proceed to Phase 2 (art integration) per the migration plan.

## 5. Why this lives inside the browser-build repository for now

`/docs/UNREAL_MIGRATION_PLAN.md` Section 3 recommends splitting the Unreal project into its own Git-LFS-enabled repository once real binary content starts flowing in, specifically because binary game assets don't belong in a plain Git history alongside the browser build's source. This scaffold is still 100% text (C++, `.ini`, `.uproject` JSON) — genuinely fine to review and iterate on here. **Split it out before the first `.uasset`/`.fbx`/texture is added** — `.gitattributes` in this directory is already pre-configured for Git LFS so that move is a `git mv`/history-preserving extraction, not a re-authoring effort, whenever the team is ready to do it.
