# Tuning Reference

> A map of every gameplay-feel tunable in the project and exactly where to change it — not a design doc, just "where do I go to make X feel different." Update this whenever a new system introduces a numeric tunable worth exposing here. Values shown are current defaults as of 2026-07-14 (Phase 2, M8 in progress) — check the actual asset/class for the live value before relying on a number here.

## Camera (`AZSPlayerCharacter`, Category `ZS|Camera`)
Set on `BP_ZS_PlayerCharacter`'s CDO or a C++ default in `ZSPlayerCharacter.h`.

| Property | Default | Effect |
|---|---|---|
| `DefaultFOV` | 100° | FirstPerson FOV, not aiming |
| `AimedFOV` | 78° | FirstPerson FOV while aiming (ADS) |
| `ThirdPersonFOV` | 105° | ThirdPerson perspective FOV |
| `GunCameraFOV` | 120° | GunCamera perspective FOV |
| `BodycamFOV` | 120° | Bodycam perspective FOV |
| `FOVInterpSpeed` | 10 | How fast FOV/third-person arm-length transitions interpolate (`UpdateAimFOV`/`UpdateThirdPersonCameraTick`) |
| `bUseChestCameraForBodycam` | false | Bodycam socket choice: chest vs. helmet (`SocketChestCamera`/`SocketHelmetCamera`, per-weapon-config) |
| `InitialCameraDistance` | 140 | ThirdPerson spring-arm resting length |
| `MinCameraDistance`/`MaxCameraDistance` | 65 / 270 | TP camera zoom bounds — **not yet wired to an input action**, no zoom control exists yet |
| `CameraZoomStep` | 50 | Same — unused until zoom input exists |

## Procedural Offsets / Springs (`AZSPlayerCharacter`, Category `ZS|Procedural Offsets`)
Each is an `FZSSpringConfig` (`Stiffness`/`CriticalDampingFactor`/`Mass` — parameter names match `UKismetMathLibrary::VectorSpringInterp`/`QuaternionSpringInterp` directly, no translation). Higher `Stiffness` = snappier/faster settle; higher `CriticalDampingFactor` = less overshoot/bounce; `Mass` scales inertia.

| Spring config | Default (S/D/M) | Drives |
|---|---|---|
| `CrouchSpringConfig` | 400 / 30 / 1 | Smooths `CurrentCrouchOffset` toward `WeaponConfig.OffsetCrouch` |
| `AimDownSightsSpringConfig` | 400 / 30 / 1 | Smooths `CurrentAimDownSightsOffset` toward `WeaponConfig.OffsetAimDownSights` |
| `RecoilSpringConfig` | 400 / 30 / 1 | Smooths `CurrentRecoil` toward `TargetRecoil` (re-kicked every shot via `AddRecoil`, decays to identity each frame) |

`MaxRecoilDecayDeltaTime` (default `0.016`) caps the delta-time used for recoil spring integration, preventing a huge frame-hitch from producing an exaggerated recoil snap.

## Movement (`AZSPlayerCharacter`)
- `SprintSpeedMultiplier` (Category `ZS|Movement`, default `1.6`) — sprint speed = `BaseWalkSpeed * SprintSpeedMultiplier`.
- Base walk speed, jump velocity, air control, braking deceleration, etc. are **standard `UCharacterMovementComponent` properties** (`MaxWalkSpeed`, `JumpZVelocity`, `AirControl`, `BrakingDecelerationWalking`, `BrakingDecelerationFalling`, `MinAnalogWalkSpeed`) — this project sets initial values for these in the `AZSPlayerCharacter` constructor, but they're editable per-instance via `BP_ZS_PlayerCharacter`'s `CharacterMovement` component defaults in the editor like any other Character project.
- `GetCharacterMovement()->bOrientRotationToMovement` is **deliberately `false`** (see `CoreLoopPlan.md`'s M8 findings) — do not re-enable without re-checking the FP camera-lock consequences documented there.

## Per-Weapon Config (`UZSWeaponConfig` — e.g. `DA_ZS_WeaponConfig_AssaultRifle`)
The gameplay-feel-relevant numeric fields (meshes/montages/sockets are content references, not tuning, and are omitted here). **Every field here is per-weapon** — a new weapon gets its own `DA_ZS_WeaponConfig_<Name>` instance with its own values, never a C++ branch (see `CLAUDE.md`'s multi-weapon rule).

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
| `RecoilRecoverySpeed` | 22 | How fast `RecoilSpringConfig` pulls `CurrentRecoil` back toward identity between shots |
| `OffsetAimDownSights` | loc(-2,-6,2.05) rot(8.4°,0,0) | ADS sight-alignment nudge on `ik_hand_gun` — small and precise by design, not a big pose change |
| `OffsetCrouch` | loc(1.5,-2,-1.5) rot(-4.3°,0,0) | Crouch weapon-position nudge on `ik_hand_gun` — deliberately subtle; the *big* visible crouch motion is the capsule's own crouch system, not this |
| `TotalAmmoCount` | 0 | **Cosmetic only** — starting fill for the magazine's visual bullet count, not the real ammo source of truth (`MagazineCapacity`/`StartingReserveAmmo` are) |

## AnimGraph Node Settings (`ABP_ZS_FirstPerson`, edited directly in the AnimGraph — not a Blueprint variable)
These live on individual node instances, not a data asset — open the AnimGraph, select the node, edit in the Details panel.

| Node | Setting | Value | Effect |
|---|---|---|---|
| `Fabrik_0`/`Fabrik_1` (both arms) | `precision` | 0.01 | FABRIK solve tolerance — lower = more accurate but more iterations |
| `Fabrik_0`/`Fabrik_1` | `maxIterations` | 10 | FABRIK solver iteration cap |
| `LayeredBoneBlend` (camera/head toggle) | branch filter bone | `head`, depth 0 | Which bone (and everything below it) the reference-pose override affects |
| `ModifyBone` ×3 (Recoil/ADS/Crouch) | target bone | `ik_hand_gun` | The single IK-target bone all three procedural offsets nudge — FABRIK then re-solves the arms to follow it |

## Not yet built / no tunables exist yet
- TP locomotion (body-facing-camera-vs-movement-direction behavior) — Phase 6, undecided.
- Crouch's actual arm *pose* (`FP_Transition_CrouchStart`/`End` blend spaces, stance-based locomotion swap) — not wired into the AnimGraph yet, so no tuning surface exists for it.
- `ABP_ZS_ThirdPerson`'s own AnimGraph (Guide 07: stance blend, `SM_AimingTransitions`, breathing/idle additive, montage slots, hand IK) — not built yet as of this file's last update.
