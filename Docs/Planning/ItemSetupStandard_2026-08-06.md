# Item Setup Standard — Plan

> **Status: DRAFT, PAUSED (not abandoned).** Written 2026-08-06 at the dev's request ("start setting up items to pick up and move around in the inventory/equip on the play... standardize a setup for items — icons, meshes, compartment privileges"). Paused the same day once the dev asked for a much larger inventory/equip rework first (11 clothing/gear slots, dedicated fixed-grid compartment WBPs, worn-mesh visuals, a live 3D character preview) — see `C:\Users\aaron\.claude\plans\encapsulated-herding-tide.md`. Everything in this doc is still a valid proposal, just blocked on that larger rework landing underneath it first (the compartment/equip-slot architecture this doc's §3/§4 assumes is about to change shape). §5's `Server_StoreInBag` size-rejection C++ change (written, never compiled) gets folded into that rework's generalized version rather than lost. Resume this doc once the rework above is compiled and stable. See §7 for the compressed list of what actually needs a yes/no before content authoring resumes.
>
> This is a **different** document from `Docs/Planning/InventoryLoadoutEquipping_Plan.md` (2026-07-22) — that one designed the `FZSItemInstance` data-model refactor, which is now built and live (confirmed by re-reading the current source, not from memory). This doc is the next layer up: given that architecture already exists, what's the standardized way to actually *author* a new item so it works correctly end-to-end (world pickup → inventory UI → compartment placement → equip)?
>
> **Resumed 2026-08-12.** The inventory/equip rework this was blocked on shipped and has been PIE-iterated on heavily since (2026-08-06 through 2026-08-11 — see `Docs/SessionHandoff.md`); the compartment/equip-slot architecture §3/§4 assume is stable now. Picking this back up specifically to support **art production**: dev is starting hands-on Blender modeling — low-poly, with detail carried by the UV-mapped texture rather than geometry, a deliberate choice since the game's camera is fixed top-down/isometric and never gets close — via Claude Desktop's own Blender MCP connector (a separate app from the Claude Code session maintaining this doc; this session has no direct Blender or live-Unreal-editor tool access, so treat anything here as plan/tracking, not an executed-and-verified state). §4b (new) sets low-poly/texture conventions for that work; §9 (new) is a running item-production tracker. §5's compartment-size-enforcement change is still unconfirmed-compiled since the original pause — worth a quick check before relying on it, not re-verified as part of this update.

## 1. What already exists (grounded in the current source, not assumed)

**`UZSItemConfig`** (`Source/ZombieShooter/Survival/ZSItemConfig.h`) already has every field a simple item needs: `DisplayName`, `Icon` (`UTexture2D`), `Weight`, `MaxStackSize`, `WorldMesh` (`UStaticMesh`), `Rarity`, `ItemSize` (`Small`/`Medium`/`Large`), plus `bIsEquippable`/`EquipSlot`/`CarryCapacityBonus`/`InsulationValue` for gear, and use-type fields (`HungerRestore`/`ThirstRestore`/bandage-clean/incubation-delay) for consumables/medical. `UZSWeaponConfig` inherits it and adds a large, already-complete set of weapon-specific fields (meshes, sockets, montages, ammo, fire/melee/jamming, aim cone).

**Existing content**: 8 non-weapon item configs (`DA_ZS_ItemConfig_Bandage/CannedFood/Flashlight/PistolPickup`, 4 ammo types), 3 weapon configs (`AssaultRifle`, `Pistol`, `Crowbar`), 1 loot table (`DA_ZS_LootTableConfig_Basic`), plus `DA_Bag.uasset`/`BP_ZS_Bag.uasset` and two test Blueprints already in place to build from: `BP_ZS_WorldItem_Test` and `BP_ZS_Container_Test` (`Content/ZS/Items/`).

**What's already correctly enforced in code** (`UZSInventoryComponent`): a bag can only go in its matching gear slot (`Server_EquipToSlot`); weapons are explicitly excluded from all three compartments (`Server_StoreInBag` rejects any `UZSWeaponConfig` instance) and live in separate weapon-mount slots instead, gated by `Handedness`/`AttackType`; mount slots reject an already-equipped instance. This is solid and shouldn't need to change.

