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
| T3.1 | ⚑ **Moodle stack — MOVED OFF THE HUD 2026-08-01 (dev-confirmed).** Not a HUD element anymore - lives in the Loadout tab's Needs section instead (T5.1's "Needs bars" region), matching the design session's "Needs live inside the Loadout panel, not on the main HUD" note literally rather than as a HUD/detail-view split. `WBP_ZS_MoodleStack`/`WBP_ZS_MoodleEntry` are built once, placed in T5's Inventory screen. Still N-needs-not-8 (CR-03), still 4 severity tiers off B0-T4.9's thresholds - only the placement changed. | P2-R6 |
| T3.2 | ⚑ **Health/wound display — MERGED WITH T3.3 AND SCOPED TO GAMEPLAY-AFFECTING STATE ONLY, 2026-08-02 (dev-confirmed).** Was a 4-zone health/wound panel plus a separate infection line, both always visible. Now **one widget, `WBP_ZS_BodyConditionIndicator`, Collapsed entirely by default** — it only shows a zone icon when that zone currently has a state that actually changes a gameplay multiplier: bleeding (health drain), a fracture (mobility), amputation (permanent penalty), or an infected wound (worsens bleed/slows recovery). A plain scratch with no bleed/infection stays hidden — it's cosmetic, nothing reads it mechanically. **No raw numeric health bar anywhere on the HUD anymore** — flagged as an open question below, not solved here. Must still make **critical head bleed (B0-T5.3) unmistakably urgent**. Top-left corner. | P3-R4 |
| T3.3 | ⚑ **Infection legibility — REVERSED 2026-07-26, then FOLDED INTO T3.2's single indicator 2026-08-02 (both dev-confirmed).** Still shows plainly when bitten and when an infection (either tier — bite or wound) is active — that requirement is unchanged. What changed is *where*: no longer its own separate HUD line, now one more state `WBP_ZS_BodyConditionIndicator` (T3.2) surfaces, so there's a single top-left element for "is anything currently wrong with my body," not two. | P3-R2, CR-06 |
| T3.4 | ⚑ **Hotbar — REPLACED 2026-08-01 (dev-confirmed) with a single equipped-item indicator, bottom-right.** No 9-slot grid, no free assignment. Weapons are key-mapped directly: **1 = Primary, 2 = Pistol, 3 = Secondary** (fixed to the 3 weapon-mount slots - mounting a weapon is what makes it key-selectable, no separate "assign to hotbar" step exists anymore). The HUD widget just shows one icon for whatever `CurrentWeapon` currently is, plus its durability/jam state (`B0-T10.2`). | P5-R1 |
| T3.5 | ⚑ **Ammo counter — CUT 2026-08-01 (dev-confirmed).** Not built. `AZSWeapon::OnMagazineAmmoChanged`/reserve-ammo access still exist in C++ if this is revisited later, just not surfaced in B1's HUD. | ~~P5-R6~~ |
| T3.6 | **Interaction prompt** — the world-space "E — Open" widget P1 specified and never shipped. Consumes `OnNearestInteractableChanged`. | P1-R7 |
| T3.7 | **Transparent stat preview** (Notes §21 pillar) — hovering an item shows its actual mechanical effect, not hidden numbers. Establish the pattern here; every later screen inherits it. | — |
| T3.8 | HUD is legible at both zoom extremes from B0-T3.1. Test at min and max, not just default. | — |
| T3.9 | ⚑ **Scoreboard — CUT 2026-08-01 (dev-confirmed).** Not built for B1. `AZSGameState::OnPlayerListChanged`/`PlayerListVersion` stay in C++ (harmless, small, may still matter for future host admin/moderation tools per `OQ-B10-12`) but nothing in the UI surfaces them. | ~~OQ-B1-07~~ |
| T3.10 | **New, added 2026-07-30.** Lightweight, queued toast/notification system — pickup confirmation, horde-approaching alert, player joined/left, and future needs share one widget rather than a bespoke UI per event type. | OQ-B1-04 |
| T3.11 | **New, added 2026-07-30.** Save/autosave indicator — small HUD icon flashes on B3-T2.1's ~10s character-save cadence. | OQ-B1-06 |
| T3.12 | **New, added 2026-08-01 (dev-confirmed).** Equipment slot indicator — a 4th quick-equip slot (grenades/quick-use equipment), key-bound to **G**, separate from the 3 weapon-mount keys and from SecondaryHand. Shown as part of the same bottom-right equipped-item area as T3.4, or immediately beside it. | — |

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
| T5.0 | **New, added 2026-07-30 — data-model prerequisite, land before any T5 widget work starts.** Extends B0-T2's item-instance model rather than reopening it: (1) `EZSCarryLocation` split so Backpack and Duffle are distinct carry compartments instead of one shared `Bag` value; (2) new `EZSItemSize {Small, Medium, Large}` field on `UZSItemConfig`, gating which compartment accepts an item (Pockets: Small only; Backpack: Small+Medium; Duffle: all); (3) new dedicated weapon-mount equip slots (2 long-gun + 1 sidearm) on `AZSPlayerCharacter`/`UZSInventoryComponent`, same `FGuid`-reference pattern already proven for Back/Hip/SecondaryHand — **these mounts are the actual weapon-carry capacity, not cosmetic**: a weapon must occupy a mount slot to be carried at all. ⚑ *Superseded 2026-08-01: the original plan here ("`HotbarSlots` becomes a quick-select pointer into a mounted weapon") was replaced by a simpler design — 3 fixed keys (1/2/3) map directly to the 3 mount slots, no separate hotbar-array/quick-select layer exists at all. See T3.4's redesign note.* (4) Duffle equip slot gated by the same `bIsBusy` mechanism already used for reload/amputation — opening its panel blocks movement/combat until closed. Reuses proven patterns throughout, not new architecture. **Content gap this creates**: every existing item/weapon config needs an `EZSItemSize` value authored — folds into `T_ContinuousTracks.md` T4's content-authoring track. Full design reasoning in `Docs/Planning/B1_UIDesignSession_2026-07-30.md`. | — |
| T5.1 | Grid/list of `CarrySlots`, **grouped by compartment** (Pockets/Backpack/Duffle per T5.0's size-tier gating) — each visually distinct, since B0-T2.9 made location mechanically meaningful. Weapons do not appear in this grid at all — they live in T5.0's mount slots. ⚑ **Also hosts the Needs bars (`WBP_ZS_MoodleStack`) as of 2026-08-01** — see T3.1's redesign note; needs display was moved here rather than the main HUD. | CR-09 |
| T5.2 | Weight/encumbrance bar with the threshold where the stamina penalty begins clearly marked. | P2-R5 |
| T5.3 | Drag-and-drop between locations, plus a keyboard/gamepad path for every drag operation. |
| T5.4 | ⚑ **REDESIGNED 2026-08-01 (dev-confirmed): equip to `Back`/`Duffle`; mount into a weapon-mount slot (which is what makes it key-selectable via 1/2/3 - no separate "assign to hotbar slot" step exists anymore); assign to the new Equipment slot (T3.12, G key); drop to world.** All operate on `FGuid`, so state (durability, condition) visibly follows the item. | P6-R3 |
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
| T7.3 | ⚑ **Superseded 2026-08-10, then CUT 2026-08-11 (both dev-confirmed).** The blackout mechanic this task was written for is gone — amputation is now just a temporary mobility penalty (`bIsAmputationShocked`), decoupled from any incapacitated state, and 0 HP enters the new downed/revive state instead (`UZSHealthComponent::bIsDowned`). A downed-state overlay (`WBP_ZS_BlackoutOverlay`/`UZSBlackoutOverlayWidget`) briefly existed for this on 2026-08-05/06 but was removed entirely 2026-08-11 per dev instruction — **downed state currently has no dedicated visual treatment at all.** Revisit as its own task if that's wanted before B1 exit. | P3-R7 |
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

## Outstanding testing, and why each item is still open

Written 2026-08-11 as a single reference point for what's genuinely untested before B1 can formally close, grouped by *why* it's still open — the blocker is almost always structural (needs 2 people, needs a feature that doesn't exist yet, needs sustained play to reach a rare state), not neglect. Cross-check against `SessionHandoff.md` for anything that's moved since this was written.

### Blocked on 2 simultaneous PIE clients + a person driving each
No PIE-input-automation path exists for this project — simulated input doesn't reliably reach a pawn, and `unreal-mcp` can't drive a second real client either — so this whole category needs two humans (or one dev alternating hands-on-keyboard) at once, which is why it's been carried forward, session after session, rather than actually run.
- **PT1's 2-client half** — fire/reload/aim/sprint/crouch/hotbar/melee/loot/drop from both clients, confirming the modal stack (T1) genuinely doesn't cross-talk between them.
- **T5.7 / Exit criterion "two clients, no UI cross-talk"** — player A's drop updates player B's world view; neither player's own UI reflects the other's.
- **T6.3 / PT3's dupe check** — two players looting the *same* container at the same instant, confirmed no duplicate item. `ZS.Inventory.ContainerTakeItemIsDupeSafe` covers the server-logic half in isolation; a live 2-client contest is a different, still-open test.
- Everything the B0→B1 carry-forward note (Exit criteria section above) bundles in: PT6 (full stage sweep A–G under 2 clients + a 30-minute unscripted session), bag-nesting replication, cross-player ammo pickup/reload, respawn-loadout co-op parity, PT4 scenario (e) simultaneous-fire noise events.

### Blocked on a genuinely naive second player
- **PT4** — "30-minute unscripted co-op session with no developer narration." By definition this can't be the dev or an AI walking through known systems — it needs someone who's never seen the UI before, a real scheduling ask rather than a quick PIE pass.

### Blocked on sustained play to reach a specific, sometimes-rare state
Not spot-checkable — these need real time in-game deliberately chasing a state:
- **PT2** — HUD readability at both zoom extremes over 20 minutes, letting needs decay into severe tiers and taking wounds in all 4 body zones *including* a critical head bleed (`bCriticalBleed` is a rare roll on a fresh Head bleed, not guaranteed). Whether "every stat is readable without opening the console" actually holds is a judgment call a human has to make while playing, not something a compile or unit test can answer.

### Blocked on a feature that isn't built yet — not testable until it exists
- **T5.3 / Exit criterion "no mouse-only interaction"** — T2.4 built generic keyboard focus-navigation, but the keyboard/gamepad equivalent for *drag-and-drop itself* (moving an item between compartments, equipping from a slot) was flagged back on 2026-08-02 as still undecided and never picked back up. This blocks the exit criterion outright, independent of any test — there's nothing to test yet.
- **T3.11** (save/autosave indicator) — blocked on B3's save system existing at all; nothing to bind to.
- 4 optional UMG animations across T3/T5 (`CriticalBleedFlash`, `FadeInOut`, `SeverityPulse`, `Spin`) — none built. All `BindWidgetAnimOptional`, so nothing is broken by their absence, just less polish than a real PT2 pass will eventually want.

### Blocked only on the dev's own solo PIE session — no special setup needed
Lower-stakes than the above, just hasn't happened yet:
- **2026-08-11's reload change** — single R still reloads normally (now with a ~0.25s hold before it commits), double-tap R discards and quick-reloads instead. Needs a rebuild + a boundary test, not a feel check: a single R press followed by a pause **longer** than `QuickReloadDoubleTapWindowSeconds` (0.25s default, `TuningReference.md`) must always commit normal reload, never quick; R-R **within** that window must always commit quick reload, never normal. If either miscategorizes at the boundary, retune the tunable — this is an input-classification correctness check, not a subjective feel pass.
- **2026-08-11's `BP_ZS_HUD` fix** — `DeathScreenClass` is now correctly assigned (verified via a property read, not PIE), but nobody has actually died in PIE since the fix to confirm the screen itself displays.
- The general backlog of "no PIE testing done on this widget yet" notes scattered through the per-task sections above (written at the time each widget was built) — several have since been spot-confirmed in later sessions (compartment reshape, melee mount, pause menu, container loot), but a single pass across every B1 screen in one sitting has never happened.

### Already solo-verified — only the 2-client half is missing
Listed separately so it's clear these aren't fully untested, just not multiplayer-verified yet:
- T1 (input-mode switching, modal stack, no-pause-while-modal) — PT1 solo pass passed clean 2026-08-01.
- Full loot loop (open → inspect → take → manage weight → close) — functionally confirmed working solo, including today's container-loot fix, but PT3's 2-client no-dupe sweep specifically hasn't run.

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

### B1-T3 — HUD

> ⚑ **Redesigned 2026-08-01, dev-confirmed, while widget-building was already underway.** No moodle stack on the HUD (moved to T5's Loadout panel), no 9-slot hotbar grid (replaced with one equipped-item icon, key-mapped straight to the 3 mount slots), no ammo counter, no scoreboard. See T3.1/T3.4/T3.5/T3.9's rows in the task-breakdown table above for the exact per-subtask disposition, and commit `6d413ac` for the underlying C++ (`AZSPlayerCharacter`'s weapon-key-slot rewrite).
>
> ⚑ **Further redesigned 2026-08-02, dev-confirmed.** T3.2's health/wound display and T3.3's infection indicator, previously two always-visible top-left elements, are now **one** `WBP_ZS_BodyConditionIndicator`, Collapsed by default, that only shows a zone when its state actually affects gameplay (bleed/fracture/amputation/wound-infection), plus bite-infection stage folded into the same widget. See T3.2/T3.3's rows above. **Needs zero new C++** — every field this reads (`bBleeding`/`WoundType`/`bAmputated`/`bCriticalBleed`/`WoundInfectionState` via `Get Zone Wound`, and `OnInfectionStageChanged`) was already exposed for the old two-widget version.

**Completed — compiled and automation-tested, commit `6d413ac`:**
- `UZSItemConfig`/`UZSWeaponConfig::GetStatPreviewLines()` (T3.7, and T5.6 reuses it) — the transparent-stat-preview contract, `BlueprintNativeEvent` so weapon configs append Damage/Fire Rate/Magazine on top of the base Hunger/Thirst/Carry Capacity/Insulation lines.
- New `UZSNotificationSubsystem` (T3.10) — client-local toast queue, same C++-state/Blueprint-presentation split as `UZSUIManager`. `AZSGameState::Multicast_ShowToast` + `AZSGameMode::PostLogin`/`Logout` wire up player-joined/left as the one concrete trigger so far. `ZS.UI.PushTestToast [message]` console command for pre-widget testing.
- **Weapon-key redesign**: `AZSPlayerCharacter::ResolveWeaponSlotInstance(SlotIndex)` resolves 0/1/2 (keys 1/2/3) live from `UZSInventoryComponent::GetMountedLongGun(0)`/`GetMountedSidearm()`/`GetMountedLongGun(1)`, and 3 (key G) from the new `EquipmentSlotInstanceId`. The old free-form `HotbarSlots` array and its manual-assignment API (`Server_AssignHotbarSlot`/`Server_ClearHotbarSlot`/`CanAssignToHotbarSlot`) are gone entirely — mounting a weapon (T5.4) is now the only "assignment" step there is. `OnActiveHotbarIndexChanged`/`OnBusyChanged`/`GetActiveHotbarIndex()` are unchanged and are what T3.4's equipped-item icon binds to.
- New Equipment slot (T3.12): `Server_AssignEquipmentSlot`/`Server_ClearEquipmentSlot`/`GetEquipmentSlotInstanceId()`/`OnEquipmentSlotChanged`, mirrors `SecondaryHandInstanceId`'s pattern. Scoped to `UZSWeaponConfig` instances only this pass — see `AZSPlayerCharacter.h`'s Equipment-slot section comment for why a genuinely non-weapon "other equipment" item isn't dispatchable through it yet (`CompleteHotbarSwitch` only knows how to equip a `UZSWeaponConfig`). A grenade is expected to be authored as one (`AttackType::Ranged` + `ProjectileClass`, reusing `AZSProjectile`'s existing simulated-projectile mechanism for the actual throw) rather than needing new dispatch logic.
- T3.2 (health/wound), T3.3 (infection), T3.6 (interaction prompt) needed **no new code** — the T3/T7 delegate audit already covers every bind point.
- **Required manual step, not yet done**: create `IA_EquipItem` (G key) — same graceful-if-missing pattern as every other Input Action in this project; the C++ finder is already in place and no-ops safely until the asset exists.
- Full rebuild clean, `ZS.` automation suite (32 tests) run clean — 2 new tests (`ZS.Loadout.WeaponKeySlotsResolveFromMounts`, `ZS.Loadout.EquipmentSlotRequiresWeaponConfig`) replace the removed hotbar-assignment test, only the same 5 pre-existing failures remain.

**Completed — 2026-08-02, uncompiled (see `SessionHandoff.md` for live compile/PIE status):**
- Following the `UZSBodyConditionIndicatorWidget` trial (confirmed a real win), every remaining T3 widget moved the same way: `UZSStatPreviewLineWidget`/`UZSStatPreviewTooltipWidget`, `UZSEquippedItemIndicatorWidget`, `UZSInteractionPromptWidget`, `UZSToastEntryWidget`/`UZSToastListWidget` — all `UZSUserWidgetBase` subclasses in `Source/ZombieShooter/UI/` with `BindWidget`/`BindWidgetAnimOptional` auto-binding and `NativeConstruct()` doing all delegate wiring. Nothing left to hand-wire in these widgets' Graph tabs.
- ~~Uncompiled.~~ **Rebuilt clean 2026-08-02** — 2 real errors hit and fixed along the way (both `UWidget::Slot` shadowing, one on a `UPROPERTY` in `UZSEquipSlotWidget.h`, one on local variables in `ZSCompartmentPanelWidget.cpp`/`ZSContainerLootWidget.cpp` — see `SessionHandoff.md`'s tooling gotchas), commits `057cbd5`/`c1f5472`. `ZS.` automation suite not re-run this pass — still owed.

**Completed — dev-confirmed in-editor, 2026-08-02:**
- `WBP_ZS_BodyConditionIndicator`'s Designer-tab hierarchy is built and reparented correctly, but its 5 Class Defaults texture slots (Wounded/Bleeding/Fracture/Infected/Amputated Icon) are **still unassigned** — needs placeholder textures (any functional-grey icon is fine for B1), flagged explicitly by dev to remember for a real-art pass later.
- **No animations built anywhere in T3 yet** (`CriticalBleedFlash` on Body Condition Indicator, `FadeInOut` on Toast Entry) — both are optional (`BindWidgetAnimOptional`), so nothing is blocked, but neither exists yet.

**Next steps:**

1. **Build the actual `WBP_ZS_*` widgets** — Designer-tab hierarchy (exact `BindWidget` names) + Class Defaults only now, no Graph-tab wiring. Full per-widget steps and exact hierarchy diagrams: UI Build Manifest artifact. **Assign `WBP_ZS_BodyConditionIndicator`'s 5 placeholder icon textures** — the one concrete unfinished item from the in-editor progress above.
2. **T3.11 (save/autosave indicator) is blocked**, not attempted — B3's save system doesn't exist yet, nothing to bind to. Revisit once B3 lands.
3. T3.8 (legibility at both zoom extremes) is a testing pass once the widgets above exist, not a code task. **No PIE testing has been done on any T3 widget yet** — everything so far is Designer-tab-only progress.
4. **Open question, not decided**: with no raw numeric health bar anywhere (HUD dropped it 2026-08-02, and T5's Inventory screen never had one), the player has no numeric HP readout at all — only wound/bleed/fracture state via `WBP_ZS_BodyConditionIndicator`, PZ-style. Confirm that's intentional before B1 exit, or add one somewhere (likely T5.1's player panel).

### B1-T5.0 — Inventory/loadout data-model prerequisite

**Completed:**
- `EZSItemSize {Small, Medium, Large}` field added to `UZSItemConfig` (commit `9c77d44`).
- **Hip/Duffle question resolved (dev-confirmed 2026-07-30):** the hip slot now holds a sidearm (pistol), long-gun mounts cosmetically attach to the equipped backpack if present, else the back directly.
- **Data model implemented** (uncompiled as of this entry - see below): `EZSEquipSlot::Hip` removed entirely, replaced by `Duffle` (`{None, Back, Duffle}`) - `Back`/`Duffle` are the two bag-capable gear slots, unchanged general `bIsEquippable`/`CarryCapacityBonus` mechanism. `EZSCarryLocation::Bag` split into `Backpack`/`Duffle`. `UZSInventoryComponent` gained `EquippedDuffle` (renamed from `EquippedHip`) and 3 new weapon-mount fields: `MountedLongGuns` (fixed 2-element `TArray<FGuid>`) + `MountedSidearm` (single `FGuid`), each a pure GUID reference into `CarrySlots` - **no `AZSWeapon` actor lifecycle**, mounting is a carry-capacity gate only, same as `EquippedBack`/`EquippedDuffle`, not the same thing as actively equipping via `CurrentWeapon`/hotbar. `Server_MountLongGun`/`Server_UnmountLongGun`/`Server_MountSidearm`/`Server_UnmountSidearm` validate via `UZSWeaponConfig::Handedness` (`TwoHanded` → long-gun slots, `OneHanded` + `AttackType::Ranged` → sidearm slot, excluding a one-handed melee weapon like a knife) - inferred from existing weapon-config fields, no new weapon-category field added. `Server_StoreInBag` now rejects `UZSWeaponConfig` instances outright (weapons excluded from all 3 compartments) and picks `Backpack` vs `Duffle` by the target bag's own `EquipSlot`. Fixed every downstream reference (`ZSNeedsComponent`'s insulation sum, one automation test).
- **Deviated from the plan written here 2026-07-30 in one way, on purpose:** that version said the sidearm mount "probably reuses the renamed `Sidearm` equip slot directly." Implemented differently - `MountedSidearm` is its own field, not routed through `EZSEquipSlot`/`Server_EquipToSlot` at all. Reasoning: `Server_EquipToSlot`'s validation (`bIsEquippable`/`CarryCapacityBonus`) is a *bag* concern; a weapon mount's validation (`UZSWeaponConfig` type + `Handedness`) is a different one, and conflating them would have meant `Server_StoreInBag` needing extra special-casing to keep excluding weapons from bag storage anyway. Kept the two systems parallel instead - lower risk, cleaner separation matching how `SecondaryHandInstanceId` already lives on `AZSPlayerCharacter` as its own thing rather than folding into the bag-slot system.

- ~~Get a real compile.~~ Done 2026-08-01 - full rebuild clean, `ZS.` automation suite run clean (`de612a5`; caught and fixed one real bug along the way, `ZS.UI.ModalStackOrdering` needed a valid `ULocalPlayer` outer chain - see `SessionHandoff.md`).
- ~~`HotbarSlots` runtime-assignment mechanism.~~ **Superseded 2026-08-01** — the free-form assignment API built here (`Server_AssignHotbarSlot`/`Server_ClearHotbarSlot`/`CanAssignToHotbarSlot`) was removed entirely by the weapon-key redesign (see T3's section below and commit `6d413ac`). Mounting a weapon is now the only "assignment" there is - a mounted weapon is automatically selectable via its fixed key (1/2/3), no separate step. `GetHotbarSlots()` is also gone (there's no array left to read).

**Next steps:**

1. **Check existing content for a silent reinterpretation risk** (not yet done). `EZSEquipSlot::Hip` occupied raw enum value `2`; `Duffle` now occupies that same position (`{None=0, Back=1, Duffle=2}`). Any already-authored `UZSItemConfig`/`UZSWeaponConfig` instance whose `EquipSlot` was set to `Hip` will likely deserialize as `Duffle`, not fail loudly. `Content/ZS/Items/DA_Bag.uasset` (untracked, your own in-progress content) is the one to check first - open it and confirm `EquipSlot` still reads as the value you intended.
2. Duffle's `bIsBusy` gate ("opening its panel blocks movement") isn't wired anywhere yet - there's no panel to open (T5's job below). The equip slot itself works today; the busy-gate is UI-triggered behavior with nothing to trigger it yet.

### B1-T5 — Inventory screen

> ⚑ **T5.1/T5.4 redesigned 2026-08-01, dev-confirmed** — see T3's redesign note above. T5.1 now also hosts the Needs bars (`WBP_ZS_MoodleStack`, relocated off the HUD). T5.4's "assign to a hotbar slot" step is gone - mounting a weapon (already part of T5.4) is the only assignment step now; a new Equipment-slot drop target (T3.12) is added instead.

**Completed — compiled and automation-tested, commit `6d413ac`:**
- `UZSInventoryComponent::GetSlotsInLocation()` (T5.1) — the Pockets/Backpack/Duffle compartment filter T5's grid groups by.
- New `UZSDragDropPayload` (T5.3) — reusable `UDragDropOperation` subclass (`InstanceId` + `SourceKind` + `SourceIndex`/`SourceEquipSlot`) every T5 item widget's `OnDragDetected` builds and every drop target's `OnDrop` reads; also reusable by T6's container transfer.
- `UZSNeedsComponent::GetMoodleEntries()`/`OnMoodleStackChanged` (now T5.1's Needs bars, not a HUD element) — one delegate + one array accessor covering Hunger/Thirst/Fatigue/Stamina/Temperature, so a 6th/7th need is a new entry in that function, not a new widget class.
- T5.2 (weight bar), T5.5 (condition/durability per instance) needed **no new code** — `GetCurrentWeight()`/`GetMaxCarryWeight()` and `FZSItemInstanceState::ConditionQuality` were already `BlueprintPure`/`BlueprintReadWrite`, directly bindable from a "Break" node in Blueprint.
- T5.6 reuses T3.7's `GetStatPreviewLines()` — see the T3 section above, nothing T5-specific to add.
- Full rebuild clean, `ZS.` suite (32 tests) run clean.

**Completed — 2026-08-02, uncompiled (see `SessionHandoff.md` for live compile/PIE status):**
- Same C++-widget-class treatment as T3, applied to every T5 widget: `UZSItemSlotWidget` (drag-detect builds the `UZSDragDropPayload`, eagerly creates+populates its own `UZSStatPreviewTooltipWidget` and calls `SetToolTip` — no manual hover wiring needed), `UZSCompartmentPanelWidget` (also now genuinely enforces the "hide Backpack/Duffle panel unless that gear slot is equipped" rule the old Blueprint-only design never actually wired), `UZSWeightBarWidget`, `UZSEquipSlotWidget`/`UZSWeaponMountSlotWidget`/`UZSEquipmentSlotWidget` (each `NativeOnDrop` reads the payload and calls the matching `Server_` function directly), `UZSMoodleEntryWidget`/`UZSMoodleStackWidget`, and the root `UZSInventoryScreenWidget` (`OpenAsModal`/`CloseAsModal` wrap the modal-stack push/pop, tab-switcher wiring included).
- **Bug fix found while wiring the drop targets**: `UZSInventoryComponent::Server_EquipToSlot`/`Server_MountLongGun`/`Server_MountSidearm` are plain `HasAuthority()`-gated calls, not real RPCs — calling them straight from a client-owned widget (as this doc's own now-superseded next-steps text below used to say to do) silently no-ops on a non-host client. Fixed with new `Server, Reliable` wrappers on `AZSPlayerCharacter` (same name, forwards to the Inventory Component), which is what `UZSEquipSlotWidget`/`UZSWeaponMountSlotWidget`/`UZSEquipmentSlotWidget` actually call now.
- ~~Uncompiled.~~ **Rebuilt clean 2026-08-02** — same rebuild as T3's, see that section's note for the 2 build errors hit and fixed (`UWidget::Slot` shadowing).
- **Two real manifest gaps found and fixed while the dev was actually building this in-editor, 2026-08-02**: (1) `Canvas_Loadout` (the Loadout tab's own page inside `Switcher_Tabs`) wasn't a named node in the manifest's hierarchy diagram at all — the player-silhouette `Image` and the `VBox_Compartments`/`VBox_PlayerCompartments` wrapper Vertical Boxes are now explicit, named nodes instead of vaguely-described "place these somewhere in the Loadout panel" prose. (2) The `WBP_ZS_ItemSlot`/`WBP_ZS_MoodleEntry`/`WBP_ZS_ToastEntry`/`WBP_ZS_StatPreviewLine`/`WBP_ZS_SleepPlayerRow` cards all incorrectly said "check Size To Content" for a widget placed inside a Wrap/Vertical/Horizontal Box — that's a Canvas Panel child-slot property and doesn't exist there; fixed to recommend a **Size Box** (Width/Height Override or Min Desired Width/Height) instead, the actual correct UMG pattern.

**Completed — dev-confirmed in-editor, 2026-08-02:**
- `WBP_ZS_ItemSlot` built (with the corrected 64×64 Size Box) — the last widget actually finished this session.
- `WBP_ZS_Inventory` is in progress — was stuck on its open/close-toggle step (originally too vague: "keep a reference... Close As Modal" without saying how a reference survives across two separate key-press events). **Elaborated in the manifest** into a concrete `InventoryScreenRef` variable + `IsValid`/`IsInViewport` branch pattern; resume there.
- **No animations built anywhere in T5 yet** (`SeverityPulse` on Moodle Entry) — optional, non-blocking, same as T3's.
- **No PIE testing done on any T5 widget yet.**

**Next steps:**

1. **Resume building `WBP_ZS_Inventory`** at its now-elaborated open/close-toggle step, then continue through its remaining child widgets — Designer-tab hierarchy (exact `BindWidget` names) + Class Defaults only, no Graph-tab wiring except the toggle logic itself. Full per-widget steps and exact hierarchy diagrams: UI Build Manifest artifact. Keyboard-driven equivalents for every drag target (T5.3's standing accessibility requirement) still need deciding/building — the C++ drop handlers don't cover input-method choice.
2. **T5.7 (2-client verified)** — folds into the same B1 exit sweep as PT1's 2-client half (see this doc's Exit criteria section); not a separate task, just don't forget it's still owed.
3. **Non-weapon Equipment-slot items** (a quick-use consumable that isn't a `UZSWeaponConfig`) — explicitly **not** built. `Server_AssignEquipmentSlot` only accepts a weapon-config instance today; broadening it to arbitrary items is new scope needing its own call, not guessed past here (see `AZSPlayerCharacter.h`'s Equipment-slot section comment).

### B1-T6 — Container loot screen

**Completed — compiled and automation-tested, commit `00d3633`:**
- `AZSContainerActor::Server_TakeItem(FGuid InstanceId, AZSPlayerCharacter* Requester)` (T6.2) — GUID-exact per-item take, replacing nothing (see below) but adding the capability T6.2 needed. Dupe-safe by construction: server RPCs execute serially, so a second take of an already-removed `InstanceId` just fails its lookup — verified by `ZS.Inventory.ContainerTakeItemIsDupeSafe` (T6.3's requirement, no extra locking needed).
- `AZSContainerActor::Server_TakeAllItems(AZSPlayerCharacter* Requester)` — the original "loot all" transfer, extracted and renamed so it's reusable by both the existing auto-interact path (unchanged behavior) and a future "Take All" button.
- `AZSContainerActor::Server_AddItemToContainer(FZSItemInstance)` — a real production capability (hand-place a guaranteed item in a specific container), also what seeds test data without depending on a real `UZSLootTableConfig` content asset.
- `AZSContainerActor::OnContainerSlotsChanged` — `ContainerSlots` had replication but no real change delegate before this; a T6 widget binds this instead of polling `GetContainerSlots()`.
- `AZSPlayerCharacter::Server_TakeContainerItem`/`Server_TakeAllContainerItems` — real `Server, Reliable` RPCs (unlike the container-level functions above, which are plain `HasAuthority()`-gated calls, not RPCs themselves) so a client's UI can actually reach the server. Same reasoning `Server_SelectHotbarSlot` already establishes for weapon-key selection.

**Completed — 2026-08-02, uncompiled (see `SessionHandoff.md` for live compile/PIE status):**
- `UZSContainerLootWidget` (`UZSUserWidgetBase` subclass) — reuses T5's `UZSItemSlotWidget`/`UZSDragDropPayload` (`EZSDragSourceKind::Container`) directly. Binds `OnContainerSlotsChanged`, its Take All button calls `Server_TakeAllContainerItems`, `OpenAsModal`/`CloseAsModal` wrap the modal stack. Per-item take is now automatic — `UZSItemSlotWidget::NativeOnMouseButtonDown` calls `Server_TakeContainerItem` itself when `SourceKind == Container`, so the old separate "per-item take" card is gone from the manifest, folded into `UZSItemSlotWidget`'s own behavior.

**Next steps:**

1. **Build `WBP_ZS_ContainerLoot`** — Designer-tab hierarchy + Class Defaults only, no Graph-tab wiring. Full steps: UI Build Manifest artifact.
2. **Open UX question, not decided:** should interacting with a container open this screen, or keep auto-looting everything (today's unchanged behavior)? `Docs/Planning/B1_UIDesignSession_2026-07-30.md` explicitly says container-loot UX "was not mocked up this session" - worth a call before rewiring `OnInteract`.
3. T6.4 (no pause while looting) needs no code - inherent, same as every other B1 screen. Testing pass only, once the widget exists.

### B1-T7 — Death, respawn & sleep screens

**Completed — compiled and automation-tested, commit `00d3633`:**
- New `FZSDeathInfo` (`Zone`/`WoundType`/`InstigatorLabel`) + `UZSHealthComponent::LastDeathInfo`/`GetLastDeathInfo()` (T7.1) — "cause of death" didn't exist anywhere before this. Captured on every hit in `Server_ApplyDamage` (plain `Replicated`, set before `bIsDead` in the same call, same pattern `AZSGameState::PendingSleepHours` already uses) - reflects the most recent hit, which for bleed-out/infection deaths (never routed through `Server_ApplyDamage` - see `TickBleed`/`TickInfection`) is the last discrete hit rather than the literal final tick. `InstigatorLabel` resolves to a player name, `"Zombie"` (an instigator with no `PlayerState`), or `"Unknown"` (no instigator at all).
- `AZSPlayerCharacter::GetRespawnDelaySeconds()` (T7.2) — `RespawnDelaySeconds` was `EditAnywhere`-only, not Blueprint-readable at all before this.
- `AZSGameState::GetSleepReadyCounts(int32& OutReady, int32& OutTotal)` (T7.4) — a "2/4 ready" readout without every consumer re-deriving the same `PlayerArray` cast-and-count loop `UpdateSleepRequestState` already does internally.
- T7.3 (blackout) needed **no new code** — `IsBlackedOut()`/`OnBlackoutChanged` already existed and were already public/`BlueprintAssignable`.

**Completed — 2026-08-02, uncompiled (see `SessionHandoff.md` for live compile/PIE status):**
- `UZSDeathScreenWidget` (binds `OnDeath`, formats cause-of-death from `GetLastDeathInfo()`, runs the local respawn countdown itself via a repeating timer), `UZSBlackoutOverlayWidget` (no `BindWidget` properties at all — just toggles its own root Visibility off `OnBlackoutChanged`), `UZSSleepPlayerRowWidget` + `UZSSleepPromptWidget` (binds `OnSleepRequestStateChanged`, loops `GameState->PlayerArray` casting each pawn to `AZSPlayerCharacter` to build the ready-row list, gates the toggle button on `IsSafeToSleep()`, `Btn_ToggleReady` calls `ToggleSleepReady()` directly). All three are `UZSUserWidgetBase` subclasses, all Graph-tab logic gone.

**Next steps:**

1. **Build the widgets** — Designer-tab hierarchy + Class Defaults only, no Graph-tab wiring for any of them (`WBP_ZS_SleepPlayerRow` needs building first as `WBP_ZS_SleepPrompt`'s row template). Full steps: UI Build Manifest artifact.
2. **Open question (OQ-B6-07):** whether a fuller death-recap screen (stats, kill feed) is wanted beyond plain cause-of-death - not decided, `GetLastDeathInfo()` covers only what's built.

### B1-T8 — Main menu & pause

**Completed — compiled and automation-tested, commit `00d3633`:**
- New `UZSGameInstance` (`UGameInstance`) — the project had **no GameInstance subclass at all** before this (confirmed by search). `HostGame(FName MapName)` (`UGameplayStatics::OpenLevel(..., "listen")`) and `JoinGame(const FString& IPAddress)` (`APlayerController::ClientTravel`) cover T8.1's host/join-by-IP - neither needs an online subsystem.
- `OnLoadingScreenShouldShow`/`OnLoadingScreenShouldHide` (T8.4), bound to `FCoreUObjectDelegates::PreLoadMap`/`PostLoadMapWithWorld` in `Init()` - fires automatically around every level transition, no manual trigger needed per `OpenLevel` call site.
- T8.2 (in-game menu) and T8.3 (settings stub) need **no new C++** - T8.2 reuses `UZSUIManager::PushModal`/`PopModal` directly, T8.3 is a placeholder button with nothing to configure yet.
- **Steam friends-list invite (T8.1) deliberately NOT built** - `OnlineSubsystemSteam` isn't wired into `ZombieShooter.Build.cs` at all (still commented out, per `CLAUDE.md`'s Off-Limits: no dedicated-server/online subsystem yet). Enabling it is an infrastructure decision (Steam AppID, plugin enablement) for you to make, not guessed past here.

**Completed — 2026-08-02, uncompiled (see `SessionHandoff.md` for live compile/PIE status):**
- `UZSMainMenuWidget` (wires Host/New Game/Join/Settings/Quit buttons directly to `GetOwningZSGameInstance()`/`UKismetSystemLibrary::QuitGame`; `Btn_Continue` deliberately untouched, stays disabled until B3), `UZSPauseMenuWidget` (wires Resume/Settings/Quit-to-Menu; `OpenAsModal`/`CloseAsModal` wrap the modal stack, same pattern as the Inventory/Container screens), `UZSLoadingScreenWidget` (binds `OnLoadingScreenShouldShow`/`Hide`, plays/stops the optional `Spin` animation). All three are `UZSUserWidgetBase` subclasses. `WBP_ZS_Settings` is the one B1 widget deliberately **not** converted — it's a static placeholder button with nothing to bind, no win from moving it to C++.

**Next steps:**

1. **Required manual step, not yet done: set `GameInstanceClass` to `ZSGameInstance`** (Project Settings → Maps & Modes, or a Blueprint child of it if you want BP-level tuning). Without this, none of `UZSGameInstance` runs at all - deliberately left undone since `Config/DefaultEngine.ini` already has your own uncommitted changes this pass didn't touch.
2. **Build the widgets** — Designer-tab hierarchy + Class Defaults only for `WBP_ZS_MainMenu`/`WBP_ZS_PauseMenu`/`WBP_ZS_LoadingScreen`, no Graph-tab wiring. `WBP_ZS_Settings` still needs its one manual `Btn_Back` → `Remove from Parent` Graph node, nothing else changed there. Full steps: UI Build Manifest artifact.
3. **Open question (OQ-B1-03):** solo pause behaviour - does a lone host get to actually pause, unlike co-op? Not decided.
4. **Decide on Steam invite** (needs `OnlineSubsystemSteam` wired into `ZombieShooter.Build.cs` + `.uproject` plugin enablement + a Steam AppID) before building it - real infrastructure scope, not a UI-only task.

### B1 — Screen input wiring (Tab/Escape/Sleep toggles + debug commands)

Every `WBP_ZS_*` screen built this session (22 widgets, MCP-driven Blueprint hierarchies) had no way to actually appear in PIE - dev asked for real inputs on the ones with a standalone hotkey, plus a debug toggle on the rest, so all 9 could be visually verified without needing a human to force each real trigger condition (die, get amputated, host a game, find a container).

**Completed — C++ written 2026-08-05, compiled clean:**
- `AZSPlayerCharacter::ToggleInventoryScreen`/`TogglePauseMenuScreen`/`HandleSleepKeyPressed` (new, in the "B1 - Screen toggles" section near `GetUIManager()`) - lazily create-then-reuse `WBP_ZS_Inventory`/`WBP_ZS_PauseMenu`/`WBP_ZS_SleepPrompt` from 3 new `TSubclassOf` Class Defaults (`InventoryScreenClass`/`PauseMenuScreenClass`/`SleepPromptScreenClass`), bound to 2 new `UInputAction*` members (`ToggleInventoryAction`/`TogglePauseMenuAction`) the same graceful-if-missing `ConstructorHelpers::FObjectFinder` pattern as every other action in this file. `TryCloseTopmostScreen()` gives Escape its dual role from `Docs/InputBindings.md` ("Main Menu" / "UI Cancel", same key) - closes whichever of Inventory/ContainerLoot/SleepPrompt/PauseMenu is open instead of stacking Pause on top, entirely via C++ state (not by which Enhanced Input mapping context happens to be layered on top - see the header comment on `TogglePauseMenuAction` for why that distinction matters). `SleepAction` now binds to `HandleSleepKeyPressed` instead of `ToggleSleepReady` directly - it calls the unchanged `ToggleSleepReady()` and additionally toggles `WBP_ZS_SleepPrompt` open/closed client-side, so pressing Sleep both readies-up and shows the group's status.
- New `Source/ZombieShooter/UI/ZSUIDebugCommands.cpp` - 5 `ZS.UI.Toggle*` console commands (`MainMenu`/`LoadingScreen`/`DeathScreen`/`BlackoutOverlay`/`ContainerLoot`), same "no PIE-input automation path exists, give the dev a console command instead" reasoning as `ZS.SpawnZombies`. For the screens with an already-correct real delegate (`UZSHealthComponent::OnDeath`, `AZSPlayerCharacter::OnBlackoutChanged`, `UZSGameInstance::OnLoadingScreenShouldShow`/`Hide`), the debug command broadcasts that SAME delegate rather than reaching into widget internals, so the debug toggle exercises the exact real code path. `WBP_ZS_Settings` needs no command of its own - reachable via `WBP_ZS_MainMenu`/`WBP_ZS_PauseMenu`'s already-wired Settings button once either of those is open.

**Completed — 2026-08-06 away session, both items originally left unwired are now resolved, compiled clean:**
1. **Container-interact UX decided (dev call): real loot screen, not auto-loot-all.** `AZSContainerActor::HandleInteracted` now calls the new `AZSPlayerCharacter::Client_OpenContainerLoot(AZSContainerActor*)` (a `Client, Reliable` RPC - interact itself only ever runs server-side, but the screen has to open on the interacting player's own machine) instead of `Server_TakeAllItems` directly. Lazily creates `WBP_ZS_ContainerLoot` from a new `ContainerLootScreenClass` Class Default the same way every other B1 screen does, then `SetContainer()` + `OpenAsModal()`. `Server_TakeAllItems`/`Server_TakeAllContainerItems` are unchanged - still what the loot screen's own "Take All" button calls. `ContainerLootScreenRef` is now included in `TryCloseTopmostScreen()`'s priority chain, so Escape closes it too. `ZS.UI.ToggleContainerLoot` still works (layout/style check without a real container nearby), its description updated to say so rather than "the only way to see this screen."
2. **The "exists at match start" gap for MainMenu/LoadingScreen/DeathScreen/BlackoutOverlay is closed.** Two different lifetimes needed, so two different owners:
   - **New `AZSHUD` class** (`Source/ZombieShooter/Framework/ZSHUD.h/.cpp`, subclass of `AHUD`) creates `WBP_ZS_DeathScreen`/`WBP_ZS_BlackoutOverlay` in `BeginPlay()` from 2 new Class Defaults (`DeathScreenClass`/`BlackoutOverlayClass`) and adds both to the viewport. `AHUD` is the standard engine answer to "per-player, created once, survives a pawn respawn" - it's owned by the `PlayerController`, not the `Pawn`, so a respawn (which destroys/recreates only the Pawn) doesn't touch it, and the engine never spawns one for a remote proxy so no extra local-player guard is needed. `AZSGameMode::HUDClass` now resolves `BP_ZS_HUD` if it exists (same graceful `ConstructorHelpers::FClassFinder` pattern as `DefaultPawnClass`), falling back to the raw `AZSHUD` class otherwise.
   - **`UZSGameInstance`** (the one object guaranteed to survive `OpenLevel`/`ClientTravel`) creates `WBP_ZS_LoadingScreen`/`WBP_ZS_MainMenu` in `Init()` from 2 new Class Defaults (`LoadingScreenClass`/`MainMenuScreenClass`), via the `CreateWidget(UGameInstance*, ...)` overload rather than needing a live `PlayerController` (which isn't guaranteed to exist yet this early). MainMenu is added to the viewport immediately, matching its own header comment ("Added directly to the viewport at game start"); `HostGame`/`JoinGame` now `RemoveFromParent()` it first - `AddToViewport()` content survives a non-seamless `OpenLevel` by default, so this removal is load-bearing, not defensive. LoadingScreen is never explicitly shown/hidden from `UZSGameInstance` - its own `NativeConstruct` already binds `OnLoadingScreenShouldShow`/`Hide`, existing is all it needs.

**Full rebuild run this away session (`Build.bat`, Mode A) - succeeded clean, first attempt, zero errors.**

**Next steps:**

1. **Required manual step, not yet done: create 2 new Input Actions and bind them in `IMC_ZS_Default`** - `IA_ToggleInventory` (Digital bool, Tab, Pressed) and `IA_TogglePauseMenu` (Digital bool, Escape, Pressed). Same graceful-if-missing pattern as every other Input Action in this project - the C++ finders are already in place and no-op safely until these exist. Deliberately in `IMC_ZS_Default`, not `IMC_ZS_UI` - see the header comment on `TogglePauseMenuAction` in `ZSPlayerCharacter.h` for why.
2. **Required manual step, not yet done: assign Class Defaults across 3 Blueprints:**
   - `BP_ZS_PlayerCharacter`: `InventoryScreenClass` → `WBP_ZS_Inventory`, `PauseMenuScreenClass` → `WBP_ZS_PauseMenu`, `SleepPromptScreenClass` → `WBP_ZS_SleepPrompt`, `ContainerLootScreenClass` → `WBP_ZS_ContainerLoot`.
   - **New Blueprint needed**: `BP_ZS_HUD` (parent `AZSHUD`, in `/Game/ZS/Framework/` to match the `ConstructorHelpers::FClassFinder` path in `AZSGameMode`'s constructor) - assign `DeathScreenClass` → `WBP_ZS_DeathScreen`. Without this Blueprint, `AZSGameMode` falls back to the raw `AZSHUD` class, which has `DeathScreenClass` unset - `BeginPlay()` just skips creating the widget, no crash, but the screen doesn't exist. (`BlackoutOverlayClass`/`WBP_ZS_BlackoutOverlay` no longer exist - the downed-state overlay was removed entirely 2026-08-11 per dev instruction, not renamed; see `SessionHandoff.md`.)
   - `BP_ZS_GameInstance` (or wherever `GameInstanceClass` ends up pointing, per T8's still-open manual step below) - assign `LoadingScreenClass` → `WBP_ZS_LoadingScreen`, `MainMenuScreenClass` → `WBP_ZS_MainMenu`.
3. **Still required, unchanged from T8's own Next steps above: set `GameInstanceClass` to `ZSGameInstance`** (Project Settings → Maps & Modes) - none of the `Init()` widget-creation above runs without this either.
4. **First PIE pass on all of the above** - confirm Tab/Escape/Sleep toggle their screens, a real container interact opens `WBP_ZS_ContainerLoot` (not the old auto-loot), MainMenu shows at boot and hides on Host/Join, LoadingScreen shows during a level transition, and the Death Screen actually displays on a real death. Spot-check the remaining debug commands too. Nothing above has been PIE-tested yet - away-session ceiling is "compiles clean," not "verified working" (`AsyncSessionProtocol.md`).
