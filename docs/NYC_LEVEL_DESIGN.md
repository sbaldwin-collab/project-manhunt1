# PROJECT MANHUNT — NYC Level Design (Unreal Engine 5)

**Source of truth this spec translates:** `src/world/World.js` (city layout data) and `src/world/NYCBlock.js` (the `CityBuilder` prop/building toolkit), tuned by `src/config/gameConfig.js` `world` block.

**Scope guardrail:** per `VERTICAL_SLICE_SCOPE.md`, this is **one level, 2–4 blocks, built once at high polish** — not a city-generation system. Everything below describes a single, specific, hand-art-directed level, using the browser prototype's proven layout as the starting blueprint.

---

## 1. Layout — direct port of the proven block arrangement

The browser world is a fixed-size playable area (±58m half-extent, i.e. a ~116m × 116m area) built from two crossing streets dividing it into four block quadrants, converging on a central intersection with a jail plaza tucked into one corner. This arrangement is proven to support the full gameplay loop (patrol routes, chase lines, multiple escape routes, cover density) and is the recommended starting blueprint for the UE5 level's blockout — re-derive it at real-world architectural scale rather than the browser's abstract units (see §2 for unit conversion).

```
                    N
                    │
        BLOCK NW    │    BLOCK NE
     (buildings L    │  (buildings L
      along inner    │   along inner
      edges, roof    │   edges)
      access #1)     │
                      │
   ───────────────────┼───────────────────  E–W cross street
                      │
        BLOCK SW    │    BLOCK SE
     (buildings L    │  (jail plaza
      along inner    │   carved into
      edges, roof    │   inner corner,
      access #2,     │   buildings set
      fenced lot)    │   back to leave
                      │   room)
                    │
                    S
              N–S avenue
```

