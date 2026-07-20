# Phase P7 — World Building & Persistence

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] Art integration phase: replace graybox with the chosen modular kit (dark, earthy, low-poly — see `GameDevPlan.md` §5/Decision 3).
- [ ] Build the region (World Partition): residential streets, main-street commercial row (hardware/pharmacy/gun store/grocery — the loot-archetype anchors), gas station, church, farm fringe, mountain/forest terrain, at least one larger "dense town" analog.
- [ ] Multiple profession-tied spawn points (Decision 4) placed across the map, with the "scatter spawns" co-op toggle wired.
- [ ] Enterable buildings as the rule; interior visibility solution for top-down (roof fade/cutaway — prototype early in P1 if it worries us).
- [ ] Save/persistence v1 (single "world continues" save per server): world item/container/door state, character sheets, clock, zombie population coarse state. Host-side SaveGame; permadeath = character deleted, world persists.
- [ ] Utilities shutoff goes live against the real map (powered lights/fridges/pumps flip off).

## Exit criteria
The real map plays end-to-end co-op, including a scattered multi-spawn start; quit → relaunch → world remembered; day ~10 the lights die.
