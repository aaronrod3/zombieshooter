# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-20, latest session)

**Dev confirmed the P3/P4 build succeeded** and ran through the handoff checklist from last round:
- **Done (dev-confirmed)**: rebuild, tuning data assets created (`DA_ZS_ZombieConfig_Shambler` etc.), `AZombieCharacter` content Blueprint created + `ZombieConfig` assigned, zombie visuals sourced (ShooterGame animations imported), orphaned `IA_RotateCameraLeft`/`IA_RotateCameraRight` cleanup.
- **P0's 2-client replication check (M7) is now confirmed done**: fire/reload/aim/sprint/crouch all replicate correctly except crouch's pose - and that's the already-separately-tracked AnimGraph bug, not a new replication finding. `Docs/Phases/P0_CloseOutReAim.md` updated, this phase's exit criteria are now met.
- **P3 damage still can't be PIE-tested**: there's no way to actually deal damage yet - guns have no projectile/hitscan implementation, melee has no attack animation. This is a real, concrete gap (not something I built blind, since "which of hitscan vs. projectile, what animations" are design calls) - next real blocker for testing `UZSHealthComponent`'s wound/bleed/infection/amputation loop.

**This session's dev notes, all handled:**
1. **Renamed "Knox-style infection" → "delayed-onset infection"** everywhere in this project's own docs/code (`CLAUDE.md` had no reference; touched `GameDevPlan.md`, `Docs/Phases/P3_HealthDamageMedical.md`, `ZSHealthTypes.h`, `ZSHealthConfig.h`). `Docs/ProjectZomboid_DesignReference.md` was deliberately left alone - that file's whole purpose is documenting PZ's own real terminology for comparison, not something this project's own naming should avoid.
2. **Design backlog added to `GameDevPlan.md` §7 (P3 section)** - not implemented, recorded for later per dev instruction: amputation needs its own animation; amputation causes a blackout (solo: ~12 real-hour time-skip, real risk of dying while blacked out, so amputation location matters; co-op: still a blackout, but another player can move the body and a revive shortens it); arm amputation restricts weapons to one-handed only; medical item tier delays infection conversion. Also recorded, flagged as a **real divergence from what's currently built**: on death, loot should stay at the death location and the character should become a zombie; co-op continues in the same world with a fresh character unless the whole party dies (world over); solo death ends that world outright (fresh world + character, not just a fresh character in the same world) - `AZSPlayerCharacter::Server_RespawnAsNewCharacter` currently always respawns into the same world regardless, so this is a scoped future change, not yet made.
3. **Analyzed the dev-imported Zombie AI content** (`/Game/ZS/Enemy/AI/`, via `unreal-mcp` - reachable this session) and wired it up:
   - `BT_Zombie`/`BB_Zombie` + 6 custom BT task Blueprints (`BTTask_MeleeAttack`/`Wander`/`GetInvestigationPoint`/`ClearLastKnownLocation`/`StartInvestigationTimer`/`StartIdleDwell`) are real, well-structured, ShooterGame-derived content - not a blind import, a genuinely usable tree. Verdict: **reuse, don't rebuild.**
   - Three of those tasks call functions on `AZombieAIController` that didn't exist (cast `OwnerController` to it, call `TriggerMeleeAttack()`/`StartInvestigationTimer()`/`StartIdleDwell()`) - added all three, plus `Blackboard` key writes (`TargetActor`/`LastKnownLocation`/`bIsInMeleeRange`/`bIsIdling`/`bInvestigationTimerStarted`) in `HandleTargetPerceptionUpdated` and a new 0.2s-interval `Tick`. **Not touched**: `BB_Zombie`'s `ZombieState` (Int) and `bCanSprint` (Bool) keys - no clear intended semantics found in the existing BT graph, left alone rather than guessed at. Two new `UZSZombieConfig` tunables (`InvestigationDurationSeconds`, `IdleDwellDurationSeconds`) back the two timers.
   - **Found and flagged, not fixed**: `/Game/ZS/Enemy/AI/BP_ZombieAIController` is an unused, empty-shell Blueprint - `AZombieCharacter`'s `AIControllerClass` correctly points at the native C++ class directly (confirmed via `get_properties`). `BT_Zombie`'s wander branch (`BTComposite_Sequence_5`) has zero child nodes - `BTTask_Wander` itself is complete and self-contained, just isn't placed into the tree yet.
4. **Added "Create the Zombie AnimBP" as an explicit P4 task** - not built yet; `UZSZombieConfig::AnimClass` is ready to receive it once authored, and the source animations are already imported per the dev.

**This is more new C++ (`AZombieAIController`, `UZSZombieConfig`) - needs another rebuild before any of it does anything.**

**Scope note**: did not proceed into P5 or beyond this round - the dev's explicit call last message was to hold off stacking more phases until P3/P4 gets verified, and everything above is either doc/backlog work or a compatibility layer for content the dev already built, not new phase work.

## Next step

1. **Rebuild** - `AZombieAIController`/`UZSZombieConfig` changed again this round.
2. **Finish wiring `BT_Zombie`'s wander branch** in-editor - drop `BTTask_Wander` into `BTComposite_Sequence_5`.
3. **Decide on `/Game/ZS/Enemy/AI/BP_ZombieAIController`** - it's unused; delete it or repurpose it, dev's call.
4. **Build an actual attack path** so P3 damage can finally be PIE-tested: hitscan or projectile for guns, a melee swing animation + damage application for the player side. This is the real next blocker, more than anything AI-side.
5. **Create the Zombie AnimBP** using the already-imported ShooterGame animations, assign to `UZSZombieConfig::AnimClass`.
6. Crouch bug: still deferred, untouched. Live-property read of `ACharacter::bIsCrouched` during a real crouch press is still the next diagnostic step whenever picked back up.
7. Once a zombie can actually take a swing at a player (steps 4-5 above): full end-to-end PIE test of the AI loop (wander → hear/see player → chase → melee → P3 wound/infection) and of P3's treatment/amputation actions.
