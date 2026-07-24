# B2 — Art Direction Lock & Asset Pipeline

**Size: M (6–8 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B0** · **Blocks: B4 (hard)**

> **Why this precedes the region build.** Old P7 bundled "replace graybox with the chosen modular kit" and "build the region" into one phase. That ordering is the classic solo-dev rework trap: geometry placed before material standards, LOD policy, and collision conventions are settled gets re-done. B2 exists to **decide the quality bar exactly once**, prove the pipeline on one room, and then let B4 be pure execution.
>
> Runs fully in parallel with B1 and B3 — different discipline, no shared files.

## Entry criteria

- [ ] B0 complete (art decisions shouldn't be made against a moving gameplay target).
- [ ] **OQ-B2-01 answered** — asset budget: is there money for marketplace/contracted art, or is this free-assets-and-Blender only? This changes everything downstream.
- [ ] Decision 3 re-confirmed: dark, earthy, slight realism, kept low-poly. Synty explicitly excluded.

## Exit criteria

- [ ] One fully-dressed reference room exists at final quality and is the committed standard.
- [ ] A new prop can go from source file to placed-in-level following a written checklist, with no judgement calls left.
- [ ] Draw-call and triangle budgets per room/prop class are written down and were **measured**, not guessed.
- [ ] Existing placeholder content is triaged: keep / retarget / replace / delete.

---

## Task breakdown

### B2-T1 — Direction lock · **S (1–2 sessions)**

| Sub-task | Definition of done |
|---|---|
| T1.1 | Mood board assembled — 20–30 references covering exterior day, exterior night, interior lit, interior dark, and one horde shot. Committed to `Docs/Art/`. |
| T1.2 | Palette defined: base environment ramp, a restricted accent set, and a **readability palette** for gameplay-critical objects (interactables, items, zombies) that must never blend into environment tones at top-down distance. |
| T1.3 | **Top-down readability test** — silhouette and value contrast checked at actual gameplay camera distance and at both B0-T3.1 zoom extremes. An art style that reads beautifully in a 3/4 screenshot and turns to mush top-down is the failure mode. |
| T1.4 | Lighting direction decided, **including the darkness mechanic (X-2)**: how dark is "requires a light source"? This is a gameplay value, not only an aesthetic one, and B4 builds against it. |

### B2-T2 — Kit selection & acquisition · **S (2 sessions)**

| Sub-task | Definition of done |
|---|---|
| T2.1 | Modular environment kit chosen against T1's direction, covering the Adirondacks brief: small-town US buildings, forest, rural fringe, interiors. |
| T2.2 | Coverage gap analysis — what the kit does not include and how each gap gets filled (buy / Blender / cut the requirement). |
| T2.3 | Licensing/repo rule applied: paid content gitignored, never committed; large free content re-downloadable rather than committed ($0 LFS budget). Add to `.gitignore` **before** import, not after. |
| T2.4 | Existing placeholder content triaged. Specifically: the dead raw Lyra/ShooterGame import in `/Game/Animation/` (references a never-migrated `SK_Mannequin`) should be **deleted** — `CLAUDE.md` already identifies it as genuinely unused, and it is a standing source of confusion with the load-bearing `Content/Animation/ZSAnims/`. |

### B2-T3 — Pipeline standards · **S (2 sessions)**

| Sub-task | Definition of done |
|---|---|
| T3.1 | **Naming conventions** for content extended beyond `CLAUDE.md`'s ZS rule to cover environment assets (`SM_`, `M_`, `MI_`, `T_`, prefixes plus a kit-scoped folder layout). |
| T3.2 | **Material standards**: master materials + instances only. A hard cap on unique master materials — this is the single biggest lever on draw calls and it is set here, not in B8. |
| T3.3 | **LOD policy** per asset class (screen sizes, reduction targets), plus whether Nanite is used at all. For a low-poly project targeting high zombie counts, Nanite's cost/benefit is genuinely unclear → OQ-B2-02. |
| T3.4 | **Collision policy** — simple collision on everything; complex-as-simple banned. Matters directly: hitscan, melee overlaps, AI navigation, and elevation floor detection all query it. |
| T3.5 | **Retarget pipeline validated** end to end: a new humanoid animation onto `SKEL_TFA_Mannequin`, documented step-by-step. This is the one shared retarget hub; the process must be repeatable without rediscovery. |
| T3.6 | Written **import checklist** — the artifact that makes B4 executable rather than exploratory. |

### B2-T4 — Reference room · **M (2–3 sessions)**

| Sub-task | Definition of done |
|---|---|
| T4.1 | One interior room built to final intended quality using only kit assets and T3's standards. |
| T4.2 | Lit for both the lit and dark (light-source-required) cases. |
| T4.3 | **Profiled** — draw calls, triangles, texture memory, frame cost captured on a packaged Development build. This becomes the per-room budget every B4 room is measured against. |
| T4.4 | Played in-engine at gameplay camera: navigate it, fight in it, loot in it. Readability confirmed under the actual camera, with a light source and without. |
| T4.5 | Time-to-build recorded honestly. **This is the number B4's XXL estimate gets validated against** — if a single room takes three sessions, the region scale in OQ-B4-01 needs revisiting immediately, not at B4 exit. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | Style readability at gameplay distance — mock a scene with placeholder blockouts in the chosen palette. | Interactables, items, and zombies are distinguishable from environment at max zoom-out. |
| **PT2** | End of T4 | Reference room played, not just viewed. Combat, looting, navigation, both lighting states. | Room is legible and fightable. Budgets are met. Build time is recorded. |

## Notes

- **Character art is not in this phase.** B6 owns character creation and its modular character needs. B2 sets the standards those inherit.
- **B2 does not build the region.** If region geometry starts appearing during B2, that is scope creep — the entire point is to decide once, then execute in B4.
- **Audio is not here.** B7 owns it. B2 only notes which surfaces need footstep material types (a T3.4 collision-policy rider), because that data must be authored during B4, not retrofitted.