- **Two streets cross at the level's center**: a north–south avenue and an east–west cross street, each with sidewalks on both sides.
- **Four block quadrants**, each built as an "L" of buildings hugging the two inner edges facing the intersection — this is deliberate, not a simplification: it keeps the highest building density and richest cover near where the player spends the most time (close to the action/jail), while the outer/far corner of each block opens into lower-density space (a fenced lot, rear-alley clutter) for visual and pacing variety.
- **The jail/lockup plaza** sits in a deliberately-void corner (SE quadrant's inner corner) — buildings on that block are set back further to leave an open, gated plaza rather than being carved out of solid geometry after the fact.
- **A uniform sidewalk depth** separates every building face from the street/curb — a real gap for streetlamps, hydrants, parked cars, and pedestrian-scale detail, not buildings opening directly onto traffic lanes.

## 2. Unit conversion & real-world scale

The browser build's units are meters-equivalent (Three.js has no inherent unit, but character radii of ~0.42–0.46 and a walk speed of 3.6 were tuned as if 1 unit = 1 meter, matching average human scale). Unreal's default unit is **centimeters**. Direct conversion table for level blockout:

| Browser value (units) | Meaning | UE5 value (cm) |
|---|---|---|
| 58 (world half-extent) | Playable area extends ±58 from center | ±5800 cm (~116m × 116m total) — **treat as a starting size, not a fixed requirement**; real architectural NYC blocks and street widths (see below) may naturally expand this somewhat once built at true scale — that's expected and fine within the "2–4 blocks" scope, not scope creep. |
| 11 (street total width, `STREET_HALF × 2` = 5.5×2) | Two-way street + both sidewalks | Real NYC secondary-street width (curb to curb) is commonly ~9–12m including sidewalks; 11m is a reasonable, already-validated starting width — keep it. |
| 3.4 (sidewalk depth, building face to curb) | Sidewalk width | ~3.4m is generous but plausible for a NYC sidewalk with streetlamps/hydrants/stoop space; keep as the starting value. |
| Building footprints: lengths 8–12, depths 12–19, heights 10–20 | Individual building plan/height | ~8–19m frontage, ~10–20m (roughly 3–6 stories at ~3.3m/floor) height — **at true architectural scale, use real story heights (see §5) rather than the browser's abstract height numbers**; the browser had no floor concept, just a box height. |
| Jail at (9, 9), radius 3.6 | Lockup plaza position/size | (~900cm, 900cm), ~3.6m radius cage — keep position (it's what makes the SE-quadrant setback work) and size (walkable, readable at a glance). |
| Roof access points: 2, at building heights ~17m and ~11m | Two fire-escape-to-rooftop transitions | Convert to real story counts: ~5–6 stories and ~3–4 stories respectively — gives the two rooftops distinct character (a taller "main" escape route, a shorter, easier secondary one). |

**Recommendation:** do not treat the browser's numbers as gospel measurements — treat them as a **proven relative-scale blueprint** (street-width-to-building-height ratios, sidewalk-to-building-depth ratios, block-to-jail distance) and rebuild it at true NYC architectural scale, which will feel more correct than a literal unit-for-unit copy once real-height buildings and real character eyelines are in play.

## 3. Block contents (per quadrant, direct port of `NYCBlock.js`'s prop toolkit)

Every quadrant should include, art-directed rather than proceduralized:

- **3–5 buildings** forming the inner-corner "L" — mix of brick apartment buildings and a brownstone or two for silhouette variety (the browser used a flat box-per-building model; UE5 should use real modular kit pieces with cornices, stoops, and varied roofline silhouettes — see §4).
- **1 storefront per 1–2 quadrants** (bodega, deli, laundromat — the browser explicitly modeled these three), each with: lit glass, a roll-down security gate (can be shown partially open for visual interest), a fabric awning, and readable signage.
- **A fire escape on one qualifying building per roof-access quadrant** (2 of the 4 quadrants get one — see §1), zig-zagging up the facade, doubling as the player's rooftop traversal route (`ANIMATION_SPEC.md` §6, `HUNTER_AI_SPEC.md` §6).
- **Rooftop detail** on every roof-access building: parapet wall (a real gameplay cover element, not just dressing), a wooden water tower, HVAC condenser units, a chimney — matching the browser's `_roofDetails()` prop set, built as real modular pieces.
- **Narrow passageways between buildings** (the browser's inter-building gaps) — these are load-bearing gameplay geometry (cut-through escape routes), not just spacing; keep them tight enough to feel like a real NYC gap-between-buildings (should read as roughly 1.5–2.5m clear width) and dress them with pipes, AC units, and grime to reinforce the scale.
- **One rear alley/interior lot per quadrant**, with dumpsters, loose trash bags, and (in one quadrant, matching the browser) a chain-link-fenced vacant lot — real cover and a distinct "back of the block" visual beat versus the street-facing side.
- **One scaffolding structure** (the browser places one specific instance) — good cover geometry and an easy, recognizable "under construction NYC" beat.
- **Graffiti** in 2+ alley/gap locations — texture/decal work, cheap to add, high atmosphere value.

## 4. Construction method

- **Modular kit approach**, not one-off unique buildings: a small library of wall segments, window/cornice trim, stoop/entrance pieces, roofline caps, and fire-escape modules, kitbashed into the specific building footprints in §1–3. This is both the standard AAA-environment-art method and the direct equivalent of the browser's `CityBuilder` toolkit-of-reusable-prop-factories pattern — the design intent (a small set of reusable, parameterized pieces assembled into the specific layout) carries over exactly; only the fidelity of each piece changes.
- **Nanite** for all static modular geometry — real brick/stone displacement detail, weathered edges, and fine trim without hand-managed polygon budgets.
- Building **street-facing facades get the full detail pass** (windows, cornices, fire escapes, signage); the far/interior-facing sides seen only briefly or from a distance can reasonably use simplified/tiling materials — apply art budget where the camera actually spends time, matching how the browser prioritized street-facing facade texturing over interior walls.
- **Streets/sidewalks**: real PBR asphalt (with a wet/rain response — see §7) and concrete sidewalk materials with crack/stain detail, plus decals for oil stains, chalk/paint markings, and grime — replacing the browser's tiled canvas textures.

## 5. Verticality & the rooftop rule

Rooftops are the level's designed "safe zone" — a player who successfully climbs a fire escape cannot be detected by the Hunter from the street (`HUNTER_AI_SPEC.md` §4's elevation gate). This is a core, already-tested escape-route design and must be preserved exactly:

- Both roof-access buildings need a **complete, walkable rooftop** (not just a fire-escape landing) with real parapet cover, so the rooftop is a genuine alternate route across part of the block, not a dead-end hiding spot.
- The two rooftops should ideally **not directly connect to each other** (matching the browser, which has two independent, unconnected roof-access points) — each is its own up-and-back escape option, not a continuous roof-level highway. This keeps street-level tension as the primary gameplay space, consistent with `VERTICAL_SLICE_SCOPE.md`'s scope (rooftop traversal is an escape *option*, not a parallel primary route).
- Floor-to-floor height used for the elevation detection gate (`HUNTER_AI_SPEC.md` §4) should be derived from the **actual built story heights** once the modular kit's floor module is finalized — do not hardcode the browser's abstract 3.2m constant without re-checking it against real geometry.

## 6. Lighting & atmosphere

Direct art-direction port of the browser's palette, executed with real UE5 lighting tools instead of flat emissive materials:

| Element | Browser approach | UE5 approach |
|---|---|---|
| Ambient/sky | Flat hemisphere light, cool blue-gray | Lumen sky/GI bounce off a cool night sky dome; a dim moonlit directional light casts soft, long shadows |
| Streetlights | Point lights, warm sodium-orange, no shadow casting (perf-driven) | Real sodium-vapor-toned point/spot lights on streetlamp meshes; selectively shadow-casting on hero lights near the demo path, non-shadow-casting on background fill lights (same perf-driven split the browser used, still valid guidance in UE5) |
| Storefront glow | Emissive plane + point light | Emissive glass material + light, plus actual interior "bleed" — a simple lit interior card behind the glass sells depth the browser's flat plane couldn't |
| Apartment windows | Instanced emissive planes, warm/cool mix, ~40% lit ratio | Same warm/cool mixed-lit-ratio approach via material instancing on the modular window trim; keep the "not every window is lit" rule, it's what makes it read as inhabited rather than a lightbox |
| Hunter flashlight | `THREE.SpotLight` on the Hunter's hand socket | `USpotLightComponent` attached to a hand/flashlight-prop socket, with a subtle flicker/handheld-sway; this is a good candidate for a cheap but high-value VFX beat (dust/rain visible in the beam via a volumetric-fog-lit spotlight) |
| Fog | `FogExp2`, cool blue-gray | Exponential Height Fog + Volumetric Fog for real light-shaft interaction with streetlights and the Hunter's flashlight |
| Rain | CSS overlay + `THREE.Points` particles | Niagara rain system + a wet-surface material function (specular/roughness response) on street and sidewalk materials — this single change (real wet-street response) is one of the highest-value upgrades over the browser build for the "this looks real" bar |
| Puddles | Flat dark circles, fixed positions | Either a small set of authored puddle decals with a planar-reflection-capable material, or a runtime wet/puddle material function driven by a rain-accumulation parameter, placed in low points (curb lines, mid-street) |
| Steam | Not implemented in the browser build | Add via Niagara at 1+ locations (a subway grate or vent) — explicitly requested and a strong, cheap atmosphere beat; treat as new work, not a port |

## 7. Gameplay-space checklist (does this level actually support the proven mechanics?)

Every item below must be true of the built level — this is the direct translation of `VERTICAL_SLICE_SCOPE.md` §2.1/2.3 into level-design acceptance checks:

- [ ] Parked cars exist at multiple points along the streets, positioned so they plausibly break a straight sightline between a street-level hiding spot and a likely Hunter patrol line.
- [ ] At least 3 distinct alley/narrow-passageway cut-throughs connect street to interior-lot space, each viable as a break-line-of-sight route.
- [ ] Dumpsters/fenced-lot geometry provide crouch-height cover, not just visual clutter — verify a crouched character is actually concealed behind them from likely Hunter approach angles.
- [ ] Both rooftops are reachable, walkable, and offer real cover once up there (not just a bare platform).
- [ ] From the jail plaza, at least two distinct street approaches exist, so a rescue attempt has a real route choice, not one obvious corridor.
- [ ] The Hunter's NavMesh covers every street/sidewalk/alley/interior-lot surface with no dead zones, and does **not** cover the rooftops (`HUNTER_AI_SPEC.md` §6).
- [ ] At least one storefront, one fenced lot, one scaffolding structure, and graffiti are present and lit per §6 — visual-variety requirements, not just mechanical ones.

## 8. Performance & scalability targets

Named explicitly since it's a real constraint, not just an art-direction question:

- **Target hardware must be confirmed before the level-art pass locks** (`UNREAL_MIGRATION_PLAN.md` §9 Phase 1) — a client demo machine (almost certainly a capable desktop/laptop GPU) is a very different performance budget than the browser build's mobile-Safari target. Do not assume the browser build's mobile-first perf discipline (capped pixel ratio, selective shadows, halved rain count on touch) is still the binding constraint; confirm the actual demo hardware and set a scalability tier (Epic/Cinematic vs. High) against it.
- **Nanite + Lumen are both GPU-cost-real**, especially Lumen's ray-traced-quality reflections on wet streets. Budget an explicit performance validation pass (`VERTICAL_SLICE_SCOPE.md` §5, item 5) on the actual demo hardware before calling the slice done — do not discover a frame-rate problem during a client rehearsal.
- If the confirmed target hardware is more constrained than expected, the first levers to pull are (in order of cost-to-benefit): Lumen scalability tier down one notch before disabling it outright; reduce volumetric fog resolution before reducing its presence entirely; reduce shadow-casting light count (keep the moon + hero streetlights on the demo path, drop background fill lights to non-shadow-casting first, matching the browser's own selective-shadow instinct) before reducing Nanite geometry detail, which is one of the build's biggest visual wins and should be protected longest.
