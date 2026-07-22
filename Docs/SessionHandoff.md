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

## Next step

1. **Read the two new planning docs** (`Docs/Planning/`) and answer their open-questions sections - both are written so you can skim straight to "Open Questions" at the end of each if you don't want the full reasoning first.
2. **Decide whether to greenlight the item-instance refactor** (`InventoryLoadoutEquipping_Plan.md` §5/§8) before any more P6 content gets authored against the current `FZSInventorySlot` shape - the longer content gets built on the current model, the more there is to migrate later.
3. **Compile last round's P5/P6 code** (see above) - still the actual blocker on any hands-on testing.
4. **Or** continue P3 testing on your `Incubating` character (treatment items, infection progression toward `Queasy`).
5. **Or** fix the zombie bite zone-targeting gap.
6. Carried forward, still open: `BT_Zombie`'s wander branch has zero children; `BP_ZombieAIController`'s fate (unused); crouch pose bug; remove temporary hit-confirmation logging once real impact feedback exists.
7. **Standing practice**: "Compile All Blueprints" after any Live Coding patch, before trusting PIE.
