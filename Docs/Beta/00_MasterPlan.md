# ZombieShooter — Master Beta Plan

> **Status: v2.0, rescoped 2026-07-26.** Originally produced (v1.0, 2026-07-23) from `ZombieShooter_Consolidated_Changes.md` + `ZombieShooter_Open_Questions_For_Beta.md` audited against `Docs/GameDevPlan.md` §1–§8. **Revised 2026-07-26** after a full rescope pass (`Docs/Planning/RescopeQuestionnaire.md`) replaced dozens of AI-authored "Rec: → CONFIRMED" resolutions with real, dated dev decisions — several of which reverse the original resolution (infection legibility, vehicles, camera fallback, save/death rules). See §2 (Contradiction Register) for the full list and §3.2 for the new two-stage phase structure this produced.
>
> **This document does not supersede `Docs/GameDevPlan.md`.** GameDevPlan stays the design plan of record (pillars, scope contract §3, decisions §7). This is the *production* plan that takes the project from its current state to a feature-complete beta. Where the two conflict, §2's Contradiction Register below is the reconciliation, and every unresolved conflict is flagged for your review rather than silently overridden.
>
> **Read order for a new session:** `Docs/SessionHandoff.md` → this file's §3 (phase list) → the specific `Docs/Beta/B<N>_*.md` phase file you're working.

---

## 0. Stated assumptions

The source prompt requires these be stated rather than left implicit. All three were confirmed by the dev on 2026-07-23.

| Assumption | Value | Consequence for this plan |
|---|---|---|
| **Team** | Solo developer + AI coding assistance (Claude Code). No contractors assumed; art/audio is bought, free-sourced, or self-made. | Phases are sized so any one task survives being picked up cold after a multi-day gap. No task assumes another person is available for a synchronous test. |
| **Cadence** | **Part-time, ~15–20 hrs/week**, worked in evening-length sessions. | Sizing is in **dev-sessions** (1 session ≈ 3–4 focused hours), not calendar weeks. ~4–5 sessions/week is the planning rate. |
| **Beta bar** | **Both tiers planned and explicitly separated.** Every deliverable is tagged `[INTERNAL]`, `[PUBLIC]`, or `[POST-BETA]`. | Marketing, store presence, localization, and accessibility are real planned work, not footnotes — but only the `[PUBLIC]` subset gates the public beta. |

**Sizing key** (used throughout):

| Size | Dev-sessions | ≈ Part-time calendar |
|---|---|---|
| **S** | 1–3 | under a week |
| **M** | 4–8 | 1–2 weeks |
| **L** | 9–20 | 2–5 weeks |
| **XL** | 21–40 | 5–10 weeks |
| **XXL** | 40+ | 10+ weeks |

**Phase numbering:** this plan uses a **`B`-series (B0–B12)**, deliberately *not* continuing the existing `P0–P10` numbering. The existing P-numbers are still referenced throughout `CLAUDE.md`, `GameDevPlan.md`, and the git log, so renumbering or inserting into that sequence would silently invalidate those references even though the `Docs/Phases/P0–P10` files themselves were deleted 2026-07-26 (fully superseded by `01_RevisionRegister_P0-P6.md` and this plan; recoverable from git history). §3.1 gives the old→new mapping.

---

## 1. Executive summary

### 1.1 What the audit found

The project is **further along than its phase plan suggests, and less verified than its architecture docs suggest.** Those two facts drive every structural decision below.

- **P0–P6 are all built in code.** Camera/control, needs simulation, 4-zone health + wound/infection/treatment, zombie AI with BT/Blackboard/perception/noise, unified attack dispatch + hotbar + per-weapon melee + durability, and a full inventory/container/loot-table/world-pickup backbone all exist as compiling C++.
- **Almost none of P5 or P6 has ever run.** Per `SessionHandoff.md` (2026-07-22), the *first real PIE confirmation of anything* built across the last several sessions happened on 2026-07-22 — and it covered exactly two things: the AnimBP rifle-pose fix and basic hotbar switching. The P5 melee/durability/knockback work, the entire P6 inventory system, and the weapon-config static-mesh restructure are **compiled, reviewed, and unrun.** P6 additionally has *zero authored content* — no item configs, no loot tables, no placed containers.
- **There is a known-unsound data model underneath the newest code.** `Docs/Planning/InventoryLoadoutEquipping_Plan.md` documents it precisely: three incompatible notions of "an item" coexist, `HotbarSlots` holds bare `UZSWeaponConfig*` rather than referencing anything the player actually carries, durability lives on the `AZSWeapon` actor and is therefore destroyed on every unequip, and ammo is not an inventory item at all. This gets **more expensive to fix with every data asset authored against today's shape.**
- **Entire beta-necessary disciplines have no phase at all.** UI/UX, art direction lock, audio, accessibility, settings/options, localization, QA workflow, release engineering, and store/marketing appear nowhere in P0–P10. The open-questions doc confirms this (§15, §16 are flagged as full gaps). The existing P10 ("production hardening → vertical slice") is a single medium phase carrying what is realistically five phases of work.
- **The consolidated changes are not purely additive.** They revise already-shipped behaviour in at least five places (see §2), add two new v1 needs, add a new firearms subsystem (jamming), add a new world subsystem (elevation/multi-level + darkness/light), and delete a shipped feature (perspective switching).

### 1.2 The five structural moves this plan makes

**Move 1 — Insert a mandatory stabilization phase (B0) before any new feature work.**
The single largest risk in the project right now is not a system that doesn't exist; it's the ~4 sessions of unverified, unrun code sitting under everything that comes next, resting on a data model its own planning doc says is wrong. B0 clears the PIE-verification debt, lands the item-instance refactor while it is still cheap, and applies the mechanical revisions the consolidated changes force onto P1–P6. **Nothing else in this plan should start first.** This is the highest-leverage phase in the document.

