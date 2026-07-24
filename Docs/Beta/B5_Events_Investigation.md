# B5 — Dynamic Events, Radio & Investigation Arc

**Size: L (18–22 dev-sessions)** · **Gate: `[PUBLIC]`** · **Depends on: B4 (hard)** · **Blocks: B11 content completeness**

> **This is the differentiator.** `GameDevPlan.md` §1 argues the project can exist next to Project Zomboid primarily because PZ has no missions/objectives layer and an empty late game. Everything else in the plan brings the project to parity; this phase is the part that makes it a different game.
>
> **Decision 6 is already resolved**: the investigation arc is an **optional capstone with no forced ending** — completion unlocks a lore epilogue and a persistent world-state change, but the world keeps running.

## Entry criteria

- [ ] B4 complete — events need real locations to fire at, clues need real containers to hide in, radio needs a real map to reference.
- [ ] **OQ-B4-02 (named locations) resolved** — this is B5's content substrate, not just B4's.
- [ ] **OQ-B5-01 answered (BLOCKING)** — the actual plot: outbreak origin, story beats, what the player can and cannot learn. `GameDevPlan` §7 P8 Q3 ("how many distinct meta-events at launch") is also still flagged BLOCKING there.
- [ ] B3-T5.7's reserved save schema available for event/clue flags.

## Exit criteria

- [ ] Two co-op sessions on the same map play out differently (old P8's exit criterion).
- [ ] A full playthrough of the investigation arc is possible start to finish.
- [ ] No clue is ever unreachable — guaranteed placement holds under every world seed.
- [ ] Radio delivers the day 1–7 arc and transitions into dynamic events without a seam.
- [ ] Completing the arc changes the world persistently and the world keeps running.

---

## Task breakdown

### B5-T1 — Event director core · **M (4–5 sessions)**

| Sub-task | Definition of done | Ref |
|---|---|---|
| T1.1 | `UZSEventDirector` (world subsystem) — scheduling, cooldowns, weighting, and an active-event registry. Server-authoritative, replicated to clients. |
| T1.2 | `UZSEventConfig` data asset: type, weight, cooldown, day-range gating, warning treatment, repeatable flag. **Zero C++ branches per event type** — the multi-weapon rule applied to events. |
| T1.3 | **Most events are repeatable within a session** (CONFIRMED). Investigation story beats may be one-time; the flag lives per config, not in code. | X-6 |
| T1.4 | **Escalation over time** — event selection weighted by `DayCount` → OQ-B5-03. |
| T1.5 | Events persist correctly (B3): active events, cooldown timers, and one-time flags survive save/reload. |

### B5-T2 — Event roster · **M (5–6 sessions)** · *depends on T1*

Roster size is OQ-B5-04; `GameDevPlan` §7 P8 Q3 has flagged the count as BLOCKING since 2026-07-19.

| Sub-task | Definition of done |
|---|---|
| T2.1 | **Helicopter/flyover** — the PZ-class archetype. Drags zombies along its path; a genuine "get indoors" moment. |
| T2.2 | **Alarm events** — car alarm, house alarm. Localized noise magnet; a lootable-but-dangerous opportunity. |
| T2.3 | **Military convoy / supply drop** — a location-based opportunity with a threat attached. |
| T2.4 | **Horde migration** — a mass of zombies moving through a zone. Directly dependent on B7's horde solution; schedule after it if needed. |
| T2.5 | **Ambient flavour events** — distant gunshots, screams. Locatability decided by OQ-B5-02 (DEFERRED item). | X-10 |
| T2.6 | **Weather-driven events** — a storm that masks noise and cripples visibility, reusing B4-T8 rather than adding a system. |
| T2.7 | Each event has: a warning treatment (T3), a gameplay consequence, an audio signature (B7), and a defined interaction with zombie population. |

### B5-T3 — Radio system · **M (4–5 sessions)** · *depends on T1*

Doing triple duty: tutorial, mission-giver, and narrative delivery.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | Radio item + station model. Diegetic: the player must have a working radio, which makes it a scavenge target and gives the utilities shutoff another lever. |
| T3.2 | Broadcast content system — data-asset driven, day-gated, with a playback queue. |
| T3.3 | **Scripted day 1–7 arc** doubling as onboarding (B6 owns the tutorial pacing; B5 owns the delivery mechanism). |
| T3.4 | **Per-event warning treatment** (CONFIRMED): some events are pre-announced by radio, others are not, varying by event type. This is what makes having a radio matter. | X-7 |
| T3.5 | Transition from scripted arc into dynamic broadcasts + investigation clues, with no perceptible seam. |
| T3.6 | Text-only vs. voiced decided → OQ-B5-05. Text-only is the scope-safe default and is what the plan assumes. |

### B5-T4 — Investigation & clue system · **L (5–6 sessions)** · *depends on T1, T3*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | `UZSClueConfig` + a clue registry. Clues are notes, documents, items, and locations. |
| T4.2 | **Guaranteed placement** (RESOLVED 2026-07-18): a predetermined pool with a random pick, so a clue is **never missing**. Verify under many world seeds, not one. |
| T4.3 | **Journal/tracking UI** — how clues are stored, read, and cross-referenced. Reuses B1's widget architecture and B4-T9.3's map markers. → OQ-B5-06. |
| T4.4 | Arc progression: discovering clue N unlocks/points toward N+1 without becoming a linear quest chain. |
| T4.5 | **Co-op clue state is shared** — one player finding a clue must advance the party. Anything else makes the arc miserable in co-op, which is the game's primary mode. |
| T4.6 | Capstone: lore epilogue + a persistent world-state change. **The world keeps running** (Decision 6). |
| T4.7 | Arc state persists (B3-T5.7). |

### B5-T5 — Radiant objectives · **S (2–3 sessions)** · *depends on T1, T2*

| Sub-task | Definition of done |
|---|---|
| T5.1 | Objective wrappers around events — "invitations with stakes, never mandatory" (`GameDevPlan` P8). |
| T5.2 | Objectives surface through the radio and the journal, never as a floating quest marker — that would break the diegetic framing the rest of the phase is built on. |
| T5.3 | Ignoring an objective has no penalty. Completing one has a real reward. |
| T5.4 | Objectives scale with `DayCount` per T1.4. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Event variety over a long session.** Play 3+ in-game days and log every event fired. | Events feel varied, not on a visible timer. Repeatable events recur without feeling scripted. Each is survivable but consequential. |
| **PT2** | End of T3 | **Radio arc, days 1–7**, played fresh. | The arc teaches the game without a tutorial popup. Warned events feel different from unwarned ones — having a radio is clearly worth the scavenge. |
| **PT3** | End of T4 | **Full arc playthrough** across at least 3 world seeds. | No clue is unreachable in any seed. The arc is followable without a wiki. Co-op progress is genuinely shared. |
| **PT4** | B5 exit | **Two separate co-op sessions on the same map**, compared side by side — old P8's exit criterion. | They play out differently. The capstone lands and the world continues afterward. |

## Notes

- **OQ-B5-01 (the plot) is the biggest content-authoring dependency in the plan** and the one most likely to expand. It is a writing task, not an engineering one, and it can be done during B4 while blocked on other things — do it there.
- **Voice acting is not assumed.** Text-only radio transcripts and notes are the scope-safe default consistent with the 1/3-depth pillar. OQ-B5-05 can overturn it, but it adds cost and a dependency on external talent.
- **NPC survivors and factions remain POST-BETA.** Hostile human roamers are the first post-v1 addition (Decision 5) and reuse P4's AI architecture cheaply. Nothing in B5 should require them.
