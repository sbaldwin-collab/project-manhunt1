# PROJECT MANHUNT — Asset Manifest (Unreal Engine 5)

**Purpose:** a complete inventory of every asset category the vertical slice (`VERTICAL_SLICE_SCOPE.md`) needs, with an honest sourcing classification for each. This document exists to make budget and timeline conversations concrete — "we need characters" is not actionable; "we need 5 custom-rigged, custom-clothed characters with a licensed mocap animation set, ~X weeks, ~$Y" is.

## Sourcing legend

| Tag | Meaning |
|---|---|
| 🔧 **Procedural** | Can be generated/authored via engine tools or material graphs with no external acquisition — genuinely cheap and fast. |
| 🛒 **Marketplace / Licensed** | A commercial asset pack (Fab/Unreal Marketplace, or a stock/licensed library) can plausibly cover this at real but bounded cost — fastest path if a suitable pack exists at the needed quality. |
| 🎨 **Custom Art** | Requires a working artist (environment/character/prop) creating bespoke content — no shortcut, budget real time. |
| 🎬 **Custom Animation** | Requires either custom mocap capture or a skilled animator hand-keying to the required quality — the highest-risk, most-underestimated category industry-wide; flagged individually below. |

Every row also carries a **risk note** where relevant — this document does not pretend any of these are free.

---

## 1. Characters

| Asset | Sourcing | Notes |
|---|---|---|
| Player character base body/model | 🛒 or 🎨 | **Metahuman** (free with UE5, via the Metahuman Creator/Bridge pipeline) is the recommended starting path — genuinely photoreal out of the box, no separate purchase, customizable. Treat as 🛒-equivalent effort (selection + light customization), not full 🎨 sculpting, *if* Metahuman's stylistic range covers the desired character — confirm this early with a quick Metahuman test build before committing the plan to it. |
| Hunter character base body/model | 🛒 or 🎨 | Same Metahuman path; needs to be visually distinct from the player per `VERTICAL_SLICE_SCOPE.md` §2.2 — differentiate via build/proportions within Metahuman's customization range plus clothing (below), not necessarily a fully custom sculpt. |
| Crew characters ×3 base bodies/models | 🛒 or 🎨 | Same path; three more Metahuman variants, differentiated from each other and from player/Hunter. |
| Character skeleton/rig | 🔧 | Metahuman auto-rigs to a UE5-compatible skeleton; if any character is sourced outside Metahuman, budget rigging (🎨) or use Marketplace characters pre-rigged to the UE5 Mannequin skeleton. |
| Hair (all 5 characters) | 🛒 or 🎨 | Metahuman includes groomed hair options (card or strand-based); if a specific hairstyle isn't covered, custom hair grooming is a specialized 🎨 skill — flag early if any character needs a distinctive silhouette-defining hairstyle. |
| Skin/eye/teeth shading | 🔧 | Included with Metahuman's material setup; no separate line item if the Metahuman path is used. |

**Risk note:** the single biggest schedule risk in this entire manifest is character art quietly slipping from "🛒/🔧 via Metahuman" to "🎨 fully custom" because the client wants a specific, non-Metahuman look. Validate the Metahuman path against the client's visual bar (reference images) **before** locking the production schedule in `UNREAL_MIGRATION_PLAN.md` §9.

## 2. Clothing

