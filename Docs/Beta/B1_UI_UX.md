# B1 — UI/UX Foundation, HUD & Input Modes

**Stage 1 — Core Playable Loop.** **Size: L (14–18 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B0** · **Blocks: B4/B4X (can't evaluate a world you can't read), B11**

> **Rescoped 2026-07-26** (`Docs/Planning/RescopeQuestionnaire.md`): the infection-legibility requirement below (T3.3) is **reversed** from the original design — show it plainly, don't hide it. The dev also expressed a concrete UI preference for the inventory screen (T5): separate equipment-slot drag targets alongside a general carry container, not necessarily a single flat scrollable list — see T5 below. Per the dev's process answers, treat every remaining "your call" item in this file as a checkpoint to raise before building, not something to guess past.

> **Gap-review pass 2026-07-30** (`Docs/Beta/90_OpenQuestions.md`): gamepad support for beta is now **cut** (`OQ-B9-01` overturned) — `T2.4`'s generic focus-navigation deliverable is unchanged, but its rationale is keyboard-accessibility only now, not gamepad-prep, and the exit-criteria bullet below is reworded accordingly. Four new HUD/menu items landed from the same pass: `T3.9` (scoreboard/player-list), `T3.10` (notifications/toast), `T3.11` (save/autosave indicator), and `T8` gained a Steam friends-invite line plus a new loading-screen task. **T1 itself is untouched by this pass** — none of the above changes anything about the in-progress input-mode-switching work.

> **Why this is a phase and not a polish item.** Six systems are built and invisible: P2's needs (`OnHungerChanged` has no consumer), P3's wounds and infection, P5's hotbar and jam state, P6's entire inventory (containers do "loot all" on interact *specifically because* no UI exists), P3's death flow, and P2's sleep readiness. Their phase exit criteria — "hunger/thirst **visibly** degrades performance," "full scavenge loop in graybox" — are unreachable without UI. **This phase is the instrument panel for everything already built.**
>
> Builds on `Docs/Planning/UI_Plan.md` (draft, dev read-through still pending per `SessionHandoff.md`). That doc's §2 correctly identifies input-mode switching as blocking everything else.

---

## Entry criteria

- [ ] B0 complete (**solo** exit criteria — 2-client PIE verification is deliberately carried forward into this phase's own exit sweep, see below) — `FZSItemInstance` exists (widgets need a stable identity to bind to and a drag/drop target that survives a move).
- [ ] B0-T4.9 done — need severity thresholds authored, or moodles have no tiers to render.
- [ ] `Docs/Planning/UI_Plan.md` read and its §7 open questions resolved (folded into `90_OpenQuestions.md` as OQ-B1-*).
- [x] **OQ-B1-01 resolved 2026-07-26 (dev-confirmed)** — functional-grey now, restyle after B2. No colour literals outside the style asset.

## Exit criteria

- [ ] A player can read every simulated stat the game tracks without opening the console.
- [ ] Left-click means "select" over a menu and "attack" otherwise, with no input leaking through in either direction.
- [ ] Full loot loop is playable through UI: open container → inspect items → take individual items → manage weight → close.
- [ ] Two clients each drive their own UI without cross-talk; no widget reads a replicated value by polling.
- [ ] **No screen hardcodes a mouse-only interaction** — every drag/click action also has a keyboard-driven path. This is a standing keyboard-accessibility requirement independent of gamepad support, which is cut for v1 (`OQ-B9-01`, overturned 2026-07-30).
- [ ] No modal screen pauses the game — real-time is non-negotiable per Decision 1.

> **Carried forward from B0 (dev decision, 2026-07-30):** B0's 2-client PIE verification was deferred here rather than dropped — debug-console-only feedback made judging what a second client sees impractical, and it's far more legible once this phase's HUD/menus exist. Fold these into this phase's exit sweep alongside the UI-specific 2-client bullet above: PT1 (2-client baseline: fire/reload/aim/sprint/crouch/hotbar/melee/loot/drop from both clients), PT6 (full stage sweep A–G under 2 clients + a 30-minute unscripted co-op session), bag-nesting replication, cross-player ammo pickup/reload, respawn-loadout co-op parity, and PT4 scenario (e) (simultaneous-fire noise events). Source detail: `B0_Stabilization.md` Exit criteria and `B0_ChecklistAndDecisions_2026-07-26.md`'s ⏸-marked items.

---

## Task breakdown

### B1-T1 — Input-mode switching · **S (2–3 sessions)** · *blocks every other B1 task*

The foundational piece. `GameDevPlan.md` §7 cross-cutting Q6 already specified the correct mechanism; this implements it.

| Sub-task | Definition of done |
|---|---|
| T1.1 | `IMC_ZS_UI` mapping context created. Left-click → UI select, plus navigation/cancel actions. |
| T1.2 | `UZSUIManager` (a `ULocalPlayerSubsystem`) owns a **modal stack**. Pushing the first modal `AddMappingContext`s `IMC_ZS_UI` at higher priority than `IMC_ZS_Default`; popping the last removes it. **A stack, not a bool** — inventory can open a container which can open a confirm dialog. |
| T1.3 | Input mode, mouse cursor visibility, and `AZSPlayerCharacter`'s cursor-facing behaviour all follow the stack state. Cursor-facing must not fight the mouse while a menu is focused. |
| T1.4 | **Verified:** with a menu open, left-click never triggers `HandleAttack`; with no menu, left-click never activates a stale widget. Test by opening a menu mid-attack and closing it mid-click. |
| T1.5 | **The game does not pause.** Zombies keep moving, needs keep decaying, and the player remains attackable while any menu is open. This is the design pillar and it must be tested explicitly, not assumed. |

### B1-T2 — Widget architecture & design tokens · **S (2 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T2.1 | `WBP_ZS_*` base classes established; a common style asset (colours, type scale, spacing) so restyling in B2/B7 is one file, not fifty. |
| T2.2 | **Every widget binds to an `OnXChanged` delegate. No widget polls replicated state** — this is the project's replication convention applied to UI, and violating it is the most likely source of co-op UI desync. |
| T2.3 | A widget-pooling policy for list-heavy screens (inventory grids), so opening a container doesn't allocate per-open. |
| T2.4 | **Focus navigation implemented generically at the base-class level, not per-screen.** Build the mechanism and verify it with keyboard arrows/tab. **Gamepad support is cut for v1** (`OQ-B9-01`, overturned 2026-07-30) — this is now a pure keyboard-accessibility requirement, not gamepad-prep architecture; build it because every screen needs a non-mouse path regardless. |

### B1-T3 — HUD · **M (4–5 sessions)** · *depends on T2*

Always-on, non-modal, never eats input.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | **Moodle stack** — designed for **N needs, not 8.** The list has already grown from 6 to 8 (CR-03); assume it grows again. 4 severity tiers each, driven by B0-T4.9's authored thresholds. | P2-R6 |
| T3.2 | **Health/wound display** — 4 zones, wound type, bleed state, splinted/amputated. Must make **critical head bleed (B0-T5.3) unmistakably urgent** or it's an invisible death. | P3-R4 |
| T3.3 | ⚑ **Infection legibility — REVERSED 2026-07-26 (dev-confirmed).** The original constraint here was "identical signals, no UI element names the tier." That's gone. Build the opposite: **clearly show the player when they've been bitten and when an infection (either tier) is active** — a legible "Infected: Bite"-style indicator is now correct, not a design violation. | P3-R2, CR-06 |
| T3.4 | **Hotbar** — 9 slots, current selection, equip-in-progress state, durability indication, **jam state (B0-T10.2)**. | P5-R1 |
| T3.5 | **Ammo counter** reading from the inventory ammo stack (post-B0-T2.11), not from weapon actor state. | P5-R6 |
| T3.6 | **Interaction prompt** — the world-space "E — Open" widget P1 specified and never shipped. Consumes `OnNearestInteractableChanged`. | P1-R7 |
| T3.7 | **Transparent stat preview** (Notes §21 pillar) — hovering an item shows its actual mechanical effect, not hidden numbers. Establish the pattern here; every later screen inherits it. | — |
| T3.8 | HUD is legible at both zoom extremes from B0-T3.1. Test at min and max, not just default. | — |
| T3.9 | **New, added 2026-07-30.** Scoreboard/player-list screen — shows connected players. Doubles as the target-selection UI for the host admin/moderation tools (`OQ-B10-12`). | OQ-B1-07 |
| T3.10 | **New, added 2026-07-30.** Lightweight, queued toast/notification system — pickup confirmation, horde-approaching alert, player joined/left, and future needs share one widget rather than a bespoke UI per event type. | OQ-B1-04 |
| T3.11 | **New, added 2026-07-30.** Save/autosave indicator — small HUD icon flashes on B3-T2.1's ~10s character-save cadence. | OQ-B1-06 |

### B1-T4 — Interaction & world-space prompts · **S (1–2 sessions)** · *depends on T3*

| Sub-task | Definition of done |
|---|---|
| T4.1 | World-space prompt widget positioned on the nearest interactable, occlusion-aware, readable top-down. |
| T4.2 | Multiple nearby interactables disambiguate clearly (nearest wins, per `UpdateNearestInteractable`). |
| T4.3 | Prompt text is data-driven from `UZSInteractableComponent`, not hardcoded — doors, containers, world items, and future barricades all reuse it. |

### B1-T5 — Inventory screen · **M–L (6–8 sessions, grew from M 4–5 2026-07-30 — see T5.0)** · *depends on T1, T2, T5.0*

The first modal screen; the real test of T1.

> **Layout decided 2026-07-30** — see `Docs/Planning/B1_UIDesignSession_2026-07-30.md` for the full design session. Confirmed structure: a `Tab`-opened Loadout/Stats/Skills menu; Loadout shows Player + Gear (Backpack/Hip/Flashlight) + dedicated weapon-mount slots (2 long-gun + 1 sidearm) + Needs, alongside three inventory compartments (Pockets/Backpack/Duffle, gated by a Small/Medium/Large size-tier rule) — not a flat list, and a full spatial/Tetris grid was explicitly considered and rejected (see that doc for the effort estimate and reasoning). T5's sub-tasks below still need a pass to reflect this once B1 implementation actually starts.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.0 | **New, added 2026-07-30 — data-model prerequisite, land before any T5 widget work starts.** Extends B0-T2's item-instance model rather than reopening it: (1) `EZSCarryLocation` split so Backpack and Duffle are distinct carry compartments instead of one shared `Bag` value; (2) new `EZSItemSize {Small, Medium, Large}` field on `UZSItemConfig`, gating which compartment accepts an item (Pockets: Small only; Backpack: Small+Medium; Duffle: all); (3) new dedicated weapon-mount equip slots (2 long-gun + 1 sidearm) on `AZSPlayerCharacter`/`UZSInventoryComponent`, same `FGuid`-reference pattern already proven for Back/Hip/SecondaryHand — **these mounts are the actual weapon-carry capacity, not cosmetic**: a weapon must occupy a mount slot to be carried at all, and `HotbarSlots` becomes a quick-select pointer into a mounted weapon (or other item) for the active loadout rather than its own capacity check for weapons specifically; (4) Duffle equip slot gated by the same `bIsBusy` mechanism already used for reload/amputation — opening its panel blocks movement/combat until closed. Reuses proven patterns throughout, not new architecture. **Content gap this creates**: every existing item/weapon config needs an `EZSItemSize` value authored — folds into `T_ContinuousTracks.md` T4's content-authoring track. Full design reasoning in `Docs/Planning/B1_UIDesignSession_2026-07-30.md`. | — |
| T5.1 | Grid/list of `CarrySlots`, **grouped by compartment** (Pockets/Backpack/Duffle per T5.0's size-tier gating) — each visually distinct, since B0-T2.9 made location mechanically meaningful. Weapons do not appear in this grid at all — they live in T5.0's mount slots. | CR-09 |
| T5.2 | Weight/encumbrance bar with the threshold where the stamina penalty begins clearly marked. | P2-R5 |
| T5.3 | Drag-and-drop between locations, plus a keyboard/gamepad path for every drag operation. |
| T5.4 | Equip to `Back`/`Hip`; assign to a hotbar slot; drop to world. All operate on `FGuid`, so state (durability, condition) visibly follows the item. | P6-R3 |
| T5.5 | **Condition/durability shown per instance** — two "rare" items with different `ConditionQuality` must look different. This is the payoff for B0-T2.10 and it is invisible without UI. | P6-R2 |
| T5.6 | Item tooltip uses T3.7's transparent-stat-preview pattern. |
| T5.7 | **2-client verified**: player A dropping an item updates player B's world view; neither player's inventory UI reflects the other's. |

### B1-T6 — Container loot screen · **S (2 sessions)** · *depends on T5*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T6.1 | Two-pane container ↔ inventory transfer, reusing T5's visual language and widgets. | P6-R4 |
| T6.2 | **Per-item take replaces "loot all"** — the UI-less bootstrap goes away. Keep "take all" as a convenience button. | P6-R4 |
| T6.3 | **Real-time contest**: two players looting the same container simultaneously must not duplicate items. Server-authoritative transfer, verified 2-client. This is a genuine dupe-bug surface. |
| T6.4 | Looting does not pause the game and the player remains attackable — the primary tension source in the scavenge loop. |

### B1-T7 — Death, respawn & sleep screens · **S (1–2 sessions)** · *depends on T2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T7.1 | Death screen — non-interactive, shows cause of death. Decision on a death-recap screen is `GameDevPlan` §7 P9 Q2 → OQ-B6-07. | — |
| T7.2 | Respawn-as-new-character flow surfaced clearly, including the co-op case. | P3-R11 |
| T7.3 | **Amputation blackout state** has its own visual treatment (B0-T7.2) — the player must understand they are incapacitated and vulnerable, not dead. | P3-R7 |
| T7.4 | Sleep/time-skip prompt showing per-player readiness across `PlayerArray`, plus `IsSafeToSleep()`'s answer **before** the player commits. | P2-R7 |

### B1-T8 — Main menu & pause · **S (2 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T8.1 | Main menu: new game, continue/load (stub until B3), host, join by IP, **Steam friends-list invite** (added 2026-07-30 — `OQ-B10-02` already promised this via Steam networking, wasn't reflected in this task list until now), quit. |
| T8.2 | In-game menu. **In co-op it does not pause** — this needs explicit UX treatment so players understand the world keeps running. Solo pause behaviour is OQ-B1-03. |
| T8.3 | Settings entry point present but stubbed — B9 fills it in. |
| T8.4 | **New, added 2026-07-30.** Simple loading/level-transition screen — tip/lore text, functional-grey placeholder now, real art after B2 (`OQ-B1-05`). |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | Input-mode switching under adversarial use: open a menu mid-attack, close mid-click, spam open/close, open a nested modal, disconnect with a menu open. | Zero input leakage either direction. Nested modals pop in order. No stuck cursor or stuck input mode. |
| **PT2** | End of T3 | **HUD readability run.** 20 minutes at both zoom extremes, letting needs decay into severe tiers and taking wounds in all 4 zones including a critical head bleed. | Every state is readable without the console. Critical head bleed is impossible to miss. **A naive tester immediately knows they've been bitten and can name which infection tier they're in** — per T3.3/CR-06's reversal, legible is correct, ambiguous is the bug. |
| **PT3** | End of T6 | **Full scavenge loop, 2-client** — P6's actual exit criterion, reachable for the first time. Run out, loot under threat, haul back, stash. Both players loot the same container simultaneously. | No dupes. Weight pressure creates real decisions. Looting while threatened feels tense because the game doesn't pause. |
| **PT4** | B1 exit | **30-minute unscripted co-op session with no developer narration.** | A second person can play without being told what anything means. |

---

## Notes and constraints

- **Never pause.** Decision 1 explicitly rejects a pause-and-plan layer. Every modal is real-time. This is the constraint most likely to be violated by accident and it changes the feel of the entire game.
- **Radial quick-use is deferred to B9.** It appears in both source docs but is never detailed, and `UI_Plan.md` ranks it lowest priority. The hotbar already covers instant re-equip.
- **Map screen is B4's**, not B1's — there is no map to draw yet.
- **UI art is B2's decision, layout is B1's.** If OQ-B1-01 says UI ships functional-grey, then T2.1's style asset is the single restyle surface later. Do not scatter colour literals.

---

## Manual setup steps

Dev-only, non-scriptable steps (see `Docs/Beta/README.md`'s convention note). **Format**: each task entry is a running **Completed** list (brief, one line each) followed by **Next steps** (full click-by-click detail). When a next step finishes, its detail comes out of Next steps and a one-line summary gets appended to Completed above it.

### B1-T1 — Input-mode switching

**Completed — T1.1-T1.5 (the solo mechanism) are done:**
- `UZSUIManager` C++ implemented — modal stack, `HandleAttack`/`IsCursorFacingActive` hard-gated directly on `IsAnyModalActive()` (commit `aee5eb1`).
- `IA_UISelect`/`IA_UICancel`/`IA_UINavigate`/`IMC_ZS_UI` all created and mapped correctly in-editor, confirmed by a clean rebuild + a passing `ZS.UI.ModalStackOrdering` automation run (`de612a5`, 2026-08-01) — the 3 misplaced rows are gone from `IMC_ZS_Default`, `IMC_ZS_UI` has the right mappings/modifiers.
- Full project rebuild verified clean, `ZS.` automation suite run clean (only the 5 pre-existing unrelated failures tracked in `SessionHandoff.md`).
- **PT1 solo pass run in PIE, all clean (dev-confirmed 2026-08-01)** — sequential push/fire, pop/fire, spam open/close, nested modal, mismatched-tag pop, and T1.5 (nothing pauses) all passed. Good enough to unblock T2+ (which don't need more than this).

**Next steps:**

1. **Not yet done, not blocking T2+**: PT1's 2-client half. This doc's own carried-forward note (Exit criteria section above) folds the deferred B0 2-client baseline sweep into *this* PT1 checkpoint specifically — fire/reload/aim/sprint/crouch/hotbar/melee/loot/drop from both clients, plus confirming modal state genuinely doesn't cross-talk between them. That's a bigger, separate session (2 PIE clients, walking through existing B0 mechanics, not new T1 work) — worth doing deliberately rather than folding into a quick solo check. Tracked here so it doesn't quietly get dropped; still gates the full B1 exit sweep either way.

### B1-T2 — Widget architecture & design tokens

**Completed:**
- `UZSUIStyleConfig` (`UPrimaryDataAsset`) C++ implemented — colour/type-scale/spacing fields, functional-grey code defaults per OQ-B1-01 (commit `fca75fc`).
- `UZSUserWidgetBase` (`UUserWidget`) C++ implemented — `Style` reference every `WBP_ZS_*` child will point at, plus T2.4's generic arrow-key focus navigation (`NativeOnKeyDown` → `FReply::SetNavigation`). All new API usage verified directly against the UE 5.8 engine source (not from memory) since no compiler was available this pass — see the commit message for exactly what was checked.
- **`DA_ZS_UIStyle_Default` data asset created** (dev-confirmed 2026-08-01) — the one restyle surface OQ-B1-01 calls for now exists.
- **`WBP_ZS_Base` Widget Blueprint created, reparented to `UZSUserWidgetBase`** (dev-confirmed 2026-08-01) — ahead of the original "wait until T3 needs it" note here, dev's call. First real screen (T3's HUD) has a base to build on.

**Next steps:**

1. T2.2 (every widget binds a delegate, never polls) and T2.3 (widget pooling for list-heavy screens) still have no code to write - T2.2 is a convention to hold future widgets to (the T3/T7 delegate audit below is the groundwork for it), T2.3 has nothing to pool until T5's grid exists. Neither is a content gap, just not-yet-applicable until a real widget exists to apply them to.

### B1-T3 / B1-T7 — Delegate audit (groundwork for HUD + sleep prompt)

**Completed:**
- Audited every T3/T7 HUD/screen requirement against existing replicated-state delegates (commit `89f466e`). Found and fixed 3 real gaps that would have blocked a future widget from following T2.2's "bind, never poll" rule:
  - `AZSGameState`: `bSleepRequestPending`/`PendingSleepHours` now broadcast `OnSleepRequestStateChanged` (T7.4's sleep prompt).
  - `AZSWeapon`: `CurrentDurability`/`CurrentConditionQuality` now broadcast `OnDurabilityChanged` (T3.4's hotbar durability indicator).
  - `AZSWeapon`: `CurrentMagazineAmmo`'s existing `OnMagazineAmmoChanged` now actually fires on the host/listen-server, not just remote clients (T3.5's ammo counter).
- Confirmed already-correct, no change needed: T3.6 (interaction prompt — `UZSInteractableComponent::InteractionVerb` + `AZSPlayerCharacter::OnNearestInteractableChanged` already cover it), T3.3 (infection legibility — wound-infection-state changes already route through `OnBodyZonesChanged`).

**Next steps:** none content/editor-related — this was pure C++, folded into the same rebuild as T1/T2. A background task was spawned separately for one same-class bug found but left alone (`CycleFireMode_Implementation` not re-broadcasting on host) since no B1 HUD task reads fire mode yet.

### B1-T5.0 — Inventory/loadout data-model prerequisite

**Completed:**
- `EZSItemSize {Small, Medium, Large}` field added to `UZSItemConfig` (commit `9c77d44`).
- **Hip/Duffle question resolved (dev-confirmed 2026-07-30):** the hip slot now holds a sidearm (pistol), long-gun mounts cosmetically attach to the equipped backpack if present, else the back directly.
- **Data model implemented** (uncompiled as of this entry - see below): `EZSEquipSlot::Hip` removed entirely, replaced by `Duffle` (`{None, Back, Duffle}`) - `Back`/`Duffle` are the two bag-capable gear slots, unchanged general `bIsEquippable`/`CarryCapacityBonus` mechanism. `EZSCarryLocation::Bag` split into `Backpack`/`Duffle`. `UZSInventoryComponent` gained `EquippedDuffle` (renamed from `EquippedHip`) and 3 new weapon-mount fields: `MountedLongGuns` (fixed 2-element `TArray<FGuid>`) + `MountedSidearm` (single `FGuid`), each a pure GUID reference into `CarrySlots` - **no `AZSWeapon` actor lifecycle**, mounting is a carry-capacity gate only, same as `EquippedBack`/`EquippedDuffle`, not the same thing as actively equipping via `CurrentWeapon`/hotbar. `Server_MountLongGun`/`Server_UnmountLongGun`/`Server_MountSidearm`/`Server_UnmountSidearm` validate via `UZSWeaponConfig::Handedness` (`TwoHanded` → long-gun slots, `OneHanded` + `AttackType::Ranged` → sidearm slot, excluding a one-handed melee weapon like a knife) - inferred from existing weapon-config fields, no new weapon-category field added. `Server_StoreInBag` now rejects `UZSWeaponConfig` instances outright (weapons excluded from all 3 compartments) and picks `Backpack` vs `Duffle` by the target bag's own `EquipSlot`. Fixed every downstream reference (`ZSNeedsComponent`'s insulation sum, one automation test).
- **Deviated from the plan written here 2026-07-30 in one way, on purpose:** that version said the sidearm mount "probably reuses the renamed `Sidearm` equip slot directly." Implemented differently - `MountedSidearm` is its own field, not routed through `EZSEquipSlot`/`Server_EquipToSlot` at all. Reasoning: `Server_EquipToSlot`'s validation (`bIsEquippable`/`CarryCapacityBonus`) is a *bag* concern; a weapon mount's validation (`UZSWeaponConfig` type + `Handedness`) is a different one, and conflating them would have meant `Server_StoreInBag` needing extra special-casing to keep excluding weapons from bag storage anyway. Kept the two systems parallel instead - lower risk, cleaner separation matching how `SecondaryHandInstanceId` already lives on `AZSPlayerCharacter` as its own thing rather than folding into the bag-slot system.

**Next steps:**

~~Get a real compile.~~ Done 2026-08-01 - full rebuild clean, `ZS.` automation suite run clean (`de612a5`; caught and fixed one real bug along the way, `ZS.UI.ModalStackOrdering` needed a valid `ULocalPlayer` outer chain - see `SessionHandoff.md`).

1. **Check existing content for a silent reinterpretation risk** (not yet done). `EZSEquipSlot::Hip` occupied raw enum value `2`; `Duffle` now occupies that same position (`{None=0, Back=1, Duffle=2}`). Any already-authored `UZSItemConfig`/`UZSWeaponConfig` instance whose `EquipSlot` was set to `Hip` will likely deserialize as `Duffle`, not fail loudly. `Content/ZS/Items/DA_Bag.uasset` (untracked, your own in-progress content) is the one to check first - open it and confirm `EquipSlot` still reads as the value you intended.
2. **`HotbarSlots` becomes a quick-select pointer into a mount, not its own capacity check — deliberately still not started.** Turns out to be bigger than originally scoped: there's currently no runtime mechanism to assign an arbitrary `CarrySlots` instance into a hotbar slot at all - `HotbarSlots` is only ever populated once, at `BeginPlay`, from `StartingHotbarLoadout`. Making a mounted weapon hotbar-selectable means either building that assignment mechanism now (real new scope, arguably more T5-widget-adjacent than data-model) or deferring the actual wiring until T5's inventory screen exists to trigger it. Worth a call before starting - not guessed past.
3. Duffle's `bIsBusy` gate ("opening its panel blocks movement") isn't wired anywhere yet - there's no panel to open (T5's job). The equip slot itself works today; the busy-gate is UI-triggered behavior with nothing to trigger it yet.
