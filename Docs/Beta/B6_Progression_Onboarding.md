# B6 — Skills, Progression, Character Creation & Onboarding

**Size: L (16–20 dev-sessions)** · **Gate: `[PUBLIC]`** · **Depends on: B0, B4 (hard)** · **Blocks: B11**

> **XP hookup touches every gameplay system**, so those systems must be final — which is why this sits after B4 rather than early. Backgrounds pick starting spawn points (Decision 4), which need the real map.
>
> **⚠ Blocked on CR-01.** `ZombieShooter_Consolidated_Changes.md` §9 refers to "the six v1 skills (Melee, Firearms, Fitness, Medicine, Carpentry, Survival)" — the **superseded 2026-07-18 list**. `GameDevPlan.md` §3.1 (revised 2026-07-19) replaced it. **This phase is planned against §3.1.** If §9 was a deliberate reversal, this phase changes shape substantially, and Carpentry would pull base-building into v1 — an XL swing. **Resolve CR-01 before starting.**

## Entry criteria

- [ ] **CR-01 resolved** — which skill roster is real.
- [ ] B0 complete — every system XP hooks into must be final.
- [ ] B4 complete — backgrounds need real spawn locations (placed in B4-T10.3).
- [ ] **OQ-B6-04/05/06 answered (BLOCKING)** — background roster, background tradeoffs, radio tutorial pacing. All three are DEFERRED items from Consolidated §10 and this is the plan's largest single gap area.

## Exit criteria

