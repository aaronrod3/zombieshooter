# Master Development Workflow

**What this is:** an execution manual — *how* to actually work through the already-planned phases, session to session, efficiently. It does not replace anything: `00_MasterPlan.md` stays the source of truth for *what* and *why* (phases, dependencies, contradiction register); each `B<N>_*.md` stays the source of truth for *which tasks* a phase contains; `90_OpenQuestions.md` stays the source of truth for *undecided things*; `TuningReference.md` stays the source of truth for *numbers*. This document is the missing layer: the repeatable procedure that turns any of those task lists into finished, verified work without accumulating the kind of debt B0 itself exists to pay down.

**Read order:** `SessionHandoff.md` (what happened last, what's next) → this file, once, to internalize the loop → the current phase's own `B<N>_*.md` (or `00_MasterPlan.md` §3.2 if you need to confirm which phase that is) → back to this file's §10 for phase-specific gotchas as you reach each one.

**Audience:** the dev, and any future Claude Code session picking this project up cold.

---

## 1. Where the project stands right now

- **Solo dev + AI assist, part-time ~15–20 hrs/week.** Sizing throughout the plan is in dev-sessions (≈3–4 focused hours), not calendar time.
- **Current phase: B1 — UI/UX Foundation** (per `SessionHandoff.md`). Most of B1's C++ is written and compiled; almost none of it has a real PIE pass yet. `B1_UI_UX.md`'s "Outstanding testing" section is the authoritative list of what's actually unverified and *why* — read it before assuming B1 is closer to done than it is.
- **B0 carries real verification debt.** A large fraction of `B0_Stabilization.md`'s task table is marked "🔧 code complete, not yet PIE-verified," not "✅ done." This is not a crisis — it's the exact situation `T_ContinuousTracks.md` T1 exists to prevent from getting worse — but it means **the highest-leverage work available right now is verification, not new features.**
- **2026-08-27: the extraction pivot (CR-13).** The design premise moved from persistent-open-world survival to a hub-and-raid loop. Three new phases exist (`BH` Hub/Hideout & Economy, `BR` Raid Lifecycle & Extraction, `BF` Human Hostile AI Faction), none scoped yet (no task-breakdown file), and the needs/skill systems were reshaped in the design docs (not yet reflected in running code — `UZSNeedsComponent` still has Fatigue/Wet/Temperature; that refactor is deliberately deferred, see §10). First additive code for the three new phases (`UZSHubSubsystem`, `AZSExtractionPointActor`, the `AZSHostileCharacter` family, `AZSGameState::Server_StartRaidReseed`) landed the same day — see the git log for the commit. None of it touches or invalidates B0/B1 work.
- **Net effect on sequencing:** B0 and B1 still come first, unchanged. `BH`/`BR` now slot in immediately after B1, before `B3`, per §2 below — this is a change from the pre-pivot dependency chain (B0→B1→{B3,B4,B6-Sys} in parallel), because save topology (`B3`) and world systems (`B4`) both need the hub/raid split settled first.

---

## 2. The full build order

One list, dependency-ordered, reflecting CR-13. This supersedes the pre-pivot chain in `00_MasterPlan.md` §4.1's ASCII diagram for the phases CR-13 touches (`BH`/`BR`/`BF` aren't drawn there yet — see that section's own note on why).

### Stage 1 — Core Playable Loop

| # | Phase | Status | Depends on | Delivers |
|---|---|---|---|---|
| **B0** | Stabilization & Reconciliation | In progress, verification debt | — | Item-instance model, camera/aim revision, needs/health revisions, all PIE-verified |
| **B1** | UI/UX Foundation | In progress | B0 | HUD, inventory screen, container loot, death/sleep screens, menus |
| **BH** | Hub/Hideout & Economy *(new, unscoped)* | First code landed | B1 | Stash, vendors, currency, contracts, loadout prep |
| **BR** | Raid Lifecycle & Extraction *(new, unscoped)* | First code landed | BH | Raid start/reseed, extraction points, permadeath save flow |
| **B3** | Persistence & Save Backbone | Not started | BH, BR (for the hub/character/zone save split) | Save topology, World Partition streaming, corpse/item lifetime |
| **B4** | World Systems (small graybox) | Not started | B1, B3 | Multi-level/elevation, darkness+light, weather-as-mechanic |
| **B6-Sys** | Progression Framework (generic) | Not started | B0 | Skill/attribute component, generic background config — narrower now, no persistent-perk plumbing |
| | **Stage 1 exit — "Core Loop Playtest"** | | | milestone, not a beta gate |

### Stage 2 — Content, Depth & Release

| # | Phase | Depends on | Delivers |
|---|---|---|---|
| **B2** | Art Direction Lock & Asset Pipeline | B0 (runs parallel to B1/B3) | Kit, material/LOD/collision standards, reference room |
| **B4X** | Region Content Build-Out *(continuous track — "the zone," one seamless raid map per CR-13)* | B4 Stage-1 systems PIE-verified, B2, B3 | The actual playable map, district by district |
| **BF** | Human Hostile AI Faction *(new, unscoped)* | B4X (needs real spawn/guard locations) | Guarded loot, heist-contract threats |
| **B5** | Vendor Contracts & Narrative Line *(reframed from "Events & Investigation")* | B4X | Scavenge/recon/heist/document contracts, radio, investigation arc as a contract line |
| **B6-Content** | Backgrounds, XP hookup, onboarding | B0, B4X | Real skill XP sources, background roster, new-mercenary flow |
| **BV** | Vehicles *(own scoping pass, not yet sized)* | Stage 1 exit | Vehicle actor, fuel/damage, storage |
| **B7** | Audio Production & Horde AI | B4X | Full audio pass, horde-coordination solution |
| **B8** | Performance, Profiling & Optimization | B4X, B7 | Budget lock, measured optimization pass |
| **B9** | Accessibility, Settings & Sandbox Options | B1 (can run parallel to B4X — best "blocked on something else" filler) | Remapping, colorblind modes, difficulty options |
| **B10** | Multiplayer Hardening & Release Engineering | B8 | Session lifecycle, network stress, build pipeline, per-world stash separation (CR-13) |
| **B11** | Internal Closed Beta | B1, B8, B9, B10 | 6–12 testers, tuning from telemetry |
| **B12** | Public Beta / Early Access Readiness | B11 | Store page, localization pipeline, launch |

### Continuous tracks (no phase gate — run alongside the whole table above)

| Track | Starts | Feeds |
|---|---|---|
| **T1** QA, bug tracking, playtest cadence | B0 | Every phase's own playtest checkpoints |
| **T2** Build, version control, release pipeline | B0 | B10's formal release process |
| **T3** Marketing, store presence, community | B4 (devlog), B5–B6 (Steam page) | B12's launch |
| **T4** Content authoring (data assets) | End of B0 | Every phase needing weapon/item/loot/skill/background configs |
| **T5** Continuous performance profiling | B8 (the stress-test harness itself, since B0-T12 was deferred here 2026-07-30) | B8's budget lock |
| **T6** Documentation & design records | Throughout | Every phase file, this document included |
| **T7** Region content build-out | Once B4 Stage-1 is PIE-verified | Feeds B5/B6-Content/B7 as districts complete |

**Full task breakdowns, entry/exit criteria, and playtest checkpoints for every phase above live in that phase's own `Docs/Beta/B<N>_*.md` file — this table is the map, not the territory.**

---

## 3. The repeatable per-phase loop

Run this procedure for every phase in §2, in order. It's the same loop for `B4` and for `B12` — only the content inside each step differs.

### Step 1 — Confirm entry criteria for real
Open the phase's `B<N>_*.md`, read its **Entry criteria** checklist, and check each box against actual current state — not memory of what was probably done. A phase started against unmet entry criteria (e.g., building B4X content before B2's material standards are locked) is the single most expensive mistake this plan's own structure exists to prevent.

