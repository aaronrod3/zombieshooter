# ZombieShooter — Master Beta Plan

> **Status: DRAFT v1.0, 2026-07-23.** Produced from `ZombieShooter_Consolidated_Changes.md` + `ZombieShooter_Open_Questions_For_Beta.md` audited against this repo's plan of record (`Docs/GameDevPlan.md` §1–§8, `Docs/Phases/P0–P10`, `Docs/SessionHandoff.md`, `Docs/Planning/*`).
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

**Phase numbering:** this plan uses a **`B`-series (B0–B12)**, deliberately *not* continuing the existing `P0–P10` numbering. The existing P-numbers are referenced throughout `CLAUDE.md`, `GameDevPlan.md`, all `Docs/Phases/` files, and the git log; renumbering or inserting into that sequence would silently invalidate those references. `Docs/Phases/P0–P10` remain untouched as build-state records for work already done. §3.1 gives the old→new mapping.

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

The prompt requires flagging any proposal that risks violating the deliberate-simplification pillar. Four flags, in descending severity:

1. **Temperature + Wet + clothing insulation (Consolidated §2)** — 🚩 **HIGH.** This takes v1 needs from 6 to 8, and "clothing insulation value" implies a layering system that `GameDevPlan.md` §3 explicitly simplified away ("single outfit slot-set with protection values"). Temperature done properly is a weather model × an indoor/outdoor model × a clothing model × a wetness model × a time-of-day model. See `90_OpenQuestions.md` OQ-B0-04 for a scoped-down proposal.
2. **Basements/underground with randomized layout selection (Consolidated §7)** — 🚩 **MEDIUM-HIGH.** This is procedural level assembly plus a multi-level streaming/visibility solution plus a lighting mechanic, and it is described in one bullet. It is a genuinely new system with no phase home in the current plan.
3. **Elevation handling / multi-level buildings (Consolidated §1)** — 🚩 **MEDIUM.** "Fully automatic, system-driven" is the right call, but auto-detecting a character's floor and resolving aim rays against it is a real subsystem, and it interacts with camera occlusion, AI navigation, and noise propagation.
4. **Firearm jamming (Consolidated §4)** — 🚩 **LOW.** Genuinely cheap given durability already exists. Flagged only because it adds a failure state that needs UI, audio, and a clear-jam action to be legible.

---

## 2. Contradiction Register

Per the source prompt: newer decisions win **unless** there is clear reason otherwise, and every conflict is surfaced for human review rather than silently resolved. Items marked **⚠ NEEDS YOUR CALL** are ones where I am not confident the newer document is actually the newer *decision*.

### CR-01 — Skill roster: the consolidated doc quotes a superseded list ⚠ NEEDS YOUR CALL

| | |
|---|---|
| **Consolidated Changes §9** | "…each of the six v1 skills (Melee, Firearms, Fitness, Medicine, Carpentry, Survival)" |
| **GameDevPlan §3.1** (revised 2026-07-19) | Attributes: Strength, Stamina, Sneak, Sprint. Skills: per-weapon-class Melee bars, Maintenance, Aiming, Reloading, First Aid. **Carpentry → "Building" and Survival → "Foraging/Cooking" are explicitly DEFERRED post-v1.** Fitness is dissolved into the Strength/Stamina attributes. |
| **Assessment** | The six-skill list is the **2026-07-18** roster that §3.1 explicitly says it "supersedes." The consolidated doc is dated July 2026 but is quoting the older decision, not making a newer one. |
| **Resolution taken** | Plan proceeds on **§3.1's revised model**. The consolidated doc's two *actual* decisions in §9 — no skill decay, per-skill XP rate exposed as a tunable — are CONFIRMED and carried forward; they apply cleanly to either roster. |
| **Why flagged** | If §9's list was a deliberate reversal back to six flat skills, B5 changes shape substantially (5 fewer skill bars, but Carpentry pulls base-building into v1 scope, which is an XL swing). **Confirm before B5 starts.** |

### CR-02 — Vehicles: assumed present in three places, CUT v1 in the scope contract ⚠ NEEDS YOUR CALL

