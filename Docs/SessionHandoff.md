# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

Two companion docs: `Docs/Beta/B0_Stabilization.md` (full technical detail per sub-task, current as of 2026-07-28) and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (the manual-steps/test-checklist/decisions companion — **start here**, current as of 2026-07-29).

## Last completed (2026-07-29) — `[compiled]`

Closed T11.2's offhand-weapon-firing content gap (implemented 2026-07-28 away session, compiled clean 2026-07-29): `AZSPlayerCharacter::SecondaryWeapon` mirrors `CurrentWeapon`'s full lifecycle; fire reuses a newly-extracted `FireWeapon(AZSWeapon*)` helper shared with the primary hand. Also fixed a real cross-client bug it surfaced in `AZSWeapon::OnRep_CurrentConfig` (was mis-attaching the offhand weapon's cosmetics on remote clients). Commit `3003f07` (feature) + `c974bdf` (compile fix — one test file called a protected function directly; production code had zero errors).

**Still not PIE-tested** — compiling only proves it builds, not that it behaves correctly at runtime.

## Next step

1. `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` §2.9 item 3 has the offhand-fire manual test steps — the one genuinely new, unverified mechanic from the last stretch. Needs your hands (no PIE-input automation path exists — see tooling gotchas below).
2. Everything else in B0 is either already PIE-confirmed or already awaiting your hands — see the checklist doc's §2 for the full remaining order.
3. 2 latent automation tests (`ZS.Combat.DownedZombieAutoRecovery`, `ZS.Health.AmputationChoreographyEntersBlackout`) compile but have never been run — say the word for a test-automation session (present-session, dev-triggered only, see `CLAUDE.md`).

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — `unreal-mcp` disconnected, and `computer-use` can't find/control the Unreal Editor window at all. Every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/` — confirmed twice. Full rebuild for anything there.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing. `DA_Bag.bIsEquippable` is `false` (a real content gap found by the automation suite) — blocks Checkpoint C in both PIE and the `ZS.Inventory.BagStoreAndRetrieve` test until set.