- [ ] A new player can create a character, understand their background's meaning, and start playing.
- [ ] Every skill and attribute gains from play, with visible, comprehensible feedback.
- [ ] "A stranger survives their first 30 minutes without a wiki and dies to something they understand" (old P9's exit criterion, kept verbatim — it is a good one).
- [ ] Death → new character → same world is clean and understood.
- [ ] Per-skill XP rate is an exposed tunable (CONFIRMED).

---

## Task breakdown

### B6-T1 — Skill/attribute framework · **M (4–5 sessions)**

Per `GameDevPlan.md` §3.1 (pending CR-01).

| Sub-task | Definition of done | Ref |
|---|---|---|
| T1.1 | `UZSSkillComponent` on `AZSPlayerCharacter` (or state on `AZSPlayerState`) — replicated, following the standard convention. Persists via B3-T5.5. |
| T1.2 | **Attributes** (passive pools, grow from broad play): Strength, Stamina, Sneak, Sprint. | §3.1 |
| T1.3 | **Skills** (levels 1–5, learn-by-doing): per-weapon-class Melee bars, Maintenance, Aiming, Reloading, First Aid. | §3.1 |
| T1.4 | `UZSSkillConfig` data asset per skill: XP curve, level thresholds, per-level effects. **N skills, zero C++ branches.** | — |
| T1.5 | **No skill decay** (CONFIRMED). Skills stay at earned level permanently. Do not build a decay path "for later." | X-8 |
| T1.6 | **Per-skill XP rate is an exposed tunable** (CONFIRMED), surfaced in B9's settings even though full sandbox sliders stay post-v1. | X-8 |

### B6-T2 — XP hookup across systems · **M (4–5 sessions)** · *depends on T1*

The cross-cutting task. Each hookup is small; there are many.

| Sub-task | Definition of done |
|---|---|
| T2.1 | **Melee class bars** ← landing hits with that weapon class (`PerformMeleeSwing`). Higher level: faster attack speed, more damage, higher crit chance. |
| T2.2 | **Aiming** ← shots fired/hits landed. Affects the B0-T3.5 aim cone, time-to-aim, and effective range. **This is now the primary skill-expression fork in ranged combat**, because CR-11 made the aim cone the sole source of combat pressure. |
| T2.3 | **Reloading** ← reloads performed. Affects reload speed and per-round loading. |
| T2.4 | **Maintenance** ← maintenance actions. Reduces wear rate; raises effective durability; also reduces B0-T10.1's jam chance, which is the natural pairing. |
| T2.5 | **First Aid** ← treatment actions. More effective items, faster application. |
| T2.6 | **Strength** ← melee kills, carrying heavy loads. Raises melee damage and carry capacity. |
| T2.7 | **Stamina** ← sprinting, swinging. Raises the pool and the economy. |
| T2.8 | **Sneak** ← crouched movement. Reduces detection radius/noise, feeding `UZSNoiseSystem`. |
| T2.9 | **Sprint** ← sprinting. Affects speed and endurance cost. |
| T2.10 | XP curves authored per skill into `TuningReference.md` → OQ-B6-01. |
| T2.11 | **Skill feedback UI** (B1's architecture): level-up notification, a character sheet, and hover-preview of what the next level actually gives — the transparent-stat-preview pillar. |

### B6-T3 — Practice loops · **S (2–3 sessions)** · *depends on T2*

OPEN FOR EXPLORATION per Consolidated §9 → OQ-B6-02.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | Each skill has a **low-friction way to practise deliberately** that is not grinding — a safe repeatable action with diminishing but non-zero returns. | X-12 |
| T3.2 | Practice loops are discoverable through play, ideally surfaced by the radio tutorial arc (T5). |
| T3.3 | Practice never outpaces real use — using a skill in genuine danger must remain the better XP source, or the game teaches players to stand in a basement hitting a wall. |

### B6-T4 — Backgrounds & character creation · **M (4–5 sessions)** · *depends on T1; blocked by OQ-B6-04/05*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | `UZSBackgroundConfig` data asset. **Primary differentiator is higher starting proficiency in specific skills** (CONFIRMED) — e.g. Soldier starts with higher Firearms. Not unique items. | X-9 |
| T4.2 | Background roster authored (5–7 per `GameDevPlan` §3) → OQ-B6-04. Must suit the setting and **not mirror another game's occupation list**. |
| T4.3 | Whether backgrounds carry balancing drawbacks → OQ-B6-05. |
| T4.4 | Whether a secondary unique unlock exists alongside proficiency → part of the same deferred pass. |
| T4.5 | **Background-tied starting spawn** (Decision 4), using B4-T10.3's placed points, with the lobby-level **scatter-spawns toggle** for co-op groups who would rather start together. |
| T4.6 | Appearance customization: modular characters from the art kit → OQ-B6-08. Name/gender/appearance independent of background choice. |
| T4.7 | **No trait point-buy** (`GameDevPlan` §3, REPLACE). Build variety comes from background choice plus emergent play. |

### B6-T5 — New-game flow & onboarding · **M (3–4 sessions)** · *depends on T4, B5-T3*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | New-game setup: world seed, difficulty options, starting-location choice, co-op host/join → OQ-B6-09. |
| T5.2 | **Radio-guided first days** — B5-T3.3's delivery mechanism carrying B6's onboarding content. Pacing per OQ-B6-06. | X-13 |
| T5.3 | Interaction hints on first encounter with each system, shown once, never as a modal wall. |
| T5.4 | **Transparent stat/action previews everywhere** (Notes §21 pillar) — the consistent answer to "what does this actually do." |
| T5.5 | Death → new character → same world flow polished, including the co-op case and the party-wipe/solo-death rules from B3's OQ-B3-01. |
| T5.6 | Death-recap screen decision → OQ-B6-07. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Progression feel** over a long session — is levelling visible and satisfying without being grindy? | Skills rise from normal play at a rate that feels earned. Level-ups are noticed without a popup demanding attention. |
| **PT2** | End of T3 | **Anti-grind test.** Deliberately try to grind each skill the cheapest way possible. | The cheap way is worse than playing. If any skill has a dominant grind, retune before shipping it. |
| **PT3** | End of T4 | **Background differentiation** — play the same opening with 3 different backgrounds. | They feel meaningfully different from minute one, from proficiency and spawn location alone. |
| **PT4** | B6 exit | **⚑ THE STRANGER TEST.** Someone who has never seen the game plays for 30 minutes with **no narration from you at all.** | They survive 30 minutes without a wiki, and when they die they understand why. This is the single best onboarding signal available and it must be run with a real person, not imagined. |

## Notes

- **Deferred skills stay deferred**: Fishing, Building, Foraging, Cooking, Mechanics are POST-BETA (`GameDevPlan` §3.1). Mechanics arrives with vehicles, which are CUT.
- **Perks/unlocks beyond passive stat improvements** → OQ-B6-03. Default assumption is no — passive improvements only, consistent with the non-grind goal.
- **Skill cap is 1–5 for everyone.** Whether rare items/traits can exceed it → OQ-B6-03.
- **PT4 is the highest-value playtest in the entire plan.** Schedule it with a real person, and resist explaining anything.
