# Phase P2 — Survival Simulation Core (Identity Test #2)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] `UZSNeedsComponent` (ActorComponent, replicated, data-asset-tuned): Hunger/Thirst/Fatigue/Stamina + rate curves.
  - Consequence model: Hunger/Thirst degrade stamina regen, healing rate, aim accuracy, and attack recovery *before* they ever threaten health directly. Health drain only at sustained, deep neglect — not the default outcome of forgetting to eat for a day.
- [ ] World clock on `AZSGameState`: day/night, configurable compression, the utilities-shutoff timer.
- [ ] Sleep/time-skip (Minecraft-style): sleeping requires being safe within a radius of hostiles; in co-op, time only advances once every player is asleep/ready, for a duration the initiating player sets.
- [ ] Moodle UI stack (UMG, 4 severity tiers) + first-pass HUD.
  - Establish the transparent stat-preview rule here for the first time: every consumable/action shows its actual effect on hover, not a hidden number.
- [ ] Items exist minimally: eat/drink consumables via a first `UZSItemConfig`.

## Exit criteria
A character's hunger/thirst visibly degrades performance (not health) under normal neglect, and sleep-based time-skip works solo and with a multi-player readiness check; replicated, 2-client PIE.

## Scope notes
6 moodles v1: Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness. Panic/Stress/Boredom/Temperature are deferred pool. No nutrition micro-sim (calories/protein/fat) — food restores Hunger, quality = bigger/longer restore.
