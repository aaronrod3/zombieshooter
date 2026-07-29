# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

Two companion docs: `Docs/Beta/B0_Stabilization.md` (full technical detail per sub-task, current as of 2026-07-28) and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (the manual-steps/test-checklist/decisions companion — **start here**, current as of 2026-07-29).

## Last completed (2026-07-29) — `[uncompiled]`

With PIE/editor access unavailable to you this stretch, did a code-review pass on T11.2's offhand-weapon-firing code (the newest, least-scrutinized part of B0) instead of stalling. Found and fixed a real bug: `Server_EquipToSecondaryHand_Implementation` never called `SeedDurabilityFromInstance` — unlike the primary hand's hotbar-equip path, which explicitly seeds durability/condition after equipping. Every offhand weapon was silently resetting to full durability on every equip instead of resuming where it left off — the exact bug class the item-instance refactor exists to prevent. Fixed in `ZSPlayerCharacter.cpp`, mirroring the primary hand's pattern exactly. Added two new tests to protect it: `ZS.Loadout.SecondaryWeaponEquipAndUnequip` and `ZS.Loadout.SecondaryWeaponDurabilityWriteback` (the latter would have caught this bug directly had it existed sooner). Full detail: `B0_Stabilization.md` T11.4's row, `B0_ChecklistAndDecisions_2026-07-26.md` §1.5.

**Nothing here has been compiled** — the editor was open the whole time, and per standing policy a rebuild is present-session/dev-triggered, not something to force through unprompted. Traced carefully against the actual current source (every field/function checked directly, not assumed), but that's not a substitute for a real compile.

Also reviewed and fixed stale cross-references across `B1_UI_UX.md` through `B12`'s docs plus `CLAUDE.md`/`GameDevPlan.md`, left over from the 2026-07-26 rescope (old rejected death rule, "vehicles cut" contradicting its own reversal, "infection ambiguous" contradicting its own reversal, stale 2-4-player references, a missing Lockpicking entry). See commits `69c5091` and `708bdff`.

## Next step

1. **Compile gate, first thing**: full `Build.bat` rebuild. Touches `ZSPlayerCharacter.cpp` again (the durability-seeding fix) plus two new test cases — expect the new tests specifically to be unverified until a real build/run.
2. `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` §2.9 item 3 has the offhand-fire manual test steps — needs your hands (no PIE-input automation path exists — see tooling gotchas below). Worth re-testing durability persistence specifically now that the seeding bug is fixed.
3. Everything else in B0 is either already PIE-confirmed or already awaiting your hands — see the checklist doc's §2 for the full remaining order.
4. 4 automation tests (2 latent, 2 new `SecondaryWeapon` ones) are written but have never been run — say the word for a test-automation session (present-session, dev-triggered only, see `CLAUDE.md`).

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — `unreal-mcp` disconnected, and `computer-use` can't find/control the Unreal Editor window at all. Every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/` — confirmed twice. Full rebuild for anything there.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing. `DA_Bag.bIsEquippable` is `false` (a real content gap found by the automation suite) — blocks Checkpoint C in both PIE and the `ZS.Inventory.BagStoreAndRetrieve` test until set.