**Move 2 — Promote UI to a first-class phase (B1), placed early.**
P6's inventory has no UI, P2's moodles have no UI, P3's wounds have no UI, and P6's containers do "loot all" on interact specifically because no per-item UI exists. `Docs/Planning/UI_Plan.md` §2 identifies input-mode switching as blocking every modal screen — and `GameDevPlan.md` §7 cross-cutting Q6 flags the same thing about left-click's dual meaning. **You cannot meaningfully playtest survival systems you cannot see.** UI is not a polish item here; it is the instrument panel for every system already built.

**Move 3 — Split "world building & persistence" (old P7) into three phases.**
Old P7 bundled the art integration pass, the region build, enterable interiors, the top-down interior-visibility solution, save/persistence v1, and utilities shutoff into one phase. That is easily XXL and contains the project's highest technical risk. It splits into **B2 (art direction lock + asset pipeline)**, **B3 (persistence/save/streaming backbone — systems only, no content)**, and **B4 (region content build, interiors, elevation, light)**. Splitting lets the save architecture be de-risked against a graybox map *before* a single production asset is placed.

**Move 4 — Convert cross-cutting disciplines into continuous tracks, not phases.**
QA, build/release pipeline, marketing, content authoring, performance profiling, and documentation do not have a start and an end — they run alongside. Modelling them as phases would produce a false sequence. They are specified in `T_ContinuousTracks.md` with per-phase entry points. Critically, **performance profiling starts at B0, not at B8** — the consolidated changes explicitly require "profile early, don't retrofit efficiency later."

**Move 5 — Separate the two beta gates explicitly.**
`[INTERNAL]` = a closed group can play unsupervised for a full session without a developer present. `[PUBLIC]` = strangers can buy/download it and the studio can survive the support load. These have genuinely different requirements — crash telemetry, an options menu, remappable controls, and a store page matter for one and not the other. Every phase tags its deliverables.

### 1.3 Where this plan flags scope risk against the "1/3 depth" pillar

The prompt requires flagging any proposal that risks violating the deliberate-simplification pillar. Four flags, in descending severity, **as originally written — see the 2026-07-26 update below each.**

1. **Temperature + Wet + clothing insulation (Consolidated §2)** — 🚩 **HIGH.** This takes v1 needs from 6 to 8, and "clothing insulation value" implies a layering system that `GameDevPlan.md` §3 explicitly simplified away ("single outfit slot-set with protection values"). Temperature done properly is a weather model × an indoor/outdoor model × a clothing model × a wetness model × a time-of-day model. See `90_OpenQuestions.md` OQ-B0-04 for a scoped-down proposal.
   - **Update 2026-07-26:** dev-confirmed KEEP on all three, still via the scoped-down single-scalar model. Risk downgraded from "is this wanted" to "just execute the scoped-down version and don't let it creep back toward the full simulation described here."
2. **Basements/underground with randomized layout selection (Consolidated §7)** — 🚩 **MEDIUM-HIGH.** This is procedural level assembly plus a multi-level streaming/visibility solution plus a lighting mechanic, and it is described in one bullet. It is a genuinely new system with no phase home in the current plan.
   - **Update 2026-07-26:** ✅ **resolved, risk removed.** Dev cut the randomized-layout-selection system specifically — "will stick to a fixed map for now." Basements, if any exist, are ordinary authored content like any other room, not a procedural/weighted-pool system. No `UZSBasementLayoutConfig`, no layout pool.
3. **Elevation handling / multi-level buildings (Consolidated §1)** — 🚩 **MEDIUM.** "Fully automatic, system-driven" is the right call, but auto-detecting a character's floor and resolving aim rays against it is a real subsystem, and it interacts with camera occlusion, AI navigation, and noise propagation.
   - **Update 2026-07-26:** dev-confirmed KEEP, full system wanted. Risk itself unchanged — still a real subsystem — but now moved to Stage 1 (B4, systems-only on graybox) specifically so it gets built and tested small before B4X's larger, phased map has to rely on it.
4. **Firearm jamming (Consolidated §4)** — 🚩 **LOW.** Genuinely cheap given durability already exists. Flagged only because it adds a failure state that needs UI, audio, and a clear-jam action to be legible.
   - **Update 2026-07-26:** dev-confirmed KEEP, unchanged.

**New scope-risk flag, added 2026-07-26:** 🚩 **Region scale increased** (dev call — "bigger, build in phases") at the same time vehicles came back into scope (CR-02) and player count rose to 4+ (CR-08/§3.3). Each of these three individually raises B4X/B8's workload; together they compound. Mitigated structurally by making region content a phased continuous track (§3.2) rather than a single committed scope, and by giving vehicles their own scoping pass rather than folding them into an existing phase's estimate.

---

## 2. Contradiction Register

> **Superseded 2026-07-26.** Every entry below was originally written with an AI-authored recommendation ("Rec:") adopted as "CONFIRMED" in the same breath — not an actual decision from the dev. A full rescope pass (`Docs/Planning/RescopeQuestionnaire.md`, answered 2026-07-26) went through every one of these with the dev directly. Resolutions below are dated and dev-confirmed; several **reverse** the original AI-guessed resolution — CR-02 and CR-06 especially. Kept in the original per-row format for continuity, with a dev-answer row replacing the old "Resolution taken."

### CR-01 — Skill roster ✅ RESOLVED 2026-07-26 (dev-confirmed)