## 2. The actual gap — two things, not one

**A) No standardized authoring convention exists.** No icon texture has ever been imported into this project (`Content/ZS/**/*Icon*` returns nothing). No "how to add a new item" doc exists anywhere in `Docs/` — the only convention that exists at all is a code comment ("new item = new `DA_ZS_ItemConfig_<Name>` instance"). Every item so far was set up ad hoc.

**B) "Compartment privileges" don't exist yet as an enforced rule — only as inert data.** `ItemSize` is a real field on every item, but nothing in the codebase reads it. Confirmed by reading `UZSCompartmentPanelWidget::RefreshCompartment()` and `UZSInventoryComponent::Server_StoreInBag`/`GetSlotsInLocation` in full: a `Large`-tagged item can be placed in `OnPerson` (Pockets) today with zero pushback. The design intent — Pockets = Small only, Backpack = Small+Medium, Duffle = everything — is recorded in `Docs/Planning/B1_UIDesignSession_2026-07-30.md:28-44`, but the gating check itself was explicitly deferred (`ZSItemConfig.h:57-65` says so directly) and never built. **If you want "compartment privileges" to actually mean anything in play, this needs a small C++ change — it's not a content-only task.** See §5.

## 3. Item archetypes — the standardized field checklist per type

Every archetype shares the same base checklist (always fill these 6): `DisplayName`, `Icon`, `WorldMesh`, `Weight`, `Rarity`, `ItemSize`. Everything below that is archetype-specific.

| Archetype | Extra required fields | Example(s) already in the project |
|---|---|---|
| **Consumable** (food/drink) | `ItemUseType=Consumable`, `HungerRestore`/`ThirstRestore`, `MaxStackSize>1` | `DA_ZS_ItemConfig_CannedFood` |
| **Medical** | `ItemUseType=Bandage/Disinfectant/Splint`, `bIsCleanBandage` (if Bandage), `MedicalIncubationDelayGameHours` | `DA_ZS_ItemConfig_Bandage` |
| **Ammo** | `MaxStackSize` in the hundreds, referenced by a weapon's `AmmoItemConfig` | the 4 `DA_ZS_ItemConfig_Ammo_*` assets |
| **Equippable gear (bag)** | `bIsEquippable=true`, `EquipSlot=Back/Duffle`, `CarryCapacityBonus` | `DA_Bag` |
| **Equippable gear (clothing/insulation)** | `bIsEquippable=true`, `InsulationValue` | none yet — first one will set the pattern |
| **Toggleable tool** | `bIsToggleable=true` | `DA_ZS_ItemConfig_Flashlight` (per `Docs/TuningReference.md:219`, worth re-confirming this flag is actually set) |
| **Melee weapon** | `UZSWeaponConfig`, `AttackType=Melee`, `MeleeDamage`/`MeleeRange`/`MeleeAttackInterval`, `MeleeMontage`, `Handedness` | `DA_ZS_WeaponConfig_Crowbar` |
| **Ranged weapon** | `UZSWeaponConfig`, `AttackType=Ranged`, `FireDamage`/`FireRange`, `AmmoItemConfig`, `MagazineCapacity`, `Handedness`, aim-cone fields | `DA_ZS_WeaponConfig_AssaultRifle`, `_Pistol` |
| **Misc/junk** | just the base 6 — no use type, `bIsEquippable=false` | none yet |

Note: `ItemSize` is meaningless for weapons today (they never enter a compartment at all) — don't spend time tuning it on `UZSWeaponConfig` instances.

## 4. Proposed content conventions

