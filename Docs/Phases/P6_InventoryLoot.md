# Phase P6 — Inventory, Loot & Scavenging

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.**

## Tasks
- [ ] `UZSInventoryComponent` (replicated), weight-based encumbrance, bag equip slots. **Not** the combat loadout slots (`PrimaryHand`/`SecondaryHand`/hotbar) — those are P5's (`Docs/Phases/P5_CombatCompletion.md`), since they're what P5's single Attack button dispatches on; this component is what they hold references into.
- [ ] `UZSItemConfig` grows into the general item contract with equip-only vs. carry-only categories, so weapons/armor claim dedicated slots while general loot doesn't.
- [ ] Container actors + data-asset loot tables keyed by building/container archetype, one item per container slot.
- [ ] Per-zone quality tiers (good/bad/in-between areas) with slight randomization, plus a finite world-count pool per rarity tier so genuinely rare items stay rare across a whole session, not just per-roll.
- [ ] Inventory UI (list-based dual-pane, bulk actions, favorite/junk) + radial quick-use, with the transparent stat-preview rule from P2 carried through every item tooltip.
- [ ] Dropped-item persistence in the running session.

## Exit criteria
Full scavenge loop in graybox: run out, loot under threat, haul back, stash; item scarcity feels intentional, not just random.
