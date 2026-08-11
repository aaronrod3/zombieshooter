# B2 — Art Direction Lock & Asset Pipeline

**Stage 2 — Content, Depth & Release.** **Size: M (6–8 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B0** · **Blocks: B4X (hard)**

> **Updated 2026-07-26** (`Docs/Planning/RescopeQuestionnaire.md`): **OQ-B2-01 resolved** — "mostly free, cheaper assets, want Door Kickers 2 level of detail so I can create assets myself later." Budget assumption for this phase is modest/free-first, not a marketplace-kit purchase; DK2 is the fidelity benchmark, and expect the Blender pipeline (`GameDevPlan.md` §5) to matter more than originally assumed, not less.

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
| T2.4 | Existing placeholder content triaged. Specifically: the dead raw Lyra/ShooterGame import — `Content/LyraAnims/` (references a never-migrated `SK_Mannequin`, verified still present 2026-08-03) — should be **deleted** — `CLAUDE.md` already identifies it as genuinely unused, and it is a standing source of confusion with the load-bearing `Content/Animation/ZSAnims/`. |

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

---

## Manual setup steps

Dev-only, non-scriptable steps (see `Docs/Beta/README.md`'s convention note). Almost all of B2 falls in this category — mood boards, kit selection, material/LOD/collision policy, and the reference room are editor + creative-judgment work with no away-session/MCP path (confirmed 2026-08-04: T1 and T2.1 are the dev's own taste/sourcing calls by design, T3 is deliberately parked for a future session rather than pre-drafted). **Format**: each task entry is a running **Completed** list (brief, one line each) followed by **Next steps** (full click-by-click detail). When a next step finishes, its detail comes out of Next steps and a one-line summary gets appended to Completed above it.

### B2-T1 — Direction lock

**Completed:** none yet.

**Next steps:**
1. Assemble the mood board (20-30 references: exterior day, exterior night, interior lit, interior dark, one horde shot). Commit to `Docs/Art/` (doesn't exist yet — create it).
2. Define the palette: base environment ramp, restricted accent set, and a readability palette for gameplay-critical objects (interactables/items/zombies) that must never blend into environment tones at top-down distance.
3. Run the top-down readability test — silhouette/value contrast at actual gameplay camera distance, at both zoom extremes from B0-T3.1.
4. Decide the lighting direction, including the darkness-mechanic value (how dark is "requires a light source") — this is a gameplay value B4 builds against, not just an aesthetic one.

### B2-T2 — Kit selection & acquisition

**Completed:** none yet. (`Docs/Planning/B2_KitCandidates_2026-08-03.md` has preliminary research if useful — not a required starting point.)

**Next steps:**
1. Browse and choose the modular kit(s) against T1's direction, covering the brief: small-town US buildings, forest, rural fringe, interiors.
2. **Before importing anything paid, add it to `.gitignore` first** — not after. Precedent: `Content/InfimaGames/` is already gitignored for the same reason ($0 LFS budget, paid content never committed).
3. Once a kit's chosen: coverage-gap analysis — what it doesn't include, and whether each gap gets bought, Blender-modeled, or cut from scope.
4. **Delete `Content/LyraAnims/`** (the dead raw Lyra/ShooterGame import, references a never-migrated `SK_Mannequin` — verified still present 2026-08-03). Already filed as [issue #19](https://github.com/aaronrod3/zombieshooter/issues/19). Standalone — doesn't block on or wait for the kit decision above.

### B2-T3 — Pipeline standards

**Parked on purpose (2026-08-04)** — naming conventions, the master-material cap, LOD screen-size bands, collision policy, the retarget-pipeline validation, and the import checklist all need real decisions, but the dev wants this as its own dedicated session to learn/research the area together rather than have numbers pre-drafted now. Revisit explicitly, don't guess past it.

### B2-T4 — Reference room

**Completed:** none yet. Blocked on T2/T3 being far enough along to build against.

**Next steps (once unblocked):**
1. Build one interior room to final intended quality using only kit assets and T3's standards.
2. Light it for both the lit and dark (light-source-required) cases.
3. Profile it on a packaged Development build — draw calls, triangles, texture memory, frame cost. This becomes the per-room budget every B4 room is measured against.
4. Play it at gameplay camera: navigate, fight, loot, both lighting states.
5. Record time-to-build honestly — this is the number B4's XXL region-scale estimate (`OQ-B4-01`) gets validated against.