| | |
|---|---|
| **Consolidated Changes §9** | "…each of the six v1 skills (Melee, Firearms, Fitness, Medicine, Carpentry, Survival)" |
| **GameDevPlan §3.1** (revised 2026-07-19) | Attributes: Strength, Stamina, Sneak, Sprint. Skills: per-weapon-class Melee bars, Maintenance, Aiming, Reloading, First Aid. **Carpentry → "Building" and Survival → "Foraging/Cooking" are explicitly DEFERRED post-v1.** Fitness is dissolved into the Strength/Stamina attributes. |
| **Assessment** | The six-skill list is the **2026-07-18** roster that §3.1 explicitly says it "supersedes." The consolidated doc is dated July 2026 but is quoting the older decision, not making a newer one. |
| **Dev answer** | **"Use the GameDevPlan.md list (longer one)."** §3.1's revised model is correct; the six-flat-skills list is dead. The consolidated doc's two other decisions in §9 (no skill decay, per-skill XP rate exposed as a tunable) still stand — they apply cleanly to either roster. |

### CR-02 — Vehicles ✅ RESOLVED 2026-07-26 (dev-confirmed) — REVERSES the prior AI resolution

| | |
|---|---|
| **Consolidated Changes** | §1 lists "driving (vehicle, future)" as an auto-zoom trigger; §5 lists "Vehicle storage" as one of four v1 container categories. **Open Questions §6** asks about vehicle combat design (running over zombies, vehicle damage, noise). |
| **GameDevPlan §3** | `Vehicles (§15) — **CUT v1, plan later** — Deferred to its own planning pass.` |
| **Assessment** | §1's "(vehicle, future)" annotation is self-consistent with CUT. §5's container list is not — a container category for a thing that doesn't exist is either forward-compat scaffolding or an unflagged scope addition. |
| **Dev answer** | **Not cut.** "We will implement vehicles later on in development, should be ready for beta." Vehicles get a dedicated Stage-2 phase (§3 below), scheduled after the Stage-1 core playable loop, but landed **before** beta ships — not an indefinite deferral. `EZSContainerType`'s reserved `Vehicle` value and the auto-zoom `Driving` context go from permanently-unused scaffolding to genuinely scheduled work. **Consequence: raises the map-scale decision** — see the updated OQ-B4-01 and §3's map-scale note below; a 1×1 km map is too small once vehicles are real. |

### CR-03 — Temperature and Wet in v1 ✅ RESOLVED 2026-07-26 (dev-confirmed)

| | |
|---|---|
| **Consolidated Changes §2** | Wet added to v1 (binary flag, ties into footstep noise). Temperature "pulled forward from deferred pool into active scope" — hot/cold, hypothermia risk. v1 needs list is now **8**: Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness, Wet, Temperature. |
| **GameDevPlan §3** | Moodles: "**6 moodles v1**." Seasons/weather/temperature: "SIMPLIFY — Day/night + rain/fog v1." `§7 P2 Q4` — "Weather: real mechanics in v1 or atmospheric-only?" was **open**. |
| **Assessment** | This is a genuine, newer decision and it wins. It also answers §7 P2 Q4 (real mechanics, not atmospheric-only) and it *forces* weather to become a gameplay system rather than a visual one. |
| **Dev answer** | **Keep all three (Wet, Temperature, clothing insulation)** — "was part of the plan," not scope creep to walk back. The scoped-down model (single body-temp scalar, four inputs, no per-limb thermal, no layering) still stands as *how* it's built — now dev-confirmed rather than AI-recommended. Dev also directly reaffirmed the governing pillar: needs should create real worry without survival micromanagement becoming "the main thing" — tune decay/thresholds forgiving, not punishing; the failure mode to avoid is players frustrated by babysitting hunger/thirst. |
| **Downstream impact** | `UZSNeedsComponent` +2 needs · `UZSItemConfig` gains insulation value · footstep audio gains a wet variant · weather actor becomes gameplay-authoritative and must replicate · moodle UI grows from 6 to 8 slots (design the container for N, not 6). |

### CR-04 — Over-the-shoulder camera ✅ RESOLVED 2026-07-26 (dev-confirmed) — changes the mechanism, not the outcome

| | |
|---|---|
| **Consolidated Changes §1** | "Removed entirely: ToggleCameraPerspective / IA_ToggleView input action. Multi-perspective camera enum (no OverShoulder mode, no perspective switching)." |
| **GameDevPlan Decision 1 / §6 risk table** | Perspective list cut to "TopDown + OverShoulder (over-shoulder kept as an aim-zoom **and as a hedge**)." §6's risk row: *"Top-down doesn't feel right → P1 is a cheap identity gate before art spend; **over-shoulder TP is the fallback**."* |
| **Assessment** | Newer decision, and it wins — but it **deletes the documented mitigation for a named project risk**, and the P1 exit criterion it hedged ("this is the go/no-go gate on Decision 1") was never formally signed off. |
| **Dev answer** | **Cut now, not gated behind a sign-off checkpoint.** Dev's own reference framing makes clear top-down is the real direction — PZ-like camera feel, DK2's steeper angle purely for visual differentiation, not a gameplay hedge ("PZ has a good camera feel, but want it slightly different to avoid copying"). `B0-PT2` still runs as a **feel/tuning checkpoint** — verify aim-cone, zoom range, readability — but its failure mode changes: a bad result means *re-tune top-down*, not *revert to a different camera system*. Delete `ToggleCameraPerspective`/`IA_ToggleView`/`EZSCameraPerspective` in B0 without gating the deletion on a pass/fail result. Dev also directly confirmed the proposed aim-cone tightness ("about right") and the ~5%/25% hip/aimed headshot split (KEEP) — treat those as dev-approved starting numbers, not guesses. |

### CR-05 — Bandage simplification ✅ CONFIRMED, unaffected by the rescope pass

Bandage persists until the wound heals; no dirty-bandage decay. The wound's own `bDirty` flag stays (drives wound-infection risk, target of `Server_Disinfect`) — only the bandage-degrades-over-time behavior is cut. No change from the original resolution.

