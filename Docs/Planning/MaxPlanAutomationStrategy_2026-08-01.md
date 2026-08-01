# Max 5x: Frequent Scheduled Sessions, Backlog Discovery + Feature Ideation, Parallel Execution, and GitHub Project Setup

> Design doc, approved 2026-08-01. Not yet executed — the 7-step "Concrete steps to execute" list at the bottom is the implementation checklist for whenever we pick this back up. Extends `Docs/AsyncSessionProtocol.md`, doesn't replace it.

## Context

You're weighing the Claude Max 5x plan and want three things layered together: (1) backend/non-manual ZombieShooter work keeps moving while you're away, (2) Claude keeps proactively researching and *ideating* — surfacing gaps against the existing plan **and** thinking up gameplay features/options nobody's written down yet, turning good ideas into concrete jobs once you sign off, and (3) real GitHub infrastructure (issue board, labels, schedule) that both of us use to stay coordinated — not a markdown file only Claude reads. The stated priority is throughput: run scheduled sessions often, keep the ball rolling, don't leave Max 5x's headroom idle.

Two facts anchor this: `Docs/AsyncSessionProtocol.md` already defines the safety rules for unattended work (Mode A/B, no PIE, no Blueprint/content authoring, never touch the `Tests/` suite unattended, one cluster per stream) — this plan extends that protocol, it doesn't replace it. And `Docs/SessionHandoff.md` confirms B1's C++ is fully closed right now — the execution backlog genuinely is thin at this exact moment, which is exactly the gap the discovery/ideation job (below) exists to keep filled continuously, rather than needing a one-time manual scoping pass.

**What Max 5x actually buys**: higher usage ceiling, not new capabilities — still no PIE automation, still no headless Blueprint/content editing (`unreal-mcp` needs a live editor). The right way to spend the headroom is *frequent, small, parallel-but-isolated sessions* — which is exactly what this plan schedules — while every existing safeguard (one cluster per stream, Tests-suite exclusion, no design-shaping decisions made solo) stays exactly as strict as it is today.

## GitHub current-state recon (checked directly via `gh` before proposing anything new, 2026-08-01)

