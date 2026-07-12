## What

## Phase / area
<!-- Which phase in Docs/CoreLoopPlan.md, and which system -->

## Testing
<!-- PIE (single-player / multiplayer via Play In Editor Multiplayer Options), what was verified -->
- [ ] Compiles clean (Live Coding or full Build.bat)
- [ ] Tested in PIE

## Checklist
- [ ] Follows the `ZS`/`Zombie` naming convention (see CLAUDE.md)
- [ ] Replicated state follows the `OnRep_X` + `Server_X()` + `HasAuthority()` convention, if applicable
- [ ] No hardcoded tunables (belongs in a `UPROPERTY(EditAnywhere)` / `UZSWeaponConfig`)
- [ ] `Docs/SessionHandoff.md` updated if this changes phase status