| | |
|---|---|
| **Consolidated Changes** | §1 lists "driving (vehicle, future)" as an auto-zoom trigger; §5 lists "Vehicle storage" as one of four v1 container categories. **Open Questions §6** asks about vehicle combat design (running over zombies, vehicle damage, noise). |
| **GameDevPlan §3** | `Vehicles (§15) — **CUT v1, plan later** — Deferred to its own planning pass.` |
| **Assessment** | §1's "(vehicle, future)" annotation is self-consistent with CUT. §5's container list is not — a container category for a thing that doesn't exist is either forward-compat scaffolding or an unflagged scope addition. |
| **Resolution taken** | Vehicles stay **CUT for beta.** `EZSContainerType` gains a `Vehicle` enum value in B0 as **forward-compat only** (zero implementation cost, prevents a later enum migration); the auto-zoom context enum likewise reserves a `Driving` value. No vehicle actor, no fuel/repair/hotwire model, no vehicle combat. |
| **Why flagged** | Vehicles are the single largest deferred system that could plausibly be argued into beta. If they are wanted, that is an **XL phase on its own** and it changes map scale requirements (a 1×1 km map is small for driving). **Confirm before B4's region scale is locked.** |

### CR-03 — Temperature and Wet pulled into v1 🚩 SCOPE RISK

| | |
|---|---|
| **Consolidated Changes §2** | Wet added to v1 (binary flag, ties into footstep noise). Temperature "pulled forward from deferred pool into active scope" — hot/cold, hypothermia risk. v1 needs list is now **8**: Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness, Wet, Temperature. |
| **GameDevPlan §3** | Moodles: "**6 moodles v1**." Seasons/weather/temperature: "SIMPLIFY — Day/night + rain/fog v1." `§7 P2 Q4` — "Weather: real mechanics in v1 or atmospheric-only?" was **open**. |
| **Assessment** | This is a genuine, newer decision and it wins. It also answers §7 P2 Q4 (real mechanics, not atmospheric-only) and it *forces* weather to become a gameplay system rather than a visual one. |
| **Resolution taken** | **CONFIRMED, adopted.** 8 needs in v1. But scoped deliberately shallow — see OQ-B0-04 for the proposed minimal temperature model (single body temp scalar, four inputs, no per-limb thermal, no layering system). Weather is promoted from "atmospheric" to a real system in B4. |
| **Downstream impact** | `UZSNeedsComponent` +2 needs · `UZSItemConfig` gains insulation value · footstep audio gains a wet variant · weather actor becomes gameplay-authoritative and must replicate · moodle UI grows from 6 to 8 slots (design the container for N, not 6). |

### CR-04 — OverShoulder perspective deleted, removing P1's own risk hedge

| | |
|---|---|
| **Consolidated Changes §1** | "Removed entirely: ToggleCameraPerspective / IA_ToggleView input action. Multi-perspective camera enum (no OverShoulder mode, no perspective switching)." |
| **GameDevPlan Decision 1 / §6 risk table** | Perspective list cut to "TopDown + OverShoulder (over-shoulder kept as an aim-zoom **and as a hedge**)." §6's risk row: *"Top-down doesn't feel right → P1 is a cheap identity gate before art spend; **over-shoulder TP is the fallback**."* |
| **Assessment** | Newer decision, and it wins — but it **deletes the documented mitigation for a named project risk**, and the P1 exit criterion it hedged ("this is the go/no-go gate on Decision 1") was never formally signed off. |
| **Resolution taken** | **CONFIRMED, adopted.** Perspective enum and `IA_ToggleView` are removed in B0-T3. The §6 risk row is rewritten: the new mitigation is the **fixed-preset zoom range + auto-zoom** system, which delivers the "get closer to read detail" affordance over-shoulder was hedging for. |
| **Action required** | B0 must include an explicit **camera feel sign-off checkpoint** (B0-PT2) as the replacement gate. Do not delete the perspective code until that checkpoint passes — it is currently the only escape hatch. This is live code (`AZSPlayerCharacter::ToggleCameraPerspective`, `IA_ToggleView` on `V`, `EZSCameraPerspective`). |

### CR-05 — Bandage simplification contradicts shipped code

