# Phase P0 — Close Out, Clean Up, Re-Aim

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: in progress.**

The simplification pass that turns the pre-pivot shooter codebase into the survival-game foundation. Nothing new gets built here — this phase only removes/verifies existing systems per `GameDevPlan.md` §2's cut list.

## Tasks
- [ ] Finish Phase 3 M7 — 2-client PIE verification of the pre-pivot replication layer. Verify what's built *before* surgery, so post-surgery breakage has a known-good baseline.
- [ ] Commit currently-uncommitted session-8 work (already compiled clean).
- [ ] De-scope pass per `GameDevPlan.md` §2:
  - Remove `Inspect`/`MagCheck`/`CycleGripAttachment` input actions + bindings + montage wiring (montages/notify classes stay — they're generic).
  - Strip `AZSLaserAttachment` and grip-attachment randomization/variants.
  - Cut weapon-owned cosmetic notifies (`AN_ZS_DropMagazine`, `AN_ZS_EjectCasing`, `ANS_ZS_HideMainMag`, `ANS_ZS_ShowReserveMag`) and `ABP_Weapon`/`ABP_Magazine`.
  - Cut `AZSPhysicsCasing`/`AZSPhysicsMagazine`/`AZSPhysicsObject` cosmetic ejects from the runtime path.
  - Cut `FP_ReloadEmpty`/`TP_ReloadEmpty` variants, gun-camera/bodycam content, procedural ADS/recoil/crouch spring-offset system.
  - Retire the FP spawn path (keep `FirstPersonMesh` component dormant or remove — smallest safe diff wins).
  - Reduce camera perspective enum from 4 (FP/TP/GunCamera/Bodycam) to 2 (TopDown/OverShoulder) — implementation happens in P1, but the enum/cut plumbing starts here.
  - Compile + PIE after each removal cluster.
- [ ] Update `CLAUDE.md`, `Docs/SessionHandoff.md` to reflect `GameDevPlan.md` as plan of record.

## Exit criteria
Clean build, 2-client PIE still passes fire/reload/aim/sprint/crouch with the slimmed action set.

## What carries forward untouched (do not remove)
`AZSGameMode/GameState/PlayerState/PlayerController` + Enhanced Input, the Phase 3 replication layer (`OnRep_`/`Server_` convention), `UZSWeaponConfig`'s data-driven pattern, the notify/montage action system, `BlueprintNativeEvent` policy. Full carry-over table: `GameDevPlan.md` §2.
