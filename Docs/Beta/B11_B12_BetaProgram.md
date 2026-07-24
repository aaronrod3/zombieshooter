# B11 & B12 — The Beta Program

Two sequential gates with genuinely different requirements. **B11 `[INTERNAL]`** = a closed group plays unsupervised for a full session without a developer present. **B12 `[PUBLIC]`** = strangers can obtain it and you can survive the support load.

---

# B11 — Internal Closed Beta

**Size: M (6–8 dev-sessions of *your* work; runs over several calendar weeks)** · **Depends on: B1, B8, B9, B10**

> B11's session estimate covers **your** effort — recruiting, building, triaging, and tuning. The calendar duration is longer because testers need real time to play. Expect 3–6 weeks wall-clock.
>
> **The purpose is not to find crashes.** It is to find out whether the game is *any good*, and whether the systems you have been building in isolation for a year actually compose into a loop someone wants to repeat.

## Entry criteria

- [ ] B10 complete — builds distributable, crashes reported, sessions stable.
- [ ] B9 complete — testers can configure and rebind, so their bug reports are about the game.
- [ ] `99_DefinitionOfBetaReady.md` internal-beta checklist fully passing.
- [ ] Feedback intake channel exists and is staffed by you (OQ-B11-01).

## Exit criteria

- [ ] 6–12 testers have each played 3+ sessions.
- [ ] No crash-class or progression-blocking bug outstanding.
- [ ] Core-loop tuning validated against telemetry, not intuition.
- [ ] Testers voluntarily return for a second session without being asked. **This is the single most important signal in the phase** — if they play once because you asked and never again, no amount of bug-fixing fixes that.

## Task breakdown

### B11-T1 — Program setup · **S (1–2 sessions)**

| Sub-task | Definition of done |
|---|---|
| T1.1 | 6–12 testers recruited (OQ-B11-02). Mix of survival-genre veterans and genre newcomers — they surface completely different problems. |
| T1.2 | Distribution + update channel; testers can get a new build without hand-holding. |
| T1.3 | Feedback intake: structured form (build version, repro steps, save attached) plus an unstructured channel for impressions. Both matter; the unstructured one usually carries the important stuff. |
| T1.4 | NDA/expectations set, however lightweight. |
| T1.5 | B10-T4.3 telemetry dashboard or query set ready — a way to actually read what testers did. |

### B11-T2 — Test cycles · **M (3–4 sessions across the window)**

| Sub-task | Definition of done |
|---|---|
| T2.1 | **Cycle 1 — solo focus.** Isolates survival-loop and onboarding problems from multiplayer noise. |
| T2.2 | **Cycle 2 — co-op focus**, 2–4 players. Session lifecycle, shared world, clue sharing, sleep aggregation. |
| T2.3 | **Cycle 3 — long-session focus.** Multi-day worlds through the utilities shutoff, into B5's investigation arc. This is where late-game emptiness or save degradation shows up, and neither appears in a 1-hour test. |
| T2.4 | Each cycle: build → play window → triage → fix → next build. Do not let cycles overlap; overlapping builds make feedback unattributable. |
| T2.5 | Every cycle produces a written summary of what changed and why. |

### B11-T3 — Triage & fixing · **M (2–3 sessions per cycle)**

| Sub-task | Definition of done |
|---|---|
| T3.1 | Severity triage: crash > progression-blocker > data-loss > gameplay bug > polish. **Data-loss ranks above gameplay bugs** in a permadeath persistent-world game — a lost world is worse than a broken feature. |
| T3.2 | Save-corruption reports get absolute priority. B3's rotating backups should mean recoverable, not lost — verify that in the wild. |
| T3.3 | Every fix gets a regression check in the next cycle. |
| T3.4 | Deferred items explicitly logged as post-beta rather than silently dropped. |

### B11-T4 — Balance & tuning · **M (2–3 sessions)**

Tune from telemetry (B10-T4.3), not from feel.

| Sub-task | Definition of done |
|---|---|
| T4.1 | **Death causes analyzed** — what actually kills players, and at what day. If starvation dominates, the "needs are debuffs first, not death spirals" pillar has failed. |
| T4.2 | **Ammo economy** tuned against real usage. `GameDevPlan` P5 flagged this as needing real playtest data, and this is that data. |
| T4.3 | **Loot scarcity + rarity pool** tuned — is the finite pool actually creating scarcity, or draining in the first two days? |
| T4.4 | **Zombie density** per zone against both fun and B8's budget. |
| T4.5 | **Needs decay rates** against session length. |
| T4.6 | **Infection frequency and the amputation decision window** — how often does the game's headline choice actually get offered, and do players understand it when it does? |
| T4.7 | **Skill XP curves** against real progression rates. |
| T4.8 | Every change lands in `TuningReference.md`. |

## Playtest checkpoints

