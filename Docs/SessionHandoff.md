# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1 progress — 2026-08-02 (rewritten at session end; full blow-by-blow history is in the git log, not repeated here)

**All B1 C++ (T1-T8) is compiled and in the editor.** T1/T2/T5.0/T3/T5/T6/T7/T8's C++ groundwork all landed across earlier commits this session (`e064bc3`, `00d3633`, `6d413ac`, `de612a5`), capped by the big one: **every remaining B1 Widget Blueprint (except `WBP_ZS_Settings`) now has a dedicated `UZSUserWidgetBase` subclass** in `Source/ZombieShooter/UI/` — ~23 new files, following the `BindWidget`/`BindWidgetAnimOptional`/`NativeConstruct()` pattern proven on the `UZSBodyConditionIndicatorWidget` trial. Every card's **Bind** row in the manifest now says "nothing to bind" and lists what the C++ already does; remaining manual work per card is Designer-tab hierarchy + Class Defaults only, no Graph-tab wiring except a handful of "Create Widget → Open As Modal" call sites. New shared getters on `UZSUserWidgetBase` (`GetOwningZSGameState`/`GetOwningZSGameInstance`, plus `PushAsModal`/`PopAsModal`), `UZSHealthComponent::GetWoundDisplayCondition`/`HasAnyGameplayAffectingCondition`, `AZSPlayerCharacter::GetKeyLabelForHotbarIndex`, `UZSDragDropPayload::Make`, and the new **arm-wound accuracy-spread penalty** (`GetAccuracySpreadMultiplier()`, wired into `FireWeapon`'s spread roll) all landed the same way.

**Two real bugs found and fixed while wiring the C++ up:**
- `UZSHealthComponent::GetWoundDisplayCondition` — the manifest's original Blueprint-described logic only treated `Fracture` as gameplay-affecting, but `GetMobilityMultiplier`/`GetAttackSpeedMultiplier`/`GetReloadSpeedMultiplier` actually penalize Arms/Legs for *any* non-None `WoundType`. Fixed to be zone-aware, covered by `ZS.Health.WoundDisplayConditionIsZoneAwareAndPrioritized`.
- `UZSInventoryComponent::Server_EquipToSlot`/`Server_MountLongGun`/`Server_MountSidearm` are plain `HasAuthority()`-gated calls, not real RPCs — calling them straight from a client-owned widget (as earlier manifest revisions said to do) silently no-ops on non-host clients. Fixed with new `Server, Reliable` wrappers of the same name on `AZSPlayerCharacter`.

**Full rebuild completed 2026-08-02** — 2 real compile errors hit and fixed along the way, both the same root cause: `UWidget` already declares its own `Slot` property (the `UPanelSlot` a widget occupies in its parent), so naming anything `Slot` in a `UWidget`-derived class collides with it. First hit as a `UPROPERTY` in `UZSEquipSlotWidget.h` (renamed to `GearSlot`, commit `057cbd5`), then again as local variables in `ZSCompartmentPanelWidget.cpp`/`ZSContainerLootWidget.cpp` (MSVC's C4458, treated as an error — renamed to `ItemSlotWidget`, commit `c1f5472`). Manifest's EquipSlot card and the new tooling-gotcha below both flag this. **`ZS.` automation suite not re-run this pass** — still owed before trusting the 5-pre-existing-failures baseline is unchanged.

**Manifest artifact (private, not in the repo) substantially refined today, several real gaps found while the dev was actually building against it in-editor:**
- Every card now has an exact **Placement** row (Fill Screen / Desired-anchored-to-a-point / auto-layout child, with real anchor+position numbers instead of "eyeball it").
- `WBP_ZS_Inventory`'s hierarchy was missing a named container for the Loadout tab's own page (`Switcher_Tabs`' first child) — the player-silhouette `Image` and the region-3/5/6/8 widgets had nowhere real to go. Added `Canvas_Loadout` (and `VBox_Compartments`/`VBox_PlayerCompartments` wrapper Vertical Boxes for the 3 stacked `WBP_ZS_CompartmentPanel` instances, in both `WBP_ZS_Inventory` and `WBP_ZS_ContainerLoot`) as real, named nodes.
- A **Layout** reference table on `WBP_ZS_Inventory`'s card gives exact Position X/Y for everything inside `Canvas_Loadout` — the earlier numbers were computed against the full 1920×1080 viewport, but Position is always relative to the immediate parent, and `Canvas_Loadout` sits inset twice (Canvas_Window's margin, then its own offset to clear `HBox_Tabs`). Corrected.
- 5 cards (`WBP_ZS_ItemSlot`/`MoodleEntry`/`ToastEntry`/`StatPreviewLine`/`SleepPlayerRow`) said "check Size To Content" for a widget placed inside a Wrap/Vertical/Horizontal Box — that's a Canvas Panel child-slot property and doesn't exist there. Fixed to recommend a **Size Box** (Width/Height Override or Min Desired Width/Height) instead, the actual correct UMG pattern. `ItemSlot` is 64×64, `MoodleEntry` 40×40, `ToastEntry`/`StatPreviewLine`/`SleepPlayerRow` use Min-Desired floors instead of fixed sizes since their content can grow.
- `WBP_ZS_Inventory`'s open/close-toggle step was too vague to build against ("keep a reference... Close As Modal" without saying how a reference survives across two separate key-press events) — elaborated into a concrete `InventoryScreenRef` variable + `IsValid`/`IsInViewport` branch pattern that reuses the same widget instance and toggles open/closed off one key.

**In-editor build progress, dev-reported end of session 2026-08-02:**
- `WBP_ZS_ItemSlot` — built (with the corrected 64×64 Size Box), the last widget actually finished this session.
- `WBP_ZS_Inventory` — in progress, picking back up tomorrow at the now-elaborated open/close-toggle step.
- `WBP_ZS_BodyConditionIndicator` — hierarchy built, but its 5 Class Defaults texture slots (Wounded/Bleeding/Fracture/Infected/Amputated) are **still unassigned** — needs placeholder textures, dev flagged to remember to swap for real art later.
- **No animations built anywhere yet** (`CriticalBleedFlash`, `FadeInOut`, `SeverityPulse`, `Spin`) — all 4 are optional (`BindWidgetAnimOptional`), nothing is blocked, but none exist yet.
- **No PIE testing done on any of this UI work yet** — everything above is Designer-tab-only progress so far.

**T1/T2/T5.0 status, unchanged since 2026-08-01 (still current):**
- **T1 (Input-mode switching) — DONE**, solo PT1 passed clean in PIE 2026-08-01 (dev-confirmed). Still open, not blocking: PT1's 2-client half (deliberately deferred as its own session).
- **T2** — C++ done, plus `DA_ZS_UIStyle_Default` and `WBP_ZS_Base` (dev-confirmed created 2026-08-01).
- **T5.0** — Hip→Duffle/weapon-mount data model, compiled and automation-tested as of `de612a5`. **Real content risk still open**: `DA_Bag.uasset`'s `EquipSlot` needs a manual check (not yet done).
- **T6/T7/T8 C++** — commit `00d3633`, unchanged by anything above. Open questions still unresolved: container-interact UX (auto-loot vs. real loot screen), death-recap scope (OQ-B6-07), solo pause behaviour (OQ-B1-03), Steam invite infra.

**Full click-by-click / per-widget steps** are in **`B1_UI_UX.md`'s "Manual setup steps" section** and the UI Build Manifest artifact (private, not in the repo) — both updated to match everything above.

## B0 exit summary (closed 2026-07-30, practically not 100% formally)

Everything B1 builds UI against is solo-PIE-confirmed: item instances (`FZSItemInstance` refactor), weapon/combat mechanics, camera/aim, `BT_Zombie`, needs simulation. Two real bugs found and fixed this session: zombie bite zone weighting (was always Torso, now a weighted random roll) and `BT_Zombie` freezing after one melee hit (stale `bIsInMeleeRange` Blackboard bool, `AZombieAIController::Tick`, commit `1693884`).

**Two scope decisions moved formal B0 exit items to later phases** (both dev calls, "not a concern right now"):
- 2-client PIE verification → folds into **B1's own exit sweep** instead (debug-console-only feedback made it impractical to judge; far more legible once B1's HUD exists). Detail: `B0_Stabilization.md` Exit criteria, `B1_UI_UX.md` Exit criteria.
- B0-T12 performance baseline → moved to **B8** entirely, not carried as a checkpoint. Reverses the plan's original "profile early" requirement — B8 now captures its own before/after baseline instead of importing an early one. Detail: `B0_Stabilization.md` T12, `B8_Performance.md` Entry/Exit criteria.

**Three items carried forward, not blocking B1** (revisit whenever convenient, none of them gate B1 work):
1. Sections 6-7 (two-tier infection, amputation/blackout) — code complete, still PIE-unverified, dev-deferred since 2026-07-29. May end up easier to verify once B1 gives real UI feedback instead of `ZS.DebugListWounds` log dumps.
2. 5 parked automation-test failures, re-run and re-confirmed 2026-08-01 (`B1T6T7T8_AutomationRun.log`, the fourth clean run in a row) — still exactly these 5, unchanged by anything in B1 so far: `ZS.Combat.DownedZombieAutoRecovery`, `ZS.Combat.ZombieBiteZoneWeightedRoll`, `ZS.Combat.ZombieDeathWhileDownedClearsDownedFlag`, `ZS.Health.AmputationChoreographyEntersBlackout`, `ZS.Inventory.BagStoreAndRetrieve`. None diagnosed further this pass — still open.
3. PT6's full-stage-sweep-A–G has never run as one explicit single pass (pieces separately confirmed).

Full detail on all of the above: `Docs/Beta/B0_Stabilization.md` and `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` (both still current, kept for reference — B0 isn't deleted, just no longer the active phase).

## Next step

**Resume in-editor widget building, picking up `WBP_ZS_Inventory` at its now-elaborated open/close-toggle step** (an `InventoryScreenRef` variable + `IsValid`/`IsInViewport` branch pattern, spelled out in the manifest artifact — the old version was too vague to build against). `WBP_ZS_ItemSlot` is done. Continue through the rest of T3/T5/T6/T7/T8's remaining `WBP_ZS_*` widgets the same way — Designer-tab hierarchy + Class Defaults only, no Graph-tab wiring except a handful of explicit "Create Widget → Open As Modal" call sites. The manifest artifact has the exact class name, hierarchy diagram (with every previously-missing node — `Canvas_Loadout`, `VBox_Compartments`, the Size Box fixes — now filled in), Placement/anchor numbers, and steps per card; `B1_UI_UX.md`'s Manual setup steps section has the same per-task-group.

**Three concrete loose ends from today, not blocking further building but worth doing before B1 exit:**
1. **`WBP_ZS_BodyConditionIndicator`'s 5 icon texture slots are unassigned** — any functional-grey placeholder works, just remember to swap for real art later.
2. **No animations exist anywhere yet** (`CriticalBleedFlash`/`FadeInOut`/`SeverityPulse`/`Spin`) — all optional, nothing is blocked, build whenever or skip for B1 entirely.
3. **Nothing has been PIE-tested yet.** Once enough of T3/T5 is built to see the HUD + Inventory screen together, a first PIE pass is worth doing before building much further — a wrong `BindWidget` name or Size Box value is cheaper to catch early than after 10 more widgets are built the same way. `ZS.` automation suite also hasn't been re-run since the 2 Slot-shadowing build-error fixes — worth a run to confirm the 5-pre-existing-failures baseline is unchanged.

**Before diving into any screen, check for an open design question flagged on it** — several are deliberately left undecided (container-interact behavior, solo pause OQ-B1-03, death-recap scope OQ-B6-07, Steam invite infra, non-weapon Equipment-slot items, no-numeric-health-bar, keyboard-equivalents for every drag target) rather than guessed past.

**One required manual step, separate from widget-building**: set `GameInstanceClass` to `ZSGameInstance` in Project Settings → Maps & Modes (or a BP child of it) - without this, none of T8's host/join/loading-screen code runs at all. Not done yet, deliberately left for you since `Config/DefaultEngine.ini` already has your own uncommitted changes this pass didn't touch.

Two smaller items still open regardless: **check `DA_Bag.uasset`'s `EquipSlot`** for the Hip→Duffle content-risk (not yet done), and PT1's 2-client sweep (deliberately deferred as its own session). Read `Docs/Planning/B1_UIDesignSession_2026-07-30.md` first if picking any screen up cold.

## Known tooling gotchas (worth remembering)

- No PIE-input automation path exists — every manual test in `B0_ChecklistAndDecisions_2026-07-26.md` genuinely needs your hands.
- A live PIE Details panel can show a replicated `TArray`-of-struct component property (e.g. `BodyZones`) as empty/greyed even when the real data is correct — cross-check with a `ZS.DebugList*` console command instead of trusting the panel.
- Live Coding does **not** reliably pick up changes under `Source/ZombieShooter/Tests/`, or any header change (new `UPROPERTY`/`UFUNCTION`) — full `Build.bat` rebuild for both.
- The automation-test command's `-log=name.log` argument didn't take effect in practice 2026-07-30, but did work correctly 2026-08-01 (two separate runs, both produced the named file) — inconsistent, cause unknown. Check the default `Saved/Logs/ZombieShooter.log` too if a named log file appears missing.
- A component added to an actor post-spawn via `NewObject`+`RegisterComponent()` does **not** reliably get `BeginPlay()` called in a synthetic (non-PIE) world — needs to be a real constructor subobject instead, or `DispatchBeginPlay()` called explicitly.
- **`UWidget` already declares its own `Slot` property** (`TObjectPtr<UPanelSlot> Slot` — the panel slot a widget occupies in its parent). Naming anything `Slot` in a `UWidget`-derived class collides with it — as a `UPROPERTY` it's a hard UHT error ("shadowing is not allowed"); as a plain local variable inside a member function it's MSVC's C4458 ("declaration hides class member"), which this project's build treats as an error too, not just a warning. Confirmed both flavors 2026-08-02 building the B1 widget classes. Avoid `Slot` as an identifier anywhere in `UI/` — use `GearSlot`, `ItemSlotWidget`, etc. instead.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation (`ZS.Debug*` console commands, muzzle-trace debug draw, on-screen hit confirmations) still needs removing once real UI exists — don't remove yet, actively used for testing.
