# B4 / B4X — World Systems (Stage 1) & Region Content Build-Out (Stage 2)

> **Split 2026-07-26** (`Docs/Planning/RescopeQuestionnaire.md`). This used to be one XXL (45–60 session) phase bundling three brand-new subsystems with the actual content-building work — the plan's own audit already flagged it as the single most likely phase to overrun. The dev's rescope answers kept the ambition (all three systems confirmed KEEP, map going *bigger*, not smaller) but asked for it "in phases" — so the split below is structural, not just a recommendation:
>
> - **B4 (this file, Stage 1 — do now):** the three systems — multi-level/elevation, darkness-requires-light, weather-as-mechanic — built and tested on the **existing small graybox/test area**, not the real map. Small, checkpointed, systems-only. Tasks T2/T3/T4/T8 below.
> - **B4X (Stage 2 — do later, as a continuous track, not a single phase):** the actual region build, now **bigger than the original ~1×1 km proposal** (dev call, partly to accommodate vehicles landing later — `BV`) and built **district by district**, each one playtested before the next starts. See `T_ContinuousTracks.md` T7. Tasks T1/T5/T6/T7/T9/T10 below become B4X's scope, with **T6 rewritten** — procedural/randomized basement layouts are **cut**; the dev wants a fixed, authored map for now, not a layout-selection system.
>
> **Sequencing principle, unchanged: systems before volume.** Build every world *system* against the small test area first (B4), then run content volume (B4X) as repeatable, checkpointed execution against those proven systems.

## Entry criteria (B4 — Stage 1 systems work)

- [ ] B1 complete — you cannot evaluate a world without a HUD.
- [ ] B3 complete — save/streaming proven on graybox (B4's systems work doesn't strictly need this, but B4X does, so get it done first anyway).

## Entry criteria (B4X — Stage 2 content, later)