- **Issues**: zero currently exist on `aaronrod3/zombieshooter`.
- **Labels**: only the original `phase-0`...`phase-6` set (plus GitHub defaults) — these map to the **retired pre-pivot P0-P6 plan** (`phase-1` = "Core gameplay framework", `phase-6` = "Core loop integration"), not the current B0-B12 Beta plan. Stale, not deleted — cheap to just stop using and add new ones alongside.
- **Projects**: two boards exist, neither usable as-is:
  - **"ZombieShooter Core Loop" (#2)** — correctly scoped to this repo, but **0 items**, private, description still points at `Docs/CoreLoopPlan.md` (explicitly retired in `CLAUDE.md`). A clean, empty, correctly-scoped vessel — this plan repurposes it rather than creating a third board.
  - **"Game Development Tracker" (#1)** — nicer field set (Priority/Area/Effort/Risk/Start-Target Date/Build) worth borrowing, but its 5 items are all issues on a **different repo**, `aaronrod3/ShooterGame` (the old pre-ZombieShooter project `CLAUDE.md` says not to reference). Left alone in this plan — flagged, not touched. Archiving is a separate call.

## The building blocks

### 1. Label taxonomy refresh

Add (don't delete anything): phase labels `B0`...`B12` matching `Docs/Beta/`, plus workflow-state labels `away-safe:ready`, `needs-triage`, `manual-only`, `design-question`, `approved`, `blocked`.

### 2. GitHub Issues as the single backlog (replaces a markdown queue file)

Every candidate task lives as an Issue: title = the task cluster (matches phase-doc `Txx` numbering where one exists), body = scope note + source link, labels = phase + workflow-state. Project **#2** becomes the live board: description updated to point at `Docs/Beta/README.md` instead of the retired `CoreLoopPlan.md`; add custom fields mirroring #1's useful ones — `Area` (single-select matching `CLAUDE.md`'s architecture sections: Framework/Player/Interaction/Survival/Weapons/Combat/Zombies/Inventory/UI), `Execution` (Away-Safe-Ready/Needs-Triage/Manual-Only/Design-Question/Approved/Blocked), `Effort`. Triageable from the GitHub mobile app while away — the same "check in from my phone" pattern this whole plan is built around.

### 3. Discovery & Ideation job — gap-mining *and* thinking up what's missing

Two things every run, not just one:

- **Gap-mining** (against what already exists): re-reads `Docs/GameDevPlan.md` §3, every `Docs/Beta/*.md` phase doc, `90_OpenQuestions.md`, `99_DefinitionOfBetaReady.md`, `CLAUDE.md`'s architecture bullets (grepping for "not yet built" / "content gap" / "deferred" — Shove/Mount/Climb, Steam invite infra, the 5 parked test failures, the crouch-pose bug), and recent git log for anything landed but undocumented. Also mirrors any `90_OpenQuestions.md` entry (OQ-B1-03, OQ-B6-07, OQ-X-03, etc.) that doesn't yet have a corresponding Issue, so the doc-based open-questions list and the GitHub board stay in sync instead of forking into two backlogs.
- **Feature ideation** (genuinely new): thinks about gameplay features and options within the design's actual scope contract (`GameDevPlan.md` §3's KEEP/SIMPLIFY/REPLACE/CUT table, and the pillars it's built around — needs/moodles, noise-as-threat, permadeath+persistent world, cure arc, 4+p co-op) and proposes concrete options/trade-offs for each idea, same shape as an `90_OpenQuestions.md` entry. Filed as a **parent** Issue labeled `design-question`, body laid out as "Option A / Option B / trade-offs" — never auto-decided.
- **Classification rule (safety-critical)**: only unambiguous, already-in-scope, pure C++/docs/tuning gaps get filed straight to `away-safe:ready`. Anything touching an open design question, anything Blueprint/content/editor-only, or anything with real ambiguity gets `needs-triage`, `manual-only`, or `design-question` — never auto-promoted to executable. New feature ideas are *always* `design-question` — ideation is inherently design-shaping, which stays your call, same principle as the existing protocol's "genuine design fork gets asked."
- Checks `gh issue list` first to avoid duplicates. Read/propose-only — never implements. Cadence: **twice a week** (e.g. Sunday + Wednesday) — cheap to run this often since it touches no code.

### 4. Triage & Decomposition job — turns your decisions into jobs, quickly

Cadence: **daily** — this is the "compiling questions to form jobs" step, and it should react same-day, not wait for the next weekly discovery pass.

- Scans Issues for a `design-question` you've resolved since the last run (label flipped to `approved`, or a decision comment) and decomposes the chosen option into concrete sub-issues (native GitHub sub-issue links under the parent), each labeled per the same classification rule above — these are what feeds the execution job.
- Also does light housekeeping: re-flags any PR left open/blocked more than ~24h from a prior execution run, so nothing silently stalls.
- Push notification only if it decomposed something or found something stalled — silent otherwise.

### 5. Execution job — parallel, worktree-isolated, runs often

Cadence: **twice a day** (e.g. `17 2 * * *` and `7 14 * * *`, adjustable) — per the priority on frequency, and cheap to fire more often since an idle/no-op run (empty `away-safe:ready` label, or editor open) is silent and harmless.

- **Source selection**: reads Issues labeled `away-safe:ready`, picks up to **2** whose `Area` field doesn't overlap (conservative default — drops to 1 on any ambiguity; raise the cap once a few trial runs confirm the machine handles 2 parallel Unreal rebuilds fine).
- **Isolation**: spawns that many parallel Agent calls with `isolation: "worktree"` — each gets its own git worktree + branch, so concurrent streams can't collide on uncommitted state. Each stream follows `AsyncSessionProtocol.md` exactly as it reads today: Mode A default (confirm editor closed, implement, full `Build.bat` compile-gate, ~4-attempt cap then Mode B, Tier-1 smoke test only, never simulated PIE input), Mode B fallback if the compile loop stalls, content/Blueprint authoring and the `Tests/` suite still fully off-limits either way. Instead of pushing straight to `origin main`, each stream pushes its branch and opens a PR tagged `Closes #<issue>`.
- **Convergence ("comes together later")**: once all of that run's streams finish (waited on, not fire-and-forget), if more than one produced a `[compiled]` PR, merge them into a temporary integration branch and run **one more** `Build.bat` compile-gate on the combined result before fast-forwarding `main` — catches a same-run collision neither stream could see alone. A single-stream run just merges straight through, same as before. `[uncompiled]` (Mode B) streams stay open as PRs, flagged in the notification, not merged.
- **Cascade safeguard**: if a prior run left an unmerged/blocked PR, the next run doesn't pile new streams on top — it re-notifies (the daily triage job also catches this) and waits.
- **Notification**: push notification only when something happened or needs attention — silent when the backlog's empty or the editor was open.

### 6. `Docs/AsyncSessionProtocol.md` amendment

Add a "Queue mode" section covering: Issues-as-source (not a markdown file), the worktree+PR flow replacing always-direct-to-main, the Area-based parallel-stream selection rule (cap 2, drop to 1 on ambiguity), the same-run integration compile-gate, and the cascade safeguard. Everything currently in the doc (Mode A/B mechanics, "Always, either mode" exclusions, End of session) stays unchanged — this is additive.

## Concrete steps to execute (not yet started)

1. `gh label create` for the `B0`...`B12` and workflow-state labels.
2. Update Project #2's description (point at `Docs/Beta/README.md`); add `Area`/`Execution`/`Effort` fields via `gh project field-create`.
3. Amend `Docs/AsyncSessionProtocol.md` with the Queue-mode section — additive, existing text untouched.
4. Create three scheduled tasks via `create_scheduled_task`, each with a fully self-contained prompt (no memory of this conversation — must name the absolute project path, doc read order, and exact rules above):
   - `zs-backlog-discovery-ideation` — twice weekly
   - `zs-triage-decomposition` — daily
   - `zs-away-session-execution` — twice daily
5. Trial each job once independently (a one-time `fireAt` a few minutes out, or a manually-seeded low-risk issue) before trusting the recurring cadence — there's no way to verify unattended tool/build behavior except by actually running it once.
6. Leave Project #1 and the old `phase-0`...`phase-6` labels untouched — flagged, not acted on.
7. Refresh `CLAUDE.md`'s "GitHub Workflow" section afterward to describe the new label taxonomy and the three scheduled jobs.

## Verification

- Steps 1-3 are directly checkable by reading labels/project/doc back after creating them.
- Step 4's actual unattended behavior can only be verified by real firings. First discovery/ideation run: sanity-check the filed issues (gaps *and* feature ideas) are genuinely new, sensibly labeled, and the ideation ones read as real options rather than vague filler, before trusting the cadence. First triage run: confirm a manually-`approved` test issue actually decomposes into sane sub-issues. First execution run: check the resulting PR(s)/merge by hand before trusting the twice-daily cadence.
- None of this touches PIE/Blueprint verification — that remains entirely your hands, unchanged from today.

## Additional ideas under discussion (2026-08-01+, not yet folded into the execution checklist)

Gathered while continuing to talk through the strategy after initial approval — none of these are scheduled or implemented yet. This section exists so they don't get lost before they're scoped properly; nothing here changes the "Concrete steps to execute" checklist above.

### Present-session techniques (not scheduled jobs — working-style changes for when you're at the keyboard)

- **Parallel research fan-out**: for "how should we do X" questions, spawn 3-4 Explore/general-purpose agents at once across different angles (existing codebase patterns, external UE5 docs, similar systems elsewhere in the project) instead of researching serially. Max 5x's ceiling is what makes this routine instead of rare.
- **Speculative multi-approach spikes**: for a genuinely uncertain design (e.g. the container-loot-screen-vs-auto-loot open question, or any `design-question` issue the discovery job below files), spawn 2 worktree-isolated agents to build both options far enough to compare in PIE, instead of designing on paper and guessing. Pick a winner, discard the other branch.

### Candidate scheduled jobs (would extend the discovery/triage/execution family in the building blocks above)

- **Doc/code drift audit**: periodically checks whether `CLAUDE.md`'s architecture section still matches the actual code — the same category of staleness the GitHub recon already caught once this session (a Projects board pointing at a retired doc nobody had a reason to notice). Propose-only, files an issue per drift found.
- **Tuning-value sanity pass**: checks `UZSNeedsConfig`/`UZSWeaponConfig`/etc. data-asset tunables against `Docs/TuningReference.md` for drift in either direction. Propose-only.
- **Test-failure diagnosis doc (present-session-restricted)**: since the 5 parked automation-test failures are explicitly off-limits to touch unattended (building/running/extending the `Tests/` suite is a present-session-only action per `AsyncSessionProtocol.md`), this job would do root-cause *investigation only* — reading code/logs, never rebuilding or running the suite — and keep a running diagnosis doc current so a present session can fix them faster whenever you sit down for one.
- **Competitive/reference research**: the design is explicitly built against `Docs/ProjectZomboid_DesignReference.md` patterns — a slower-cadence job could research how PZ (or comparable survival co-op games) solved a specific system before you design it from scratch, feeding the feature-ideation half of the discovery job instead of duplicating it.
- **File hygiene / readability audit**: scans source files for setup efficiency and readability — commented-out code blocks (the project's own convention is "No commented-out code — use branches," so any survivor is a direct violation), stale temporary debug instrumentation that's overstayed its welcome (the `ZS.Debug*` console commands are already flagged in `SessionHandoff.md` as "needs removing once real UI exists — don't remove yet"), and inconsistent patterns against the Conventions section of `CLAUDE.md`. Propose-only — flags candidates as issues, never deletes or edits anything itself, since several of these (like the debug instrumentation) are deliberately being kept for now and removing them is a judgment call, not a mechanical cleanup.
