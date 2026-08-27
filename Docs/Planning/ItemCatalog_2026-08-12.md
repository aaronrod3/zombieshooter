# Item Catalog — Full Game Scope

> **Written 2026-08-12.** A majority-coverage list of the items this game plausibly needs at full scope — not just the near-term Blender queue (that's `Docs/Planning/ItemSetupStandard_2026-08-06.md` §9, keep using that one for "what's active right now"). This is the bigger picture underneath it: what a reasonably complete PZ-style survival catalog looks like for this specific project, grounded in the archetypes/mechanics that already exist in code (`UZSItemConfig`/`UZSWeaponConfig`, `EZSEquipSlot`'s 11 slots, the medical/needs systems) rather than invented from a generic list.
>
> **Status tags**, one per row: `[Done]` real `DA_ZS_*` + mesh exist · `[Data-only]` a source pack already has a usable mesh, just needs a `DA_ZS_ItemConfig`/`DA_ZS_WeaponConfig` instance authored · `[Blender]` a real modeling gap · `[Texture-only]` no mesh at all, ever — planned as a skin/material swap · `[Source]` better bought/downloaded than hand-modeled (a rigged character, mainly).
>
> **Deliberately not covered here**: deep crafting materials/recipes (`CLAUDE.md`'s Off-Limits section lists deep crafting as CUT, needs its own planning pass before it's real scope), vehicles (own future phase, `BV`), NPC faction-specific gear (full factions are post-v1). Counts below are a reasonable genre-appropriate spread, not an attempt at exhaustiveness — pad categories further only when a real design reason calls for it (a new loot-tier need, a new recipe, a new skill), not just to hit a bigger number.

## Weapons — Ranged

| Item | Notes | Status |
|---|---|---|
| Assault Rifle | | `[Done]` |
| Pistol | mesh sourcing unconfirmed this pass, worth a quick check | `[Data-only]`? |
| Revolver | | `[Blender]` |
| Pump Shotgun | | `[Blender]` |
| Sawn-off Shotgun | cheap variant, reuses the pump shotgun's texture/material where possible | `[Blender]` |
| SMG | | `[Blender]` |
| Bolt-Action / Hunting Rifle | high damage, slow follow-up — a real alternative to the AR, not a reskin | `[Blender]` |

## Weapons — Melee

| Item | Notes | Status |
|---|---|---|
| Crowbar | | `[Done]` |
| Kitchen Knife | one-handed, fits `SecondaryHand`-eligible per `Handedness`/`bUsableInSecondaryHand` | `[Blender]` |
| Baseball Bat | | `[Blender]` |
| Fire Axe | two-handed | `[Blender]` |
| Machete | | `[Blender]` |
| Claw Hammer | doubles conceptually as a Tools-category item — one mesh, two `DA_ZS_ItemConfig`/`DA_ZS_WeaponConfig` uses is a design call, not required | `[Blender]` |
| Sledgehammer | two-handed, slow/heavy | `[Blender]` |

## Ammo

| Item | Notes | Status |
|---|---|---|
| 9mm (Pistol/SMG) | | `[Data-only]` — check `Content/Mega_Survival_Tools/` first |
| 5.56 (AR) | | `[Data-only]` — check pack first |
| .308 (Hunting Rifle) | | `[Blender]` if no pack equivalent |
| 12 Gauge (Shotgun) | | `[Blender]` if no pack equivalent |
| .357 (Revolver) | | `[Blender]` if no pack equivalent |

## Magazines (loose, carryable — distinct from a weapon's built-in cosmetic mag prop)

| Item | Notes | Status |
|---|---|---|
| AR Magazine | check whether `SM_AR_Magazine` (already on the gun) can be reused first | `[Blender]`/reuse |
| Pistol Magazine | | `[Blender]` |
| SMG Magazine | | `[Blender]` |
| Revolver/Shotgun | N/A — these two reload by individual round or don't use a detachable box magazine; don't force a magazine item onto them, needs its own reload-mode design question (flagged in Open Questions below) | — |

## Weapon Attachments (one set of 4 slots per `UZSWeaponConfig`: Muzzle/Handguard/Grip/Optic)

| Item | Slot | Status |
|---|---|---|
| Suppressor | Muzzle | `[Blender]` |
| Compensator | Muzzle | `[Blender]` |
| Flashlight (rail-mounted) | Handguard | `[Blender]` |
| Laser sight | Handguard | `[Blender]` |
| Vertical grip | Grip | `[Blender]` |
| Angled grip | Grip | `[Blender]` |
| Red dot sight | Optic | `[Blender]` |
| Basic 4x scope | Optic | `[Blender]` |

## Medical

| Item | Use type | Status |
|---|---|---|
| Bandage (clean) | `Bandage` | `[Done]` |
| Bandage (dirty rag) | `Bandage` | `[Data-only]` — pack has 3 bandage variants already |
| Alcohol / Disinfectant | `Disinfectant` | `[Data-only]` — `SM_Alcholo` already in `FirstAidCabinet` |
| Tincture of Iodine | `Disinfectant`, better tier | `[Data-only]` — already in pack |
| Splint | `Splint` | `[Blender]` — real gap, nothing splint-shaped in the pack |
| Painkillers | `Consumable`, `HealthRestore` | `[Data-only]` — pack has painkiller/aspirin textures, mesh tbc |
| Antibiotics | delays bite/wound infection further than a basic disinfectant — `MedicalIncubationDelayGameHours`, higher tier | `[Blender]` |
| Suture kit | higher-tier `Bandage`, faster bleed stop | `[Blender]` |
| Antiseptic wipes | `Disinfectant`, low tier, common | `[Blender]` |

## Food

| Item | Notes | Status |
|---|---|---|
| Canned Beans | | `[Done]`/`[Data-only]` — `T_CanofBeans` exists, confirm DA |
| Canned Tomatoes / Cucumbers / Pineapple / Dog Food | | `[Data-only]` — all already modeled in `Poly-MegaSurvivalFood` |
| Raw/Cooked/Rotten produce (apple, carrot, cabbage, etc.) | the pack already covers this 3-state decay pattern extensively | `[Data-only]` |
| Bread | | `[Data-only]` — pack has `SM_Bread_Black`; or model a simple loaf if style match matters |
| Crackers | | `[Data-only]` |
| Cheese | | `[Data-only]` |
| Jerky / preserved meat | long-shelf-life, high value for the hunger/decay loop — check pack coverage | `[Blender]` if not covered |
| Generic unlabeled can | reusable base mesh, texture-swap across several canned goods | `[Blender]` |
| Wrapped snack/candy bar | | `[Blender]` |

## Drinks

| Item | Notes | Status |
|---|---|---|
| Water Bottle | core hydration item, high priority despite being "simple" | `[Blender]` |
| Soda / Cola | | `[Data-only]` — pack has bottle variants |
| Purification tablets or a boiling mechanic | design question, not an item per se — see Open Questions | — |
| Alcohol (beverage, distinct from medical alcohol) | | `[Data-only]`/`[Blender]` |
| Coffee | minor `Fatigue`-adjacent flavor item, optional | `[Blender]` |

## Clothing (texture/material swap only — `Head`/`Eyes`/`Mask`/`Shirt`/`Pants`/`Shoes`, no mesh ever, per the stated 2026-08-07 direction)

| Slot | Example variants | Status |
|---|---|---|
| Shirt | plain tee, flannel, jacket (higher `InsulationValue`) | `[Texture-only]` |
| Pants | jeans, cargo pants | `[Texture-only]` |
| Shoes | sneakers, boots (higher `InsulationValue`) | `[Texture-only]` |
| Head | baseball cap, beanie | `[Texture-only]` |
| Eyes | glasses, sunglasses | `[Texture-only]` |
| Mask | bandana, dust mask | `[Texture-only]` |

This whole category is a texture-authoring task (once the base skin mesh's material-slot approach is actually built — still not implemented, per `project_clothing_texture_vs_gear_mesh` — flagged again in Open Questions), not a Blender-modeling one. Listed here for completeness of the full catalog, not as a near-term checklist item.

## Gear (physical worn mesh — `Helmet`/`Vest`/`Belt`/`Backpack`/`Duffle`)

| Item | Notes | Status |
|---|---|---|
| Backpack | | `[Blender]` — in progress |
| Helmet | | `[Blender]` |
| Vest | | `[Blender]` |
| Belt | | `[Blender]` |
| Duffle | shares `Backpack`'s socket (`SocketBack`) — lower priority, same visual slot | `[Blender]`, deferred |
| Backpack (large variant) | second tier, higher `CarryCapacityBonus` — reuse the base backpack's topology, different texture/scale | `[Blender]`, later |

## Tools & Utility

| Item | Notes | Status |
|---|---|---|
| Flashlight | `bIsToggleable`, `SecondaryHand`-eligible | `[Done]`/`[Data-only]` — DA exists, mesh tbc |
| Lockpick | pairs with `OQ-B4-08`'s resolved hybrid breach+lockpicking design | `[Blender]` |
| Battery | consumable for flashlight/future electronics | `[Blender]` |
| Matches / Lighter | fire-starting, likely ties into a future cooking/warmth mechanic | `[Blender]` |
| Rope | generic utility, ties/climbing-adjacent flavor | `[Blender]` |
| Scissors | | `[Data-only]` — already in `FirstAidCabinet` |
| Screwdriver | | `[Blender]` |
| Duct Tape | repair-flavor, generic utility | `[Blender]` |
| Map | investigation-arc adjacent (`B5`), likely a UI item more than a 3D one — flag, don't model yet | — |

## World Containers (loot furniture, reused across many building interiors)

| Item | Notes | Status |
|---|---|---|
| Medical Cabinet | | `[Done]`/`[Data-only]` — `SM_Cabinet`/`SM_Cabinet_door` already in `FirstAidCabinet` |
| Wooden Crate | generic, reusable everywhere | `[Blender]` |
| Footlocker | | `[Blender]` |
| Kitchen Cabinet | food-site variant | `[Blender]` |
| Locker | school/gym/police-site variant | `[Blender]` |
| Dumpster | exterior loot | `[Blender]` |

## Misc / Junk / Flavor Loot

Low-value, no mechanical purpose beyond loot variety and world dressing — keep these genuinely simple, they're volume filler, not priority.

| Item | Status |
|---|---|
| Wallet | `[Blender]` |
| Cigarette pack | `[Blender]` |
| Playing cards | `[Blender]` |
| Empty can/bottle (post-use trash) | `[Blender]` |
| Newspaper/magazine | `[Blender]` |

---

## Open questions this catalog surfaces (not resolved here — flag for the dev/design pass, don't guess)

- **Revolver/Shotgun reload model**: the current magazine system (`UZSMagazineConfig`) assumes a detachable box magazine. A revolver/shotgun's per-round reload is a different mechanic entirely — needs its own design decision before those two weapons can actually be built, not just a content gap.
- **Water purification**: is drinking untreated water ever risky (a PZ-style mechanic), or is all water safe once found? Changes whether "purification tablets" is a real item or not.
- **Clothing-as-texture-swap system**: still not implemented (`project_clothing_texture_vs_gear_mesh` memory) — the whole Clothing category above is blocked on that landing before any of it is real content, independent of how much texture art exists.
- ~~**Cooking/fire mechanic**~~ **Resolved, checked against `Docs/Beta/99_DefinitionOfBetaReady.md`**: Cooking is an explicitly deferred skill, not beta scope. Matches/lighter still have a use (fire-starting for light/warmth, not cooking) but jerky/preserved-meat's "cooked" framing doesn't apply pre-beta — treat those food items as found-already-preserved (canned/dried), not player-cookable.
- **Hammer's dual role** (Tools vs. Melee): one mesh, two possible `DA_ZS_*` uses — fine either way, just a naming/organization call, not a blocker.