| | |
|---|---|
| **Consolidated Changes §3** | "A bandage persists and remains effective until the wound is fully healed. **No dirty-bandage decay mechanic, no re-bandage bleeding risk.**" |
| **Shipped code / CLAUDE.md** | `FZSBodyZoneWound` carries `bleed/**dirty**/splinted/amputated` flags. `GameDevPlan` P3: "Treatment actions: bandage (**cleanliness flag**), disinfect, splint." |
| **Assessment** | Newer decision, wins cleanly. This is a genuine simplification consistent with the 1/3-depth pillar. |
| **Resolution taken** | **CONFIRMED, adopted.** B0-T5 removes the dirty-bandage *decay* behaviour. **Note the nuance:** the `bDirty` flag on the *wound* stays (a wound can be dirty from the injury itself, which is what `Server_Disinfect` acts on and what feeds wound-infection risk) — what's removed is any notion of the *bandage* becoming dirty over time and needing replacement. These are two different things and the docs blur them. |

### CR-06 — Two-tier infection model refines, does not replace, shipped infection

| | |
|---|---|
| **Consolidated Changes §3** | Two tiers: **wound infection** (any injury, curable by disinfecting, slows healing, never fatal alone) and **bite infection** (zombie bite only, hidden roll, queasy→fever→death, interruptible only by amputation, never by disinfecting). |
| **Shipped code** | `UZSHealthComponent` has **one** infection concept: bite wounds roll a hidden chance, then `EZSInfectionStage` progresses None→Incubating→Queasy→Fever→Critical→death. There is no separate wound-infection track. |
| **Assessment** | Additive refinement, not a contradiction. The shipped model *is* tier 2; tier 1 is new. |
| **Resolution taken** | **CONFIRMED, adopted as additive.** B0-T6 adds a per-zone `EZSWoundInfectionState` distinct from `EZSInfectionStage`. Critically, the design intent is **ambiguity**: the player must not be able to tell from the UI which tier they have. See OQ-B0-07 — this has a direct, non-obvious UI consequence and it is easy to accidentally destroy the ambiguity by showing a clear moodle. |

### CR-07 — Save architecture specified, but cross-cutting Q5 still open

| | |
|---|---|
| **Consolidated Changes §7/§11** | Layered: ~10s character-state saves · periodic full-world save · chunk saves on unload (World Partition-friendly) · clean save on graceful shutdown · **rotating backup slots (last 2–3), not single overwrite.** |
| **GameDevPlan §7 cross-cutting Q5** | "Save architecture: one world/save per server, or multiple concurrent slots?" — **still open.** §6 risk table asserts "listen-server-host-owns-the-save, single world save." |
| **Assessment** | The consolidated doc specifies save *mechanics* thoroughly but not save *topology*. These are different questions and only the first is answered. |
| **Resolution taken** | Mechanics **CONFIRMED**. Topology remains open — see **OQ-B3-01 (BLOCKING for B3)**. |
| **Compounding factor** | `GameDevPlan.md` §7 P3's backlog contains an unresolved and load-bearing rule: *"Solo: death ends that world outright — a fresh world + fresh character."* That makes world lifetime a function of player count and party-wipe state, which the save topology must model. It also directly contradicts what `Server_RespawnAsNewCharacter` currently does. Folded into OQ-B3-01. |

### CR-08 — Zombie perception fixed-per-type vs. the ~150-concurrent target

| | |
|---|---|
| **Consolidated Changes §6** | Senses fixed per zombie TYPE via `UZSZombieConfig`, never randomized per-individual. Search-last-known-location added to the BT. Horde coordination: no Rally Leader committed — *"whatever system best supports efficient large-horde processing performance-wise should drive the design."* |
| **GameDevPlan §7 P4 Q1** | "Is ~150 concurrent on-screen zombies the right target?" — open. |
| **Assessment** | No contradiction, but the consolidated doc explicitly **subordinates a design decision to a performance measurement that has not been taken.** That is a dependency, and it means horde design is blocked on profiling. |
| **Resolution taken** | Fixed-per-type **CONFIRMED**. Search-last-known **CONFIRMED**, scheduled B0-T8. Horde coordination is deferred to **B7**, explicitly gated on the B0 profiling baseline. See OQ-B7-01. |

### CR-09 — Container categories: four named, two built