### CR-06 — Infection legibility ⚑ REVERSED 2026-07-26 (dev-confirmed) — the single biggest content change from the rescope pass

| | |
|---|---|
| **Original design** | Two infection tiers — wound infection (curable, never fatal alone) and bite infection (not curable by disinfecting, fatal timeline) — designed to be **deliberately indistinguishable in the UI**, a named identity pillar ("is this a cold or am I dying?"). |
| **Dev answer** | **"Plainly show the player if they are bitten and infected."** The ambiguity is not wanted — clear, legible feedback instead. The player should know they were bitten, and know when an infection (either tier) is active. |
| **What survives unchanged** | The two-tier *mechanical* model (wound infection vs. bite infection, different cure paths, bite infection's ~3-in-game-day fatal timeline — confirmed "feels right" — and amputation as the escape valve). Only the *legibility* requirement flips. Multi-day fracture healing and a rare critical head-bleed outcome both confirmed KEEP too. |
| **What this changes downstream** | `GameDevPlan.md` §1's "Infection ambiguity" pillar (already updated). B0-T6.3 and B1-T3.3 in the per-phase files currently say the opposite of this — update when touched. `OQ-B0-07`'s entire premise is moot. **Flagged prominently since it reverses a named pillar** — nothing has been built against it yet, so it reverts easily if this was a misread. |

### CR-07 — Save topology & death/world-continuity ✅ RESOLVED 2026-07-26 (dev-confirmed) — merged with CR-12, simpler than either prior proposal

| | |
|---|---|
| **Consolidated Changes §7/§11** | Layered: ~10s character-state saves · periodic full-world save · chunk saves on unload (World Partition-friendly) · clean save on graceful shutdown · **rotating backup slots (last 2–3), not single overwrite.** |
| **GameDevPlan §7 cross-cutting Q5** | "Save architecture: one world/save per server, or multiple concurrent slots?" — **still open.** §6 risk table asserts "listen-server-host-owns-the-save, single world save." |
| **Tension** | An old backlog note said solo character death should end the *entire world* (fresh world + fresh character) while co-op continues unless the whole party dies — an asymmetric rule never actually confirmed, just carried as an open question (`OQ-B3-01`). Save topology (one world per slot vs. multiple slots) was separately unresolved. |
| **Dev answer, death rule** | **No asymmetric rule.** Death — solo or co-op — always respawns a fresh character into the **same persistent world**. Loot stays where it dropped; any base/safehouse remains reachable. Simpler than both the original backlog note and the "multiple world slots" proposal. |
| **Dev answer, save topology** | **One continuously-overwritten world save**, not multiple slots — "so player can't load an old save to fix a mistake," no player-facing rollback/save-scumming. **Rotating backups for crash/corruption recovery are a separate concern and stay** — those exist to survive a bad write, not to let a player undo a decision. |
| **Consequence** | `Server_RespawnAsNewCharacter`'s current always-same-world behavior was already right — this removes a planned special-case rather than forcing a code change. `AZombieCharacter` death-triggered spawning (the "you become a zombie" backlog item) is unaffected and still scheduled (B0-T9). |

### CR-08 — Zombie perception fixed-per-type & horde scale ✅ CONFIRMED, and ambition raised

| | |
|---|---|
| **Consolidated Changes §6** | Senses fixed per zombie TYPE via `UZSZombieConfig`, never randomized per-individual. Search-last-known-location added to the BT. Horde coordination: no Rally Leader committed — *"whatever system best supports efficient large-horde processing performance-wise should drive the design."* |
| **GameDevPlan §7 P4 Q1** | "Is ~150 concurrent on-screen zombies the right target?" — open. |
| **Assessment** | No contradiction, but the consolidated doc explicitly **subordinates a design decision to a performance measurement that has not been taken.** That is a dependency, and it means horde design is blocked on profiling. |
| **Resolution** | Fixed-per-type **CONFIRMED**, unaffected. Horde-coordination *design* stays gated on B0's profiling baseline (unchanged). **What changed:** the dev confirmed a genuinely large, visually distinct horde (100+ zombies) is **important to the vision**, not a number to trade away under performance pressure. Treat ~150 concurrent as a floor to engineer toward, not a stretch goal to quietly lower — see updated `GameDevPlan.md` §7 P4. |

### CR-09 — Container categories ✅ CONFIRMED, unaffected by the rescope pass

Four categories (on-person, bag, world, vehicle-reserved) — dev confirmed KEEP on all four in the item-instance-refactor section of the questionnaire. Vehicle category moves from "reserved, no plan" to "reserved, scheduled" per CR-02.

### CR-10 — Fatigue reduces the player's own perception ✅ RESOLVED 2026-07-26 (dev-confirmed)

Confirmed reading (A): a tired *player* perceives worse (presentation degradation — vignette, muffled audio), not "harder for zombies to spot." Exactly the originally-assumed reading — now a real decision instead of an AI recommendation.

### CR-11 — Panic deferred, aim-cone is the sole combat-pressure source ✅ CONFIRMED, tuning validated

Unaffected — see CR-04 above for the dev's direct confirmation of the aim-cone tightness and headshot-weighting numbers.

### CR-12 — Death/world-continuity backlog — merged into CR-07 above

The full P3 backlog this row covered (player-becomes-zombie, loot-at-death-location, amputation blackout/time-skip, one-handed restriction, medical-tier incubation delay) keeps its original phase assignments (B0-T9, B0-T7, B0-T2, B0-T6) — only the party-wipe/solo-world-ending piece changed, and it's now resolved under CR-07, not left open as `OQ-B3-01`.

---

## 3. Revised master phase list

### 3.1 Old → new mapping

| Old (GameDevPlan §4) | Status | New home |
|---|---|---|
| P0 Close-out & re-aim | ✅ Complete | — (historical) |
| P1 Camera & control | ✅ Built | Revisions → **B0** (`01_RevisionRegister_P0-P6.md`) |
| P2 Survival core | ✅ Built | Revisions → **B0** |
| P3 Health/damage/medical | ✅ Built | Revisions → **B0** |
| P4 Zombies | ✅ Built | Revisions → **B0**; horde perf → **B7** |
| P5 Loadout & combat | ⚠ Built, largely unverified | Verification + revisions → **B0** |
| P6 Inventory & loot | ⚠ Built, unverified, no content | Verification + refactor → **B0**; UI → **B1**; content → **T4** |
| P7 World building & persistence | Not started | **Split**: art pipeline → **B2**, save/streaming → **B3**, region content → **B4** |
| P8 Events & investigation arc | Not started | **B5** |
| P9 Meta-loop, onboarding & difficulty | Not started | **B6** |
| P10 Production hardening → vertical slice | Not started | **Split**: audio → **B7**, perf → **B8**, a11y/settings → **B9**, MP/release → **B10**, beta gates → **B11/B12** |
| *(none)* | — | **B1** UI/UX — new, no prior home |

### 3.2 The B-series — restructured 2026-07-26 into two stages

> **Why a two-stage split.** The dev's rescope answer to "what's the real near-term goal" (`RescopeQuestionnaire.md` §1.2/1.3) was explicit: not the full public-beta ambition right now — **a playable game where every core feature actually works, rough edges and all, tested incrementally as it's built.** Full breadth is still the target (almost nothing here got cut — see the Contradiction Register above), but the *order* changes: get the mechanical core loop fully working and tested before sinking time into content volume, narrative writing, art polish, audio, and release engineering. **Stage 1** is that core loop. **Stage 2** is everything that makes it a full game and eventually a shippable beta. B-numbers are kept stable throughout (no renumbering) so every existing `B4-T7`/`OQ-B5-01`-style cross-reference still resolves correctly; two phases get a lineage-suffix instead of a new number (`B4X`, `BV`) to slot into the existing scheme without colliding.
>
> **This also reflects the dev's process answers directly**, not just content ones: checkpoint after each feature (not phase-end only), ask before implementing anything design-shaping, minimize cascading dependency between steps, written test scripts the dev runs personally. Every phase file gets re-cut to that shape as it's opened — see `B0_Stabilization.md` for the first one done this way.

#### Stage 1 — Core Playable Loop

Everything here is systems-only, tested on the existing graybox/test level — not content volume, not narrative, not polish. Exit signal: **a full day/night cycle, solo or co-op, where every core system (needs, health/infection, combat, zombies incl. a real horde test, inventory/items, multi-level buildings, darkness, weather) is playable end-to-end, rough but working**, per the dev's own description of what "real progress" looks like right now.

| # | Phase | Size | Sessions (rough) | Gate |
|---|---|---|---|---|
| **B0** | Stabilization & Reconciliation *(in progress)* | **L** | 14–18 | — |
| **B1** | UI/UX Foundation, HUD & Input Modes | **L** | 14–18 | `[INTERNAL]` |
| **B3** | Persistence & Save Backbone *(systems only — simplified per CR-07)* | **L** | 14–18 | `[INTERNAL]` |
| **B4** | World Systems: Multi-Level, Darkness/Light, Weather-as-Mechanic *(small graybox area, not the real map)* | **M–L** | 18–24 | `[INTERNAL]` |
| **B6-Sys** | Progression Framework *(generic, data-driven skill/attribute/background system — no roster content yet)* | **S–M** | 6–8 | `[INTERNAL]` |
| | **Stage 1 exit — "Core Loop Playtest"** | | | milestone, not a beta gate |

#### Stage 2 — Content, Depth & Release

Everything that turns the working core loop into a full, shippable game. Several former single-phase items become **continuous tracks** instead (see `T_ContinuousTracks.md`), specifically to avoid the "one giant phase, one checkpoint at the end" problem this whole rescope exists to fix.

| # | Phase | Size | Sessions (rough) | Gate |
|---|---|---|---|---|
| **B2** | Art Direction Lock & Asset Pipeline | **M** | 6–8 | `[INTERNAL]` |
| **B4X** | Region Content Build-Out *(continuous track, phased district-by-district — see `T_ContinuousTracks.md` T7)* | ongoing | — | `[INTERNAL]`→`[PUBLIC]` |
| **B5** | Dynamic Events, Radio & Investigation Arc | **L** | 18–22 | `[PUBLIC]` |
| **B6-Content** | Backgrounds Roster, Narrative Flavor & Onboarding Content | **M** | 8–10 | `[PUBLIC]` |
| **BV** | Vehicles *(new phase — see CR-02)* | **L** *(estimate, unscoped)* | 12–18 *(placeholder)* | `[PUBLIC]` |
| **B7** | Audio Production & Horde-at-Scale AI | **L** | 14–18 | `[PUBLIC]` |
| **B8** | Performance, Profiling & Optimization *(budget raised: 4+ players, bigger map, real large hordes — see CR-08)* | **L** | 12–18 | `[INTERNAL]` |
| **B9** | Accessibility, Settings & Sandbox Options | **M** | 8–10 | `[PUBLIC]` |
| **B10** | Multiplayer Hardening & Release Engineering *(4+ players, optional paid dedicated-server path — see updated `GameDevPlan.md` §3)* | **L** | 16–20 | `[PUBLIC]` |
| **B11** | Internal Closed Beta | **M** | 6–8 | `[INTERNAL]` |
| **B12** | Public Beta / Early Access Readiness *(EA confirmed, ~$9.99 target, Steam-only first)* | **L** | 12–16 | `[PUBLIC]` |

> **On the session totals.** Deliberately not re-adding these to a new single "~N months" headline number. The dev's own process answers (checkpoint after each feature, ask before implementing specifics, avoid long interdependent chains) mean the real pacing driver from here is checkpoint count and how often a check-in is needed, not a session-count estimate — treat every number above as a rough planning input, re-forecast at each phase's own checkpoints (as `B4X`'s district-by-district structure already forces), not a commitment. **BV (Vehicles) has no real estimate yet** — it needs its own design/scope pass when Stage 1 is done, the same way B4 originally needed one before this rescope; don't let a placeholder number harden into an assumption.

### 3.3 Scope boundaries per phase

Full task breakdowns live in the per-phase files. This table is the **scope contract** — if something is in the OUT column and starts happening, that is scope creep and should be caught in review.

| Phase | Explicitly IN | Explicitly OUT |
|---|---|---|
| **B0** | PIE-verify everything built in P5/P6 · item-instance/GUID refactor, done as independently-testable steps (dev preference) · ammo-as-inventory-item · handedness fields · camera revisions (perspective removal — cut now, not gated on a sign-off — zoom presets, aim-cone, elevation *stub*) · Wet + Temperature needs · two-tier infection **with plain/legible UI feedback (reversed from ambiguous — CR-06)** · bandage simplification · jamming · downed-zombie finishers (needs a non-PZ-clone variant, per dev note) · BT search-last-known · death→zombie + loot-at-death, unified world-persists rule · melee display resolved (grouped poses by weapon category) · profiling baseline | No new UI (B1) · no real multi-level geometry (B4) · no weather visuals (B4) · no save (B3) · no horde-coordination redesign (B7) · no stat-affecting attachments (now planned, but later — see B4X/weapon-depth track) |
| **B1** | Input-mode/`IMC_ZS_UI` switching · HUD (needs, health/wounds, ammo, hotbar, interaction prompt) · inventory screen **with separate equipment-drag-slots + a general carry container (dev preference, not a single flat list)** · container loot screen · death/respawn screen · sleep prompt · main menu + pause · moodle system for N needs, shown plainly per CR-06 | Radial quick-use (B9) · map screen (B4X) · character creation UI (B6-Content) · settings menu (B9) · localization (B12) · final art pass on UI (B2/B7) |
| **B3** | Save topology (unified, single continuously-overwritten world, no player-facing rollback — CR-07) · `UZSSaveGameSubsystem` · layered save (10s character, periodic world, chunk-on-unload, shutdown) · rotating **corruption-recovery** backups (not save-scumming) · World Partition setup + streaming policy on graybox · corpse/item pooling + dual-limit cleanup | Region content (B4X) · late-join flow (B10) · cloud saves (post-beta) · save migration/versioning beyond a version stamp (B12) |
| **B2** | Art direction lock (mood board, palette, reference set) · modular kit selection — **modest/free-first budget, DK2 as the fidelity bar, self-modeled assets expected later** · material/shader standards · LOD + collision + naming conventions · retarget pipeline validation · one fully-dressed reference room as the quality bar | Building the region (B4X) · character art (B6-Content) · VFX polish (B7) · UI art (B1 owns layout, B2 owns tokens) |
| **B4** | (Stage 1, systems only) **elevation/multi-level system** · **darkness + light-source mechanic** · weather system (rain/fog/snow) as real gameplay · day/night, all built and tested on the existing small graybox area | Real region content/scale (B4X) · **procedural/randomized basement layouts (CUT — dev wants a fixed authored map for now, not a layout-selection system)** · vehicles (BV) · farming (OQ-B4-06) · seasons (post-beta) |
| **B4X** | (Stage 2, continuous track) Building out the **larger, phased** region on the real kit · enterable interiors, fixed/authored (no procedural basements) · utilities shutoff on the real map · map screen · spawn points · generic placeholder names for locations until B6-Content/B5 need real ones | Named-location *narrative* content (B5 supplies it, B4X just builds the space) · events (B5) |
| **B5** | `UZSEventDirector` · repeatable event roster · per-event radio warning treatment · radio broadcast arc · investigation clue system + journal/tracking UI · guaranteed clue placement · radiant objectives · the actual plot (still open — dev is "still planning," brainstorm together when this phase starts) | Voice acting (OQ-B5-05) · branching dialogue · NPC survivors (POST-BETA) · the capstone's post-completion world modifier (POST-BETA) |
| **B6-Sys** | (Stage 1) Generic skill/attribute component + config assets, XP hookup plumbing, background *data structure* (assignable starting values per type) — no authored roster yet | Any specific background names/flavor (B6-Content) |
| **B6-Content** | (Stage 2) Background roster **with real tradeoffs, not purely additive** (dev confirmed) · XP curves + per-skill rate tunable · character creation flow + appearance · new-game setup · first-hour onboarding pass | Skill decay (CONFIRMED cut) · perks/unlocks (OQ-B6-03) · deferred skills — Fishing/Building/Foraging/Cooking/Mechanics (POST-BETA) |
| **BV** | Vehicle actor(s), fuel/damage/hotwire-or-key model (TBD, own design pass), vehicle combat feel, vehicle storage (the reserved `EZSContainerType::Vehicle` becomes real) | Scheduling itself — this phase needs its own scoping pass before a task breakdown exists, same treatment B4 got before this rescope |
| **B7** | Ambient beds per biome/interior/time-of-day · zombie vocalization set (including a "freshness" audio read if the zombie-degradation mechanic lands here) · weapon/melee/impact SFX · footstep surface + wet variants · UI SFX · music direction + implementation · audio occlusion/attenuation policy · **horde coordination + large-group AI — genuinely large hordes (100+) are a confirmed priority, not a cuttable stretch goal** | VO (OQ-B5-05) · adaptive/vertical music (POST-BETA) · full Wwise/FMOD migration unless OQ-B7-02 says otherwise |
| **B8** | Fixed stress-test map/scenario · profiling on packaged Development builds · zombie-count budget lock (engineer toward the target, don't lower it first) · draw-call/material consolidation · AI tick budgeting + LOD · network bandwidth pass **re-baselined for 4+ players** · memory/GC pass · min-spec decision (dev wants to "keep the game light") | Console optimization (POST-BETA) |
| **B9** | Settings menu (video/audio/gameplay/controls) · full control remapping · gamepad support pass · colorblind modes · subtitle/text-size options · difficulty options · the XP-rate tunable surfaced | Full PZ-style sandbox slider suite (POST-BETA) · screen-reader support (POST-BETA) · localization (B12) |
| **B10** | Late-join flow · disconnect/reconnect character handling · network stress + packet-loss testing · direct-IP/LAN hardening (primary) · **optional paid dedicated-server hosting path (new — dev confirmed)** · packaged build pipeline · crash reporting/telemetry · versioning + release checklist | Host migration (still not planned) · voice chat (still relying on Discord) · cross-platform (POST-BETA) |
| **B11** | Closed-group beta with 6–12 testers · structured feedback + bug intake · tuning passes from real data · balance of ammo/loot/zombie density from telemetry | Public marketing · store page · press |
| **B12** | Steam page + capsule art + trailer · **Early Access confirmed, ~$9.99 target price, Steam-only at first** · localization pass · public bug-report pipeline · community channels · launch checklist | Post-launch roadmap content · DLC |

---

## 4. Dependency map

### 4.1 Hard blocking chain

```
STAGE 1 — CORE PLAYABLE LOOP

B0 Stabilization  ──────────────────────────────────────────────┐
  │  (item-instance model, verified combat/inventory,           │
  │   camera model locked, needs complete, profiling baseline)  │
  ├──> B1 UI/UX ──────────────────────────────────┐             │
  │      (input-mode switching, HUD, inventory)   │             │
  ├──> B3 Persistence (systems) ───────────────────┤             │
  ├──> B4 World Systems (graybox: multi-level,     │             │
  │      darkness, weather) ───────────────────────┤             │
  └──> B6-Sys Progression Framework ───────────────┘             │
                                                                 v
                              ══ STAGE 1 EXIT: CORE LOOP PLAYTEST ══
                                                                 │
STAGE 2 — CONTENT, DEPTH & RELEASE                              v
  ├──> B2 Art Lock ──> B4X Region Content (continuous) ─────────┤
  │                              │                              │
  │                              v                               │
  │                    B5 Events/Investigation                   │
  │                              │                              │
  ├──> B6-Content Backgrounds/Narrative ──────────┐              │
  │                                               │              │
  ├──> BV Vehicles (own scoping pass first) ──────┤              │
  │                                               v              v
  │                                   B7 Audio + Horde-at-Scale ─┤
  │                                               │              │
  │                                               v              │
  │                                   B8 Performance (re-baselined:│
  │                                     4+ players, bigger map)   │
  │                                               │              │
  │                                               v              │
  │                                   B9 A11y/Settings            │
  │                                               │              │
  │                                               v              │
  │                                   B10 MP Hardening + Release  │
  │                                               │              │
  │                                               v              │
  │                                   B11 INTERNAL BETA GATE      │
  │                                               │              │
  │                                               v              │
  └────────────────────────────────>  B12 PUBLIC BETA GATE
```

### 4.2 Why each edge exists

| Edge | Reason it is a hard dependency |
|---|---|
| B0 → everything | The item-instance model is the substrate for inventory UI, save serialization, loot, and crafting. Building any of those against today's model means building them twice. |
| B0 → B1 | An inventory screen must render *something*. Today `CarrySlots` has no stable per-item identity to bind a widget to, and no drag/drop target that survives a move. |
| B1 → B4X | Region content cannot be playtested without a HUD showing needs/health, or a map screen. You would be building a world you cannot evaluate. |
| B2 → B4X | Placing production geometry before the kit, material standards, and LOD/collision conventions are locked means re-doing placement. This is the classic solo-dev rework trap. |
| B3 → B4X | World Partition setup and the chunk-on-unload save hook must exist *before* the region is streamed, or every cell gets revisited. De-risk save against graybox — this is exactly what B4 (Stage 1, systems-only, graybox) already does before B4X starts. |
| B0 → B3 | Save serialization needs a stable item identity (`FGuid`) and a settled needs list. Serializing 6 needs then adding 2 means a save-version migration during beta. |
| B4X → B5 | Events need real locations to fire at; clue placement needs real containers/buildings; radio needs a map to reference. |
| B0+B4X → B6-Content | XP hookup touches every gameplay system, so those systems must be final. Backgrounds pick spawn points, which need the real map. B6-Sys (the generic framework) has no such dependency and runs in Stage 1. |
| B4X → B7 | Ambient audio design is per-biome/per-interior; you cannot author beds for spaces that don't exist. |
| B0 → B8 (baseline only) | Profiling **starts** at B0 (per CONFIRMED "profile early") but the dedicated optimization phase needs final content — and the new 4+/bigger-map/real-horde targets — to optimize against. |
| B8 → B10 | Network hardening measures against a known-good frame budget; optimizing after network work invalidates the network measurements. |
| B1+B9 → B11 | Testers without a settings menu or remappable controls generate noise-bugs about hardware/preference, drowning real findings. |
| Stage 1 exit → BV's scoping pass | Vehicles get their own design/scope session once the core loop works, the same treatment B4 got before this rescope — no task breakdown exists yet on purpose. |

### 4.3 Parallelizable work (safe to interleave when blocked)

- **T4 content authoring** (weapon/item/loot data assets) — from end of B0 onward, continuously.
- **B2 art lock** can run entirely in parallel with **B1** and **B3** — different disciplines, no shared files.
- **B9's control remapping** can start any time after B0 (Enhanced Input is already in place).
- **T3 marketing/community** should start at **B4X**, not B12 — see `T_ContinuousTracks.md` for why the timing matters.
- **OQ resolution** is always parallelizable and should be batched into design sessions rather than blocking implementation sessions.
- **BV's scoping pass** can happen any time Stage 1 is winding down — it doesn't need to wait for Stage 2's other phases.

### 4.4 Highest-risk items, de-risk early

| Risk | Where it bites | Prototype/spike at |
|---|---|---|
| Multi-level interiors + top-down camera occlusion | B4 (would be a rebuild if wrong) | **B0-T3 spike**, then B4-T1, on graybox before B4X ever starts |
| Save/load correctness under co-op + World Partition | B3 (data loss in beta = fatal) | **B3-T1 spike** on graybox |
| Zombie count vs. frame budget, now at a higher bar (4+ players, real large hordes) | B8 (may force a design change to horde behaviour) | **B0-T12 baseline**, re-baselined B8 against the raised targets |
| Item-instance refactor breaking shipped behaviour | B0 (touches 5 files of live code) | B0-T1 verification pass *first*; refactor itself now split into independently-testable steps per dev preference |
| Live Coding Blueprint corruption during heavy C++ churn | B0 specifically — the highest-C++-churn phase in the plan | See `CLAUDE.md` lesson; B0-T0 sets a full-rebuild policy |
| Map going bigger (dev call) without a re-validated build-time estimate | B4X, phased content track | Build **one** district to the B2 quality bar first, time it, re-forecast before committing to the next — same discipline the old B4 already recommended, now load-bearing since scale went up |

---

## 5. Revised risk register

Replaces `GameDevPlan.md` §6 for beta-scope purposes. Updated 2026-07-26 post-rescope.

| Risk | Severity | Mitigation |
|---|---|---|
| **Unverified code accumulates faster than it is tested** — the failure mode that started this whole rescope; 4+ sessions of unrun code shipped before the first PIE confirmation | **HIGH** | B0 is a hard verification gate, being restructured into per-feature checkpoints (dev preference, checkpoint size = "A", after each individual feature/fix). New standing rule: no phase may exit with unverified deliverables; `SessionHandoff.md` stays the sole owner of verification status. |
| **Region content build overruns** — moved from "B4 is XXL and might overrun" to "B4X is a continuous track and the map just got bigger" | **HIGH** | District-by-district: build *one* fully-finished district to the B2 quality bar, playtest it, time it, re-forecast before starting the next. The phased/incremental approach is now structural (a continuous track), not just a recommendation. |
| **Save/persistence data loss in co-op** | **HIGH** | Rotating corruption-recovery backups + save-corruption soak test in B3 + versioned save format from day one. Simplified further by the unified death rule (CR-07) — one world, one continuously-overwritten save, less state-machine complexity than the old asymmetric solo/co-op rule would have needed. |
| **A named identity pillar got reversed (CR-06, infection legibility) without anything built against it yet** | **MEDIUM-HIGH** | Caught early specifically *because* this rescope pass happened before B0-T6 was implemented. Flag any doc still describing "ambiguous infection" as stale when touched. |
| **4+ players / bigger map / real large hordes raises the performance bar versus the original 2–4/1×1km/~150 assumptions** | **MEDIUM-HIGH** | B8's budget gets re-baselined against the new targets, not the old ones. Engineer toward the target zombie count before considering lowering it (CR-08) — the dev was explicit that a real horde matters to the vision. |
| **Live Coding Blueprint corruption** during B0's heavy C++ churn | **MEDIUM** | Full rebuild over Ctrl+Alt+F11 during B0; "Compile All Blueprints" pass after each patch cluster; check Output Log for `is not a child class of` / `invalid target type` first when anything behaves oddly. |
| **Vehicles (BV) and stat-affecting attachments are real scope additions with no task breakdown yet** | **MEDIUM** | Both explicitly deferred to their own scoping pass rather than bolted onto an existing phase's task list — same discipline that (belatedly) protected B4 originally. Don't let either harden into "just add it to B0/B5" without that pass. |
| **Open questions block implementation mid-phase** | **MEDIUM** | Every OQ is tagged BLOCKING/SEQUENCEABLE/LATE and grouped by phase; resolve a phase's BLOCKING set in one design session before the phase starts. Per the dev's process answer, also flag anything design-shaping *inside* a phase for a check-in before implementing, not just at phase boundaries. |
| **Scope pressure from the investigation arc** | **LOW-MEDIUM** | Decision 6 already resolved it as an optional capstone. The dev is still planning the actual plot — brainstorm together when B5 starts, don't let a placeholder plot harden into content. |

---

## 6. File index

| File | Contents |
|---|---|
| `00_MasterPlan.md` | **This file.** Assumptions, audit, contradiction register, phase list, dependency map, risk register. |
| `01_RevisionRegister_P0-P6.md` | Per-phase deltas the consolidated changes force onto already-built P1–P6 work. Feeds B0. |
| `B0_Stabilization.md` … `B12_PublicBeta.md` | Per-phase detail: entry/exit criteria, atomic tasks, data structures, dependencies, playtest checkpoints. |
| `T_ContinuousTracks.md` | QA, build/release, marketing, content authoring, profiling, docs — cross-cutting tracks with per-phase entry points. |
| `90_OpenQuestions.md` | Every undecided item, grouped by phase, with 2–4 options, tradeoffs, a recommendation, and a BLOCKING/SEQUENCEABLE/LATE tag. |
| `99_DefinitionOfBetaReady.md` | The concrete, testable checklists for the internal and public beta gates. |
