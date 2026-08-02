# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1 progress — 2026-08-01

**Every B1 task group (T1-T8) has its C++ groundwork done, compiled, and automation-tested.** T1/T2/T5.0 closed out earlier this session; T3/T5 landed (`e064bc3`); T6/T7/T8 followed (`00d3633`). **Then, while widget-building was already underway, the dev revised the HUD/hotbar design** (`6d413ac`) - documented below since it changes what several T3/T5 widgets actually are.

**HUD redesign, 2026-08-01 (dev-confirmed, commit `6d413ac`):**
- **No moodle stack on the HUD** - relocated to T5's Loadout panel (`WBP_ZS_MoodleStack` is now a Needs-bars widget inside the Inventory screen, not a HUD element). Matches the design session's "Needs live in the Loadout panel" note taken literally.
- **No 9-slot hotbar grid.** Replaced with a single equipped-item icon (bottom-right). Weapons are key-mapped directly to the 3 mount slots - **1 = Primary, 2 = Pistol, 3 = Secondary** - so mounting a weapon is now the *only* "assignment" step; the old free-form `HotbarSlots` array and its `Server_AssignHotbarSlot`/`Server_ClearHotbarSlot`/`CanAssignToHotbarSlot` API (all built last session) are removed entirely. `AZSPlayerCharacter::ResolveWeaponSlotInstance(SlotIndex)` resolves each key live from `UZSInventoryComponent`'s mount accessors instead.
- **New Equipment slot** (4th key, **G**) for grenades/quick-use equipment - `EquipmentSlotInstanceId`/`Server_AssignEquipmentSlot`/`Server_ClearEquipmentSlot`/`OnEquipmentSlotChanged`, mirrors `SecondaryHandInstanceId`'s pattern. Scoped to `UZSWeaponConfig` instances only this pass (flagged in code + `B1_UI_UX.md` - a non-weapon "other equipment" item isn't dispatchable through it yet, since the equip path only knows how to equip a weapon config).
- **No ammo counter, no scoreboard.** Cut outright, not deferred - the underlying C++ (`OnMagazineAmmoChanged`, `OnPlayerListChanged`/`PlayerListVersion`) stays in place unused, in case either is wanted later.
- All the proven equip-timing machinery (`Server_SelectHotbarSlot`/`CompleteHotbarSwitch`/`bIsBusy` gating/durability writeback) is reused unchanged, just re-pointed at the new resolver - nothing about *how* equipping works changed, only *what* selects what.
- **Required manual step, not yet done**: create `IA_EquipItem` (G key) - the C++ finder is already in place, graceful-if-missing like every other Input Action here.
- 2 new automation tests (`ZS.Loadout.WeaponKeySlotsResolveFromMounts`, `ZS.Loadout.EquipmentSlotRequiresWeaponConfig`) replace the removed hotbar-assignment test. Full rebuild clean, `ZS.` suite (32 tests) clean - only the same 5 pre-existing failures remain.

**T6/T7/T8 — commit `00d3633` (unchanged by the redesign):**
- **T6 (container loot)**: `AZSContainerActor::Server_TakeItem` (GUID-exact per-item take, dupe-safe by construction), `Server_TakeAllItems`, `Server_AddItemToContainer`, `OnContainerSlotsChanged`, and the client→server RPC wrappers on `AZSPlayerCharacter`. **Open UX question**: should interacting with a container open a real loot screen instead of auto-looting? Not decided.
- **T7 (death/respawn/sleep)**: new `FZSDeathInfo`/`GetLastDeathInfo()` - "cause of death" didn't exist before this. `GetRespawnDelaySeconds()`. `AZSGameState::GetSleepReadyCounts()`. **Open question (OQ-B6-07)**: fuller death-recap screen, not decided.
- **T8 (main menu/pause)**: new `UZSGameInstance` (the project had none before). `HostGame`/`JoinGame`, loading-screen delegates. **Steam invite deliberately not built** (needs `OnlineSubsystemSteam`, an infra decision for you). **Open question (OQ-B1-03)**: solo pause behaviour.

**T1/T2/T5.0 status, unchanged since 2026-08-01 (still current):**
- **T1 (Input-mode switching) — DONE**, solo PT1 passed clean in PIE 2026-08-01 (dev-confirmed). Still open, not blocking: PT1's 2-client half (deliberately deferred as its own session).
- **T2** — C++ done, plus `DA_ZS_UIStyle_Default` and `WBP_ZS_Base` (dev-confirmed created 2026-08-01).
- **T5.0** — Hip→Duffle/weapon-mount data model, compiled and automation-tested as of `de612a5`. **Real content risk still open**: `DA_Bag.uasset`'s `EquipSlot` needs a manual check (not yet done).