| | |
|---|---|
| **Consolidated Changes §5** | Four categories: on-person (pockets/worn, no bag required) · bags/backpacks (equipped bag slots) · vehicle storage · world containers. |
| **Shipped code** | `UZSInventoryComponent` has flat `CarrySlots` + two equip slots (`Back`, `Hip`) granting `CarryCapacityBonus`. `AZSContainerActor` covers world containers. **No on-person-vs-bag distinction** — a bag just raises one flat capacity number. |
| **Assessment** | Newer decision, wins. But it is a **real model change**, not a config change: today capacity is one scalar; the new model needs the *location* of an item to matter. |
| **Resolution taken** | **CONFIRMED, adopted.** Folded into B0's item-instance refactor (B0-T2) where it is cheapest — adding a container-location concept after content is authored is significantly worse. Vehicle category is enum-only per CR-02. |

### CR-10 — "Fatigue reduces zombie-detection perception" is ambiguous ⚠ NEEDS YOUR CALL

| | |
|---|---|
| **Consolidated Changes §2** | "**Fatigue/Tiredness:** At high severity, reduces zombie-detection perception (hearing/sight radius), tying fatigue into the noise/perception system." |
| **Assessment** | Two readings. **(A)** A tired *player* is worse at detecting zombies — but the player has no detection stat; the player detects zombies with their eyes on a screen. Implementing this means degrading the *presentation* (vignette, muffled audio, reduced camera range). **(B)** A tired player is *harder for zombies to detect* — which is backwards as a penalty, and would make exhaustion a stealth advantage. |
| **Resolution taken** | Proceeding on **reading (A)**, implemented as perceptual degradation (see OQ-B0-05 for three concrete options). Reading (B) is almost certainly not intended. |
| **Why flagged** | Cheap either way, but they are opposite mechanics. **Confirm before B0-T4.** |

### CR-11 — Panic: deferred in one place, load-bearing in another

| | |
|---|---|
| **Consolidated Changes §2 & §4** | Panic stays deferred. "Combat accuracy pressure governed **purely** by hip-fire cone/aim mechanics and weapon condition." |
| **Assessment** | Consistent and self-reinforcing. No contradiction — recorded because it makes the aim-cone model the *sole* source of combat pressure, which raises the stakes on getting its numbers right. |
| **Resolution taken** | **CONFIRMED.** Elevates aim-cone tuning (OQ-B0-02) from a tuning detail to a **BLOCKING** design question — it is now the only thing standing between the player and perfect marksmanship under stress. |

### CR-12 — `GameDevPlan` §7 P3 backlog: death/loot/world-continuity rules are unscheduled

| | |
|---|---|
| **Source** | `GameDevPlan.md` §7 P3 post-completion backlog (dev notes 2026-07-20), never scheduled into any phase: dead players **become zombies**; loot stays at the death location; co-op continues unless the **whole party** dies; solo death ends the world; amputation causes a **blackout with ~12h time acceleration**; arm amputation restricts to one-handed weapons; medical tier extends the incubation window. |
| **Assessment** | Not a contradiction with the consolidated doc — a set of confirmed dev intentions with **no phase home**, which is how features get silently lost. Several are load-bearing for save topology (CR-07) and for the P5 handedness fields. |
| **Resolution taken** | Explicitly scheduled: player-becomes-zombie → **B0-T9**; loot-at-death-location → **B0-T9**; party-wipe/world-lifetime rules → **B3** (OQ-B3-01); amputation blackout + time skip → **B0-T7**; one-handed restriction → **B0-T2** (needs the handedness field from the item-instance refactor); medical-tier incubation delay → **B0-T6**. |

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

### 3.2 The B-series

| # | Phase | Size | Sessions | Gate |
|---|---|---|---|---|
| **B0** | Stabilization & Reconciliation | **L** | 14–18 | — |
| **B1** | UI/UX Foundation, HUD & Input Modes | **L** | 14–18 | `[INTERNAL]` |
| **B2** | Art Direction Lock & Asset Pipeline | **M** | 6–8 | `[INTERNAL]` |
| **B3** | Persistence, Save Architecture & Streaming Backbone | **L** | 16–20 | `[INTERNAL]` |
| **B4** | World Content: Region, Interiors, Elevation & Light | **XXL** | 45–60 | `[INTERNAL]` |
| **B5** | Dynamic Events, Radio & Investigation Arc | **L** | 18–22 | `[PUBLIC]` |
| **B6** | Skills, Progression, Character Creation & Onboarding | **L** | 16–20 | `[PUBLIC]` |
| **B7** | Audio Production & Sound Design | **L** | 14–18 | `[PUBLIC]` |
| **B8** | Performance, Profiling & Optimization | **L** | 12–16 | `[INTERNAL]` |
| **B9** | Accessibility, Settings & Sandbox Options | **M** | 8–10 | `[PUBLIC]` |
| **B10** | Multiplayer Hardening & Release Engineering | **L** | 14–18 | `[PUBLIC]` |
| **B11** | Internal Closed Beta | **M** | 6–8 | `[INTERNAL]` |
| **B12** | Public Beta / Early Access Readiness | **L** | 12–16 | `[PUBLIC]` |
| | **Total** | | **~195–250** | ≈ **10–14 months part-time** |

