# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-21, same-day follow-up #9) — P5 real-time hotbar built, NOT yet compiled/PIE-tested

Built the actual loadout hotbar on top of the design resolved last round, per the dev's explicit go-ahead ("start building the p5 loadout system"):

- **`AZSPlayerCharacter` gained a real-time 9-slot hotbar**: `HotbarSlots` (replicated `TArray<UZSWeaponConfig*>`, always exactly 9 elements), seeded at `BeginPlay` from a new `StartingHotbarLoadout` array field (replaces the old single `StartingWeaponConfig` field, which is now retired - **the dev will need to re-author `BP_ZS_PlayerCharacter`'s loadout as an array entry**, the old field is gone, not aliased).
- **Player now genuinely starts unarmed** - `BeginPlay` no longer auto-equips anything. `RefreshBodyMeshFromWeapon` also now restores a body mesh cached at `BeginPlay` (`UnarmedBodyMesh`) whenever unequipped, so the body visually reverts too, not just the weapon actor.
- **Both resolved input schemes built**: `SelectHotbarSlot`/`HandleHotbarSelect` (number keys 1-9, direct) and `CycleHotbar`/`HandleHotbarCycle` (scroll, skips empty slots, wraps around). Re-pressing the already-equipped slot's number unequips back to bare-fist (toggle, not a separate key).
- **Switching takes real time, not instant**: `Server_SelectHotbarSlot` sets `bIsBusy` and starts a timer for the target weapon's new `UZSWeaponConfig::EquipTimeSeconds` field (default 0.75s) - or a flat `UnequipTimeSeconds` (0.4s) when the destination is bare-fist - then `CompleteHotbarSwitch` actually calls the existing `EquipWeapon`/clears `CurrentWeapon`. One combined delay, not a separate holster-then-equip two-phase sequence (v1 simplification, documented in the header - no equip montage content exists yet to justify more).
- **Deliberately NOT built**: `SecondaryHand`. The design pass resolved it as independently-usable, but *how its own action gets triggered* is still open (`GameDevPlan.md` §7) - a slot with no way to use it would just be dead code, so it's staying out until that question has an answer.
- `CurrentWeapon` itself was **not renamed** to `PrimaryHand` - it already functions as that slot, and a rename risks the exact "any Blueprint anywhere referencing this identifier can silently break" class of bug already logged under `CLAUDE.md`'s Live Coding lesson. Documented as a deliberate choice everywhere this decision shows up (`GameDevPlan.md`, `CLAUDE.md`, `Docs/Phases/P5_CombatCompletion.md`).

**Not done yet, blocking any test**:
1. **`IA_HotbarSelect` and `IA_HotbarCycle` don't exist as `.uasset`s yet** - need dev creation in-editor (`IA_HotbarSelect`: Axis1D, `IMC_ZS_Default` maps Digit1..Digit9 each to a Scalar modifier of 1..9 on this one action; `IA_HotbarCycle`: Axis1D, mouse wheel). The C++ finders degrade gracefully (same pattern as every other `IA_*`) until then - no crash, just no binding.
2. **`BP_ZS_PlayerCharacter` needs re-authoring** - its old `StartingWeaponConfig` single-reference field is gone; the dev needs to add the same weapon config (and any others to test switching against) into the new `StartingHotbarLoadout` array instead.
3. **No compile attempted this round** (editor likely open - see `CLAUDE.md`'s Workflow Efficiency note on not auto-attempting `Build.bat` while it's open). This is new C++ (new UPROPERTYs on `AZSPlayerCharacter`) - per the Live Coding lesson, prefer a full Rider/VS build or `Build.bat` over Ctrl+Alt+F11 for this round, then a "Compile All Blueprints" pass before trusting PIE.

## Carried forward from earlier rounds - still current, not re-verified this round

**First full P3+P4 integration proof, PIE-confirmed** (2026-07-21, follow-up #7): zombie chase → melee → damage → infection roll all worked end-to-end. Dev's character from that test is still `Incubating` on a Torso wound.

**Known gap, flagged not fixed** (dev's call, deprioritized twice now): `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso unconditionally - and since `Server_AmputateZone` is Arms/Legs only, amputation's infection-clearing path is currently unreachable from a real bite. See `Docs/Phases/P3_HealthDamageMedical.md`.

## Next step

1. **Compile this round's C++** (Rider/VS `ZombieShooterEditor Win64 Development`, or `Build.bat` via PowerShell) - new `UPROPERTY`s on `AZSPlayerCharacter` again, so expect the same Blueprint-parent-link-and-cast-node risk as previous rounds; a "Compile All Blueprints" pass afterward is worth it before trusting PIE.
2. **Dev creates `IA_HotbarSelect`/`IA_HotbarCycle`** and maps them in `IMC_ZS_Default`, and re-authors `BP_ZS_PlayerCharacter`'s `StartingHotbarLoadout` array (the old `StartingWeaponConfig` field is gone) - both required before the hotbar is testable at all.
3. **PIE-test the hotbar**: confirm starting unarmed (no mesh/weapon at spawn), number-key equip with a visible delay before the weapon actually appears, re-pressing the same number unequips, scroll cycles between multiple authored slots, `IA_Attack` still dispatches correctly (ranged/melee/bare-fist) against whatever's currently equipped.
4. **Or continue P3 testing** on the dev's live `Incubating` character: treatment items (`Server_UseItem`) and/or letting the infection clock progress toward `Queasy`.
5. **Or fix the zombie bite zone-targeting gap** (still deferred) - would unblock testing amputation's infection-clearing path specifically.
6. **Zombie visuals** (`Create the Zombie AnimBP`) still not built - every AI test currently relies on log/on-screen-text trust rather than seeing anything.
7. Carried forward, still open: `BT_Zombie`'s wander branch (`BTComposite_Sequence_5`) has zero children; `BP_ZombieAIController`'s fate (delete/repurpose, unused); crouch pose bug untouched; remove temporary hit-confirmation logging/on-screen messages once real impact feedback exists; `SecondaryHand`'s own action-trigger mechanism is still an open design question.
8. **Standing practice**: after any Live Coding patch, consider a "Compile All Blueprints" pass before trusting PIE results (see `CLAUDE.md`'s Live Coding lesson).
