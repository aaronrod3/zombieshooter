# Continuous Tracks

Six disciplines that have no start and no end and therefore are **not phases**. Modelling them as phases would produce a false sequence and would mean, for example, that no bug tracking exists until B11. Each track lists its per-phase entry point.

---

## T1 — QA, Bug Tracking & Playtest Cadence

**Starts: B0. Runs forever.**

The project currently has no automated test suite and no bug tracker — `CLAUDE.md` states "PIE only, no automated suite," and `SessionHandoff.md` carries known bugs as prose paragraphs. That works for one person holding everything in their head; it stops working the moment a tester exists.

| Practice | From | Detail |
|---|---|---|
| **`SessionHandoff.md` remains the sole owner of verification status** | now | Already the rule (`CLAUDE.md`). The B0 addition: **no phase may exit with unverified deliverables.** The failure mode this plan exists to correct is 4+ sessions of unrun code. |
| **Bug tracker** | B0 | GitHub Issues on the existing repo, with the `phase-*` labels and Projects board already set up. Prose bug notes in a handoff doc do not survive a beta. |
| **Regression checklist** | B0 | `Docs/Testing/P5_P6_CharacterSetupVerification.md` is the model — a written stage list with pass criteria. Extend it per phase; run it before each phase exit. |
| **Automated smoke test** | B10 | Not a full suite. One automated test that boots the game, loads a save, spawns a player, and exits cleanly catches the catastrophic build break for very little cost. |
| **Playtest cadence** | B0 | Every phase file defines its own checkpoints. **Run them; do not defer them into the next phase** — that is exactly how the current verification debt accumulated. |
| **External testers** | B11 | See B11-T1. |

**Standing rule:** a bug found during a verification pass is **filed, not fixed inline** (B0-T1.10), unless it blocks the rest of the pass. Verification passes that become debugging passes never finish.

---

## T2 — Build, Version Control & Release Pipeline

**Starts: B0 (hygiene). Formalizes: B10.**

| Practice | From | Detail |
|---|---|---|
| **Build policy during heavy C++ churn** | B0 | Full rebuild over Live Coding for header changes. `CLAUDE.md`'s Live Coding lesson documents two confirmed silent-corruption incidents; B0 is the highest-churn phase in the plan. |
| **"Compile All Blueprints" after patch clusters** | B0 | Cheap insurance against the corruption class that produces no crash and no visible error. |
| **Commit per sub-task** | now | Already the rule. Matters more during B0's refactor — small commits are the bisect targets. |
| **Untracked-content decision** | B0 entry | `git status` currently shows large untracked content directories (`Content/Animation/`, `Content/Maps/`, `Plugins/`, `Content/LowPolyWeapons/`, etc.). **Resolve gitignore-vs-commit before B0's refactor makes the diff unreadable.** $0 LFS budget is the constraint. |
| **Branching strategy** | B10-T5.1 | Everything is on `main` today. A beta needs at minimum a release branch so tester builds are reproducible while development continues. Never force-push `main` (standing rule). |
| **Version stamping** | B10-T3.3 | In-game, in saves, in crash reports. Unattributable bug reports are nearly worthless. |
| **Release checklist** | B10-T5.4 | `Docs/ReleaseChecklist.md`. |

---

## T3 — Marketing, Store Presence & Community

**Starts: B4. Not B12.**

> **The timing is the point.** Wishlists accumulate over months and are the primary driver of launch-day visibility on Steam. A store page that appears two weeks before a public beta has no wishlist base to convert. B12 is where the *launch* happens; the *audience* has to start earlier.
>
> B4 is the right start because it is the first phase producing footage of a game that looks like a game.

| Activity | From | Detail |
|---|---|---|
| **Devlog / build-in-public** | B4 | Low effort, high compounding. A solo survival game is a genre where development transparency is itself a draw. Screenshots and short clips from B4's content work cost almost nothing extra. |
| **Steam page up (wishlists open)** | B5–B6 | Wishlists need runway. The page can be sparse and improve. |
| **Community channel** | B5 | Discord or equivalent. Also becomes B11's tester recruiting pool, which is a real practical benefit, not just marketing. |
| **Trailer production** | B12-T2.3 | Needs B7's audio to not feel cheap. |
| **Demo decision** | B12-T1 (OQ-B12-03) | A demo is a significant additional scope commitment — a separately balanced, separately supported build. Decide deliberately. |
| **Press/creator outreach** | B12 | Survival co-op has an active creator scene. |