- [x] **OQ-B4-01 resolved 2026-07-26 (dev-confirmed)** — region scale is **bigger** than the original ~1×1 km proposal, built in phases (this is why it's a continuous track, not a single phase). **Multi-biome**: "will have urban, rural, wooded, suburban areas" — plan district variety across at least these four area types, not one dense town repeated. Still validate district build-time against B2-T4.5's measured per-room cost before committing to the full scale.
- [ ] **OQ-B4-02 — partially resolved.** Named locations get **generic/functional placeholder names for now** ("Town Center," "Hospital") — dev wants to focus on mechanics first, real naming/flavor comes later, likely alongside B5. Mechanical-identity spread (medical loot site, firearms site, etc.) still applies.
- [x] **CR-02 resolved 2026-07-26 (dev-confirmed)** — vehicles are **not** cut; they get their own phase (`BV`) later in Stage 2. This is *why* the map is going bigger.
- [ ] B2 complete — kit, material standards, LOD/collision policy, and the reference room's measured budget all locked.

## Exit criteria (B4 — Stage 1 systems)

- [ ] Multi-level buildings are navigable, fightable, and lootable by both players simultaneously, on the small test area.
- [ ] Dark spaces genuinely require a light source and that constraint feels like a mechanic, not an annoyance.
- [ ] Weather visibly and mechanically affects Temperature and Wet, and replicates correctly.
- [ ] Frame budget from B0-T12 still met on the test area with these systems active.

## Exit criteria (B4X — Stage 2 content, ongoing/final)

- [ ] The real (bigger, phased) map plays end-to-end in co-op, from multiple spawn points.
- [ ] Utilities shutoff fires on the real map and changes how it plays.
- [ ] Frame budget from B0-T12/B8 still met with production content at target zombie density (re-baselined for 4+ players per CR-08).

---

## Task breakdown

### B4X-T1 — Region blockout · **M (5–6 sessions)** · *depends on entry criteria* · **Stage 2 (content), first task of the B4X continuous track**

| Sub-task | Definition of done |
|---|---|
| T1.1 | Whole region blocked out in graybox at final scale per OQ-B4-01: one dense area + rural fringe (`GameDevPlan` §3). |
| T1.2 | Named locations placed per OQ-B4-02, with the investigation arc's needs in mind — B5 needs specific places to hide clues. |
| T1.3 | **Traversal pass**: walk/run/sprint the whole region. Distances must feel right *without vehicles* (CR-02). This is where an over-ambitious map scale gets caught, while it is still free to fix. |
| T1.4 | Streaming cells laid out against the blockout; B3's cell size validated against real geometry distribution. |
| T1.5 | Zone boundaries defined — the zone system P4-R7 and P6-R7 both need and neither could build. **One definition, used by zombie population, loot quality tiers, and ambient audio.** |

### B4-T2 — Interior visibility for top-down · **M (4–5 sessions)** · *depends on T1* · **Stage 1 (systems), build/spike on the small test area**

The problem old P7 named and never solved. Must be settled before interiors are built at volume.

| Sub-task | Definition of done |
|---|---|
| T2.1 | Solution chosen per OQ-B4-03. **Dev-confirmed approach: run the spike** — roof-fade on entry vs. camera-relative cutaway plane, prototyped against the reference room; the dev hasn't picked a winner yet, that's what this task settles. |
| T2.2 | Implemented and tested against the reference room from B2-T4. |
| T2.3 | **Co-op case handled** — two players in different rooms, or one inside and one outside, each need a correct view. This is where naive roof-hiding breaks: it is a per-player rendering concern on a shared world. |
| T2.4 | Interacts correctly with B0-T3.2's auto-zoom `Interior` context — entering a building both changes visibility and triggers zoom. |
| T2.5 | Performance cost measured. A per-player occlusion solution at 2–4 players is a real cost, not free. |

### B4-T3 — Elevation & multi-level system · **L (6–8 sessions)** · *depends on T2* · **Stage 1 (systems), build/verify on the small test area before B4X ever needs it**

Replaces B0-T3.7's single-floor stub with the real implementation. CONFIRMED as fully automatic, no manual player control.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | **Floor detection** — `UZSElevationSubsystem` resolves any actor to a floor index / Z-plane. **Resolved (OQ-B4-04): authored floor volumes** — reliable, explicit, designer-controlled; the authoring cost folds into B4X-T10's per-building pass. | P1-R6 |
| T3.2 | **Aim ray resolves against the character's current floor plane** — the CONFIRMED requirement. Shooting on floor 2 must not hit something on floor 1 through the geometry. | P1-R6 |
| T3.3 | Camera follows floor changes automatically, coordinated with T2's visibility solution — going upstairs reveals the new floor and hides the one below. |
| T3.4 | **AI navigation across floors** — zombies use stairs, chase between floors, and pathfind correctly. NavMesh across multiple levels needs explicit setup, not defaults. |
| T3.5 | **Noise propagation respects floors** — a gunshot upstairs should reach downstairs, but attenuated. Feeds `UZSNoiseSystem`; needs a rule, even a simple per-floor multiplier. |
| T3.6 | Zombie spawn/population is floor-aware — no zombies spawning inside geometry or on unreachable floors. |
| T3.7 | Verified 2-client with players on different floors of the same building. |

### B4-T4 — Darkness & light sources · **M (4–5 sessions)** · *depends on T2* · **Stage 1 (systems), build/verify on the small test area**

CONFIRMED first-class mechanic (Consolidated §7). Makes B0-T11's `SecondaryHand` load-bearing.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | Darkness values authored per space type per B2-T1.4's direction. "Dark" is a gameplay threshold, not only an art choice. | X-2 |
| T4.2 | Flashlight (B0-T11.4) works as a real light in the world: cone, range, falloff, and it is **visible to other players** — a light source is a co-op positional signal. |
| T4.3 | Additional light sources: lantern (placeable, hands-free), and whatever the utilities-shutoff transition makes scarce. |
| T4.4 | **Light attracts zombies.** **Resolved (OQ-B4-07): yes** — "zombies are attracted to light and sound." Light extends effective detection radius against the holder, same principle as noise. |
| T4.5 | Battery/fuel consumption creates a real resource decision, feeding the scavenge loop. |
| T4.6 | Utilities shutoff makes interiors dark, converting light from convenience to necessity at a known day. **This is the phase transition doing real work.** |

### B4X-T5 — Enterable buildings & doors · **M (4–5 sessions)** · *depends on T2, T3* · **Stage 2 (content), but the door actor/behavior itself is worth a small systems-only spike in Stage 1 if T3's multi-level work needs a door to test against**

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | Door actor reusing `UZSInteractableComponent`: open/close, replicated, with real transition time. |
| T5.2 | **Door-thumping** — P4's unbuilt deliverable. Zombies attack doors between them and a heard target; doors have health and break. | P4-R8 |
| T5.3 | **Resolved (OQ-B4-08): breaching + a new Lockpicking skill**, a hybrid, not a single pure option. Breaching (force the door, generate noise) stays as the loud/fast path. Lockpicking is a new levelable skill — pure success-chance roll by level, no minigame; failed attempts generate noise (spamming picks is a real risk); higher levels grant more speed and stealth. Add **Lockpicking** to `GameDevPlan.md` §3.1's skill list (new skill, reverses the plan's earlier "roster is settled" assumption). |
| T5.4 | Doors block/attenuate noise propagation, tying into T3.5's rules. |
| T5.5 | Windows as an alternate entry and a zombie entry point. |
| T5.6 | Building interiors are populated with containers seeded from `UZSLootTableConfig` per zone quality tier — the payoff for T1.5's zone system. | P6-R7 |

