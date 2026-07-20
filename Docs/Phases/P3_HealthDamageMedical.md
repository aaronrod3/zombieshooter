# Phase P3 — Health, Damage & Medical-Lite

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] `UZSHealthComponent`: 4 zones (Head, Torso, Arms, Legs). Wound types: scratch/bite/laceration/fracture, each mapped to a concrete gameplay effect (leg wounds → mobility/speed, arm wounds → attack speed/reload time). Bleed-over-time. All damage through `TakeDamage`.
- [ ] Treatment actions (montage + notify-gated, reusing the existing action system): bandage (cleanliness flag), disinfect, splint.
- [ ] Knox-style infection: bite → hidden infection roll → delayed queasy→fever→death arc, deliberately UI-ambiguous vs. ordinary sickness.
- [ ] Emergency amputation (new mechanic): removing a bitten/infected limb before the infection timer completes stops that infection source outright, at the cost of permanent capability loss to that limb (permanent version of the zone-mapping penalties above). Build the simplest version first — any bladed/tool item, solo-capable, works any time before the infection timer expires — refine once played. Open questions (tool requirement, co-op-assist-only, timing window) are in `GameDevPlan.md` §7.
- [ ] Player death → spectate/respawn-as-new-character flow (permadeath groundwork).

## Exit criteria
A scripted damage source can wound (with the correct gameplay-effect mapping), infect, and kill a player who mismanages treatment — and a player who amputates in time survives a bite that would otherwise have killed them. Second client sees everything correctly.
