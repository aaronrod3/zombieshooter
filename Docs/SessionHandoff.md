# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** Read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B1 — UI/UX Foundation, HUD & Input Modes

**B0 → B1 transition confirmed by dev, 2026-07-30.** Two companion docs for B1: `Docs/Beta/B1_UI_UX.md` (task breakdown, entry/exit criteria) and `Docs/Planning/B1_UIDesignSession_2026-07-30.md` (the actual UI design — HUD philosophy, Tab-menu structure, inventory compartments — decided ahead of implementation, read this before touching any layout work).

## B1 progress — 2026-08-06 (away session, Mode A per `AsyncSessionProtocol.md`)

**Both items flagged as "left alone" at the end of 2026-08-05's session are now resolved and compiled clean.** One design fork (container-interact UX) was asked in-chat and answered immediately (dev picked "real loot screen"), so both went into one cluster:
1. **`AZSContainerActor::HandleInteracted` now opens `WBP_ZS_ContainerLoot` on interact** (via new `AZSPlayerCharacter::Client_OpenContainerLoot`), replacing the old auto-loot-all. `Server_TakeAllItems` unchanged, still what the loot screen's own "Take All" button calls.
2. **New `AZSHUD`** (`Framework/ZSHUD.h/.cpp`) creates `WBP_ZS_DeathScreen`/`WBP_ZS_BlackoutOverlay` at `BeginPlay()` (survives pawn respawn, unlike a widget owned by the pawn); **`UZSGameInstance::Init()`** now creates `WBP_ZS_LoadingScreen`/`WBP_ZS_MainMenu` (survives level travel). Closes the "must exist at match start, nothing creates it" gap from 2026-08-05 for all 4 screens.

