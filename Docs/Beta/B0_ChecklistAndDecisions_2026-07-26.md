# B0 Manual Steps — Test Walkthrough & Decisions

> **What this is.** Everything in this doc needs your hands — either manual PIE testing (no automation path exists, see `CLAUDE.md`) or a real editor content-authoring step. Full technical detail per sub-task lives in `Docs/Beta/B0_Stabilization.md`; compile/PIE-verification *status* lives in `Docs/SessionHandoff.md`. This doc is the walkthrough: what to click, what you should see, what counts as a pass.
>
> **Status as of 2026-07-29**: sections 1, 2 (core), 3 (solo), 4, 5 (pipeline itself), and 8 (needs) confirmed working in PIE. Sections 6-7 deferred by you to a dedicated future pass. Two real gameplay findings still open: zombie bites always land on Torso regardless of angle (root cause found — the hit-zone trace samples a fixed height, not angle-dependent; needs a design call on a fix), and zombies stop attacking after one hit (traced C++, looks clean — likely a `BT_Zombie` graph issue needing editor eyes). Section 9's scroll-wheel zoom bug also still needs an editor-side content fix. A few debug-command bugs of my own were found and fixed too (zone-name parsing, sleep-ready feedback) — **everything below marked "needs a rebuild" does.**

---

## Start here — all 3 confirmed working, 2026-07-29

Your own testing hit all three directly. All confirmed working now — nothing left to do here except retest #1 after the rebuild.

### 1. Rebuild — a real bug just got fixed
You reported the magazine staying behind when unequipping a rifle. Root cause: the magazine is a separate actor just glued onto the weapon visually, and destroying the weapon didn't take the magazine with it. Fixed in `AZSWeapon::Destroyed()`. **Needs a rebuild before you'll see it fixed.** Retest: equip a rifle, unequip it (switch hotbar slots or go bare-fisted) — the magazine should vanish along with the rifle, not float in place.

### 2. ✅ Confirmed — `StressTestZombieClass` assigned, `ZS.SpawnZombies` works
Both pieces already existed, nothing needed creating: `BP_ZS_GameMode`'s `StressTestZombieClass` now points at `Content/ZS/Enemy/Character/AZombieCharacter.uasset` (the existing, already-working zombie Blueprint, just not named `BP_Zombie_*` like the usual convention). `ZS.SpawnZombies 25` now spawns real zombies. Worth doing the same for `AZSPlayerCharacter::DeathZombieClass` on `BP_ZS_PlayerCharacter` if you haven't already — same asset, same fix, needed for section 10's zombie-conversion test.

### 3. ✅ Confirmed — bag grant/equip/store all working
`ZS.DebugEquipFirstBagItem`/`ZS.DebugStoreFirstItemInBag` originally warned `no bag/clothing-type item carried` because there was no placed world pickup or guaranteed loot-table roll for `DA_Bag` — not a `bIsEquippable` problem. Fixed with `ZS.DebugGiveItem`, and the full chain is now confirmed in PIE:
1. `ZS.DebugGiveItem /Game/ZS/Items/DA_Bag.DA_Bag` → granted.
2. `ZS.DebugEquipFirstBagItem` → equipped to Back (slot 1) — confirms `DA_Bag.bIsEquippable` is already `true`, this content gap is resolved.
3. `ZS.DebugStoreFirstItemInBag` → stored a carried rifle inside the bag successfully.

**Deferred by you for later** (not urgent, not a bug): drop the bag (with the rifle nested inside) and pick it back up, confirming the nesting survives — section 2 below, step 5.

`ZS.DebugGiveItem <object path> [count]` works for any `UZSItemConfig`-derived asset, not just the bag — useful any time you've authored a new item with no placed pickup yet.

---

## Reading state while there's no UI yet

**Watching live values.** While PIE is running, click your pawn (or any actor — a zombie, `AZSGameState`) in the World Outliner. The Details panel updates live. Use the component dropdown at the top of Details to jump to `Zs Needs Component` / `Zs Health Component` / `Zs Inventory Component` directly. Nested arrays expand too — a bag's `ContainedItems` is browsable right there.

You can **watch** these values but not **edit** them by typing into Details — replicated gameplay state is deliberately `VisibleAnywhere`, not `EditAnywhere` (so nobody can hand-edit server state through the panel). The only way to change state is the real mechanism (get shot, get wet, get bitten) or a console command / temporary debug key calling the matching `Server_` function.

**2-player PIE.** Only the host's window has a real Outliner. To check a second client's pawn, select *their* pawn from the **host's** Outliner — the host sees every replicated actor in the session.