**Full click-by-click / per-widget steps** are in **`B1_UI_UX.md`'s "Manual setup steps" section** - every task group (T1-T8) now has its own entry listing every `WBP_ZS_*` widget still to build, which delegate/accessor each one binds to, and every open question flagged inline. A compiled visual checklist (hierarchy diagrams, screen wireframes) of the same list also exists as a private Claude artifact (not part of the repo) - not yet re-synced to this redesign, flag it if you use it before that catches up.

## B0 exit summary (closed 2026-07-30, practically not 100% formally)

Everything B1 builds UI against is solo-PIE-confirmed: item instances (`FZSItemInstance` refactor), weapon/combat mechanics, camera/aim, `BT_Zombie`, needs simulation. Two real bugs found and fixed this session: zombie bite zone weighting (was always Torso, now a weighted random roll) and `BT_Zombie` freezing after one melee hit (stale `bIsInMeleeRange` Blackboard bool, `AZombieAIController::Tick`, commit `1693884`).

**Two scope decisions moved formal B0 exit items to later phases** (both dev calls, "not a concern right now"):
- 2-client PIE verification → folds into **B1's own exit sweep** instead (debug-console-only feedback made it impractical to judge; far more legible once B1's HUD exists). Detail: `B0_Stabilization.md` Exit criteria, `B1_UI_UX.md` Exit criteria.
- B0-T12 performance baseline → moved to **B8** entirely, not carried as a checkpoint. Reverses the plan's original "profile early" requirement — B8 now captures its own before/after baseline instead of importing an early one. Detail: `B0_Stabilization.md` T12, `B8_Performance.md` Entry/Exit criteria.

**Three items carried forward, not blocking B1** (revisit whenever convenient, none of them gate B1 work):
1. Sections 6-7 (two-tier infection, amputation/blackout) — code complete, still PIE-unverified, dev-deferred since 2026-07-29. May end up easier to verify once B1 gives real UI feedback instead of `ZS.DebugListWounds` log dumps.
2. 5 parked automation-test failures, re-run and re-confirmed 2026-08-01 (`B1T6T7T8_AutomationRun.log`, the fourth clean run in a row) — still exactly these 5, unchanged by anything in B1 so far: `ZS.Combat.DownedZombieAutoRecovery`, `ZS.Combat.ZombieBiteZoneWeightedRoll`, `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag`, `ZS.Health.AmputationChoreographyEntersBlackout`, `ZS.Inventory.BagStoreAndRetrieve`. None diagnosed further this pass — still open.
3. PT6's full-stage-sweep-A–G has never run as one explicit single pass (pieces separately confirmed).

Full detail on all of the above: `Docs/Beta/B0_Stabilization.md` and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (both still current, kept for reference — B0 isn't deleted, just no longer the active phase).

## Next step

**All of B1's C++ is closed - T1 through T8, including the HUD redesign.** Everything through `6d413ac` compiles, passes the full `ZS.` automation suite (32 tests, only the same 5 pre-existing failures below), and needs no further rebuild to catch up. From here, the entire remaining B1 scope is Blueprint widget construction against the delegates/accessors already in place - `B1_UI_UX.md`'s Manual setup steps has a full per-widget checklist for every task group, task by task, same pattern as `WBP_ZS_Base`. Before diving into a screen, check that section for any open design question flagged on it - several are deliberately left undecided (container-interact behavior, solo pause OQ-B1-03, death-recap scope OQ-B6-07, Steam invite infra, non-weapon Equipment-slot items) rather than guessed past.

**One required manual step, separate from widget-building**: set `GameInstanceClass` to `ZSGameInstance` in Project Settings → Maps & Modes (or a BP child of it) - without this, none of T8's host/join/loading-screen code runs at all. Not done yet, deliberately left for you since `Config/DefaultEngine.ini` already has your own uncommitted changes this pass didn't touch.

Two smaller items still open regardless: **check `DA_Bag.uasset`'s `EquipSlot`** for the Hip→Duffle content-risk (not yet done), and PT1's 2-client sweep (deliberately deferred as its own session). Read `Docs/Planning/B1_UIDesignSession_2026-07-30.md` first if picking any screen up cold.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30, but did work correctly 2026-08-01 (two separate runs, both produced the named file) — inconsistent, cause unknown. Check the default `Saved/Logs/ZombieShooter.log` too if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
