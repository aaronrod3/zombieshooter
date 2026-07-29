# B0 Autonomous Push — Test Checklist & Decisions Log (2026-07-26)

> **What this is.** You stepped away and asked for as much of B0 as possible to get done without you, plus a list of tests to work through on your return and a list of decisions that need your call. This is that list. Full technical detail for every item here already lives in `Docs/Beta/B0_Stabilization.md` (every sub-task row was updated as it was built) — this doc is the curated, sequential "start here" companion, not a replacement.
>
> **Update, 2026-07-27**: the module now compiles clean (`Build.bat ZombieShooterEditor Win64 Development` succeeded). One real error surfaced and was fixed along the way — `FZSItemInstance` had contained a `TArray<FZSItemInstance>` for bag contents (`ContainedItems`), which Unreal's header tool rejects (a USTRUCT can't hold an array of itself). Fixed by splitting the field's type into a non-recursive `FZSItemInstanceBase` — see the note at the top of §2.1's Checkpoint C below for the one behavior change this caused. **Nothing has been PIE-tested yet** — that's what the rest of this doc is for. Section 0 below is otherwise still accurate; items 1-2 are now done.
>
> **Update, 2026-07-28 (away session)**: T11.2's offhand-weapon-firing content gap (§3.2 below used to list this as deliberately not built) is now implemented — `AZSPlayerCharacter::SecondaryWeapon` mirrors `CurrentWeapon`'s full lifecycle, ranged fire goes through a newly-extracted `FireWeapon(AZSWeapon*)` helper shared with the primary hand, melee reuses the already-generic `PerformMeleeSwing` unchanged. Also fixed a real cross-client bug this surfaced: `AZSWeapon::OnRep_CurrentConfig` used to call the character's primary-weapon attach functions unconditionally regardless of which weapon actor triggered it. **Not built or PIE-tested** — the dev's editor was open for this entire away session, and per the 2026-07-28 workflow policy, editor-closing for a build is present-session/dev-triggered only, so it wasn't attempted. See `Docs/Beta/B0_Stabilization.md` T11.4's row for full detail; this is the first thing to verify on return, before anything else in this doc.

---

## 0. Before anything else: build

1. **Close the editor if it's open, then run a full `Build.bat` rebuild** (not Live Coding — this stretch touched dozens of headers, and CLAUDE.md's own Live Coding lesson is explicit about header changes needing a real rebuild). Expect this to surface the first real compile errors this code has seen.
2. **Regenerate IDE project files** (`Build.bat -projectfiles -project=...uproject -game -engine`) — new files this stretch: `Player/ZSCameraDirector.h/.cpp`, `Framework/ZSElevationSubsystem.h/.cpp`.
3. Once it compiles, open the editor and run a **"Compile All Blueprints" pass** (Content Browser bulk action) before trusting any PIE result — standing B0-T0.1 policy, doubly important after this much C++ churn.
4. Check the Output Log for `is not a child class of` / `invalid target type` — the Live Coding corruption pattern. Shouldn't apply (no Live Coding used), but costs nothing to check.

**If it doesn't compile clean**, that's the actual next step, not the test list below — come back to this doc once it does.

---

## 1. Content that needs authoring before some features are testable

None of this blocks compiling or most testing, but several features degrade gracefully to "does nothing" without it. Batch-authoring these first will make Section 2 go faster instead of hitting gaps mid-checklist.

**New Input Actions needed** (all follow the existing pattern — create the `.uasset`, wire it into `IMC_ZS_Default`, the C++ `ConstructorHelpers::FObjectFinder` picks it up automatically, no code change needed):
- `IA_Zoom` — Axis1D, mouse wheel + `=`/`-` (T3.4)
- `IA_Rack` — digital, `Alt+R` (T10.1, "Rack Firearm")
- `IA_Finisher` — digital, `Space` (T10.6). ⚑ **Naming flagged for review, 2026-07-27**: `Space` is documented (`InputBindings.md`) as one bundled, context-aware action covering Finisher *and* the still-undesigned Shove/Mount-Climb — but `IA_Finisher`/`FinisherAction`/`HandleFinisher` are named narrowly, for just the finisher case, unlike this project's existing pattern for context-dispatching inputs (`IA_Attack`, `IA_SecondaryAction` — one generic action, one C++ handler that branches internally). Worth renaming to something generic (e.g. `IA_ContextAction`) before Shove/Mount-Climb get built expecting a narrowly-named input — see chat discussion 2026-07-27 and the dev's own `IA_ComboAction.uasset` (purpose TBD, may already be this).
- `IA_SecondaryAction` — digital, `T` (T11.2)

**Orphaned/dead content to clean up:**
- `Content/ZS/Input/IA_ToggleView.uasset` — the C++ reference was deleted (T3.9, TopDown-only now). Delete the asset itself, or it'll just sit unused.

