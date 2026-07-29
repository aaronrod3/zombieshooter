# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

Two companion docs, both current as of 2026-07-28: `Docs/Beta/B0_Stabilization.md` (full technical detail per sub-task) and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (curated build-first/test-in-order/decisions companion — **start here**).

## Last completed (2026-07-29) — `[compiled]`

Closed T11.2's offhand-weapon-firing content gap (implemented 2026-07-28 away session, compiled 2026-07-29): `AZSPlayerCharacter::SecondaryWeapon` now mirrors `CurrentWeapon`'s full lifecycle (spawn/attach/destroy/durability-writeback), fire reuses a newly-extracted `FireWeapon(AZSWeapon*)` helper shared with the primary hand, melee reuses the already-generic `PerformMeleeSwing` unchanged. Also fixed a real bug it surfaced: `AZSWeapon::OnRep_CurrentConfig` used to call the character's primary-weapon attach functions unconditionally regardless of which weapon actor was replicating — would have silently mis-attached the offhand weapon's cosmetics for remote clients. Commit `3003f07`.

**Full `Build.bat` rebuild (editor closed, dev-triggered) succeeded — but not on the first try.** `Result: Failed (OtherCompilationError)` on the first pass: `ZS.Health.AmputationChoreographyEntersBlackout` (written 2026-07-28, part of the still-unverified latent-test batch, never previously compiled) called the protected `AZSPlayerCharacter::Server_AmputateZone` directly instead of the public `AmputateZone()` wrapper other passing tests already use correctly. Fixed the one call site, rebuilt, `Result: Succeeded` clean. The feature code itself (`ZSPlayerCharacter.h/.cpp`, `ZSWeapon.cpp`) had zero errors — the bug was in test code, not production code.

**Still not PIE-tested** — compiling only proves it builds, not that it behaves correctly at runtime.

## Next step

1. `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` §2.9 item 3 has the offhand-fire manual test steps (updated 2026-07-28) — this is the one genuinely new, unverified mechanic from the last stretch. **Blocked on your hands, confirmed 2026-07-29**: asked to run this myself and re-confirmed there's no path — `unreal-mcp` is still disconnected and `computer-use`'s `request_access` still can't find/control the Unreal Editor window at all (not just "input doesn't land," it can't see the app). Same as every other PIE check this project has ever needed. Do this whenever you're next at the PC.
2. Everything else in B0 is either already PIE-confirmed or was already awaiting your hands before this stretch — see the checklist doc's own §2 for the full remaining order.
3. The 3 latent automation tests (`ZS.Combat.DownedZombieAutoRecovery`, `ZS.Health.AmputationChoreographyEntersBlackout`, and whichever else is in that batch) are now known to compile but have never actually been *run* — say the word when you want a test-automation session and they'll get their first real run, per the testing-capability policy below.

## Testing capability update (2026-07-28)

`Source/ZombieShooter/Tests/` now holds headless Unreal Automation Tests (12 tests, last known state 9/10 passing — the 3 latent ones from the newest batch are written but unverified) covering pure server-logic/state/math. **This is a present-session, dev-triggered activity only** — say the word when you want a batch built/run; see `Docs/CommandReference.md`'s "Editor close/rebuild for automation test runs" section for the mechanics (graceful close only, never force-kill — confirmed there's no safe way to save-first or inject commands into an already-running editor instance).

## Known tooling gotchas (worth remembering)

- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/` — confirmed twice. Full rebuild for anything there.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.
- `unreal-mcp` was disconnected for the 2026-07-26 autonomous stretch and everything since — every Data Asset/Blueprint/Input Action left as a documented content gap, none faked.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing. `DA_Bag.bIsEquippable` is `false` (a real content gap found by the automation suite) — blocks Checkpoint C in both PIE and the `ZS.Inventory.BagStoreAndRetrieve` test until set.
