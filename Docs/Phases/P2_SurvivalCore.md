# Phase P2 — Survival Simulation Core (Identity Test #2)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: see `Docs/SessionHandoff.md` for current verification status.**

## Tasks
- [x] `UZSNeedsComponent` (ActorComponent, replicated, data-asset-tuned): Hunger/Thirst/Fatigue/Stamina + rate/performance curves (`UZSNeedsConfig`).
  - Consequence model: `GetPerformanceMultiplier()` (Hunger×Thirst×Fatigue curves, default 1 until curves are authored) scales Stamina regen now — healing rate/aim accuracy/attack recovery are deferred hookups (P3/P5, those systems don't exist yet) but read the same accessor when built. No health drain hookup at all yet — `UZSHealthComponent` doesn't exist until P3.
  - Stamina gates `AZSPlayerCharacter::StartSprint` (`NeedsComponent->CanSprint()`) and force-stops an in-progress sprint at zero.
- [x] World clock on `AZSGameState`: `TimeOfDayHours`/`DayCount` (configurable `RealSecondsPerGameDay` compression), utilities-shutoff timer (`bUtilitiesShutoffTriggered`, randomized day in `[MinUtilitiesShutoffDay, MaxUtilitiesShutoffDay]`). Day/night is data only — no lighting/visual hookup (art/level task, not code).
- [x] Sleep/time-skip (Minecraft-style): `AZSPlayerCharacter::RequestSleep`/`CancelSleepReady` (per-player ready flag) + `AZSGameState::UpdateSleepRequestState` (scans `PlayerArray`, advances the clock by the first/initiating player's requested duration once everyone's ready, applies `Server_ApplySleepRecovery` to each player's needs). `IsSafeToSleep()` (the hostile-radius check) is **stubbed `true`** — real check needs P4's zombies to exist first, tracked there.
- [ ] Moodle UI stack (UMG, 4 severity tiers) + first-pass HUD. **Not built** — presentation-layer editor work, deliberately left as a follow-up same as P1's interaction prompt widget. C++ hooks exist: `OnHungerChanged`/`OnThirstChanged`/`OnFatigueChanged`/`OnStaminaChanged` delegates + `UZSNeedsConfig::GetSeverityTier()` for the 4-tier read.
  - Transparent stat-preview rule not established yet — no UI exists to establish it in.
- [x] Items exist minimally: `UZSItemConfig` (HungerRestore/ThirstRestore) + `UZSNeedsComponent::Server_ConsumeItem`. No `DA_ZS_ItemConfig_*` content instances authored yet, no pickup/inventory (P6).

## Exit criteria
A character's hunger/thirst visibly degrades performance (not health) under normal neglect, and sleep-based time-skip works solo and with a multi-player readiness check; replicated, 2-client PIE. "Visibly" depends on the not-yet-built moodle UI. See `Docs/SessionHandoff.md` for current status against this bar.

## Scope notes
6 moodles v1: Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness. Panic/Stress/Boredom/Temperature are deferred pool. No nutrition micro-sim (calories/protein/fat) — food restores Hunger, quality = bigger/longer restore.