| ID | When | Signal being read | Pass condition |
|---|---|---|---|
| **PT1** | After cycle 1 | **First-session experience.** Where did they get stuck, confused, or bored? | Median tester survives >20 minutes and understands their death. |
| **PT2** | After cycle 2 | **Co-op dynamics.** Do players split up or stay glued? Does the shared world create stories? | Co-op is meaningfully better than solo, not just solo-with-someone-watching. |
| **PT3** | After cycle 3 | **Long-session retention.** Does day 10 have anything to do? | Late game is not empty — the failure mode `GameDevPlan` §1 explicitly names as PZ's weakness and this project's differentiator. |
| **PT4** | B11 exit | **Voluntary return rate.** | Testers ask when the next build is. |

---

# B12 — Public Beta / Early Access Readiness

**Size: L (12–16 dev-sessions)** · **Depends on: B11**

> Everything here is about **strangers**, and strangers are unforgiving in ways a friendly tester group is not. A confusing first ten minutes that a tester pushed through is a refund from a stranger.

## Entry criteria

- [ ] B11 exit criteria met, including voluntary return.
- [ ] **OQ-B12-01, 02, 03 answered** — pricing, Early Access vs. single launch, and demo yes/no.
- [ ] Business decisions made: `GameDevPlan` §7 cross-cutting Q2 (real marketing title) is still blank, and the repo/project name is still the placeholder "ZombieShooter."

## Exit criteria

- [ ] Store page live with capsule art, screenshots, trailer, and honest copy.
- [ ] A stranger can buy/download, install, launch, and play without contacting you.
- [ ] Public bug-report and community channels exist and are monitored.
- [ ] Legal and business basics are in place.

## Task breakdown

### B12-T1 — Product identity · **S (2–3 sessions)**

| Sub-task | Definition of done |
|---|---|
| T1.1 | **Real title chosen.** `GameDevPlan` §7 cross-cutting Q2 has been open since the pivot. "ZombieShooter" is a placeholder and is also actively misleading — this is a survival sim, not a shooter. |
| T1.2 | **One-sentence pitch/logline** (Open Questions §1). §1 of `GameDevPlan` has a good long-form version; it needs a one-line form that survives a store page. |
| T1.3 | Key art / capsule art direction. |
| T1.4 | Trademark/name availability checked before anything is printed on a store page. |

### B12-T2 — Store presence · **M (3–4 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T2.1 | Steam page: description, feature list, tags, system requirements from B8's min spec. |
| T2.2 | Screenshots — **from the real game at final quality**, not staged mockups. |
| T2.3 | Trailer. The old P10 already scoped a "trailer-able vertical slice"; this is the cash-out on that. |
| T2.4 | Capsule art at every required size. |
| T2.5 | **Honest positioning against Project Zomboid.** Comparisons will be made whether or not you make them; making them yourself, accurately, is better than being measured against a claim you did not make. |
| T2.6 | Pricing and EA decision published (OQ-B12-01, 02). |
| T2.7 | Wishlist campaign started **well before launch** — see the timing note in `T_ContinuousTracks.md` T3. |

### B12-T3 — Localization · **M (2–3 sessions)** · *depends on B9*

| Sub-task | Definition of done |
|---|---|
| T3.1 | All UI text routed through `FText` + string tables (B9's note made this a standing requirement). |
| T3.2 | Localization scope decided → OQ-B12-04. **Recommended: English-only for beta**, with the pipeline in place so adding languages later is a translation cost, not an engineering cost. |
| T3.3 | If any languages are added, verify text expansion does not break B1's layouts (German and Russian are the usual layout-breakers). |

### B12-T4 — Community & support · **S (2–3 sessions)**

| Sub-task | Definition of done |
|---|---|
| T4.1 | Public bug-report pipeline (Discord channel, forum, or issue tracker) → OQ-B12-05. |
| T4.2 | Community channels live and monitored. |
| T4.3 | Known-issues list published — pre-empting reports is cheaper than answering them individually. |
| T4.4 | A stated response/patch cadence, so expectations are set. |
| T4.5 | Refund/support policy understood, especially under Early Access. |

### B12-T5 — Launch readiness · **M (3–4 sessions)**

| Sub-task | Definition of done |
|---|---|
| T5.1 | `99_DefinitionOfBetaReady.md` public checklist fully passing. |
| T5.2 | Launch-day runbook: what to monitor, what triggers a hotfix, how a hotfix ships. |
| T5.3 | Rollback tested, not just documented (B10-T5.5). |
| T5.4 | **Load expectations set** — for a listen-server game there is no server load, but *your* support load is real and is the constraint that actually bites a solo developer. |
| T5.5 | Post-launch roadmap drafted from the POST-BETA backlog: hostile human roamers (Decision 5, the confirmed first addition), vehicles, deferred skills, sandbox sliders, dedicated servers, seasons. |

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Store-page cold read** — show it to someone who knows nothing about the project. | They can say what the game is and who it is for, unprompted. |
| **PT2** | End of T3 | Localization smoke test at max text expansion. | No layout breaks, no truncation, no missing strings. |
| **PT3** | B12 exit | **Cold-install stranger test** — someone with no context buys/downloads and plays for an hour with zero help from you, including finding and configuring settings and hosting a co-op session for a friend. | They get in, play, and can host. Any point where they would have contacted you is a launch blocker. |
