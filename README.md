# PROJECT MANHUNT

*A Vintage Street Games production*

A nighttime NYC stealth-chase vertical slice built with [Three.js](https://threejs.org/). One Hunter, one player, three crew, ninety seconds, four city blocks. Survive, break line of sight, and get your crew out of the lockup.

This README is written for whoever picks this codebase up next — a CTO, a senior engineer, or a future version of the team doing a production pass. It explains what's here, why it's built this way, what's still a placeholder, and what to do next.

---

## Project status: this build's role has changed

The client has reviewed this browser build and — correctly — rejected its visual fidelity for a commercial pitch. **PROJECT MANHUNT is now moving to a photorealistic Unreal Engine 5 vertical slice**, planned in full in [`docs/`](docs/).

This does not make the browser build obsolete. Its role going forward is:

- **The proven gameplay/systems reference.** Every rule in `docs/HUNTER_AI_SPEC.md`, `docs/ANIMATION_SPEC.md`, and `docs/NYC_LEVEL_DESIGN.md` was extracted from the working code in `src/` — this is the design source of truth the Unreal build is translated from, not a discarded prototype.
- **The regression target.** Anyone building the Unreal AI, camera, or movement systems can play this build in a browser with zero setup to check "does this feel like it's supposed to."
- **No longer the target of visual polish work.** Further presentation-quality effort goes into the Unreal build; this repository's Three.js code stays functionally maintained but is not being pushed toward AAA visual fidelity — that was never achievable in this stack, and isn't the goal anymore.

**Start here for the Unreal migration:** [`docs/UNREAL_MIGRATION_PLAN.md`](docs/UNREAL_MIGRATION_PLAN.md), which links out to the rest of the production package (scope, asset manifest, animation spec, Hunter AI spec, level design, and client acceptance criteria).

---

## Quick start

No build step, no bundler, no `npm install` required to run it.

```bash
# from the repo root
python3 -m http.server 8080
# or: npx http-server -p 8080
```

Then open `http://localhost:8080/`. That's it — `index.html` loads Three.js as a native ES module via an import map and everything else through `<script type="module">`.

**Why no build step:** the whole point of this slice is that it must run unmodified from GitHub Pages, from a plain static file server, and from a `file://`-adjacent static host with zero surprises. Three.js is vendored locally (see below) instead of pulled from a CDN, so the game also runs with no network access beyond the initial page load.

### Controls

| Action | Desktop | Mobile |
|---|---|---|
| Move | `WASD` / arrow keys | Left-side virtual joystick |
| Look / camera | Right-click drag | Right-side touch drag |
| Sprint | `Shift` (hold) | ⚡ button (hold) |
| Crouch | `C` (toggle) | ◒ button (toggle) |
| Decoy noise | `F` | ◉ button |
| Rescue (near lockup) | `E` | ↥ button |
| Pause | `Esc` | Ⅱ button |
| Climb / descend a fire escape | Walk up to it — automatic | Same |

---

## Architecture

```
index.html              Entry point. Import map + HUD markup + stylesheet.
styles/main.css          All HUD/overlay CSS (no inline styles).
vendor/three/            Vendored Three.js ESM build + the GLTFLoader addon
                          (see "Why Three.js is vendored" below).
src/
  main.js                 Bootstraps Game and mounts it to #game.
  utils.js                 Small math helpers (lerp, angle lerp, clamp, damp).
  config/
    gameConfig.js          Every tunable number in the game. Start here to
                            change speeds, detection ranges, timers, colors,
                            audio cue names, or the character/asset manifest.
  world/
    CollisionSystem.js      AABB colliders + line-of-sight raycasting, tagged
                             by "layer" (street vs. a specific rooftop) so
                             the player, the Hunter and the camera all agree
                             on what's solid.
    textures.js              Procedural canvas textures (brick, asphalt,
                              sidewalk, chain-link, signage, graffiti,
                              window-lit grids). No binary texture assets.
    WindowInstancer.js        One InstancedMesh for every lit/unlit window
                               in the city (hundreds of windows, one draw call).
    NYCBlock.js                `CityBuilder`: a toolkit of building/prop
                                factories (building, storefront, fire escape,
                                rooftop details, dumpster, car, hydrant,
                                fence, scaffolding, graffiti, streetlamp...).
                                Knows nothing about the city layout.
    World.js                    The actual city layout (data) + lighting,
                                 fog, ground/streets, rain, the jail plaza,
                                 and the roof-access registry. Composes
                                 CityBuilder calls into four connected blocks
                                 around a crossing intersection.
  game/
    InputManager.js           Single source of truth for keyboard, touch
                               joystick/look, and mouse-drag look. HUD
                               buttons call into it directly.
    CharacterFactory.js        Procedural, rig-animated civilian character
                                (see "Characters" below) + the GLTF drop-in
                                hook for a future production pipeline.
    PlayerController.js         Player movement, stamina, crouch, sprint
                                 noise, decoy throw, rooftop climb/descend.
    CrewAI.js                    The three crew members: wander, flee the
                                  Hunter when spotted, get captured, get
                                  rescued.
    HunterAI.js                   The Hunter's full state machine (see
                                   "Hunter AI" below).
    DetectionSystem.js             Shared FOV + line-of-sight + noise query
                                    logic used by the Hunter and by crew
                                    (to decide when to flee).
    RescueSystem.js                 Owns the jail: capturing crew, arranging
                                     them behind bars, releasing them.
    Game.js                          Orchestrator. Owns the renderer/scene/
                                      camera, wires every system together,
                                      runs the round timer and the frame loop.
  camera/
    ThirdPersonCamera.js       Over-the-shoulder follow camera: shoulder
                                offset, wall collision via raycast, sprint
                                FOV widen, shake, cinematic round-intro sweep.
  audio/
    AudioManager.js             All audio cues, synthesized at runtime with
                                 the Web Audio API (see "Audio" below).
  ui/
    HUD.js                       Thin controller over the static HUD markup
                                  in index.html — binds buttons, updates
                                  values, shows/hides overlays. Builds no DOM.
assets/
  models/ textures/ audio/ environment/ characters/
                                Empty, `.gitkeep`-tracked. This is where a
                                real asset pipeline drops its output — see
                                "Current limitations" and "Next steps".
```

**Why this split:** every system that used to live inline in one 4,700-line `<script>` tag now has one job. `World` doesn't know how the Hunter thinks; `HunterAI` doesn't know how a building is textured; `PlayerController` doesn't know how the camera avoids clipping through a wall. The `CollisionSystem` is the one shared source of truth all three query, so they can never disagree about what's solid.

### Why Three.js is vendored instead of loaded from a CDN

`vendor/three/three.module.js` is the official `three@0.160.0` ESM build, copied in as-is (MIT-licensed, header preserved), plus the `GLTFLoader` addon and its one dependency (`BufferGeometryUtils`). `index.html`'s import map points `"three"` at this local file.

This isn't a style preference — a CDN dependency is a real production risk for a game that needs to run reliably on GitHub Pages: a CDN outage, a regional block, or a strict CSP on the hosting domain all become launch-day incidents. Vendoring costs ~700KB of repo weight and buys total control. To upgrade Three.js, replace the two files under `vendor/three/` with a newer release's build output and re-test.

---

## Hunter AI

`src/game/HunterAI.js` is a straightforward explicit state machine (`HunterState` enum), not a behavior tree — deliberately, for a single enemy with well-understood behavior this is easier to read, debug and hand off than a general-purpose AI framework would be.

| State | What it does | Transitions out |
|---|---|---|
| **Patrol** | Walks to random points around the map, listens for noise pings within hearing radius. | Sees a target → **Suspicious**. Hears a noise → **Investigate**. |
| **Suspicious** | Something entered its vision cone. Meter fills over `suspiciousToSpotMs`; if the target leaves sight before it fills, meter drains and it falls back to **Patrol**. | Meter fills → **Chase**. Target lost → **Patrol**. |
| **Investigate** | Walks to the last noise location and looks around briefly. | Times out → **Return**. Spots someone en route → **Suspicious**/**Chase**. |
| **Chase** | Full speed toward the last known position of whichever target (player or crew) it currently sees. | Loses sight → **Search**. |
| **Search (last known position)** | Heads for the last confirmed position and lingers, hoping the target reappears. | Reaches the spot or times out → **Return**. Re-spots target → **Chase**. |
| **Return to patrol** | A short settle beat (looking around) before resuming patrol, so the Hunter doesn't feel like it's teleporting back to routine. | Timer expires → **Patrol**. |
| **Guard (captured crew)** | After a capture, there's a configurable chance (`hunter.guardCrewChance`) the Hunter lingers and orbits the lockup instead of immediately patrolling — a risk/reward window for a bold rescue attempt. | Timer expires → **Return**. |

**Detection** (`DetectionSystem.canSee`) combines: straight-line distance vs. a range that shrinks for a patrolling (less alert) Hunter and for a crouching target; a field-of-view cone (`hunter.fovDegrees`, default 100°); and a real line-of-sight raycast/segment test against every building and prop registered with the `CollisionSystem` — buildings, parked cars, dumpsters and fences all block sight, not just movement. Sprinting has a per-second chance to emit a noise ping the Hunter can hear through walls (but not see through). A rooftop is a distinct collision "layer" with its own elevation, so a Hunter on the street literally cannot perceive a target on a roof — see "Current limitations."

---

## Characters

There are no licensed or authored character models in this repository, so `CharacterFactory.js` builds a **procedural, animated stand-in**: a small hierarchy of pivot `Group`s (hip → spine → head, hip → legs, spine → arms) with primitive meshes hung off each joint, driven by a hand-written locomotion cycle (idle sway, walk/run stride, crouch, a scripted ladder-climb pose, and one-shot capture/rescue reaction poses). Player, Hunter and each of the three crew get a distinct silhouette via accent color, headwear (hood / beanie / flat cap), and a chest accessory (backpack vs. tactical vest).

This is deliberately built like a real character controller would be — the animation driver operates on joint rotations, not on swapping meshes — specifically so it's straightforward to point the same driver at bone transforms on an imported skinned GLTF rig later, instead of throwing this code away.

`loadExternalCharacter()` in the same file is the drop-in hook for that: flip `GameConfig.characters.useExternalModels` to `true`, point `GameConfig.characters.manifest` at real `.glb` files under `assets/characters/`, and `Game.js`'s character setup will load them via the vendored `GLTFLoader` instead of building the procedural rig. That path is wired but untested against a real asset, because none exists in this repo yet.

---

## Audio

Same situation as characters: zero binary audio assets ship in this repo. `AudioManager.js` synthesizes every cue at runtime with the Web Audio API — filtered noise loops for rain/city ambience/chase tension, oscillator sweeps for sirens, filtered noise bursts for footsteps/decoys, and a heartbeat that speeds up and gets louder with the detection meter. `GameConfig.audio.assets` has a named slot for every cue (`footstep`, `rain`, `siren`, `capture`, `rescue`, etc.); pointing one at a real file and teaching `play()`/`_startLoop()` to prefer a decoded buffer over synthesis is the entire integration surface — nothing else in the game calls into synthesis directly.

---

## Environment

Four connected blocks around a crossing intersection, built from a small data-driven layout (`World.js`) rather than hand-placed one-off geometry: brick apartment buildings with punched, instanced window grids (mixed warm/cool, mixed lit/unlit), two storefronts (deli, laundromat, bodega — glass, roll-gate, awning, canvas-texture sign), fire escapes doubling as rooftop access ladders, rooftop parapets/water tanks/HVAC units/chimneys, a fenced vacant lot, scaffolding, graffiti, parked cars, dumpsters, trash bags, hydrants, manholes, puddles, crosswalks, and a caged lockup plaza. Every solid prop registers an AABB with `CollisionSystem`, so cars, dumpsters, fences and building corners are all genuine cover — not just decoration — and the same colliders double as camera-collision occluders and Hunter line-of-sight blockers.

Rain, fog, a cool moonlit ambient + warm sodium streetlight palette, and per-window emissive tinting give the "2 AM in the city" look; the Hunter carries a spotlight for the flashlight beam.

---

## Camera

`ThirdPersonCamera.js`: smooth exponential follow with a shoulder offset, a raycast against every world occluder so it pulls in instead of clipping through a wall, a sprint-triggered FOV widen, a decaying shake impulse (triggered on spot/capture), and a short orbiting cinematic sweep at the start of each round.

---

## Performance

- Renderer pixel ratio is capped (1.5 on touch devices, 2 on desktop) rather than using the raw device pixel ratio.
- Shadows are selective: only the moon directional light and the Hunter's flashlight spotlight cast shadows; every streetlamp/sign/jail light is a non-shadow-casting point light.
- Every lit/unlit window in the city (hundreds of them) is one `InstancedMesh` draw call via `WindowInstancer`, not one mesh per window.
- Rain particle count is halved on touch devices.
- Materials and textures are all procedural/canvas-generated at a small resolution (128–512px), not multi-megabyte authored textures.

This has **not** been profiled on a physical low-end phone. It has been verified to run without console errors under a software (non-GPU-accelerated) WebGL renderer, which is a strictly harder condition than any real device with hardware WebGL. Before a client demo on real hardware, run it on the actual target devices and check the frame time, not just correctness.

---

## Deployment (GitHub Pages)

Nothing to configure beyond turning Pages on for this repository (Settings → Pages → deploy from the branch you push this to, root directory). Every asset reference in `index.html` and every module import is a **relative path** (`./vendor/...`, `src/main.js`, `styles/main.css`) — there are no absolute `/`-rooted paths anywhere, so this works identically whether the site is served from a domain root or from a GitHub Pages project subpath (`username.github.io/project-manhunt1/`). There's no server-side code, no environment variables, and no `localhost`-only assumption anywhere in the codebase.

---

## Current limitations

Being explicit about these because a vertical slice that hides its gaps is worse than one that names them:

- **No authored character or audio assets.** Both are procedural placeholders with a documented, working drop-in hook (see "Characters" and "Audio" above). This is the single biggest gap between this build and the "cinematic realistic" visual bar described in the brief — procedural code can't generate a photoreal human model or a mixed, mastered audio bed; that needs an actual art/audio pipeline (Mixamo/Metahuman/a contracted character artist; a sound designer or a licensed SFX library).
- **The Hunter is street-level only.** It never pursues onto a rooftop. A player who climbs a fire escape is safe by design, not by accident — but "guard a rooftop" or "chase up a ladder" AI doesn't exist yet.
- **Crew members never use rooftops** or fire escapes; only the player can climb.
- **Two rooftops, two fire escapes.** The brief asks for "at least two" — there are exactly two. More is a data-entry exercise in `World.js`, not an architecture change.
- **No save/settings persistence** beyond a single `localStorage` best-time. No audio volume slider, no graphics quality picker exposed in the UI (the knobs exist in `gameConfig.js`, just not surfaced).
- **Not profiled on physical hardware.** See "Performance" above.
- **Single Hunter, single round shape.** No difficulty curve, no alternate mission types, no meta-progression — this is one 90-second round, as scoped.
- **English-only, no accessibility pass** (colorblind-safe HUD colors, remappable controls, subtitle/caption support for audio cues are all absent).

---

## Recommended next production steps

**The commercial-fidelity path is now the Unreal Engine 5 vertical slice planned in `docs/`** — see the project status banner at the top of this README and start at [`docs/UNREAL_MIGRATION_PLAN.md`](docs/UNREAL_MIGRATION_PLAN.md). The items below are next steps **for this browser build specifically**, in its role as the ongoing gameplay/systems reference — not a path toward AAA visual fidelity in this stack, which is out of scope going forward.

1. **Keep this build's gameplay in sync with the Unreal build's.** If a gameplay number or rule changes during UE5 development (e.g. a Hunter detection range, a stamina value), update `src/config/gameConfig.js` and the relevant `HunterAI.js`/`PlayerController.js` logic here too, so this remains an accurate reference rather than drifting into a stale one.
2. **Rooftop-aware Hunter AI.** The Unreal spec (`docs/HUNTER_AI_SPEC.md` §6) explicitly carries forward the current street-only Hunter as a *design decision*, not a gap to close — do not extend `HunterAI.js` here to chase onto rooftops without a corresponding decision in the Unreal spec, or the two builds will disagree about a core mechanic.
3. **Automated tests.** There are currently none. `CollisionSystem`, `DetectionSystem` and the `HunterAI` state transitions are the highest-value places to start — they're pure enough logic to unit test without a renderer, and would make this build a more reliable regression reference for the Unreal team (`docs/HUNTER_AI_SPEC.md` §7 explicitly calls for side-by-side behavioral validation).
4. **Bug fixes only, no visual polish.** Character/environment/audio fidelity investment now goes into the Unreal build (`docs/ASSET_MANIFEST.md`); this build's Three.js presentation is intentionally frozen at its current quality level.

---

## Debugging

The running `Game` instance is exposed at `window.__game` in every build (not just a dev flag) — open devtools and inspect `__game.player`, `__game.hunter.state`, `__game.world.collision`, etc. This is intentional: it costs nothing at runtime and makes bug reports from a build far more actionable.
