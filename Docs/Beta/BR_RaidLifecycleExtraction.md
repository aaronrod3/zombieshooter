# BR — Raid Lifecycle & Extraction

**Stage 1 — Core Playable Loop.** **Size: L (estimate, unscoped precedent — see Notes)** · **Gate: `[INTERNAL]`** · **Depends on: `BH`-T1 (stash exists to bank loot into)** · **Blocks: `B3` (save schema needs the raid/session shape settled), `B4` (world systems interact with whatever "entering a raid" actually is)**

> **⚑ PIVOT NOTICE, 2026-08-29 (`00_MasterPlan.md` CR-14) — "raid" as a bounded, reloaded session is retired.** Two days after this phase's original scoping, the dev reversed the session-boundary half of the extraction loop: the quarantine zone and the hub (Grayback Lodge, `BH_HubHideoutEconomy.md`) are now **one continuous, persistent, never-reloaded world**. Players drive or walk between them freely and independently — no party lockstep, no level reload, no "raid ends" moment. This phase's name and most of its original task shape (below) describe the **retired** model; **treat every task marked ✅ Done as still real, working code that now needs rework or removal, not as validated-and-final** — none of it has been touched yet, this file is a design-record update only (see `00_MasterPlan.md` CR-14's own "Not yet touched" line).
>
> **What survives from the original scope, essentially unchanged:** permadeath itself (`BR-T4` below) — a character still loses skills/XP/everything carried on death, dropped loot still stays exactly where it fell. What's retired: the raid-session-boundary machinery built around it — `AZSGameState::IsRaidOver`/`Server_CheckRaidEndAndReset`, the spectate-until-raid-ends flow (`Server_EnterSpectatorAfterDeath`), and the level-reload-per-entry reseed model (`OQ-BR-03`, `BR-T3` below). What's new: a spawn-director + horde-migration system for zombies and a finite, non-replenishing loot economy (`OQ-BR-05`, new `BR-T3` below), plus a first slice of drivable vehicles (`OQ-BR-06`, new `BR-T5` below).
>
> **One assumption flagged, not yet dev-confirmed** (`OQ-BR-04`): with no raid to extract *from*, the extraction-point ceremony (`AZSExtractionPointActor`, `Server_RequestExtraction`) likely has no reason to exist as a separate mechanic — carried loot probably just becomes safe once physically deposited in the stash at the compound, an ordinary container-deposit action. `BR-T2` below is written against this assumption but flagged, not committed — confirm before deleting `AZSExtractionPointActor`.

## Entry criteria

- [x] `BH-T1` (stash/currency backend) exists — banking loot has somewhere real to go.
- [x] **OQ-BR-01 (BLOCKING)** — ⚠️ MOOT 2026-08-29 (see pivot notice above): the question ("can one player leave a shared raid without ending it") no longer applies once there's no raid session to leave or end. The spectate-until-raid-ends implementation this produced is believed obsolete, not yet removed.
- [x] **OQ-BR-03 (BLOCKING)** — ⚠️ SUPERSEDED 2026-08-29 (see pivot notice above): **persistently loaded, never reloaded** — reverses the 2026-08-28 "level reload each raid entry" answer entirely.
- [x] `OQ-BH-01` — ⚠️ SUPERSEDED 2026-08-29: walkable physical hub (Grayback Lodge), not menu-driven — see `BH_HubHideoutEconomy.md`.
- [x] **OQ-BR-05 (BLOCKING)** — ✅ RESOLVED 2026-08-29: zombie spawn-director + horde migration; finite non-replenishing loot. Feeds the new `BR-T3` below.
- [x] **OQ-BR-06 (BLOCKING)** — ✅ RESOLVED 2026-08-29: basic drivable vehicles now, full `BV` depth (fuel/damage/combat) stays deferred. Feeds the new `BR-T5` below.
- [ ] **OQ-BR-04 (SEQUENCEABLE, flagged)** — does the extraction-point ceremony still exist? Needed before `BR-T2` can be finalized, not before this phase starts.

## Exit criteria

- [ ] A player can carry loot back to the compound and deposit it into the stash — whether that's a plain container-deposit interaction or a retained extraction-style ceremony depends on `OQ-BR-04`.
- [ ] A player who dies has their loot dropped at the death location (already true — `Server_HandleDeathLootAndZombie`) and a fresh mercenary starts at the compound (skills/XP reset per Decision 8; the fresh character's actual starting kit is a real, deliberate answer, not the pre-pivot `StartingHotbarLoadout` behavior left unexamined).
- [ ] The zombie spawn-director observably keeps regions the player isn't looking at populated, and at least one horde migration is observable crossing the map, once `B4X` content exists to test either against.
- [ ] At least one loot container demonstrably does NOT replenish after being emptied, confirming the non-respawn model actually holds.
- [ ] A vehicle can be entered, driven out of the Motor Barn, driven back in, and exited, with no fuel/damage/combat modeling required yet.
- [ ] `AZSGameState::IsRaidOver`/`Server_CheckRaidEndAndReset` and the spectate-until-raid-ends flow are either removed or repurposed — not left in place as dead/misleading code once this phase's implementation pass starts.
- [ ] `SessionHandoff.md` shows zero "built but unverified" BR items.

---

## Task breakdown

### BR-T1 — Raid session lifecycle · **M** · ⚠️ **RETIRED 2026-08-29 — no more raid session to have a lifecycle** (see pivot notice)

| Sub-task | Definition of done |
|---|---|
| T1.1 | ⚠️ Built 2026-08-27, believed obsolete 2026-08-29. `Server_RequestExtraction`, `Server_LeaveRaidAndReturnToHub(bool bWasExtraction)`, `AZSGameMode::Server_ReturnPlayerToHub` — depends on `OQ-BR-04`'s outcome whether any of this survives in a repurposed form (a plain stash-deposit call) or gets deleted outright. |
| T1.2 | ⚠️ Built 2026-08-28, believed obsolete 2026-08-29. `AZSGameState::IsRaidOver`/`Server_CheckRaidEndAndReset`, the `bWasExtraction` branch in `Server_ReturnPlayerToHub`, and `Server_EnterSpectatorAfterDeath`'s spectate-until-raid-ends flow — all premised on a raid having an "end" to detect. With no session boundary, there's nothing for this to check. Not yet removed from code (design-record-only pass, `00_MasterPlan.md` CR-14). |
| T1.3 | **Still real, unaffected by the pivot.** New-mercenary-at-compound creation flow: what does a fresh character actually start with? Ties directly into `BH`'s vendor system (buy a starting kit?) and `B6-Content`'s character-creation/background work. |
| T1.4 | ⚠️ Reframed. "Raid-entry flow: choosing an entry point, leaving the hub" no longer applies — there's no entry point to choose, a player just walks or drives out of the compound into the persistent zone with whatever loadout `T1.3` produced. Simpler than originally scoped, not harder. |

### BR-T2 — Extraction points · **S** · flagged pending `OQ-BR-04`

| Sub-task | Definition of done |
|---|---|
| T2.1 | ⚠️ Built 2026-08-27, flagged. `AZSExtractionPointActor` (reuses `UZSInteractableComponent` exactly like `AZSContainerActor`) — kept or deleted depending on `OQ-BR-04`'s answer. If retired, its job is fully absorbed by an ordinary stash-deposit interaction at the Cache (`BH_HubHideoutEconomy.md`'s Grayback Lodge structures). |
| T2.2 | Superseded if `OQ-BR-04` retires the ceremony — "multiple extraction points" stops being meaningful once there's one stash you can always walk or drive back to. |
| T2.3 | **`OQ-BR-02`** — likely moot for the same reason (see its own entry in `90_OpenQuestions.md`) — a channel-time question only matters if there's a discrete extract action left to gate. |
| T2.4 | Superseded — "extraction-point availability tied to a vehicle" doesn't apply once vehicles are just free transport in and out of one continuous world, not a gate on a discrete extraction action. |

### BR-T3 — World threat & loot director · **M** · retitled 2026-08-29 (was "Raid reseed") · **`OQ-BR-05` resolved: spawn-director + horde migration (zombies), finite non-replenishing loot (items)**

| Sub-task | Definition of done |
|---|---|
| T3.1 | ⚠️ Built 2026-08-27 for the retired reseed model. `AZSGameState::Server_StartRaidReseed` (loot-rarity pool restore + per-raid utilities hazard roll) — the utilities-hazard half needs its own rework (a hazard that rolls once per raid-entry has no trigger left to fire on; likely becomes a periodic real-time/in-world-day roll instead, not yet designed). The loot-rarity-pool-restore half is superseded outright by the new non-replenishing model below — restoring the pool contradicts "containers do not replenish." |
| T3.2 | **New.** Zombie spawn-director: a system that spawns zombies into map regions outside every connected player's current view/perception, keeping population up without visible pop-in. Needs real spawn-volume content (`B4X`'s job) and a "is any player looking at this region" query — likely built on the same perception primitives `AZombieAIController`'s `AIPerception` already uses, just inverted (querying player cameras, not zombie senses). |
| T3.3 | **New.** Horde migration: a separate system that periodically routes a horde (a real, moving group of zombies, not just a density bump) along a path through the map, independent of player position — a roaming threat a player can stumble into or deliberately avoid. |
| T3.4 | **New.** Non-replenishing loot: `AZSContainerActor`'s existing `BeginPlay`-time `ContainerSlots` roll needs to become a one-time roll tied to real save data (a slice of `B3` pulled forward, same as the old reseed model needed for dropped-loot persistence) rather than something that re-rolls on any kind of reload — there is no reload left to re-roll on, but the roll-once-ever semantics need an explicit persisted "has this container already been rolled" flag, which doesn't exist yet. |

