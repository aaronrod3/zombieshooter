# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

Two companion docs: `Docs/Beta/B0_Stabilization.md` (full technical detail per sub-task) and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (the manual-steps/test-checklist/decisions companion — current as of 2026-07-30).

## Last completed (2026-07-30)

**Side session, not B0 itself**: a full B1 (UI/UX) design conversation happened ahead of implementation — HUD philosophy, Tab-menu structure, inventory compartments (Pockets/Backpack/Duffle + weapon-mount slots), and a plan-wide efficiency review across all B0–B12 docs. Written up and committed: `Docs/Planning/B1_UIDesignSession_2026-07-30.md`, plus updates to `B1_UI_UX.md` (new T5.0 data-model prerequisite) and `T_ContinuousTracks.md` (icon-authoring gap closed). B0 stays the active phase — B1 has not started.

**B0's two remaining deferred fixes from 2026-07-29 are now implemented, rebuilt, and test-run**: zombie bite zone weighted random roll (`UZSZombieConfig::HeadBiteChance`/`ArmsBiteChance`/`LegsBiteChance`, replacing the fixed-height trace) and `Server_StoreInBag`'s hotbar/SecondaryHand guard (new `AZSPlayerCharacter::Server_StoreInBagChecked`). Commits `30f7ad7`, `ced011a`.

**First-ever run of the full `ZS.*` automation suite** (23 tests — this whole batch had never actually been executed before today, per prior sessions' own notes). **18 passed, 5 failed**:
- `ZS.Inventory.StoreInBagRejectsSecondaryHandInstance` — ✅ pass, new fix confirmed.
- `ZS.Inventory.BagStoreAndRetrieve` — known pre-existing content gap, already documented, not new.
- `ZS.Combat.ZombieBiteZoneWeightedRoll` — ❌ **new fix's own test fails**: no wound landed anywhere (not even wrong-zone), meaning damage never arrived rather than the zone math being wrong. Every *other* zone/wound test in the suite calls `Server_ApplyDamage` directly; this is the first to go through the real `ApplyPointDamage → TakeDamage` chain. Added a health-before/after diagnostic (commit `ced011a`, not yet rebuilt/retested) to pinpoint where it breaks next run.
- `ZS.Combat.DownedZombieAutoRecovery`, `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag`, `ZS.Health.AmputationChoreographyEntersBlackout` — pre-existing tests, never run before today, now found failing. Not caused by this session's changes. **Dev decision 2026-07-30: parked for later, not investigated this session.**

## Last completed, continued (2026-07-30)

**Scroll-wheel zoom bug fixed** (editor-side `IA_Zoom` mapping, dev's own fix) and **PT2 camera checkpoint passed** — zoom smooth 600-1400 both directions, hotbar untouched by scroll, hip-fire vs. aimed spread/headshot-rate split confirmed, 20+ min full pass already covered in earlier testing.

**`BT_Zombie` "stops attacking after one hit" fixed and PIE-confirmed**: `AZombieAIController::Tick` was leaving `bIsInMeleeRange` stale-true when `TargetActor` went null (a momentary perception loss at point-blank range), so the Selector kept re-picking the Attacking branch forever while `TriggerMeleeAttack` silently no-op'd on the null target. Now clears the bool on the same early-return. Commit `1693884`. Fought a zombie through multiple hits in PIE — no longer freezes. Detail: `B0_Stabilization.md` T8.6.

**Scope decision: 2-client PIE verification is deferred out of B0 entirely, into B1's own exit sweep** (dev call, 2026-07-30) — debug-console-only feedback makes judging a second client's state impractical right now, and it'll be far more legible once B1's HUD/menus exist to observe it against. B0's exit criteria, Playtest Checkpoints (PT1, PT4 scenario e, PT6), and every scattered "2-client check" in the checklist doc are all updated to reflect this — B0 no longer formally blocks on any of it. Full detail: `B0_Stabilization.md` Exit criteria (carried-forward note) and `B1_UI_UX.md` Exit criteria (matching note on the receiving end).

## Next step

1. **Performance baseline is now the main remaining B0 item** — decide dedicated `Lvl_ZS_StressTest` map vs. reusing `Lvl_ThirdPerson`, then packaged-build profiling at 25/50/100/150/250 zombies, committed to `Docs/Testing/PerfBaseline_B0.md`.
2. Sections 6–7 (two-tier infection, amputation/blackout) — still deferred by dev choice, no dependency on anything above.
3. **Parked, your call when ready**: 5 automation-test failures from the 2026-07-30 run (1 new-fix bug + 3 pre-existing + 1 known content gap) — rebuild for commit `ced011a` (the diagnostic addition) first, before touching the `ZombieBiteZoneWeightedRoll` one specifically.
4. Once 1–3 are clear, B0's remaining (solo) exit criteria are essentially done — worth a final read of `B0_Stabilization.md` Exit criteria before declaring B0 exited and starting B1 implementation.

Full sequenced runbook for step 1 (with exact commands) is in this conversation's history — re-derive from `B0_ChecklistAndDecisions_2026-07-26.md` if picked up cold in a future session.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30 — output landed in the default `Saved/Logs/ZombieShooter.log` instead. Check there if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
