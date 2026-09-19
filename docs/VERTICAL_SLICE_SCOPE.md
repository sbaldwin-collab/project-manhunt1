# PROJECT MANHUNT — Unreal Engine 5 Vertical Slice Scope

**Purpose:** a hard scope contract for the client-facing UE5 build. Everything in "In Scope" is what gets built. Everything in "Explicitly Out of Scope" is named on purpose, so it is never accidentally assumed to be included and never silently creeps into the timeline.

**Guiding rule, verbatim from direction received:** *"Do NOT attempt to build an entire city. Build one exceptionally polished vertical slice."*

---

## 1. What "the vertical slice" is

One playable level, one 90-second round shape, one Hunter, one player, three crew — built to a bar where footage of it could sit inside a AAA publisher pitch deck without an asterisk. Depth and polish over breadth, in every dimension: fewer blocks rendered at higher fidelity beats more blocks at prototype fidelity.

## 2. In scope

### 2.1 Level
- **2–4 connected NYC blocks**, matching the layout proven in `src/world/World.js` and detailed in `NYC_LEVEL_DESIGN.md`: two crossing streets, four block quadrants, a caged jail/lockup plaza near one intersection corner.
- Full street-level detail per `NYC_LEVEL_DESIGN.md`: brick buildings, brownstones, 2–3 dressed storefronts (bodega/deli/laundromat), fire escapes, alleys and narrow passageways, a fenced vacant lot, scaffolding, graffiti, parked cars, dumpsters, trash bags, hydrants, streetlamps, manholes, crosswalks, puddles.
- **Two rooftops reachable via fire escape**, each with real cover geometry (water tower, HVAC units, parapet) and a second descent route — matching the two roof-access points the browser prototype implements.
- Nighttime lighting: Lumen GI, moonlight + sodium streetlight palette, storefront glow, practicals in windows, the Hunter's flashlight.
- Atmosphere: rain, wet/reflective asphalt, fog/haze, steam (subway grate or vent steam in at least one location).

