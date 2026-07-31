# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1 progress — 2026-07-30, NONE of this session's C++ has been compiled yet

Two stretches this session: a present, interactive pass (T1), then an away-session-style pass (T2/T3-T7-audit/T5.0-partial) while the dev worked T1's manual editor steps in parallel. **Every commit from the second stretch is tagged `[uncompiled]`** — the editor was closed (likely mid-rebuild) or busy with the dev's own hands-on work the whole time, so none of it could be build-gated or PIE-tested. Treat it as "implemented against established patterns, not yet verified," not "done."

- **T1 (Input-mode switching)** — `UZSUIManager` modal stack + hard-gated `HandleAttack`/`IsCursorFacingActive` (commit `aee5eb1`). `IA_UISelect`/`IA_UICancel`/`IA_UINavigate`/`IMC_ZS_UI` all now exist on disk (dev's own editor work, in progress).
- **T2.1/T2.4** — `UZSUIStyleConfig` + `UZSUserWidgetBase` (focus nav via `NativeOnKeyDown`), commit `fca75fc`. New API usage was checked against the actual UE 5.8 engine source, not memory, since no compiler was available.
- **T3/T7 delegate audit** — closed 3 real "no OnRep needed, no UI yet" gaps now that UI is coming (sleep-request state, weapon durability, ammo not broadcasting on host), commit `89f466e`.
- **T5.0 partial** — `EZSItemSize` field only, commit `9c77d44`. The rest (EZSCarryLocation Backpack/Duffle split, weapon-mount slots, HotbarSlots rewiring) was deliberately **not** attempted — found a genuine open design question (what happens to `EZSEquipSlot::Hip` under the new compartment model) and the risk of blind-editing B0's already-verified hotbar/equip flow with no way to compile-check it was too high. Full detail + the specific question: `B1_UI_UX.md`'s Manual setup steps, T5.0 entry.

**Full click-by-click steps for all of the above** (T1's remaining PIE verification, T2's `DA_ZS_UIStyle_Default` creation, the rebuild everything needs) are in **`B1_UI_UX.md`'s "Manual setup steps" section** — that's the one place this level of detail lives now (see `Docs/Beta/README.md`'s convention note). **First thing next session: a real compile.** Everything from `fca75fc` onward is unverified.

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

**Compile first.** A full `Build.bat` rebuild covers everything from this session in one pass — T1's `IMC_ZS_UI`/content fixes plus all the new C++ (`Source/ZombieShooter/UI/`, `ZSGameState`/`ZSWeapon` delegate additions, `ZSItemConfig`'s new enum). Fix whatever doesn't compile before trusting any of it. Then work `B1_UI_UX.md`'s "Manual setup steps" section top to bottom (T1's PT1 PIE pass, T2's `DA_ZS_UIStyle_Default`), and raise T5.0's open Hip/Duffle question when convenient — it blocks the rest of T5.0 but not T2/T3 work. Read `Docs/Planning/B1_UIDesignSession_2026-07-30.md` first if picking this up cold.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30 — output landed in the default `Saved/Logs/ZombieShooter.log` instead. Check there if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