- **Icons**: new folder `Content/ZS/UI/Icons/`, named `T_Icon_<ItemName>` (matches the project's existing `T_`/`DA_`/`BP_`/`WBP_` prefix convention). Suggest 128×128, sRGB enabled, mip-mapping off (UI textures don't need mips) — open to your preference, flagged in §7.
- **World meshes**: no folder convention exists yet for pickup meshes specifically — worth deciding whether these live under `Content/ZS/Items/Meshes/` or reuse whatever source pack a given item's mesh came from (e.g. `Content/Mega_Survival_Tools/` already has candidate meshes per `GameDevPlan.md`'s asset table). Flagged in §7 rather than guessed.
- **Data assets**: already established, just formalizing it — `DA_ZS_ItemConfig_<Name>` / `DA_ZS_WeaponConfig_<Name>`, one instance per content item, never a new C++ branch.
- **`ItemSize` rubric** (proposed, matches the Pockets/Backpack/Duffle design intent): `Small` = fits in a pocket (ammo, meds, food, small tools, flashlight); `Medium` = backpack-only (mid-size clothing, larger tools); `Large` = duffle-only (bulky gear). Needs your sign-off since no cutoff has ever been assigned to a real item before.

## 5. Compartment-privilege enforcement — built, awaiting your compile

Decided 2026-08-06 (§7): build it. Mapped every call site where `FZSItemInstance::Location` actually changes (`ZSInventoryComponent.cpp`/`ZSLootTableConfig.cpp`) before writing anything:
- `Server_AddItem`/`Server_AddItemInstance` (fresh pickups/grants) → always `OnPerson`, unconditionally, unchanged.
- `Server_RetrieveFromBag` (bag → general carry) → always `OnPerson`, unconditionally, unchanged.
- `UZSLootTableConfig::RollLoot` → always `World`, unrelated to compartments, unchanged.
- **`Server_StoreInBag` (general carry → a specific bag) → this is the one real "place an item into a specific compartment" verb, and the only place gated.**