**Full `Build.bat` rebuild — succeeded clean, first attempt.** Headless smoke-test launch showed no crashes/asserts — only the expected graceful content-gap errors (new Input Actions/`BP_ZS_HUD` not created yet, logged and skipped, not fatal). The smoke-test process itself never processed its `Quit` command and had to be left running (sandbox blocked killing it) — harmless, doesn't hold the build lock, close it manually next time you're at the keyboard (`Stop-Process -Id 16896 -Force` if it's still there, check with `Get-Process UnrealEditor-Cmd` first since the PID may differ by now).

Commit tagged `[compiled]`, pushed. Full detail: `B1_UI_UX.md`'s "Screen input wiring" subsection.

## B1 progress — 2026-08-05 (kept for history; superseded by the entry above for current status)

**All 22 `WBP_ZS_*` Widget Blueprints in the UI Build Manifest are now built and compiled clean**, via `unreal-mcp` tool calls against the live editor (`UMGToolSet`/`ObjectTools`/`BlueprintTools`) rather than by hand — dev had deleted all previously hand-built WBPs and asked for the full manifest to be executed through MCP instead, in one unattended pass. Every widget was built in Build Order (Tier A leaf widgets first), verified via `GetWidgetDescription` tree dumps before each compile, and compiled clean on the first attempt for all 22.

**All 3 originally-unconfirmed MCP capabilities are now confirmed working, not just theorized:**
1. **Reparenting to real C++ classes with genuine `BindWidget` requirements** — works; `CreateWidgetBlueprint`'s `parentClass` arg reparents at creation, no separate step needed. Every widget compiled clean against real `BindWidget` UPROPERTYs. Also confirmed: `BindWidget` resolution does **not** require `bIsVariable: true` — several widgets came back `bIsVariable: false` from `AddWidget` yet still compiled clean, binding purely by exact WidgetTree name.
2. **Nesting `WBP_ZS_*` Blueprint instances as children of another Blueprint** (Tier B checkpoint, `WBP_ZS_MoodleStack` nesting 5× `WBP_ZS_MoodleEntry`) — works. Pass a nested Blueprint's generated-class refPath (`.../WBP_ZS_X.WBP_ZS_X_C`) as `AddWidget`'s `widgetClass`.
3. **Setting per-instance custom `EditAnywhere` properties on a nested instance** (Tier C, `WBP_ZS_Inventory`'s 6 properties across 5 instances — `GearSlot`/`MountIndex`+`IsSidearm`/`Location` — plus `WBP_ZS_ContainerLoot`'s 3× `Location`) — works via `ObjectTools.set_properties` on the nested instance's own refPath, same as any other widget property.

**Two deviations from the manifest's literal hierarchy tree, both the same established pattern** (insert a `SizeBox` wrapper for an auto-layout child that needs an explicit fixed size, since Overlay/HBox/VBox-slot children have no Position/Size-override of their own):
- `WBP_ZS_SleepPrompt`: inserted `SizeBox_Dialog` (770×590) between `Overlay_0` and `VBox_Dialog`.
- `WBP_ZS_Inventory`: inserted `SizeBox` wrappers around `Image_PlayerSilhouette` (200×352), each of the 5 clothing-slot placeholder Images (64×64), and the melee-placeholder Image (64×192) — none shown as needing one in the manifest's literal diagram, but all are plain `Image`s inside auto-layout Boxes with zero intrinsic size.

**Left unset/deferred for the dev, not guessed:**
- `WBP_ZS_PauseMenu`'s `Main Menu Level Name` Class Default — genuinely ambiguous, since this project has no dedicated main-menu level (single persistent world, B0-T9.4). `WBP_ZS_MainMenu`'s `Target Level Name` **was** set, to `Lvl_ThirdPerson` (the current `GameDefaultMap`, legacy-named from before the TopDown pivot). Both widgets' `Settings Class` **was** set, to `WBP_ZS_Settings`, once that widget existed.
- `WBP_ZS_ContainerLoot`'s 4 top-level Canvas positions — the manifest itself flags these as "eyeballed, never precision-computed" (unlike `WBP_ZS_Inventory`'s exact numbers); used reasonable approximate values in that same spirit, needs a visual check once in-editor.
- `DA_ZS_UIStyle_Default` (the `style` property every widget carries) — still not assigned anywhere, unchanged from before this session.
- `WBP_ZS_BodyConditionIndicator`'s 5 icon textures, and all 4 optional UMG animations — unchanged from before, not touched this session.
- ~~All EventGraph wiring~~ **Tab/Escape/Sleep production input wiring done later this same session** (see below) — Inventory/PauseMenu/SleepPrompt are covered. `ContainerLoot`'s `SetContainer` and its real interact-trigger are still unwired (deliberately — see below), and MainMenu/LoadingScreen/DeathScreen/BlackoutOverlay still have no real "create me at match start" call site (also below).
- **Still no PIE testing done on any of this UI work** — everything above is Designer-tab/Class-Defaults-only progress, same caveat as every prior B1 session note.

**Later the same session, dev asked to make sure every screen has an input and can be toggled in PIE — C++ written, not yet compiled:**
- `AZSPlayerCharacter::ToggleInventoryScreen`/`TogglePauseMenuScreen`/`HandleSleepKeyPressed` (new) give Tab/Escape/Sleep real production wiring — Tab and Escape need 2 new Input Action assets (`IA_ToggleInventory`, `IA_TogglePauseMenu`) the dev must create manually (same MCP limitation as every other Input Action in this project), plus 3 Class Defaults on `BP_ZS_PlayerCharacter` (`InventoryScreenClass`/`PauseMenuScreenClass`/`SleepPromptScreenClass`). Full detail: `B1_UI_UX.md`'s new "B1 — Screen input wiring" subsection.
- New `Source/ZombieShooter/UI/ZSUIDebugCommands.cpp` — 5 `ZS.UI.Toggle*` console commands cover the other screens (MainMenu/LoadingScreen/DeathScreen/BlackoutOverlay/ContainerLoot) for visual verification; for the 3 with a real existing delegate (`OnDeath`/`OnBlackoutChanged`/`OnLoadingScreenShouldShow`+`Hide`, all confirmed already correctly wired from earlier B1 work) the debug command broadcasts that same delegate rather than faking anything.
- **Genuine content gap surfaced, not fixed here**: `WBP_ZS_MainMenu`/`WBP_ZS_LoadingScreen`/`WBP_ZS_DeathScreen`/`WBP_ZS_BlackoutOverlay` all need to exist "at match start" per their own header comments, but nothing anywhere creates an instance of any of them — no HUD-root widget or GameInstance-owned singleton does this yet. Their internal show/hide logic is real and correct; they just have no creation point. Tracked as a separate future task in `B1_UI_UX.md`.
- **Deliberately not wired**: `ContainerLoot`'s real trigger — container-interact UX (auto-loot vs. real loot screen) is still an open, undecided question; wiring it into `TryInteract` would have silently resolved that question rather than let it get a real answer.

