# Phase P8 — Dynamic Events, Objectives & the Investigation Arc

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

This is the differentiator phase — the system PZ never shipped.

## Tasks
- [ ] `UZSEventDirector` (server): scheduled + random world events from data assets — helicopter flyover (drags hordes), distant gunshots/screams (ambient migration), crashed convoy/supply drop (timed loot beacon = risk/reward), house alarms. Expanded roster relative to PZ's own set — event variety is a stated priority, not a nice-to-have.
- [ ] Radio channel: scripted broadcast arc for days 1-7 (diegetic tutorial) that transitions into dynamic event/objective announcements *and* the first investigation-arc clues.
- [ ] Investigation/cure questline: notes, documents, and items scattered through the world let players piece together how the outbreak started and pursue leads toward a cure.
  - Per Decision 6, completion is an optional capstone (epilogue + persistent world-state change), never a forced ending.
  - Clue placement: each clue is a `UZSItemConfig` instance flagged as an investigation item, carrying a predetermined pool of eligible spawn locations (same location-tag system as regular loot, P6) — but placement is **guaranteed, not probabilistic**: the system always spawns each clue at exactly one randomly-chosen location from its pool, so a world can never end up missing a clue entirely.
- [ ] Radiant objective wrappers ("reach the drop before it's swarmed," "restore the station generator") — objectives are invitations with stakes, never mandatory quests.

## Exit criteria
Two co-op sessions on the same map play out differently because the director dealt different beats; a full playthrough of the investigation arc is possible and its ending behaves per Decision 6's resolution.
