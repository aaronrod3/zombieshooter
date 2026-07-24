# B7 — Audio Production & Horde AI Solution

**Size: L (14–18 dev-sessions)** · **Gate: `[PUBLIC]`** · **Depends on: B4 (hard)** · **Blocks: B8, B11**

> **Two things share this phase for a reason.** Audio needs real spaces to be authored for. The horde-coordination solution needs B0's profiling baseline *and* B4's production content to be designed against, because CONFIRMED guidance explicitly subordinates that design to whatever performs best. Both land here, and they interact: a horde is as much an audio event as an AI one.
>
> **Audio is not decoration in this game.** Noise is the threat currency (`GameDevPlan` §1 pillar). The player's own audio feedback is how they judge whether they just made a mistake.

## Entry criteria

- [ ] B4 complete — ambient beds are per-biome/per-interior and need real spaces.
- [ ] B0-T12 profiling baseline captured.
- [ ] **OQ-B7-01 answered** — horde coordination approach, informed by the baseline.
- [ ] **OQ-B7-02 answered** — audio middleware: UE built-in MetaSounds, or Wwise/FMOD.

## Exit criteria

- [ ] A player can close their eyes and tell roughly where the threat is and how many.
- [ ] Every noise-generating action has audio matching its actual `UZSNoiseSystem` radius — what the player *hears* and what zombies *hear* must agree.
- [ ] Horde behaviour holds the frame budget at target density on production content.
- [ ] No silent gameplay states remain: jams, breaks, infection onset, critical bleed all have audio.

---

## Task breakdown

### B7-T1 — Audio architecture · **S (2–3 sessions)**

| Sub-task | Definition of done |
|---|---|
| T1.1 | Middleware decision implemented per OQ-B7-02. **Default recommendation: UE built-in + MetaSounds** — no licensing, no extra build complexity, and the project's needs are not exotic. |
| T1.2 | Attenuation and occlusion policy. **Occlusion matters mechanically here**, not just aesthetically: a zombie behind a wall should sound like it. Ties to B4-T3.5's per-floor noise rules and B4-T5.4's doors. |
| T1.3 | Sound-class/mix hierarchy so B9's volume sliders have real buses to control. |
| T1.4 | Concurrency limits — 150+ zombies each with vocalizations will exceed voice count. Set caps, virtualization, and distance culling **now**, before authoring. |
| T1.5 | **Surface types authored on collision** (B2-T3.4's rider) so footsteps vary by material. Retrofitting this across a finished region is miserable. |

### B7-T2 — Zombie audio · **M (3–4 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T2.1 | Vocalization set per state: idle/wander, alerted, investigating, chasing, attacking, downed, dying. **State must be audible** — this is the player's primary read on AI intent in a top-down view where facing is hard to see. |
| T2.2 | Per-`UZSZombieConfig` voice assignment, so a future type is a data change. |
| T2.3 | **Crowd/horde audio** — a distinct layered treatment so 30 zombies sound like a horde, not 30 individuals. This is also the voice-count solution: one crowd bed replaces N individual voices. |
| T2.4 | Distance and occlusion behaviour that lets a player estimate count and direction without seeing anything. |

### B7-T3 — Combat & player audio · **M (3–4 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | Firearms: shot, tail/reflection by space, dry-fire, reload stages, **jam and jam-clear** (B0-T10.2 requires this to be legible). | P5-R1 |
| T3.2 | **Gunshot audio radius matches `FireNoiseRadius`.** If the player hears it further than zombies do, the noise pillar is a lie. Verify against B0-PT4's measured radii. |
| T3.3 | Melee: swing, hit by material, break, **stomp/finisher** (B0-T10.6). |
| T3.4 | Footsteps by surface, movement mode, and **wet variant** (B0-T4.2). Wet must be audibly distinct — it is a CONFIRMED gameplay signal, not flavour. |
| T3.5 | Body/health audio: breathing by stamina, pain, **critical head bleed** (B0-T5.3 — needs an unmistakable cue), infection onset kept **deliberately ambiguous** per CR-06. |
| T3.6 | UI audio for B1's screens. |

### B7-T4 — Ambience & music · **M (3–4 sessions)** · *depends on T1, B4*

| Sub-task | Definition of done |
|---|---|
| T4.1 | Ambient beds per zone type × time of day × weather (B4-T8) × indoor/outdoor. Use B4-T1.5's zones as the key — one zone definition, reused again. |
| T4.2 | Weather audio driven by the same replicated authoritative state clients already agree on. |
| T4.3 | Underground/basement ambience — the darkness mechanic's audio half, and where horror does most of its work. |
| T4.4 | Music direction decided → OQ-B7-04. **Recommended: sparse and event-driven, not continuous** — a persistent score fights the noise-as-threat pillar by masking the audio the player needs. |
| T4.5 | Event audio signatures for B5's roster — a flyover must be recognizable before it is visible. |

### B7-T5 — Horde coordination & large-group AI · **M (4–5 sessions)** · *depends on B0-T12, B4*

CONFIRMED: no Rally Leader committed; whatever supports efficient large-horde processing should drive the design.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | Profile the current per-zombie cost on production content at 50/100/150/250, using the B0-T12 stress map and methodology. | X-4 |
| T5.2 | Approach chosen per OQ-B7-01 (flow-field, shared-target grouping, LOD'd individual AI, or hybrid) **on measured evidence**. |
| T5.3 | **AI tick LOD** — distant zombies tick less often. Usually the single biggest win and it is approach-agnostic, so it is worth doing regardless. |
| T5.4 | Perception query budgeting — AIPerception is typically the dominant cost at scale. |
| T5.5 | **Senses stay fixed per type** (CONFIRMED) — no per-individual randomization sneaking in as an optimization. | P4-R2 |
| T5.6 | Horde behaviour verified against B5-T2.4's migration event on the real map. |
| T5.7 | Zombie count budget proposed for B8 to ratify. | OQ-B8-01 |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Eyes-closed threat assessment.** A tester with the screen off estimates direction and rough count of approaching zombies. | Consistently correct within one "band" (one / a few / a lot) and roughly the right direction. |
| **PT2** | End of T3 | **Noise-radius honesty.** Fire at measured distances; compare what the player hears against what zombies respond to. | They agree. Any mismatch is a bug against the game's central pillar. |
| **PT3** | End of T4 | **Atmosphere pass** — 30 minutes across day/night, indoor/outdoor, weather, underground. | Spaces feel distinct. Underground is genuinely tense. Music never masks a threat cue. |
| **PT4** | End of T5 | **Horde stress**, production content, target density, 2 clients. | Frame budget holds. Horde reads as a horde audibly and behaviourally. No AI popping or stalling. |

## Notes

- **Voice acting is not in scope** unless OQ-B5-05 overturns it. Zombie vocalizations are sound design, not VO, and can be sourced or created.
- **Audio is a strong candidate for paid assets** — libraries are cheap relative to the time cost of recording, and this is a good use of any budget from OQ-B2-01.
- **A minimum-viable audio subset should be pulled forward into B0/B4** wherever a gameplay state would otherwise be silent (jams, breaks, critical bleed). Do not let a mechanic ship inaudible just because B7 is later.