| Asset | Sourcing | Notes |
|---|---|---|
| Player streetwear outfit | 🛒 | Marketplace clothing packs for UE5/Metahuman exist (jackets, hoodies, jeans, sneakers) and are a fast, realistic path for contemporary streetwear specifically. |
| Hunter outfit (distinct, slightly more "tactical"/utilitarian silhouette per the browser's design intent) | 🛒 | Same marketplace category; select pieces that read as distinct from the player's silhouette at a glance (per `VERTICAL_SLICE_SCOPE.md` §2.2), matching the browser's design intent of giving the Hunter a bulkier vest/jacket silhouette. |
| Crew outfits ×3 (distinct from each other) | 🛒 | Same category; the browser differentiated crew via hood/beanie/cap headwear and jacket color — a cheap, effective pattern to replicate with marketplace pieces + material color variants. |
| Cloth simulation setup | 🔧 | UE5's Chaos Cloth on jacket/hood pieces for believable movement — configuration work on top of purchased assets, not a separate acquisition. |

**Risk note:** clothing that needs to physically deform correctly with the custom locomotion/vault/climb animation set (`ANIMATION_SPEC.md`) may need cloth-sim tuning per outfit — budget iteration time, not just asset purchase time.

## 3. Animations

Cross-reference `ANIMATION_SPEC.md` for the full clip list this manifest sources. This is the **second-highest risk category** after characters.

| Asset | Sourcing | Notes |
|---|---|---|
| Base locomotion (idle/walk/jog/sprint/crouch/crouch-walk/strafe/start/stop/turns) | 🛒 | High-quality UE5-Mannequin-targeted locomotion packs exist on the Marketplace/Fab and on **Mixamo** (free, Adobe, auto-retargets to a humanoid skeleton) — Mixamo is a strong, low-cost starting point for the full base locomotion set and was already identified as the fast bridge path in the browser build's own README. |
| Vault / mantle / climb / roof-access / landing | 🛒 or 🎬 | Parkour/traversal animation packs exist on the Marketplace and can cover generic vault/mantle; **the fire-escape-specific climb (ladder-style, matching the browser's exact ground↔roof access points) is more likely to need custom keying or a short custom mocap session** to get the hand/foot contact right against the actual built geometry — budget as 🎬, validate against a Marketplace pack first in case one fits well enough via Motion Warping (`ANIMATION_SPEC.md` §6) to avoid the custom cost. |
| Capture (two-actor: Hunter grab + player/crew reaction) | 🎬 | Two-actor synced interaction animations are a specialized category — generic Marketplace "takedown"/"grab" packs rarely sync convincingly to a specific narrative beat (a Hunter physically seizing a fleeing target) without at least custom timing/retiming work. Budget real animator time or a short custom mocap pass; this is one of the demo's key emotional beats (`CLIENT_DEMO_ACCEPTANCE_CRITERIA.md`) and is not a place to accept an off-the-shelf approximation. |
| Struggle (captured crew, looping) | 🛒 or 🎬 | More generic — a "restrained"/"captured" loop from a Marketplace pack likely covers this adequately with minor retiming. |
| Rescue (relief/celebration reaction) | 🛒 | Generic celebration/relief clips from Marketplace packs are a reasonable fit. |
| Flashlight-aim upper-body layer (Hunter) | 🛒 or 🎨 | Aim-offset-style upper body packs exist; may need light custom keying to match the actual flashlight prop's hand socket. |
| Turn-in-place (90°/180°) | 🛒 | Standard inclusion in most quality locomotion packs (including Mixamo-adjacent and Marketplace sets); verify explicitly when selecting a pack since some base packs omit it. |

**Risk note, stated plainly:** licensed/marketplace animation gets the base locomotion and generic reaction set to a good place quickly and affordably. The Hunter/player **capture interaction specifically** is the one animation line item most likely to need real custom animator time or a short mocap session to land the "this feels like a real, dangerous moment" beat the client is paying for — do not budget it as a marketplace-pack line item.

## 4. Buildings / modular kit

| Asset | Sourcing | Notes |
|---|---|---|
| Modular brick/brownstone wall, window, cornice, stoop, roofline kit | 🛒 or 🎨 | Strong NYC/urban modular building kits exist on the Marketplace/Fab (search terms: "modular city," "urban street," "brownstone," "NYC apartment") — this is one of the more commodity-available categories; a good kit can cover the bulk of §3-in-`NYC_LEVEL_DESIGN.md`'s building needs. Custom art fills any specific silhouette/detail gap a purchased kit doesn't cover. |
| Fire escape modules | 🛒 | Common enough as a standalone prop/kit piece in urban asset packs. |
| Rooftop props: water tower, HVAC units, parapet trim, chimney | 🛒 | Standard rooftop/industrial prop categories, widely available. |
| Storefront-specific kit pieces (roll gate, awning frame, signage backer) | 🛒 or 🎨 | Roll gates and awnings are common in urban kits; **signage text/branding itself (bodega/deli/laundromat names) is bespoke** — see §9 Decals. |

## 5. Storefronts

Covered primarily by §4's kit + §9's decals/signage; no separate acquisition category beyond those two, plus:

| Asset | Sourcing | Notes |
|---|---|---|
| Interior "bleed" card (simple lit interior behind glass) | 🎨 | A cheap custom touch (`NYC_LEVEL_DESIGN.md` §6) — a simple flat interior card with an emissive material, not a fully modeled interior; low cost, real visual payoff. |

## 6. Vehicles

| Asset | Sourcing | Notes |
|---|---|---|
| Parked cars (3–5 variants) | 🛒 | Static/parked-only vehicle props (no drivable vehicle system needed per `VERTICAL_SLICE_SCOPE.md`) are widely available and cheap on the Marketplace — this is a low-risk, commodity category. |

## 7. Street props

| Asset | Sourcing | Notes |
|---|---|---|
| Dumpsters, trash bags, hydrants, manholes, streetlamps, traffic signals, street signs, chain-link fence, scaffolding | 🛒 | All standard "urban prop pack" contents — low risk, high availability, direct 1:1 replacements for the browser's procedural equivalents (`NYCBlock.js`'s `dumpster()`, `hydrant()`, `streetLamp()`, `chainLinkFence()`, `scaffolding()` functions name exactly this prop list). |

## 8. Roof props

Covered by §4 (water tower, HVAC, parapet, chimney) — no additional category.

## 9. Materials, decals, VFX

| Asset | Sourcing | Notes |
|---|---|---|
| PBR base materials: brick, concrete, asphalt, metal, glass, fabric (awnings/clothing) | 🛒 | Material libraries (Quixel/Megascans — included free with UE5 via Bridge — is the standout resource here) cover the overwhelming majority of this category at effectively no incremental cost. |
| Wet-surface / rain-response material function | 🎨 (light) | A parameterized master material function (roughness/specular response to a rain-accumulation value) — moderate one-time technical-art setup, then reused across every surface; high visual payoff per `NYC_LEVEL_DESIGN.md` §6. |
| Grime/grunge/crack detail layers | 🛒 | Megascans decal/detail libraries cover this well. |
| Custom signage (bodega/deli/laundromat/street signs, graffiti art) | 🎨 | Bespoke, brand-specific text/logo work — cannot be sourced generically; budget a texture/graphic-design pass. Low cost relative to character/animation work, but not zero. |
| Rain (Niagara) | 🔧 | Built with Niagara's standard particle tools; UE5 ships example rain systems as a strong starting point — configuration/tuning work, not acquisition. |
| Fog/volumetric fog | 🔧 | Engine-native (Exponential Height Fog + Volumetric Fog) — configuration, not acquisition. |
| Steam (vent/grate) | 🔧 | Niagara, similar to rain — configuration work. |
| Puddle reflections | 🔧 or 🎨 | Achievable via material function (🔧) for the common case; a hero puddle with planar reflection on the primary demo path could get a custom pass (🎨) if budget allows, since it's a high-visibility "real game" tell. |
| Chase/capture camera VFX (subtle, e.g. desaturation/vignette on capture) | 🔧 | Post-process material/volume work — engine-native tools. |

## 10. Audio

| Asset | Sourcing | Notes |
|---|---|---|
| Footsteps (surface-aware) | 🛒 | SFX libraries (licensed stock libraries, e.g. from established sound-effect vendors) cover this affordably and well. |
| Rain, distant traffic, sirens, subway rumble, city ambience | 🛒 or 🔧 | Licensed ambience libraries are the fast path; MetaSounds can also proceduralize some of these (rain especially) reasonably well if budget favors engine-time over licensing spend — either is viable, unlike the character/animation categories where procedural is not viable. |
| Hunter chase cue (musical/tension layer) | 🎨 (composition) | Needs an actual composer/sound designer for a purpose-built tension cue that intensifies with the chase state (`HUNTER_AI_SPEC.md` §5) — a licensed generic "tense music" stock track is a viable fallback if a composer isn't budgeted, but will read as more generic. |
| Heartbeat / breathing | 🛒 | Common stock SFX category. |
| Capture / rescue / interaction stingers | 🛒 | Short, common stock SFX category — low risk. |
| Mix/implementation via MetaSounds | 🔧 | Engine-native implementation work once source audio is acquired — not a separate acquisition, but real technical-audio time to wire concurrency, distance attenuation, and the chase-intensity layering correctly. |

---

## 11. Priority summary — what to lock down first

Ordered by schedule risk, highest first, informing `UNREAL_MIGRATION_PLAN.md` §9's phasing:

1. **Character art direction (Metahuman viability check)** — a go/no-go decision needed before Phase 3 can be scheduled with confidence. Do this first, cheaply, as a spike.
2. **Capture animation approach** (custom keying vs. short mocap session vs. accept a Marketplace approximation) — the highest-uncertainty single line item; get a quote/timeline for the real option before committing a client delivery date.
3. **NYC modular building kit selection** — evaluate 2–3 candidate Marketplace kits against `NYC_LEVEL_DESIGN.md`'s requirements early; this gates the entire environment-art timeline.
4. **Everything tagged 🛒 or 🔧 elsewhere** — genuinely lower risk, can be sourced/built in parallel once the above three are resolved, without blocking the schedule.

## 12. What this manifest deliberately does not include

Per `VERTICAL_SLICE_SCOPE.md` §3 (explicitly out of scope): no additional neighborhoods/city blocks beyond the one slice, no multiplayer-related assets, no character customization system assets, no console-certification assets, no localization assets, no assets for weather/time-of-day variation beyond the single art-directed nighttime-rain scenario.
