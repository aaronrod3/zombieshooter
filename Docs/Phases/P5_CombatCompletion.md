# Phase P5 — Loadout & Unified Combat (Melee + Full Loop Feel)

> Plan of record: `Docs/GameDevPlan.md`. This file tracks build status for this phase only. Naming/tech-stack/replication conventions live in `CLAUDE.md` — not repeated here. Read `GameDevPlan.md` §7 for this phase's open questions before starting.

**Status: not started.** Expanded 2026-07-21 (dev request: a proven loadout system + one Attack button that changes behavior per equipped weapon) — was "Combat Completion" only, now also owns the equip-slot/hotbar loadout system, since that's what a single Attack button dispatches on. Full inventory (weight, containers, loot tables) stays P6's — this phase owns only the combat-facing slots that reference into it.

## Tasks

### Loadout system
- [ ] Two hand slots on `AZSPlayerCharacter`: `PrimaryHand` + `SecondaryHand`. Two-handed items occupy both; one-handed items can pair independently (P5 open question: is `SecondaryHand` ever active on its own, e.g. offhand pistol, or purely "the other grip" for v1?).
- [ ] Empty `PrimaryHand` = bare-fist unarmed, always available — the fallback that today's pre-P5 `Server_MeleeAttack`/`Unarmed*` tunables on `AZSPlayerCharacter` already implement. Player starts unequipped by default.
- [ ] Real-time hotbar (no pause/menu, per Decision 1) bound to specific inventory item references for instant re-equip — size/key-scheme is an open question (§7).
- [ ] Equip/holster/switch takes real time via the existing notify/montage/`bIsBusy` choreography system (same pattern `Server_StartReload` already uses), not instant.
- [ ] Hotbar/hand-slots reference into P6's `UZSInventoryComponent` rather than duplicating item state — this task has a real dependency on P6 existing for the item data, even though the slot/dispatch machinery itself lives here.

### Unified attack input
- [ ] Generalize `UZSWeaponConfig` to cover melee weapons too via a new `EZSAttackType { Unarmed, Melee, Ranged }` field, rather than a parallel melee-config data asset — consistent with the existing multi-weapon rule.
- [ ] `Server_Attack()` (replaces the placeholder `Server_MeleeAttack` built pre-P5) dispatches on `PrimaryHand`'s config: null → bare-fist (unchanged); `Melee` → swing using that weapon's own damage/range/interval (not the flat `Unarmed*` tunables); `Ranged` → today's `Server_Fire` hitscan logic.
- [ ] `HandleFireStarted`'s auto-fire timer plumbing moves under the same dispatch — a `Ranged` weapon with `Auto` in `SupportedFireModes` still holds-to-fire; melee/unarmed stays one swing per press. This is what retires the separate `IA_Fire` action in favor of `IA_Attack` alone.
- [ ] Melee weapon types through this pipeline: swing timing via the existing notify system; shove + stomp as always-available options; stamina economy tuned against P2's `UZSNeedsComponent`.
- [ ] Firearms integration with noise + zombie mass: ammo scarcity tuning, simple hit-reaction/knockdown.
- [ ] Weapon durability-lite (melee breaks; no repair sim v1).

## Pre-P5 groundwork already built (2026-07-21, to unblock P3/P4 testing before this phase started)
`AZSPlayerCharacter::Server_MeleeAttack` (bare-fist sphere-overlap melee) and `Server_Fire`'s hitscan both exist and work today, bound to separate inputs (`IA_Attack` → melee, `IA_Fire` → gunfire) with no dispatch between them yet. `IA_Attack` was named ahead of this phase specifically so the dev's placeholder input action wouldn't need a second rename once real dispatch lands. See `Docs/Phases/P4_Zombies.md` and `Docs/SessionHandoff.md` for what's already PIE-testable.

## Exit criteria
A player starts unarmed, equips a weapon from the hotbar in real time, and one Attack button does the right thing whether they're bare-handed, swinging a bat, or holding a rifle. On top of that, the PZ death loop exists — greed + noise + stamina mismanagement kills a player who had every tool to survive.
