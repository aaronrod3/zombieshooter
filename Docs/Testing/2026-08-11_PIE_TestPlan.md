# PIE Test Session — Downed/Revive/Drag-and-Drop Backlog + Full Inventory Pass

> Written 2026-08-10, at the dev's request for "a full plan for tomorrow to come to, to get a lot of testing done" — covers everything from the 2026-08-10 session (blackout removal → 0-HP downed/revive, amputation shock, the drag-and-drop backlog) plus the separately-requested full inventory pass (icons, multiple items, container-to-container transfer, shooting/reload/ammo). Nothing in Part 1 has touched a running game yet — this is the first PIE exposure for all of it. Run top to bottom the first time; each stage is independently re-runnable after that.
>
> Two parts, deliberately ordered: **Part 1** is the actual 2026-08-10 changeset (verify what shipped). **Part 2** is the inventory-specific pass the dev asked to plan separately — broader scope, some of it (container-to-container transfer) is genuinely new territory nothing has explicitly exercised yet.

## 0. Prerequisites — resolve before Stage A

1. **Build is current.** Last confirmed-fresh build: 2026-08-10, `Build.bat ZombieShooterEditor Win64 Development`, `Binaries\Win64\UnrealEditor-ZombieShooter.dll` timestamped 2026-08-10 evening. If you've changed any source since, rebuild first — and per the tooling gotcha in `SessionHandoff.md`, don't just trust a "successful build" message; check that DLL's timestamp is newer than your newest edit if anything seems off.
2. **PIE multiplayer settings** (Editor Preferences → Play, or the PIE dropdown): **Number of Players = 2**, **Net Mode = Play As Listen Server**. Stages A/C/G below specifically need two clients — everything else works solo.
3. **Content gaps to resolve or work around first** — several items this plan needs may not be authored/wired yet:
   - **A `HealthRestore`-flagged item.** `UZSItemConfig::HealthRestore` is a brand-new field this session (painkiller-style flat HP top-up) — confirm at least one item config has it set to something nonzero before Stage C (self-heal-while-downed). If nothing has it authored yet, that specific check can't run until it is.
   - **A `Bandage`/`Disinfectant`/`Splint`-typed item.** `DA_ZS_ItemConfig_Bandage` moved to `Content/ZS/Items/ItemDataAssets/` (confirmed via content search, not deleted — see `SessionHandoff.md`'s tooling-gotcha note) — there's also a separate, newer `DA_Bandage.uasset` in the same folder. Confirm which one is the real, current one before Stage H.
   - **A working ranged weapon + its ammo.** `DA_ZS_WeaponConfig_AssaultRifle` moved to `Content/ZS/Weapons/Rifle/`; the 4 ammo configs moved to `Content/ZS/Weapons/Ammo/DataAssets/`. `BP_ZS_PlayerCharacter::StartingHotbarLoadout` was deliberately cleared to empty back on 2026-08-09 (testing the full pickup loop, not spawning pre-geared) — confirm a pickupable weapon + its ammo item are actually placed somewhere in the test level, or temporarily re-seed `StartingHotbarLoadout` for this session's convenience (your call, not assumed).
   - **Two containers placed near each other** (or a container plus a wearable bag) for Stage L (container-to-container transfer) — `BP_ZS_Container_Test` exists; confirm at least two instances (or one + a Backpack/Duffle) are placed within walking distance of each other and of a Pockets-fillable amount of loose items.
   - **Zombies spawnable.** `ZS.SpawnZombies <n>` (host-only console command) works in any level provided `AZSGameMode::StressTestZombieClass` is assigned — confirm before Stage A, since downed/revive testing needs a real damage source.
4. **Known pre-existing gaps — don't mistake these for new bugs from this session:**
   - Automation suite sits at 29/37 (up from 24/37 pre-fix) — the 8 remaining failures are itemized in `SessionHandoff.md`'s 2026-08-10 entry. Two are worth a specific PIE cross-check while you're in there anyway (both flagged below, Stage F and Stage L): does picking up a Backpack still auto-equip it for real, and does a second identical stackable food item actually behave the way you'd expect.
   - No skeleton sockets exist for the 11 clothing/gear slots yet (`GetSocketForEquipSlot`'s names) — worn-mesh placement will look wrong/default-positioned, not a functional bug.
   - `WBP_ZS_BodyConditionIndicator`'s 5 icon textures and every optional UMG animation are still unassigned — cosmetic only.
5. **Useful debug console commands while testing:**
   - `ZS.SpawnZombies <n>` — scattered zombie batch for combat/downed testing.
   - `ZS.DebugListCarrySlots` / `ZS.DebugListWounds` — ground-truth state dumps; cross-check these against the Details panel per the known "replicated array can show empty in a live Details panel" gotcha, don't trust the panel alone.
   - `ZS.DebugUseItem` — force-use an item without needing to find it in the world first (zone-target argument was removed 2026-08-09, auto-targeting now).
   - `ZS.UI.ToggleBlackoutOverlay` — now broadcasts `HealthComponent->OnDownedChanged` (retargeted from the old blackout delegate), useful for a pure visual check of the downed overlay without actually going downed.

---

## Part 1 — 2026-08-10 backlog: downed/revive, amputation shock, drag-and-drop

### Stage A — Downed → revive (2 players, listen server)

| # | Action | Expected | If wrong |
|---|---|---|---|
| A1 | Player 1 takes damage down to 0 HP (zombie hits, or repeated `ZS.DebugApplyDamage`-style command if one exists — otherwise just fight a zombie down to 0) | Player 1 enters **Downed**, not dead. Movement disables, the revive-prompt interactable becomes active nearby. No blackout-style full lockout. | If Player 1 dies outright: `UZSHealthComponent::HandleHealthDepleted()` isn't routing to `Server_EnterDowned()` — check `bIsDowned`/`bIsDead` in `ZS.DebugListWounds` or a Details panel (cross-checked, per the gotcha above). |
| A2 | While downed, Player 1 tries to fire their weapon at a nearby zombie | **Fire works** — no longer blocked. Should be visibly less accurate than normal (wider spread cone) — compare hip-fire spread to a quick pre-downed shot if you can. | If fire does nothing: `CanFire()` still has a stale `IsDowned()` gate somewhere, or `HealthComponent` is null on this pawn. |
| A3 | While downed, Player 1 tries to reload or press a hotbar weapon-swap key | Both work, but **visibly slower** than normal — compare timing to a pre-downed swap/reload if you can eyeball it. | If instant or blocked entirely: `BeginBusyAction`'s downed scaling or `Server_SelectHotbarSlot_Implementation`'s `SwitchDelay` scaling isn't applying. |
| A4 | Player 2 walks up to downed Player 1 and interacts | Revive prompt fires, Player 1's HP jumps to `ReviveHealthAmount` (20 by default) and stands back up. | If nothing happens: `ReviveInteractable->bIsInteractable` may not have flipped true on downed-entry — check `HandleDownedChanged`. |
| A5 | Immediately after being revived | Player 1 moves at **reduced speed** for a short window (`PostReviveSlowDurationSeconds`, 10s default), then returns to normal on its own — don't need to do anything, just wait it out and confirm it clears. | If speed never recovers, or never drops in the first place: `bIsPostReviveSlowed`/`OnRep_IsPostReviveSlowed` isn't wired into `UpdateMovementSpeed` correctly. |

### Stage B — Solo downed (single player, no one to revive)

| # | Action | Expected | If wrong |
|---|---|---|---|
| B1 | Alone, take damage down to 0 HP | Still goes **Downed**, not instant death — this is the reversed behavior from earlier today ("solo can still be downed, just lower odds of surviving"). | If instant death: an old solo-death check somehow survived — shouldn't, `HasOtherPlayersInSession()` was removed entirely. |
| B2 | While solo-downed, use a `HealthRestore` item (if one's authored per Prerequisites) | HP crosses back above 0, downed ends on your own, no teammate needed. | If nothing happens: `Server_RestoreHealth`'s downed-exit check, or the item isn't actually flagged with a nonzero `HealthRestore`. |
| B3 | (Optional, slow) Let the 2-minute `DownedDurationSeconds` timer expire with nobody reviving and no self-heal | Real death after the timer — loot drops, death zombie spawns (if `DeathZombieClass` is assigned), same as any other death. | If nothing happens after 2 minutes: `HandleDownedTimerExpired`/`Die()` isn't firing. |

### Stage C — Finishing blow

| # | Action | Expected | If wrong |
|---|---|---|---|
| C1 | While downed (either player count), take a fresh hit from a zombie | **Instant death**, not a fresh countdown — a hit landing on an already-downed player is meant to finish them off. | If it just refreshes/extends the downed timer instead: `HandleHealthDepleted`'s already-downed branch isn't calling `Die()` outright. |

### Stage D — Amputation shock (decoupled from downed entirely)

| # | Action | Expected | If wrong |
|---|---|---|---|
| D1 | Get a limb amputated (via whatever the current amputation trigger is — `ZS.DebugAmputateZone` or the real bite-infection path) | **No blackout, no incapacitation at all** — you keep moving/fighting/using items the whole time, just noticeably slower for a while (`AmputationShockDurationSeconds`, 45s default at `AmputationShockMobilityMultiplier` 0.3x). | If movement locks up or a blackout-style overlay appears: some old blackout code path survived somewhere — flag directly, shouldn't be possible, it was removed entirely. |
| D2 | Wait out the shock duration | Movement speed picks back up to the *permanent* (smaller) amputated-zone penalty, not full speed — the permanent penalty was never removed, only the temporary shock layer clears. | — |
| D3 | Confirm you were never flagged downed at any point during D1/D2 | `IsDowned()` should stay false throughout — amputation and downed are fully independent now. | If downed triggers alongside amputation: something's still calling into the downed system from the amputation path — shouldn't exist anymore. |

### Stage E — Drag-and-drop: Pockets/compartment reorder

| # | Action | Expected | If wrong |
|---|---|---|---|
| E1 | Drag an item between two Pockets slots | Items swap cleanly, positions persist (close and reopen the inventory screen, order should still match). | — |
| E2 | Drag an item from Pockets into a worn bag's compartment, then back out | Moves correctly both directions, no duplication, no vanishing. | — |
| E3 | Drag an item onto an already-occupied slot | Swaps atomically — neither item disappears or gets orphaned. | — |

### Stage F — Drag-and-drop: weapon auto-mount/swap + container-bearing auto-equip

| # | Action | Expected | If wrong |
|---|---|---|---|
| F1 | Pick up a two-handed long gun with both long-gun mounts empty | Auto-mounts into the first free mount slot, immediately key-selectable. | — |
| F2 | Pick up a second long gun with both mounts full | Bumps the current/last-equipped one to a loose Pockets item, mounts the new one. | — |
| F3 | Drag a mounted weapon directly onto another occupied mount slot | Swaps cleanly — bumped weapon either drops to world or returns to its source container, depending on where the incoming one came from. | — |
| F4 | **Pick up a Backpack/Vest/Belt/Duffle with that slot empty** | Auto-equips immediately, compartment opens, icon shows. **Specifically flagged**: one of today's automation tests (`CompartmentCapacityAndStoreRegression`) showed this auto-equip *not* firing in a synthetic harness for reasons not yet root-caused — worth confirming this still works correctly for real in PIE, not just assuming the automation failure is a test-only artifact. | If it doesn't auto-equip: this is a real bug, not just a test-harness quirk — flag it, don't just note it. |

### Stage G — Drag-and-drop: container deposit (2 players useful but not required)

| # | Action | Expected | If wrong |
|---|---|---|---|
| G1 | Open a world container, drag an item from your inventory into it | Item leaves your inventory, appears in the container. | — |
| G2 | Bump a mounted weapon by mounting a new one that came from an open container | Bumped weapon deposits back into that same container instead of dropping to the world. | — |

### Stage H — Healing auto-targeting

| # | Action | Expected | If wrong |
|---|---|---|---|
| H1 | Get wounds on 2+ zones with different severities (e.g. a critical bleed on Head, a normal wound elsewhere), then use a Bandage-type item | Auto-targets the most severe zone (Head/critical-bleed) without you picking one. | — |
| H2 | Use a Disinfectant-type item on a dirty-but-not-infected zone vs. an infected one (if both exist) | Targets the infected one first. | — |
| H3 | Use a Splint-type item on a fractured zone | Targets the fracture, mitigates its mobility penalty. | — |
| H4 | Use a `HealthRestore`-flagged consumable at partial HP | Flat HP top-up, doesn't touch wound state at all (no bleed/dirty/fracture changes). | — |

### Stage I — Scratch wounds

| # | Action | Expected | If wrong |
|---|---|---|---|
| I1 | Take a Scratch-type wound | No bleeding starts — Scratch is the one wound type that never bleeds. | — |
| I2 | Heal back to full HP (any means) | The Scratch wound clears on its own, no bandage needed. | — |

---

## Part 2 — Full inventory pass (separately planned, broader scope)

### Stage J — Icons

| # | Action | Expected | If wrong |
|---|---|---|---|
| J1 | Check every slot type at rest (nothing equipped/carried): Pockets, Vest/Belt/Backpack/Duffle compartments, both long-gun mounts, sidearm mount, melee mount, Equipment slot, SecondaryHand slot, all 11 clothing/gear equip slots | Every one shows the "unoccupied" placeholder texture — none blank/transparent. | — |
| J2 | Equip/carry a real item in each slot type | Placeholder is replaced by that item's real icon. | If it stays on the placeholder: that item's `Icon` field isn't authored — a content gap, not a code bug. |
| J3 | Remove/unequip an item that had a real icon | Slot reverts cleanly to the placeholder, not a blank hole. | — |

### Stage K — Multiple items, stacking, capacity

| # | Action | Expected | If wrong |
|---|---|---|---|
| K1 | Pick up two units of the same stackable item | Merges into one stack (count shown on the icon), doesn't take two slots. | — |
| K2 | Pick up two units of a non-stackable item (e.g. two weapons of the same config) | Two distinct slots, each independently draggable/trackable (different `InstanceId`s — matters for durability/condition tracking). | — |
| K3 | Fill a compartment (Pockets/Vest/Belt/Backpack/Duffle) to its exact grid capacity, then try one more | Rejected outright — item stays wherever it was (world or source container), not silently destroyed. | — |
| K4 | Mix several distinct item types in one compartment | All render correctly side by side, no visual overlap or slot-index collision. | — |

### Stage L — Container-to-container transfer

This is genuinely new territory — today's container-deposit work covers player↔container specifically; bag↔container and container↔container haven't been explicitly exercised.

| # | Action | Expected | If wrong |
|---|---|---|---|
| L1 | Open a world container, drag an item directly into a currently-worn bag's compartment (not via your Pockets first) | Item moves container → worn bag in one drag. | If this doesn't work at all: may need a dedicated drop-handler, not yet built — flag as a real gap if so, don't assume it should already work. |
| L2 | Reverse: drag an item from a worn bag directly into an open world container | Same, other direction. | Same caveat as L1. |
| L3 | With two containers open at once (if the UI even allows that — may not), drag between them directly | Either works cleanly, or the UI doesn't support two containers open simultaneously (in which case this isn't a bug, just a UX scope question worth noting). | — |
| L4 | **Cross-check against the still-unexplained `BagStoreAndRetrieve` automation failure**: add two units of the same food/consumable config, confirm both actually register as separate carried instances (not silently merged into one stack when you expected two, or vice versa) | Behavior should match whatever `DA_ZS_ItemConfig_CannedFood`'s `MaxStackSize` is actually authored as. | If two adds of the same config always merge into one instance but a test/design assumption expected two distinct ones: that's a stacking-config mismatch, worth resolving either in the test or the content, not the code. |

### Stage M — Shooting, reload, ammo consumption

| # | Action | Expected | If wrong |
|---|---|---|---|
| M1 | Equip a ranged weapon with reserve ammo carried | Firing consumes ammo from the carried reserve item, not an infinite/unlimited pool. | — |
| M2 | Run reserve ammo to zero, try to reload | Reload correctly fails/no-ops when there's nothing to reload from. | — |
| M3 | Reload with ammo available | Pulls from the correct ammo item (matching `UZSWeaponConfig::AmmoItemConfig`), magazine count updates. | — |
| M4 | Force a jam (`BaseJamChance`/`MaxJamChance` over repeated fire, or however jams are currently forced for testing) | Weapon stops firing, "Rack Firearm" action clears it. | — |
| M5 | Fire/reload while downed (ties back to Stage A) | Both work, both visibly slower/less accurate per the downed penalties — good moment to confirm ammo consumption itself isn't affected by downed state, only timing/accuracy. | — |

---

## After this session

Once both parts are run: re-run the `ZS.` automation suite one more time to confirm nothing regressed from any PIE-driven content fixes, and revisit the 8 still-open automation failures listed in `SessionHandoff.md`'s 2026-08-10 entry with fresh PIE context in hand (especially the two flagged for direct cross-check above, Stage F/F4 and Stage L/L4).
