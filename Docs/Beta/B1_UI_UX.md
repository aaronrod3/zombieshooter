# B1 — UI/UX Foundation, HUD & Input Modes

**Size: L (14–18 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B0** · **Blocks: B4 (can't evaluate a world you can't read), B11**

> **Why this is a phase and not a polish item.** Six systems are built and invisible: P2's needs (`OnHungerChanged` has no consumer), P3's wounds and infection, P5's hotbar and jam state, P6's entire inventory (containers do "loot all" on interact *specifically because* no UI exists), P3's death flow, and P2's sleep readiness. Their phase exit criteria — "hunger/thirst **visibly** degrades performance," "full scavenge loop in graybox" — are unreachable without UI. **This phase is the instrument panel for everything already built.**
>
> Builds on `Docs/Planning/UI_Plan.md` (draft, dev read-through still pending per `SessionHandoff.md`). That doc's §2 correctly identifies input-mode switching as blocking everything else.

---

## Entry criteria

- [ ] B0 complete — `FZSItemInstance` exists (widgets need a stable identity to bind to and a drag/drop target that survives a move).
- [ ] B0-T4.9 done — need severity thresholds authored, or moodles have no tiers to render.
- [ ] `Docs/Planning/UI_Plan.md` read and its §7 open questions resolved (folded into `90_OpenQuestions.md` as OQ-B1-*).
- [ ] **OQ-B1-01 answered** (UI art direction — do UI tokens come from B2's art lock, or does UI ship functional-grey and get restyled later?).

## Exit criteria

- [ ] A player can read every simulated stat the game tracks without opening the console.
- [ ] Left-click means "select" over a menu and "attack" otherwise, with no input leaking through in either direction.
- [ ] Full loot loop is playable through UI: open container → inspect items → take individual items → manage weight → close.
- [ ] Two clients each drive their own UI without cross-talk; no widget reads a replicated value by polling.
- [ ] **No screen hardcodes a mouse-only interaction** — every drag/click action also has a keyboard-driven path. (Gamepad *verification* is deferred to B9 per OQ-B9-01, but this structural requirement stays: it is an accessibility requirement regardless, and it is what makes B9 cheap instead of a rewrite.)
- [ ] No modal screen pauses the game — real-time is non-negotiable per Decision 1.

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
| T2.4 | **Focus navigation implemented generically at the base-class level, not per-screen.** Build the mechanism; do **not** verify it on a gamepad (deferred to B9, OQ-B9-01). This is the one piece of gamepad architecture kept deliberately — one base-class implementation now costs hours, retrofitting it across every screen in B9 costs a week. Verify it with keyboard arrows/tab, which also satisfies the accessibility requirement. |

### B1-T3 — HUD · **M (4–5 sessions)** · *depends on T2*

Always-on, non-modal, never eats input.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | **Moodle stack** — designed for **N needs, not 8.** The list has already grown from 6 to 8 (CR-03); assume it grows again. 4 severity tiers each, driven by B0-T4.9's authored thresholds. | P2-R6 |
| T3.2 | **Health/wound display** — 4 zones, wound type, bleed state, splinted/amputated. Must make **critical head bleed (B0-T5.3) unmistakably urgent** or it's an invisible death. | P3-R4 |
| T3.3 | **⚑ Infection ambiguity constraint.** Wound infection and bite infection must surface through *identical* signals (nausea, temperature, weakness). **No UI element may name which tier is active.** This is a CONFIRMED design intent that a well-meaning "Infected: Bite" label destroys entirely. | P3-R2, OQ-B0-07 |
| T3.4 | **Hotbar** — 9 slots, current selection, equip-in-progress state, durability indication, **jam state (B0-T10.2)**. | P5-R1 |
| T3.5 | **Ammo counter** reading from the inventory ammo stack (post-B0-T2.11), not from weapon actor state. | P5-R6 |
| T3.6 | **Interaction prompt** — the world-space "E — Open" widget P1 specified and never shipped. Consumes `OnNearestInteractableChanged`. | P1-R7 |
| T3.7 | **Transparent stat preview** (Notes §21 pillar) — hovering an item shows its actual mechanical effect, not hidden numbers. Establish the pattern here; every later screen inherits it. | — |
| T3.8 | HUD is legible at both zoom extremes from B0-T3.1. Test at min and max, not just default. | — |

### B1-T4 — Interaction & world-space prompts · **S (1–2 sessions)** · *depends on T3*

| Sub-task | Definition of done |
|---|---|
| T4.1 | World-space prompt widget positioned on the nearest interactable, occlusion-aware, readable top-down. |
| T4.2 | Multiple nearby interactables disambiguate clearly (nearest wins, per `UpdateNearestInteractable`). |
| T4.3 | Prompt text is data-driven from `UZSInteractableComponent`, not hardcoded — doors, containers, world items, and future barricades all reuse it. |

### B1-T5 — Inventory screen · **M (4–5 sessions)** · *depends on T1, T2*

The first modal screen; the real test of T1.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | Grid/list of `CarrySlots`, **grouped by `EZSCarryLocation`** — on-person vs. bag are visually distinct sections, since B0-T2.9 made location mechanically meaningful. | CR-09 |
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
| T8.1 | Main menu: new game, continue/load (stub until B3), host, join by IP, quit. |
| T8.2 | In-game menu. **In co-op it does not pause** — this needs explicit UX treatment so players understand the world keeps running. Solo pause behaviour is OQ-B1-03. |
| T8.3 | Settings entry point present but stubbed — B9 fills it in. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | Input-mode switching under adversarial use: open a menu mid-attack, close mid-click, spam open/close, open a nested modal, disconnect with a menu open. | Zero input leakage either direction. Nested modals pop in order. No stuck cursor or stuck input mode. |
| **PT2** | End of T3 | **HUD readability run.** 20 minutes at both zoom extremes, letting needs decay into severe tiers and taking wounds in all 4 zones including a critical head bleed. | Every state is readable without the console. Critical head bleed is impossible to miss. **A naive tester cannot tell which infection tier they have.** |
| **PT3** | End of T6 | **Full scavenge loop, 2-client** — P6's actual exit criterion, reachable for the first time. Run out, loot under threat, haul back, stash. Both players loot the same container simultaneously. | No dupes. Weight pressure creates real decisions. Looting while threatened feels tense because the game doesn't pause. |
| **PT4** | B1 exit | **30-minute unscripted co-op session with no developer narration.** | A second person can play without being told what anything means. |

---

## Notes and constraints

- **Never pause.** Decision 1 explicitly rejects a pause-and-plan layer. Every modal is real-time. This is the constraint most likely to be violated by accident and it changes the feel of the entire game.
- **Radial quick-use is deferred to B9.** It appears in both source docs but is never detailed, and `UI_Plan.md` ranks it lowest priority. The hotbar already covers instant re-equip.
- **Map screen is B4's**, not B1's — there is no map to draw yet.
- **UI art is B2's decision, layout is B1's.** If OQ-B1-01 says UI ships functional-grey, then T2.1's style asset is the single restyle surface later. Do not scatter colour literals.