**Console commands that exist today** (open console with `` ` ``, all host-only, all act on the local/host player only). All the ones below marked **New** need a rebuild before they'll work.

*Inventory/loot:*
| Command | What it does |
|---|---|
| `ZS.DebugGiveItem <object path> [count]` | Grants any `UZSItemConfig` item straight into `CarrySlots` — no placed pickup or loot roll needed. e.g. `ZS.DebugGiveItem /Game/ZS/Items/DA_Bag.DA_Bag`. |
| `ZS.DebugDropFirstItem` | Drops 1 unit of the first item in `CarrySlots`. |
| `ZS.DebugEquipFirstBagItem` | Equips the first bag/clothing-type item you're carrying into its own Back/Hip slot — this is how you get the bag's `CarryCapacityBonus` (extra slots) without any UI. |
| `ZS.DebugStoreFirstItemInBag` | Moves the first non-bag item into the first bag-type item (must already be equipped via the command above). |
| `ZS.DebugListCarrySlots` | Logs your full `CarrySlots` to the Output Log, including nested bag contents and weight. |

*Loadout/combat:*
| Command | What it does |
|---|---|
| `ZS.DebugEquipFirstToSecondaryHand` | Tries to equip the first SecondaryHand-legal item (toggleable, or a `OneHanded`+`bUsableInSecondaryHand` weapon) into SecondaryHand, logs `SUCCEEDED`/`REJECTED`. |
| `ZS.DebugTriggerSecondaryAction` | **New.** Fires/toggles whatever's in SecondaryHand — stand-in for the `T` key until `IA_SecondaryAction` exists. |
| `ZS.DebugForceJam` | **New.** Forces `CurrentWeapon` to jam instantly, instead of grinding the ~1% per-shot chance. |
| `ZS.DebugRackFirearm` | **New.** Racks `CurrentWeapon`/`SecondaryWeapon` to clear a jam — stand-in until `IA_Rack` exists. |
| `ZS.DebugPerformFinisher` | **New.** Attempts a finisher on a nearby downed zombie (within `FinisherRange`) — stand-in until `IA_Finisher` exists. |

*Health/wounds:*
| Command | What it does |
|---|---|
| `ZS.DebugListWounds` | Logs `CurrentHealth`, bite `InfectionStage`, and every body zone's wound state (type, bleeding, clean, splinted, critical bleed, infection source, amputated, wound infection). |
| `ZS.DebugUseItem <object path> [Zone 0-3]` | **New.** Applies a bandage/disinfectant/splint/consumable config to a zone (0=Head 1=Torso 2=Arms 3=Legs, default Torso) — doesn't require actually carrying the item first. e.g. `ZS.DebugUseItem /Game/ZS/Items/DA_ZS_ItemConfig_Bandage.DA_ZS_ItemConfig_Bandage 1`. |
| `ZS.DebugAmputateZone <Zone 2 or 3>` | **New.** Amputates Arms(2)/Legs(3) directly — no infection-progress gate exists, so this works any time. |
| `ZS.DebugKillSelf` | **New.** Applies 9999 Bite damage to Torso — instant death that also exercises the infection-roll/zombie-conversion path, for repeatable death-flow testing without a real fight. |

*Needs/survival:*
| Command | What it does |
|---|---|
| `ZS.DebugListNeeds` | **New.** Logs Hunger/Thirst/Fatigue/Stamina/Temperature/Wet, all 4 severity tiers, and performance/perception multipliers in one shot. |
| `ZS.DebugSetWet <0\|1>` | **New.** Sets `bIsWet` directly. |
| `ZS.DebugSetIndoors <0\|1>` | **New.** Sets the indoor/outdoor Temperature input directly. |
| `ZS.DebugToggleSleepReady` | **New.** Toggles ready-to-sleep — stand-in until `IA_Sleep` exists. |
| `ZS.DebugSetTimeCompression <seconds, default 1440>` | **New, important.** See "Time compression" below — this is the real lever for every time-gated test, not `Server_AdvanceTimeByGameHours`. |

*World:*
| Command | What it does |
|---|---|
| `ZS.SpawnZombies <n>` | Spawns `<n>` (1-500) zombies around you. |

**Time compression — read this before any wait-based test.** There are two different "time" systems in this game and they don't talk to each other:
- **The displayed world clock** (`TimeOfDayHours`/`DayCount`, and anything the sleep system jumps forward) — this is what `AZSGameState::Server_AdvanceTimeByGameHours` moves. It does **not** speed up anything below.
- **Everything that decays/progresses over real time** — Wet dry-out, Temperature drift, Hunger/Thirst/Fatigue decay, wound-infection onset, fracture recovery, bite-infection stage progression. All of these independently convert real `DeltaTime` into "game hours" using the same ratio (`RealSecondsPerGameDay`, default 1440 = 1 real minute per game hour). **`ZS.DebugSetTimeCompression <seconds>` is the actual way to speed these up** — it overrides that ratio live, no rebuild needed (e.g. `ZS.DebugSetTimeCompression 60` makes 1 game-hour = 1 real second). Set it back to `1440` afterward if you want realistic pacing again for other tests.
- For anything not worth fully compressing, the older approach still works too: **check the rate over a short window** instead of waiting it out (e.g. confirm progress climbs ~10 units over 10 real minutes rather than waiting the full 240).

**Multiplayer setup**: PIE → Advanced Settings → Multiplayer Options → Number of Players ≥ 2, Net Mode "Play As Listen Server."

**Profiling**: `stat fps`, `stat unit`, `stat ai` — standard engine commands, needed for the stress test at the end.

---

## The walkthrough

Work top to bottom — later sections build on earlier ones, so an early failure may explain a later one.

### 1. Item durability persists across equip/unequip
**✅ Confirmed working, 2026-07-29.**
**Needs:** a weapon config with `MaxDurabilityHits > 0` (check the `DA_ZS_WeaponConfig_*` asset — 0 means unbreakable, nothing to test).

1. Loot the weapon (world pickup, or a container's "loot all").
2. Press its hotbar number, wait for the equip delay, confirm it's now your active weapon.
3. Melee-swing it until you've landed about half of `MaxDurabilityHits`. To watch the number without hunting the Outliner: either click directly on the weapon mesh on your character in the viewport (selects it, syncs Details), or add a temporary "Print String" debug key calling `GetCurrentDurability()`.
4. Switch to a different hotbar slot, then switch back.
   - **Pass:** durability still reads the same half-value, not reset to full.
5. Keep swinging until it hits 0.
   - **Pass:** the weapon actor is destroyed, its instance is gone from `CarrySlots` (check with `ZS.DebugListCarrySlots`), and the hotbar slot is empty.

### 2. Bag capacity & nested contents
**Needs:** a bag item (`bIsEquippable = true`, `EquipSlot = Back` or `Hip`, `CarryCapacityBonus > 0` — see "Start here" #3 above) and one other plain item.
**✅ Core mechanism confirmed working, 2026-07-29** — grant/equip/store (steps 2-4) all verified in PIE. Step 5 (drop/re-pickup persistence) deliberately deferred by you for later; steps 6-7 (2-client check, condition-quality variance) still open.
> Bag nesting is capped at one level by design — a bag can't hold another bag. `Server_StoreInBag` returns `false` cleanly if you try; worth a quick check that it does.

1. Note `GetMaxCarryWeight()` before equipping the bag.
2. Get a bag into `CarrySlots` — either loot one from the world/a container if one's placed, or run `ZS.DebugGiveItem /Game/ZS/Items/DA_Bag.DA_Bag` if not (see "Start here" #3). Then run `ZS.DebugEquipFirstBagItem` (no bound input exists yet to equip a gear slot from the world, this is the stand-in until real inventory UI lands).
   - **Pass:** `GetMaxCarryWeight()` rises by the bag's `CarryCapacityBonus`.
3. Loot a second, non-bag item (or `ZS.DebugGiveItem` one, e.g. the pistol/crowbar configs you're already carrying by default).
4. Run `ZS.DebugStoreFirstItemInBag`.
   - **Pass:** `ZS.DebugListCarrySlots` shows the item indented under the bag, no longer as its own top-level line.
5. Drop the bag, walk up to it, pick it back up.
   - **Pass:** the item is still nested inside.
6. **2-client check:** with a second player connected, inspect the bag from the **host's** Outliner after it replicates — confirm the nested contents arrived on their side too.
7. Loot the *same* rare-tier item twice.
   - **Pass:** `ConditionQuality` differs between the two instances (it's rolled per-instance).

### 3. Ammo as a real carried item
**✅ Solo pass already confirmed** (2026-07-27) — pickup and reload both work. Only the 2-player leg below is still open.
1. ~~Get ammo into `CarrySlots`, equip the matching weapon, fire it empty, press Reload.~~ **Done, confirmed.**
2. **Still to test:** drop some ammo, have a second player pick it up, confirm they can reload the same weapon type from it.

### 4. Two-Handed blocks SecondaryHand
**✅ Confirmed working, 2026-07-29.**
There's no bound input for equipping SecondaryHand at all yet (that's real UI/input work, section 12 below), so this needs the new debug command: `ZS.DebugEquipFirstToSecondaryHand` (needs a rebuild). It picks the first item in your `CarrySlots` legal for SecondaryHand — a toggleable item (the flashlight) or a `OneHanded` weapon flagged `bUsableInSecondaryHand` — and tries to equip it, then logs `SUCCEEDED` or `REJECTED`. You don't need to know which of your weapons is which `Handedness` up front; the command logs your current primary weapon's name each time so you can cross-check afterward.

1. Get a flashlight into `CarrySlots` if you don't have one: `ZS.DebugGiveItem /Game/ZS/Items/DA_ZS_ItemConfig_Flashlight.DA_ZS_ItemConfig_Flashlight` (a toggleable item is the simplest legal SecondaryHand candidate — it doesn't care about weapon `Handedness` at all, only a weapon does).
2. Equip a weapon as your primary (hotbar 1-9). Most weapons default to `TwoHanded` unless a specific config was changed (see `CLAUDE.md`'s B0-T2.12 note) — if you're not sure which one you have equipped, the command's log line names it.
3. Run `ZS.DebugEquipFirstToSecondaryHand`.
   - **Pass:** logs `REJECTED`, and `Character->GetSecondaryHandInstanceId()` stays empty (confirm via Details on your pawn, or the log's `unchanged` value being all-zero).
4. Switch primary to bare fists (unequip — hotbar to an empty slot, or however you currently drop to bare-handed) so `CurrentWeapon` is null. This sidesteps needing to know which specific weapon config is `OneHanded` — no equipped weapon at all can't be `TwoHanded`.
5. Run `ZS.DebugEquipFirstToSecondaryHand` again.
   - **Pass:** logs `SUCCEEDED`, `SecondaryHandInstanceId` now matches the flashlight's InstanceId from step 1.

### 5. Wound zones & bleeding
**✅ Damage/infection pipeline itself confirmed working** — `ZS.DebugListWounds` mid-fight showed `Torso: WoundType=Bite Bleeding=1 InfectionSource=1`, exactly matching a fresh bite. The live Details panel showing `BodyZones` empty at the same moment was a display quirk, not real data loss (see `CLAUDE.md`'s MCP/Editor Tooling lessons) — trust `ZS.DebugListWounds` over the panel.

🐛 **Two real findings from 2026-07-29 testing, both root-caused by reading the code — read before retesting:**

- **"Always Torso, no matter which angle I approach from" — root cause found, this is a real gap, not a testing mistake.** `AZombieCharacter::Server_MeleeAttack` (`ZombieCharacter.cpp`) determines the hit zone with a straight horizontal line-trace from the zombie's center to the target's center, both offset by a **fixed** `+40` on Z. That height doesn't change no matter which direction the zombie approaches from — front, side, or behind, the trace always samples the same vertical band on your character, which lands on Torso-region bones essentially every time. Approach angle was never going to produce zone variance, because the code doesn't vary trace *height* at all, only trace *direction* (which doesn't matter for a straight through-the-middle line). **This needs a real fix to ever produce Head/Arms/Legs hits** — the cleanest approach, mirroring the existing headshot-weighting precedent for player weapons (`Server_Fire_Implementation`'s chance-roll override of `Hit.BoneName`), would be a weighted random zone roll on each zombie bite (e.g. mostly Torso, occasionally Head/Arms/Legs) instead of relying on trace geometry. **This is a real design call** (what should the actual odds be per zone?) so I haven't implemented it — say the word and I will, with a proposed default weighting to review.
- **"Zombie only attacks once, can't get a second hit in" — traced the C++, it's clean; the actual cause is very likely in `BT_Zombie`'s graph, which needs your eyes in the editor, not something fixable from code.** `Server_MeleeAttack`'s cooldown (1.5s) is a per-call gate only — nothing in C++ prevents repeat calls, and `BTTask_MeleeAttack` (the leaf node that calls it) is stateless, just triggers the attack and returns `Succeeded` every time it's ticked. The Blackboard key it depends on (`bIsInMeleeRange`) does keep updating every 0.2s (confirmed `AZombieAIController::Tick` really runs). That means the zombie *not* re-attacking while still adjacent is almost certainly the `BT_Zombie` graph itself not re-entering the melee-attack branch after the first `Succeeded` — e.g. missing a Loop/Repeat wrapper around it, or a decorator with "observer aborts" that doesn't re-trigger. Worth opening `BT_Zombie` and checking whether the Melee Attack task sits inside something that re-ticks it while `bIsInMeleeRange` stays true, or whether the tree just falls through to a different branch (Chase/Wander) and never comes back. I can't inspect or fix the graph itself without editor/MCP access.

1. **Zone variance.** Blocked for now by the fixed-height-trace issue above — no angle will produce a different zone until that's fixed. Hold off on this specific check.
   - **Pass:** the zone showing a non-`None` `WoundType` varies across attempts, not always Torso.
2. **Critical head bleed.** Take repeated Head hits until a bleed starts there (only 8% chance per fresh Head bleed, expect several tries).
   - **Pass:** once `bCriticalBleed` is set, health drains noticeably faster (4/s vs. normal). Bandage it — both `bCriticalBleed` and `bBleeding` should clear.
3. **Fracture recovery.** Take a Legs Fracture hit. Watch `FractureRecoveryProgressGameHours` — check the *rate* (~1/real-minute) rather than waiting the full 240h.
   - **Pass:** splinting sets `bSplinted = true`. A fresh Fracture hit on the same zone mid-recovery resets progress to 0.

### 6. Two-tier infection
**⏸ Deferred by you to a dedicated future testing pass, 2026-07-29** — noted, not treated as failing.

🐛 **Bug found and fixed while you were testing this**: `ZS.DebugUseItem .../DA_ZS_ItemConfig_Bandage <Zone>` accepted `torso` as the `<Zone>` argument, but the command only parsed raw numbers (0-3) — `FCString::Atoi("torso")` silently returns `0` for any non-numeric text, so it applied the bandage to **Head** instead of Torso with no error at all ("It looks like the bandage only applies to Head" was this bug, not a `Server_ApplyBandage` bug). Fixed: `ZS.DebugUseItem`/`ZS.DebugAmputateZone` now both accept the zone by name (`head`/`torso`/`arms`/`legs`, case-insensitive) or by number, and warn instead of silently defaulting if given something unrecognized. **Needs a rebuild.**

Run `ZS.DebugSetTimeCompression 30` (or lower) first — at the default 1440, waiting out 24h of wound-infection onset or 48-96h of bite-infection stages for real is impractical. Set it back to `1440` when you're done with this section.

1. **Wound infection.** Leave a dirty wound (`bClean = false`, default for any fresh unbandaged wound) untreated past `WoundInfectionOnsetGameHours` (24h), checking `ZS.DebugListWounds` periodically.
   - **Pass:** `WoundInfectionState` flips `None → Infected`, and bleed/fracture-recovery visibly worsens.
2. **Clearing it.** Run `ZS.DebugUseItem /Game/ZS/Items/DA_ZS_ItemConfig_Bandage.DA_ZS_ItemConfig_Bandage torso` (name or number both work now) — a clean bandage disinfects too.
   - **Pass:** `WoundInfectionState` clears to `None` right away (check via `ZS.DebugListWounds`). The separate *bite* infection (`InfectionStage`) is untouched either way — they're independent systems.
3. **Bite infection duration.** Get bitten (non-lethally — `ZS.DebugKillSelf` in section 10 applies lethal damage, not useful here since you need to survive the hit to watch the infection play out) and let the hidden 40% infection chance land, may take a few bites. Watch `InfectionStage` progress `Incubating → Queasy → Fever → Critical` via `ZS.DebugListWounds`, with time compression still active from step 1.
   - **Pass:** total time-to-death varies 48-96h across *multiple separate* infections — this is a statistical check across several playthroughs, not one pass/fail. A bandage/disinfectant with a nonzero incubation-delay stat should step `InfectionStageProgressGameHours` backward (no such item is authored yet — every existing one is 0/no-effect, so this specific sub-check can't be verified until one is).

### 7. Amputation & blackout
**⏸ Deferred by you to a dedicated future testing pass, 2026-07-29** — noted, not treated as failing.

Turns out amputation has no infection-progress gate at all — `AmputateZone` works on Arms/Legs any time, infected or not (confirmed by reading the code, not assumed). Use `ZS.DebugAmputateZone <2=Arms|3=Legs>` directly — no need to progress an infection first unless you specifically want to confirm amputating the infection-source zone clears it (step 1 below).
1. Get a bite infection going on an Arm or Leg (see section 6), then run `ZS.DebugAmputateZone` on that same zone.
   - **Pass:** a busy/timed window occurs, then `InfectionStage` clears, `bAmputated` is set (`ZS.DebugListWounds`), and the zone's multiplier is permanently 0.25 regardless of any other wound.
2. Immediately after:
   - **Pass:** `bIsBlackedOut` is true — movement disabled, can't attack/fire, but you're still damageable/targetable (not the death path).
3. **Solo:** watch the world clock jump forward 12h the instant blackout begins (check the value right before and after — it's a discrete jump, not a wait). Then wait out 60 real seconds.
   - **Pass:** `bIsBlackedOut` clears on its own.
4. **Co-op:** black out one player; a teammate walks up, finds the revive prompt active (only while blacked out), interacts (`F`).
   - **Pass:** blackout ends immediately, not on the remaining timer.
5. Amputate an Arm specifically.
   - **Pass:** equipping a `TwoHanded` weapon afterward is rejected.

### 8. Needs simulation (hunger/thirst/fatigue/stamina/temperature/wet)
**✅ Confirmed working, 2026-07-29** — items 1, 3, 4, 5, 6 below all verified in PIE. Item 2 deferred (no audio yet, see below). Item 7's command has been improved (see below), worth a quick retest after rebuild but not expected to reveal anything new.

Use `ZS.DebugListNeeds` throughout instead of the Details panel — one log line for everything, and consistent with the Details-panel display quirk already found in section 5.

1. Run `ZS.DebugSetWet 1`.
   - **Pass:** `ZS.DebugListNeeds` shows `Wet=1`, auto-clears after 2 real minutes (`WetDryOutGameHours`) — or instantly if you've lowered time compression (section 6). **Confirmed.**
2. While wet and walking (not sprinting — sprint reports noise separately), confirm a noise event fires every 0.6 real seconds. **This only matters with a zombie nearby** — it's a comparative test (does a zombie react to a wet player sooner than a dry one), not just "does the function run." **Deferred by you** — no audio cue exists yet to notice a zombie reacting, makes this hard to judge by feel; address in a future pass once there's something audible/visible to confirm against.
3. Run `ZS.DebugSetIndoors 1`/`0`, equip something with an `InsulationValue` (needs content — none exists yet), watch `Temperature` (via `ZS.DebugListNeeds`) move toward the new target. Push past the hypothermia/hyperthermia thresholds (25/75) and confirm the performance multiplier drops below 1.0 near those edges. **Confirmed** — Temperature correctly rises back toward neutral when going indoors.
4. With Hunger/Thirst/Fatigue/Temperature all at their best values simultaneously:
   - **Pass:** `ZS.DebugListNeeds`' `PerformanceMult` reads exactly 1.00, never higher. **Confirmed.**
5. Load past 1.5x your max weight (`ZS.DebugGiveItem` a bunch of something heavy) and sprint.
   - **Pass:** Stamina drains faster (up to 2x), but sprint is only blocked once Stamina hits 0 — never blocked by weight alone. **Confirmed.**
6. Run `ZS.DebugSetTimeCompression 30` and walk each need through all 4 severity tiers, checking `ZS.DebugListNeeds`' `(tierN)` suffixes.
   - **Pass:** tier changes happen at 75/50/25 as expected. **Confirmed.**
7. Get a zombie to notice you, immediately run `ZS.DebugToggleSleepReady` (stand-in until `IA_Sleep` exists).
   - **Pass:** blocked until the detection cooldown elapses. **Known, expected gap:** once that clears, sleep succeeds anywhere, indoors or not — the "real shelter" check doesn't exist until B4. Not a bug.
   - 🔧 **Command feedback bug found and fixed**: the log only ever said `requested`, giving no way to tell whether the toggle actually succeeded or was silently blocked by `IsSafeToSleep()`. Fixed — it now logs the before/after ready state and `IsSafeToSleep()`'s value directly. **Needs a rebuild** to see the improved log; worth a quick retest but the underlying gate itself was never in doubt.

### 9. Camera & aiming
🐛 **Bug reported 2026-07-29: scroll wheel, either direction, zooms in and stays zoomed in.** Diagnosed by reading the code, not yet fixable by me directly — this is almost certainly a content/configuration issue in the `IA_Zoom` Input Action or its mapping in `IMC_ZS_Default`, not a C++ bug:
- The C++ math (`UZSCameraDirector::ApplyManualZoom`) is simple and correct: it trusts whatever signed float value Enhanced Input hands it (`Value.Get<float>()` from `HandleZoom`) and subtracts it from the target distance — a positive value zooms in, negative zooms out, both directions clamped between `MinCameraDistance`/`MaxCameraDistance`. There's no accumulation bug or missing clamp on the C++ side.
- If scrolling **either direction** produces zoom-in, that means Enhanced Input is handing `HandleZoom` a **positive value regardless of scroll direction** — which happens if `IA_Zoom`'s key mapping uses the two separate discrete "Mouse Wheel Axis Up" / "Mouse Wheel Axis Down" keys (each fires an unsigned 1.0 pulse) without a **Negate** modifier on one of them, instead of the single combined "Mouse Wheel Axis" key (which is already correctly signed on its own, no modifiers needed).
- **To check/fix**: open `IA_Zoom.uasset` in the editor, look at its key mappings (or `IMC_ZS_Default`'s row for `IA_Zoom`). If you see two separate wheel-direction keys, either delete one and use the single signed "Mouse Wheel Axis" key instead, or add a Negate modifier to whichever direction is missing one. "Permanently zoomed in" is consistent with this exact mistake — every scroll tick nudges `ManualTargetArmLength` down toward `MinCameraDistance` regardless of direction, and nothing ever pushes it back the other way.

1. Confirm there's no way to leave TopDown view at all.
2. Fix the scroll-wheel issue above, then confirm: scroll wheel and `=`/`-` zoom smoothly between 600-1400 units, not a snap, and scrolling **down** actually zooms back out.
3. Press `1`-`9` for hotbar. Confirm scroll wheel does nothing to the hotbar anymore — it's 100% reassigned to zoom.
4. Pick one stationary **player** target (not a zombie — see why below) at a fixed distance. Fire 20-30 shots hip-fired, then the same aimed.
   - **Pass:** the aimed group is visibly tighter, and its headshot rate is noticeably higher (~25% aimed vs. ~5% hip-fired). You need real sample size — a handful of shots won't show a clean split. This only shows up against a player target because zombies have no hit-zone model yet.
5. **Full pass:** 20+ real minutes — at least one interior space, 3+ zombie fights at range and melee, loot a container, check readability at both zoom extremes. Anything that feels off is a tuning note, not a bug — there's no fallback camera to revert to.

### 10. Death, loot & zombie conversion
**Needs:** `AZSPlayerCharacter::DeathZombieClass` assigned (done in "Start here" #2, if you did the "while you're in there" step).
`ZS.DebugKillSelf` (new) applies 9999 Bite damage to Torso, instantly killing you — repeatable death testing without a real fight every time. Use it for steps 2-3 below; it doesn't help step 4 (that one specifically needs infection to run its full course, not instant lethal damage).

1. Load up: something hotbarred, something in Back/Hip, a bag with a nested item. Note the list (`ZS.DebugListCarrySlots`) before dying.
2. Run `ZS.DebugKillSelf` (or die to zombie damage normally).
   - **Pass:** every item — hotbarred, equipped, nested-in-bag, all of it — drops as its own world pickup at the death spot, preserving durability/condition. Check your half-durability weapon from section 1 specifically.
3. **Pass:** a new zombie spawns at the same spot.
4. Repeat via a bite-infection death instead of direct damage — get bitten, let it progress to Critical and run out (use `ZS.DebugSetTimeCompression`, section 6, to not wait 48-96h for real).
   - **Pass:** identical drop/zombie-spawn behavior.
5. Respawn.
   - **Pass:** you get the default starting loadout, not the gear you just dropped — same in solo and co-op.

### 11. Combat revision (jamming, melee stamina, downed/finisher)
1. Run `ZS.DebugForceJam` on your equipped weapon (new — a pristine weapon's real ~1%/shot chance makes this impractical to hit naturally; a heavily-degraded one, like your section 1 weapon, jams up to 30%/shot if you'd rather test it the real way).
   - **Pass:** once jammed, firing does nothing and consumes no ammo. Run `ZS.DebugRackFirearm` (new, stand-in until `IA_Rack` exists), wait the clear-jam time, firing resumes.
2. Melee repeatedly, bare-fisted then weapon-equipped. Watch Stamina drop per swing whether it connects or not.
   - **Pass:** swings still execute at 0 Stamina — no hard block, that's intentional (see decisions log).
3. Land a hit clearing the downed threshold (150 knockback) on a zombie.
   - **Pass:** it flips to downed, stops moving/attacking entirely (its whole behavior tree is paused as a stand-in, see decisions log), and recovers on its own after 6 seconds if left alone.
4. While a zombie is downed, land a normal standing melee swing on it anyway.
   - **Pass:** it doesn't register at all — downed zombies are excluded from standing-melee targeting.
5. Within range of a downed zombie, run `ZS.DebugPerformFinisher` (new, stand-in until `IA_Finisher` exists).
   - **Pass:** guaranteed kill either bare-handed or weapon-equipped. The finishing move itself is a no-op cosmetically (no montage authored yet) but the kill should land regardless.
6. **Full pass:** loot → hotbar → equip → degrade near-broken → jam → clear jam → break entirely. Separately: melee to 0 Stamina. Separately: knock down and finish a zombie both bare-handed and weapon-equipped.

### 12. SecondaryHand & flashlight
1. With a `TwoHanded` primary, run `ZS.DebugEquipFirstToSecondaryHand`.
   - **Pass:** rejected (same as section 4).
2. Switch to `OneHanded`/bare fists. Make sure you have a flashlight (`ZS.DebugGiveItem /Game/ZS/Items/DA_ZS_ItemConfig_Flashlight.DA_ZS_ItemConfig_Flashlight` if not), run `ZS.DebugEquipFirstToSecondaryHand` again, then `ZS.DebugTriggerSecondaryAction` (new, stand-in until `IA_SecondaryAction` exists).
   - **Pass:** a real spotlight visibly turns on/off (rough chest-height placement for now, not a real hand socket — don't be alarmed it's not final-looking).
3. Equip a one-handed *weapon* into SecondaryHand instead (needs `bUsableInSecondaryHand` set on a config, e.g. an offhand pistol — check whether any real weapon config has this flag set yet; if none do, this specific sub-step is still a content gap, not a bug). Use `ZS.DebugTriggerSecondaryAction` to fire/swing it.
   - **Pass:** the slot accepts it, and you can see it spawn/attach on your character. Triggering a ranged config: it should fire (ammo drops, jam/headshot rolls apply, same as primary). Triggering a melee config: it should swing and can break, clearing the slot on break.
   - **Two deliberate scope cuts, not bugs:** no auto-fire (each trigger is one shot/swing), and primary + secondary share one attack cooldown (firing primary then immediately trying secondary should be gated the same as firing primary twice in a row).
   - **Known visual gap:** both weapons currently render at the same attachment socket — overlapping meshes are expected, not a bug, until a dedicated offhand socket is authored.

### 13. Stress-test spawning
**Needs:** `StressTestZombieClass` assigned (see "Start here" #2 above).
1. In any existing level, run `ZS.SpawnZombies 25`.
   - **Pass:** exactly 25 zombies appear scattered around you.
2. Repeat at 50, 100, 150, 250, checking `stat fps`/`stat unit`/`stat ai` after each batch settles (give the AI a few seconds to start ticking first). Record the numbers somewhere — this run *is* the profiling work, there's no separate step after it.

---

## Bugs already fixed this session — worth retesting specifically

None of these have been through PIE yet. When you do get to the related section above, these are the specific things to double-check since they were silently broken before:

| Fixed | What to check now |
|---|---|
| Magazine left behind on unequip | Unequip a rifle — magazine should disappear with it (section: Start here #1) |
| Offhand weapon durability reset on every equip | Equip a damaged offhand weapon, unequip, re-equip — durability should persist, not reset to full |
| Dying with a weapon equipped leaked the weapon actor & dropped stale-durability loot | Die with a damaged weapon equipped — dropped loot should show the real durability, and no ghost weapon actor should remain |
| A zombie killed while downed stayed flagged "downed" forever | Down a zombie, then kill it — its corpse shouldn't be stuck in the downed pose/state |
| A dead/blacked-out player's stale "ready to sleep" flag could let the party sleep anyway | Have one player die or black out while ready-to-sleep, confirm the party's sleep vote no longer counts them |
| A jammed offhand weapon had no way to ever clear the jam | Jam an offhand weapon specifically, confirm rack-firearm clears it |
| A lower-severity hit onto an already-Fractured zone could incorrectly set bleeding | Fracture a zone, then land a lighter hit (e.g. Scratch) on it — bleeding shouldn't newly start from that lighter hit |
| Storing an equipped Back/Hip item into a bag silently orphaned the gear slot | Try to store your equipped backpack/holster item into another bag — should be rejected, not silently break the slot |

---

## Automated test coverage (headless, not PIE — supplements the above, doesn't replace it)

`Source/ZombieShooter/Tests/ZSAutomationTests.cpp` covers the pure state/math parts of this checklist so they don't need re-verifying by hand every time. Run via:
```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -ExecCmds="Automation RunTests ZS.; Quit" -unattended -nopause -nullrhi -log=some_name.log
```
Always pass an explicit `-log=` — the default log path silently fails while the GUI editor is open too.

**21 tests total** as of this rewrite (added `ZS.Weapons.DestroyingWeaponAlsoDestroysMagazine` for the fix above). Full list and individual pass/fail history: `Docs/Beta/B0_Stabilization.md`. Two known standing items:
- `ZS.Inventory.BagStoreAndRetrieve` **fails on a real finding**, not a test bug: `DA_Bag.bIsEquippable` was `false` — see "Start here" #3.
- Everything written 2026-07-29 (the offhand-weapon and bug-fix regression tests, 11 of the 21) has never actually been compiled or run yet — say the word for a test session (present-session, dev-triggered only).

**Still fundamentally needs PIE, no way around it:** 2-client replication, camera/aim feel, combat feel, anything visual (flashlight placement, animations), the full hotbar/equip integration (as opposed to the underlying mechanism), offhand-weapon-fire, Shove/Mount-Climb (undesigned), and real `BT_Zombie` branching behavior.

---

## Content still needed

Doesn't block compiling, but several features do-nothing without it. The two most urgent (bag, stress-test class) are called out at the top of this doc since your own testing already hit them.

**Input Actions** (create the `.uasset`, wire into `IMC_ZS_Default`, C++ picks it up automatically). None of these block testing anymore — each has a `ZS.Debug*` console stand-in (see the command table above) — but all are still real content gaps for actual play:
- `IA_Zoom` — Axis1D, mouse wheel + `=`/`-`. **Already exists but misconfigured** — see section 9's scroll-wheel bug.
- `IA_Rack` — digital, `Alt+R` ("Rack Firearm"). Stand-in: `ZS.DebugRackFirearm`.
- `IA_Finisher` — digital, `Space`. Stand-in: `ZS.DebugPerformFinisher`. ⚑ Naming flagged for review — `Space` is meant to be one shared context-aware action (finisher today, Shove/Mount-Climb later), but it's currently named narrowly just for the finisher case, unlike this project's usual pattern for dispatching inputs (one generic action, C++ branches internally). Worth renaming before Shove/Mount-Climb get built expecting a narrow name — see the dev's own `IA_ComboAction.uasset` (purpose TBD, may already be this).
- `IA_SecondaryAction` — digital, `T`. Stand-in: `ZS.DebugTriggerSecondaryAction`.
- `IA_Sleep` — digital. Stand-in: `ZS.DebugToggleSleepReady`.

**Dead content to delete:** `Content/ZS/Input/IA_ToggleView.uasset` — the C++ reference was deleted (TopDown-only now), it just sits unused.

**Data Assets to author or extend:**
- Ammo item configs + assign `AmmoItemConfig` on every weapon (reload doesn't work at all without this — pre-existing gap)
- Real `Weight` values on every weapon config (still the placeholder 0.5)
- `DA_ZS_ItemConfig_Flashlight` (`bIsToggleable = true`)
- Clothing with a real `InsulationValue` — optional, defaults to 0/no warmth without it
- Pistol's own hip-fire/aimed spread values (currently inherits the rifle's, too tight)

**Blueprint class references to assign** (both `EditDefaultsOnly` — only settable on Class Defaults, never live in PIE):
- `AZSPlayerCharacter::DeathZombieClass` on `BP_ZS_PlayerCharacter`
- `AZSGameMode::StressTestZombieClass` — see "Start here" #2 above

**Not built at all:** `Lvl_ZS_StressTest` graybox map — doesn't block the stress-test command itself, just the dedicated map for it.

---

## Decisions log

Judgment calls made without you while PIE access was unavailable. None of these block testing — flagging for a second look, not blocking on it.

| Area | What I decided | Why |
|---|---|---|
| Melee stamina | No hard Stamina gate on melee — swings execute at 0 Stamina, cost is just a drain with no block | Matches this project's soft-penalty-everywhere-else pattern. Could instead be read as wanting a real block at 0 — your call. |
| Downed zombie state | The whole behavior tree is paused/resumed rather than a real branch in `BT_Zombie`'s graph | No editor access to add a proper branch this session. Works, but a real branch (e.g. a prone idle pose) would look better once you have editor time. |
| Player-becomes-zombie loot | Loot pile and the spawned zombie are just co-located, not a literal zombie inventory | A real carry-inventory on zombies would be a much bigger feature, and zombies deliberately don't share players' inventory machinery. Confirm this matches what you pictured. |
| Headshot weighting | A chance-roll overrides the hit to "head" rather than deriving it from real geometry | Minimal-diff way to reuse the existing zone-inference code. Functionally identical outcome, just worth knowing it's a probability roll layered on the physical trace, not a literal "did the ray hit the head" check. |
| Needs severity tiers | Wet has no 4-tier severity (binary), Injury/Infection use their own existing concepts rather than the shared 0-100/4-tier scale | Reads most naturally this way once you look at what each need actually is, but it's an interpretation, not an explicit prior call. |

**Deliberately not built (documented gaps, not oversights):**
- Shove and Mount/Climb — bundled on the same `Space` input per the input doc, but genuinely undesigned. Only the finisher exists.
- Racking beyond jam-clearing — whether "Rack Firearm" needs a role after certain reloads too (not just jam-clear) is still open.
- Real shelter check for sleep — only the aggro-cooldown half is real; "must be indoors/barricaded" is stubbed true until B4.
- `BT_Zombie` branch for the downed state — see decisions table above.
- `BT_Zombie`'s `ClearLastKnownLocation` wiring — pre-existing open item, ambiguous overlap with the investigation timer's own expiry, not touched.

**Future design note, not needed for B0 (dev's own flag, 2026-07-29):** carried weapons currently just `Destroy()` when unequipped (writing back durability first) rather than staying visible on the character somewhere (e.g. holstered on the back). Dev wants this eventually — a real cosmetic attach/detach system rather than spawn-on-equip/destroy-on-unequip. Explicitly not urgent; noted here so it isn't lost, not scheduled against any B-phase yet.
