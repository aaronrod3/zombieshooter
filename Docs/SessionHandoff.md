# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1-T1 (Input-mode switching) — code written 2026-07-30, blocked on dev for the rest

**What's done (commit `aee5eb1`):** `UZSUIManager` (`ULocalPlayerSubsystem`, `Source/ZombieShooter/UI/`) implements the modal stack from `B1_UI_UX.md` T1.2 — `PushModal`/`PopModal(FName ModalTag)`, `IsAnyModalActive()`, `GetTopModalTag()`, nested-modal-safe (verified by a new headless automation test, `ZS.UI.ModalStackOrdering`, not yet run — see below). `AZSPlayerCharacter::HandleAttack()` and `IsCursorFacingActive()` (T1.3/T1.4) both hard-guard on `IsAnyModalActive()` directly, not just on the `IMC_ZS_UI` priority mechanism. `ZS.UI.PushTestModal`/`PopTestModal` console commands stand in for real screens so PT1 can be tested before T2+ exists.

**Not done — needs the dev, in order:**
1. **Create content assets in-editor** (none of this is scriptable, same as every prior Enhanced Input asset in this project — see `Docs/InputBindings.md`'s UI section for the exact fields): `IA_UISelect` (Digital bool, LMB), `IA_UICancel` (Digital bool, Escape), `IA_UINavigate` (Axis2D, Arrow Keys — deliberately not WASD, movement must keep working with a menu open), then `IMC_ZS_UI` mapping all three. Until these exist, `UZSUIManager`'s `ConstructorHelpers::FObjectFinder` silently no-ops (same graceful-if-missing pattern as `RackAction`/`FinisherAction`/`ZoomAction` before they were authored) — the modal stack bookkeeping still works, but nothing actually changes the active Enhanced Input context yet.
2. **Regen project files + full `Build.bat` rebuild** (not Live Coding — a brand-new `UCLASS`, `Source/ZombieShooter/UI/` is a new module folder). Editor was open all session, so this was never attempted — see `Docs/CommandReference.md`.
3. **Run `ZS.Combat.*`/etc. automation suite once** to confirm `ZS.UI.ModalStackOrdering` (new test, pure state logic, no PIE needed) actually passes — written but never executed against a real build.
4. **PT1 in PIE** (hands-only, no automation path): `ZS.UI.PushTestModal`/`PopTestModal` mid-attack, spam open/close, nested push (`PushTestModal A` then `PushTestModal B`, confirm `PopTestModal B` lands back on `A`), disconnect with a modal open. Confirm zero input leakage either direction, and explicitly confirm T1.5 (zombies/needs/attackability keep going while a test modal is "open" — nothing pauses).

Once PT1 passes, T1 is done and T2 (widget architecture/design tokens) is next.

## B0 exit summary (closed 2026-07-30, practically not 100% formally)

Everything B1 builds UI against is solo-PIE-confirmed: item instances (`FZSItemInstance` refactor), weapon/combat mechanics, camera/aim, `BT_Zombie`, needs simulation. Two real bugs found and fixed this session: zombie bite zone weighting (was always Torso, now a weighted random roll) and `BT_Zombie` freezing after one melee hit (stale `bIsInMeleeRange` Blackboard bool, `AZombieAIController::Tick`, commit `1693884`).

**Two scope decisions moved formal B0 exit items to later phases** (both dev calls, "not a concern right now"):
- 2-client PIE verification → folds into **B1's own exit sweep** instead (debug-console-only feedback made it impractical to judge; far more legible once B1's HUD exists). Detail: `B0_Stabilization.md` Exit criteria, `B1_UI_UX.md` Exit criteria.
- B0-T12 performance baseline → moved to **B8** entirely, not carried as a checkpoint. Reverses the plan's original "profile early" requirement — B8 now captures its own before/after baseline instead of importing an early one. Detail: `B0_Stabilization.md` T12, `B8_Performance.md` Entry/Exit criteria.

**Three items carried forward, not blocking B1** (revisit whenever convenient, none of them gate B1 work):
1. Sections 6-7 (two-tier infection, amputation/blackout) — code complete, still PIE-unverified, dev-deferred since 2026-07-29. May end up easier to verify once B1 gives real UI feedback instead of `ZS.DebugListWounds` log dumps.
2. 5 parked automation-test failures — 1 new-fix bug (`ZS.Combat.ZombieBiteZoneWeightedRoll`, diagnostic added in commit `ced011a`, needs a rebuild+retest to interpret), 3 pre-existing (`ZS.Combat.DownedZombieAutoRecovery`, `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag`, `ZS.Health.AmputationChoreographyEntersBlackout`), 1 known content gap (`ZS.Inventory.BagStoreAndRetrieve`, `DA_Bag.bIsEquippable` — actually already fixed, this one might pass on retest).
3. PT6's full-stage-sweep-A–G has never run as one explicit single pass (pieces separately confirmed).

Full detail on all of the above: `Docs/Beta/B0_Stabilization.md` and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (both still current, kept for reference — B0 isn't deleted, just no longer the active phase).

## Next step

**Finish B1-T1** per the four dev-blocking steps above (content assets → rebuild → automation test → PT1 in PIE), then move to **B1-T2 — Widget architecture & design tokens**. Read `Docs/Planning/B1_UIDesignSession_2026-07-30.md` first if picking this up cold — it has the actual HUD/Tab-menu/inventory design decisions T1 and later tasks need to build against.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30 — output landed in the default `Saved/Logs/ZombieShooter.log` instead. Check there if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
