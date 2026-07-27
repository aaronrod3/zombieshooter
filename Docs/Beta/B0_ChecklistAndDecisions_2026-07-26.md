# B0 Autonomous Push — Test Checklist & Decisions Log (2026-07-26)

> **What this is.** You stepped away and asked for as much of B0 as possible to get done without you, plus a list of tests to work through on your return and a list of decisions that need your call. This is that list. Full technical detail for every item here already lives in `Docs/Beta/B0_Stabilization.md` (every sub-task row was updated as it was built) — this doc is the curated, sequential "start here" companion, not a replacement.
>
> **Nothing in this entire stretch has been compiled or run.** Every feature below is C++ that type-checks by my own reading, following every established pattern in the codebase, but the editor was never opened this session (per standing policy — I don't attempt `Build.bat` while you might have the editor open, and there was no way to run PIE headlessly either way). Section 0 is not optional.

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
- `IA_Finisher` — digital, `Space` (T10.6)
- `IA_SecondaryAction` — digital, `T` (T11.2)

**Orphaned/dead content to clean up:**
- `Content/ZS/Input/IA_ToggleView.uasset` — the C++ reference was deleted (T3.9, TopDown-only now). Delete the asset itself, or it'll just sit unused.

**Data Assets to author or extend:**
- `DA_ZS_ItemConfig_Ammo_<Caliber>` instances + assign `AmmoItemConfig` on each weapon config (T2.11 — **pre-existing gap from the prior session**, still open; every weapon's `CanReload()` returns false until this exists)
- Weapon `Weight` fields — still the inherited placeholder 0.5 on every `DA_ZS_WeaponConfig_*` (T2.4 — **pre-existing gap**)
- `DA_ZS_ItemConfig_Flashlight` — a real item with `bIsToggleable = true` (T11.4)
- Clothing items with a real `InsulationValue` (T4.4) — optional, defaults to 0 (no warmth effect) without it
- `DA_ZS_WeaponConfig_Pistol`'s own `HipFireSpreadDegrees`/`AimedSpreadDegrees` (8°→2° per OQ-B0-02) — currently inherits the rifle-shaped default (5°→1°) (T3.5)

**Blueprint class references to assign** (both are `TSubclassOf` fields that are `nullptr` by default — the features they gate simply no-op with a log warning until assigned):
- `AZSPlayerCharacter::DeathZombieClass` — which zombie BP a dead player becomes (T9.2)
- `AZSGameMode::StressTestZombieClass` — which zombie BP `ZS.SpawnZombies` spawns (T12.1)

**Not built at all — a full `.umap`, out of scope for a code-only pass:**
- `Lvl_ZS_StressTest` (T12.1) — the console command works in any existing level in the meantime.

---

## 2. Test checklist, in dependency order

Work through in this order — later items build on earlier ones, so a failure early on may explain a failure downstream.

### 2.1 — Item-instance system (T2, prior session — still unverified)
These checkpoints are already written up in detail in `B0_Stabilization.md`; only Checkpoint A has a confirmed pass. Referencing, not repeating, the instructions:
- [ ] **Checkpoint B** (`B0_Stabilization.md` line ~156) — the headline durability-persistence test: loot a weapon, hotbar it, equip, use to half durability, unequip/re-equip, confirm durability held. Break a weapon, confirm it's gone from `CarrySlots` entirely.
- [ ] **Checkpoint C** (~line 165) — equip a bag, confirm capacity rises; store/retrieve an item in it via `Server_StoreInBag`/`Server_RetrieveFromBag` (no UI yet — call via Blueprint or a temporary console hook); drop the bag, confirm contents travel with it; loot the same rare item twice, confirm `ConditionQuality` varies. Also confirms nested `ContainedItems` replication reaches a second client.
- [ ] **Checkpoint D** (~line 173) — needs `AmmoItemConfig` authored first (Section 1 above). Fire empty, reload from a carried ammo stack, confirm the count drops correctly.
- [ ] **Checkpoint E** (~line 181) — confirm a `TwoHanded` weapon config genuinely blocks `SecondaryHand` (now built — see 2.6 below).

### 2.2 — Wound model (T5)
- [ ] Get bitten by a zombie from various angles/positions — confirm the wound zone (Head/Arms/Legs/Torso, visible via `HealthComponent`'s state) actually varies instead of always landing on Torso (the bug T5.1 fixed).
- [ ] Take repeated Head-zone hits until a bleed starts; over enough hits, confirm you occasionally see a **critical head bleed** (steep bleed rate) — `CriticalHeadBleedChance` is only 8% per fresh Head bleed, so this may take several tries. Bandage it, confirm it clears.
- [ ] Take a Legs Fracture wound, confirm `FractureRecoveryProgressGameHours` ticks up over in-game time (compress the clock for testing) and the wound heals on its own eventually — unsplinted should take ~240h, splinted ~96h. A fresh fracture hit should reset progress.

### 2.3 — Two-tier infection (T6)
- [ ] Get a dirty wound, leave it untreated past `WoundInfectionOnsetGameHours` (24h) — confirm it escalates to wound-`Infected` (a distinct state from bite infection) and bleed/fracture-recovery both worsen.
- [ ] Disinfect or clean-bandage a wound-infected zone — confirm it clears immediately, and confirm this does **not** touch bite infection state (`InfectionStage`) at all.
- [ ] Get bitten, let bite infection run — confirm the total time-to-death now varies run-to-run (48-96h range), not a fixed 72h. Apply a bandage/disinfectant with `MedicalIncubationDelayGameHours` set (needs a content-authored value — currently 0/no-effect on every existing item) and confirm it pushes the clock back.

### 2.4 — Amputation & blackout (T7)
- [ ] Let a bite infection reach the point where amputation is offered; amputate — confirm a busy/timed action occurs (no montage authored yet, so it's timer-only), infection clears, and the zone gets a permanent penalty.
- [ ] Confirm amputation triggers **blackout**: movement disabled, can't attack/fire/switch loadout, but still damageable/targetable (not the same as death).
- [ ] **Solo**: confirm the world clock jumps forward ~12 game-hours on blackout entry, and you auto-recover after ~60 real seconds.
- [ ] **Co-op**: have a teammate interact with the blacked-out player (a new interactable, only active while blacked out) — confirm it ends blackout immediately instead of waiting out the timer.
- [ ] Amputate an Arm, confirm you can no longer equip a `TwoHanded` weapon.

### 2.5 — Needs simulation (T4) — this is **PT3** from `B0_Stabilization.md`
- [ ] `Server_SetWet(true)` (debug entry point, no real weather source yet) — confirm `bIsWet` replicates, and confirm it auto-clears after `WetDryOutGameHours` (2h default).
- [ ] While wet and walking (not sprinting), confirm a noise event fires periodically (`WetFootstepNoiseRadius`/`Interval`) — **PT4's own note**: verify zombies actually respond differently to wet vs. dry footsteps at the same distance, since dry footsteps report nothing at all.
- [ ] Toggle wet/indoors, equip clothing with `InsulationValue` (needs content authored first), confirm `Temperature` moves toward the expected target and `GetPerformanceMultiplier()` degrades near the hypothermia/hyperthermia thresholds (25/75).
- [ ] Confirm Hunger/Thirst/Fatigue/Temperature can never push the performance multiplier **above** 1.0 no matter how sated/comfortable (T4.7 — should hold by construction, worth a spot-check anyway).
- [ ] Sprint while overloaded (past `OverloadWeightRatio`) — confirm Stamina drains faster (up to `MaxEncumbranceStaminaDrainMultiplier`, 2x ceiling) but sprint is **never hard-blocked** by encumbrance, only by Stamina reaching 0.
- [ ] Run Hunger/Thirst/Fatigue/Stamina/Temperature each through every severity tier (`GetXSeverityTier()`) and confirm sensible transitions.
- [ ] Get chased by a zombie (triggers `Server_NotifyHostileDetection`), try to sleep — confirm it's blocked during the aggro-cooldown window. **Known gap**: the "real shelter" half of the sleep gate is stubbed `true` (no barricade/indoor system exists) — sleep will currently succeed anywhere once the aggro-cooldown clears, not just indoors. That's expected, not a bug to chase.

### 2.6 — Camera & aiming (T3) — this is **PT2**
- [ ] Confirm the character now **only** has the TopDown camera — no `ToggleCameraPerspective` input exists anymore, no ThirdPerson fallback.
- [ ] Once `IA_Zoom` exists (Section 1): scroll wheel and `=`/`-` should zoom the TopDown camera smoothly between `MinCameraDistance`/`MaxCameraDistance` (600-1400 default).
- [ ] Confirm `1`-`9` hotbar keys still work directly — scroll-wheel hotbar cycling is gone entirely (OQ-B0-01), don't expect the mouse wheel to touch the hotbar at all anymore.
- [ ] Fire hip-fired vs. aimed at a stationary target from a fixed distance many times — confirm the spread cone is visibly tighter while aiming, and confirm headshots (Head-zone hits, per `HealthComponent`'s wound state) show up more often while aimed (~25%) than hip-fired (~5%). This only matters against **another player** right now — zombies have no zone model to weight yet, so headshot chance has no visible effect on zombie combat until that changes.
- [ ] Full **PT2 pass**: 20+ minutes, interior navigation, 3+ zombie fights at range and melee, loot a container, check character state readable at both zoom extremes. If cone width/zoom/headshot split feel wrong, that's a retune, not a revert — there's no ThirdPerson fallback anymore.

### 2.7 — Death, loot & zombie conversion (T9)
- [ ] Once `DeathZombieClass` is assigned (Section 1): die with a full inventory (including a bag with nested items, and whatever's hotbarred/equipped in Back/Hip) — confirm every carried item drops as its own world item at the death location, preserving durability/condition (check a partially-durability weapon specifically), and confirm a new zombie spawns at the same spot.
- [ ] Confirm this triggers identically for a normal combat death **and** a bite-infection death (T6.4's timeline).
- [ ] Respawn — confirm you get a genuinely fresh character (default starting loadout, not the gear you just dropped), and confirm this is identical in solo and co-op.

### 2.8 — Combat revision (T10) — this is **PT5**
- [ ] Fire a weapon repeatedly until it jams (chance scales with condition — a fresh/pristine weapon jams rarely, ~1%/shot). Confirm the trigger does nothing once jammed (no ammo consumed). Once `IA_Rack` exists (Section 1): rack the firearm, confirm the jam clears and firing resumes.
- [ ] Melee-swing repeatedly (bare-fisted, then with a weapon) — confirm Stamina drains per swing whether it lands or not, and confirm swings **still work at 0 Stamina** (no hard block — a deliberate interpretation, see Decisions Log).
- [ ] Land a strong knockback hit on a zombie (`DownedKnockbackThreshold`, 150 default) — confirm it enters a **downed** state: stops moving/attacking (the BT itself is paused as a stand-in — see Decisions Log), stays down for `DownedRecoverySeconds` (6s default) unless finished, then gets back up on its own.
- [ ] Confirm a normal standing melee swing **never** hits a downed zombie, even if it's in range/arc.
- [ ] Once `IA_Finisher` exists (Section 1): stand over a downed zombie, press Space — bare-handed should stomp; with a melee weapon equipped, it should use the weapon's own downward-strike montage instead (once authored — currently no-op cosmetically, but the kill itself should land either way). Confirm it's a guaranteed kill.
- [ ] Full **PT5 pass**: loot → hotbar → equip → degrade → jam → clear jam → break, melee to exhaustion, knock down + finish both bare-handed and weapon-equipped.

### 2.9 — SecondaryHand & flashlight (T11)
- [ ] Try to equip a one-handed weapon or a toggleable item into SecondaryHand while a `TwoHanded` weapon is in the primary hand — confirm it's rejected.
- [ ] Equip a `bIsToggleable` item (needs `DA_ZS_ItemConfig_Flashlight`, Section 1) into SecondaryHand. Once `IA_SecondaryAction` exists: press `T`, confirm a real spotlight visibly turns on/off on the character (rough chest-height placement for now, not a real socket).
- [ ] Confirm equipping a one-handed **weapon** (e.g. an offhand pistol config, if one exists) into SecondaryHand succeeds validation-wise, but pressing `T` over it does **nothing** — this is a known, documented gap (see Decisions Log), not a bug.

### 2.10 — Stress-test spawning (T12.1)
- [ ] Once `StressTestZombieClass` is assigned (Section 1): run `ZS.SpawnZombies 25` (then 50/100/150/250) from the host console in any level — confirm the right count spawns scattered around the player, and confirm perf triage (`stat unit`/`stat fps`/`stat ai`) is possible against them. This is the actual T12.2-T12.5 profiling work (packaged build, triage sequence, baseline capture) — none of that is code, it's a session of running the game and recording numbers.

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

- **Offhand weapon firing (T11.2)** — SecondaryHand's slot/validation logic is complete for both weapons and toggleable items, but only the toggleable-item dispatch (flashlight) actually does anything when you press `T`. An offhand weapon (e.g. a second pistol) would need its own spawned `AZSWeapon` actor and ammo/equip choreography mirroring the primary hand's — `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §6 itself calls this "genuinely new surface... scope it as its own small task," so I left it as a clean, documented boundary rather than a half-built parallel system.
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
