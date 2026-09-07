# Environment Catalog — Buildings, Architecture, Furniture, World Props

> **Written 2026-08-29.** Companion to `ItemCatalog_2026-08-12.md` (which covers carryable items/weapons/gear) — this one covers everything that makes up the *place* those items sit in: buildings, modular architecture, furniture, and exterior world dressing, for both playtesting and the completed game. Same rule as the item catalog: grounded in what the game's own design docs actually call for, not a generic "buildings a game needs" list.
>
> **Corrected same-day, before this doc was even a day old**: it originally said the hub was menu-driven and out of scope here, based on `OQ-BH-01`'s 2026-08-28 resolution. `CLAUDE.md`/`SessionHandoff.md` updated since (CR-14, still 2026-08-29) **reverse that** — the hub is now **Grayback Lodge, a real physical, walkable, persistent compound**, sitting inside the same continuous, never-reloaded world as the raid zone, not a separate menu screen. So the hub compound **is** in scope here after all, alongside the city/suburbs/rural zone content below — see the dedicated section for it.
>
> **Status of this content today: zero.** Confirmed by checking every top-level `Content/` folder — `Content/LevelPrototyping/` is Epic's own primitive greybox plugin (cubes, cylinders, ramps, a door/jump-pad/target — literally what B4 Stage 1's "small graybox test area" is built from), not a real kit. No environment/architecture/furniture pack of any kind is owned yet, unlike the item side where several packs turned out to already exist unused. `B2-T2` (kit selection) genuinely hasn't happened.

## Why this is a bigger undertaking than the item catalog

Worth being honest about before diving into the list: modular architecture isn't just "more props." It needs pieces that snap together on a grid, consistent collision/socket conventions, enough wall/floor/roof variety to avoid visible repetition across a city-sized area, and — per `B4-T3`'s multi-level system — real floor-to-floor navigation (stairs, floor volumes). `B2_ArtPipeline.md` already anticipates this: its own direction (`OQ-B2-01`) is "mostly free/cheap assets, Door Kickers 2 fidelity, dev wants to make some assets myself later" — which reads as **buy a modular kit for the structural pieces, hand-model the smaller furniture/prop layer**, not hand-model a whole building system from scratch. That's a recommendation grounded in the project's own stated direction, not a new decision — the actual kit choice is still `B2-T2`'s to make.

## Hub compound — Grayback Lodge (added post-CR-14 correction, 2026-08-29)

A converted mountain hunting-lodge/outfitter compound, walkable, persistent, part of the same continuous world as the raid zone — not a separate space. Four named structures, per `SessionHandoff.md`'s CR-14 entry:

| Structure | Function | Furniture/dressing notes |
|---|---|---|
| The Lodge | Vendor NPC + contract board | Taxidermy and gun-rack decor as the armory backdrop — this is flavor the dev specifically called out, worth prioritizing over generic dressing |
| The Cache | Stash vault | Root-cellar/walk-in-cooler aesthetic — shelving/storage furniture distinct from a generic container |
| The Motor Barn | Vehicle storage | Basic drivable vehicles land as part of this same pivot — this building needs actual vehicle meshes stored inside it, not just architecture. Vehicles are their own asset category, out of scope for this catalog, but flagging the dependency here since the Motor Barn is meaningless without them |
| Ridge lookout tower | Flavor only, not yet a scoped mechanic (`SessionHandoff.md`'s own qualifier) | Don't invest real content here until it has an actual gameplay purpose |

This compound is small (4 structures) relative to the full raid zone below, and — being the game's persistent "home base" the player sees every single session — arguably deserves priority over raid-zone building variety for an early playtest: a rough Lodge + Cache is enough to test the vendor/stash loop end to end, long before the full city needs to exist.

## Building types (by biome — matches `OQ-B4-01`'s confirmed urban/rural/wooded/suburban spread)

Each needs at least one interior layout; repeat/vary per biome to avoid the "every house is identical" problem `B4X-T1` will get caught on if ignored.

| Building | Biome | Mechanical identity (`OQ-B4-02`'s spread) |
|---|---|---|
| Apartment building | Urban | General residential loot, multi-level (`B4-T3`) |
| Convenience store | Urban | Food-site |
| Pharmacy / small hospital | Urban | Medical-site — the highest-value medical loot concentration |
| Gun store / police station | Urban | Firearms-site |
| Office building | Urban | Document/intel-site — feeds the new Classified Dossier contract item (`ItemCatalog_2026-08-12.md`'s pivot-aware section) |
| Grocery store | Urban/Suburban | Food-site, larger |
| Bank | Urban | Heist-contract site — keycard/vault content lives here |
| Single-family house (2–3 variants minimum) | Suburban | General residential, the most-repeated building — variety matters most here |
| Garage | Suburban | Tools-site |
| Strip mall | Suburban | Mixed small commercial |
| Farmhouse + barn | Rural | General residential + tools/containers |
| Gas station | Rural | Fuel/tools-site, small |
| Hunting cabin / ranger station | Wooded | Weapons/tools-site, small footprint |
| Warehouse | Industrial fringe | Large open loot volume, good horde-event space (`B5`'s Horde Migration candidate) |
| Power substation / water treatment | Industrial fringe | Ties directly into the utilities-shutoff mechanic (`AZSGameState`'s existing timer) — this is where that system should visibly live |

## Architectural modular kit (the actual `B2-T2` purchase/build target)

- **Walls** — exterior and interior, at least one damaged/breached variant (pairs with the resolved hybrid breach+lockpicking design, `OQ-B4-08`)
- **Floors / ceilings** — including a variant that reads correctly under `B4-T2`'s eventual interior-visibility solution (roof-fade vs. cutaway plane — whichever wins the spike needs matching geometry)
- **Doors** — intact, breachable, and a locked variant distinct enough to read at a glance
- **Windows** — intact and broken
- **Stairs** — `B4-T3`'s multi-level system is CONFIRMED as automatic/no manual control, but still needs real stair geometry and floor volumes to detect
- **Fences / chain-link** — exterior boundary pieces, urban and rural variants read differently
- **Basements** — per `OQ-B4-05`/`B4X-T6`: **fixed, authored layouts, not procedural** (dev-cut the layout-selection system) — this means basements are hand-placed content, not a kit-piece concern, but still need their own smaller modular set (concrete walls, support pillars, utility pipes)

## Furniture, by function

| Room type | Furniture |
|---|---|
| Residential — living | Sofa, TV stand, coffee table, bookshelf |
| Residential — bedroom | Bed, dresser, nightstand, closet |
| Residential — bathroom | Sink, toilet, bathtub/shower |
| Kitchen | Counter, stove, refrigerator, dining table + chairs (kitchen cabinet itself is already on `ItemCatalog_2026-08-12.md`'s World Containers list — don't remodel it here) |
| Office | Desk, office chair, filing cabinet, computer/monitor (set dressing, not interactive) |
| Medical | Hospital bed, medical cart, exam table |
| Retail | Shelving unit, checkout counter, cash register |
| Garage/Industrial | Workbench, tool rack, shelving |

## Exterior / other worldly props

Streetlight · traffic light/stop sign · mailbox · exterior trash can · fire hydrant · **abandoned/wrecked vehicle (static, non-drivable — world dressing and loot cover, distinct from the real drivable vehicles CR-14 puts in the Motor Barn)** · barricade/sandbag · road debris/rubble · utility pole (feeds the power-substation building's visual logic) · vegetation (tree, bush, tall grass — biome-differentiating, matters most for the wooded/rural split) · streetside dumpster (distinct from the lootable interior dumpster already on the item catalog, this one's pure dressing).

**Vehicles themselves are a separate asset category, out of this catalog's scope** — CR-14 lands *basic* drivable vehicles (movement + enter/exit seats) alongside the physical hub, meaning at least one real drivable vehicle mesh is now near-term-relevant, not a distant `BV`-phase concern. Worth its own tracking once vehicle scope is actually broken down; noted here only so it isn't lost.

## Open questions this surfaces

- **Kit sourcing is entirely undecided** — `B2-T2.1` ("modular kit chosen against the direction, covering small-town US buildings, forest, rural fringe, interiors") hasn't started. Nothing above should be modeled by hand in bulk until that decision lands, per `B2`'s own "decide once, then execute" sequencing — the risk of hand-building a wall/floor/door system that gets thrown away once a real kit is chosen is real.
- **Interior-visibility technique** (`OQ-B4-03`, roof-fade vs. cutaway plane) affects how ceiling/roof geometry needs to be built — worth resolving before any real interior gets dressed, not just before B4-T2 formally starts.
- **B5's event/investigation content's fate under the pivot is unconfirmed** (see `ItemCatalog_2026-08-12.md`'s pivot-aware section) — if it survives in some form, several building types above (the ones flagged as "document/intel-site," "clue-hiding" locations) inherit requirements from it; if it doesn't, those buildings still work fine as generic loot sites, just without the narrative layer.