### B4X-T6 — Basements & underground · **S (2–3 sessions, down from M 4–5)** · *depends on T3, T4* · **Stage 2 (content) — 🚩 risk removed 2026-07-26**

⚑ **Cut 2026-07-26 (dev-confirmed).** The randomized/procedural layout-selection system described below is **no longer being built** — "will stick to a fixed map for now." Basements, if any are placed, are ordinary hand-authored rooms like any other part of the map — built once, placed once, no weighted-pool selection system, no `UZSBasementLayoutConfig`. This removes one of the plan's two biggest scope-risk flags outright.

| Sub-task | Definition of done | Ref |
|---|---|---|
| ~~T6.1~~ | ~~Randomized layout selection from an authored pool~~ — **cut.** If a basement exists, it's one specific, hand-placed room. | ~~X-3~~ superseded |
| ~~T6.2~~ | ~~`UZSBasementLayoutConfig` data asset~~ — **not needed**, no selection system exists. | superseded |
| T6.3 | Any placed basement persists identically on revisit — this is just ordinary world-state persistence (B3), not a special basement rule anymore. | — |
| T6.4 | Underground is always dark — T4's mechanic at its strongest. Still applies to any authored basement. | — |
| T6.5 | Streaming and NavMesh work underground; T3's floor detection handles negative floor indices. Still applies. | — |
| ~~T6.6~~ | ~~Budget guard: cap elaborate-complex count~~ — **moot**, there's no procedural pool to cap. | superseded |

### B4X-T7 — Zombie AI depth pass, population & zones · **L (6–8 sessions)** · *depends on T1.5, OQ-B4-12* · **Stage 2 (content, needs real zones) — now also owns the new zombie "freshness" mechanic, see T7.0**

