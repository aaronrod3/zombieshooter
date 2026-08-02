# Max 5x: Frequent Scheduled Sessions, Backlog Discovery + Feature Ideation, Parallel Execution, and GitHub Project Setup

> Design doc, approved 2026-08-01. **Phase 0 and Phase 1 complete** (2026-08-01) — see "Phased adoption plan" near the bottom. Phase 2 onward (anything that runs a build or creates a persistent scheduled task) is intentionally paused pending an explicit go-ahead. Extends `Docs/AsyncSessionProtocol.md`, doesn't replace it.

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

### Professional game-dev workflow patterns (researched 2026-08-01)

Cross-checked against how actual studios run production/QA/postmortems, to borrow what transfers to a solo dev + Claude rather than inventing process from scratch. Framing quote worth keeping in mind: research on indie studios specifically found the successful ones "adopt AAA discipline on planning, QA, and live-ops while preserving indie agility on design" — that's the target shape here, not corporate process for its own sake. The QA-industry principle of splitting *repetitive/automatable* work (smoke tests, regression suites, triage clustering) from *judgment* work (exploratory testing, subjective quality) that stays human is also a good validation that this plan's overall shape — Claude proposes/builds, you feel-test and decide — is the right split, not just a new job to add.

- **QA-standard bug triage states**: add `needs-repro` and `awaiting-pie-verification` to the workflow-state label set (block 1). A bug found in PIE currently has nowhere structured to land — this gives it one, using the same states professional bug trackers use to keep backlogs from turning to mush (`Needs Repro` / `Awaiting Logs` / `Blocked` are standard industry triage states, not invented here).
- **Playtest note template + bug-intake pipeline**: a lightweight, pre-built capture template (version/commit, date, objective, what changed) for you to jot during a PIE session, which Claude then triages into properly labeled Issues afterward — including reasoning about likely root cause from the code path even though it can't repro in PIE itself. Mirrors the standard playtest note-taking discipline studios use specifically because unstructured feedback doesn't turn into action.
- **Regression watch-list**: after any merge, flag which `Docs/Testing/` runbooks touch the changed systems, so your next playtest naturally re-covers at-risk areas — the same "test related areas after a fix" discipline studios apply, without needing PIE automation to do it mechanically.
- **Changelog / devlog auto-draft**: draft a changelog entry from git log + closed issues (weekly, or per execution-job run) — cheap, and gives a running human-readable "what shipped" history that `SessionHandoff.md`'s deliberately terse format doesn't try to be. Standard studio devlog practice, and useful raw material if the public repo ever wants real devlogs later.
- **Phase-retrospective job**: at each B-phase close-out, reconstruct a short "went well / went sideways" note by mining git log + `SessionHandoff.md` history + the phase's checklist/decisions doc — the documented professional postmortem process is literally "comb through commits and old docs to reconstruct what happened," which is exactly an LLM-agent-friendly task and exactly the ritual solo devs skip because there's no team forcing it.
- **LFS / repo-hygiene monitor**: periodic check that LFS is tracking what it should and usage is nowhere near a billing edge — cheap insurance against `CLAUDE.md`'s explicit "$0 Actions/LFS spend cap, fail-safe" constraint ever actually getting tripped by an accident (e.g. a large binary committed untracked).
- **Milestone gate-check, sharpened**: professional gates check three things together — playable build, performance benchmark, content review — not just issue-closure counts. Sharpens the earlier "beta-readiness forecast" idea into a real three-legged check against `99_DefinitionOfBetaReady.md` instead of a single progress percentage.

**What doesn't transfer (yet)**: telemetry-driven live balance tuning (Apex/Supercell-style pick-rate analytics) needs live players generating data — not applicable pre-launch, revisit once there's an external playtest pool. Perforce-based asset pipelines and publisher milestone-gate approvals are AAA-scale infrastructure concerns that don't apply to a solo git+LFS project.

## Phased adoption plan