## B1 progress — 2026-08-02 (kept for history; superseded by the entry above for current status)

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

**Everything below is compiled clean (2026-08-06 away session) — next is in-editor manual setup, then a first real PIE pass. Nothing has been PIE-tested yet.**

1. **Required manual steps, none done yet (all need a live editor):**
   - Create `IA_ToggleInventory` (Tab, Pressed) and `IA_TogglePauseMenu` (Escape, Pressed), both in `IMC_ZS_Default`. Detail: `Docs/InputBindings.md`'s UI section.
   - Create `BP_ZS_HUD` (parent `AZSHUD`, in `/Game/ZS/Framework/`) and assign its 2 Class Defaults (`DeathScreenClass`→`WBP_ZS_DeathScreen`, `BlackoutOverlayClass`→`WBP_ZS_BlackoutOverlay`).
   - Assign Class Defaults on `BP_ZS_PlayerCharacter` (`InventoryScreenClass`/`PauseMenuScreenClass`/`SleepPromptScreenClass`/`ContainerLootScreenClass`) and on the GameInstance Blueprint (`LoadingScreenClass`/`MainMenuScreenClass`).
   - Still outstanding from T8: set `GameInstanceClass` to `ZSGameInstance` in Project Settings — none of the above runs without it either.
2. **Open the editor and eyeball every screen** — nothing has been screenshot-verified yet, only tree-dump-verified. `WBP_ZS_ContainerLoot`'s 4 top-level positions especially need a look (manifest calls those eyeballed, not computed).
3. **Assign real textures/colors** — every `Image` in the built widgets is still an unset/default brush; `DA_ZS_UIStyle_Default` isn't assigned to any widget's `style` property yet either.
4. **Decide `WBP_ZS_PauseMenu`'s quit-to-menu flow** — its `Main Menu Level Name` Class Default is deliberately left unset; no dedicated main-menu level exists in this single-persistent-world project, so this needs a real design call.
5. **First PIE pass** once the manual steps above are done — confirm Tab/Escape/Sleep toggle their screens, a real container interact opens the loot screen (not the old auto-loot), MainMenu shows at boot and hides on Host/Join, LoadingScreen shows during a level transition, Death/Blackout display on a real death/blackout, and the 5 `ZS.UI.Toggle*` debug commands still work.
6. **`ZS.` automation suite still hasn't been re-run since the 2 Slot-shadowing build-error fixes from 2026-08-02** — worth a run to confirm the 5-pre-existing-failures baseline is unchanged.
7. **One stray background process left running from the smoke test** — a headless `UnrealEditor-Cmd.exe` (was PID 16896) that never processed its `Quit` command; harmless (doesn't hold the build lock), close it manually next time you're at the keyboard.

**Two older loose ends, unchanged:**
1. `WBP_ZS_BodyConditionIndicator`'s 5 icon texture slots are unassigned — any functional-grey placeholder works, swap for real art later.
2. No animations exist anywhere yet (`CriticalBleedFlash`/`FadeInOut`/`SeverityPulse`/`Spin`) — all optional, nothing is blocked, build whenever or skip for B1 entirely.

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
