# Docs/Beta — Production Plan to Feature-Complete Beta

**Created 2026-07-23.** Produced from `ZombieShooter_Consolidated_Changes.md` and `ZombieShooter_Open_Questions_For_Beta.md`, audited against this repo's plan of record.

## What this is, and what it is not

**This is the production plan** — phases, tasks, dependencies, sequencing, and gates that take the project from its current state to a feature-complete beta.

**`Docs/GameDevPlan.md` remains the design plan of record** — pillars, the §3 scope contract, and the numbered Decisions. This plan does not supersede it. Where the two conflict, `00_MasterPlan.md` §2's Contradiction Register is the reconciliation, and unresolved conflicts are flagged for review rather than silently overridden.

**`Docs/Phases/P0–P10*.md` are frozen.** They remain build-state records of work already done and are not edited by this plan. `01_RevisionRegister_P0-P6.md` is the diff between what they describe and what the confirmed beta design requires.

## Read order

**Every session:** `Docs/SessionHandoff.md` → the `B<N>_*.md` file for the phase you are working.

**Coming to this plan fresh:** `00_MasterPlan.md` §1 (executive summary) → §2 (contradictions — three need your call) → §3 (phase list) → §4 (dependency map).

**Before starting any phase:** that phase's entry criteria, then its BLOCKING questions in `90_OpenQuestions.md` §Summary. Batch a phase's blocking set into one design session rather than hitting them one at a time mid-implementation.

## Files

| File | Contents |
|---|---|
| `00_MasterPlan.md` | Assumptions · executive summary · **Contradiction Register** · revised master phase list with IN/OUT scope and sizes · dependency map · risk register |
| `01_RevisionRegister_P0-P6.md` | Every delta the confirmed decisions force onto already-built P1–P6 work. All of it feeds B0. |
| `B0_Stabilization.md` | Verification debt · item-instance refactor · camera/needs/health/combat revisions · profiling baseline |
| `B1_UI_UX.md` | Input modes · HUD · inventory · containers · death/sleep screens · menus |
| `B2_ArtPipeline.md` | Art direction lock · kit selection · pipeline standards · reference room |
| `B3_Persistence.md` | Save topology · layered save · World Partition · corpse/item lifetime |
| `B4_WorldContent.md` | Region · interiors · elevation · darkness · basements · weather · zones · content volume |
| `B5_Events_Investigation.md` | Event director · event roster · radio · clue system · investigation arc |
| `B6_Progression_Onboarding.md` | Skills · XP hookup · backgrounds · character creation · onboarding |
| `B7_Audio.md` | Audio architecture · zombie/combat/ambient audio · **horde AI solution** |
| `B8_Performance.md` | Budget definition · measurement · rendering/AI/network optimization |
| `B9_Accessibility_Settings.md` | Settings framework · remapping · gamepad · accessibility · difficulty |
| `B10_Multiplayer_Release.md` | Session lifecycle · network stress · build pipeline · crash reporting |
| `B11_B12_BetaProgram.md` | Internal closed beta · public beta / Early Access readiness |
| `T_ContinuousTracks.md` | QA · build/VCS · marketing · content authoring · profiling · docs |
| `90_OpenQuestions.md` | ~70 open questions by phase, each with options, tradeoffs, a recommendation, and a BLOCKING/SEQUENCEABLE/LATE tag |
| `99_DefinitionOfBetaReady.md` | The two testable gate checklists |

## The short version

| | |
|---|---|
| **Assumptions** | Solo dev + AI assist · part-time ~15–20 hrs/wk · both beta tiers planned and tagged separately |
| **Phases** | B0–B12, ~195–250 dev-sessions ≈ **10–14 months part-time** |
| **Start here** | **B0.** Nothing else should start first. |
| **Answer first** | **OQ-B0-13** — item-instance refactor go/no-go. A third of B0 depends on it. |
| **Needs your call** | **CR-01** (skill roster — the consolidated doc quotes a superseded list) · **CR-02** (vehicles) · **CR-10** (fatigue/perception ambiguity) |
| **Resolved so far** | **OQ-X-01** → PC only for initial launch · **OQ-B9-01** → gamepad in scope but all work deferred to B9 (architecture hooks kept in B1) · **P4-R9** → keep `BP_ZombieAIController` |
| **Highest-consequence gate** | **B0-PT2** — camera sign-off. The over-shoulder fallback is being deleted; do not delete the code until this passes. |
| **Biggest phase** | **B4** (XXL, 45–60 sessions). Validate its scale against B2-T4.5's measured room build time before committing. |
| **Biggest scope risk** | Temperature + Wet + clothing insulation (🚩 CR-03) — scoped-down proposal in OQ-B0-04. |

## Why B0 exists

Roughly four sessions of C++ shipped between 2026-07-21 and 2026-07-22 with one PIE confirmation covering two features. Underneath sits a data model the project's own planning doc says is wrong. On top of it, the confirmed decisions revise five shipped behaviours and add three subsystems.

Every one of those facts gets more expensive the longer it waits. **B0 is the cheapest this work will ever be.**
