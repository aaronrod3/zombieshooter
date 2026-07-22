# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-21, same-day follow-up #2)

Dev confirmed the missing-mesh bug is fixed (mesh visible in PIE now) and tested the punch — reported "player still shooting" while testing melee, then asked whether to hold off building the real attack dispatch until per-weapon animations exist, or build the functional system now and refine later ("whatever works best with getting these phases done").

**Diagnosis of "still shooting":** `IA_Fire` and `IA_Attack` were both separately bound to input (both apparently mapped to left-click) - every click triggered gunfire *and* a bare-fist punch simultaneously, and since guns visually dominate, the punch was invisible/moot. This was a real, concrete bug, not a testing mistake.

**Fix built this round** (recommended building now rather than waiting on animations - every action system in this codebase already treats a missing montage as a no-op, so there's no real animation dependency blocking this):
- **`EZSAttackType { Ranged, Melee }`** added to `ZSWeaponTypes.h`; new `UZSWeaponConfig::AttackType` field (defaults `Ranged`, so existing configs needed no data migration). No `Unarmed` value on the enum - bare-fist isn't a weapon config instance, it's `AZSPlayerCharacter`'s own fallback when nothing's equipped.
- **`AZSPlayerCharacter::HandleAttack` now dispatches**: `CurrentWeapon`'s config `Ranged` → routes into the existing `HandleFireStarted`/auto-fire-timer machinery (unchanged); no weapon, or `Melee` (no melee `UZSWeaponConfig` instances exist yet to author, so this branch shares the bare-fist path for now) → `Server_MeleeAttack` (unchanged, still the flat `Unarmed*` tunables). New `HandleAttackStopped` (bound to `IA_Attack` Completed) stops the auto-fire timer if one was started - harmless no-op after a melee swing.
- **`IA_Fire` is no longer separately bound** in `SetupPlayerInputComponent` - `IA_Attack` alone drives both paths now. This is the actual fix for "still shooting."
- **Deliberately not built this round** (per the dev's own framing, "get phases done, refine later"): the real `PrimaryHand`/`SecondaryHand`/hotbar loadout system (today "equipped" is still just `CurrentWeapon`, auto-equipped once at `BeginPlay` from `StartingWeaponConfig`, with no in-game unequip/switch action yet - so the `Melee` dispatch branch is currently untestable in practice, nothing can produce a `Melee`-typed `CurrentWeapon`), and real per-weapon melee stats on `UZSWeaponConfig` (a `Melee`-typed config would silently get bare-fist stats today). Both are explicit P5 follow-ups, documented in `Docs/Phases/P5_CombatCompletion.md`.

**This is more header changes** (`ZSWeaponTypes.h`, `ZSWeaponConfig.h`, `ZSPlayerCharacter.h`) - needs another recompile. Given this same session's Live Coding class-link bug (see below), **strongly prefer a full editor Compile or `Build.bat` over Ctrl+Alt+F11** this time, or at minimum re-verify `BP_ZS_PlayerCharacter`'s parent class after compiling (ask to check via `unreal-mcp` if unsure) before trusting PIE results.

**Earlier this same round**, also diagnosed and fixed: PIE showing no player mesh was traced (via the Output Log, not guessed) to a Live Coding class-reinstancing bug that broke `BP_ZS_PlayerCharacter`'s parent-class link to native `AZSPlayerCharacter`, which made `AZSGameMode` silently fall back to spawning the raw C++ class (no `StartingWeaponConfig`, hence no mesh ever equipped). Confirmed self-healed and logged as a new `CLAUDE.md` lesson - see git history (`0b4b55f`) for the full write-up, not repeated here.

## Next step

1. **Recompile** (full Compile/`Build.bat` preferred this round, see above) and re-verify `BP_ZS_PlayerCharacter`'s parent class is intact before testing.
2. **PIE test**: left-click with the assault rifle equipped should now *only* fire, no simultaneous punch. To test the bare-fist branch, clear `StartingWeaponConfig` on `BP_ZS_PlayerCharacter`'s CDO first (no code needed) - with nothing equipped, left-click should throw the punch instead.
3. **P5's remaining scope** (loadout slots, hotbar, per-weapon melee stats) is documented but not started - read `Docs/Phases/P5_CombatCompletion.md` and `GameDevPlan.md` §7's P5 open questions before picking it up, there are real unresolved calls (hotbar key scheme, `SecondaryHand` semantics) worth the dev's input first.
4. Once attack input is confirmed clean: full end-to-end AI+P3 test (wander → hear/see player → chase → melee/bite → P3 wound/infection → treatment/amputation).
5. Carried forward, still open: `BT_Zombie`'s wander branch needs `BTTask_Wander` wired into `BTComposite_Sequence_5`; `BP_ZombieAIController`'s fate (delete/repurpose); Zombie AnimBP not created; crouch pose bug untouched.