> **Reality check against `GameDevPlan.md` §6's estimate.** That doc's "order-of-magnitude ~6–9 months" covered P0–P10 and was written before UI, art lock, audio, accessibility, release engineering, and the two beta gates had phases. ~10–14 months **from today, for a genuinely beta-grade build** is the honest number, and it assumes the part-time cadence holds without long gaps. Treat B4 as the number most likely to be wrong — content build is where solo projects overrun, and it is XXL for a reason.

### 3.3 Scope boundaries per phase

Full task breakdowns live in the per-phase files. This table is the **scope contract** — if something is in the OUT column and starts happening, that is scope creep and should be caught in review.

| Phase | Explicitly IN | Explicitly OUT |
|---|---|---|
| **B0** | PIE-verify everything built in P5/P6 · item-instance/GUID refactor · ammo-as-inventory-item · handedness fields · camera revisions (perspective removal, zoom presets, aim-cone, elevation *stub*) · Wet + Temperature needs · two-tier infection · bandage simplification · jamming · downed-zombie finishers · BT search-last-known · death→zombie + loot-at-death · profiling baseline | No new UI (B1) · no real multi-level geometry (B4) · no weather visuals (B4) · no save (B3) · no horde-coordination redesign (B7) · no attachments-with-stats |
| **B1** | Input-mode/`IMC_ZS_UI` switching · HUD (needs, health/wounds, ammo, hotbar, interaction prompt) · inventory screen · container loot screen · death/respawn screen · sleep prompt · main menu + pause · moodle system for N needs | Radial quick-use (B9) · map screen (B4) · character creation UI (B6) · settings menu (B9) · localization (B12) · final art pass on UI (B2/B7) |
| **B2** | Art direction lock (mood board, palette, reference set) · modular kit selection/purchase · material/shader standards · LOD + collision + naming conventions · retarget pipeline validation · one fully-dressed reference room as the quality bar | Building the region (B4) · character art (B6) · VFX polish (B7) · UI art (B1 owns layout, B2 owns tokens) |
| **B3** | Save topology decision · `UZSSaveGameSubsystem` · layered save (10s character, periodic world, chunk-on-unload, shutdown) · rotating backup slots · World Partition setup + streaming policy · corpse/item pooling + dual-limit cleanup · persistence of despawn timers by time-of-death | Region content (B4) · late-join flow (B10) · cloud saves (post-beta) · save migration/versioning beyond a version stamp (B12) |
| **B4** | Region build on the real kit · enterable interiors · **elevation/multi-level system** · **darkness + light-source mechanic** · **basement layout selection** · weather system (rain/fog/snow) as gameplay · day/night · utilities shutoff on the real map · map screen · spawn points | Named-location *narrative* content (B5) · events (B5) · vehicles (CUT) · farming (see OQ-B4-06) · seasons (post-beta) |
| **B5** | `UZSEventDirector` · repeatable event roster · per-event radio warning treatment · radio broadcast arc · investigation clue system + journal/tracking UI · guaranteed clue placement · radiant objectives · named locations + their narrative content | Voice acting (OQ-B5-05) · branching dialogue · NPC survivors (POST-BETA) · the capstone's post-completion world modifier (POST-BETA) |
| **B6** | Skill/attribute XP hookup across all systems · XP curves + per-skill rate tunable · background roster + starting proficiencies · character creation flow + appearance · new-game setup (seed, difficulty, spawn) · first-hour onboarding pass · death→new-character polish | Skill decay (CONFIRMED cut) · perks/unlocks (OQ-B6-03) · deferred skills — Fishing/Building/Foraging/Cooking/Mechanics (POST-BETA) · sandbox sliders (B9, partial) |
| **B7** | Ambient beds per biome/interior/time-of-day · zombie vocalization set · weapon/melee/impact SFX · footstep surface + wet variants · UI SFX · music direction + implementation · audio occlusion/attenuation policy · **horde coordination + large-group AI solution** | VO (OQ-B5-05) · adaptive/vertical music (POST-BETA) · full Wwise/FMOD migration unless OQ-B7-02 says otherwise |
| **B8** | Fixed stress-test map/scenario (CONFIRMED) · profiling on packaged Development builds · zombie-count budget lock · draw-call/material consolidation · AI tick budgeting + LOD · network bandwidth pass · memory/GC pass · min-spec decision | Console optimization (POST-BETA) · dedicated-server perf (POST-BETA) |
| **B9** | Settings menu (video/audio/gameplay/controls) · full control remapping · gamepad support pass · colorblind modes · subtitle/text-size options · difficulty options · the XP-rate tunable surfaced · partial sandbox options | Full PZ-style sandbox slider suite (POST-BETA) · screen-reader support (POST-BETA) · localization (B12) |
| **B10** | Late-join flow · disconnect/reconnect character handling · host-migration policy decision · network stress + packet-loss testing · direct-IP/LAN hardening · packaged build pipeline · crash reporting/telemetry · versioning + release checklist | Dedicated servers (OQ-B10-01) · Steam/EOS integration (OQ-B10-02) · voice chat (OQ-B10-04) · cross-platform (POST-BETA) |
| **B11** | Closed-group beta with 6–12 testers · structured feedback + bug intake · tuning passes from real data · balance of ammo/loot/zombie density from telemetry | Public marketing · store page · press |
| **B12** | Steam page + capsule art + trailer · demo build decision · pricing/EA decision · localization pass · public bug-report pipeline · community channels · launch checklist | Post-launch roadmap content · DLC |

