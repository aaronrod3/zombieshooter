# Tuning Reference

> A map of every gameplay-feel tunable in the project and exactly where to change it — not a design doc, just "where do I go to make X feel different." Update this whenever a new system introduces a numeric tunable worth exposing here. Values shown are current defaults as of 2026-07-19 (post-pivot, TP-only) — check the actual asset/class for the live value before relying on a number here.

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

## AnimGraph (`ABP_ZS_ThirdPerson`, on Infima's `SKEL_TFA_Mannequin`)
No tunables documented yet — Stage A locomotion (Idle/Move state machine, crouch layer, aim layer) is not built as of this file's last update. This section will fill in as Stage A lands; use Infima's own animation set as the source, not the broken Lyra-sourced import (see `CLAUDE.md`).

## Not yet built / no tunables exist yet
- TopDown camera pitch/zoom/yaw-step tunables — P1, not yet implemented.
- Stage A locomotion state machine and its blend-space feeds — in progress.
- Needs/moodle rate curves (`UZSNeedsComponent`) — P2, not started.
- Zombie config (`UZSZombieConfig`) speed/health/senses/damage tunables — P4, not started.
