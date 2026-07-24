# B4 — World Content: Region, Interiors, Elevation & Light

**Size: XXL (45–60 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B1, B2, B3 (all hard)** · **Blocks: B5, B6, B7**

> **The largest phase in the plan and the most likely to overrun.** It contains the region build (pure content volume), plus three genuinely new subsystems the consolidated changes introduced with one bullet each: elevation/multi-level, darkness-requires-light, and randomized basement layouts. Weather is also promoted here from atmospheric to gameplay-authoritative, because B0's Temperature and Wet needs have nothing to read from until it is.
>
> **Sequencing principle: systems before volume.** Build every world *system* against a small test area first (T1–T6), then run content volume (T7–T10) as repeatable execution. If a system change lands after 30 buildings are placed, those 30 get revisited.

## Entry criteria

- [ ] B1 complete — you cannot evaluate a world without a HUD or a map.
- [ ] B2 complete — kit, material standards, LOD/collision policy, and the reference room's measured budget all locked.
- [ ] B3 complete through PT5 — save and streaming proven on graybox.
- [ ] **OQ-B4-01 answered (BLOCKING)** — region scale, validated against B2-T4.5's measured per-room build time.
- [ ] **OQ-B4-02 answered (BLOCKING)** — named locations. Deferred twice; B5's investigation arc and B6's spawn points both depend on it.
- [ ] **CR-02 answered** — vehicles in or out, because it changes viable map scale.

## Exit criteria

- [ ] The real map plays end-to-end in co-op, from multiple spawn points.
- [ ] Multi-level buildings and basements are navigable, fightable, and lootable by both players simultaneously.
- [ ] Dark spaces genuinely require a light source and that constraint feels like a mechanic, not an annoyance.
- [ ] Weather visibly and mechanically affects Temperature and Wet.
- [ ] Utilities shutoff fires on the real map and changes how it plays.
- [ ] Frame budget from B0-T12 still met with production content at target zombie density.

---

## Task breakdown

### B4-T1 — Region blockout · **M (5–6 sessions)** · *depends on entry criteria*

| Sub-task | Definition of done |
|---|---|
| T1.1 | Whole region blocked out in graybox at final scale per OQ-B4-01: one dense area + rural fringe (`GameDevPlan` §3). |
| T1.2 | Named locations placed per OQ-B4-02, with the investigation arc's needs in mind — B5 needs specific places to hide clues. |
| T1.3 | **Traversal pass**: walk/run/sprint the whole region. Distances must feel right *without vehicles* (CR-02). This is where an over-ambitious map scale gets caught, while it is still free to fix. |
| T1.4 | Streaming cells laid out against the blockout; B3's cell size validated against real geometry distribution. |
| T1.5 | Zone boundaries defined — the zone system P4-R7 and P6-R7 both need and neither could build. **One definition, used by zombie population, loot quality tiers, and ambient audio.** |

### B4-T2 — Interior visibility for top-down · **M (4–5 sessions)** · *depends on T1*

The problem old P7 named and never solved. Must be settled before interiors are built at volume.

| Sub-task | Definition of done |
|---|---|
| T2.1 | Solution chosen per OQ-B4-03: roof-fade on entry, cutaway/section plane, per-room reveal, or dithered occlusion. |
| T2.2 | Implemented and tested against the reference room from B2-T4. |
| T2.3 | **Co-op case handled** — two players in different rooms, or one inside and one outside, each need a correct view. This is where naive roof-hiding breaks: it is a per-player rendering concern on a shared world. |
| T2.4 | Interacts correctly with B0-T3.2's auto-zoom `Interior` context — entering a building both changes visibility and triggers zoom. |
| T2.5 | Performance cost measured. A per-player occlusion solution at 2–4 players is a real cost, not free. |

### B4-T3 — Elevation & multi-level system · **L (6–8 sessions)** · *depends on T2*

Replaces B0-T3.7's single-floor stub with the real implementation. CONFIRMED as fully automatic, no manual player control.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | **Floor detection** — `UZSElevationSubsystem` resolves any actor to a floor index / Z-plane. Volume-based (authored per floor) is more reliable than trace-based; decide in OQ-B4-04. | P1-R6 |
| T3.2 | **Aim ray resolves against the character's current floor plane** — the CONFIRMED requirement. Shooting on floor 2 must not hit something on floor 1 through the geometry. | P1-R6 |
| T3.3 | Camera follows floor changes automatically, coordinated with T2's visibility solution — going upstairs reveals the new floor and hides the one below. |
| T3.4 | **AI navigation across floors** — zombies use stairs, chase between floors, and pathfind correctly. NavMesh across multiple levels needs explicit setup, not defaults. |
| T3.5 | **Noise propagation respects floors** — a gunshot upstairs should reach downstairs, but attenuated. Feeds `UZSNoiseSystem`; needs a rule, even a simple per-floor multiplier. |
| T3.6 | Zombie spawn/population is floor-aware — no zombies spawning inside geometry or on unreachable floors. |
| T3.7 | Verified 2-client with players on different floors of the same building. |

### B4-T4 — Darkness & light sources · **M (4–5 sessions)** · *depends on T2*

CONFIRMED first-class mechanic (Consolidated §7). Makes B0-T11's `SecondaryHand` load-bearing.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | Darkness values authored per space type per B2-T1.4's direction. "Dark" is a gameplay threshold, not only an art choice. | X-2 |
| T4.2 | Flashlight (B0-T11.4) works as a real light in the world: cone, range, falloff, and it is **visible to other players** — a light source is a co-op positional signal. |
| T4.3 | Additional light sources: lantern (placeable, hands-free), and whatever the utilities-shutoff transition makes scarce. |
| T4.4 | **Light attracts zombies** — or explicitly does not. This is an unstated but obvious interaction with the noise-as-threat pillar and it needs a decision → OQ-B4-07. |
| T4.5 | Battery/fuel consumption creates a real resource decision, feeding the scavenge loop. |
| T4.6 | Utilities shutoff makes interiors dark, converting light from convenience to necessity at a known day. **This is the phase transition doing real work.** |

### B4-T5 — Enterable buildings & doors · **M (4–5 sessions)** · *depends on T2, T3*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | Door actor reusing `UZSInteractableComponent`: open/close, replicated, with real transition time. |
| T5.2 | **Door-thumping** — P4's unbuilt deliverable. Zombies attack doors between them and a heard target; doors have health and break. | P4-R8 |
| T5.3 | Locked doors + keys/lockpicking, or a decision not to have them → OQ-B4-08. |
| T5.4 | Doors block/attenuate noise propagation, tying into T3.5's rules. |
| T5.5 | Windows as an alternate entry and a zombie entry point. |
| T5.6 | Building interiors are populated with containers seeded from `UZSLootTableConfig` per zone quality tier — the payoff for T1.5's zone system. | P6-R7 |

### B4-T6 — Basements & underground · **M (4–5 sessions)** · *depends on T3, T4* 🚩

CONFIRMED with randomized layout selection. **Scope-flagged** — this is procedural level assembly described in one bullet.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T6.1 | **Randomized layout selection from an authored pool**, not procedural generation. Most instances are simple single rooms; rare instances are elaborate bunker/tunnel complexes. **Authored-pool-and-pick is the scope-safe reading of "randomized"** and it is what fits the 1/3-depth pillar. | X-3 |
| T6.2 | `UZSBasementLayoutConfig` data asset — a weighted pool, matching the project's data-asset-driven rule. New layouts = new data assets, zero C++. |
| T6.3 | Layout selection is **seeded and persisted** (B3) so a basement is the same on every visit. A re-rolling basement is a bug that will read as a ghost story. |
| T6.4 | Underground is always dark — T4's mechanic at its strongest. |
| T6.5 | Streaming and NavMesh work underground; T3's floor detection handles negative floor indices. |
| T6.6 | **Budget guard:** cap the elaborate-complex count for beta (suggest 2–3 authored). This is where content ambition quietly doubles the phase. |

### B4-T7 — Zombie AI depth pass, population & zones · **L (6–8 sessions)** · *depends on T1.5, OQ-B4-12*

> **Grew 2026-07-23.** Was M (3–4 sessions) covering population/zones only. B0-T0's Blueprint triage found `BT_Zombie` running on a placeholder ShooterGame-derived loop with real design work explicitly deferred here — see **OQ-B4-12**. T7.0 below is that redesign; T7.1–T7.5 are the original population/zone scope, now tuned against real behavior instead of a stopgap.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T7.0 | **PZ-fidelity behavior redesign** per OQ-B4-12: ambient wandering, bounded last-known-location memory, crowd-following, door/obstacle destruction spec (feeds T5.2). Audit the disconnected `BTTask_*` assets found in B0 (`Wander`, `GetInvestigationPoint`, `ClearLastKnownLocation`, `StartIdleDwell`, `StartInvestigationTimer`) — decide keep vs. rebuild per-node, don't assume either. Do this **before** T7.1, so population/density tuning happens against real behavior. | OQ-B4-12 |
| T7.1 | Zone-based population densities driven by `UZSZombieConfig` + per-zone density values. | P4-R7 |
| T7.2 | **Repopulation rule** for cleared areas decided and implemented → OQ-B4-05. Persists via B3-T5.6. |
| T7.3 | Spawn placement avoids player line-of-sight — zombies must never pop in visibly. |
| T7.4 | **Per-zone loot quality tiers** — P6's deferred feature, now unblocked by a real zone system. | P6-R7 |
| T7.5 | Density validated against B0-T12's baseline on production geometry, not graybox. |
| T7.6 | Crowd-following behavior (T7.0) verified specifically against B5-T2.4's horde migration event, and its result feeds **OQ-B7-01** — a working crowd-follow may reduce or remove the need for explicit Rally-Leader-style coordination. |

### B4-T8 — Weather & day/night · **M (4–5 sessions)** · *depends on T1*

Promoted from atmospheric to gameplay-authoritative by CR-03.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T8.1 | Weather actor is **server-authoritative and replicated** — clients must agree on whether it is raining, because it drives Wet and Temperature. |
| T8.2 | Rain, fog, and snow (Adirondacks-appropriate) with defined gameplay effects: visibility, noise masking, Wet, Temperature. | CR-03 |
| T8.3 | Rain sets B0-T4.1's `Wet` flag; shelter clears it. Replaces the debug setter. | P2-R1 |
| T8.4 | Ambient temperature drives B0-T4.3's model from weather + time-of-day + indoor/outdoor. | P2-R2 |
| T8.5 | **Rain masks noise** — a genuine stealth interaction with the noise pillar. Confirm it is wanted → OQ-B4-09. |
| T8.6 | Day/night cycle length decided → OQ-B4-10. Night is meaningfully darker, tying into T4. |
| T8.7 | Seasons remain **POST-BETA** (`GameDevPlan` §3 SIMPLIFY). Do not build a season system; do author weather so a season layer could drive it later. |

### B4-T9 — Map screen & navigation · **S (2–3 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T9.1 | Map screen (B1's widget architecture) showing the region, named locations, and player position. |
| T9.2 | Co-op: teammate positions shown, or explicitly not → OQ-B4-11. |
| T9.3 | Player-placed markers — the substrate B5's clue/journal system will pin to. |
| T9.4 | Map discovery model: fully revealed, or revealed by exploration → OQ-B4-11. |

### B4-T10 — Content volume pass · **XL (12–18 sessions)** · *depends on T1–T9*

Pure execution against locked systems. **Do not start until T1–T9 are stable.**

| Sub-task | Definition of done |
|---|---|
| T10.1 | All buildings dressed to B2-T4's quality bar and per-room budget. |
| T10.2 | Container placement and loot-table assignment across every zone. |
| T10.3 | Spawn points authored, including profession/background-tied starts (Decision 4) and the scatter-spawns toggle. **Backgrounds are B6's, but their spawn locations are placed here.** |
| T10.4 | Landmark/environmental storytelling passes on named locations, feeding B5. |
| T10.5 | Utilities shutoff wired against the real map on its randomized day. |
| T10.6 | Full-region performance pass against B0-T12's baseline. |
| T10.7 | **Progress checkpoint every ~4 sessions**: measure completed-area percentage against session count and re-forecast. This phase overruns silently otherwise. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | **Traversal-and-scale test.** Cross the whole region on foot at every movement speed. | Distances feel right without vehicles. If the region feels like it needs a car, either scale down or reopen CR-02 — **now, not at T10.** |
| **PT2** | End of T3 | **Multi-level combat, 2-client.** Fight up and down a 3-storey building. One player upstairs, one downstairs. Shoot on one floor while a zombie is on another. | Aim never resolves through floors. Zombies path between floors. Both players see correct visibility. Noise attenuates per floor. |
| **PT3** | End of T4 | **Darkness run.** Explore an unlit interior and a basement with and without a light source. | Without light it is genuinely unplayable-dark, not just moody. With light it is tense but fair. Light is visible to the other player. |
| **PT4** | End of T6 | **Basement variety.** Enter 10+ basements across the region, then reload and revisit them. | Layouts vary. Each is identical on revisit. No streaming or nav failures underground. |
| **PT5** | End of T8 | **Weather → survival loop.** Get caught in rain far from shelter at night. | Wet + cold compounds into real danger. Sheltering is a genuine decision. Both clients agree on the weather. |
| **PT6** | B4 exit | **Full co-op session, 90+ minutes, unscripted, on the real map through the utilities shutoff.** | The map plays. Frame budget holds at target density. Nothing about the world reads as placeholder. |

## Prototyping vs. stable-systems guidance

| Prototype early | Wait |
|---|---|
| **T2 interior visibility** — spike all four OQ-B4-03 options against the B2 reference room before committing; the wrong choice is a rebuild. | **T10 content volume** — obviously last. |
| **T3.1 floor detection** — volume vs. trace is testable in an afternoon and everything else in T3 depends on it. | **T7.5 density validation** — needs production geometry to be meaningful. |
| **T6.1 basement pool** — prove selection + persistence with two placeholder layouts before authoring the real pool. | **T5.6 container placement** — needs T1.5's zones final. |

## Notes

- **T10 is the overrun risk.** Its estimate rests entirely on B2-T4.5's measured build time. Re-forecast at every T10.7 checkpoint and treat region scale as the lever — cutting area is always cheaper than cutting quality.
- **Farming/agriculture** (`GameDevPlan` §3: "v1: farming-lite + foraging zones") has no task here → OQ-B4-06. It is the one §3 commitment this plan does not schedule, deliberately, pending that decision.
- **Vehicles stay CUT.** If PT1 says the map needs them, that is a scope decision to escalate, not to solve inline.
