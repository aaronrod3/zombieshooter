# Phase P4 — Zombies (The Enemy, Finally)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: see `Docs/SessionHandoff.md` for current verification status.**

## Tasks
- [x] `AZombieCharacter` + `AZombieAIController` built; `RunBehaviorTree` call is wired but there is **no Behavior Tree/Blackboard asset** (editor-authored content, not built blind - a graceful no-op until one exists, same pattern as every other "content doesn't exist yet" case in this project).
- [x] `UZSZombieConfig` data asset: speed/health/senses/damage/mesh/anim-class/BehaviorTree - a second zombie type is a new `DA_ZS_ZombieConfig_<Name>` instance, zero C++ branches, same rule as weapons. No content instances authored yet, no mesh/anim sourced (placeholder Mixamo/UE-mannequin per this file's own task item - not sourced this session).
- [ ] Explicit performance target from the outset: profile early. **Not done** - nothing to profile without placed zombies/a level.
- [x] Perception: `AIPerceptionComponent` + `UAISenseConfig_Sight`/`UAISenseConfig_Hearing`, radii read from `ZombieConfig` at `OnPossess`. `DetectionByAffiliation` set to detect everyone (a deliberate v1 simplification - no `IGenericTeamAgentInterface` wiring on `AZSPlayerController`, not needed with only one hostile faction).
- [x] `UZSNoiseSystem::ReportNoise` - thin wrapper over `UAISense_Hearing::ReportNoiseEvent`. Wired into `AZSPlayerCharacter::Server_Fire` (`UZSWeaponConfig::FireNoiseRadius`, per-weapon) and `Server_StartSprint` (one event on sprint start, not per-tick). Breaking glass/barricading etc. aren't built systems yet, so not wired - extend as those land.
- [ ] Behaviors v1 (wander/investigate/chase/attack/door-thumping): **not built** - these are Behavior Tree tasks/nodes, editor-authored content. `AZombieCharacter::Server_MeleeAttack` (self-validates range/cooldown, applies P3 damage+wound type) and `SetChasing` (walk/chase speed swap) exist as C++ entry points ready for a BT task to call, but nothing calls them yet.
- [ ] Zombie reintroduction / zone-based population / respawn-into-cleared-zones: **not built** - a spawner/population system is more level-design/spawner-architecture work than a system `AZombieCharacter` itself should own; not attempted this session.
- [ ] Placeholder visuals: **not sourced**. `UZSZombieConfig::ZombieMesh`/`AnimClass` fields exist and are consumed (`AZombieCharacter::AssembleCosmeticsFromConfig`), but no Mixamo content has been imported/assigned.

## Design note (build for later, don't build now)
Build this phase's AI architecture (Behavior Tree/Blackboard structure, `AIPerception`, the noise system, config-driven "N enemy types, zero C++ branches" pattern) so a second, always-hostile-to-everyone human variant can be added cheaply post-v1 without rearchitecting. Do NOT build that variant now — hostile roamers are confirmed design intent, deliberately deferred past the v1 slice (see Phase P10).

## Exit criteria
A graybox block with a profiled zombie-count budget met; a gunshot visibly drags the neighborhood onto the shooter; 2-client PIE holds up. See `Docs/SessionHandoff.md` for current status against this bar.