**Data Assets to author or extend:**
- `DA_ZS_ItemConfig_Ammo_<Caliber>` instances + assign `AmmoItemConfig` on each weapon config (T2.11 — **pre-existing gap from the prior session**, still open; every weapon's `CanReload()` returns false until this exists)
- Weapon `Weight` fields — still the inherited placeholder 0.5 on every `DA_ZS_WeaponConfig_*` (T2.4 — **pre-existing gap**)
- `DA_ZS_ItemConfig_Flashlight` — a real item with `bIsToggleable = true` (T11.4)
- Clothing items with a real `InsulationValue` (T4.4) — optional, defaults to 0 (no warmth effect) without it
- `DA_ZS_WeaponConfig_Pistol`'s own `HipFireSpreadDegrees`/`AimedSpreadDegrees` (8°→2° per OQ-B0-02) — currently inherits the rifle-shaped default (5°→1°) (T3.5)

**Blueprint class references to assign** (both are `TSubclassOf` fields that are `nullptr` by default — the features they gate simply no-op with a log warning until assigned). Both are `EditDefaultsOnly`, meaning they're **only** settable on a Blueprint's Class Defaults, never per-instance and never live in a running PIE session:
- `AZSPlayerCharacter::DeathZombieClass` (Category `ZS|Health`) — open `BP_ZS_PlayerCharacter`, Class Defaults, assign whatever zombie Blueprint you're already using for normal spawns (needs a real `UZSZombieConfig` on its CDO — a bare `AZombieCharacter` has nothing to assemble mesh/AI from). `BP_ZS_PlayerCharacter` definitely exists (`AZSGameMode`'s constructor already finds it by path).
- `AZSGameMode::StressTestZombieClass` (Category `ZS|StressTest`) — same idea, but check first whether a `BP_ZS_GameMode` Blueprint exists at all. `AZSGameMode`'s constructor doesn't reference one the way it does `BP_ZS_PlayerCharacter` — if the level/project is just running the raw native `AZSGameMode` class (no Blueprint subclass), there's nowhere to set an `EditDefaultsOnly` property at all yet. If no `BP_ZS_GameMode` exists: Content Browser → Blueprint Class → parent `ZSGameMode` → create it → assign `StressTestZombieClass` on its Class Defaults → set it as the level's GameMode Override (World Settings) or the project's default GameMode, then assign the zombie class.

**Not built at all — a full `.umap`, out of scope for a code-only pass:**
- `Lvl_ZS_StressTest` (T12.1) — **doesn't block testing the command itself.** `ZS.SpawnZombies <n>` works in whatever level you're already testing in once `StressTestZombieClass` is assigned above — building the dedicated graybox map is a separate, purely-cosmetic later task.

---

## 1.5 — Automated coverage (Unreal Automation Tests), added 2026-07-28

`Source/ZombieShooter/Tests/ZSAutomationTests.cpp` + `ZSTestHarnessActor.h/.cpp` — a first batch of headless automation tests covering the pure state/math parts of the checklist below, so they don't need to be manually re-verified in PIE every time. Calls the same `Server_`-prefixed functions real gameplay uses (no simulated input, no viewport, no MCP - that path is confirmed unreliable, see `CLAUDE.md`). Run via:

```powershell
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -ExecCmds="Automation RunTests ZS.; Quit" -unattended -nopause -nullrhi
```

Results land in the log named by `-log=` (default `Saved/Logs/ZombieShooter.log` if omitted, search for `Test Completed`) - the process's own exit code is non-zero whenever any test fails, by design (that's the framework signaling failure, not a launch error). **Always pass an explicit `-log=some_name.log`** - running against the default log path while the GUI editor is also open causes the run to silently fail before it even loads the project (confirmed 2026-07-28); an isolated log name sidesteps it cleanly and works whether or not the editor is open.

**Current status: 9/10 passing.** Second batch added 2026-07-28.

| Test | Covers | Result |
|---|---|---|
| `ZS.Needs.SeverityTierBoundaries` | T4.9 tier-threshold math | ✅ Pass |
| `ZS.Inventory.ItemInstanceWeightRollup` | `FZSItemInstance::GetTotalWeight()` incl. nested `ContainedItems` - direct coverage of the 2026-07-27 UHT recursion fix actually computing right, not just compiling | ✅ Pass |
| `ZS.Loot.ConditionQualityBands` | T2.10 - 200 rolls per rarity tier, all land inside the authored band | ✅ Pass |
| `ZS.Weapons.DurabilityPersistence` | Checkpoint B's mechanism (seed from instance, consume hits, breaks at exactly 0) - not the full hotbar/equip UI flow, that still needs PIE | ✅ Pass |
| `ZS.Health.WoundZonesAndInfection` | T5.1's bite-zone fix (hits land on the named zone, not always Torso) + T6's infection roll - calls `Server_ApplyDamage` directly, not the real capsule-trace bite path | ✅ Pass - **this is the same mechanism your 2026-07-27 zombie-bite test exercised**; it passing here confirms the zone/infection code itself is correct, which narrows your "zombie attack only once" finding toward range or the BT graph, not a hidden bug in `Server_ApplyDamage` |
| `ZS.Weapons.JamChanceBounds` | T10.1 - jam rate at pristine vs. worst condition, both direction and rough bound checked against `Lerp(MaxJamChance, BaseJamChance, ConditionQuality)` | ✅ Pass |
| `ZS.Combat.DownedZombieState` | T10.4 - `Server_EnterDownedState`/`Server_ExitDownedState` entry/exit on a real spawned zombie. Not covered: the automatic recovery-after-`DownedRecoverySeconds` timer - needs a latent test or PIE | ✅ Pass |
| `ZS.Health.AmputationStateTransition` | T7 - `Server_AmputateZone`'s state transition (zone marked amputated, wound cleared, bite infection cleared when the zone was the infection source, rejects re-amputating/Torso). Not covered: `AZSPlayerCharacter::Server_AmputateZone`'s outer timed choreography (`bIsBusy` → 3s wait → `EnterBlackout`) - a real RPC wrapper around this, needs a latent test or PIE | ✅ Pass (after a fix - see below) |
| `ZS.Loadout.SecondaryHandBlocksTwoHanded` | Checkpoint E - a `TwoHanded` primary (equipped via the existing public `EquipWeapon()`, not the timed hotbar flow) blocks `Server_EquipToSecondaryHand` | ✅ Pass |
| `ZS.Inventory.BagStoreAndRetrieve` | Checkpoint C's mechanics (store/retrieve/GUID-preservation, plus the new reject-nested-loaded-bag guard) - not the 2-client replication half, that still needs PIE | ❌ **Fails on a real finding**: `DA_Bag.uasset`'s `bIsEquippable` is `false`. Not a test bug - Checkpoint C can't work at all (in PIE either) until this is set. Open `DA_Bag`, set `bIsEquippable = true` and a real `EquipSlot`, and this test should go green along with unblocking manual testing. |