Rolled out in order of consequence, not in the order the ideas were designed — propose-only/doc-only work goes live first, anything that writes code or runs unattended goes live last, and each phase has to be trustworthy before the next one starts. This is the QA principle above applied to the rollout itself: automate the repetitive/reversible stuff first, keep judgment gates in front of anything higher-stakes.

- **Phase 0 — foundation (reversible config/docs, no automation running yet)**: create the `B0`-`B12` + workflow-state labels (including `needs-repro`/`awaiting-pie-verification`), update Project #2's description and add its `Area`/`Execution`/`Effort` fields, amend `Docs/AsyncSessionProtocol.md` with the Queue-mode section. Nothing here runs on its own — it's just the substrate the later phases need.
- **Phase 1 — seed the board manually**: run one discovery-and-ideation pass by hand (this session, not a scheduled job) to file real gap-mining and feature-ideation Issues — gives you something genuine to triage today instead of an empty board, and proves the classification rule (away-safe:ready vs needs-triage vs design-question) produces sane output before any job runs unsupervised.
- **Phase 2 — trial the execution flow once, supervised**: before any scheduled job exists, run exactly one `away-safe:ready` issue through the worktree-isolated Mode A flow with you present and the editor state checked first — proves the compile-gate → PR → `Closes #N` mechanics actually work end to end on this machine before trusting it unattended.
- **Phase 3 — automate discovery & ideation only**: turn on the twice-weekly scheduled job. Lowest-risk one to automate first — it never touches code, only proposes.
- **Phase 4 — automate triage & decomposition**: turn on the daily job. Still no code execution, just label/sub-issue bookkeeping.
- **Phase 5 — automate execution, conservatively**: single-stream only, nightly only (not yet twice-daily, not yet 2-way parallel) — the first job that unattended-writes code and pushes. Let it run clean for a stretch before touching cadence or concurrency.
- **Phase 6 — scale execution up**: raise to twice-daily and/or the 2-way parallel worktree cap, only once Phase 5's single-stream nightly runs have been clean for long enough to trust the machine and the process.
- **Phase 7 — layer in the professional-workflow additions**: bug-intake/playtest-note pipeline, regression watch-list, changelog auto-draft, phase-retrospective job, LFS/repo-hygiene monitor, sharpened milestone gate-check. Mostly propose-only/doc-generation, so these can actually slot in earlier (alongside Phase 3-4) rather than strictly last — noted here as a phase mainly to keep the list complete, not because they're gated behind Phase 6.

**Where this stands right now (2026-08-01)**:
- **Phase 0 — done.** 21 labels created (`B0`-`B12` + `away-safe:ready`/`needs-triage`/`manual-only`/`design-question`/`approved`/`blocked`, plus `needs-repro`/`awaiting-pie-verification`/`tests-suite-only` added while seeding — see Phase 1 below for why). Project #2's description and `Area`/`Execution`/`Effort` fields added. `Docs/AsyncSessionProtocol.md`'s Queue-mode section landed.
- **Phase 1 — done.** 15 real seed issues filed and added to Project #2's board (`aaronrod3/zombieshooter` issues #1-15) — 8 `design-question` (synced from existing open questions in `GameDevPlan.md`/`SessionHandoff.md`/`CLAUDE.md`, several enriched with concrete options), 4 `manual-only`, 1 `tests-suite-only`+`needs-triage`, 1 `needs-triage`+`manual-only`, 1 `blocked`. **Zero `away-safe:ready`** — accurately reflects that B1's C++ is fully closed right now (per `SessionHandoff.md`); this will change once B2 opens or a `design-question` gets `approved` and decomposed. The `Area`/`Execution` custom Project fields weren't back-filled per-item this pass (labels already carry the same classification and are what the scheduled jobs will actually filter on — the Project fields are a visual-board nicety, not load-bearing) — fine to do later if the kanban view matters to you.
- **Phase 2 onward — intentionally paused.** Nothing runs a build or creates a persistent scheduled task yet. That's the first point where something executes unattended against the real project instead of just being written down or filed as an issue, and it's waiting on an explicit go-ahead.