**Time cost:** budget ~1 session per fortnight from B4 onward. It is not free, and pretending it is means it does not happen.

---

## T4 — Content Authoring (data assets)

**Starts: end of B0. Runs continuously through B11.**

The project's data-asset-driven architecture means most content is authoring work, not code — which makes it perfectly suited to sessions where you are blocked, tired, or short on time. Treat it as the default filler task.

| Content | From | Blocked by |
|---|---|---|
| **Weapon configs** — 4–6 melee archetypes (one per feel-category) + firearm roster | B0-T1 (minimum), then continuous | OQ-B0-11 (melee display), OQ-B0-12 (roster) |
| **Item configs** — consumables, medical tiers, materials, clothing with insulation | B0-T1 minimum, then continuous | OQ-B0-15 (weight numbers) |
| **Loot tables** per zone quality tier | B4-T7.4 | Zone system (B4-T1.5) |
| **Zombie configs** | B0 | Roster decision (OQ-B7-03) |
| **Needs config** — severity thresholds, decay curves | B0-T4.9 | — |
| **Skill configs** — XP curves, level effects | B6-T1.4 | CR-01, OQ-B6-01 |
| **Background configs** | B6-T4.2 | OQ-B6-04/05 |
| **Event configs** | B5-T2 | OQ-B5-04 |
| **Clue configs + narrative content** | B5-T4 | OQ-B5-01 (the plot) |
| **Basement layouts** | B4-T6.2 | — |

**Standing rule:** every new tunable goes into `TuningReference.md` in the same session it is introduced. `CLAUDE.md` already requires this; it degrades fastest during content-heavy phases.

---

## T5 — Continuous Performance Profiling

**Starts: B0-T12. Feeds B8.**

CONFIRMED requirement: profile early, don't retrofit. A single fixed stress-test scenario, reused project-wide, is also CONFIRMED.

| Practice | From | Detail |
|---|---|---|
| **Fixed stress-test scenario** | B0-T12 | Built early, deliberately, so it measures the un-optimized case and every later comparison shares a baseline. |
| **Methodology** | B0-T12 | `stat unit`/`stat fps` → `stat ai`/`stat anim`/`stat physics` → `stat gpu`/`stat drawcount`. **Packaged Development builds only.** |
| **Re-measure at each phase exit** | B0 onward | Cheap (one session), and it catches the phase that quietly cost 20% frame time while it is still attributable to one phase. |
| **Per-room budget** | B2-T4.3 | The reference room's measured cost is the bar every B4 room is checked against. |
| **Budget lock** | B8-T1 | Observations become commitments. |

**Standing rule:** never quote a PIE or Debug number as a performance fact. CONFIRMED methodology, and it is the single easiest way to be confidently wrong.

---

## T6 — Documentation & Design Records

**Runs throughout.**

| Document | Owner of | Cadence |
|---|---|---|
| `Docs/SessionHandoff.md` | **Verification status only** — compiled? PIE-tested? what's next | Rewritten every session, never appended |
| `Docs/GameDevPlan.md` | Design plan of record — pillars, scope contract §3, decisions §7 | Amended when a decision changes |
| `Docs/Beta/00_MasterPlan.md` | Production plan — phases, dependencies, contradiction register | Amended when a phase is restructured |
| `Docs/Beta/90_OpenQuestions.md` | Every undecided item | Updated as questions resolve; **record the answer and the date, don't delete the question** |
| `Docs/Beta/B<N>_*.md` | Per-phase tasks and status | Checked off as tasks complete |
| `Docs/TuningReference.md` | Every gameplay tunable | Same session the tunable is added |
| `CLAUDE.md` | Conventions, architecture, lessons | When architecture or a lesson changes |
| `Docs/Phases/P0–P10*.md` | **Historical build records.** Not edited by this plan. | Frozen |

**Standing rules** (from `CLAUDE.md`, restated because they are the ones that decay):

- **One fact, one home.** Verification status lives only in `SessionHandoff.md`. Architecture lives only in `CLAUDE.md`. Otherwise every status change becomes a three-file edit and the files drift.
- **Targeted reads for large docs** — `Grep` with context or `Read` with offset/limit. `GameDevPlan.md` is 52KB; this plan adds more.
- **Coarse task granularity** — one task per major deliverable, not per file.
- **When an open question is answered, write the answer *and the reasoning* into `90_OpenQuestions.md`.** A decision without its reasoning gets re-litigated in six months, which is exactly what CR-01 in the master plan is an instance of.