### BR-T4 — Permadeath & compound-return · **S** · *mostly done, unaffected in substance by the pivot* (retitled from "hub-return," same scope)

| Sub-task | Definition of done |
|---|---|
| T4.1 | ✅ **Done, pre-existing + reused, still correct.** Death drops every carried instance at the death location (`Server_HandleDeathLootAndZombie`) — under CR-14 this loot simply sits in the one persistent world until any future character walks or drives back to recover it, no raid-boundary bookkeeping needed. |
| T4.2 | Skill/XP reset on death — currently a genuine no-op, since no skill system exists yet (`B6-Sys`). Flagged here as a forward hook: whenever `B6-Sys` lands, wherever it stores skill XP must live on the character or a non-hub-persistent part of `AZSPlayerState`, never mixed into the hub stash/currency fields that survive death (`Currency`/`Stash`, `BH-T1.4`), or Decision 8's "full reset" breaks silently. |
| T4.3 | **Re-examine `BP_ZS_PlayerCharacter::StartingHotbarLoadout`** — it currently re-grants the identical starting gear to every fresh character, which was correct under the pre-pivot "new character, same world" design and is a real open question under "new mercenary from scratch" (should a fresh mercenary start bare, or with a hub-purchased starter kit per `T1.3`?). Content/design work, not guessed here. |
| T4.4 | ⚠️ Simplified by the pivot. "Returns to the hub as a genuinely fresh mercenary" no longer needs a travel/state-transition step (`Server_ReturnPlayerToHub`'s old job) — since the compound is just a place in the persistent world, a fresh character simply spawns there directly, same as `RestartPlayer` already does for any respawn. |

### BR-T5 — Basic drivable vehicles · **M** · **new 2026-08-29, `OQ-BR-06` resolved: transport-only slice, not full `BV`**

| Sub-task | Definition of done |
|---|---|
| T5.1 | A vehicle actor/pawn class (likely `AZSVehicle`, sibling naming to the rest of the `ZS`-prefixed convention) with real drivable movement — no fuel, damage, or combat modeling. |
| T5.2 | Enter/exit seat interaction, reusing `UZSInteractableComponent` the same way every other interactable in this project does — no new interaction path needed. |
| T5.3 | At least one vehicle parked/spawnable at the Motor Barn (`BH_HubHideoutEconomy.md`'s Grayback Lodge structures) that can be driven out into the persistent zone and back in. |
| T5.4 | Explicitly out of scope for this slice, deferred to `BV`'s own pass (`CR-02`, unscheduled): fuel, damage/durability, vehicle combat (running over zombies, drive-by), cargo/storage integration with `EZSContainerType::Vehicle`'s already-reserved category. Don't build toward these speculatively. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2/`OQ-BR-04` | **Solo loot round-trip.** Loot out in the zone, walk or drive back to the compound, deposit into the stash. | Stash gains exactly what was carried; no separate "extract" action is needed unless `OQ-BR-04` decided to keep one. |
| **PT2** | End of T4 | **Solo death round-trip.** Die carrying loot. | Loot is on the ground at the death location on a later visit; the new mercenary starts genuinely fresh (skills/XP, per Decision 8) directly at the compound, no travel step. |
| **PT3** | End of T3 | **World-director verification.** Clear zombies from a region, look away/leave, come back later; separately, observe the map for a horde migration; separately, fully empty one container. | The cleared region has repopulated some zombies without an obvious "pop-in" moment; a horde is observed moving through the map on its own; the emptied container stays empty (no replenish). |
| **PT4** | End of T5 | **Vehicle round-trip.** Drive a vehicle out of the Motor Barn, out into the zone, back in. | Movement feels real (not a placeholder teleport); no fuel/damage/combat systems are required to pass this. |
| **PT5** | Needs 2+ humans | **Independent-movement verification.** One player drives out into the zone while another stays at the compound, simultaneously. | Both players' experiences are fully undisturbed by the other's location — `00_MasterPlan.md` CR-14's party-independence answer tested for real, not just documented. Replaces the old `PT4` "shared-raid departure" check, which assumed a session boundary that no longer exists. |

## Notes

- **No historical size basis** — same caveat as `BH`; re-forecast after `BR-T3`/`T5` land for real (the old `BR-T1`/`T2` estimates no longer apply to retired scope).
- **`OQ-BR-03` was the load-bearing decision in this entire phase, twice now** — resolved 2026-08-28 (level reload), then reversed 2026-08-29 (persistently loaded, see `00_MasterPlan.md` CR-14). The persistence work `B3`/`B4X` need is unchanged in kind (loot/state needs to survive somehow) but simpler in shape now — there's no reload event to survive across, just an ordinary always-on world to save.
- **Ability/support-strike consumable *effects*** (spawning the actual airstrike/care-package actor) belong here, once `BH-T2.4` makes the item purchasable — the item existing and the item doing something in the zone are two different tasks, split across the two phases on purpose. Unaffected by this pivot.
- **This phase's name ("Raid Lifecycle & Extraction") is now a historical artifact** of CR-13's original framing — not renamed in this pass to avoid invalidating cross-references (`BR-T*` task IDs, other files' links to this one), same discipline the project already applies to phase-ID suffixes elsewhere (`B4X`, `BV`). If this phase gets a fuller rewrite later, renaming it to something like "World Systems & Vehicles" would be more honest to its actual post-CR-14 scope.
