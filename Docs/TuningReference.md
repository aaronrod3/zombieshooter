# Tuning Reference

> A map of every gameplay-feel tunable in the project and exactly where to change it — not a design doc, just "where do I go to make X feel different." Update this whenever a new system introduces a numeric tunable worth exposing here. Values shown are current defaults as of 2026-07-20 — check the actual asset/class for the live value before relying on a number here.

## Camera (`AZSPlayerCharacter`, Category `ZS|Camera`)
🔧 **B0-T3.9, 2026-07-26: TopDown only now** — `EZSCameraPerspective`, `ToggleCameraPerspective`, `IA_ToggleView`, and the whole ThirdPerson/OverShoulder camera path were deleted (dev-confirmed permanent, no fallback wanted). `Content/ZS/Input/IA_ToggleView.uasset` is now orphaned — needs manual deletion in-editor (no MCP access this session to do it headlessly).

| Property | Default | Effect |
|---|---|---|
| `CameraFOV` | 105° | TopDown camera FOV (renamed from `ThirdPersonFOV` — TopDown is the only perspective now) |
| `TopDownCameraPitch` | -70° | Boom pitch (negative = looking down) |

Zoom distance/bounds moved off the character entirely — see **Camera Director** below.

## Camera Director (`UZSCameraDirector`, on `AZSPlayerCharacter`, Category `ZS|Camera|Presets`/`ZS|Camera|Bounds`) — B0-T3.1-T3.3, 2026-07-26
🔧 **Code complete, not yet PIE-verified.** New component owning TopDown zoom distance — a push/pop `EZSCameraContext` stack (`Outdoor`/`Interior`/`Underground`/`Driving`) for auto-zoom presets, plus a manual-override layer that immediately and fully disengages auto-zoom (no cooldown) until a context change moves the active context away from whatever it was at override time. `AZSPlayerCharacter::Tick` calls `CameraDirector->TickZoom()` explicitly (not a self-ticking component) and applies the result to `CameraBoom->TargetArmLength`.

| Property | Default | Effect |
|---|---|---|
| `OutdoorDistance` | 900 | Auto-zoom preset, `EZSCameraContext::Outdoor` (also the starting/default context) |
| `InteriorDistance` | 650 | Auto-zoom preset, `Interior` |
| `UndergroundDistance` | 550 | Auto-zoom preset, `Underground` |
| `DrivingDistance` | 1100 | Auto-zoom preset, `Driving` — reserved for the `BV` vehicle phase, nothing pushes this context yet |
| `MinCameraDistance`/`MaxCameraDistance` | 600 / 1400 | Manual-zoom clamp bounds |
| `CameraZoomStep` | 100 | Distance change per unit of `ApplyManualZoom` input |
| `ZoomInterpSpeed` | 6 | How fast the live arm length interpolates toward its target (auto or manual) |