> **Grew 2026-07-23.** Was M (3–4 sessions) covering population/zones only. B0-T0's Blueprint triage found `BT_Zombie` running on a placeholder ShooterGame-derived loop with real design work explicitly deferred here — see **OQ-B4-12**. T7.0 below is that redesign; T7.1–T7.5 are the original population/zone scope, now tuned against real behavior instead of a stopgap.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T7.0 | **PZ-fidelity behavior redesign** per OQ-B4-12: ambient wandering, bounded last-known-location memory, crowd-following, door/obstacle destruction spec (feeds T5.2). Audit the disconnected `BTTask_*` assets found in B0 (`Wander`, `GetInvestigationPoint`, `ClearLastKnownLocation`, `StartIdleDwell`, `StartInvestigationTimer`) — decide keep vs. rebuild per-node, don't assume either. Do this **before** T7.1, so population/density tuning happens against real behavior. **New scope, added 2026-07-26 (dev-confirmed):** design the zombie "freshness" mechanic here too — recently-turned zombies faster/stronger, degrading toward slower/weaker over time, likely a per-type curve on `UZSZombieConfig`. Also carries a raised bar: genuine large hordes (100+) are confirmed important to the dev's vision, not a number to trade away — see CR-08. | OQ-B4-12 |
| T7.1 | Zone-based population densities driven by `UZSZombieConfig` + per-zone density values. | P4-R7 |
| T7.2 | **Repopulation rule** for cleared areas. **Resolved (OQ-B4-05): slow migration-based repopulation** from adjacent zones — diegetic, not a spawn timer. Persists via B3-T5.6. |
| T7.3 | Spawn placement avoids player line-of-sight — zombies must never pop in visibly. |
| T7.4 | **Per-zone loot quality tiers** — P6's deferred feature, now unblocked by a real zone system. | P6-R7 |
| T7.5 | Density validated against B0-T12's baseline on production geometry, not graybox. |
| T7.6 | Crowd-following behavior (T7.0) verified specifically against B5-T2.4's horde migration event, and its result feeds **OQ-B7-01** — a working crowd-follow may reduce or remove the need for explicit Rally-Leader-style coordination. |

### B4-T8 — Weather & day/night · **M (4–5 sessions)** · *depends on T1* · **Stage 1 (systems) — build the weather actor/rules on the small test area; T8's dependency on "T1" means the graybox test area, not B4X's real map**

Promoted from atmospheric to gameplay-authoritative by CR-03.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T8.1 | Weather actor is **server-authoritative and replicated** — clients must agree on whether it is raining, because it drives Wet and Temperature. |
| T8.2 | Rain, fog, and snow (Adirondacks-appropriate) with defined gameplay effects: visibility, noise masking, Wet, Temperature. | CR-03 |
| T8.3 | Rain sets B0-T4.1's `Wet` flag; shelter clears it. Replaces the debug setter. | P2-R1 |
| T8.4 | Ambient temperature drives B0-T4.3's model from weather + time-of-day + indoor/outdoor. | P2-R2 |
| T8.5 | **Rain masks noise** — a genuine stealth interaction with the noise pillar. Confirm it is wanted → OQ-B4-09. |
| T8.6 | Day/night cycle length. **Resolved (OQ-B4-10): confirmed as a tunable**; ~2 real hours (night ~1/3) stands as the starting value to test from, not a locked number. Night is meaningfully darker, tying into T4. |
| T8.7 | Seasons remain **POST-BETA** (`GameDevPlan` §3 SIMPLIFY). Do not build a season system; do author weather so a season layer could drive it later. |

### B4X-T9 — Map screen & navigation · **S (2–3 sessions)** · *depends on T1* · **Stage 2 (content, needs the real map)**

