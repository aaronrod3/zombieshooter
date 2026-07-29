# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

Two companion docs: `Docs/Beta/B0_Stabilization.md` (full technical detail per sub-task, current as of 2026-07-28) and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (the manual-steps/test-checklist/decisions companion — **start here**, current as of 2026-07-29).

## Last completed (2026-07-29) — `[uncompiled]`

With PIE/editor access unavailable to you this stretch, ran three rounds of code review across B0 instead of stalling, finding and fixing **eight real bugs**, all reusing existing functions (no new abstractions):

1. `Server_EquipToSecondaryHand_Implementation` never seeded offhand-weapon durability from the carried instance — every re-equip silently reset to full durability.
2. `Server_HandleDeathLootAndZombie` never wrote back weapon durability or destroyed `CurrentWeapon`/`SecondaryWeapon` before dropping loot — a permanent actor leak plus stale-durability loot on every death with a weapon equipped.
3. `AZombieCharacter::Die()` never reset `bIsDowned` — a zombie killed while downed stayed flagged downed on its corpse forever.
4. Sleep-readiness aggregation never accounted for a dead/blacked-out player, and neither `HandleDeath()` nor `EnterBlackout()` cancelled it — a party's sleep/time-skip could succeed off an incapacitated teammate's stale ready flag.
5. `CanRackFirearm`/`Server_StartRackFirearm` were hard-coded to `CurrentWeapon` only — a jammed offhand weapon had no way to ever clear the jam.
6. The bleed-flag logic in `Server_ApplyDamage` gated on the incoming hit's `WoundType`, not the zone's resolved one — a lower-severity hit onto an already-Fractured zone could set/leave `bBleeding=true` on a zone that never actually drains from it.
7. `Server_StoreInBag` never guarded against storing a currently-equipped instance — **only partially fixed**: the `EquippedBack`/`EquippedHip` half is fixed (lives on the component itself, no new cross-component reach needed); the `HotbarSlots`/`SecondaryHandInstanceId` half lives on `AZSPlayerCharacter` and is a real design call, left open on purpose.
8. **Dev-reported via your own PIE testing**: unequipping a rifle left its magazine prop floating in the world. `AZSMagazine` is a separate, unreplicated actor merely attached to `AZSWeapon`'s mesh — actor attachment doesn't cascade `Destroy()`, so every `CurrentWeapon->Destroy()` call site (unequip, death, weapon-break) orphaned it. Fixed with one `AZSWeapon::Destroyed()` override that explicitly destroys the magazine first, closing every call site at once.

Added 9 new tests to protect these (one bug had two symptoms covered by the same durability-writeback test). Full detail: `B0_Stabilization.md`'s T2.7/T9.1/T10.1/T10.4/T11.4/T4.10/T5.3/T2.9 rows, `B0_ChecklistAndDecisions_2026-07-26.md`'s automated-coverage section.

**Nothing has been compiled** — the editor was open the whole stretch, and per standing policy a rebuild is present-session/dev-triggered, not something to force through unprompted. Every function referenced was traced directly against the current source (not assumed), including a Plan-mode design pass that re-verified the highest-confidence findings before any code was touched — but none of that substitutes for a real compile. **One compile error was already reported and fixed** (a test called a protected `Server_` RPC directly instead of its public wrapper, same mistake pattern twice this session) — rebuild not yet confirmed since.

Also reviewed and fixed stale cross-references across `B1_UI_UX.md` through `B12`'s docs plus `CLAUDE.md`/`GameDevPlan.md`, left over from the 2026-07-26 rescope. See commits `69c5091` and `708bdff`. Rewrote `B0_ChecklistAndDecisions_2026-07-26.md` into a plain step-by-step walkthrough format per dev request, folding in the magazine bug fix and the two content gaps (`StressTestZombieClass` unset, `DA_Bag.bIsEquippable`) confirmed blocking by the dev's own console-command testing.

## Next step

1. **Compile gate, first thing**: full `Build.bat` rebuild. This stretch touched `ZSPlayerCharacter.cpp` (4 fixes), `ZombieCharacter.cpp` (1 fix), `ZSHealthComponent.cpp` (1 fix), `ZSInventoryComponent.cpp` (1 fix), `ZSWeapon.h`/`.cpp` (1 fix, the magazine-orphan bug), and added 9 test cases on top of the 2 from the previous stretch — a real batch of changes, worth a careful build/run pass, not a quick skim.
2. Once compiled, run the full `ZS.*` automation filter, not just the newest tests — the death-path fixes touch `HandleDeath`, which no pre-existing test covers at all.
3. **`Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` is now a walkthrough, start there** — its "Start here" section has 3 immediate items: rebuild (magazine fix), assign `StressTestZombieClass`, and check `DA_Bag.bIsEquippable` (the exact two gaps your own console testing just hit).
4. **A real design decision is waiting on you**: bug 7's `HotbarSlots`/`SecondaryHandInstanceId` half (whether `UZSInventoryComponent` should gain a cross-component query into `AZSPlayerCharacter`'s loadout state, or the character should validate before calling `Server_StoreInBag`) — see `B0_Stabilization.md`'s T2.9 row.
5. Everything else in B0 is either already PIE-confirmed or already awaiting your hands — see the checklist doc's walkthrough for the full remaining order.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — `unreal-mcp` disconnected, and `computer-use` can't find/control the Unreal Editor window at all. Every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/` — confirmed twice. Full rebuild for anything there.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing. `DA_Bag.bIsEquippable` is `false` (a real content gap found by the automation suite) — blocks Checkpoint C in both PIE and the `ZS.Inventory.BagStoreAndRetrieve` test until set.