**Content gap**: nothing calls `PushContext`/`PopContext` yet — no indoor/underground-detection system exists (B4's job, see `UZSElevationSubsystem` below). The mechanism is real and testable via `ApplyManualZoom` (mouse wheel / `=`/`-`) today; auto-zoom context switching needs B4's trigger volumes to actually fire.

## Movement (`AZSPlayerCharacter`)
- `SprintSpeedMultiplier` (Category `ZS|Movement`, default `1.6`) — sprint speed = `BaseWalkSpeed * SprintSpeedMultiplier`.
- Base walk speed, jump velocity, air control, braking deceleration, etc. are **standard `UCharacterMovementComponent` properties** (`MaxWalkSpeed`, `JumpZVelocity`, `AirControl`, `BrakingDecelerationWalking`, `BrakingDecelerationFalling`, `MinAnalogWalkSpeed`) — set as constructor defaults in `AZSPlayerCharacter`, editable per-instance via `BP_ZS_PlayerCharacter`'s `CharacterMovement` component defaults.
- `GetCharacterMovement()->bOrientRotationToMovement` is **`true`** (restored in the P0 de-scope, now that FP's camera-lock constraint is gone) — P1's cursor-aim will override facing only while actively aiming/attacking/interacting.
- `SprintNoiseRadius` (default `1200`) — one noise event on sprint start (`Server_StartSprint`), not per-tick.
- `WetFootstepNoiseRadius`/`WetFootstepNoiseIntervalSeconds` (default `600` / `0.6s`) — B0-T4.2, 2026-07-26: while `NeedsComponent->IsWet()` and actually moving (not sprinting — sprint's own report already covers that case), reports a noise event on a walking cadence via `TickWetFootstepNoise`. Dry footsteps report nothing at all (no footstep-audio-cue system exists to key off of), so this is what makes a wet player "audibly distinct," per the sub-task's definition of done.

## Per-Weapon Config (`UZSWeaponConfig` — e.g. `DA_ZS_WeaponConfig_AssaultRifle`)
The gameplay-feel-relevant numeric fields (meshes/montages/sockets are content references, not tuning, and are omitted here). **Every field here is per-weapon** — a new weapon gets its own `DA_ZS_WeaponConfig_<Name>` instance with its own values, never a C++ branch (see `CLAUDE.md`'s multi-weapon rule). Config was slimmed from ~90 to ~22 fields in the P0 de-scope (cosmetic/FP-only fields removed).

| Field | AR default | Effect |
|---|---|---|
| `MagazineCapacity` | 30 | Rounds per magazine |
| `AmmoItemConfig` | unset | B0-T2.11, 2026-07-26: which `UZSItemConfig` this weapon's magazine reloads from - reserve ammo is a real, lootable, stackable inventory item now, not a flat counter (`StartingReserveAmmo`/`MaxReserveAmmo` removed entirely). Unset means the weapon can never reload. Needs a real `DA_ZS_ItemConfig_Ammo_<Caliber>` instance authored and assigned per weapon - content task, not done yet for AR/Pistol. |
| `SupportedFireModes` | `[Semi, Auto]` | Which fire modes `CycleFireMode` cycles through |
| `RoundsPerMinute` | 600 | Fire rate |
| `RecoilPitchRange` | (0.5, 1.5) | Per-shot recoil pitch kick, randomized within this range |
| `RecoilYawRange` | (-0.5, 0.5) | Per-shot recoil yaw kick, randomized within this range |
| `RecoilRampMinShots`/`RecoilRampMaxShots` | 5 / 25 | Consecutive-shot range over which recoil ramps from min to max intensity |
| `RecoilRecoverySpeed` | 22 | How fast recoil pulls back toward identity between shots |
| `OffsetCrouch` | loc(1.5,-2,-1.5) rot(-4.3°,0,0) | Crouch weapon-position nudge on `ik_hand_gun` |
| `TotalAmmoCount` | 0 | **Cosmetic only** — starting fill for the magazine's visual bullet count, not the real ammo source of truth |
| `FireNoiseRadius` | 3000 | P4: how far a shot's noise event reaches (`UZSNoiseSystem::ReportNoise`, called from `Server_Fire`) |
| `FireDamage` | 25 | P4: hitscan damage per shot, applied via `ApplyPointDamage` |
| `FireRange` | 5000 | P4: hitscan trace distance from `SocketMuzzle` (falls back to eye height if the socket's missing) |
| `FireDamageTypeClass` | unset (→ `UZSDamageType_Laceration`) | Which `EZSWoundType` a gunshot applies to a player target |
| `ProjectileClass` | unset (AR/Pistol: `AZSProjectile`) | P5, 2026-07-26: when set, `Server_Fire` spawns a real traveling `AZSProjectile` from `SocketMuzzle` instead of resolving an instant hitscan trace — opt-in per weapon, unset keeps the old hitscan path |
| `ProjectileMesh` | unset (AR/Pistol: engine placeholder `Sphere`) | Cosmetic mesh on the spawned projectile — needs a real bullet mesh per weapon before this is presentable |
| `ProjectileSpeed` | 6000 | Projectile travel speed (`UProjectileMovementComponent::InitialSpeed`/`MaxSpeed`) |
| `HipFireSpreadDegrees`/`AimedSpreadDegrees` | 5° / 1° | B0-T3.5, 2026-07-26: cone half-angle `Server_Fire` randomizes the fire direction within (`FMath::VRandCone`) — these are the rifle numbers from OQ-B0-02's dev-approved starting values; **content gap**: `DA_ZS_WeaponConfig_Pistol` still needs its own authored override (8°→2°) |
| `HipFireHeadshotChance`/`AimedHeadshotChance` | 0.05 / 0.25 | B0-T3.6: 0-1 chance a landed hit is upgraded to the Head zone regardless of which bone the cone ray physically struck (`Hit.BoneName` override, read by `AZSPlayerCharacter::BodyZoneFromBoneName`) — OQ-B0-02's "~5% hip-fire / ~25% aimed" starting values. Only affects a target with a `UZSHealthComponent` (players) — zombies have no zone model to weight (`CLAUDE.md`'s Zombies/ note) |
| `MeleeStaminaCost` | 10 | B0-T10.3, 2026-07-26: `NeedsComponent->Server_ConsumeStamina` cost per weapon-melee swing (whether it lands or not) — meaningful only when `AttackType == Melee` |
| `FinisherMontage` | unset | B0-T10.6: cosmetic downward swing/strike montage used by the Space finisher when this (melee) weapon is equipped over a downed zombie, instead of the bare-handed stomp |
| `bJamImmune` | false | B0-T10.1: true for weapons that never jam (revolvers/bolt-actions) — ranged only |
| `BaseJamChance`/`MaxJamChance` | 0.01 / 0.3 | Per-shot jam chance, interpolated by `AZSWeapon::CurrentConditionQuality` (1 = pristine → `BaseJamChance`, 0 = worst → `MaxJamChance`). Rolled in `Server_Fire` before ammo is consumed — a jam replaces the shot outright |
| `TP_ClearJam`/`ClearJamTimeSeconds` | unset / 1.5s | Cosmetic "Rack Firearm" (Alt+R) clear-jam montage + busy duration, same choreography pattern as `TP_Reload`/`EquipTimeSeconds` |
| `AttackType` | `Ranged` | P5: which half of `IA_Attack`'s dispatch this weapon uses (`ZSWeaponTypes.h`'s `EZSAttackType`) — `Ranged` routes to `Server_Fire`, `Melee` currently falls back to the bare-fist stats below (no melee-specific weapon fields exist yet) |
| `EquipTimeSeconds` | 0.75s | P5: how long switching the hotbar to this weapon takes (`Server_SelectHotbarSlot` → `CompleteHotbarSwitch`) — `SetBusy(true)` for the duration, same choreography pattern as reload |
| `MeleeDamage`/`MeleeRange`/`MeleeAttackInterval` | 35 / 180 / 0.9s | P5, 2026-07-21: real per-weapon melee stats, used when `AttackType == Melee` — mirrors the `Unarmed*` fields below one-for-one |
| `MeleeDamageTypeClass` | unset (→ `UZSDamageType_Laceration`) | Which `EZSWoundType` a weapon-melee hit applies to a player target |
| `MeleeMontage` | unset | Cosmetic TP swing montage for weapon melee — no-op until authored |
| `MaxDurabilityHits` | 0 (unbreakable) | P5: how many landed melee hits this weapon survives before breaking (`AZSWeapon::CurrentDurability`/`Server_ConsumeDurabilityHit`) — breaking auto-unequips **and** clears the weapon from its own hotbar slot; see `Docs/Beta/B0_Stabilization.md` B0-T2.8 for the item-instance-refactor version of this rule |
| `MeleeKnockbackStrength` | 0 | P5: `LaunchCharacter` impulse strength on a landed weapon-melee hit against an `ACharacter` target — physical-only, not a real stagger/AI state |
| `FireKnockbackStrength` | 120 | Same, for a landed hitscan shot (`Server_Fire`) |

## Loadout Hotbar (`AZSPlayerCharacter`, Category `ZS|Loadout`)
P5, 2026-07-21: player starts unarmed — nothing auto-equips at `BeginPlay` anymore. `StartingHotbarLoadout` (an `EditDefaultsOnly` array on the character/BP, same placeholder-content-reference spot `StartingWeaponConfig` used to occupy pre-P5) seeds a fixed 9-slot `HotbarSlots` array at `BeginPlay`. `SelectHotbarSlot`/`CycleHotbar` (bound to `HotbarSelectAction`/`HotbarCycleAction`, both **not yet created** as `.uasset`s) both route through `Server_SelectHotbarSlot`, which schedules `CompleteHotbarSwitch` after a delay and sets `bIsBusy` for its duration — a real weapon switch isn't instant. Re-selecting the already-equipped slot unequips back to bare-fist.

| Property | Default | Effect |
|---|---|---|
| `UnequipTimeSeconds` | 0.4s | Switch delay when the destination is bare-fist (no `UZSWeaponConfig` to read `EquipTimeSeconds` from) |
| `NumHotbarSlots` | 9 (`static constexpr`, not editable) | Fixed hotbar size — every number key 1-9 is always a valid target, empty or not |

## Player Unarmed Melee (`AZSPlayerCharacter`, Category `ZS|Combat|Melee`)
Bound to `IA_Attack` — `IA_Fire` is no longer separately bound (P5, 2026-07-21: both on the same key was double-triggering fire+melee). `HandleAttack` dispatches on `CurrentWeapon`'s `AttackType`: `Ranged` fires; no weapon or `Melee` uses these bare-fist stats. This is the "unarmed" fallback the loadout hotbar above always falls back to whenever nothing's equipped — named `Unarmed*` for that reason, not a temporary name.

| Property | Default | Effect |
|---|---|---|
| `UnarmedMeleeDamage` | 20 | Damage applied to the nearest valid target in range |
| `UnarmedMeleeRange` | 150 | Sphere-overlap radius + max target distance |
| `UnarmedMeleeAttackInterval` | 1s | Cooldown between swings |
| `UnarmedMeleeDamageTypeClass` | unset (→ `UZSDamageType_Laceration`) | Which `EZSWoundType` a melee hit applies to a player target |
| `UnarmedMeleeMontage` | unset | Cosmetic TP swing montage — no-op until authored |
| `UnarmedMeleeKnockbackStrength` | 80 | P5, 2026-07-21: same `ApplyHitKnockback` weapon melee/gunfire use, given a bare-fist punch a little heft too |
| `UnarmedStaminaCost` | 8 | B0-T10.3, 2026-07-26: `NeedsComponent->Server_ConsumeStamina` cost per bare-fist swing (whether it lands or not) |

## Downed State & Finisher (`AZSPlayerCharacter`/`AZombieCharacter`/`UZSZombieConfig`) — B0-T10.4-T10.6, 2026-07-26
| Property | Default | Effect |
|---|---|---|
| `AZSPlayerCharacter::DownedKnockbackThreshold` | 150 | `ApplyHitKnockback`'s `Strength` at or above which a zombie target enters the downed state instead of just physically launching — centralized in `ApplyHitKnockback` so every damage path (hitscan, projectile, weapon melee, bare-fist) triggers it identically |
| `UZSZombieConfig::DownedRecoverySeconds` | 6 | How long a downed zombie stays down before automatically recovering (`AZombieCharacter::Server_ExitDownedState`) unless finished first — a temporary stagger, not a permanent knockdown |
| `AZSPlayerCharacter::FinisherRange` | 200 | Sphere-overlap radius `FindNearestDownedZombie` scans for a Space-finisher target |
| `AZSPlayerCharacter::FinisherDamage` | 9999 | Deliberately large enough to guarantee a kill through the normal `TakeDamage`/`ApplyPointDamage` pipeline — an execution, not a damage roll |
| `AZSPlayerCharacter::UnarmedFinisherMontage` | unset | Bare-handed stomp montage — used when no melee weapon is equipped over the downed target |

**Content gap**: `BT_Zombie`'s graph isn't wired to branch on the new `bIsDowned` Blackboard key yet (no `unreal-mcp`/editor access this session to add the branch). `AZombieAIController::SetDowned` pauses/resumes the whole behavior tree directly (`BrainComponent::PauseLogic`/`ResumeLogic`) as a functional stand-in, so "downed" (stops chasing/attacking) actually works in PIE today even without the graph edit — the Blackboard key exists ready for a nicer BT-native branch later.

## TopDown Camera (`AZSPlayerCharacter`, Category `ZS|Camera|TopDown`)
Boom length now lives on **Camera Director** above — this section is just the fixed pitch/yaw.

| Property | Default | Effect |
|---|---|---|
| `TopDownFixedYaw` | captured once per `EnableTopDownPerspective()` call | Not player-rotatable — the Q/E yaw-rotation feature was built then removed 2026-07-20 at dev request |

## Elevation (`UZSElevationSubsystem`, a `UWorldSubsystem`) — B0-T3.7, 2026-07-26
🔧 **Code complete, not yet PIE-verified.** Answers "what floor/Z-plane is this actor on?" for `AZSPlayerCharacter::GetCursorGroundLocation`'s cursor-facing ground-projection (previously hardcoded to the querying actor's own Z inline). B0 ships a single-floor stub — `GetElevationZ` always returns the querying actor's own current Z, identical behavior to before this existed. B4's real multi-level implementation replaces the stub body only; callers don't change.

## AnimGraph (`ABP_ZS_ThirdPerson`, on Infima's `SKEL_TFA_Mannequin`)
No tunables documented yet — Stage A locomotion (Idle/Move state machine, crouch layer, aim layer) is not built as of this file's last update. This section will fill in as Stage A lands; use Infima's own animation set as the source, not the broken Lyra-sourced import (see `CLAUDE.md`).

## Needs (`UZSNeedsConfig` — e.g. `DA_ZS_NeedsConfig_Default`, read by `UZSNeedsComponent`)
| Field | Default | Effect |
|---|---|---|
| `HungerDecayPerGameHour` | 2 | Hunger lost per in-game hour |
| `ThirstDecayPerGameHour` | 3 | Thirst lost per in-game hour |
| `FatigueRisePerGameHour` | 4 | Fatigue gained per in-game hour awake |
| `FatigueRecoveryPerSleptGameHour` | 12.5 | Fatigue lost per in-game hour slept |
| `StaminaDrainPerSecondSprinting` | 12 | Stamina lost per real second sprinting |
| `StaminaRegenPerSecondIdle` | 8 | Stamina regained per real second not sprinting, scaled by `GetPerformanceMultiplier()` |
| `MaxEncumbranceStaminaDrainMultiplier` | 2 | B0-T4.8, 2026-07-26: sprint stamina drain scales by `1/GetEncumbranceMultiplier()`, clamped to this ceiling — heavier load drains stamina faster, never a hard sprint block (`StartSprint`'s gate stays `Stamina > 0` only) |
| `WetDryOutGameHours` | 2 | B0-T4.1, 2026-07-26: game-hours a wet player stays wet before auto-drying, absent a real weather system re-triggering it. `UZSNeedsComponent::Server_SetWet` is the only trigger until B4 |
| `NeutralTemperature` | 50 | B0-T4.3, 2026-07-26: `Temperature`'s comfortable resting value on the 0-100 scale (0 = hypothermic, 100 = hyperthermic) |
| `TemperatureChangeRatePerGameHour` | 15 | How far `Temperature` moves toward its target per in-game hour |
| `WetTemperaturePenalty` | 20 | Subtracted from the temperature target while `bIsWet` |
| `IndoorTemperatureBonus` | 15 | Added to the temperature target while `bIsIndoors` (stub input — `Server_SetIndoors`, no real indoor-detection system exists yet) |
| `HypothermiaThreshold`/`HyperthermiaThreshold` | 25 / 75 | Temperature bounds past which `GetTemperaturePerformanceMultiplier()` starts falling below 1.0 |
| `TemperatureExtremePerformanceMultiplier` | 0.5 | B0-T4.5: performance multiplier at the extreme end (Temperature at 0 or 100) — a linear falloff from 1.0 at the threshold, not an authored curve, so hypothermia/hyperthermia are testable without new content |
| `FatiguePerceptionCurve` | unset (= no degradation) | B0-T4.6, 2026-07-26 (CR-10): `UCurveFloat`, inverted-Fatigue value (0-100) → perception multiplier (0-1), read by `GetPerceptionMultiplier()`. Presentation-only (vignette/audio) — never touches gameplay math, distinct from `GetPerformanceMultiplier()`. Not authored yet — content task |
| `HungerPerformanceCurve`/`ThirstPerformanceCurve`/`FatiguePerformanceCurve` | unset (= no penalty) | `UCurveFloat` assets, need value (0-100) → performance multiplier (0-1); multiplied together (with `GetTemperaturePerformanceMultiplier()`, B0-T4.5) into `GetPerformanceMultiplier()`. Not authored yet — content task, not a code task. Each factor is independently clamped to [0,1], so the product can never exceed 1.0 — B0-T4.7's "penalty-only" requirement holds by construction |
| `SeverityTier2Max`/`SeverityTier3Max`/`SeverityTier4Max` | 75 / 50 / 25 | B0-T4.9: 4-tier moodle severity thresholds shared across Hunger/Thirst/Fatigue/Stamina, and (via a transformed "comfort value") Temperature. `Wet` is binary (no 4-tier shape applies); Injury/Pain and Infection/Sickness get their own severity concepts on `UZSHealthComponent` (wound flags, `EZSInfectionStage`) rather than this shared scale — see `Docs/Beta/B0_Stabilization.md` T4.9 note |

## World Clock (`AZSGameState`, Category `ZS|WorldClock`)
| Property | Default | Effect |
|---|---|---|
| `RealSecondsPerGameDay` | 1440 (24 real min/game day) | Time compression — lower = faster |
| `MinUtilitiesShutoffDay`/`MaxUtilitiesShutoffDay` | 8 / 12 | Randomized-once-per-session range for the utilities-shutoff day |

## Health / Medical (`UZSHealthConfig` — e.g. `DA_ZS_HealthConfig_Default`, read by `UZSHealthComponent`)
| Field | Default | Effect |
|---|---|---|
| `MaxHealth` | 100 | Overall health pool |
| `BleedDamagePerSecond_Scratch`/`_Laceration`/`_Bite` | 0.1 / 0.4 / 0.3 | Per-second drain while that zone is bleeding (Fracture never bleeds) |
| `DirtyWoundBleedMultiplier` | 1.5 | Bleed rate multiplier while a wound is dirty (not disinfected/clean-bandaged) |
| `CriticalHeadBleedChance` | 0.08 | B0-T5.3, 2026-07-26: rare per-hit roll on a fresh bleeding Head-zone wound |
| `BleedDamagePerSecond_CriticalHead` | 4 | B0-T5.3: overrides the normal wound-type bleed rate entirely while active - deliberately steep |
| `LegLacerationMobilityMultiplier` | 0.75 | Move speed multiplier, any non-Fracture Legs wound |
| `LegFractureMobilityMultiplier`/`LegSplintedFractureMobilityMultiplier` | 0.35 / 0.7 | Move speed multiplier, Legs Fracture unsplinted/splinted |
| `FractureRecoveryDurationGameHours`/`SplintedFractureRecoveryDurationGameHours` | 240 / 96 | B0-T5.4, 2026-07-26: game-hours a Fracture takes to heal on its own, unsplinted/splinted - splint shortens by 60%, doesn't trivialize |
| `ArmWoundedAttackSpeedMultiplier`/`ArmWoundedReloadSpeedMultiplier` | 0.75 / 0.7 | Fire-rate / reload-speed multiplier, any active Arms wound |
| `AmputatedZoneMultiplier` | 0.25 | Overrides all of the above once a zone is permanently amputated |
| `BiteInfectionChance` | 0.4 | Hidden per-Bite roll (0-1) |
| `MinBiteInfectionDurationGameHours`/`MaxBiteInfectionDurationGameHours` | 48 / 96 | B0-T6.4, 2026-07-26: dev-confirmed 2-4 in-game-day range - each infection rolls its own total within this band |
| `IncubatingDurationGameHours`/`QueasyDurationGameHours`/`FeverDurationGameHours`/`CriticalDurationGameHours` | 18 / 24 / 18 / 12 | ⚑ B0-T6.4: no longer fixed durations - a **base proportional split** (sums to 72h/3 days, the range's midpoint) scaled per-infection to fit the rolled total above. Death at the end of the scaled Critical duration if not amputated first |
| `WoundInfectionOnsetGameHours` | 24 | B0-T6.1, 2026-07-26: how long a wound can stay dirty before it's marked Infected (distinct from bite infection) |
| `WoundInfectionBleedMultiplier` | 1.3 | B0-T6.1: additional bleed-rate multiplier while Infected, stacks with `DirtyWoundBleedMultiplier` |
| `WoundInfectionFractureRecoverySlowMultiplier` | 0.5 | B0-T6.1: fracture recovery accrues at this fraction of normal speed while the zone is wound-infected |

## Zombies (`UZSZombieConfig` — e.g. `DA_ZS_ZombieConfig_Shambler`, read by `AZombieCharacter`/`AZombieAIController`)
| Field | Default | Effect |
|---|---|---|
| `MaxHealth` | 100 | Zombie's flat health pool (not `UZSHealthComponent` - see `CLAUDE.md`'s Zombies/ note) |
| `MeleeDamage`/`MeleeRange`/`AttackInterval` | 15 / 150 / 1.5s | `Server_MeleeAttack`'s damage, self-validated range, and cooldown |
| `AttackDamageTypeClass` | unset (falls back to `UZSDamageType_Bite`) | Which `EZSWoundType` a hit applies to a bitten player |
| `DownedRecoverySeconds` | 6 | B0-T10.4 — see the Downed State & Finisher section below |
| `WalkSpeed`/`ChaseSpeed` | 150 / 300 | `MaxWalkSpeed` at rest / while `SetChasing(true)` (not yet called by anything - no BT exists) |
| `SightRadius`/`LoseSightRadius`/`PeripheralVisionAngleDegrees` | 1500 / 1800 / 90° | `AISenseConfig_Sight`, applied at `OnPossess` |
| `HearingRange` | 3000 | `AISenseConfig_Hearing`, applied at `OnPossess` - this is what picks up `UZSNoiseSystem::ReportNoise` events |
| `BehaviorTree` | unset | Assign `BT_Zombie` (`/Game/ZS/Enemy/AI/`) to activate - `RunBehaviorTree` no-ops until then |
| `InvestigationDurationSeconds` | 10 | How long `AZombieAIController::StartInvestigationTimer` investigates a lost target's last known location before giving up |
| `IdleDwellDurationSeconds` | 3 | How long `StartIdleDwell` pauses between wander moves |

### Stress-test spawning (`AZSGameMode::StressTestZombieClass`) — B0-T12.1, 2026-07-26
`ZS.SpawnZombies <n>` (`Zombies/ZombieCharacter.cpp`, host-only) spawns `<n>` (clamped 1-500, default 10) instances of `StressTestZombieClass` in a 2000-unit ring around the local player's pawn. Unset `StressTestZombieClass` = the command warns and no-ops. **Content gaps**: no `BP_Zombie_*` assigned yet, and `Lvl_ZS_StressTest` (the dedicated graybox map T12.1 actually calls for) doesn't exist - the command works in any level in the meantime.

## Inventory (`UZSInventoryComponent`, Category `ZS|Inventory`) — built 2026-07-21, untested
| Property | Default | Effect |
|---|---|---|
| `BaseCarryWeight` | 25 | `GetMaxCarryWeight()` before any equipped bag's `CarryCapacityBonus` |
| `OverloadWeightRatio` | 1.5 | Weight ratio (current/max) at which `GetEncumbranceMultiplier()` bottoms out at `MinEncumbranceMultiplier` |
| `MinEncumbranceMultiplier` | 0.5 | Movement-speed floor while badly overloaded — a soft penalty (folded into `AZSPlayerCharacter::UpdateMovementSpeed`), not a hard carry block |
| `DropDistance` | 100 | How far in front of the owner `Server_DropItem` spawns the `AZSWorldItemActor` |
| `Server_DropAllItems(DropLocation)` | — | B0-T9.1, 2026-07-26: not a tunable, a mechanism note — dumps every `CarrySlots` instance as its own `AZSWorldItemActor` at `DropLocation` (no `DropDistance` offset — this drops in place, not thrown), preserving each instance's identity. Called by `AZSPlayerCharacter::Server_HandleDeathLootAndZombie` on death. |

## Death (`AZSPlayerCharacter`, Category `ZS|Health`) — B0-T9, 2026-07-26
| Property | Default | Effect |
|---|---|---|
| `DeathZombieClass` | unset | B0-T9.2: which zombie Blueprint (needs a real `UZSZombieConfig` on its CDO) a dead player turns into, spawned at the death location alongside the dropped loot pile (`Server_HandleDeathLootAndZombie`). Unset = death proceeds normally, just without the zombie-conversion half — **content gap**, no `BP_Zombie_*` assigned yet. |

## Per-Item Config (`UZSItemConfig`) — P6 fields added 2026-07-21
| Field | Default | Effect |
|---|---|---|
| `Weight` | 0.5 | Per-unit weight, consumed by `UZSInventoryComponent::GetCurrentWeight()`/`FZSItemInstance::GetTotalWeight()` |
| `MaxStackSize` | 1 | How many stack per `FZSItemInstance` (B0-T2 renamed from `FZSInventorySlot`) — 1 = doesn't stack |
| `bIsEquippable`/`EquipSlot`/`CarryCapacityBonus` | false / `None` / 0 | Whether this item claims one of the two resolved equip slots (`Back`/`Hip`) and how much carry capacity it grants while worn |
| `Rarity` | `Common` | Consulted by the finite rarity-pool system (Rare/VeryRare only — see `AZSGameState` below); also bands `ConditionQuality` roll (B0-T2.10) |
| `WorldMesh` | unset | `AZSWorldItemActor`'s pickup mesh — unset is an invisible pickup, same "content not sourced yet" pattern as the zombie mesh |
| `MedicalIncubationDelayGameHours` | 0 | B0-T6.5, 2026-07-26: Bandage/Disinfectant only - applied to the bite-infection-source zone, pushes the infection clock back by this many game-hours. 0 (a basic bandage/disinfectant) = no effect; a "better" medical tier authors a real value |
| `InsulationValue` | 0 | B0-T4.4, 2026-07-26: equippable items only - sums into `UZSNeedsComponent`'s Temperature target while worn in `Back`/`Hip`. Proxy scope: no dedicated clothing-slot system exists yet, so whatever's equipped in the two general gear slots is what's summed - a real wardrobe system is bigger scope than this pass |
| `bIsToggleable` | false | B0-T11.3, 2026-07-26: whether `IA_SecondaryAction` toggles this item on/off (a flashlight) instead of dispatching an attack, when it's the item equipped in `SecondaryHand` - checked before falling through to weapon-attack dispatch |

## SecondaryHand & Flashlight (`AZSPlayerCharacter`, Category `ZS|Loadout`/`ZS|Combat`) — B0-T11, 2026-07-26
🔧 **Code complete, not yet PIE-verified.** `SecondaryHandInstanceId` (a GUID into `CarrySlots`, same reference-not-remove model as `HotbarSlots`) accepts either (a) a `UZSWeaponConfig` with `Handedness == OneHanded && bUsableInSecondaryHand` (an offhand pistol), or (b) any `UZSItemConfig` with `bIsToggleable` (a flashlight) — a `TwoHanded` primary blocks the slot entirely either way (`Server_EquipToSecondaryHand`). `IA_SecondaryAction` (`T`) dispatches via `Server_HandleSecondaryAction`: a toggleable item flips `bSecondaryItemActive` (broadcasting `OnSecondaryItemToggled`, whose default C++ implementation toggles `FlashlightComponent`'s visibility — a real `USpotLightComponent` on the character, not delegated to unauthored Blueprint content). **Content gap, by design (see `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §6's own scope note)**: an offhand *weapon* doesn't actually fire/swing yet — that would need its own spawned `AZSWeapon` actor and ammo/equip choreography mirroring `CurrentWeapon`'s, flagged as its own follow-up task, not built here. The slot/validation mechanism and the flashlight toggle are both real and testable today.

| Property | Default | Effect |
|---|---|---|
| `FlashlightComponent` (Intensity/OuterConeAngle/AttenuationRadius) | 5000 / 30° / 2000 | `USpotLightComponent` defaults — content-polish task to retune once a real flashlight mesh/socket exists. Attached at a rough chest-height offset on `GetMesh()` (no confirmed left-hand socket on the skeleton) — positioning is cosmetic polish, the light itself works today |
| `FlashlightComponent`'s shadow casting | off | A per-player dynamic shadow-casting spotlight is a real perf cost, not needed for B0 |

**Content gap**: `IA_SecondaryAction` needs manual creation in-editor (`T`, per `Docs/InputBindings.md`), and no real `DA_ZS_ItemConfig_Flashlight` instance (`bIsToggleable = true`) has been authored yet.

## Loot (`UZSLootTableConfig`, `AZSGameState`) — built 2026-07-21, untested
| Property | Default | Effect |
|---|---|---|
| `UZSLootTableConfig::NumRolls` | 3 | How many weighted rolls a container makes across its `Entries` — can repeat the same entry |
| `AZSGameState::RarityPoolEntries` | empty | Per-session budget for Rare/VeryRare items — unlisted items are ungated. See `Docs/GameDevPlan.md` §7 P6 for the resolved "global per-session, not per-zone" model |

## Not yet built / no tunables exist yet
- Stage A locomotion state machine and its blend-space feeds — in progress, crouch pose bug open (see `SessionHandoff.md`).
- `UZSItemConfig` (`HungerRestore`/`ThirstRestore`/`bIsCleanBandage`) — per-item, no `DA_ZS_ItemConfig_*` instances authored yet.
- `UZSHealthConfig`/`UZSZombieConfig` content instances — fields above are code defaults, not authored/tuned data assets yet.