| Sub-task | Definition of done |
|---|---|
| T9.1 | Map screen (B1's widget architecture) showing the region, named locations, and player position. |
| T9.2 | Co-op: teammate positions shown, or explicitly not → OQ-B4-11. |
| T9.3 | Player-placed markers — the substrate B5's clue/journal system will pin to. |
| T9.4 | Map discovery model: fully revealed, or revealed by exploration → OQ-B4-11. |

### B4X-T10 — Content volume pass · **ongoing, continuous track — not a fixed session count** · *depends on T1–T9* · **Stage 2 (content)**

Pure execution against locked systems. **Do not start until B4's Stage-1 systems (T2/T3/T4/T8) are stable and PIE-verified.** ⚑ **Restructured 2026-07-26**: this is no longer a single XL (12–18 session) block — it's the main body of `B4X`, the continuous content-build track (`T_ContinuousTracks.md` T7), executed **district by district**, each one playtested and time-recorded before the next starts. The map is bigger than originally planned (dev call), which makes this discipline load-bearing, not optional.

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
| **PT1** | End of B4X-T1 | **Traversal-and-scale test.** Cross the whole (bigger) region on foot at every movement speed. | Distances feel right for now, on foot, ahead of `BV` landing. Since vehicles are confirmed coming (CR-02) rather than cut, a "this feels like it wants a car" result here is expected and fine — it's not the red flag it used to be. Flag anything that feels *broken* on foot (not just slow), since that's a genuine district-scale problem. |
| **PT2** | End of B4-T3 (Stage 1, small test area) | **Multi-level combat, 2-client.** Fight up and down a 3-storey building. One player upstairs, one downstairs. Shoot on one floor while a zombie is on another. | Aim never resolves through floors. Zombies path between floors. Both players see correct visibility. Noise attenuates per floor. Pass this **before** B4X ever builds a multi-level building for real. |
| **PT3** | End of B4-T4 (Stage 1, small test area) | **Darkness run.** Explore an unlit interior with and without a light source. | Without light it is genuinely unplayable-dark, not just moody. With light it is tense but fair. Light is visible to the other player. |
| ~~PT4~~ | ~~End of T6~~ | ~~Basement variety~~ — **removed 2026-07-26**, procedural basements cut. If a hand-authored basement exists, it just needs the same treatment as any other room (B2's reference-room checkpoint), not a dedicated variety test. | — |
| **PT5** | End of B4-T8 (Stage 1, small test area) | **Weather → survival loop.** Get caught in rain far from shelter at night. | Wet + cold compounds into real danger. Sheltering is a genuine decision. Both clients agree on the weather. Pass this before B4X relies on weather for real content pacing. |
| **PT6** | B4X exit (end of the content build-out track) | **Full co-op session, 90+ minutes, unscripted, on the real map through the utilities shutoff.** | The map plays. Frame budget holds at target density (re-baselined for 4+ players, CR-08). Nothing about the world reads as placeholder. |

## Prototyping vs. stable-systems guidance

| Prototype early | Wait |
|---|---|
| **B4-T2 interior visibility** — spike all four OQ-B4-03 options against the B2 reference room before committing; the wrong choice is a rebuild. | **B4X-T10 content volume** — obviously last, and now an ongoing track rather than a single push. |
| **B4-T3.1 floor detection** — volume vs. trace is testable in an afternoon and everything else in T3 depends on it. | **B4X-T7.5 density validation** — needs production geometry to be meaningful. |
| ~~T6.1 basement pool~~ — moot, no pool exists. | **B4X-T5.6 container placement** — needs T1.5's zones final. |

## Notes

- **B4X (content) is the overrun risk, not B4 (systems).** With region scale now bigger (dev call) and delivered as a continuous track rather than one committed phase, the mitigation is structural: build **one district**, playtest it, time it, re-forecast before starting the next — the same discipline the old single-phase B4 already recommended, now load-bearing since scale went up. See `T_ContinuousTracks.md` T7.
- **Farming/agriculture.** **Resolved (OQ-B4-06): foraging zones only for beta** — marked areas yield food on a timer, no growth simulation. Farming-lite stays post-beta with the deferred Foraging skill. Add a T-task for foraging zones (small, similar scope to a loot-container pass) when B4X-T10 content authoring reaches this.
- **Vehicles stay CUT.** If PT1 says the map needs them, that is a scope decision to escalate, not to solve inline.
