# B1 Full Summary — UI/Inventory/Downed-Revive Work + Test Plan

> Written 2026-08-10 as a pure test plan, restructured 2026-08-11 into a full B1 handoff document
> after the dev paused active testing ("let's just save all this testing for later, there is still
> more work to do"). Four parts: **(1)** what's actually been built across the whole B1 phase, not
> just this session, **(2)** what's still unverified, **(3)** prerequisites, **(4)** exact test
> steps — console commands and key names, not "fight a zombie until X happens." Sections 3-4 are
> unchanged from the 2026-08-10/11 test-plan version; sections 1-2 are new, synthesized from
> `SessionHandoff.md`'s full B1 history and `Docs/Beta/B1_UI_UX.md`'s exit criteria.

---

## 1. What's been built — full B1 summary

B1 ("UI/UX Foundation, HUD & Input Modes") started 2026-07-30. Everything below shipped since then;
organized by subsystem, not chronologically — full blow-by-blow history lives in
`SessionHandoff.md`'s dated entries and git log if a specific decision's reasoning is needed.

### UI framework & screens
- `UZSUIManager` (`ULocalPlayerSubsystem`) owns a modal input-mode stack — pushing a modal adds
  `IMC_ZS_UI` at higher Enhanced Input priority than gameplay's `IMC_ZS_Default`; popping the last
  one removes it. Left-click means UI-select over a menu, attack otherwise.
- All 22 `WBP_ZS_*` Widget Blueprints built (via `unreal-mcp` tool calls against the live editor),
  each backed by a dedicated `UZSUserWidgetBase` C++ subclass — `BindWidget`/`NativeConstruct()`
  does all the wiring natively, Blueprint side is Designer-tab layout + Class Defaults only.
- Tab (Inventory), Escape (Pause, closes whichever of Inventory/Sleep/Pause is topmost), and Sleep
  all have real production input wiring. `AZSHUD` creates Death/Blackout-turned-Downed screens at
  `BeginPlay` (survives respawn); `UZSGameInstance::Init()` creates MainMenu/LoadingScreen (survives
  level travel). 5 `ZS.UI.Toggle*` debug commands cover manual visual checks of each screen.
- Interacting with a world container opens the real `WBP_ZS_ContainerLoot` screen (dev-resolved
  2026-08-05: a real loot screen, not the original auto-loot-all placeholder — that behavior still
  exists as `Server_TakeAllItems`, now just the loot screen's "Take All" button).
- **Three separate widget-invisibility bugs found and fixed this phase**, each a different root
  cause worth knowing about if a new widget goes blank again: (1) a root `CanvasPanel`'s
  `ComputeDesiredSize()` only measures children with `bAutoSize:true` — 5 Loadout-tab widgets had
  it false, reported `(0,0)`, rendered at literal 0×0. (2) 4 slot-widget classes called
  `SetVisibility(Collapsed)` on their only visual element whenever empty — reversed per dev intent
  ("an empty slot is still real, visible UI"), now always visible, only the icon brush clears. (3)
  That same clear-to-`nullptr` pattern then wiped a dev-authored default placeholder texture on
  `NativeConstruct` — fixed by caching a `DefaultIconBrush` per widget and restoring it instead of
  clearing to blank.
- 3D Loadout-tab character preview (`USceneCaptureComponent2D`) — fixed a sideways/near-black
  capture by attaching to the capsule instead of the mesh (retargeted-skeleton forward mismatch) and
  forcing manual exposure.

### Inventory / equip / loadout rework
- `EZSEquipSlot` expanded from 3 values to 12 — Clothing (Head/Eyes/Mask/Shirt/Pants/Shoes) + Gear
  (Helmet/Vest/Belt/Backpack/Duffle). Helmet force-unequips Head (one-way).
- `FZSItemInstance::SlotIndex` — every carried item remembers its own grid position; a slot renders
  whichever instance's `SlotIndex` matches it, not "the Nth item in carry order."
- `UZSCompartmentPanelWidget` reworked onto a fixed `UniformGridPanel` (slots built once, reused,
  not rebuilt every refresh) with a generalized `GatingSlot` — Pockets gates on `Pants`, each bag
  compartment gates on its own equip slot. An unworn bag's compartment collapses; a worn-but-empty
  one still shows all its slots.
- Real capacity enforcement (`GetCompartmentCapacity`: Pockets 4, Vest 8, Belt 8, Backpack 20,
  Duffle 18) — a pickup or store that would overflow is atomically rejected, item stays put rather
  than being silently destroyed.
- Worn-mesh visual attachment (11 `WornMeshComponents`) — wired and functional, but skeleton sockets
  for all 11 slots don't exist yet, so placement will look wrong until content catches up.
- Weapon auto-mount-on-pickup and container-bearing-gear (Vest/Belt/Backpack/Duffle) auto-equip-on-
  pickup — both dev-confirmed "no separate manual-equip step" design intent.

### Drag-and-drop rule set (2026-08-10 backlog)
- Compartment/Pockets slot-position persistence, drag-out-of-equip/mount-slot support, weapon-mount
  swap-drop rules (bumped weapon returns to world or its source container depending on where the
  incoming one came from), a new container-deposit system (player → world container), dedicated
  melee weapon mount (`MountedMelee`, 4th weapon-key slot).

### Downed/revive redesign (2026-08-10, most recent major system change)
- Blackout removed entirely, replaced by `UZSHealthComponent::bIsDowned`. 0 HP always enters a
  2-minute downed window (solo included — no more instant death) unless already downed, in which
  case a fresh hit is a finishing blow. Downed players can still fight, just penalized (wider
  accuracy spread, slower reload/swap/busy-actions). Self-heal (`HealthRestore` item) or teammate
  revive both exit downed and start a temporary post-revive movement-speed penalty. Amputation is
  now fully decoupled from any incapacitated state — just a temporary extra mobility penalty
  stacked on the pre-existing permanent zone penalty. Full tunables: `Docs/TuningReference.md`.
- Healing auto-targeting (severity-tiered) + a dedicated `HealthRestore` field for flat-HP items.
  Scratch wounds added (one-time damage, never bleeds, clears at full health).

### Tonight's fixes (2026-08-11, first real PIE pass on any of the above)
- Added `ZS.DebugRestoreHealth` — simulates a HealthRestore item so self-revive is testable with
  zero content authoring.
- **Two real content bugs found and fixed live**: `DA_ZS_WeaponConfig_AssaultRifle` had
  `MaxStackSize` set above 1, which routed pickups through `Server_AddItemInstance`'s stackable
  branch — that branch mints a fresh GUID for the new `CarrySlots` entry, orphaning the GUID
  `Server_TryAutoMountWeapon` was called with, so the rifle picked up successfully but silently
  never mounted. Fixed in content. A second, separate `Handedness`/`AttackType` misconfiguration on
  the same DA caused the same silent-no-mount symptom after the first fix; also resolved in content.
- Added logging to `Server_TryAutoMountWeapon` (previously silent on every path, success or
  failure) — now logs which mount slot succeeded, or exactly why nothing matched.
- **Confirmed working end-to-end in PIE**: Stage A1/A2 (enter Downed via `ZS.DebugKillSelf`, fire
  while downed with visibly wider spread) and Stage F1 (rifle world-pickup → auto-mount → equip-in-
  hand via hotbar key). Marked in Section 4 below.

### Automation suite
Currently **29/37 passing** (climbed from a 24/37 baseline this session — an earlier "12 failures"
report was a stale-build artifact, not real). Fixed along the way: 2 real regressions (tests
asserting instant death, now obsolete since 0 HP goes downed first), 5 stale `LoadObject` paths from
a content-folder reorganization, 2 tests with a stale assumption predating the auto-equip-on-pickup
feature. 8 failures remain — see Section 2 below for the current list and why each is still open.

### Code-review pass (dev-requested, "go through most of the ui/inventory/health systems... ensure
they are efficient and done well")
- **Fixed**: `Server_EquipToSlot`'s Helmet-force-unequips-Head branch bypassed
  `RefreshOnPersonSlotIndex`, reproducing the exact stale-SlotIndex collision bug that helper exists
  to prevent.
- **Fixed**: extracted a shared `CanFitInPockets` helper (one pass, no allocation) replacing
  duplicated overflow-simulation logic in `Server_AddItem`/`Server_AddItemInstance`.
- **Fixed**: `UZSCompartmentPanelWidget::RefreshCompartment` now builds a `TMap<int32, FZSItemInstance>`
  once per refresh instead of an O(slots×items) scan per grid cell.
- **Left open, by design** (see Section 2): `UZSContainerLootWidget`'s rebuild-every-mutation
  pattern, and a defensive-validation gap in `Server_MoveToSlot`.

---

## 2. What's not verified

### PIE-unverified
Every system above except the three items tonight confirmed (Stage A1/A2, Stage F1 — see Section 4)
has had **zero PIE testing**. That includes the entire inventory/equip rework, the full
drag-and-drop rule set, healing auto-targeting, scratch wounds, most of downed/revive (self-heal,
teammate revive, finishing blow, amputation shock, post-revive slow), and every B1 screen's actual
in-game behavior beyond a Designer-tab preview.

### Automation failures still open (8)
- 3 unrelated pre-existing zombie-AI failures (`ZombieBiteZoneWeightedRoll`,
  `ZombieDeathWhileDownedClearsDownedFlag`, and one of the two latent-timer failures below).
- 1 unrelated pre-existing wound-display logic failure (`WoundDisplayConditionIsZoneAwareAndPrioritized`).
- 2 share a "state never flips after the deadline wait" symptom on a real-time timer wait in the
  headless test harness (`DownedZombieAutoRecovery`, `AmputationChoreographyAppliesShock`) — suspected
  harness limitation (`-nullrhi`), not confirmed as two separate logic bugs.
- `EquipHelmetForceUnequipsHead` — `Server_EquipToSlot(Head, ...)` returns false for a plain, valid
  instance in the test harness; code reads correct by inspection, genuinely unexplained.
- `BagStoreAndRetrieve` — fails at "second food item found," likely because the food config is
  genuinely stackable and the test's two-separate-instances premise no longer holds; not confirmed.
- `CompartmentCapacityAndStoreRegression` — the Backpack auto-equip-on-add case doesn't fire in this
  synthetic harness, for reasons not root-caused. **Note**: tonight's PIE session confirmed weapon
  auto-*mount* works correctly (Stage F1), but that's a different code path from container-bearing
  gear auto-*equip* (Vest/Belt/Backpack/Duffle) — this specific failure is still an open flag,
  worth a direct PIE check (Bucket 1 Stage F, row 3, in Section 4) before assuming it's harness-only.

### Exit-criteria gaps (`Docs/Beta/B1_UI_UX.md`)
- 2-client verification (PT1/PT6's 2-client half) — deliberately deferred as its own session, still
  owed, not attempted at all yet.
- "Full loot loop playable through UI: open → inspect → take individual → manage weight → close" —
  not confirmed end-to-end; individual pieces exist, never run as one sweep.
- "No screen hardcodes a mouse-only interaction, every drag/click also has a keyboard path" — not
  audited against the current drag-and-drop system.
- "A player can read every simulated stat without opening the console" — moodle/stat display exists
  per earlier B1 work, not re-confirmed against the current systems.

### Open code-review findings (not bugs, deferred by design)
- `UZSContainerLootWidget::RefreshContainerGrid` destroys/rebuilds every item-slot widget on every
  mutation, unlike the fixed-grid pattern `UZSCompartmentPanelWidget` uses — fixing it means deciding
  whether container contents get position persistence too, a real design call.
- `Server_MoveToSlot`'s `FindItemAnywhere` doesn't check whether the resolved item is currently
  equipped/mounted — unreachable through the normal UI flow, but a raw RPC call bypassing the UI
  wouldn't be caught server-side. Low priority given this project's threat model.

### Content gaps
- **No clothing items exist at all** (this session's finding) — blocks the Pockets panel entirely
  until the Bucket 2 Pants placeholder is built (see Section 4), and blocks the 5 remaining Clothing
  slots + Helmet regardless.
- No skeleton sockets for any of the 11 clothing/gear slots.
- `DA_ZS_UIStyle_Default` still not assigned to any widget's `style` property.
- `WBP_ZS_BodyConditionIndicator`'s 5 icon textures and all 4 optional UMG animations unbuilt.
- Two similarly-named Bandage assets (`DA_ZS_ItemConfig_Bandage` / `DA_Bandage`) — which is current
  hasn't been resolved.
- Item icon art largely unauthored across the board — bounds how much of Bucket 4's icon pass can
  run regardless of code correctness.

---

## 3. Prerequisites — resolve before testing

1. **Build is current.** Last confirmed-fresh build: 2026-08-10. This session adds one new debug
   command (`ZS.DebugRestoreHealth`, see Bucket 1 below) — **rebuild before testing**, and per the
   standing tooling gotcha, verify `Binaries\Win64\UnrealEditor-ZombieShooter.dll`'s timestamp is
   newer than your newest edit before trusting a "successful build" message.
2. **PIE multiplayer settings** (Editor Preferences → Play): **Number of Players = 2**, **Net Mode
   = Play As Listen Server**. Only Stage A needs two clients — everything else works solo. When a
   step says "open console," that's the `~` key in whichever PIE viewport has focus.
3. **Zombies spawnable.** `ZS.SpawnZombies <n>` (host-only) works in any level provided
   `AZSGameMode::StressTestZombieClass` is assigned — confirm before Bucket 1.
4. **Known pre-existing gaps — don't mistake these for new bugs:**
   - Automation suite sits at 29/37 — 8 remaining failures itemized in `SessionHandoff.md`'s
     2026-08-10 entry.
   - No skeleton sockets exist for the 11 clothing/gear slots yet — worn-mesh placement will look
     wrong/default-positioned regardless of what you build. Not a functional bug, don't chase it.
   - `WBP_ZS_BodyConditionIndicator`'s icon textures and optional UMG animations are unassigned —
     cosmetic only.

---

## 4. Exact testing steps

## Bucket 1 — Ready right now (no new content, exact steps)

Everything below runs against content you already have (existing weapon/ammo/bag configs) plus
console commands that stand in for anything not yet authored. Work through these first.

### A — Downed → revive (2 players)

1. **[✔ confirmed 2026-08-11]** In Player 1's PIE viewport, open console, run `ZS.DebugKillSelf`.
2. **Expect:** Player 1 enters Downed, not dead (`HandleHealthDepleted()` always downs on the
   *first* 0-HP crossing — see `ZSHealthComponent.cpp:901`). Movement disables, no blackout-style
   overlay.
3. **[✔ confirmed 2026-08-11]** While downed, aim (RMB) and fire (LMB) at a nearby zombie
   (`ZS.SpawnZombies 3` if none are up). **Expect:** fire works, visibly wider spread than normal.
4. While downed, press `R` (reload) or a hotbar number key to swap weapons. **Expect:** both work,
   visibly slower than normal.
5. In Player 2's viewport, walk up to Player 1 and press `F` (Interact). **Expect:** revive prompt
   fires, Player 1's HP jumps to `ReviveHealthAmount` (20 default) and stands up.
6. Immediately after standing, try to move. **Expect:** noticeably slower for ~10s
   (`PostReviveSlowDurationSeconds`), then back to normal on its own — don't do anything, just wait
   and confirm it clears.
- **If wrong:** step 2 instant-dies → check `bIsDowned`/`bIsDead` via `ZS.DebugListWounds`. Step 3
  blocked entirely → stale `IsDowned()` gate on `CanFire()`. Step 5 no-ops → check
  `ReviveInteractable->bIsInteractable` flips in `HandleDownedChanged`.

### B — Solo downed + self-heal

1. Alone (no Player 2), open console, run `ZS.DebugKillSelf`.
2. **Expect:** still enters Downed, not instant death (2026-08-10 reversal: solo can be downed too).
3. While downed, open console, run `ZS.DebugRestoreHealth 50`.
4. **Expect:** HP crosses back above 0, Downed ends on its own — this is the new command standing
   in for a real `HealthRestore` item until one's authored; it calls the exact same
   `Server_RestoreHealth` path a real item would.
5. *(Optional, slow)* Repeat step 1, then don't revive or heal — wait the full 2 minutes
   (`DownedDurationSeconds`). **Expect:** real death after the timer, loot drops, death zombie
   spawns if `DeathZombieClass` is assigned.
- **If wrong:** step 2 instant-dies → an old solo-death check survived, shouldn't be possible. Step
  4 does nothing → check `Server_RestoreHealth`'s downed-exit branch.

### C — Finishing blow

1. Run `ZS.DebugKillSelf` once (enters Downed, per Bucket A/B above).
2. While still downed, run `ZS.DebugKillSelf` a second time.
3. **Expect:** instant real death this time, not a refreshed/extended downed timer —
   `HandleHealthDepleted()`'s `if (bIsDowned) { Die(); return; }` branch.
- **If wrong:** second call just re-triggers Downed → that branch isn't firing.

### D — Amputation shock

1. Open console, run `ZS.DebugAmputateZone 2` (Arms) or `3` (Legs).
2. **Expect:** no blackout, no incapacitation — you keep moving/fighting/using items, just
   noticeably slower for 45s (`AmputationShockDurationSeconds` at `AmputationShockMobilityMultiplier`
   0.3x).
3. Wait out the 45s. **Expect:** speed picks back up to the *permanent* (smaller) amputated-zone
   penalty, not full speed — the shock layer clears, the permanent one doesn't.
4. Throughout steps 1–3, periodically check `ZS.DebugListWounds`. **Expect:** `bIsDowned` stays
   false the entire time — amputation and downed are fully independent.
- **If wrong:** any movement lockup or overlay → old blackout code path survived, flag directly.

### F — Weapon auto-mount + Gear-family auto-equip

Needs one placed world pickup per weapon tested — if `BP_ZS_WorldItem_Test` instances aren't
already in your test level, drag one in from the Content Browser and assign a weapon config
(2 minutes, no new DataAsset needed, existing configs work).

1. **[✔ confirmed 2026-08-11, after fixing 2 content bugs on `DA_ZS_WeaponConfig_AssaultRifle` —
   wrong `MaxStackSize` and a `Handedness`/`AttackType` misconfiguration, both caused a silent
   no-mount; `Server_TryAutoMountWeapon` now logs every outcome so a repeat wouldn't be silent]**
   With both long-gun mounts empty, walk to a placed two-handed weapon and press `F`.
2. **Expect:** auto-mounts into the first free mount slot, immediately key-selectable (`1`/`2`).
   Press the key to confirm it's actually held in-hand, not just mounted.
3. Repeat step 1 with both mounts full (place a second weapon pickup first).
4. **Expect:** bumps the current/last-equipped weapon to a loose Pockets item, mounts the new one.
5. Walk to a placed Vest/Belt/Backpack/Duffle-slot item (you already have a Bag) and press `F`.
6. **Expect:** auto-equips immediately, compartment opens. **This is specifically flagged** — an
   automation test (`CompartmentCapacityAndStoreRegression`) showed this *not* firing in a
   synthetic harness for reasons not yet root-caused. Confirming this works for real in PIE (not
   just a test-harness artifact) matters — if it doesn't auto-equip, treat it as a real bug.
- **If wrong:** no auto-mount → check `Server_TryAutoMountWeapon` is actually being called from
  `AZSWorldItemActor::HandleInteracted`.

### G — Container deposit (bag-to-container, sidesteps Pockets entirely)

1. Open a world container (`F` on `BP_ZS_Container_Test`).
2. Drag an item from your **worn bag's compartment** (not Pockets — no Pants needed for this) into
   the open container.
3. **Expect:** item leaves the bag, appears in the container.
4. With a mounted weapon, walk to a second weapon pickup that's sitting inside an *open* container
   and mount it (drag or auto-mount).
5. **Expect:** the bumped weapon deposits back into that same container, not dropped to world.

### I — Scratch wounds

1. Take a Scratch-type wound (weakest zombie hit, or however Scratch is currently forced for
   testing — check `ZSDamageTypes.h`'s marker classes if unsure which hit type produces one).
2. **Expect:** no bleeding starts.
3. Heal back to full HP by any means (`ZS.DebugRestoreHealth 999` works fine here too).
4. **Expect:** the Scratch wound clears on its own, no bandage needed.

### K — Stacking, capacity, slot collisions (via your worn Bag, not Pockets)

The capacity/stacking/slot-index logic is identical for every compartment type — Pockets isn't
special-cased, it's just one more `EZSCarryLocation`. Testing against your already-equipped Bag
exercises the same code Pockets will use once Pants exists.

1. Run `ZS.DebugGiveItem <path to a stackable food config>` twice in a row (e.g. Canned Food).
2. **Expect:** merges into one stack (visible count on the icon) once you store both into your bag
   — check via `ZS.DebugListCarrySlots`, which prints `ContainedItems` nested under the bag.
3. Run `ZS.DebugGiveItem <path to a non-stackable config> 1` twice with two different weapon
   configs (or the same one twice).
4. **Expect:** two distinct entries in `ZS.DebugListCarrySlots`, different `InstanceId`s each.
5. Fill your bag's compartment to its exact grid capacity (Backpack = 20, Duffle = 18 — see
   `GetCompartmentCapacity`), then try one more `Server_StoreInBag`.
6. **Expect:** rejected outright, item stays wherever it was — not silently destroyed.
- **Note:** this validates the shared plumbing, but *doesn't* exercise `CanFitInPockets` or
  `FindFirstFreeOnPersonSlotIndex` specifically, which are Pockets-only functions — re-run K1/K3
  against Pockets once Bucket 2 unlocks it, as a quick confirmation rather than a full re-test.

### L — Container-to-container / bag-to-container transfer

1. Place a second `BP_ZS_Container_Test` instance near the first (1 minute, no new content).
2. Open one container, drag an item directly into your worn bag's compartment.
3. **Expect:** item moves container → bag in one drag.
4. Reverse: drag an item from your bag directly into an open container.
5. **Expect:** same, other direction.
6. With two containers open at once (if the UI allows it), drag between them directly. **Expect:**
   either works cleanly, or the UI doesn't support two open containers simultaneously — the latter
   is a UX scope question, not a bug.
- **If wrong (L2/L3):** may need a dedicated drop-handler not yet built — flag as a real gap, don't
  assume it should already work.

### M — Shooting, reload, ammo consumption

1. Walk to a placed ranged-weapon pickup, press `F` (auto-mounts, per Bucket F above).
2. Open console, run `ZS.DebugGiveItem <path to that weapon's AmmoItemConfig>` with a healthy
   count (e.g. `30`).
3. Aim (RMB), fire (LMB) at a zombie repeatedly.
4. **Expect:** ammo drains from the carried reserve — confirm via `ZS.DebugListCarrySlots` before
   and after.
5. Fire until reserve hits 0, press `R` (reload). **Expect:** reload correctly fails/no-ops, nothing
   to pull from.
6. `ZS.DebugGiveItem` more ammo, press `R` again. **Expect:** pulls from the correct ammo config,
   magazine count updates.
7. Run `ZS.DebugForceJam`, then fire. **Expect:** weapon stops firing. Press `Alt+R` (Rack Firearm).
   **Expect:** clears the jam.
8. Repeat steps 3–4 while downed (`ZS.DebugKillSelf` first). **Expect:** both still work, just
   slower/less accurate — ammo consumption itself shouldn't change with downed state.

---

## Bucket 2 — Build one item first: Pants (~5 minutes)

Unlocks the Pockets compartment panel entirely — it's gated on `Pants` being equipped
(`ZSCompartmentPanelWidget.cpp:53`), independent of Pockets' actual capacity/contents. This is the
single highest-leverage item to author before your next pass — no mesh, no icon, no clothing art
needed, since skeleton sockets for clothing aren't wired yet anyway (worn-mesh visuals are already
expected to look wrong regardless).

**Exact steps to build it:**

1. Content Browser → navigate to `Content/ZS/Items/ItemDataAssets/` (or wherever your other item
   configs live).
2. Right-click → Miscellaneous → Data Asset → class `ZSItemConfig`. Name it
   `DA_ZS_ItemConfig_Pants_Placeholder`.
3. Open it, set: `DisplayName` = "Pants" (anything non-empty — avoids the blank-name log issue
   seen with the Bag), `EquipSlot` = `Pants`, `bIsEquippable` = true. Leave `WornMesh`/`Icon` unset.
4. Save.

**Then run, in order:**

1. Open console, run `ZS.DebugGiveItem /Game/ZS/Items/ItemDataAssets/DA_ZS_ItemConfig_Pants_Placeholder.DA_ZS_ItemConfig_Pants_Placeholder`
   (adjust path to match wherever you saved it).
2. Run `ZS.DebugEquipFirstBagItem` (this equips any carried clothing/bag-type item — Pants counts).
3. **Expect:** Pockets panel becomes visible in the inventory screen (`Tab`).
4. Now re-run **Stage E** (below) and re-confirm **K1/K3** from Bucket 1 against Pockets directly
   instead of the Bag.

### E — Pockets drag-and-drop (needs Bucket 2 done first)

1. Open inventory (`Tab`). Pick up two loose items (`ZS.DebugGiveItem` works fine, or real pickups).
2. Drag one Pockets item onto another empty Pockets slot. **Expect:** moves cleanly.
3. Drag two occupied Pockets items onto each other. **Expect:** swaps atomically, neither vanishes.
4. Close and reopen the inventory screen. **Expect:** positions persisted exactly as left.
5. Drag an item from Pockets into your worn bag's compartment, then drag it back out.
6. **Expect:** moves correctly both directions, no duplication, no vanishing.

---

## Bucket 3 — Needs one content decision, not new art

### H — Healing auto-targeting

**Before running:** resolve which is the real Bandage config — `DA_ZS_ItemConfig_Bandage`
(`Content/ZS/Items/ItemDataAssets/`) or the separate, newer `DA_Bandage.uasset` in the same folder.
Open both, check which has real values set (not just defaults) — that's the live one. Consider
deleting or renaming the other so this ambiguity doesn't resurface.

1. Take wounds on 2+ zones with different severities — e.g. fight a zombie until you have both a
   Head wound and a Torso wound (`ZS.DebugListWounds` to confirm state).
2. Run `ZS.DebugUseItem <resolved Bandage path>`.
3. **Expect:** auto-targets the most severe zone without you picking one.
4. If a Disinfectant-type config exists, use it on a dirty-but-not-infected zone vs. an infected
   one (if both exist). **Expect:** targets the infected one first.
5. If a Splint-type config exists, use it on a fractured zone. **Expect:** targets the fracture,
   mitigates its mobility penalty.
6. Use `ZS.DebugRestoreHealth 30` at partial HP. **Expect:** flat HP top-up only, no wound-state
   changes (no bleed/dirty/fracture changes) — confirms the real item will behave the same once
   authored, since this command routes through the identical `Server_RestoreHealth` call.

---

## Bucket 4 — Wait for real asset buildout

Don't block on these — they need actual content variety or art, not a quick placeholder.

- **J — Icons, full pass.** Checking every slot shows a real icon (not the placeholder) is bounded
  by how many items have `Icon` textures authored. Test incrementally as icons land, not all at
  once.
- **The 5 remaining Clothing-family slots** (Head/Eyes/Mask/Shirt/Shoes) **and Helmet** — need the
  real clothing set. Pants (Bucket 2) was the one slot worth faking early because of the Pockets
  gate; the rest have no equivalent hidden dependency, so there's no rush to placeholder them.
- **K4 — several distinct item types in one compartment at once.** Needs actual item variety
  (5+ different real configs), not a repeat of the same placeholder.

---

## After this session

Once Buckets 1–3 are run: re-run the `ZS.` automation suite to confirm nothing regressed, and
cross-check the two flagged automation failures (Stage F/step 6, Stage L/K's stacking check)
against what you just saw in PIE with fresh context. Revisit `SessionHandoff.md`'s 2026-08-10 entry
for the other 6 still-open automation failures.