### Step 2 — Batch-resolve BLOCKING open questions
Grep `90_OpenQuestions.md` for this phase's `OQ-B<N>-*` entries tagged 🔴 BLOCKING. Resolve all of them **in one sitting** before writing any implementation code for the phase — not one at a time as they're hit mid-task. As of this writing the still-BLOCKING set is: `OQ-B5-04` (event/contract roster count), `OQ-B7-01` (horde-coordination approach — gated on B8's measurements, don't force it early), `OQ-B8-01`/`OQ-B8-02` (performance budget numbers). Record the answer **and the reasoning**, dated, directly in `90_OpenQuestions.md` — a decision without its reasoning gets re-litigated later (this is literally what produced the Contradiction Register once already).

### Step 3 — Scope the phase, if it has no task file yet
`BH`, `BR`, `BF`, and `BV` currently have no `B<N>_*.md`. Before writing implementation code for one of these, run a dedicated scoping session that produces that file, matching the exact shape every other phase file already uses:
1. **Size line** — Stage, size estimate (S/M/L/XL), gate (`[INTERNAL]`/`[PUBLIC]`), Depends on / Blocks.
2. **Entry criteria** — a literal checklist, not prose.
3. **Exit criteria** — a literal, testable checklist (if you can't tell pass/fail by performing an action, it doesn't belong here — `99_DefinitionOfBetaReady.md`'s own rule).
4. **Task breakdown** — sub-tasks grouped into T-numbered groups, each with a size estimate and a one-line definition of done.
5. **Playtest checkpoints** — 2–4 `PT` entries with a concrete pass condition, not "feels good."
6. **Notes** — anything explicitly cut, deferred, or genuinely undecided (mirror every other phase file's honesty about gaps).

Seed material for `BH`/`BR`/`BF` already exists in `00_MasterPlan.md` §3.3's scope-boundary table (IN/OUT columns) and CR-13's write-up — don't start from a blank page, start from what's already agreed there. Get the dev's sign-off on the resulting file before starting implementation, the same way `BV`'s own note insists it get "its own design/scope session" rather than being folded into an existing phase's estimate.

### Step 4 — Break the phase into checkpoint-sized units
A checkpoint is **one feature or fix, independently testable**, sized to finish inside roughly one session. This is the dev's own explicit process preference (`RescopeQuestionnaire.md` Part 0), applied throughout every phase file since the 2026-07-26 rescope. Minimize chained dependency between checkpoints on purpose — B0-T2's item-instance refactor is the model: a task that used to be "one uninterrupted block" became five independently-checkpointed steps (A–E) specifically so a failure partway through doesn't invalidate everything built on top of it.

### Step 5 — Run the sub-task loop (§4) for each checkpoint, in order.

### Step 6 — Re-forecast at natural checkpoints, not just phase-end
Every ~4–5 sub-tasks (mirroring `B4X-T10.7`'s "progress checkpoint every ~4 sessions" rule), write down: how much is actually done, how long it actually took, and whether the phase's original size estimate still holds. If a phase is overrunning, surface it immediately — cutting scope is always cheaper than discovering the overrun at exit.

### Step 7 — Run the phase's own playtest checkpoints when scheduled, not deferred
Each `B<N>_*.md` defines its own `PT1`, `PT2`, … at specific points in the task list. Run them there. **T1's standing rule: "run them; do not defer them into the next phase" — that is exactly how the current verification debt accumulated.** If a checkpoint genuinely can't run yet (needs 2 humans, needs a feature that doesn't exist), say so explicitly and track it — `B1_UI_UX.md`'s "Outstanding testing" section is the model for how to do this honestly rather than silently.

### Step 8 — Exit the phase against its literal Exit criteria checklist
Every box either genuinely passes, or is explicitly carried forward with a written reason (the B0→B1 2-client-verification carry-forward is the precedent — done deliberately, documented in both files, not silently dropped).

### Step 9 — Update `SessionHandoff.md` once, at the end of the actual working session
Not mid-session, not per sub-task — see §9.

---

## 4. The sub-task loop (inside Step 5 above)

This is what happens for one checkpoint-sized unit of work, end to end.

1. **Classify the work.** C++ (base classes, data contracts, perf-sensitive/shared machinery) or Blueprint (gameplay config, tuning, AnimGraphs, data assets, Behavior Trees) — per `CLAUDE.md`'s tech-split convention. A task that's genuinely both splits into a C++ sub-step (buildable/testable now) and a content sub-step (dev-hands-only, tracked separately in the phase file's "Manual setup steps" section).
2. **Find the sibling to mirror before inventing a new pattern.** This project's biggest efficiency lever is the multi-config rule (N types = N data-asset instances, never a new C++ branch) and reusing an existing class family's shape. Concrete precedent from this session: `AZSHostileCharacter`/`AZSHostileAIController` were built by mirroring `AZombieCharacter`/`AZombieAIController` almost line-for-line, which made a brand-new AI faction low-risk to write without any PIE testing available. Before writing a new system, grep for the nearest existing analog and read it fully first.
3. **Implement**, following the standing conventions: `Server_`-prefixed mutators gated on `HasAuthority()`; `ReplicatedUsing=OnRep_X` + manual `OnRep_X()` call right after every authoritative mutation (OnRep never fires on the authoring machine); `GetLifetimeReplicatedProps`/`DOREPLIFETIME`; `BlueprintNativeEvent` for gameplay execution points a Blueprint child should be able to override.
4. **Build.** Decision tree:
   - `.cpp`-only change, editor already open → Live Coding (Ctrl+Alt+F11) is fine.
   - Any header change, OR a heavy-C++-churn stretch (B0-style), OR the editor is closed → full rebuild:
     ```bash
     & "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ZombieShooterEditor Win64 Development "-project=C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -waitmutex
     ```
     Confirm the editor isn't running first (`Get-Process | Where-Object { $_.ProcessName -match "Unreal|LiveCoding" }` in PowerShell, not Bash's `tasklist` — confirmed unreliable through this environment's shell layer).
   - After **any** Live Coding patch: run a "Compile All Blueprints" content-browser pass before trusting PIE results. Two confirmed silent-corruption incidents exist in this project's history (`CLAUDE.md`'s Live Coding lesson) with no crash and no visible error — only `is not a child class of` / `invalid target type` in the Output Log.
   - **Verify the build actually recompiled** — check `Binaries\Win64\UnrealEditor-ZombieShooter.dll`'s `LastWriteTime` against the newest edited source file. A "successful build" report has been confirmed stale before in this project.
5. **Verify**, by kind:
   - **Pure server-logic/state/math** → write or extend an Unreal Automation Test in `Source/ZombieShooter/Tests/ZSAutomationTests.cpp`. Use `AZSTestHarnessActor` for real constructor-subobject components (`NewObject`+`RegisterComponent` does not reliably call `BeginPlay()`), `ZSTest::FScopedTestWorld` for a bare offline `UWorld`. Run headless:
     ```bash
     & "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -ExecCmds="Automation RunTests ZS.; Quit" -unattended -nopause -nullrhi -log="<unique_name>.log"
     ```
     Always pass an explicit, unique `-log=` — the default log path silently fails to run at all while the GUI editor is open. **Gotchas hit and fixed this session, worth re-checking any time a test fails unexpectedly:** (a) a `TArray` returned **by value** (e.g. `GetCarrySlots()`) creates a dangling pointer if `.FindByPredicate()` is called directly on the temporary — store it in a named local first; (b) a `UGameInstanceSubsystem` declares `ClassWithin=UGameInstance` — `NewObject<>()` needs a real `UGameInstance` outer, not the default transient package; (c) content-asset `LoadObject` paths in tests go stale silently whenever content gets reorganized — cross-check the string against the actual file on disk (`find`/`Glob`) before assuming the *logic* is broken.
   - **Feel/visual/multiplayer/anything needing real PIE input** → no automation path exists (confirmed dead end: simulated input doesn't reliably reach a pawn, `unreal-mcp` can't drive a second client, `computer-use` can't even resolve the editor window). Hand off a **precise checklist** of exactly what to check and why — mirror `B1_UI_UX.md`'s "Outstanding testing" section's structure (grouped by *why* it's blocked: needs 2 humans, needs a naive tester, needs sustained play to reach a rare state, needs a not-yet-built feature). Never report this kind of work as "done" — only as "written/compiled, needs your PIE pass," and say exactly what to look for.
6. **Update docs that changed as a side effect**: a new tunable → `TuningReference.md`, same session it's introduced (T4's standing rule); a dev-hands-only step discovered → the phase file's own "Manual setup steps" section.
7. **Commit.** One commit per sub-task (`CLAUDE.md`'s standing rule — also T2's, restated because it's the one that decays fastest). See §8.
8. **Only then** move to the next checkpoint. Never stack multiple unverified checkpoints — this exact failure mode (4+ sessions of unrun code before the first PIE check) is why B0 exists at all.

---

## 5. Efficiency doctrine

Rules already learned the hard way in this project's own history, stated as a checklist so they don't have to be re-derived:

1. **Systems before volume.** Prove a system on a small graybox test area before building content against it (`B4` before `B4X` — the entire reason that split exists).
2. **Lock the pipeline exactly once, then execute.** `B2` decides art/material/LOD/collision standards a single time so `B4X` is pure, repeatable execution, not exploration.
3. **De-risk save against graybox before content exists.** `B3` before `B4X` — a save bug found after content is built means revisiting every streaming cell.
4. **Batch open-question resolution.** Don't drip-feed decisions mid-implementation (§3 Step 2).
5. **Checkpoint size = one feature, not one phase** (§3 Step 4, §4).
6. **Never trust a PIE or Debug-build number as a performance fact.** Packaged Development builds only, always (T5's standing rule — "the single easiest way to be confidently wrong").
7. **Profile in triage order, optimize in measured order.** `stat unit`/`stat fps` → `stat ai`/`stat anim`/`stat physics` → `stat gpu`/`stat drawcount`. Never guess the order.
8. **File a bug found during verification; don't fix it inline** unless it blocks the rest of the pass (T1's rule) — a verification pass that becomes a debugging pass never finishes.
9. **Reuse relentlessly.** The multi-config rule and the sibling-class pattern (§4 step 2) are the biggest available lever against re-solving the same problem per system.
10. **Let continuous tracks fill idle/blocked time.** Content authoring (T4) is explicitly the default filler task; `B9` is explicitly the best "blocked on something else" filler, since almost all of it depends only on `B1`.
11. **Build content one unit at a time, timed, before committing to the next.** District-by-district for `B4X` (post-pivot: zone-by-zone within the one raid map) — build one, playtest it, time it honestly, *then* re-forecast the rest from that real number, never from the original estimate.
12. **One fact, one home** (§9) — every fact duplicated across files is a future inconsistency waiting to happen.
13. **Ask before implementing anything design-shaping, mid-phase, not just at phase boundaries.** The dev's own stated process preference, honored throughout every rescoped phase file.
14. **Watch the Explicitly OUT column.** `00_MasterPlan.md` §3.3 exists precisely so scope creep gets caught in review instead of discovered at exit.

---

## 6. Testing & verification workflow — quick reference

| Situation | What to do |
|---|---|
| Pure logic/state/math change | Automation test in `ZSAutomationTests.cpp`, run headless (command in §4.5) |
| Feel, visuals, multiplayer, anything needing real input | No automation path — hand off a precise checklist, dev's hands only |
| Just rebuilt | Check the DLL timestamp before trusting the result |
| Just ran Live Coding | "Compile All Blueprints" pass before trusting PIE |
| A test fails unexpectedly | Check for a stale content-asset path before assuming the logic broke |
| Editor state unknown | `Get-Process` in PowerShell (not Bash `tasklist`) before deciding whether Build.bat is safe to run |

---

## 7. Git workflow — quick reference

- **Commit after each sub-task** (standing rule) — small commits are the bisect targets during any heavy-churn phase.
- **Never commit `Content/**/*.uasset` changes** (modified, deleted, or untracked) — standing, dev-confirmed policy. Stage `Docs/`, `Source/`, and config files explicitly; leave `Content/` out silently, don't re-flag it.
- **Commit message: why, not just what.** Multi-part sessions get a bulleted body, not just a one-line subject.
- **Never force-push `main`.** Everything is on `main` today (no release branches yet — that's `B10-T5.1`'s job).
- **`git push origin main`** targets Gitea (the primary remote); the GitHub mirror syncs automatically via the configured push mirror — no separate push needed.

---

## 8. Documentation workflow — one fact, one home

| Fact | Lives only in |
|---|---|
| Verification status (compiled? PIE-tested? what's next) | `SessionHandoff.md` — rewritten once per session, never appended |
| Architecture / current-state design | `CLAUDE.md` |
| Design pillars, scope contract, numbered decisions | `GameDevPlan.md` |
| Phases, dependencies, contradiction register | `00_MasterPlan.md` |
| Per-phase tasks and their status | Each `B<N>_*.md` |
| Undecided items | `90_OpenQuestions.md` — record the answer *and the reasoning*, dated; don't delete a resolved question |
| Gameplay tunables | `TuningReference.md` — same session the tunable is introduced |
| This execution procedure | `02_MasterWorkflow.md` (this file) |

Rewriting `SessionHandoff.md` mid-session, restating architecture in a phase file, or letting a tunable live only in code are all the same mistake: a fact that now needs a multi-file edit to stay correct, which means it eventually won't.

---

## 9. Phase-specific execution notes

Short, not a restatement of each file's task table — just the sequencing gotcha or first move specific to that phase. Full detail always lives in the phase's own file.

- **B0** — Verification debt is the actual bottleneck right now, not missing code. First move: work the outstanding Checkpoint B/C/D items in `B0-T2`, `B0-T3`, `B0-T4` — most of what's left is "code complete, needs a PIE pass," not "needs to be written."
- **B1** — Current phase. First move per its own file: resume `WBP_ZS_Inventory`'s open/close-toggle build step, then run the "Outstanding testing" backlog as a deliberately scheduled multi-hour, 2-human PIE session — several items are blocked purely on needing two people, not on missing code.
- **BH** — Needs its own scoping pass (§3 Step 3) before more code lands. Seed it from `00_MasterPlan.md` §3.3's `BH` row. First code (`UZSHubSubsystem`) is deliberately minimal and doesn't presuppose the scoping answers (no disk persistence yet, no per-player-vs-shared-stash decision made).
- **BR** — Same: needs scoping. The one open architecture question worth resolving in that same session: *can one player leave a shared listen-server raid without disrupting the teammates still playing?* `AZSGameMode::Server_ReturnPlayerToHub`'s own code comment flags this as genuinely undecided — it's the one call site that changes once it's answered.
- **B3** — Don't start until `BH`/`BR`'s save-shape needs are known. The save-payload inventory in `B3_Persistence.md` already reserves schema slots for future systems (B5/B6) — the pivot adds a hub-save/character-save/zone-save three-way split to that same pattern, not a new architecture.
- **B4** — Stage 1 systems only, on the small graybox test area. If real region geometry starts appearing here, that's `B4X`'s job leaking in early — a scope violation, not progress.
- **B6-Sys** — Narrower post-pivot: no persistent-perk-track plumbing to build (Decision 8 confirmed full reset on death, no exception). Cheaper than the original estimate.
- **B2** — Parallel-safe with B1/B3, any time after B0. Kit selection and pipeline standards are the dev's own taste/research work — Claude Code's role here is mostly file-triage (deleting `Content/LyraAnims/`) and writing down what's decided, not deciding it.
- **B4X** — Continuous track, one unit at a time (§5 rule 11). Start only once B4's Stage-1 systems are PIE-verified on the graybox area.
- **BF** — Needs scoping. First code landed (`AZSHostileCharacter`/`AZSHostileAIController`/`UZSHostileConfig`). No `BT_Hostile` content exists — same graceful no-op pattern `BT_Zombie` used before it existed; perception/damage work today independent of any tree.
- **B5** — `OQ-B5-04` (contract/event roster count and tone) is BLOCKING and is explicitly a writing task, not an engineering one — the phase's own file recommends doing it during `B4`'s downtime, live with the dev, not solo.
- **B6-Content** — Waits on `B4X` (real spawn locations) and `B0` (every system XP hooks into must be final).
- **BV** — Explicitly deferred; needs its own scoping pass once Stage 1 exits, same treatment `B4` got before its own rescope.
- **B7** — `OQ-B7-01` (horde-coordination approach) is BLOCKING and measurement-gated — don't decide it before `B8-T2`'s profiling data exists, no matter how tempting.
- **B8** — `OQ-B8-01`/`OQ-B8-02` (budget numbers) are BLOCKING, re-baselined for 4+ players. This phase also now builds the stress-test harness originally meant for `B0-T12` (deferred here 2026-07-30) — its own "before" baseline, not an imported one.
- **B9** — The best "blocked on something else" filler in the entire plan — nearly everything here depends only on `B1`.
- **B10** — Sequenced after `B8` on purpose. Don't touch performance again afterward without re-measuring network — optimizing invalidates the network numbers.
- **B11 / B12** — `B6-PT4` (the stranger test) and `B11-PT4` (voluntary return rate) are, respectively, the highest-value playtest and the single most important signal in the *entire* plan. Schedule both with a real, genuinely un-briefed person — resist explaining anything.

---

## 10. Immediate next actions

Concrete, as of this writing:

1. **Finish B1's outstanding PIE verification backlog** (`B1_UI_UX.md`'s "Outstanding testing" section is the exact list) — this is the real current bottleneck, not new feature work.
2. **Schedule the 2-client PIE sessions** several of those items need — they're blocked on needing two people, not on missing code, so they won't resolve by continuing to write more C++.
3. **Run `BH`'s scoping session** before writing more `BH` code — produce `Docs/Beta/BH_HubHideoutEconomy.md` per §3 Step 3.
4. **Resolve the per-raid hub-transition question** (can one player leave a shared raid without ending it for teammates?) as part of `BR`'s scoping session — it's the one open call blocking `Server_ReturnPlayerToHub` from being more than a documented stub.
5. **Confirm or revise Decision 10** (investigation-arc folds into the contract system) before `B5` content work starts — it's currently a flagged recommendation, not a dev-confirmed decision.
6. **Decide the needs/skill-system code refactor's own timing** (cutting Fatigue/Wet/Temperature, narrowing the skill roster) — currently deferred on purpose as its own reviewable pass, since it touches already-verified code with real downstream consumers (UI, tuning, footstep noise). Don't fold it silently into an unrelated checkpoint.

---

## 11. Appendix — command sheet

```bash
# Is the editor running? (PowerShell only — Bash's tasklist is unreliable here)
Get-Process | Where-Object { $_.ProcessName -match "Unreal|LiveCoding" }

# Full rebuild (only when the editor is confirmed closed, or a header changed)
& "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ZombieShooterEditor Win64 Development "-project=C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -waitmutex

# Verify the build actually recompiled
Get-Item "C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\Binaries\Win64\UnrealEditor-ZombieShooter.dll" | Select-Object LastWriteTime

# Run the full automation suite headlessly (always a unique -log name)
& "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "C:\Users\aaron\Documents\Unreal Projects\ZombieShooter\ZombieShooter.uproject" -ExecCmds="Automation RunTests ZS.; Quit" -unattended -nopause -nullrhi -log="<name>.log"
```

Full editor-close/rebuild sequencing (when the editor *is* open and needs graceful closing first): `Docs/CommandReference.md`.
