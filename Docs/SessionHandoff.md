# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-21, latest session)

**Built the attack path** — the concrete blocker the previous session ended on: P3's wound/bleed/infection/amputation loop had no way to actually take damage in PIE (guns had no projectile/hitscan, melee didn't exist). Both pieces are now built, dev confirmed "yes, build both":

1. **Gun hitscan damage** (`AZSPlayerCharacter::Server_Fire`): a real `LineTraceSingleByChannel` (`ECC_Visibility`) from the weapon's `SocketMuzzle` (falls back to eye height if that socket's missing on a given weapon), along the character's current forward vector — which P1's cursor-facing override has already turned toward the mouse cursor by the time a shot fires. Applies `UZSWeaponConfig::FireDamage` via `ApplyPointDamage` on a hit, using the new `FireDamageTypeClass` field (unset falls back to `UZSDamageType_Laceration`, same "unset = generic marker" pattern as `UZSZombieConfig::AttackDamageTypeClass`). New `UZSWeaponConfig` fields: `FireDamage` (25), `FireRange` (5000), `FireDamageTypeClass`.
2. **Player melee attack** (`AZSPlayerCharacter::Server_MeleeAttack`, new): independent of `CurrentWeapon` — works bare-handed or with a gun equipped, one flat melee action (no per-weapon melee config yet, that's a v2 concern). Finds the nearest valid target via a sphere overlap (same `ECC_Pawn` object-query pattern `UpdateNearestInteractable` already uses), excluding self and any other `AZSPlayerCharacter` (no PvP melee in v1) and anything behind the character (forward-cone check). Dead zombie corpses are excluded for free — `AZombieCharacter::Die()` already disables collision, so they never show up in the overlap. Applies `MeleeDamage` (20 default) the same `ApplyPointDamage` way. Bound to a new `IA_Melee` input action (not yet created as content — see Next step).

Both routes end in the same `TakeDamage` override every other damage source already used — no branching added there. `AZombieCharacter` takes flat damage as before; `AZSPlayerCharacter` gets a real wound/zone/infection roll as before. This directly unblocks P3's exit criteria ("a scripted damage source can wound/infect/kill") and contributes to P4's ("a gunshot visibly drags the neighborhood onto the shooter" now has a real gunshot).

**Not built this round**: muzzle-flash/impact VFX, a melee swing montage (`MeleeMontage` field exists, no-op until authored), the `IA_Melee` input asset itself + its `IMC_ZS_Default` key mapping.

**Compile status: unverified.** The editor was open (`UnrealEditor.exe` running, Live Coding active — confirmed via `Get-Process`, not the `tasklist` filter that misfired first) when I checked, so `Build.bat` correctly refused ("Unable to build while Live Coding is active"). Per this project's own Live Coding convention (`.cpp`-only is safe via Ctrl+Alt+F11; header changes need a real recompile), this round touched two headers (`ZSWeaponConfig.h`: 3 new fields; `ZSPlayerCharacter.h`: new `IA_Melee` input action, `CanMelee`/`HandleMeleeAttack`/`Server_MeleeAttack`, 5 new tunables) — Live Coding alone may not be reliable for that. Use the editor's own **Compile** button (does a full recompile + proper hot-reload of reflection data), not just Ctrl+Alt+F11, or close the editor and run `Build.bat` if Compile fails.

**Carried forward, untouched this round** (from last session, still open):
- `BT_Zombie`'s wander branch (`BTComposite_Sequence_5`) still has zero children — `BTTask_Wander` exists and works, just isn't placed into the tree yet.
- `/Game/ZS/Enemy/AI/BP_ZombieAIController` is still an unused empty-shell Blueprint — dev's call whether to delete or repurpose.
- The Zombie AnimBP still isn't created — ShooterGame animations are imported and ready, `UZSZombieConfig::AnimClass` is waiting for it.
- Crouch pose bug: still deferred, untouched.

## Next step

1. **Create `IA_Melee`** (Input Action asset) and add it to `IMC_ZS_Default` with a key mapping — the C++ side degrades gracefully without it (same no-op-until-content-exists pattern as every other optional Input Action here), but melee won't fire until it exists.
2. **Recompile** — use the editor's Compile button (safer than Ctrl+Alt+F11 for this round's header changes) or close the editor and run `Build.bat`.
3. **Tune `FireDamage`/`FireRange`/`FireDamageTypeClass`** per `DA_ZS_WeaponConfig_*` instance if the 25 dmg / 5000 range code defaults don't feel right.
4. **PIE test**: a gunshot should now actually damage a zombie (watch `CurrentHealth` via `get_properties` or just watch it die) and, once step 1 is done, melee should too. This is the first real end-to-end shot at P3's wound/infection/amputation loop.
5. Once damage is confirmed working: full end-to-end AI+P3 test (wander → hear/see player → chase → melee/bite → P3 wound/infection → treatment/amputation).
6. Finish wiring `BT_Zombie`'s wander branch in-editor; decide `BP_ZombieAIController`'s fate; create the Zombie AnimBP. (Unchanged from last round — see "Carried forward" above.)
7. Crouch bug: still deferred, untouched. Live-property read of `ACharacter::bIsCrouched` during a real crouch press is still the next diagnostic step whenever picked back up.
