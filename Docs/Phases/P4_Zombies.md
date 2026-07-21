# Phase P4 — Zombies (The Enemy, Finally)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: see `Docs/SessionHandoff.md` for current verification status.**

## Tasks
- [x] `AZombieCharacter` + `AZombieAIController` built and now wired to real dev-authored content: `BT_Zombie`/`BB_Zombie` + 6 custom BT task Blueprints exist in `/Game/ZS/Enemy/AI/` (ShooterGame-derived: `BTTask_MeleeAttack`/`Wander`/`GetInvestigationPoint`/`ClearLastKnownLocation`/`StartInvestigationTimer`/`StartIdleDwell`). Three of those tasks call functions directly on `AZombieAIController` (cast from `OwnerController`) that didn't exist until this pass - `TriggerMeleeAttack()`, `StartInvestigationTimer()`, `StartIdleDwell()` were added to match. `HandleTargetPerceptionUpdated` now writes `BB_Zombie`'s `TargetActor`/`LastKnownLocation` keys and calls `SetChasing`; a 0.2s-interval `Tick` maintains `bIsInMeleeRange`. `BB_Zombie`'s `ZombieState` (Int) and `bCanSprint` (Bool) keys are untouched - no clear intended semantics found in the existing BT graph, not guessed at.
  - **`/Game/ZS/Enemy/AI/BP_ZombieAIController` is unused** - `AZombieCharacter`'s `AIControllerClass` correctly points at the native `AZombieAIController` class directly (confirmed via `get_properties`), and this Blueprint's EventGraph is an empty shell (bare BeginPlay/Tick, nothing wired). Flagging as an unused-asset cleanup candidate, not deleting without confirmation.
  - **`BT_Zombie`'s wander branch (`BTComposite_Sequence_5`) has zero child nodes** - `BTTask_Wander` exists as a class and is self-contained/functional (verified via `read_graph_dsl` - no C++ dependency, just native Nav/AI nodes), but isn't actually placed into the tree yet. Needs finishing in-editor.
- [ ] **Create the Zombie AnimBP** - not built yet. Zombie animations (from ShooterGame) are already imported and ready; `UZSZombieConfig::AnimClass` is ready to receive whatever ABP gets authored.
- [x] `UZSZombieConfig` data asset: speed/health/senses/damage/mesh/anim-class/BehaviorTree/investigation+idle-dwell durations - a second zombie type is a new `DA_ZS_ZombieConfig_<Name>` instance, zero C++ branches, same rule as weapons. `DA_ZS_ZombieConfig_Shambler` exists and is assigned to the `AZombieCharacter` content Blueprint.
- [ ] Explicit performance target from the outset: profile early. **Not done** - nothing to profile without placed zombies/a level.
- [x] Perception: `AIPerceptionComponent` + `UAISenseConfig_Sight`/`UAISenseConfig_Hearing`, radii read from `ZombieConfig` at `OnPossess`. `DetectionByAffiliation` set to detect everyone (a deliberate v1 simplification - no `IGenericTeamAgentInterface` wiring on `AZSPlayerController`, not needed with only one hostile faction).
- [x] `UZSNoiseSystem::ReportNoise` - thin wrapper over `UAISense_Hearing::ReportNoiseEvent`. Wired into `AZSPlayerCharacter::Server_Fire` (`UZSWeaponConfig::FireNoiseRadius`, per-weapon) and `Server_StartSprint` (one event on sprint start, not per-tick). Breaking glass/barricading etc. aren't built systems yet, so not wired - extend as those land.
- [x] Behaviors v1 (wander/investigate/chase/attack): the BT + tasks above cover this (pending the wander-branch wiring gap noted above). Door-thumping (destructible door HP) is **not built** - no destructible door system exists yet.
- [ ] Zombie reintroduction / zone-based population / respawn-into-cleared-zones: **not built** - a spawner/population system is more level-design/spawner-architecture work than a system `AZombieCharacter` itself should own; not attempted this session.
- [ ] Placeholder visuals: mesh not sourced (`UZSZombieConfig::ZombieMesh` unset), but the animations themselves are imported per the dev (see the AnimBP task above).

## Design note (build for later, don't build now)
Build this phase's AI architecture (Behavior Tree/Blackboard structure, `AIPerception`, the noise system, config-driven "N enemy types, zero C++ branches" pattern) so a second, always-hostile-to-everyone human variant can be added cheaply post-v1 without rearchitecting. Do NOT build that variant now — hostile roamers are confirmed design intent, deliberately deferred past the v1 slice (see Phase P10).

## Exit criteria
A graybox block with a profiled zombie-count budget met; a gunshot visibly drags the neighborhood onto the shooter; 2-client PIE holds up. See `Docs/SessionHandoff.md` for current status against this bar.