**Bug caught and fixed while building the second batch**: `AmputationStateTransition` initially failed 50/50 attempts to roll a 40%-chance bite infection - reproducible, not bad luck (~1-in-20-billion if truly random). Root cause: forgot the same `DispatchBeginPlay()` call `WoundZonesAndInfection` already needed - without it `BodyZones` never seeds, so the whole wound-application block (including the infection roll) silently never runs. Same class of bug as the first batch's, now present in both places it applies.

**Not yet covered, candidates for a further batch**: everything time-based (blackout's 3s choreography, downed-zombie auto-recovery, fracture/infection duration completion) - all need a real "latent" automation test (waits across frames) or PIE, a different pattern than anything built so far. Also not attempted: offhand-weapon-fire (it's an intentional gap, nothing to test), Shove/Mount-Climb (undesigned).

**Still fundamentally needs PIE, no way around it**: 2-client replication, camera/aim feel (PT2), combat feel (PT5), anything visual (flashlight placement, animations), the full player-character hotbar/equip integration (as opposed to the mechanism alone), and anything requiring the real `BT_Zombie` graph's actual branching behavior.

---

## 2. Test checklist, in dependency order

Work through in this order — later items build on earlier ones, so a failure early on may explain a failure downstream.

### 2.0 — Before you start: how to observe/trigger state (no UI exists yet)

**Watching replicated state live.** While PIE is running, click any actor in the World Outliner (your pawn, a zombie, `AZSGameState`) — the Details panel shows its properties updating live. Use the component dropdown at the top of Details to jump straight to `Zs Needs Component`/`Zs Health Component`/`Zs Inventory Component`/etc. on your pawn. Nested structs and arrays expand too — a `CarrySlots` entry's own `ContainedItems` (a bag's contents) is browsable right there in the tree, no console command required.

Because this project deliberately marks replicated gameplay state `VisibleAnywhere` rather than `EditAnywhere` (`ZSGameState.h`'s own comment: "so nobody can hand-edit server-authoritative state through it"), **you can watch these values but not hand-edit them** — there's no shortcut of typing a new number into `CurrentHealth` or `bIsWet` in the Details panel. State only changes through the real mechanism (get shot, get wet, get bitten) or a `Server_`-prefixed function actually being invoked. If a property you need isn't visible in Details for some reason, the universal fallback is a temporary Blueprint "Print String" wired to the matching `GetX()` accessor.

**Cross-client checks in 2-player PIE.** Only the host's window is a real editor instance with an Outliner. To check a *second* client's pawn/inventory, select *their* pawn from the **host's** Outliner (the server/host sees every replicated actor in the session, including remote clients' pawns) rather than looking for editor UI in the client's own window.

