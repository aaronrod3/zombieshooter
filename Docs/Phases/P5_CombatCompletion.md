# Phase P5 — Combat Completion (Melee + Full Loop Feel)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] Melee weapon type through the *same* `UZSWeaponConfig` pipeline (a melee config specifies swing montages, reach, stamina cost, damage, durability — the multi-weapon rule pays off here).
- [ ] Swing timing via the existing notify system; shove + stomp as always-available options; stamina economy tuned against P2.
- [ ] Firearms integration with noise + zombie mass: ammo scarcity tuning, simple hit-reaction/knockdown.
- [ ] Weapon durability-lite (melee breaks; no repair sim v1).

## Exit criteria
The PZ death loop exists — greed + noise + stamina mismanagement kills a player who had every tool to survive.
