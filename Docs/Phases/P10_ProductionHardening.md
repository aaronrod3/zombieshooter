# Phase P10 — Production Hardening → Public Vertical Slice

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] Audio pass (see `GameDevPlan.md` §5 asset list).
- [ ] VFX pass (low-poly-friendly: flat-shaded blood/muzzle/impact).
- [ ] Performance pass: zombie + hostile-roamer count profiling, LODs/HISM on the kit, net relevancy.
- [ ] Fixed-tick save safety, crash/soak testing.
- [ ] Packaged Windows build tested over real LAN/direct-IP.
- [ ] Trailer-able vertical slice: 20-40 minutes of tuned co-op survival on the real map, including at least one meta event and a taste of the investigation arc.

## Exit criteria
Shippable demo build.

## First post-v1 addition
Hostile human roamers, built cheaply on top of P4's zombie AI architecture (Decision 5). Planning pass after that picks from: full NPC survivors/factions, vehicles, sandbox sliders, deeper seasons/temperature, Steam/EOS + dedicated server, other deferred-pool systems.
