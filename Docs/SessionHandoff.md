# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

Two companion docs: `Docs/Beta/B0_Stabilization.md` (full technical detail per sub-task) and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (the manual-steps/test-checklist/decisions companion — **start here**, current as of 2026-07-29).

## Last completed (2026-07-29)

Long session, real PIE testing finally happened (first since the last big bug-fix batch landed). Net result: **B0 is not yet exit-ready, but the remaining gap is now well understood and small.**

**Confirmed working in PIE** (checklist doc sections 1-4, 5's pipeline, 8): item durability persistence, bag grant/equip/store, ammo (solo), TwoHanded blocking SecondaryHand, the wound/infection/zone data pipeline itself, and the full Needs simulation (wet, indoor/outdoor temperature, performance multiplier, encumbrance, severity tiers).

**Real bugs found and fixed this session** (8 gameplay bugs + 1 content-adjacent + assorted debug-tooling bugs — full list in `B0_Stabilization.md`'s per-task rows): offhand-weapon durability reset on equip, death not writing back durability/destroying weapon actors, a zombie killed while downed staying flagged downed, sleep-readiness not accounting for a dead/blacked-out player, jammed offhand weapons having no way to clear, a lower-severity hit incorrectly setting bleed on an already-Fractured zone, `Server_StoreInBag` not rejecting equipped instances (partially — a real design call left open, see below), and a magazine actor surviving weapon `Destroy()`. Also fixed two bugs in my own new debug console commands (a zone argument silently misparsing "torso" as Head, and `ZS.DebugToggleSleepReady`'s log not showing whether the toggle actually took effect).

**Two real gameplay findings, diagnosed but deliberately not fixed** (dev's own call — "leave the zombie alone for now"): zombie bites always land on Torso regardless of approach angle (root cause: `Server_MeleeAttack`'s hit-zone trace samples a fixed Z-height, so angle was never going to produce variance — needs a design call on a proposed fix, a weighted random zone roll mirroring the existing headshot-weighting precedent), and zombies stop attacking after one hit (C++ traced clean — almost certainly a `BT_Zombie` graph issue, needs editor eyes, not fixable from code).

**One real bug found in editor content, not yet fixed**: scroll wheel zooms in regardless of direction and stays there — almost certainly `IA_Zoom`'s wheel-direction key mapping missing a Negate modifier on one side. Needs your hands in the editor, not a rebuild.

Added a large batch of `ZS.Debug*` console commands (full list in the checklist doc) so every remaining B0 checklist section has a way to trigger the relevant state without needing real input bindings yet. One genuine architectural discovery while building these: `AZSGameState::Server_AdvanceTimeByGameHours` only moves the *displayed* clock — Needs/Health both derive decay/progression from real `DeltaTime` scaled by `RealSecondsPerGameDay` independently. Added `Server_SetRealSecondsPerGameDay()` + `ZS.DebugSetTimeCompression` as the actual live time-skip lever.

**Deferred by the dev, not failures**: sections 6 (two-tier infection) and 7 (amputation/blackout) — dev wants a dedicated future testing pass rather than continuing today. Section 8 item 2 (wet noise reaction) — no audio exists yet to judge it by.

**Compile status**: two batches this session remain uncompiled as of writing — the big `ZS.Debug*` command batch (commit `9b8e43c`) **was rebuilt and confirmed working** (that's what produced today's PIE results above). A second, smaller batch on top of it (commit `5ff878c` — the zone-name-parsing fix and sleep-ready log improvement) has **not** been rebuilt/retested yet.

### What's actually blocking B0's exit (analyzed today against `B0_Stabilization.md`'s own written exit criteria)

Not content gaps or more debug tooling — two things that need the dev's hands directly:
1. **2-client testing hasn't happened at all this session.** B0's own checkpoints (`PT1` baseline, `PT4` noise stress test, `PT6` final sweep) explicitly require it, and everything tested so far has been solo.
2. **No performance baseline from a packaged Development build exists.** Real exit-criteria line item, nothing started — needs `Lvl_ZS_StressTest` (doesn't exist, `ZS.SpawnZombies` works in any level meanwhile) and an actual packaged-build run with results committed to `Docs/Testing/`.
3. **PT2's camera feel/tuning pass** can't happen until the scroll-wheel zoom bug above is fixed, and hasn't happened regardless.

Soft/non-blocking: `SessionHandoff.md`'s "zero unverified" gate is just a symptom of the above; the exit criteria literally point at `Docs/Testing/P5_P6_CharacterSetupVerification.md`, which predates the item-instance refactor and should be formally retired in favor of the newer checklist doc rather than chased stage-by-stage.

**Decision, 2026-07-29**: dev is ending the day here, staying focused on B0 (not starting B1 yet) — a future session may pick up B1 separately.

## Next step

1. **Rebuild** for commit `5ff878c` (zone-name parsing, sleep-ready log improvement) — not yet compiled or retested.
2. Run the full `ZS.*` automation filter — still hasn't happened at all this session, despite a lot of new/changed test coverage.
3. **The 2-client session and packaged-build perf baseline are the real remaining blockers for B0's exit** — both need the dev directly, not more code. Everything else in the checklist doc is either done, deferred by choice, or quick content authoring.
4. **Fix the scroll-wheel `IA_Zoom` mapping** in the editor (likely a missing Negate modifier on one wheel-direction key) — blocks PT2's camera pass.
5. **A real design decision is still waiting on you**: `Server_StoreInBag`'s partial fix — whether `UZSInventoryComponent` should gain a cross-component query into `AZSPlayerCharacter`'s loadout state, or the character should validate before calling in. See `B0_Stabilization.md`'s T2.9 row.
6. **Your own call, not urgent**: wanting unequipped weapons holstered on the character instead of destroyed, eventually — noted in the checklist doc's decisions log, not scheduled.
7. **Deliberately deferred, your call**: sections 6-7 of the checklist (infection/amputation) need a dedicated future pass; the zombie zone/attack-loop bugs are parked until you want to pick them back up.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — `unreal-mcp` disconnected, and `computer-use` can't find/control the Unreal Editor window at all. Every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands — including the `ZS.Debug*` console commands, since they still require a live PIE session to type into.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — confirmed 2026-07-29, cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/` — confirmed twice. Full rebuild for anything there.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