**Console commands that exist today** (open the console with the default `` ` `` key; all four are host-only and only ever act on the local/host player):
- `ZS.DebugDropFirstItem` — drops 1 unit of the first item in the host's `CarrySlots`.
- `ZS.DebugStoreFirstItemInBag` — moves the first non-bag `CarrySlots` item into the first bag-type `CarrySlots` item.
- `ZS.DebugListCarrySlots` — logs the host's full `CarrySlots` to the Output Log, including nested bag contents and each instance's weight.
- `ZS.SpawnZombies <n>` — spawns `<n>` (1-500) zombies in a ring around the host's pawn.

**No console command exists yet for**: forcing `bIsWet`/indoors, or forcing a specific wound. `UZSNeedsComponent::Server_SetWet(bool)`/`Server_SetIndoors(bool)` are real, callable functions, but today the only way to invoke them is a temporary Blueprint node (open `BP_ZS_PlayerCharacter`, add a debug key event calling `NeedsComponent → Server Set Wet`) — there's no `ZS.Debug` console wrapper for these, unlike the inventory ones above. *(Say the word if you'd like matching `ZS.DebugSetWet`/`ZS.DebugSetIndoors` commands added — same pattern as the four above, quick to add.)*

**Compressing time for multi-hour/multi-day tests.** At the default `RealSecondsPerGameDay = 1440`, 1 game-hour = 1 real minute. That makes some tests directly waitable (Wet's 2h dry-out = 2 real minutes) but others impractical to sit through (an unsplinted Fracture's 240h recovery = 4 real hours). `RealSecondsPerGameDay` is `EditDefaultsOnly` on `AZSGameState` — **not** editable per-instance or live during a running PIE session, and `AZSGameMode` currently hardcodes the native `AZSGameState` class directly (no Blueprint subclass exists to hold an overridden default). For anything measured in many real hours:
- **Preferred**: verify the *rate* over a short window instead of waiting for full completion — e.g. confirm `FractureRecoveryProgressGameHours` climbs by roughly 10 over 10 real minutes, rather than waiting the full 240 minutes for it to finish.
- **If you want to actually watch one complete**: temporarily lower the default in `ZSGameState.h` (~line 139) to something like `60.f` (1 real second = 1 game hour), rebuild (header-only default change, fast incremental build), test, then revert and rebuild again before moving on.

**Multiplayer tests**: PIE → Advanced Settings → Multiplayer Options → Number of Players ≥ 2, Net Mode "Play As Listen Server" (same setup Checkpoint A already used).

**Profiling**: `stat fps`, `stat unit`, `stat ai` are standard engine console commands, needed for 2.10.

### 2.1 — Item-instance system (T2, prior session — still unverified)

**Checkpoint B — durability persistence (the headline test).** ⏸ **Deferred by dev, 2026-07-27** — couldn't select the spawned weapon actor in the World Outliner to watch `CurrentDurability` (a live PIE session's Outliner gets crowded with spawned actors — zombies, projectiles, the weapon itself — and it doesn't have an obviously distinct name to search for). Revisit once real inventory UI exists, or try the better method below in the meantime. Prerequisite: a weapon config with `MaxDurabilityHits > 0` (check the `DA_ZS_WeaponConfig_*` asset — durability tracking is a no-op at the default 0/unbreakable).
1. Loot the weapon (world pickup or container "loot all").
2. Press its hotbar number (1-9), wait for `EquipTimeSeconds` to elapse, confirm it's now `CurrentWeapon`.
3. Melee-swing it until you've landed roughly half of `MaxDurabilityHits`. **Better method than hunting the Outliner**: either (a) click directly on the weapon mesh attached to your character in the viewport — this selects the actor and syncs Details to it, faster than scrolling/searching a crowded PIE Outliner list — or (b) add a temporary Blueprint "Print String" node calling `AZSWeapon::GetCurrentDurability()` on a spare debug key, which sidesteps actor-selection entirely.
4. Switch to a different hotbar slot, then switch back. **Pass**: `CurrentDurability` still reads the same half-value — not reset to `MaxDurabilityHits`.
5. Keep swinging until `CurrentDurability` hits 0. **Pass**: the weapon actor is destroyed, its instance is entirely gone from `CarrySlots` (`ZS.DebugListCarrySlots`), and the hotbar slot that held it is now empty.

**Checkpoint C — bag capacity & nested contents.** ⏸ **Not yet tested, dev's own call, 2026-07-27** — waiting for more inventory/UI setup before this one. Prerequisite: a bag-type item config (`bIsEquippable = true`, `EquipSlot = Back`/`Hip`, `CarryCapacityBonus > 0`) and one other plain item.
> **Note from this session's compile fix**: `ContainedItems` had to change type to fix a real build error (a struct can't contain an array of itself) — this caps bag nesting at one level, matching the "Tier 2 nesting isn't scoped" note that was already in the code. `Server_StoreInBag` now explicitly rejects storing an item whose own `ContainedItems` is non-empty (returns `false`) rather than silently truncating it. Worth a quick check that this returns false cleanly, alongside the normal flow below.
1. Note `GetMaxCarryWeight()` before equipping the bag.
2. Loot the bag. There's no bound input for gear-slot (Back/Hip) equipping yet either (same gap as store/retrieve below) — call `Server_EquipToSlot` via a temporary Blueprint node. **Pass**: `GetMaxCarryWeight()` rises by the bag's `CarryCapacityBonus`.
3. Loot a second, non-bag item so you have 2+ items in `CarrySlots`.
4. Run `ZS.DebugStoreFirstItemInBag`. Confirm via `ZS.DebugListCarrySlots`: the item now prints indented under the bag ("contains: ...") and no longer appears as its own top-level line.
5. Drop the bag, walk up to the dropped `AZSWorldItemActor`, pick it back up. **Pass**: the item is still nested inside on re-pickup.
6. **2-client check**: with a second player connected, inspect the bag from the **host's** Outliner after it replicates to them — confirm `ContainedItems` arrived correctly on the non-host side too.
7. Loot the *same* rare-tier item twice. **Pass**: `InstanceState.ConditionQuality` differs between the two instances (rolled per-instance via `AZSGameState::ConditionQualityBands`).

**Checkpoint D — ammo as a real item.** ✅ **Solo pass confirmed, 2026-07-27** — pickup and reload both work. **2-player leg not yet tested** (deferred, dev's own call — "too much right now").
1. ~~Once assigned: get some of that ammo item into `CarrySlots`.~~ Done — ammo items exist and are assigned.
2. ~~Equip the matching weapon, fire it empty.~~ Done.
3. ~~Press Reload (`R`). **Pass**: the ammo stack's `StackCount` drops by the correct amount and the magazine refills.~~ **Confirmed.**
4. **Still open**: drop some ammo, have a second player pick it up, confirm they can reload the same weapon type from it.

**Checkpoint E — TwoHanded blocks SecondaryHand.**
1. Equip a `TwoHanded` weapon as primary. Try to equip anything into SecondaryHand (see 2.9 for the exact flow). **Pass**: rejected, `SecondaryHandInstanceId` stays empty.
2. Switch to a `OneHanded` weapon or bare fists, retry. **Pass**: SecondaryHand equip now succeeds.

### 2.2 — Wound model (T5)
🔍 **In progress, 2026-07-27 — two open findings, not yet resolved:**
- **"Body zones not set in editor."** Checked: `UZSHealthComponent::BeginPlay` *does* seed all 4 zones (Head/Torso/Arms/Legs) into `BodyZones`, gated on `HasAuthority()` — this only runs on an actual spawned instance, never on a Blueprint's Class Defaults/CDO (which never calls `BeginPlay`). If you were looking at `BP_ZS_PlayerCharacter`'s Class Defaults rather than your pawn selected live in the World Outliner *during* PIE, that'd explain seeing it empty — the array simply doesn't exist until an instance actually spawns and runs `BeginPlay`. Worth double-checking which one you had selected before assuming it's a bug.
- **"Zombie attack only once."** Checked `AZombieCharacter::Server_MeleeAttack`: the cooldown (`Now - LastAttackTime < AttackInterval`, 1.5s default) is a per-attack gate, not a one-shot lock — nothing in the C++ prevents repeat attacks. **2026-07-28 update**: added `ZS.Health.WoundZonesAndInfection` (§1.5) to directly exercise `Server_ApplyDamage` — it passes clean, confirming the damage/zone/infection pipeline itself has no hidden bug that would stop a second hit from registering. That narrows this down to two remaining explanations, need more detail to pin down which: (a) the zombie lost `MeleeRange` (150 units) after the first hit and never got back in range before you stopped testing, or (b) **if you hit it back at all**, this session's new downed-state system (`DownedKnockbackThreshold`, 150) pauses the zombie's *entire* behavior tree the instant it triggers — that would look exactly like "stopped attacking on its own" if you landed a knockback hit without realizing that's what it did. Can't diagnose further without the `BT_Zombie` graph itself (no editor/MCP access this session) — if you can say whether the zombie kept chasing/closing distance after the first bite, or just froze/wandered off, that'd narrow it down.

1. **Zone variance.** Get bitten from a few different relative angles (front, side, while facing away). Watch `HealthComponent`'s `BodyZones` (Details, or `GetZoneWound(Zone)`) — the zone that takes the hit (`Head`/`Torso`/`Arms`/`Legs`) should vary across attempts, not always land on `Torso` (the bug T5.1 fixed).
2. **Critical head bleed.** Take repeated Head-zone hits until a bleed starts there — `CriticalHeadBleedChance` is only 8% per fresh Head bleed, so expect several tries. Watch `bCriticalBleed` on the Head entry; once set, `CurrentHealth` should drain noticeably faster (4/s vs. the normal per-type rate). Bandage it (`Server_ApplyBandage`) — **pass** if both `bCriticalBleed` and `bBleeding` clear.
3. **Fracture recovery.** Take a hit causing a Legs Fracture. Watch `FractureRecoveryProgressGameHours` on that zone — per 2.0's time-compression note, verify the *rate* (~1/real-minute) rather than waiting out the full 240h. Splint it (`Server_Splint`) and confirm `bSplinted = true` (the shorter 96h total is inferred from the tunable, not worth waiting to observe directly). Take a fresh Fracture hit on the same zone mid-recovery — **pass** if progress resets to 0.

### 2.3 — Two-tier infection (T6)
1. **Wound infection.** Get a dirty wound (`bClean = false`, the default for any fresh unbandaged wound) and leave it. Watch `WoundInfectionState` on that zone past `WoundInfectionOnsetGameHours` (24h — use the rate-check approach from 2.0 rather than a literal 24-minute wait). **Pass**: it flips `None → Infected`, and bleed/fracture-recovery rate visibly worsens (`WoundInfectionBleedMultiplier`/`WoundInfectionFractureRecoverySlowMultiplier`).
2. **Clearing it.** `Server_Disinfect` or a clean `Server_ApplyBandage` on that zone. **Pass**: `WoundInfectionState` clears to `None` immediately, and `InfectionStage` (the separate *bite* infection on the same component) is untouched either way.
3. **Bite infection variable duration.** Get bitten, let the hidden `BiteInfectionChance` roll succeed (40% — may take a few bites), watch `InfectionStage` progress `Incubating → Queasy → Fever → Critical`. **Pass**: total time-to-death varies within 48-96h across multiple separate infections, not a fixed 72h each time — this is a statistical check across several playthroughs, not a single-session pass/fail. Apply a bandage/disinfectant with nonzero `MedicalIncubationDelayGameHours` (needs content — every existing item is 0/no-effect today) and confirm `InfectionStageProgressGameHours` steps backward by that amount.

### 2.4 — Amputation & blackout (T7)
1. Progress a bite infection to wherever amputation becomes available (no dedicated prompt exists yet — likely a direct `Server_AmputateZone` call in the meantime) on the infected zone. **Pass**: a busy/timed window occurs (`bIsBusy` true for `AmputationDurationSeconds`, no montage since none's authored), then `InfectionStage` clears to `None`, `bAmputated` is set, and the zone's multiplier is permanently `AmputatedZoneMultiplier` (0.25) regardless of any other wound state.
2. **Pass**: immediately after, `bIsBlackedOut` is true (watch `OnBlackoutChanged` or select yourself in Outliner) — movement is disabled, `CanAttack()`/`CanFire()` both false, but you remain damageable/targetable (not the death path).
3. **Solo**: watch `AZSGameState::GetTimeOfDayHours()`/`GetDayCount()` jump forward by `BlackoutTimeSkipGameHours` (12h) the instant blackout begins — check the value immediately before and after, it's a discrete jump not a wait. Then wait out `BlackoutDurationSeconds` (60 real seconds, real-time — no compression involved). **Pass**: `bIsBlackedOut` clears on its own.
4. **Co-op**: black out one player; a teammate walking up finds `ReviveInteractable` active (only while blacked out). Interact (`F`). **Pass**: blackout ends immediately, not waiting the remaining timer.
5. Amputate an Arm specifically. **Pass**: equipping a `TwoHanded` weapon afterward is rejected.

### 2.5 — Needs simulation (T4) — this is **PT3** from `B0_Stabilization.md`
1. Call `Server_SetWet(true)` (temporary Blueprint node — see 2.0). **Pass**: `bIsWet` replicates, and — directly waitable at 2 real minutes — auto-clears after `WetDryOutGameHours`.
2. While wet and walking (not sprinting — sprint reports its own noise separately), confirm a noise event fires every `WetFootstepNoiseIntervalSeconds` (0.6s real-time). **This specifically needs a nearby zombie to be meaningful** — it's a comparative test (does a zombie react to the wet player at a distance a dry player wouldn't trigger a reaction from), not just "does the function run."
3. Toggle wet/indoors, equip something with `InsulationValue` (needs content, Section 1), watch `Temperature` move toward the new target. Push past `HypothermiaThreshold`/`HyperthermiaThreshold` (25/75) and confirm `GetPerformanceMultiplier()`/`GetTemperaturePerformanceMultiplier()` drops below 1.0 near those edges.
4. **T4.7 spot-check**: with Hunger/Thirst/Fatigue/Temperature all at their best values simultaneously, confirm `GetPerformanceMultiplier()` reads exactly 1.0, never higher.
5. Load past `OverloadWeightRatio` (weight/max > 1.5) and sprint. **Pass**: Stamina drains faster (toward the 2x `MaxEncumbranceStaminaDrainMultiplier` ceiling) but sprint is only blocked once Stamina hits 0 — never blocked by encumbrance directly.
6. Walk Hunger/Thirst/Fatigue/Stamina/Temperature each through all 4 severity tiers (`GetXSeverityTier()`) — confirm the tier changes at the expected thresholds (75/50/25).
7. Get a zombie to notice you (`Server_NotifyHostileDetection` fires from its perception), immediately try `RequestSleep`. **Pass**: blocked until `HostileDetectionCooldownSeconds` elapses since the last detection. **Known, expected gap**: once that cooldown clears, sleep succeeds anywhere, indoors or not — the "real shelter" half of `IsSafeToSleep()` is stubbed `true` until B4. Not a bug.

### 2.6 — Camera & aiming (T3) — this is **PT2**
1. Confirm there's no way to leave TopDown at all — nothing to even call `ToggleCameraPerspective` on anymore.
2. Once `IA_Zoom` exists (Section 1): scroll wheel and `=`/`-` zoom smoothly between `MinCameraDistance`/`MaxCameraDistance` (600-1400), not a snap.
3. Press `1`-`9` — direct hotbar select still works; confirm the scroll wheel does *nothing* to the hotbar anymore (100% reassigned to zoom).
4. Pick one stationary target (another player, not a zombie — see below) at a fixed distance. Fire a large sample (20-30 shots) hip-fired, then the same aimed. **Pass**: the aimed group's spread is visibly tighter, and its headshot rate is noticeably higher (~25% aimed vs. ~5% hip-fired) — a handful of shots won't show a clean split, you need real sample size. This only shows up against a **player** target (check `HealthComponent`'s hit zone) — zombies have no zone model to weight yet.
5. Full **PT2 pass**: 20+ real minutes, at least one interior space, 3+ zombie fights at range and melee, loot a container, sanity-check character readability at both zoom extremes. Anything that feels off is a tuning note (adjust `TuningReference.md`), not a sign of a bug — there's no fallback camera to revert to.

### 2.7 — Death, loot & zombie conversion (T9)
Prerequisite: `AZSPlayerCharacter::DeathZombieClass` assigned (Section 1) — without it, death still works, you just won't see the zombie-conversion half.
1. Load up: something hotbarred, something in Back/Hip, a bag with a nested item. Note the list (`ZS.DebugListCarrySlots`) before dying.
2. Die to zombie damage. **Pass**: every item — hotbarred/equipped/nested-in-bag included — drops as its own `AZSWorldItemActor` at the death location, preserving durability/condition (check your half-durability weapon from Checkpoint B specifically).
3. **Pass**: a new `DeathZombieClass` zombie spawns at the same spot.
4. Repeat via a bite-infection death (let `InfectionStage` run out Critical) instead of direct damage. **Pass**: identical drop/zombie-spawn behavior.
5. Respawn. **Pass**: default `StartingHotbarLoadout`, not the gear you just dropped — identical in solo and co-op.

### 2.8 — Combat revision (T10) — this is **PT5**
1. Fire the same weapon repeatedly (empty, reload, repeat) until it jams — a pristine weapon only jams ~1%/shot (`BaseJamChance`), so this takes a while; a heavily-degraded one (low `CurrentConditionQuality`, e.g. your Checkpoint B weapon) approaches `MaxJamChance` (30%) and jams much sooner. **Pass**: once `IsJammed()`, firing does nothing and consumes no ammo. Once `IA_Rack` exists (Section 1): `Alt+R`, wait `ClearJamTimeSeconds`, confirm firing resumes.
2. Melee repeatedly, bare-fisted then weapon-equipped — watch Stamina drop by `UnarmedStaminaCost`/`MeleeStaminaCost` per swing regardless of whether it connects. **Pass**: swings still execute at 0 Stamina (no hard block — intentional, see Decisions Log).
3. Land a hit clearing `DownedKnockbackThreshold` (150) on a zombie. **Pass**: `IsDowned()` flips true, it stops moving/attacking entirely (its whole BT is paused as a stand-in — see Decisions Log), and it recovers on its own after `DownedRecoverySeconds` (6s) if left alone.
4. While a zombie is downed, land a normal standing melee swing on it anyway. **Pass**: it doesn't register at all — downed zombies are unconditionally excluded from the standing-melee target filter.
5. Once `IA_Finisher` exists (Section 1): within `FinisherRange` (200) of a downed zombie, press Space. **Pass**: guaranteed kill either bare-handed (stomp) or weapon-equipped (strike) — the montage is cosmetic/no-op until authored, but the kill itself (`FinisherDamage` = 9999) should land regardless.
6. Full **PT5 pass**: loot → hotbar → equip → degrade near-broken → jam → clear jam → break entirely; separately, melee to 0 Stamina; separately, knock down and finish a zombie both bare-handed and weapon-equipped.

### 2.9 — SecondaryHand & flashlight (T11)
1. With a `TwoHanded` weapon as primary, try equipping anything into SecondaryHand. **Pass**: rejected (same check as Checkpoint E in 2.1).
2. Switch to `OneHanded`/bare fists. Equip a `bIsToggleable` item (needs `DA_ZS_ItemConfig_Flashlight`, Section 1) into SecondaryHand. Once `IA_SecondaryAction` exists (Section 1): press `T`. **Pass**: `IsSecondaryItemActive()` flips and a real spotlight visibly turns on/off (rough chest-height placement for now, not a real hand socket — don't be alarmed it's not final-looking).
3. ⚑ **Updated 2026-07-28**: Equip a one-handed *weapon* into SecondaryHand instead (needs `bUsableInSecondaryHand`, e.g. an offhand pistol config). **Pass**: the slot accepts it, and `SecondaryWeapon` spawns/attaches (check via the World Outliner or `GetSecondaryWeapon()`). Press `T` on a `Ranged` config: **pass** if it fires (ammo drops, jam/headshot rolls apply, same as the primary hand). Press `T` on a `Melee` config: **pass** if it swings and can break the weapon (`Server_ConsumeDurabilityHit`), clearing the slot on break. Note the two deliberate scope cuts while testing: no auto-fire (each `T` press is one shot/swing only), and the primary and secondary hands share one attack cooldown (`LastAttackTime`) - firing primary then immediately trying secondary should be gated the same as firing primary twice in a row. Also 2-client-check the visual attach - both weapons currently render at the same socket (`SocketGunAttachment`, no dedicated offhand socket authored), so overlapping meshes are an expected content gap, not a bug.

### 2.10 — Stress-test spawning (T12.1)
Prerequisite: `AZSGameMode::StressTestZombieClass` assigned (Section 1).
1. In any existing level (`Lvl_ZS_StressTest` doesn't exist yet, Section 1), run `ZS.SpawnZombies 25`. **Pass**: exactly 25 zombies appear scattered in a ring around your pawn.
2. Repeat at 50, 100, 150, 250, checking `stat fps`/`stat unit`/`stat ai` after each batch settles (give the AI a few seconds to start ticking before reading numbers). Record the numbers somewhere — this run *is* T12.2-T12.5's profiling/triage work, there's no separate code step left for it.

---

## 3. Decisions & content gaps log

Everything below is either a judgment call made without you (documented, reversible) or a genuine open question. None of it blocks compiling or the checklist above — flagging for your review, not blocking on it.

### 3.1 — Judgment calls worth a second look

| Area | What I decided | Why | Where |
|---|---|---|---|
| Melee stamina (T10.3) | No hard Stamina gate on melee — swings still execute at 0 Stamina, cost is just a resource drain with no block | "No separate strain mechanic" read as "the drain itself is the whole mechanism," matching this project's soft-penalty-everywhere-else philosophy. Could instead be read as wanting a real block/penalty at 0 Stamina. | `ZSPlayerCharacter.cpp` `Server_MeleeAttack_Implementation`/`Server_WeaponMeleeAttack_Implementation` |
| Downed zombie state (T10.4) | `AZombieAIController::SetDowned` pauses/resumes the **entire** behavior tree (`BrainComponent::PauseLogic`) rather than a real BT-graph branch | No `unreal-mcp`/editor access this session to add a `bIsDowned` branch to `BT_Zombie`'s graph. This is a blunt stand-in — it works (a downed zombie genuinely stops), but a real BT branch could look/feel better (e.g. a prone idle pose instead of a frozen mid-stride pose) once you have editor time. | `ZombieAIController.cpp` `SetDowned` |
| Player-becomes-zombie (T9.2) | "Holding on to loot and clothing" implemented as co-locating the dropped loot pile and the spawned zombie at the same point, **not** giving the zombie a literal inventory | A real carry-inventory system on `AZombieCharacter` would be a much bigger addition, and `CLAUDE.md`'s own Zombies/ note is explicit that zombies deliberately don't share players' inventory/health machinery. Confirm this satisfies what you actually pictured — if you wanted the zombie visibly *wearing*/*holding* specific items, that's a different, bigger feature. | `ZSPlayerCharacter.cpp` `Server_HandleDeathLootAndZombie` |
| Headshot weighting (T3.6) | Implemented by overriding `Hit.BoneName` to `"head"` on a successful chance-roll, rather than deriving it from real geometry | Minimal-diff way to feed the existing `BodyZoneFromBoneName` inference without restructuring the damage-event pipeline. Functionally correct (same outcome a real geometric headshot would produce) but worth knowing it's a probability roll layered on top of the physical trace, not a "did the ray actually hit the head" check. | `ZSPlayerCharacter.cpp` `Server_Fire_Implementation`, `ZSProjectile.cpp` `HandleHit` |
| Needs severity tiers (T4.9) | `Wet` has no 4-tier severity (it's binary); Injury/Pain and Infection/Sickness use their own existing concepts on `UZSHealthComponent` rather than the shared 0-100/4-tier scale | The doc's "8 needs, 4 tiers each" reads most naturally this way once you look at what each need actually *is*, but it's an interpretation of a slightly ambiguous requirement, not an explicit prior decision. | `B0_Stabilization.md` T4.9 row |

### 3.2 — Deliberately not built (documented gaps, not oversights)

- ~~**Offhand weapon firing (T11.2)**~~ **Closed 2026-07-28** (away session) — see §2.9 item 3 above and `Docs/Beta/B0_Stabilization.md` T11.4's row. Built, not yet compiled or PIE-tested.
- **Shove and Mount/Climb (T10.6)** — bundled onto the same `Space` input as the finisher per `Docs/InputBindings.md`, but genuinely undesigned anywhere in the plan. Only the finisher (stomp/weapon-strike over a downed zombie) is implemented.
- **Racking beyond jam-clearing (T10.1)** — `Docs/InputBindings.md`'s name ("Rack Firearm") implies a manual-chamber action that might also matter after certain reloads, not just a jam-clear button. Only the confirmed jam-clear case is built; whether racking needs a role elsewhere is still open.
- **Real shelter check for sleep (T4.10)** — the aggro-cooldown half of `IsSafeToSleep()` is real; the "must be in a barricaded room/behind a locked door/in a vehicle" half is stubbed `true` since no indoor-detection system exists (B4's job).
- **BT_Zombie graph branch for the downed state (T10.4)** — see 3.1 above.
- **`BT_Zombie`'s `ClearLastKnownLocation` wiring (T8.2)** — pre-existing open item from before this stretch, genuinely ambiguous (may overlap with the investigation timer's own expiry-driven clear), not touched.

### 3.3 — Content that must be authored before certain features work at all

Consolidated from Section 1 above, for reference:
- Ammo items + `AmmoItemConfig` assignments (reload doesn't work without this — pre-existing gap)
- Real weapon `Weight` values (pre-existing gap)
- 4 new Input Actions (`IA_Zoom`/`IA_Rack`/`IA_Finisher`/`IA_SecondaryAction`)
- `DA_ZS_ItemConfig_Flashlight`
- `AZSPlayerCharacter::DeathZombieClass` and `AZSGameMode::StressTestZombieClass` assignments
- `Lvl_ZS_StressTest` map (T12.1's own deliverable, not built)
- Delete orphaned `IA_ToggleView.uasset`

### 3.4 — Everything committed and pushed this stretch

Nine commits on `main`, each independently revertable if a regression needs bisecting: `B0-T4` (needs simulation), `B0-T3` (camera/aiming), `B0-T9` (death/loot), `B0-T10 remainder` (jamming/stamina/downed/finisher), `B0-T11` (SecondaryHand/flashlight), `B0-T12.1` (stress-test command) — plus, from the portion of this push before the context reset, `B0-T2 Steps B-E`, `B0-T5`, `B0-T6`, `B0-T7`. `git log --oneline` from `b0-baseline` forward shows the full sequence.
