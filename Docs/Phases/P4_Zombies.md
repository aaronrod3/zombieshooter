# Phase P4 — Zombies (The Enemy, Finally)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] `AZombieCharacter` + `AZombieAIController`, classic Behavior Tree + Blackboard (per the standing `CLAUDE.md` decision — not StateTree).
- [ ] `UZSZombieConfig` data asset (speed/health/senses/damage — N zombie types, zero C++ branches, same rule as weapons).
- [ ] Explicit performance target from the outset: profile early, don't retrofit efficiency later.
- [ ] Perception: `AIPerception` sight cone + hearing.
- [ ] `UZSNoiseSystem`: every loud act (gunshot, sprint, breaking glass) reports a noise event with a radius. **This is the load-bearing system of the whole game.**
- [ ] Behaviors v1: wander, investigate noise, chase, attack (melee hit → P3 damage/infection), door-thumping (destructible door HP).
- [ ] Zombie reintroduction: zone-based population from data, server-authoritative, respawn-into-cleared-zones on a slow timer.
- [ ] Placeholder visuals: Mixamo/UE-mannequin zombie + Mixamo zombie animations (free) until the art phase.

## Design note (build for later, don't build now)
Build this phase's AI architecture (Behavior Tree/Blackboard structure, `AIPerception`, the noise system, config-driven "N enemy types, zero C++ branches" pattern) so a second, always-hostile-to-everyone human variant can be added cheaply post-v1 without rearchitecting. Do NOT build that variant now — hostile roamers are confirmed design intent, deliberately deferred past the v1 slice (see Phase P10).

## Exit criteria
A graybox block with a profiled zombie-count budget met; a gunshot visibly drags the neighborhood onto the shooter; 2-client PIE holds up.
