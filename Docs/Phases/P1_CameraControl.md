# Phase P1 — Camera & Control Prototype (Identity Test #1)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: see `Docs/SessionHandoff.md` for current verification status.**

This is the go/no-go gate on Decision 1 (camera) — settle this before any art money is spent.

## Stage A locomotion/animation (GameDevPlan.md §5.1 — not originally itemized here, added 2026-07-20)
- [x] Skeleton/retarget hub decision: `SKEL_TFA_Mannequin` (Infima's skeleton). Lyra locomotion blend spaces retargeted onto it, curated into `Content/ZSAnims/`.
- [x] Locomotion architecture: 2 blend spaces (standing, crouched) + Layered Blend Per Bone compositing Infima's rifle idle/aim poses over them. No AO layer, no jump verb.
- [x] `ABP_ZS_ThirdPerson` AnimGraph rebuilt via MCP, compiled clean. `bIsCrouched` added to `UZSAnimInstanceBase`, wired to the stance selector.
- [ ] Crouch pose doesn't apply in the AnimGraph — confirmed real (not a stale-compile artifact), explicitly deferred. See `Docs/SessionHandoff.md`.

## Tasks
- [x] TopDown perspective in `ApplyCameraPerspective` — `EZSCameraPerspective::TopDown` added, `EnableTopDownPerspective()` sets fixed pitch (`TopDownCameraPitch`, default -70°) + zoom (`TopDownCameraDistance`). Movement (`DoMove`) uses the boom's own yaw instead of controller rotation while in TopDown. Defaults to TopDown on spawn; `ToggleCameraPerspective`/`IA_ToggleView` (bound to `V`) toggles to ThirdPerson (serving as the "OverShoulder" fallback per Decision 1) and back. **Yaw is fixed, not player-rotatable** — the Q/E discrete-rotation feature (`RotateCameraYawStep`, DPad_Left/Right) was built then removed same-session at the dev's request, suspected of interfering with movement (`TopDownFixedYaw` captured once on entering TopDown, never changes). Revisit later if camera rotation is wanted again.
- [x] Hybrid facing: `UpdateCursorFacing` (called from `Tick()`) overrides actor rotation to face the mouse cursor's ground-plane projection, but only while `IsCursorFacingActive()` (aiming, or within `CursorFacingActionWindow` of a fire/interact input) — plain movement is untouched otherwise. Mouse cursor visibility/input mode now toggles with camera perspective (shown+`GameAndUI` in TopDown, hidden+captured in ThirdPerson) since deprojection needs a real tracked cursor.
- [x] Interaction system v1: `UZSInteractableComponent` (new `Source/ZombieShooter/Interaction/`) + `AZSPlayerCharacter::TryInteract`/`Server_Interact`/`UpdateNearestInteractable` (sphere-overlap scan each tick) + `OnNearestInteractableChanged` delegate for a HUD Blueprint to bind. **Visual world prompt widget (the actual "F — Open" UMG) is not built** — this is presentation-layer UMG/Blueprint work, deliberately left as a follow-up rather than fabricated blind; the C++ hook (`OnNearestInteractableChanged`, `InteractionVerb`) is what a WBP needs to bind to.
- [x] Gamepad bindings — already existed for every pre-P1 action (confirmed by reading `IMC_ZS_Default` directly); added matching gamepad bindings for `IA_Interact` (FaceButton_Right) and `IA_ToggleView` (FaceButton_Top). `IA_RotateCameraLeft`/`IA_RotateCameraRight` (DPad_Left/Right) are now orphaned along with their C++ — harmless, not yet manually deleted (see `Docs/SessionHandoff.md`). Real controller-in-hand feel validation still needs the dev.
- [ ] Graybox test map — **doesn't exist**. `Lvl_ThirdPerson` (the stock template map) is still the only playable level. Not attempted this session — real level layout is a design-judgment task, not something to fabricate blind.
- [ ] Switch to LowPolyWeapons assets — **investigated, not attempted**. `Content/LowPolyWeapons/` only has static meshes; the weapon's visual mesh path (`AZSWeapon`'s receiver) currently expects a skeletal mesh compatible with Infima's socket/attachment setup for magazine/animation support. A real swap needs either a rigging pass on a chosen LowPolyWeapons mesh or an `AZSWeapon` mesh-component change — a dev asset-selection + integration task, not a blind C++ swap.

## Exit criteria
Moving/aiming/shooting *feels good* at top-down distance with both input methods, 2-client PIE. See `Docs/SessionHandoff.md` for current status against this bar.

## Non-negotiable constraint
Full direct player control and normal movement at all times — this camera decision borrows only *Door Kickers 2*'s angle/framing, never its squad-command/pause-and-plan gameplay layer.

## Note for future fixes
Camera shakes when changing aimpoint.
