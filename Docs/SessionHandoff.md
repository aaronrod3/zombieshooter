# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1 progress — 2026-07-30

Three stretches this session: a present, interactive pass (T1), an away-session-style pass while the dev worked T1's manual editor steps in parallel (T2/T3-T7-audit/T5.0-partial), then back to present/interactive for two build-error fixes and T5.0's remaining data-model work.

**Compiles clean AND automation-tested as of `de612a5`** — full rebuild succeeded, then the `ZS.` automation suite ran clean (dev-requested, present session): only the same 5 pre-existing failures below, nothing new broken by this session. Chain: `aee5eb1`/`fca75fc`/`89f466e`/`9c77d44` → two build fixes (`d8b9e94` a local `Navigation` var in `ZSUserWidgetBase.cpp` shadowed `UWidget::Navigation`, C4458; `1b7d4a5` LNK2019 - `FReply`'s constructors needed `SlateCore` added as an explicit `ZombieShooter.Build.cs` dependency, not just transitively through `Slate`) → `75af0fe` (T5.0 Hip→Duffle/weapon-mounts) → `de612a5` (a real bug the test run caught: `NewObject<UZSUIManager>()` with no outer violates `ULocalPlayerSubsystem`'s `UCLASS(Within = LocalPlayer)` - fixed with a throwaway `ULocalPlayer` outer).

- **T1 (Input-mode switching)** — `UZSUIManager` modal stack + hard-gated `HandleAttack`/`IsCursorFacingActive`. `IA_UISelect`/`IA_UICancel`/`IA_UINavigate`/`IMC_ZS_UI` all exist on disk (dev's own editor work) - **PT1 PIE pass is the one thing left**, hands-only.
- **T2.1/T2.4** — `UZSUIStyleConfig` + `UZSUserWidgetBase` (arrow-key focus nav). `DA_ZS_UIStyle_Default` data asset instance not created yet.
- **T3/T7 delegate audit** — closed 3 real "no OnRep needed, no UI yet" gaps (sleep-request state, weapon durability, ammo not broadcasting on host).
- **T5.0** — Hip→Sidearm question got a dev answer mid-session ("Hip is for the pistol, name it Sidearm; long-gun mounts attach to the backpack if equipped else the back"), unblocking most of the rest. `EZSEquipSlot::Hip` retired entirely (replaced by `Duffle`, the second bag slot), `EZSCarryLocation::Bag` split into `Backpack`/`Duffle`, `UZSInventoryComponent` gained real weapon-mount slots (`MountedLongGuns[2]` + `MountedSidearm`, pure `FGuid` capacity-gate references, no `AZSWeapon` actor lifecycle) validated against `UZSWeaponConfig::Handedness`/`AttackType`. **Real content risk still open**: `Hip`'s old raw enum value now belongs to `Duffle` - any existing config with `EquipSlot == Hip` likely silently reinterprets as `Duffle` rather than failing to load; `DA_Bag.uasset` needs a manual check (not yet done). Full detail: `B1_UI_UX.md`'s T5.0 entry.

**Still deliberately not started**: `HotbarSlots` becoming a quick-select pointer into a mount - turned out bigger than scoped (no runtime "assign an item to hotbar slot N" mechanism exists at all yet, `HotbarSlots` is currently `BeginPlay`-seeded only). Needs a call on whether to build that now or defer to T5's inventory screen - see `B1_UI_UX.md`.

**Full click-by-click steps** (T1's PT1 PIE pass, T2's `DA_ZS_UIStyle_Default` creation, the content-risk check) are in **`B1_UI_UX.md`'s "Manual setup steps" section**.

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

Everything through `de612a5` compiles and passes automation - no rebuild needed to catch up. **Check `DA_Bag.uasset`'s `EquipSlot`** for the Hip→Duffle content-risk noted above (not yet done), then work `B1_UI_UX.md`'s "Manual setup steps" section top to bottom (T1's PT1 PIE pass, T2's `DA_ZS_UIStyle_Default`), and decide on `HotbarSlots`' rewiring scope when ready to continue T5.0. Read `Docs/Planning/B1_UIDesignSession_2026-07-30.md` first if picking this up cold.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30, but did work correctly 2026-08-01 (two separate runs, both produced the named file) — inconsistent, cause unknown. Check the default `Saved/Logs/ZombieShooter.log` too if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
