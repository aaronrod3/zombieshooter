# B8 — Performance, Profiling & Optimization

**Size: L (12–16 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B4, B7** · **Blocks: B10, B11**

> **Profiling starts at B0, not here.** CONFIRMED guidance is "profile early, don't retrofit efficiency later," and B0-T12 builds the fixed stress-test scenario for exactly that reason. This phase is the **dedicated optimization pass against final content** — the point where budgets stop being observations and become commitments.
>
> **Methodology is CONFIRMED and non-negotiable** (Consolidated §12): standard UE triage — `stat unit` / `stat fps` first for Game/Draw/RHI/GPU-bound triage, then `stat ai` / `stat anim` / `stat physics` for gameplay cost, then `stat gpu` / `stat drawcount` for rendering cost. **Profile on packaged Development/Test builds only. Never Debug, never raw PIE numbers.**

## Entry criteria

- [ ] B4 complete — optimizing against placeholder content measures the wrong thing.
- [ ] B7-T5 complete — the horde approach is chosen and implemented.
- [ ] B0-T12 baseline available for before/after comparison on the same fixed scenario.
- [ ] **OQ-B8-01 and OQ-B8-02 answered** — performance budget numbers and minimum hardware target. Both DEFERRED since Consolidated §12; both are now decidable because the measurements exist.

## Exit criteria

- [ ] Target frame rate held at target zombie density on min-spec hardware, packaged Development build, 2–4 players.
- [ ] Zombie count budget is a written, enforced number (**zombie count is the primary budget metric** — CONFIRMED).
- [ ] No frame-time spike above a stated threshold during streaming, saving, or horde events.
- [ ] Before/after numbers on the B0-T12 scenario are committed and show measured improvement.

---

## Task breakdown

### B8-T1 — Budget definition · **S (2 sessions)**

| Sub-task | Definition of done | Ref |
|---|---|---|
| T1.1 | **Minimum hardware target decided** → OQ-B8-02. Reference point from Consolidated §12: PZ's min spec is ~quad-core 2.77GHz / 8GB / 2GB VRAM and its "actually smooth" spec is i5-9600K / Ryzen 5600-class + RTX 3060-class + 16GB — **but PZ is a 2D sprite engine and is explicitly not a valid direct baseline for a 3D UE project.** | X-14 |
| T1.2 | **Target frame rate** at min spec, and the acceptable 1%-low. An average FPS target alone hides exactly the hitches that ruin a survival game. |
| T1.3 | **Zombie count budget** — the primary metric, ratified from B7-T5.7's proposal against `GameDevPlan` §7 P4 Q1's ~150 question. | X-14 |
| T1.4 | Sub-budgets: draw calls, triangles, AI ms/frame, animation ms/frame, physics ms/frame, network bandwidth per client, memory ceiling. |
| T1.5 | Budgets committed to `Docs/Testing/PerfBudget.md` and referenced from `TuningReference.md`. |

### B8-T2 — Measurement pass · **S (2–3 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T2.1 | Packaged Development build of the full game on the real map. |
| T2.2 | Triage sequence run per CONFIRMED methodology and recorded: bound-type first, then gameplay costs, then rendering costs. |
| T2.3 | Measurements at 25 / 50 / 100 / 150 / 250 zombies on production content, matching B0-T12's tiers so the comparison is apples-to-apples. |
| T2.4 | Scenarios covered: quiet exploration, horde event, multi-level interior combat, streaming traversal, save-in-progress, 4-player co-op. |
| T2.5 | Min-spec hardware measured, or a documented proxy (frequency/core limiting) if no such machine is available. |
| T2.6 | A ranked list of the top 10 costs. **Optimize in measured order, never in guessed order** — this is the discipline the whole phase rests on. |

### B8-T3 — Rendering optimization · **M (3–4 sessions)** · *depends on T2*

| Sub-task | Definition of done |
|---|---|
| T3.1 | Draw-call reduction: instancing, HISM for repeated props, merged actors where sensible. B2-T3.2's master-material cap pays off here. |
| T3.2 | Material complexity audit; consolidate onto the master-material set. |
| T3.3 | LOD verification across the region — B2-T3.3's policy actually applied, not just written. |
| T3.4 | Shadow and light cost, especially interiors and B4-T4's dynamic light sources. Player-carried lights at 4 players is a real cost. |
| T3.5 | Culling: distance, occlusion, and the interaction with B4-T2's interior visibility solution. |
| T3.6 | Texture streaming and memory pool sizing. |

### B8-T4 — Gameplay & AI optimization · **M (3–4 sessions)** · *depends on T2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | AI tick LOD verified/extended from B7-T5.3. | — |
| T4.2 | Perception query cost — typically the dominant AI cost at scale. Budget query frequency by distance. |
| T4.3 | **Animation cost**: URO (update rate optimization) for distant zombies, and consider a mesh-merge or lighter path for crowds. `stat anim` per CONFIRMED methodology. |
| T4.4 | Physics: collision complexity, `ApplyHitKnockback`'s `LaunchCharacter` cost at horde scale, ragdoll policy on death. |
| T4.5 | **Object pooling verified** end-to-end for corpses, world items, and zombies (B3-T4.2's CONFIRMED requirement) — measure GC pressure before and after. | X-1 |
| T4.6 | Tick audit: every `Tick` in the codebase justified or converted to a timer. `AZombieAIController`'s 0.2s tick and `UpdateNearestInteractable`'s per-tick sphere scan are the two known ones. |

### B8-T5 — Network optimization · **S (2–3 sessions)** · *depends on T2*

| Sub-task | Definition of done |
|---|---|
| T5.1 | Bandwidth per client measured at target density with 4 players. |
| T5.2 | Replication frequency and relevance distance tuned per actor class. Zombies are the volume driver. |
| T5.3 | `FZSItemInstance` replication cost checked — arrays of structs with nested structs are a classic hidden bandwidth cost. |
| T5.4 | Listen-server host cost isolated: the host pays both server and client costs and is the real min-spec case. |

### B8-T6 — Verification & regression guard · **S (2 sessions)** · *depends on T3–T5*

| Sub-task | Definition of done |
|---|---|
| T6.1 | Re-run T2's full measurement suite; before/after committed to `Docs/Testing/`. |
| T6.2 | All budgets met, or explicitly renegotiated with the reason written down. |
| T6.3 | A repeatable profiling checklist so the B0-T12 scenario keeps being used identically for the rest of the project. |
| T6.4 | Performance regression check added to T2's build checklist (`T_ContinuousTracks.md`). |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Honest baseline** on final content at min spec. | Numbers exist for every scenario. The top-10 cost list is measured, not assumed. |
| **PT2** | End of T4 | **Horde stress at target density**, 4 players, production content. | Budget held. No stalls. AI still behaves correctly after LOD changes — optimization that breaks behaviour is not a win. |
| **PT3** | B8 exit | **Full co-op session on min-spec hardware**, 90 minutes, including streaming, saving, a horde event, and multi-level combat. | Target frame rate and 1%-lows held throughout. No spike above threshold. |

## Notes

- **Console optimization is POST-BETA.** `GameDevPlan` §1 notes console-friendliness as a design consideration (which is partly why top-down was chosen), but no console work is scheduled.
- **Dedicated-server performance is POST-BETA** — listen-server only (OQ-B10-01).
- **If the zombie budget lands well below the design assumption**, that is a *design* input, not just an engineering result. Feed it back to B7-T5 and, if needed, escalate — horde-based events may need rescoping rather than the engine being pushed harder.
