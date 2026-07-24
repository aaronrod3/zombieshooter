# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves now.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **In progress: B0-T0 (build hygiene).**

## Last completed (2026-07-23) — beta production plan written; B0-T0.4 done

**`Docs/Beta/` created** — 18-file production plan from the current state to a feature-complete beta, built from the two consolidated-changes documents audited against `GameDevPlan.md`, `Docs/Phases/P0–P10`, this file, and `Docs/Planning/`. Phases B0–B12 plus six continuous tracks, ~70 open questions each with options and a BLOCKING/SEQUENCEABLE/LATE tag, and two gate checklists. `Docs/Phases/P0–P10*.md` are untouched and frozen as build-state records.

**B0-T0.4 complete — untracked content resolved.** ~1.55 GB of re-downloadable third-party content gitignored (Fab packs, LyraAnims, raw imports, `Plugins/Marketplace`, pack demo maps); ~39 MB of genuinely authored content committed (`Content/Animation/ZSAnims/`, `Enemy/`, `BlendSpace/`, the working level's partition data, `PhysicsSetup.ini`). `Docs/AssetSources.md` added as the reinstall record that makes the trade safe. Notable: `Content/Maps/` turned out to hold only `ImpactParticles.umap` from the Impacts pack — **it is not the game's map**, so nothing authored was at risk.

**`b0-baseline` tag placed** on the pre-refactor commit as a bisect target for B0-T2.

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

B0 is the highest-C++-churn phase in the plan and runs straight into `CLAUDE.md`'s Live Coding lesson. For the duration of B0:

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits.
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**

## Decisions made 2026-07-23

- **T0.3 — keep `BP_ZombieAIController`**, in case it is wanted later. It stays an unused Blueprint and therefore a live Live-Coding corruption surface, so **include it in every "Compile All Blueprints" pass** rather than assuming it is inert.
- **T0.5 / OQ-B9-01 — all gamepad work and testing deferred to B9.** Not cut, just not verified until then. Assume it does not work.
- **OQ-X-01 — PC only for the initial launch.** Closes `GameDevPlan` §7 cross-cutting Q3, open since the pivot. Console/Steam Deck are POST-BETA and are never a valid scope argument in any B-phase.
- **The one gamepad thing that is NOT deferred**: B1-T2.4 still builds generic focus navigation at the widget base class, and no screen may hardcode a mouse-only interaction. Deferring gamepad *testing* is free; deferring gamepad *architecture* means retrofitting every UI screen in B9. Verify the hook with keyboard arrows/tab, which satisfies accessibility anyway.

## Immediate next step — one sub-task left in B0-T0 (needs the editor)

**T0.2 — "Compile All Blueprints" pass.** Content Browser → select all → bulk Compile. Watch the Output Log for `is not a child class of` or `invalid target type` and fix anything found **before** refactor work starts. This is the clean-slate check, and per `CLAUDE.md`'s Live Coding lesson it is the only reliable way to catch the silent-corruption class of bug.

Then **B0-T1, the verification sweep** — `Docs/Testing/P5_P6_CharacterSetupVerification.md` Stages B–G, 2-client. That's the real work of the next few sessions, ~4–5 of them. Its one content prerequisite is a `Melee`-typed weapon config (Stage F), which needs **OQ-B0-11** — or the documented temporary workaround (reuse the rifle `TP_Mesh`, flagged as temporary) to unblock Stages B–E.

## Blocking decisions needed before B0-T2 (not before T0/T1)

- **OQ-B0-13 — item-instance refactor go/no-go.** The hard blocker; ~5–6 sessions of B0-T2 depend on it, and half is unrecoverable if the direction changes mid-way. Recommendation and reasoning in `Docs/Beta/90_OpenQuestions.md`.
- Also blocking B0, in the same design session: **OQ-B0-01** (scroll arbitration), **OQ-B0-02** (aim cone), **OQ-B0-04** (temperature scope), **OQ-B0-05** (fatigue/perception), **OQ-B0-07** (infection ambiguity in UI), **OQ-B0-11** (melee weapon display — content-blocking for T1.1).
- **Three contradictions need your call**: `Docs/Beta/00_MasterPlan.md` §2 — **CR-01** (skill roster: the consolidated doc quotes a superseded list), **CR-02** (vehicles in or out), **CR-10** (fatigue/perception reading).

## Verification status — carried forward, still current

**Everything built across 2026-07-21/22 remains unverified except two items.** PIE-confirmed on 2026-07-22: the AnimBP rifle-pose fix (unarmed no longer shows a rifle stance) and basic hotbar switching (number keys change the equipped weapon). That is Stage A of `Docs/Testing/P5_P6_CharacterSetupVerification.md`.

**Still unverified:** Stage B (equip delay, attachment sockets, magazine, `TP_Mesh` swap, rifle pose *re*appearing) · Stage C (ranged hitscan) · Stage D (unequip) · Stage E (two-weapon switching) · Stage F (melee dispatch — no `Melee`-typed config authored yet) · Stage G (all of P6's inventory/loot — no content authored yet). This is B0-T1's entire job.

**Two autonomous P6 design calls still unreviewed** (bag-slot depth `Back`+`Hip`; rarity pool global per-session) — now carried as OQ-B0-14 with a recommendation to keep both.

**Known gap, still unfixed:** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso — amputation's Arms/Legs infection-clearing path is unreachable from a real bite. Scheduled as **B0-T5.1**.

## Other still-open items (lower priority)

`BT_Zombie`'s wander branch has zero children (→ B0-T8.1); crouch pose bug untouched; temporary hit-confirmation logging still needs removing (→ B0-T5.5).
