# Tuning Reference

> A map of every gameplay-feel tunable in the project and exactly where to change it — not a design doc, just "where do I go to make X feel different." Update this whenever a new system introduces a numeric tunable worth exposing here. Values shown are current defaults as of 2026-07-20 — check the actual asset/class for the live value before relying on a number here.

## Camera (`AZSPlayerCharacter`, Category `ZS|Camera`)
Set on `BP_ZS_PlayerCharacter`'s CDO or a C++ default in `ZSPlayerCharacter.h`. FirstPerson/GunCamera/Bodycam perspectives and their FOV/spring-offset tunables were removed in the P0 de-scope — this section now covers ThirdPerson only, pending P1's TopDown addition.

| Property | Default | Effect |
|---|---|---|
| `ThirdPersonFOV` | 105° | ThirdPerson perspective FOV |
| `FOVInterpSpeed` | 10 | How fast FOV/third-person arm-length transitions interpolate |
| `InitialCameraDistance` | 140 | ThirdPerson spring-arm resting length |
| `MinCameraDistance`/`MaxCameraDistance` | 65 / 270 | TP camera zoom bounds — **not yet wired to an input action**, no zoom control exists yet |
| `CameraZoomStep` | 50 | Same — unused until zoom input exists |

## Movement (`AZSPlayerCharacter`)
- `SprintSpeedMultiplier` (Category `ZS|Movement`, default `1.6`) — sprint speed = `BaseWalkSpeed * SprintSpeedMultiplier`.
- Base walk speed, jump velocity, air control, braking deceleration, etc. are **standard `UCharacterMovementComponent` properties** (`MaxWalkSpeed`, `JumpZVelocity`, `AirControl`, `BrakingDecelerationWalking`, `BrakingDecelerationFalling`, `MinAnalogWalkSpeed`) — set as constructor defaults in `AZSPlayerCharacter`, editable per-instance via `BP_ZS_PlayerCharacter`'s `CharacterMovement` component defaults.
- `GetCharacterMovement()->bOrientRotationToMovement` is **`true`** (restored in the P0 de-scope, now that FP's camera-lock constraint is gone) — P1's cursor-aim will override facing only while actively aiming/attacking/interacting.

## Per-Weapon Config (`UZSWeaponConfig` — e.g. `DA_ZS_WeaponConfig_AssaultRifle`)
The gameplay-feel-relevant numeric fields (meshes/montages/sockets are content references, not tuning, and are omitted here). **Every field here is per-weapon** — a new weapon gets its own `DA_ZS_WeaponConfig_<Name>` instance with its own values, never a C++ branch (see `CLAUDE.md`'s multi-weapon rule). Config was slimmed from ~90 to ~22 fields in the P0 de-scope (cosmetic/FP-only fields removed).

| Field | AR default | Effect |
|---|---|---|
| `MagazineCapacity` | 30 | Rounds per magazine |
| `StartingReserveAmmo` | 90 | Reserve ammo at spawn |
| `MaxReserveAmmo` | 180 | Reserve ammo cap |
| `SupportedFireModes` | `[Semi, Auto]` | Which fire modes `CycleFireMode` cycles through |
| `RoundsPerMinute` | 600 | Fire rate |
| `RecoilPitchRange` | (0.5, 1.5) | Per-shot recoil pitch kick, randomized within this range |
| `RecoilYawRange` | (-0.5, 0.5) | Per-shot recoil yaw kick, randomized within this range |
| `RecoilRampMinShots`/`RecoilRampMaxShots` | 5 / 25 | Consecutive-shot range over which recoil ramps from min to max intensity |
| `RecoilRecoverySpeed` | 22 | How fast recoil pulls back toward identity between shots |
| `OffsetCrouch` | loc(1.5,-2,-1.5) rot(-4.3°,0,0) | Crouch weapon-position nudge on `ik_hand_gun` |
| `TotalAmmoCount` | 0 | **Cosmetic only** — starting fill for the magazine's visual bullet count, not the real ammo source of truth |

## TopDown Camera (`AZSPlayerCharacter`, Category `ZS|Camera|TopDown`)
| Property | Default | Effect |
|---|---|---|
| `TopDownCameraPitch` | -70° | Boom pitch while in TopDown (steeper than a classic ~45° isometric) |
| `TopDownCameraDistance` | 900 | Boom length while in TopDown |
| `TopDownMinCameraDistance`/`TopDownMaxCameraDistance` | 600 / 1400 | TopDown zoom bounds — not yet wired to an input action |
| `TopDownFixedYaw` | captured once per `EnableTopDownPerspective()` call | Not player-rotatable — the Q/E yaw-rotation feature was built then removed 2026-07-20 at dev request |

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
| `HungerPerformanceCurve`/`ThirstPerformanceCurve`/`FatiguePerformanceCurve` | unset (= no penalty) | `UCurveFloat` assets, need value (0-100) → performance multiplier (0-1); multiplied together into `GetPerformanceMultiplier()`. Not authored yet — content task, not a code task. |
| `SeverityTier2Max`/`SeverityTier3Max`/`SeverityTier4Max` | 75 / 50 / 25 | 4-tier moodle severity thresholds shared across Hunger/Thirst/Fatigue |

## World Clock (`AZSGameState`, Category `ZS|WorldClock`)
| Property | Default | Effect |
|---|---|---|
| `RealSecondsPerGameDay` | 1440 (24 real min/game day) | Time compression — lower = faster |
| `MinUtilitiesShutoffDay`/`MaxUtilitiesShutoffDay` | 8 / 12 | Randomized-once-per-session range for the utilities-shutoff day |

## Not yet built / no tunables exist yet
- Stage A locomotion state machine and its blend-space feeds — in progress, crouch pose bug open (see `SessionHandoff.md`).
- `UZSItemConfig` (`HungerRestore`/`ThirstRestore`) — per-consumable, no `DA_ZS_ItemConfig_*` instances authored yet.
- Zombie config (`UZSZombieConfig`) speed/health/senses/damage tunables — P4, not started.
