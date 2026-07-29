# B0 Manual Steps — Test Walkthrough & Decisions

> **What this is.** Everything in this doc needs your hands — either manual PIE testing (no automation path exists, see `CLAUDE.md`) or a real editor content-authoring step. Full technical detail per sub-task lives in `Docs/Beta/B0_Stabilization.md`; compile/PIE-verification *status* lives in `Docs/SessionHandoff.md`. This doc is the walkthrough: what to click, what you should see, what counts as a pass.
>
> **Status as of 2026-07-29**: module compiles clean (pending one just-fixed bug below — rebuild first). **Nothing below has been PIE-tested yet.**

---

## Start here — do these 3 things before testing anything else

Your own testing already hit two of these directly (see console output below), so knock these out first or you'll just hit the same walls again mid-checklist.

### 1. Rebuild — a real bug just got fixed
You reported the magazine staying behind when unequipping a rifle. Root cause: the magazine is a separate actor just glued onto the weapon visually, and destroying the weapon didn't take the magazine with it. Fixed in `AZSWeapon::Destroyed()`. **Needs a rebuild before you'll see it fixed.** Retest: equip a rifle, unequip it (switch hotbar slots or go bare-fisted) — the magazine should vanish along with the rifle, not float in place.

### 2. Assign `StressTestZombieClass`
Your `ZS.SpawnZombies` run logged:
```
Warning: ZS.SpawnZombies: AZSGameMode::StressTestZombieClass is unset
```
This is expected — nobody's assigned it yet. Steps:
1. Check if `BP_ZS_GameMode` exists in the Content Browser. If not: Content Browser → Add → Blueprint Class → parent `ZSGameMode` → name it `BP_ZS_GameMode`.
2. Open it, go to Class Defaults, find `StressTestZombieClass` (category `ZS|StressTest`), assign whatever zombie Blueprint you already use for normal spawns.
3. Make sure it's actually the active GameMode: World Settings → GameMode Override (or the project's default GameMode) → `BP_ZS_GameMode`.
4. Re-run `ZS.SpawnZombies 25` — zombies should now actually appear.

