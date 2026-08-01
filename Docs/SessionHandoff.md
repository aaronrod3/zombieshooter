# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1 progress — 2026-07-31

Continuing from 2026-08-01's T1/T2/T5.0 close-out (below). This pass: an away-session-style C++ scaffolding push for T3 (HUD) and T5 (inventory screen), while the editor stayed open on the dev's machine - so **this pass is UNCOMPILED, not yet automation-tested**. Per `CLAUDE.md`, `Build.bat` wasn't attempted (Live Coding is unreliable for header/`Tests/` changes, and this pass touched both extensively) - a full rebuild + `ZS.` automation run is the immediate next step, dev-triggered.

- **T3 (HUD) — C++ scaffolding done, commit `ea64726`**: moodle-stack aggregation (`UZSNeedsComponent::GetMoodleEntries()`/`OnMoodleStackChanged`, one bind covering Hunger/Thirst/Fatigue/Stamina/Temperature), the transparent-stat-preview contract (`UZSItemConfig`/`UZSWeaponConfig::GetStatPreviewLines()`, T3.7 and T5.6 share it), a scoreboard change-delegate (`AZSGameState::PlayerListVersion`/`OnPlayerListChanged`, since `PlayerArray` has none natively) wired from new `AZSGameMode::PostLogin`/`Logout` overrides, and a new client-local `UZSNotificationSubsystem` (T3.10's toast queue) with player-joined/left wired as its first concrete trigger via a new `AZSGameState::Multicast_ShowToast`. T3.2/T3.3/T3.4/T3.5/T3.6 needed no new code - last session's delegate audit already covered them. T3.11 (autosave indicator) is explicitly blocked - no B3 save system exists yet.
- **T5 (inventory screen) — C++ scaffolding done, commit `ea64726`**: the `HotbarSlots` runtime-assignment gap flagged below is now closed (`AZSPlayerCharacter::Server_AssignHotbarSlot`/`Server_ClearHotbarSlot`/`CanAssignToHotbarSlot`, scoped to mounted weapons only, per the design already on record - also added `GetHotbarSlots()`, which had no public accessor at all before this), plus a compartment-filter helper (`UZSInventoryComponent::GetSlotsInLocation`) and a reusable drag-drop payload class (`UZSDragDropPayload`). T5.2/T5.5/T5.6 needed no new code - already-exposed accessors cover them.
- **6 new automation tests** added alongside (hotbar-assignment gating, compartment filtering, notification queue) - not yet run, blocked on the same compile.

**T1/T2/T5.0 status, unchanged since 2026-08-01 (still current):**
- **T1 (Input-mode switching) — DONE**, solo PT1 passed clean in PIE 2026-08-01 (dev-confirmed): `UZSUIManager` modal stack + hard-gated `HandleAttack`/`IsCursorFacingActive`, `IA_UISelect`/`IA_UICancel`/`IA_UINavigate`/`IMC_ZS_UI` all correctly mapped. Still open, not blocking: PT1's 2-client half (folds into this exact checkpoint per `B1_UI_UX.md`'s Exit criteria note, deliberately deferred as its own session).
- **T2** — `UZSUIStyleConfig`/`UZSUserWidgetBase` (arrow-key focus nav) C++, plus `DA_ZS_UIStyle_Default` and `WBP_ZS_Base` (both dev-confirmed created 2026-08-01). T2.2/T2.3 have nothing to apply to until a real widget exists.
- **T5.0** — Hip→Duffle/weapon-mount data model, compiled and automation-tested as of `de612a5`. **Real content risk still open**: `DA_Bag.uasset`'s `EquipSlot` needs a manual check for the Hip→Duffle silent-reinterpretation risk (not yet done).

**Full click-by-click / per-widget steps** are in **`B1_UI_UX.md`'s "Manual setup steps" section** - T3 and T5 each now have their own entry there listing every `WBP_ZS_*` widget still to build and which delegate/accessor each one binds to.

## B0 exit summary (closed 2026-07-30, practically not 100% formally)

Everything B1 builds UI against is solo-PIE-confirmed: item instances (`FZSItemInstance` refactor), weapon/combat mechanics, camera/aim, `BT_Zombie`, needs simulation. Two real bugs found and fixed this session: zombie bite zone weighting (was always Torso, now a weighted random roll) and `BT_Zombie` freezing after one melee hit (stale `bIsInMeleeRange` Blackboard bool, `AZombieAIController::Tick`, commit `1693884`).

**Two scope decisions moved formal B0 exit items to later phases** (both dev calls, "not a concern right now"):
- 2-client PIE verification → folds into **B1's own exit sweep** instead (debug-console-only feedback made it impractical to judge; far more legible once B1's HUD exists). Detail: `B0_Stabilization.md` Exit criteria, `B1_UI_UX.md` Exit criteria.
- B0-T12 performance baseline → moved to **B8** entirely, not carried as a checkpoint. Reverses the plan's original "profile early" requirement — B8 now captures its own before/after baseline instead of importing an early one. Detail: `B0_Stabilization.md` T12, `B8_Performance.md` Entry/Exit criteria.

**Three items carried forward, not blocking B1** (revisit whenever convenient, none of them gate B1 work):
1. Sections 6-7 (two-tier infection, amputation/blackout) — code complete, still PIE-unverified, dev-deferred since 2026-07-29. May end up easier to verify once B1 gives real UI feedback instead of `ZS.DebugListWounds` log dumps.
2. 5 parked automation-test failures, re-run and re-confirmed 2026-08-01 (`B1T5_AutomationRun2.log`) — still exactly these 5, unchanged by anything in B1 so far: `ZS.Combat.DownedZombieAutoRecovery`, `ZS.Combat.ZombieBiteZoneWeightedRoll`, `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag`, `ZS.Health.AmputationChoreographyEntersBlackout`, `ZS.Inventory.BagStoreAndRetrieve`. None diagnosed further this pass — still open.
3. PT6's full-stage-sweep-A–G has never run as one explicit single pass (pieces separately confirmed).

Full detail on all of the above: `Docs/Beta/B0_Stabilization.md` and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (both still current, kept for reference — B0 isn't deleted, just no longer the active phase).

## Next step

**Rebuild first.** This session's T3/T5 C++ (commit `ea64726`) is uncompiled - editor was open, so `Build.bat` wasn't attempted per `CLAUDE.md`. Close the editor, full rebuild, then run the `ZS.` automation suite (6 new tests this pass) and fix anything that breaks before trusting any of it. Everything through `de612a5` (T1/T2/T5.0) is still solid and doesn't need re-verifying.

After that: `B1_UI_UX.md`'s Manual setup steps now has a full per-widget list for both T3 and T5 - building the actual `WBP_ZS_*` widgets against the delegates/accessors already in place is the real remaining work, task by task, same pattern as `WBP_ZS_Base`. Two smaller items still open regardless: **check `DA_Bag.uasset`'s `EquipSlot`** for the Hip→Duffle content-risk (not yet done), and PT1's 2-client sweep (deliberately deferred as its own session). Read `Docs/Planning/B1_UIDesignSession_2026-07-30.md` first if picking T5 up cold.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30, but did work correctly 2026-08-01 (two separate runs, both produced the named file) — inconsistent, cause unknown. Check the default `Saved/Logs/ZombieShooter.log` too if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