### 2.2 Characters
- **Player character**: fully rigged, clothed, animated (see `ANIMATION_SPEC.md`), believable human proportions and materials.
- **Hunter character**: visually distinct from the player (silhouette, clothing, and — if budget allows — a flashlight prop attachment), same animation fidelity bar as the player.
- **Three crew members**: visually distinct from each other and from the player/Hunter (clothing silhouette variation is sufficient; they do not need the player's full animation set — see `ANIMATION_SPEC.md` for the reduced crew animation list).

### 2.3 Gameplay systems
- Third-person movement: walk, jog/run, sprint, crouch, crouch-walk, directional strafing, start/stop, 90°/180° turns.
- Traversal: vault (low obstacles), climb/mantle (fire escape access), rooftop transition, landing.
- Stamina system gating sprint (direct port of the browser's drain/regen values, retuned to feel in UE5).
- Decoy/noise distraction ability.
- Hunter full state machine per `HUNTER_AI_SPEC.md`: patrol, suspicious, investigate, chase, lose-sight, search-last-known, return-to-patrol, guard-the-jail.
- Detection: vision cone, occlusion/line-of-sight, hearing (including sprint noise and decoys), crouch visibility reduction, verticality (rooftop = undetectable from the street, matching the browser rule).
- Capture: Hunter reaching the player ends the round (loss); Hunter reaching a crew member captures them to the jail.
- Rescue: player reaching the jail while crew are held releases them.
- 90-second round timer; win by surviving; restart flow.
- Third-person over-the-shoulder camera per `UNREAL_MIGRATION_PLAN.md` §7: collision, shoulder offset, sprint FOV, chase shake, capture camera cut, cinematic round-open.
- HUD: round timer, stamina, detection/alert meter, free/captured crew count, contextual rescue prompt, pause, win/lose, restart — ported from `src/ui/HUD.js`.

### 2.4 Presentation
- Branded intro/menu screen ("PROJECT MANHUNT" / "Vintage Street Games").
- A directed, rehearsed demo playthrough path through the level that reliably showcases: street stealth, a spotted-and-chased beat, a break-line-of-sight escape (alley or car cover), a rooftop escape, a capture-and-rescue beat, and a survive-to-win finish. (See `CLIENT_DEMO_ACCEPTANCE_CRITERIA.md`.)
- Recorded trailer-quality capture of that playthrough, in addition to the live playable build.

## 3. Explicitly out of scope

Naming these prevents scope creep and protects the timeline in `UNREAL_MIGRATION_PLAN.md` §9:

- **Any level content beyond the 2–4 block vertical slice.** No additional neighborhoods, no procedural city generation, no "more blocks later" placeholder streets.
- **Multiple Hunters, multiple rounds, difficulty progression, or a meta-game/menu-driven mission select.** One Hunter, one round shape, as proven.
- **Multiplayer/networking.** GameMode/GameState are structured in a replication-friendly way per `UNREAL_MIGRATION_PLAN.md` (good practice regardless), but netcode, matchmaking, and multiplayer testing are not part of this engagement.
- **Full character customization systems.** Each of the five characters (player, Hunter, 3 crew) is a fixed, art-directed character — not a modular customization system.
- **Console/platform certification.** The vertical slice targets PC (and the specific demo hardware it will be shown on); no submission-cert work for any storefront or console.
- **Save systems, settings menus, accessibility options, localization.** Out of scope for the pitch build; flagged as a real production-phase need in `UNREAL_MIGRATION_PLAN.md` §"next steps" thinking, not in this slice.
- **Full audio implementation beyond the demo's needs.** Enough MetaSounds/SFX/ambience to sell the rehearsed demo path convincingly; not a fully mixed, QA'd audio pass across every possible player action.
- **Rooftop-to-rooftop AI pursuit, or any Hunter rooftop awareness.** The browser prototype's rooftop-is-safe rule carries forward as-is (see `HUNTER_AI_SPEC.md` §6) — extending Hunter AI to rooftops is future-production work, not this slice.
- **Weather/time-of-day variation.** One fixed nighttime/rain lighting scenario, art-directed once, not a dynamic day-night or weather system.

## 4. Milestones (maps to `UNREAL_MIGRATION_PLAN.md` §9 phases)

| Milestone | Internal deliverable | Client-visible? |
|---|---|---|
| M1 — Gray-box parity | Full level layout blocked out, full Hunter AI functioning, full gameplay loop working on Mannequin placeholder characters. | Internal only. This is the "does it still play like the proven browser prototype" gate. |
| M2 — Art-in | Real characters, real modular NYC kit, base Lumen lighting, base MetaSounds audio all integrated into the M1 level. | Internal review; optional early client look if reassurance is needed. |
| M3 — Animation & camera pass | Full blend space locomotion, traversal motion warping, capture/rescue montages and camera cuts, chase shake tuned. | Internal review. |
| M4 — Vertical slice complete | Polish, performance validated against target hardware, rehearsed demo path locked. | **Client delivery.** Evaluated against `CLIENT_DEMO_ACCEPTANCE_CRITERIA.md`. |

## 5. Definition of done for the vertical slice

The slice is done when **all** of the following are true simultaneously (this list is the input to, and should stay in sync with, `CLIENT_DEMO_ACCEPTANCE_CRITERIA.md`):

1. Every system in §2.3 is functional and matches the proven browser-build rules (numerically re-tuned for UE5 feel where needed, behaviorally identical).
2. Every environment element listed in §2.1 is present, built from real (not placeholder/gray-box) art, and lit with the final Lumen lighting pass.
3. All five characters are final art, final rig, final animation set — no Mannequin placeholders remain in the demo path.
4. The rehearsed demo playthrough runs start-to-finish without a bug that breaks the intended beat sequence.
5. The build runs at a stable, demo-safe frame rate on the confirmed target hardware (see `NYC_LEVEL_DESIGN.md` §6) — validated by an actual play session on that hardware, not assumed from editor performance.
6. A recorded trailer-quality capture of the demo playthrough exists as a fallback if a live demo is not possible in the pitch setting.
