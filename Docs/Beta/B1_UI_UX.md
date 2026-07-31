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

Dev-only, non-scriptable steps (see `Docs/Beta/README.md`'s convention note). Update in place as they're completed.

### B1-T1 — Input-mode switching

**Status as of 2026-07-30**: C++ side is done (`UZSUIManager`, commit `aee5eb1`). `IA_UISelect`/`IA_UICancel`/`IA_UINavigate` already exist as `.uasset`s with the correct Value Types (Boolean/Boolean/Axis2D, confirmed via `unreal-mcp`) — **but their key mappings are currently sitting inside `IMC_ZS_Default`** (confirmed via `ObjectTools.get_properties` on its `defaultKeyMappings`) instead of a dedicated `IMC_ZS_UI`, which doesn't exist yet. Left as-is, this defeats the whole point: `IA_UISelect`→LMB and `IA_Attack`→LMB would sit in the *same* context at the *same* priority, so both fire on every click regardless of whether a modal is open — there's no priority difference to consume one over the other within a single context. `IA_UINavigate`'s row also has no key assigned yet (`key: "None"`).

1. **Remove the 3 misplaced rows from `IMC_ZS_Default`.** Open `/Game/ZS/Input/IMC_ZS_Default` in the Content Browser (double-click). In the Input Mapping Context editor, find the rows `IA_UISelect` → `LeftMouseButton`, `IA_UICancel` → `Escape`, and `IA_UINavigate` → `None`. Click the small trash-can/✕ icon at the right edge of each row to delete it. `Ctrl+S` to save. (Leave every other row alone — `IA_Attack`'s own `LeftMouseButton` mapping stays exactly where it is.)

2. **Create `IMC_ZS_UI`.** In the Content Browser, navigate to `/Game/ZS/Input/`. Right-click empty space → **Input → Input Mapping Context**. Name it `IMC_ZS_UI`. Double-click to open it.

3. **Add the mappings**, using **Add** / the **+** next to Mappings for each row:
   - `IA_UISelect` → Key `Left Mouse Button`. No triggers, no modifiers (same as `IA_Attack`'s own binding in `IMC_ZS_Default`).
   - `IA_UICancel` → Key `Escape`. No triggers, no modifiers.
   - `IA_UINavigate` (Axis2D) → **4 separate rows**, one per arrow key. Mirror `IA_Move`'s existing W/A/S/D setup exactly (verified via `unreal-mcp` inspection of the live asset) — same modifier classes, same order:

     | Key | Modifiers (in order) |
     |---|---|
     | Up Arrow | `Swizzle Input Axis Values` |
     | Down Arrow | `Swizzle Input Axis Values`, then `Negate` |
     | Left Arrow | `Negate` |
     | Right Arrow | *(none)* |

     This is exactly `IA_Move`'s W (Swizzle only) / S (Swizzle then Negate) / A (Negate only) / D (no modifiers) pattern — Right/Up map to the raw digital-key default (X+/after-swizzle Y+), Left/Down flip the sign. To add a modifier to a row: click the **+** under that row's **Modifiers** list, pick the class from the dropdown. Order matters (top-to-bottom = evaluation order) — Swizzle before Negate on Down Arrow, matching S.
   - `Ctrl+S` to save.

4. **Regen project files + full rebuild** — this adds a brand-new `UCLASS` (`Source/ZombieShooter/UI/`), not a Live Coding patch. Exact commands in `Docs/CommandReference.md` (close the editor first — check nothing's holding the build open, then `Build.bat -projectfiles`, then the normal `Build.bat ZombieShooterEditor Win64 Development`).

5. **Run the automation suite once** to confirm `ZS.UI.ModalStackOrdering` actually passes (new test, pure state logic, no PIE needed) — command in `Docs/CommandReference.md`'s "Editor close/rebuild for automation test runs" section.

6. **PT1 in PIE** (hands-only, no automation path exists for this):
   - `ZS.UI.PushTestModal Test` while mid-attack (e.g. holding the trigger on an auto weapon) — attack should stop dead, no leaked shots.
   - `ZS.UI.PopTestModal Test` at the exact moment of a click — that click should not fire an attack.
   - Spam `PushTestModal`/`PopTestModal` rapidly (different tags each time) — no stuck cursor, no stuck input mode, no error spam.
   - Nested modal: `ZS.UI.PushTestModal A` then `ZS.UI.PushTestModal B`, then `ZS.UI.PopTestModal B` — should land back on `A` (each command logs `IsAnyModalActive` and the tag to the output log; watch there since there's no on-screen readout yet).
   - Try popping a tag that isn't on top (e.g. push `A` then `B`, then `PopTestModal A`) — should log a mismatch warning but still pop the real top (`B`), not corrupt the stack.
   - 2-client: disconnect one client with a modal open — the other client's UI state should be unaffected (this is per-local-player state, not replicated).
   - Confirm T1.5 throughout every step above: zombies keep moving, needs keep decaying, the player remains attackable while a test modal is "open." Nothing should visibly pause.
