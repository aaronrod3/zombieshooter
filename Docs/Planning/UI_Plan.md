# UI — Planning Draft

> **Status: DRAFT PROPOSAL, not plan of record.** Written 2026-07-22, unsupervised (dev away), following on from `Docs/Planning/InventoryLoadoutEquipping_Plan.md`. Kept out of `GameDevPlan.md` for the same reason as that doc — nothing here is settled, it's a starting point for review. No UMG/Blueprint content was touched or will be (that's editor work, and this is a planning pass, not a build session) — this is entirely about what C++ surface needs to exist for a UI to bind to, and what the screens should actually contain.
>
> Every UI system this project will ever build already depends on the item-instance work in the companion doc for its inventory/container/hotbar screens specifically — read that one first if you haven't. Sections here that assume it's been approved say so.

## 1. What already constrains this

Three things pin down the shape of the UI before a single widget gets designed:

1. **`DevMarkupNotes.md` §21 (your own note)**: *"Simplify inventory; have an easy stat system to show what items and actions do to the player. Player knowledge is mainly built on common sense and playing the game."* — no hidden numbers, no wiki-required mechanics, minimal chrome. This is a review checklist for every screen below, not just the inventory one.
2. **Decision 1 (`GameDevPlan.md`)**: real-time, no pause-and-plan menus. UI is a modeless overlay on a running world, the same way PZ's own inventory panel doesn't pause a shared multiplayer world. Nothing proposed here should stop simulation.
3. **The dual-purpose left-click question you raised during the P5 design pass** (recorded in `GameDevPlan.md` §7, item 6): *"when interacting with nearby options that open menus, player will select an option in the menu, so left clicking on mouse needs to be able to select an option, but when not in a menu, left click performs an appropriate action."* This was flagged as "not built yet" and nothing has touched it since — but it's not a detail, it's the foundation every other screen below sits on. §2 resolves it.

## 2. Foundational: input-mode switching (blocks everything else)

Right now `LMB` is unconditionally `IA_Attack`. The moment any UI screen needs mouse clicks for its own purpose (selecting an inventory item, clicking a container slot), that binding has to stop reaching gameplay while the screen is open, and resume when it closes. This isn't a new problem UE hasn't solved — it's the standard Enhanced Input pattern, and I'd recommend not inventing something bespoke:

**`RECOMMENDATION`**:
- New Input Mapping Context, `IMC_ZS_UI`, mapping `LMB` (and whatever else a given screen needs) to UMG's own click handling rather than to any `IA_*` gameplay action.
- A small menu-stack on `AZSPlayerController` (or a new `UZSUIManagerSubsystem` if more than one modal screen ever needs to be aware of each other — start with the simplest version and only add the subsystem if a second modal screen actually needs to coordinate with the first): `OpenModalWidget(TSubclassOf<UUserWidget>)`/`CloseModalWidget()`. Opening pushes `IMC_ZS_UI` onto the `UEnhancedInputLocalPlayerSubsystem` (higher priority than `IMC_ZS_Default`, or swap entirely — swap is simpler and matches "you're either playing or you're in a menu," which fits the real-time/no-pause framing better than a layered-priority model that could let both fire at once by accident) and switches `SetInputMode` from `FInputModeGameAndUI`/`FInputModeGameOnly` (whichever `AZSPlayerCharacter`'s current camera perspective already uses — TopDown already runs `FInputModeGameAndUI` with the cursor visible, so this is mostly "also stop `IA_Attack` from firing," not "also show a cursor," for the TopDown case at least) to something that routes clicks to Slate. Closing reverses both.
- Non-modal HUD elements (moodles, hotbar bar, interaction prompt — §4.1) never go through this at all; they're always visible, never eat input, and don't care about `IMC_ZS_UI`.

This single piece needs to exist before the Inventory screen, Container screen, or any future menu can be built at all — call it the literal first UI task, before any widget design.

## 3. What C++ surface already exists vs. what's missing

Because every gameplay system in this project already follows "OnRep_X broadcasts a delegate" (the replication convention, `CLAUDE.md`), most of the HUD is *already wired for a UI to bind to and nothing is using it yet* — the C++ work for the non-modal HUD is close to zero. The inventory/container/loot screens need new C++ because P6 was built UI-less on purpose (see the companion doc). Laying it out plainly:

### Already exists, zero new C++ needed
| Delegate | Lives on | Covers |
|---|---|---|
| `OnHungerChanged`/`OnThirstChanged`/`OnFatigueChanged`/`OnStaminaChanged` | `UZSNeedsComponent` | Moodle stack |
| `FZSOnHealthChanged`-shaped delegate, `OnBodyZonesChanged`, `OnInfectionStageChanged`, `OnDeath` | `UZSHealthComponent` | Health panel, death screen trigger |
| `OnNearestInteractableChanged` | `AZSPlayerCharacter` | Interaction prompt |
| `OnActiveHotbarIndexChanged`, `OnHotbarSlotsChanged` | `AZSPlayerCharacter` | Hotbar bar |
| `OnInventoryChanged` | `UZSInventoryComponent` | Inventory screen refresh (coarse - see below) |
| `OnReadyToSleepChanged` | `AZSPlayerCharacter` | Sleep prompt (partial - see below) |
| `OnBusyChanged` | `AZSPlayerCharacter` | Generic "you're mid-action" UI lock indicator, if wanted |

### Missing — needs small, targeted C++ additions before the relevant screen can be built
- **Hotbar switch progress.** `Server_SelectHotbarSlot`/`CompleteHotbarSwitch` know the switch duration and when it started, but nothing's exposed for a UI to show a fill/radial during the delay. Need either a `BlueprintPure GetHotbarSwitchProgress01()` (computed from the timer handle's remaining time) or a delegate firing `(float Duration)` when a switch starts, letting the widget drive its own timeline. Small.
- **Container contents, for real (not loot-all).** `AZSContainerActor::ContainerSlots` has an internal `OnRep_ContainerSlots` but no `BlueprintAssignable` delegate exposed, and no per-item transfer function — today's only interaction is the blind "loot everything" action. A real container screen needs `ContainerSlots` readable + a `Server_TransferItem(FGuid InstanceId, AZSPlayerCharacter* Looter)` (assumes the item-instance model from the companion doc; without it, transfer by array index instead, which is fragile if the array reorders mid-transfer — the GUID approach is recommended specifically to avoid that). Medium.
- **Hotbar-assign-from-inventory.** Today `SelectHotbarSlot` only picks among configs *already* in `HotbarSlots`. There's no mutator for "take this carried weapon and put it in hotbar slot N," which the Inventory screen needs for drag-or-click-to-assign. New function, small once the item-instance model exists.
- **Stat-preview text.** The "transparent stat-preview rule" (P2, carried through every phase since) has never had a generic C++ helper — every place that's shown numbers so far has been a raw `UE_LOG`/on-screen-debug-message, not real UI data. Need something like `UZSItemConfig::GetPreviewText() -> FText` (or a free function reading the item's `ItemUseType` and relevant fields) that a tooltip widget can bind to without duplicating "what does Bandage vs Consumable vs a weapon actually do" logic in Blueprint. Small but touches several existing classes (`UZSItemConfig`, `UZSWeaponConfig`) to gather the numbers.
- **Sleep-ready aggregate count.** `AZSGameState` tracks `bSleepRequestPending`/`PendingSleepHours` but not "how many of N connected players are currently ready" — needed for the "3/4 ready" UI text called out in §4.5. Small (`PlayerArray.Num()` + a count of `IsReadyToSleep()==true` across it, already computed transiently inside `UpdateSleepRequestState` — just needs to be exposed as a `BlueprintPure`, not recomputed).
- **Death cause.** `HandleDeath` knows nothing about *why* the character died by the time it runs — `UZSHealthComponent::Die()` has the information in scope (the killing blow's zone/wound type) but doesn't currently pass it anywhere persistent. Need to capture "last damage source description" somewhere the death screen can read it. Small-medium.

None of the above is large in isolation. They add up to "every screen needs a little bit of new C++ before its Blueprint/UMG half can be built," which is worth knowing going in rather than discovering mid-screen.

## 4. Screen-by-screen plan

### 4.1 HUD (always-on, non-modal, never eats input)

- **Moodle stack.** Needs/Health delegates above, direct bind. Per your own §21 note and the PZ reference's explicit warning that PZ's own moodle icons are notoriously cryptic — `RECOMMENDATION`: icon + short label + a severity color (the existing `SeverityTier2Max`/`3Max`/`4Max` thresholds on `UZSNeedsConfig` already give you the 4-tier banding to color against), not icon-only. Readable at a glance is the actual design goal here, not fidelity to PZ's layout.
- **Hotbar bar.** 9 slots, bottom-center is the genre-standard placement. Bind to `HotbarSlots`/`ActiveHotbarIndex`, show `UZSItemConfig::Icon` (already exists) per slot, highlight the active one, and show the switch-progress fill from §3 during a pending equip. Empty slots should read as obviously empty, not just blank.
- **Interaction prompt.** `NearestInteractable->InteractionVerb` + whatever key `IA_Interact` is bound to (read the binding dynamically rather than hardcoding "F" in the widget, so a future rebind doesn't silently desync the prompt text). Screen-space, bottom-center or near-crosshair — `RECOMMENDATION`: screen-space over world-space floating text, since TopDown is the default camera and a world-space label would constantly be foreshortened/hard to read at that angle; flag if you had world-space in mind.
- **Health/body-zone indicator.** The project's health model is already a simplified 4-zone system (Head/Torso/Arms/Legs) rather than PZ's more granular per-limb diagram — `RECOMMENDATION`: match that simplification in the UI too (4 icons, not a full body diagram), consistent with the "simplify" note and with work already done elsewhere (P3 deliberately chose 4 zones over PZ's finer granularity).
- **Crosshair/reticle.** Standard, not systems-driven — no design questions here, just note it needs to exist and should probably hide entirely in TopDown (where cursor-facing already does the aiming job) and show in the ThirdPerson/OverShoulder fallback.

### 4.2 Inventory screen (modal — first screen that needs §2's input-mode work)

Opens on a new `IA_Inventory` (doesn't exist yet, needs creating same as every other `IA_*` this project has added).

Per `ProjectZomboid_DesignReference.md` §12's own explicit warning — PZ's dual-pane drag-drop grid is *"the most-modded, most-complained-about surface in the game... treat as a must-beat benchmark, not a template"* — and your own §21 note wanting simplification, `RECOMMENDATION`: **a single scrollable list, not a dual-pane grid.** Each row: icon, name, count, weight, and the stat-preview text from §3 on hover. Equip slots (`Back`/`Hip`) shown as two fixed icon slots, separate from the scrolling list, always visible while the screen is open. A prominent weight bar (current/max, from `GetCurrentWeight()`/`GetMaxCarryWeight()`) — this is the single most important piece of transparent-stat-preview UI in the whole screen, since encumbrance is a soft movement penalty the player needs to see coming, not discover by feeling suddenly slow.

Actions per row: equip/unequip (gear items), drop, use (routes through the existing `Server_UseItem` for Consumable/Bandage/Disinfectant/Splint — no new dispatch logic needed, just a UI entry point into what already exists), assign-to-hotbar (weapons — needs the new mutator from §3).

Bulk actions (your §21 "modern, transparent UX" framing, echoed in `GameDevPlan.md` P6): loot-all already exists at the container level; a "drop all of this type" or similar could be a nice-to-have pass later, not blocking a first version.

### 4.3 Container loot screen (modal, reuses the Inventory screen's visual language)

This is the first real gap to close in P6's content, not just its UI — today `AZSContainerActor` only supports a blind "loot everything" action with **no UI at all**, which was an explicit, documented v1 bootstrap, not the intended final behavior. `Docs/Beta/B1_UI_UX.md`'s T6.2 already calls for letting a player "pick individual items out one at a time."

Layout: player's own carry list (left/top) next to the container's `ContainerSlots` (right/bottom), click-or-drag to transfer one item at a time, plus a "take all" button that still exists for the times you genuinely just want everything. Needs the `ContainerSlots` delegate + `Server_TransferItem` from §3.

### 4.4 Death / respawn screen (non-interactive countdown, arguably not "modal" since there's nothing to click)

Simple: cause-of-death text (needs the death-cause capture from §3), a countdown to `RespawnDelaySeconds` (already exists, just needs reading). There's no real spectator camera yet, just a timer — the UI can ship a countdown overlay without waiting on that; a spectate camera is a separate, bigger feature if it's ever wanted (flagged in §5, not planned here).

### 4.5 Sleep / time-skip prompt (small, non-modal, low risk)

"Press [key] to ready up — 3/4 ready." Bound to `IsReadyToSleep()` (existing) and the new ready-count aggregate from §3. Low complexity, good candidate to build early since it needs almost nothing new.

### 4.6 Radial quick-use (mentioned in both source docs, never detailed — lowest priority of everything here)

Both `DevMarkupNotes.md` §21 and `GameDevPlan.md`'s P6 section reference "radial quick-use" as a want, but neither says what it should contain. `RECOMMENDATION`: a hold-key radial (the PZ reference calls its own vehicle radial out as "one of the few radials" in that game, i.e. a rare-but-liked pattern, not something to reach for everywhere) scoped to fast medical/consumable access specifically — items whose `ItemUseType` is `Consumable`/`Bandage`/`Disinfectant`/`Splint` — as an alternative to opening the full Inventory screen mid-fight. This is squarely a "nice, but not urgent" item; recommend it lands after the Inventory and Container screens are solid, not before.

## 5. Explicitly not planned here (out of scope for this pass, flagging so they're not silently forgotten)

- A real spectator/death camera (vs. today's disabled-input timer) — bigger scope, already a known gap.
- Any crafting UI — no crafting system exists yet anywhere in the codebase; premature to plan its screen.
- A map/quest-log UI — belongs to P8 (events/investigation), not this pass.
- Settings/options menus, main menu, multiplayer lobby UI — none of these were asked for and none block gameplay testing; standard UE boilerplate whenever they're actually needed.

## 6. Recommended build order

1. **Input-mode switching infrastructure (§2)** — nothing else works without it.
2. **HUD (§4.1)** — highest existing-delegate coverage, effectively "free" given how much of the backing data already broadcasts changes.
3. **Sleep prompt (§4.5)** — small, good early win, needs one small new aggregate.
4. **Inventory screen (§4.2)** + its missing C++ (hotbar-assign, stat-preview text).
5. **Container loot screen (§4.3)** + its missing C++ (transfer function, exposed delegate) — this is also where P6's actual exit criteria ("loot under threat... item scarcity feels intentional") becomes reachable for the first time.
6. **Death screen (§4.4)** — small, low risk, can slot in anywhere once the death-cause capture exists.
7. **Radial quick-use (§4.6)** — last, lowest priority per §4.6's own reasoning.

## 7. Open questions

> **Updated 2026-07-26** — question 2 answered (partially) via `Docs/Planning/RescopeQuestionnaire.md`; still not a final layout, per the dev's own "I will design this later."

1. **`IA_Inventory`'s key binding** — needs to exist before the Inventory screen can open at all; your call same as every other input action so far.
2. ⚑ **Answered, differently than recommended — not final.** Dev's own words: "I like the idea of separate containers for inventory, and equipment slots that the player drags items into to assign." That leans toward **distinct equipment-slot drag targets** (hotbar/`Back`/`Hip`/future clothing as visible icon slots) **alongside** a general carry-container list — closer to a hybrid than this doc's single-flat-scrollable-list recommendation, though not necessarily PZ's full dual-pane grid either. Treat §4.2 below as a starting point to react to, not a locked layout — raise it as a real check-in when this screen actually gets designed.
3. **Screen-space vs. world-space interaction prompt (§4.1)** — recommended screen-space given TopDown's camera angle; flag if you pictured something else.
4. **Radial quick-use priority (§4.6)** — build in the first UI pass, or genuinely defer toward P9-style polish? Recommended the latter.
5. **Spectator camera for the death screen (§4.4)** — worth planning properly at some point, or is a countdown overlay the intended permanent behavior?