---

## 4. Dependency map

### 4.1 Hard blocking chain

```
B0 Stabilization  ──────────────────────────────────────────────┐
  │  (item-instance model, verified combat/inventory,           │
  │   camera model locked, needs complete, profiling baseline)  │
  ├──> B1 UI/UX ──────────────────────────────────┐             │
  │      (input-mode switching, HUD, inventory)   │             │
  │                                               │             │
  ├──> B2 Art Lock ──> B4 World Content <─────────┤             │
  │      (kit, tokens)      ^                     │             │
  │                         │                     │             │
  └──> B3 Persistence ──────┘                     │             │
         (save, streaming, pooling)               │             │
                                                  v             v
                              B5 Events/Investigation      B6 Progression
                                        │                       │
                                        └───────────┬───────────┘
                                                    v
                                        B7 Audio ──> B8 Performance
                                                    │
                                                    v
                                        B9 A11y/Settings
                                                    │
                                                    v
                                        B10 MP Hardening + Release Eng
                                                    │
                                                    v
                                        B11 INTERNAL BETA GATE
                                                    │
                                                    v
                                        B12 PUBLIC BETA GATE
```

### 4.2 Why each edge exists

| Edge | Reason it is a hard dependency |
|---|---|
| B0 → everything | The item-instance model is the substrate for inventory UI, save serialization, loot, and crafting. Building any of those against today's model means building them twice. |
| B0 → B1 | An inventory screen must render *something*. Today `CarrySlots` has no stable per-item identity to bind a widget to, and no drag/drop target that survives a move. |
| B1 → B4 | Region content cannot be playtested without a HUD showing needs/health, or a map screen. You would be building a world you cannot evaluate. |
| B2 → B4 | Placing production geometry before the kit, material standards, and LOD/collision conventions are locked means re-doing placement. This is the classic solo-dev rework trap. |
| B3 → B4 | World Partition setup and the chunk-on-unload save hook must exist *before* the region is streamed, or every cell gets revisited. De-risk save against graybox. |
| B0 → B3 | Save serialization needs a stable item identity (`FGuid`) and a settled needs list. Serializing 6 needs then adding 2 means a save-version migration during beta. |
| B4 → B5 | Events need real locations to fire at; clue placement needs real containers/buildings; radio needs a map to reference. |
| B0+B4 → B6 | XP hookup touches every gameplay system, so those systems must be final. Backgrounds pick spawn points, which need the real map. |
| B4 → B7 | Ambient audio design is per-biome/per-interior; you cannot author beds for spaces that don't exist. |
| B0 → B8 (baseline only) | Profiling **starts** at B0 (per CONFIRMED "profile early") but the dedicated optimization phase needs final content to optimize against. |
| B8 → B10 | Network hardening measures against a known-good frame budget; optimizing after network work invalidates the network measurements. |
| B1+B9 → B11 | Testers without a settings menu or remappable controls generate noise-bugs about hardware/preference, drowning real findings. |