### 3. Set `DA_Bag`'s `bIsEquippable` to true
Your `ZS.DebugStoreFirstItemInBag` run logged:
```
Warning: ZS.DebugStoreFirstItemInBag: no bag-type item carried
```
`ZS.DebugListCarrySlots` confirmed why — you were only carrying a Pistol and a Crowbar, no bag. This command specifically looks for any carried item whose `Config->bIsEquippable == true`. Two things to check:
1. Open `DA_Bag.uasset`, confirm `bIsEquippable` is `true` (this was found `false` by the automation suite — may already be fixed if you've touched it recently, worth a quick look either way).
2. Make sure you've actually looted a bag into `CarrySlots` before running the command — it only searches what you're currently carrying.

Once both check out, retry `ZS.DebugStoreFirstItemInBag` — you should get a `storing X in Y` log line instead of the warning.

---

## Reading state while there's no UI yet

**Watching live values.** While PIE is running, click your pawn (or any actor — a zombie, `AZSGameState`) in the World Outliner. The Details panel updates live. Use the component dropdown at the top of Details to jump to `Zs Needs Component` / `Zs Health Component` / `Zs Inventory Component` directly. Nested arrays expand too — a bag's `ContainedItems` is browsable right there.

You can **watch** these values but not **edit** them by typing into Details — replicated gameplay state is deliberately `VisibleAnywhere`, not `EditAnywhere` (so nobody can hand-edit server state through the panel). The only way to change state is the real mechanism (get shot, get wet, get bitten) or a console command / temporary debug key calling the matching `Server_` function.

**2-player PIE.** Only the host's window has a real Outliner. To check a second client's pawn, select *their* pawn from the **host's** Outliner — the host sees every replicated actor in the session.

**Console commands that exist today** (open console with `` ` ``, all host-only, all act on the local/host player only):
| Command | What it does |
|---|---|
| `ZS.DebugDropFirstItem` | Drops 1 unit of the first item in `CarrySlots`. |
| `ZS.DebugStoreFirstItemInBag` | Moves the first non-bag item into the first bag-type item. |
| `ZS.DebugListCarrySlots` | Logs your full `CarrySlots` to the Output Log, including nested bag contents and weight. |
| `ZS.SpawnZombies <n>` | Spawns `<n>` (1-500) zombies around you. |

**No command exists yet for**: forcing wet/indoors, or forcing a specific wound. `Server_SetWet(bool)`/`Server_SetIndoors(bool)` are real functions, just no console wrapper — you'd need a temporary Blueprint debug key on `BP_ZS_PlayerCharacter` calling them. Say the word if you want `ZS.DebugSetWet`/`ZS.DebugSetIndoors` added — same pattern as the four above, quick to add.

**Time compression.** 1 game-hour = 1 real minute by default (`RealSecondsPerGameDay = 1440`). Some tests are directly waitable this way (Wet's 2h dry-out = 2 real minutes). Others aren't (an unsplinted Fracture's 240h recovery = 4 real hours) — for those, **check the rate over a short window** instead of waiting it out (e.g. confirm progress climbs ~10 units over 10 real minutes rather than waiting the full 240). If you really want to watch one finish, temporarily drop `RealSecondsPerGameDay` in `ZSGameState.h` (~line 139) to `60.f`, rebuild, test, then revert and rebuild again.

**Multiplayer setup**: PIE → Advanced Settings → Multiplayer Options → Number of Players ≥ 2, Net Mode "Play As Listen Server."

**Profiling**: `stat fps`, `stat unit`, `stat ai` — standard engine commands, needed for the stress test at the end.

---

## The walkthrough

Work top to bottom — later sections build on earlier ones, so an early failure may explain a later one.

### 1. Item durability persists across equip/unequip
**Needs:** a weapon config with `MaxDurabilityHits > 0` (check the `DA_ZS_WeaponConfig_*` asset — 0 means unbreakable, nothing to test).
**⏸ Deferred once already** (2026-07-27) — couldn't find the spawned weapon actor in a crowded Outliner. Two easier ways below fix that.

1. Loot the weapon (world pickup, or a container's "loot all").
2. Press its hotbar number, wait for the equip delay, confirm it's now your active weapon.
3. Melee-swing it until you've landed about half of `MaxDurabilityHits`. To watch the number without hunting the Outliner: either click directly on the weapon mesh on your character in the viewport (selects it, syncs Details), or add a temporary "Print String" debug key calling `GetCurrentDurability()`.
4. Switch to a different hotbar slot, then switch back.
   - **Pass:** durability still reads the same half-value, not reset to full.
5. Keep swinging until it hits 0.
   - **Pass:** the weapon actor is destroyed, its instance is gone from `CarrySlots` (check with `ZS.DebugListCarrySlots`), and the hotbar slot is empty.

### 2. Bag capacity & nested contents
**Needs:** a bag item (`bIsEquippable = true`, `EquipSlot = Back` or `Hip`, `CarryCapacityBonus > 0` — see "Start here" #3 above) and one other plain item.
**⏸ Not yet tested** (dev's own call, 2026-07-27 — waiting on more inventory/UI setup).
> Bag nesting is capped at one level by design — a bag can't hold another bag. `Server_StoreInBag` returns `false` cleanly if you try; worth a quick check that it does.

1. Note `GetMaxCarryWeight()` before equipping the bag.
2. Loot the bag. There's no bound input for equipping to a gear slot yet — call `Server_EquipToSlot` via a temporary Blueprint node.
   - **Pass:** `GetMaxCarryWeight()` rises by the bag's `CarryCapacityBonus`.
3. Loot a second, non-bag item.
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
1. Equip a `TwoHanded` weapon as primary. Try to equip anything into SecondaryHand (see section 9 below for the flow).
   - **Pass:** rejected, SecondaryHand stays empty.
2. Switch to `OneHanded` or bare fists, retry.
   - **Pass:** SecondaryHand equip now succeeds.

### 5. Wound zones & bleeding
🔍 **Two open findings from 2026-07-27, still unresolved — read before testing:**
- **"Body zones not set in editor"**: zones only populate on a spawned instance during PIE (`BeginPlay`), never on a Blueprint's Class Defaults. If you were looking at Class Defaults rather than your live pawn in the Outliner, that's why it looked empty — not a bug. Double check which one you had selected.
- **"Zombie only attacks once"**: the attack cooldown is a per-attack gate (1.5s), not a one-shot lock — nothing stops repeat attacks in the code, and a direct test (`ZS.Health.WoundZonesAndInfection`) confirms the damage pipeline itself has no hidden bug. Two remaining explanations if you see this again: (a) the zombie lost range after the first hit and never closed distance again, or (b) **if you hit it back**, a hit clearing `DownedKnockbackThreshold` (150) pauses its entire behavior tree — that looks exactly like "stopped attacking" if you didn't realize you'd knocked it down. Next time this happens, note whether the zombie kept chasing after the first bite, or just froze/wandered — that'll pin down which.

1. **Zone variance.** Get bitten from a few different angles (front, side, while facing away).
   - **Pass:** the hit zone (Head/Torso/Arms/Legs) varies across attempts, not always Torso.
2. **Critical head bleed.** Take repeated Head hits until a bleed starts there (only 8% chance per fresh Head bleed, expect several tries).
   - **Pass:** once `bCriticalBleed` is set, health drains noticeably faster (4/s vs. normal). Bandage it — both `bCriticalBleed` and `bBleeding` should clear.
3. **Fracture recovery.** Take a Legs Fracture hit. Watch `FractureRecoveryProgressGameHours` — check the *rate* (~1/real-minute) rather than waiting the full 240h.
   - **Pass:** splinting sets `bSplinted = true`. A fresh Fracture hit on the same zone mid-recovery resets progress to 0.

### 6. Two-tier infection
1. **Wound infection.** Leave a dirty wound (`bClean = false`, default for any fresh unbandaged wound) untreated past `WoundInfectionOnsetGameHours` (24h — use the rate-check approach above).
   - **Pass:** `WoundInfectionState` flips `None → Infected`, and bleed/fracture-recovery visibly worsens.
2. **Clearing it.** `Server_Disinfect`, or a clean `Server_ApplyBandage`, on that zone.
   - **Pass:** `WoundInfectionState` clears to `None` right away. The separate *bite* infection (`InfectionStage`) is untouched either way — they're independent systems.
3. **Bite infection duration.** Get bitten, let the hidden 40% infection chance land (may take a few bites). Watch `InfectionStage` progress `Incubating → Queasy → Fever → Critical`.
   - **Pass:** total time-to-death varies 48-96h across *multiple separate* infections — this is a statistical check across several playthroughs, not one pass/fail. A bandage/disinfectant with a nonzero incubation-delay stat should step `InfectionStageProgressGameHours` backward (no such item is authored yet — every existing one is 0/no-effect).

### 7. Amputation & blackout
1. Progress a bite infection to where amputation becomes available (no prompt exists yet — call `Server_AmputateZone` directly for now) on the infected zone.
   - **Pass:** a busy/timed window occurs, then `InfectionStage` clears, `bAmputated` is set, and the zone's multiplier is permanently 0.25 regardless of any other wound.
2. Immediately after:
   - **Pass:** `bIsBlackedOut` is true — movement disabled, can't attack/fire, but you're still damageable/targetable (not the death path).
3. **Solo:** watch the world clock jump forward 12h the instant blackout begins (check the value right before and after — it's a discrete jump, not a wait). Then wait out 60 real seconds.
   - **Pass:** `bIsBlackedOut` clears on its own.
4. **Co-op:** black out one player; a teammate walks up, finds the revive prompt active (only while blacked out), interacts (`F`).
   - **Pass:** blackout ends immediately, not on the remaining timer.
5. Amputate an Arm specifically.
   - **Pass:** equipping a `TwoHanded` weapon afterward is rejected.

### 8. Needs simulation (hunger/thirst/fatigue/stamina/temperature/wet)
1. Call `Server_SetWet(true)` via a temporary debug key.
   - **Pass:** `bIsWet` replicates, auto-clears after 2 real minutes (`WetDryOutGameHours`).
2. While wet and walking (not sprinting — sprint reports noise separately), confirm a noise event fires every 0.6 real seconds. **This only matters with a zombie nearby** — it's a comparative test (does a zombie react to a wet player sooner than a dry one), not just "does the function run."
3. Toggle wet/indoors, equip something with an `InsulationValue` (needs content — none exists yet), watch `Temperature` move toward the new target. Push past the hypothermia/hyperthermia thresholds (25/75) and confirm the performance multiplier drops below 1.0 near those edges.
4. With Hunger/Thirst/Fatigue/Temperature all at their best values simultaneously:
   - **Pass:** `GetPerformanceMultiplier()` reads exactly 1.0, never higher.
5. Load past 1.5x your max weight and sprint.
   - **Pass:** Stamina drains faster (up to 2x), but sprint is only blocked once Stamina hits 0 — never blocked by weight alone.
6. Walk each need through all 4 severity tiers.
   - **Pass:** tier changes happen at 75/50/25 as expected.
7. Get a zombie to notice you, immediately try to sleep.
   - **Pass:** blocked until the detection cooldown elapses. **Known, expected gap:** once that clears, sleep succeeds anywhere, indoors or not — the "real shelter" check doesn't exist until B4. Not a bug.

### 9. Camera & aiming
1. Confirm there's no way to leave TopDown view at all.
2. Once `IA_Zoom` exists (content gap, see backlog below): scroll wheel and `=`/`-` zoom smoothly between 600-1400 units, not a snap.
3. Press `1`-`9` for hotbar. Confirm scroll wheel does nothing to the hotbar anymore — it's 100% reassigned to zoom.
4. Pick one stationary **player** target (not a zombie — see why below) at a fixed distance. Fire 20-30 shots hip-fired, then the same aimed.
   - **Pass:** the aimed group is visibly tighter, and its headshot rate is noticeably higher (~25% aimed vs. ~5% hip-fired). You need real sample size — a handful of shots won't show a clean split. This only shows up against a player target because zombies have no hit-zone model yet.
5. **Full pass:** 20+ real minutes — at least one interior space, 3+ zombie fights at range and melee, loot a container, check readability at both zoom extremes. Anything that feels off is a tuning note, not a bug — there's no fallback camera to revert to.

### 10. Death, loot & zombie conversion
**Needs:** `AZSPlayerCharacter::DeathZombieClass` assigned (backlog below) — without it, death still works, you just won't see the zombie-conversion half.
1. Load up: something hotbarred, something in Back/Hip, a bag with a nested item. Note the list (`ZS.DebugListCarrySlots`) before dying.
2. Die to zombie damage.
   - **Pass:** every item — hotbarred, equipped, nested-in-bag, all of it — drops as its own world pickup at the death spot, preserving durability/condition. Check your half-durability weapon from section 1 specifically.
3. **Pass:** a new zombie spawns at the same spot.
4. Repeat via a bite-infection death instead of direct damage.
   - **Pass:** identical drop/zombie-spawn behavior.
5. Respawn.
   - **Pass:** you get the default starting loadout, not the gear you just dropped — same in solo and co-op.

### 11. Combat revision (jamming, melee stamina, downed/finisher)
1. Fire the same weapon repeatedly (empty, reload, repeat) until it jams. A pristine weapon only jams ~1%/shot, so this takes a while — a heavily-degraded one (your section 1 weapon) jams much sooner, up to 30%.
   - **Pass:** once jammed, firing does nothing and consumes no ammo. Once `IA_Rack` exists (backlog): rack it, wait the clear-jam time, firing resumes.
2. Melee repeatedly, bare-fisted then weapon-equipped. Watch Stamina drop per swing whether it connects or not.
   - **Pass:** swings still execute at 0 Stamina — no hard block, that's intentional (see decisions log).
3. Land a hit clearing the downed threshold (150 knockback) on a zombie.
   - **Pass:** it flips to downed, stops moving/attacking entirely (its whole behavior tree is paused as a stand-in, see decisions log), and recovers on its own after 6 seconds if left alone.
4. While a zombie is downed, land a normal standing melee swing on it anyway.
   - **Pass:** it doesn't register at all — downed zombies are excluded from standing-melee targeting.
5. Once `IA_Finisher` exists (backlog): within range of a downed zombie, press Space.
   - **Pass:** guaranteed kill either bare-handed or weapon-equipped. The finishing move itself is a no-op cosmetically (no montage authored yet) but the kill should land regardless.
6. **Full pass:** loot → hotbar → equip → degrade near-broken → jam → clear jam → break entirely. Separately: melee to 0 Stamina. Separately: knock down and finish a zombie both bare-handed and weapon-equipped.

### 12. SecondaryHand & flashlight
1. With a `TwoHanded` primary, try equipping anything into SecondaryHand.
   - **Pass:** rejected (same as section 4).
2. Switch to `OneHanded`/bare fists. Equip a toggleable item (needs a flashlight content asset, backlog below) into SecondaryHand. Once `IA_SecondaryAction` exists: press `T`.
   - **Pass:** a real spotlight visibly turns on/off (rough chest-height placement for now, not a real hand socket — don't be alarmed it's not final-looking).
3. Equip a one-handed *weapon* into SecondaryHand instead (needs `bUsableInSecondaryHand` set on a config, e.g. an offhand pistol).
   - **Pass:** the slot accepts it, and you can see it spawn/attach on your character. Press `T` on a ranged config: it should fire (ammo drops, jam/headshot rolls apply, same as primary). Press `T` on a melee config: it should swing and can break, clearing the slot on break.
   - **Two deliberate scope cuts, not bugs:** no auto-fire (each `T` press is one shot/swing), and primary + secondary share one attack cooldown (firing primary then immediately trying secondary should be gated the same as firing primary twice in a row).
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

**Input Actions** (create the `.uasset`, wire into `IMC_ZS_Default`, C++ picks it up automatically):
- `IA_Zoom` — Axis1D, mouse wheel + `=`/`-`
- `IA_Rack` — digital, `Alt+R` ("Rack Firearm")
- `IA_Finisher` — digital, `Space`. ⚑ Naming flagged for review — `Space` is meant to be one shared context-aware action (finisher today, Shove/Mount-Climb later), but it's currently named narrowly just for the finisher case, unlike this project's usual pattern for dispatching inputs (one generic action, C++ branches internally). Worth renaming before Shove/Mount-Climb get built expecting a narrow name — see the dev's own `IA_ComboAction.uasset` (purpose TBD, may already be this).
- `IA_SecondaryAction` — digital, `T`

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