**Why Pockets/`OnPerson` isn't gated at all**: it's the always-available, no-bag-required fallback (`EZSCarryLocation::OnPerson`'s own comment) — a player doesn't "choose" to keep something in their pockets the way they choose to stash it in a bag, so gating it would mean either rejecting pickups outright (harsh before you've found your first bag) or inventing fallback-routing logic nobody asked for. The one real gate is Backpack vs. Duffle, which is exactly what `Server_StoreInBag` already handles.

**Implemented**: `Server_StoreInBag` now rejects storing a `Large`-sized item into a Back-slotted bag; Duffle accepts everything (matches `EZSEquipSlot::Duffle`'s own "largest capacity bonus" comment). `ZSInventoryComponent.cpp`/`.h` and `ZSItemConfig.h`'s doc comments updated to match. **Not yet compiled** — editor was open when this was written, so no `Build.bat` attempt was made (would just fail on the Live Coding lock). This is a safe Ctrl+Alt+F11 Live Coding change (only doc-comment header edits, no new reflected members) whenever you're ready.

## 6. Standardized "add a new item" checklist

1. Pick the archetype (§3), confirm which fields are required.
2. Create `DA_ZS_ItemConfig_<Name>` (or `DA_ZS_WeaponConfig_<Name>`) in `Content/ZS/Items/` (or `Content/ZS/Weapons/<Category>/` for weapons, matching existing folders).
3. Fill the base 6 + archetype-specific fields.
4. Import/assign `Icon` per §4's convention.
5. Assign `WorldMesh`.
6. Set `ItemSize` per the §4 rubric.
7. Set `Rarity` — decide if it should be gated through `AZSGameState::RarityPoolEntries` (Rare/VeryRare only).
8. If lootable, add an entry to a `UZSLootTableConfig` (`Weight`/`MinCount`/`MaxCount`).
9. For direct pickup testing: place a `BP_ZS_WorldItem_Test`-style actor (or a thin per-item BP wrapper) in a test level, or drop it into a `BP_ZS_Container_Test`'s loot table.
10. PIE test: pick up → confirm icon/stack/condition tint render correctly in the inventory grid → move between compartments (if non-weapon) → equip (if applicable).

## 7. Decisions — 2026-08-06

1. **Compartment-size enforcement: build it.** Confirmed — "items will have a standard size, and the components in the inventory UI will have to match that... restricted by which component they are allowed to get put into." **My interpretation, flagged so it's easy to correct if wrong**: implementing this as the *hierarchical* model already recorded in `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (Pockets accepts Small only; Backpack accepts Small+Medium; Duffle accepts all three) rather than a strict per-compartment exact-size-match. Hierarchical means a Small item can go in any compartment, a Large item only in Duffle — matches "common sense" (§21 of your own dev markup notes: a tiny bandage isn't blocked from a big duffle bag). If you actually meant strict exact-match-only (a Small item can't go in Duffle either), say so before I build it — cheap to build either way, but the wrong one means redoing it.
2. **Icons: not mine to build.** You're transferring an icon-generator system from ShooterGame. I won't create placeholder icons for the items below — `Icon` stays unset (same graceful-if-missing pattern as every other not-yet-authored content reference in this project) until you point it at real generated icons. **One thing I do need from you when it's transferred**: what output convention does that generator produce (resolution, file format, naming)? I'll fold it into this doc once known, rather than dictating my own.
3. **World meshes: dedicated `Content/ZS/` folders, organized by item type.** See §4a below for the concrete proposal.
4. **Priority order: confirmed** — Bandage → CannedFood → Ammo → Bag → Weapons.
5. **Test setup**: defaulting to reusing the existing `BP_ZS_WorldItem_Test`/`BP_ZS_Container_Test` Blueprints for pickup verification (matches "don't build new abstractions beyond what's needed") — say so if you'd rather have a dedicated graybox test level instead.

## 4a. Proposed folder reorganization

Grounded in what's actually sitting in the raw FAB packs right now (not guessed): `Content/Mega_Survival_Tools/` (273 assets — tools, batteries, containers, chemicals, junk), `Content/Poly-MegaSurvivalFood/` (241 assets — produce, canned goods, drinks, cooked/raw/rotten meat variants), `Content/LowPolyWeapons/` (914 assets — this one's already fully in use via `Content/ZS/Weapons/`, no reorg needed there).

**Precedent already established, worth matching rather than inventing something new**: `Content/ZS/Weapons/<Category>/` is flat — one folder per weapon category, mesh + its own materials + its `DA_ZS_WeaponConfig_*` all living together in that one folder (e.g. `Content/ZS/Weapons/AssaultRifle/` holds the mesh *and* `DA_ZS_WeaponConfig_AssaultRifle`). No separate top-level `Meshes/`/`Materials/`/`DataAssets/` split exists anywhere in this project.

**Proposed, matching that same pattern:**

```
Content/ZS/Items/
  Food/        <- meshes + materials moved from Poly-MegaSurvivalFood, + DA_ZS_ItemConfig_CannedFood (moves here from flat Items/)
  Medical/     <- + DA_ZS_ItemConfig_Bandage (moves here)
  Tools/       <- meshes moved from Mega_Survival_Tools, + DA_ZS_ItemConfig_Flashlight (moves here)
  Containers/  <- bag/backpack/duffle meshes, + DA_Bag (moves here)
  Misc/        <- junk/crafting-fodder meshes, no items authored yet

Content/ZS/Weapons/    <- unchanged, already correct
  Ammo/ AssaultRifle/ Melee/ Pistol/
```

Only move what's actually being kept (per your plan to clean up unused packs later) — no need to import all 273/241 source assets, just the ones a real `DA_ZS_ItemConfig_*` ends up referencing. In-editor, this needs the Content Browser's own "Move" (not a raw file copy — `.uasset` internal references need redirector fixup, which the Move tool handles automatically) or `Fix Up Redirectors` afterward if moved via the OS filesystem. This is editor-only work — I can't perform it without `unreal-mcp` connected, so it's on you to execute when you're back at the keyboard, using the target structure above as the map.

## 8. What I'm explicitly not touching

No changes to the `FZSItemInstance`/equip/mount architecture — that's settled and working, per `InventoryLoadoutEquipping_Plan.md`'s now-mostly-resolved status. This plan is purely: authoring convention + (if approved) the one missing enforcement rule.

---

## 4b. Blender low-poly pipeline conventions (added 2026-08-12)

Dev's direction: model low-poly, put detail in the UV-mapped texture rather than geometry. Correct call for a fixed top-down/isometric camera that never gets close, and it cuts both modeling time and runtime cost. **Nothing below is enforced by code or measured yet** — starting points only, same "propose, don't guess-and-lock" spirit as the rest of this doc. Revisit after the first few real items go through the pipeline once, and again if B2-T3 (still parked, per `Docs/Beta/B2_ArtPipeline.md`) ever formally sets project-wide LOD/material budgets that should supersede these.

**Triangle budget, by class:**

| Class | Budget | Examples |
|---|---|---|
| Small handheld/pickup | 100–400 tris | food, meds, ammo boxes, small tools |
| Worn gear, small | 300–800 tris | helmet, belt |
| Worn gear, large | 500–1200 tris | backpack, duffle, vest |
| Weapons | 600–1500 tris | roughly matches the density of the already-owned LowPolyWeapons pack parts |

**Texture/material convention — one shared master material + per-item instance, not an atlas:**
- One master material (e.g. `M_ZS_Item`, doesn't exist yet) with a `MI_<ItemName>` instance per item, per B2-T3.2's already-decided "master materials + instances only" rule. This keeps material count (the real draw-call lever) flat no matter how many items exist, without the ongoing overhead of packing/repacking a shared atlas every time an item is added — a category atlas was considered and rejected here specifically because repacking is *more* authoring friction over time, not less, which cuts against the "reduce time spent" goal.
- **BaseColor texture only**, by default — the UV/texture is meant to carry the detail, but that doesn't mean a full PBR texture set. Skip Normal maps entirely unless one specific item actually needs the extra read at gameplay zoom; low-poly + a camera that's always far away means they're very unlikely to earn their cost. Roughness/Metallic as flat scalar parameters on the Material Instance, not texture maps, unless an item genuinely mixes materials (e.g. a metal-buckled cloth backpack) — call that per item, not by rule.
- Texture size: 256×256 default, 128×128 for small/simple clutter (ammo, food). Power-of-two, matches UE compression expectations.
- Naming matches what's already sitting in the project (`Content/FirstAidCabinet/Textures/`'s own convention, already precedent, not invented here): mesh `SM_<ItemName>`, texture `T_<ItemName>_BaseColor`, material instance `MI_<ItemName>`.

**Blender → UE export checklist:**
1. Apply all transforms before export (`Ctrl+A` → All Transforms) — an un-applied rotation/scale is the most common cause of a mesh importing rotated or at the wrong size.
2. Origin placement matters most for `WornMesh` items — they attach straight to a character socket with no manual per-item offset step in code (`AZSPlayerCharacter::RefreshWornMeshes` / `GetSocketForEquipSlot`), so the mesh's own origin needs to already sit where it should relative to that socket. `WorldMesh` (ground pickup) origin is less critical, just needs to look right sitting on the floor.
3. FBX export: Blender's default Forward/Up (`-Z Forward`, `Y Up`) is the likely-correct starting point, **unverified** — `Docs/BlenderNotes.md` §3 (added 2026-08-12) is the canonical source going forward, including the exact empirical test to confirm scale/axis before trusting any exported asset dimensionally. Update this line to match once that test's actually run.
4. Collision: don't spend Blender time hand-building collision hulls for `WornMesh` or weapon-attachment meshes. Confirmed by reading the source (`ZSPlayerCharacter.cpp`): every `WornMeshComponents` entry gets `SetCollisionEnabled(ECollisionEnabled::NoCollision)` at creation, unconditionally, regardless of what the imported mesh carries — same rule `AZSWeapon::AssignNewStaticMesh` already applies to weapon attachments, per `CLAUDE.md`'s "Cosmetic attachments must be `NoCollision`" convention. `WorldMesh` pickups can keep simple auto-generated collision (or none) — interaction is a separate sphere-overlap component (`UZSInteractableComponent`), not mesh collision.
5. Import into the folder matching §4a's layout above.

---

## 9. Item production tracker (added 2026-08-12)

Category-level, not per-mesh — the source packs alone are 1,400+ files, a per-asset table isn't useful. Update the Status/Gap columns as categories move through the pipeline. "DA" = `DA_ZS_ItemConfig_*`/`DA_ZS_WeaponConfig_*` data asset actually authored and wired up, not just "a mesh exists somewhere."

| Category | Assets on hand | Real gap | Blender need |
|---|---|---|---|
| Medical | `Content/FirstAidCabinet/` pack, 128 assets — bandages ×3, alcohol, spray, medicine box, scissors, syringe, iodine, plus aspirin/multivitamin/painkiller textures | Only Bandage has a DA; Disinfectant/Splint/Painkiller (`HealthRestore`) archetypes don't yet, despite the meshes/textures already existing | Low — mostly a DA-authoring pass over an already-owned pack |
| Food | `Content/Poly-MegaSurvivalFood/` pack, 242 assets — raw/cooked/rotten produce, canned goods, drinks, burger/pizza/sandwich modular kits, bread/cheese/crackers | Only CannedFood has a DA — a tiny fraction of a large pack is turned into real items | Very low — same as Medical, DA-authoring not modeling |
| Ammo | 1 generic DA (`DA_ZS_ItemConfig_Ammo_Other`) exists | **Magazines are a real gap**: `UZSMagazineConfig` (the 2026-08-11 per-instance-magazine rework, see `CLAUDE.md`) needs its own loose, pickable-up `WorldMesh` distinct from a weapon's cosmetic `MagazineMesh` prop — no `DA_ZS_MagazineConfig_*` exists yet, flagged in `CLAUDE.md` itself as a content gap | Possible — check `Content/Mega_Survival_Tools/` (273 assets: tools/batteries/containers/chemicals per this doc's §4a) for a usable magazine/box prop before modeling one |
| Weapons — ranged | `LowPolyWeapons` pack, 914 assets, already fully in use — AssaultRifle alone has 900+ mesh parts under `Content/ZS/Items/Weapons/Meshes/` | AssaultRifle + Pistol DAs exist | Very low — assemble from the pack; only model new if you want a specific gun the pack doesn't have |
| Weapons — melee | Same pack | Crowbar DA exists | Very low, same reasoning |
| Weapon attachments (Muzzle/Handguard/Grip/Optic) | Coverage from the LowPolyWeapons pack unconfirmed this pass | No attachment DAs/content authored yet | Check the pack first |
| **Gear — worn** (Helmet/Vest/Belt/Backpack/Duffle) | `DA_Bag` exists, but its mesh sourcing and its `EquipSlot` enum value are both still flagged unconfirmed since the `Back`→`Backpack` rename (§ above, "Real content risk" in `Docs/SessionHandoff.md`) | Helmet/Vest/Belt have no DA at all | **Best first Blender target.** These need real volumetric bulk (a backpack's actual shape, a helmet's silhouette) that the food/medical packs can't supply — see [[project_clothing_texture_vs_gear_mesh]] — and they're worn on-screen continuously, so hand-modeling time pays off more here than on a can of beans |
| Clothing (Head/Eyes/Mask/Shirt/Pants/Shoes) | N/A | N/A | **None at all.** Stated future direction is a texture/material swap on the base skin mesh, not a separate attached mesh ([[project_clothing_texture_vs_gear_mesh]]) — don't spend Blender time here |
| World containers (crates/cabinets) | FirstAidCabinet pack includes a full cabinet (`SM_Cabinet`/`SM_Cabinet_door`) | `BP_ZS_Container_Test` is the only container BP so far — no per-archetype container (e.g. a medical cabinet) built yet | Low — the cabinet mesh alone could back a first real `BP_ZS_Container_Medical` |
| Tools/misc/junk | `Content/Mega_Survival_Tools/`, 273 assets — not inventoried in detail this pass | Flashlight DA exists; rest unauthored | Unknown until the pack's actually surveyed |

**Good first Blender targets, in order:** worn gear (backpack → helmet → vest → belt) — least existing coverage, most on-screen visibility, and the one category the food/medical packs structurally can't fill in for.

**Active as of 2026-08-12: Backpack.** Full spec (socket, budget, texture, scale reference, destination folder) is in `Docs/BlenderNotes.md` §7 — that file is the one to keep current as this moves through modeling, not this tracker's per-row text.

### Baseline Blender checklist — "get some gameplay going" (added 2026-08-12)

Scoped narrower than the tracker table above: only real, confirmed gaps that are actually Blender-appropriate static props (not data-authoring tasks, not things a pack already covers, not full building/environment content — that's blocked on a B2 kit decision, a sourcing task not a modeling one). Check items off in place; don't append a duplicate list below when one's done.

**Gear (proves the equip-visual loop end to end — nothing worn exists yet):**
- [ ] Backpack — **in progress**, spec in `Docs/BlenderNotes.md` §7
- [ ] Helmet
- [ ] Vest
- [ ] Belt
- Duffle deliberately not listed — shares `Backpack`'s socket (`SocketBack`), low value to model a second item for the same slot this early

**Weapons (mechanical blocker, not just cosmetic — neither existing gun can currently reload at all, per `Docs/TuningReference.md`'s Per-Weapon Config section, no `DA_ZS_MagazineConfig_*` exists yet):**
- [ ] AR Magazine (loose, carryable `WorldMesh`) — **check first** whether `SM_AR_Magazine` (the cosmetic mesh already on the gun) can just be reused before modeling a new one
- [ ] Pistol Magazine (loose, carryable `WorldMesh`) — same check first; Pistol's own mesh sourcing was never confirmed this pass

**Optional / lower priority — don't block on these:**
- [ ] Projectile/bullet mesh — purely cosmetic (`ProjectileMesh` currently falls back to an engine placeholder Sphere), trivial geometry, decent quick pipeline-test if wanted
- [ ] Zombie mesh + skin — **the single biggest content gap overall** (confirmed still true: animations imported, no mesh ever sourced, `BP_Zombie_*`/`DeathZombieClass`/`StressTestZombieClass` all still unset per `Docs/TuningReference.md`). Listed last specifically because it's a **different pipeline** than everything else here — a rigged/skinned character plus an animation retarget onto (or matching) `SKEL_TFA_Mannequin`, not a static low-poly prop. Probably better sourced from a pack (matches B2's "mostly free/cheap" direction) than hand-modeled from scratch — include here only if the dev specifically wants to build one.

**Expanded 2026-08-12** at dev request to also include Food/Medical, World Containers, and Weapon Attachments — these were originally left off as "pack already covers it," but the dev wants some of these hand-modeled too (style consistency with everything else in this pipeline, and reps on the process), not just wired up from the source packs.

**Food (keep genuinely simple — single/near-single-mesh items, not the pack's modular multi-part kits like the burger/pizza/sandwich assemblies, which don't need remaking):**
- [ ] Generic unlabeled can — reusable across several canned-goods items via texture swap alone, good first example of the UV-does-the-work philosophy paying off
- [ ] Water bottle
- [ ] Wrapped snack/candy bar
- [ ] Bread loaf

**Medical (simple; several of these already exist in `Content/FirstAidCabinet/` — model your own only if you want it style-matched to everything else, otherwise just reuse the pack mesh and skip):**
- [ ] Splint — genuinely not covered by the pack (nothing splint-shaped in the FirstAidCabinet inventory), real gap
- [ ] Pill bottle (painkillers — `HealthRestore` archetype has no DA yet either, pairs with this)
- [ ] Antiseptic wipes packet — small, flat, easy

**World containers (simple loot props, generic enough to reuse across many building types):**
- [ ] Wooden crate
- [ ] Footlocker

**Weapon attachments (one simple pass per `UZSWeaponConfig` attachment slot):**
- [ ] Muzzle (suppressor or compensator)
- [ ] Handguard-mounted flashlight
- [ ] Grip
- [ ] Optic (red dot or basic scope)

**The full, longer-horizon item list for the whole game** (not just this near-term Blender queue) is now tracked separately in `Docs/Planning/ItemCatalog_2026-08-12.md` — this section stays scoped to "what's actually next," that doc is the fuller picture.