### 4.3 Parallelizable work (safe to interleave when blocked)

- **T4 content authoring** (weapon/item/loot data assets) — from end of B0 onward, continuously.
- **B2 art lock** can run entirely in parallel with **B1** and **B3** — different disciplines, no shared files.
- **B9's control remapping** can start any time after B0 (Enhanced Input is already in place).
- **T3 marketing/community** should start at **B4**, not B12 — see `T_ContinuousTracks.md` for why the timing matters.
- **OQ resolution** is always parallelizable and should be batched into design sessions rather than blocking implementation sessions.

### 4.4 Highest-risk items, de-risk early

| Risk | Where it bites | Prototype/spike at |
|---|---|---|
| Multi-level interiors + top-down camera occlusion | B4 (XXL, would be a rebuild) | **B0-T3 spike**, then B4-T1 |
| Save/load correctness under co-op + World Partition | B3 (data loss in beta = fatal) | **B3-T1 spike** on graybox |
| Zombie count vs. frame budget | B8 (may force a design change to horde behaviour) | **B0-T12 baseline**, budget locked B8 |
| Item-instance refactor breaking shipped behaviour | B0 (touches 5 files of live code) | B0-T1 verification pass *first* |
| Live Coding Blueprint corruption during heavy C++ churn | B0 specifically — the highest-C++-churn phase in the plan | See `CLAUDE.md` lesson; B0-T0 sets a full-rebuild policy |

---

## 5. Revised risk register

Replaces `GameDevPlan.md` §6 for beta-scope purposes.

| Risk | Severity | Mitigation |
|---|---|---|
| **Unverified code accumulates faster than it is tested** — the current failure mode; 4+ sessions of unrun code shipped before the first PIE confirmation | **HIGH** | B0 is a hard verification gate. New standing rule: no phase may exit with unverified deliverables; `SessionHandoff.md` stays the sole owner of verification status. |
| **Top-down doesn't feel right** — mitigation removed by CR-04 | **HIGH** | New gate: B0-PT2 camera sign-off **before** the perspective enum is deleted. Zoom presets + auto-zoom replace over-shoulder as the detail-reading affordance. |
| **B4 content build overruns** — the single largest phase, solo, part-time | **HIGH** | Vertical-slice-first: build *one* fully-finished town block to the B2 quality bar and time it, then extrapolate before committing to region scale. Map scale is a lever (OQ-B4-01). |
| **Save/persistence data loss in co-op** | **HIGH** | Rotating backups (CONFIRMED) + save-corruption soak test in B3 + versioned save format from day one. |
| **Simulation creep** — 8 needs, temperature, weather, elevation, basements all added at once | **MEDIUM-HIGH** | §3.3's IN/OUT table is a contract. Every scope-risk item is 🚩-flagged in §1.3 with a scoped-down proposal in `90_OpenQuestions.md`. |
| **Live Coding Blueprint corruption** during B0's heavy C++ churn | **MEDIUM** | Full rebuild over Ctrl+Alt+F11 during B0; "Compile All Blueprints" pass after each patch cluster; check Output Log for `is not a child class of` / `invalid target type` first when anything behaves oddly. |
| **Zombie counts vs. performance** | **MEDIUM** | Profiling baseline at B0, fixed stress-test scenario (CONFIRMED), zombie count as the primary budget metric (CONFIRMED). |
| **Solo-dev art volume** | **MEDIUM** | Buy the core kit; one dense region, not a county. B2's reference room sets the bar so quality is decided once. |
| **Open questions block implementation mid-phase** | **MEDIUM** | Every OQ is tagged BLOCKING/SEQUENCEABLE/LATE and grouped by phase; resolve a phase's BLOCKING set in one design session before the phase starts. |
| **Scope pressure from the investigation arc** | **LOW-MEDIUM** | Decision 6 already resolved it as an optional capstone. B5 builds the system; content volume is the lever. |

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
