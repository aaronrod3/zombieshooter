# Phase P0 — Close Out, Clean Up, Re-Aim

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: in progress.**

The simplification pass that turns the pre-pivot shooter codebase into the survival-game foundation. Nothing new gets built here — this phase only removes/verifies existing systems per `GameDevPlan.md` §2's cut list.

## Tasks
- [ ] Finish Phase 3 M7 — 2-client PIE verification of the pre-pivot replication layer. **Still the one genuinely open item in this phase** — not confirmed done in any session log or doc.
- [x] Commit currently-uncommitted session-8 work (already compiled clean).
- [x] De-scope pass per `GameDevPlan.md` §2 — compiled clean (−1,896 net lines):
  - [x] Remove `Inspect`/`MagCheck`/`CycleGripAttachment` input actions + bindings + montage wiring (montages/notify classes stay — they're generic).
  - [x] Strip `AZSLaserAttachment` and grip-attachment randomization/variants.
  - [x] Cut weapon-owned cosmetic notifies (`AN_ZS_DropMagazine`, `AN_ZS_EjectCasing`, `ANS_ZS_HideMainMag`, `ANS_ZS_ShowReserveMag`) and `ABP_Weapon`/`ABP_Magazine`.
  - [x] Cut `AZSPhysicsCasing`/`AZSPhysicsMagazine`/`AZSPhysicsObject` cosmetic ejects from the runtime path.
  - [x] Cut `FP_ReloadEmpty`/`TP_ReloadEmpty` variants, gun-camera/bodycam content, procedural ADS/recoil/crouch spring-offset system.
  - [x] Retire the FP spawn path (removed outright — dev directive, stronger than "shelve").
  - [x] Reduce camera perspective enum to `ThirdPerson` only (enum kept for P1's `TopDown` addition — the cut portion is done, the P1 addition isn't).
- [x] Update `CLAUDE.md`, `Docs/SessionHandoff.md` to reflect `GameDevPlan.md` as plan of record.

## Exit criteria
Clean build passes (confirmed). **2-client PIE verification (M7) still outstanding** — the rest of this phase's exit criteria depends on it.

## What carries forward untouched (do not remove)
`AZSGameMode/GameState/PlayerState/PlayerController` + Enhanced Input, the Phase 3 replication layer (`OnRep_`/`Server_` convention), `UZSWeaponConfig`'s data-driven pattern, the notify/montage action system, `BlueprintNativeEvent` policy. Full carry-over table: `GameDevPlan.md` §2.
