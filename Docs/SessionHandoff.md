# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-22, overnight/unsupervised follow-up #11) — two new planning documents, NOT code

You asked me to "thoroughly plan the inventory, loadout, and player equipping items, tie everything together, setup modularity for weapons," then plan UI, and to keep these plans in **separate files from the rest of the project** for your review later. Done - this was a planning pass only, no source files were touched, nothing was compiled, no `unreal-mcp`/editor use.

- **`Docs/Planning/InventoryLoadoutEquipping_Plan.md`** (new folder, new file): audits the actual gap between P5's loadout and P6's inventory (they don't reference each other - `HotbarSlots` still holds bare `UZSWeaponConfig*`, not anything tied to `CarrySlots`), identifies that **ammo isn't integrated with the inventory at all** (a gap nobody had flagged before this pass - `AZSWeapon::CurrentReserveAmmo` is a fake, non-lootable counter), and proposes a unified "item instance" model (`FZSItemInstance` with a stable `FGuid`, replacing the bare config-pointer-and-count approach everywhere) to tie loadout/inventory/equipping together under one data model. Also covers weapon "modularity": recommends a small, cheap Tier 1 (handedness/secondary-hand-legality fields - unblocks the already-written-down amputation-restricts-weapons rule) and flags a deeper Tier 2 attachment system as **optional, recommended against for now** (PZ's own attachments are shallow, nothing in any design doc asks for deep weapon customization, and it doesn't fit the project's stated "~1/3 of PZ's depth" pillar). Ends with a 7-step migration order and 6 explicit open questions for you.
- **`Docs/Planning/UI_Plan.md`** (new file): resolves the standing "left-click needs to both attack and select menu options" question from the P5 design pass with a concrete input-mode-switching architecture (a `IMC_ZS_UI` context swap, gated by a small modal-widget stack) - this blocks every other screen, so it's positioned as the literal first UI task. Inventories which HUD data is already broadcasting (most of it - Needs/Health/Interaction/Hotbar delegates all already exist and are unused) vs. what's still missing (hotbar switch-progress, container-contents delegate + per-item transfer, a stat-preview text helper, sleep-ready count, death-cause capture). Screen-by-screen plan for HUD/Inventory/Container-loot/Death/Sleep/Radial-quick-use, each with a recommendation grounded in your own `DevMarkupNotes.md` §21 note and `ProjectZomboid_DesignReference.md`'s explicit warning that PZ's own dual-pane inventory is "the most-modded, most-complained-about surface in the game" (recommending a single scrollable list instead). Ends with a build order and 5 open questions.
- **Both files are explicitly marked DRAFT PROPOSAL, not plan of record** - I did not merge anything into `GameDevPlan.md` or the `Docs/Phases/` files. Read them when you're back; each has a compressed "Open Questions" section at the end rather than burying decisions in prose.

## Carried forward from the previous round (2026-07-21, follow-up #10) - still current, nothing new done to it

**P5 finished (melee stats/durability/knockback) and P6 built from scratch (inventory/loot) - still uncompiled, still not PIE-tested.** Nothing changed about this status during tonight's planning pass; it's exactly where it was. Full detail was already written up last round in this file's prior version (see git history / commit `65f53e4`) and in `Docs/Phases/P5_CombatCompletion.md` / `Docs/Phases/P6_InventoryLoot.md` directly - not re-duplicating it here.

**Before touching any of that in PIE:**
1. **Compile.** New `UPROPERTY`s on `AZSPlayerCharacter`/`AZSWeapon`/`AZSGameState`/`UZSItemConfig`, plus new UCLASSes (`UZSInventoryComponent`, `AZSWorldItemActor`, `AZSContainerActor`, `UZSLootTableConfig`). Use Rider/VS or `Build.bat` (PowerShell), not Ctrl+Alt+F11.
2. **"Compile All Blueprints" pass** after, before trusting PIE - standing practice, see `CLAUDE.md`'s Live Coding lesson.
3. Create `IA_HotbarSelect`/`IA_HotbarCycle`, re-author `BP_ZS_PlayerCharacter`'s `StartingHotbarLoadout` array (the old `StartingWeaponConfig` field is gone).
4. P6 has zero content authored yet - no item/loot-table data assets, no container Blueprints, no pickup meshes.
5. **Review the two autonomous P6 design calls** (bag-slot depth, rarity-pool model) and the durability "breaking clears the hotbar slot" interpretation - all flagged as cheap-to-change-now, less cheap later.

**Also carried forward, unrelated to P5/P6:**
- First full P3+P4 integration proof, PIE-confirmed (2026-07-21, follow-up #7): zombie chase → melee → damage → infection roll all worked end-to-end. Your character from that test is still `Incubating` on a Torso wound.
- **Known gap, flagged not fixed, deprioritized three times now**: `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso unconditionally - amputation's infection-clearing path (Arms/Legs only) is unreachable from a real bite. See `Docs/Phases/P3_HealthDamageMedical.md`.

## Testing checklist (2026-07-22) — do this before the next session

This is the consolidated list of everything that needs your hands (compiling, in-editor content, PIE) before we can move forward. Nothing here can be done unsupervised, which is why it queued up across two overnight rounds. Roughly in dependency order — each numbered group assumes the one before it passed.

### 0. Setup (blocking everything else)
- [ ] **Compile** `ZombieShooterEditor Win64 Development` via Rider/VS or `Build.bat` (PowerShell, not Bash, not Ctrl+Alt+F11 - this round touched headers on `AZSPlayerCharacter`/`AZSWeapon`/`AZSGameState`/`UZSItemConfig` plus brand-new UCLASSes, exactly the kind of round the Live Coding lesson warns about).
- [ ] **Compile All Blueprints** (Content Browser bulk action) after a clean compile, before trusting anything below in PIE.
- [ ] Create `IA_HotbarSelect` (Axis1D; map `Digit1`-`Digit9` in `IMC_ZS_Default`, each with a Scalar modifier of 1-9) and `IA_HotbarCycle` (Axis1D; mouse wheel) - both referenced by name already, both currently missing as assets.
- [ ] Re-author `BP_ZS_PlayerCharacter`'s `StartingHotbarLoadout` (new array field - the old single `StartingWeaponConfig` field is gone). Put **at least two different weapon configs** in it so switching/cycling is actually testable, not just a one-slot no-op.

### 1. P5 — Loadout & combat
- [ ] **Start unarmed**: spawn in PIE, confirm no weapon and the bare-fist body mesh (not a leftover gun-holding mesh).
- [ ] **Number-key equip**: press a hotbar number with a weapon in it, confirm a visible delay (`EquipTimeSeconds`) before the weapon actually appears/attaches - it shouldn't be instant.
- [ ] **Unequip toggle**: press the *same* number again, confirm it un-equips back to bare-fist (with `UnequipTimeSeconds`'s delay), not a re-equip.
- [ ] **Scroll cycling**: `IA_HotbarCycle` moves between your authored slots, skipping empty ones, wrapping around at the ends.
- [ ] **Attack dispatch per weapon type**: `IA_Attack` fires the gun when a `Ranged` weapon's equipped (unchanged from before), swings bare-fist when nothing's equipped (unchanged), and - **this is new** - uses the weapon's own `MeleeDamage`/`MeleeRange`/etc. when a `Melee`-typed weapon is equipped. No melee weapon config has been authored yet (see `Docs/Phases/P5_CombatCompletion.md`), so to test this branch at all you'll need to temporarily flip an existing config's `AttackType` to `Melee`, or author a placeholder one.
- [ ] **Durability**: with `MaxDurabilityHits` set on a test weapon, land enough melee hits to break it - confirm it auto-unequips, and confirm re-selecting that same hotbar slot does **not** bring it back (the slot should now be empty, not silently re-spawn a fresh weapon).
- [ ] **Knockback**: confirm a melee/ranged hit visibly shoves the target (zombie) backward.
- [ ] **Busy-gating**: try attacking or switching hotbar slots while a switch is already in progress - both should be no-ops until the current switch finishes.

### 2. P6 — Inventory & loot (needs content first - none exists yet)
- [ ] Author at least one `DA_ZS_ItemConfig_*` (any simple carry item is enough to start), one `DA_ZS_LootTableConfig_*` referencing it, and place one `AZSContainerActor` + one `AZSWorldItemActor` (or a dropped item) in your test level.
- [ ] **World pickup**: interact with a placed/dropped `AZSWorldItemActor`, confirm the item lands in your carry list and the actor disappears.
- [ ] **Drop**: drop an item from your inventory, confirm a new `AZSWorldItemActor` spawns in front of you and the item leaves your carry list.
- [ ] **Container loot-all**: interact with a container, confirm everything in it transfers to you and it stops being interactable once empty.
- [ ] **Encumbrance**: carry enough weight to exceed capacity, confirm movement visibly slows.
- [ ] **Equip slots**: equip a `bIsEquippable` item into `Back` or `Hip`, confirm its `CarryCapacityBonus` actually raises your max carry weight.
- [ ] **Rarity pool** (optional, only if you set up a `Rare`/`VeryRare` item with a `RarityPoolEntries` budget on `AZSGameState`): loot it enough times to exhaust the pool, confirm it stops being granted afterward.

### 3. Carried forward from earlier rounds
- [ ] Continue P3 testing on your `Incubating` character - treatment items (`Server_UseItem` paths: bandage/disinfect/splint) and/or letting the infection clock progress toward `Queasy`.
- [ ] Zombie bite zone-targeting gap is still open (every bite lands on Torso) - not something to "test," just a known reproduction if you want to re-confirm it before it's fixed.

### Not a test - a reading task
- [ ] Read `Docs/Planning/InventoryLoadoutEquipping_Plan.md` and `Docs/Planning/UI_Plan.md`, answer their **Open Questions** sections. The item-instance refactor proposed there gets more expensive to do the more P6 content gets authored against today's shape - worth deciding before you sink much time into §2 above.

## Other still-open items (lower priority, no action needed yet)
- `BT_Zombie`'s wander branch has zero children; `BP_ZombieAIController`'s fate (unused) undecided; crouch pose bug untouched; temporary hit-confirmation logging still needs removing once real impact feedback exists.
