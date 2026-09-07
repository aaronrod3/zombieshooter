# Priority Roadmap — Working Order Across the Three Planning Docs

> **Written 2026-08-29.** Synthesizes `ItemCatalog_2026-08-12.md`, `ItemSetupStandard_2026-08-06.md`, and `EnvironmentCatalog_2026-08-29.md` into one sequence: what actually unblocks a testable gameplay mechanic, ahead of what adds variety within a category that already works. Work top to bottom — don't skip to Tier 6/7 flavor items while Tier 1's free wins are still sitting on the table.

## Tier 1 — Wire up what's already free (zero Blender time)

The 2026-08-29 pack audit found real, usable meshes for all of these already sitting in owned packs, unused. This is pure `DA_ZS_ItemConfig`/`DA_ZS_WeaponConfig` authoring — no modeling, no Blender session needed — and it's the highest-value work available right now because it's nearly free.

- **Weapons**: Pistol, SMG (`Content/ZS/Items/Weapons/Meshes/`) — two more working guns for zero art time
- **Melee**: Fire Axe, Claw Hammer, Sledgehammer, Chainsaw
- **Combat/tactical**: Frag Grenade, Molotov Cocktail, Landmine, Bolt/Wire Cutters
- **Tools**: Flashlight, Lockpick, Battery, Matches/Lighter, Rope, Screwdriver, Duct Tape
- **Food/Drinks**: Water Bottle, Generic Can, Coffee
- **Ammo**: .357 (reskin of `SM_Bullet_45Cal`)
- **Containers**: Wooden Crate

## Tier 2 — Gear (the confirmed real Blender gap blocking the whole equip-visual loop)

Nothing worn exists yet at all. Already the active near-term target — full spec in `Docs/BlenderNotes.md` §7.

1. Backpack — in progress
2. Helmet
3. Vest
4. Belt

## Tier 3 — Real gaps blocking a specific mechanic from being fully testable

- **Pistol Magazine + SMG Magazine** (loose, carryable) — Tier 1 wires up two new guns, but they can't reload without these. Sequence right after Tier 1's Pistol/SMG wiring, not after Gear.
- **Splint, Antibiotics, Antiseptic Wipes** — the Disinfectant/Splint archetypes are fully built in code (`EZSItemUseType`) but have zero real items. Only Bandage is exercised today.

## Tier 4 — Zombie mesh + skin (parallel track, not sequenced into the Blender queue)

Still the single biggest content gap overall — animations imported, no mesh ever sourced. Listed on its own because it's a **different pipeline** (rigged/skinned character + animation retarget), not a static prop — pursue via sourcing a pack in parallel with the tiers above, don't block either on the other.

## Tier 5 — The B2 kit decision (a decision, not a modeling task)

Nothing in `EnvironmentCatalog_2026-08-29.md` can proceed — not the Lodge, not a single raid-zone building — until `B2-T2` (modular kit selection) actually happens. This is the one item on this whole roadmap that isn't "make something," it's "decide something," and everything environment-side is stalled behind it. Worth doing before Tier 8, not after.

## Tier 6 — Visual-confirm and wire the "plausible" weapon matches (editor work, not Blender)

The pack audit found candidate meshes for these but couldn't confirm which specific variant matches which archetype without opening the pack in-editor. Pick the right one, copy it into `Content/LowPolyPackUsedMeshes/`, wire the DA:

- Revolver, Pump Shotgun, Sawn-off Shotgun, Bolt-Action/Hunting Rifle
- Kitchen Knife, Baseball Bat

## Tier 7 — Remaining real Blender gaps (genre depth, nothing blocking)

- Machete — the one true melee gap left after Tier 6
- Smoke Grenade, Flare, Ballistic Plate, Weapon Cleaning/Repair Kit, Binoculars
- Classified Dossier, USB Drive/Data Stick, Keycard
- Tourniquet, Morphine/Pain Injector, Adrenaline Shot
- Wrapped Snack/Candy Bar
- Kitchen Cabinet, Locker, Dumpster (World Containers)
- Wallet, Cigarette Pack, Playing Cards (Misc/junk — lowest priority on the entire roadmap by design, pure volume filler)

## Tier 8 — Environment content execution (only after Tier 5 lands)

1. **Grayback Lodge rough pass first** — the Lodge + the Cache specifically. This is the one environment target worth prioritizing ahead of the raid zone: it's the persistent home base seen every single session, and a rough version is enough to test the vendor/stash loop end to end without needing the full city to exist.
2. Raid-zone buildings by biome (urban → suburban → rural → wooded, matching `OQ-B4-01`'s confirmed spread) — start with the "mechanical identity" sites (medical, firearms, food) since those directly serve the loot loop, before generic residential variety.
3. Furniture, by room type, following whichever buildings get built first.
4. Exterior world props — lowest priority, pure dressing.

## Blocked — needs a decision, not a queue position

Don't attempt these until the flagged question is actually resolved; they'd just need redoing otherwise.

- **Cash Bundle** — needs a code-side decision (does it bypass inventory entirely, or sit as a carryable item?) before any content is built for it.
- **Clothing** (all 6 slots) — blocked on the texture/material-swap system landing on the base skin mesh, which doesn't exist yet. Not a content gap at all right now.
- **Radio item** — B5's fate under the hub-and-raid pivot is unconfirmed; don't model this until that's resolved.
