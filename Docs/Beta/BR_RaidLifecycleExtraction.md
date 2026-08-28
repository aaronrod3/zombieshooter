# BR — Raid Lifecycle & Extraction

**Stage 1 — Core Playable Loop.** **Size: L (estimate, unscoped precedent — see Notes)** · **Gate: `[INTERNAL]`** · **Depends on: `BH`-T1 (stash exists to bank loot into)** · **Blocks: `B3` (save schema needs the raid/session shape settled), `B4` (world systems interact with whatever "entering a raid" actually is)**

> **New phase, CR-13 (`00_MasterPlan.md` §2, extraction pivot 2026-08-27).** The other new half of the hub-and-raid loop: how a raid starts, reseeds, and ends (by extraction or by death). First code landed the same day as the pivot — `AZSExtractionPointActor`, `AZSPlayerCharacter::Server_RequestExtraction`/`Server_LeaveRaidAndReturnToHub`, `AZSGameMode::Server_ReturnPlayerToHub`, `AZSGameState::Server_StartRaidReseed` — all additive, all deliberately stopping at an honest content/architecture gap rather than guessing past one. This file is the scoping pass, per `02_MasterWorkflow.md` §3 Step 3.
>
> **The single highest-leverage open question in this phase is `OQ-BR-03`** (does entering the zone mean a level reload, or does the zone stay persistently loaded?) — it isn't answered yet, and a wrong guess here would ripple into `B3`'s save topology and `B4X`'s content architecture. Resolve it before building `BR-T3`'s reseed mechanism out further than what already exists.

## Entry criteria

- [x] `BH-T1` (stash/currency backend) exists — extraction has somewhere real to bank loot.
- [ ] **OQ-BR-01 (BLOCKING)** — can one player leave a shared listen-server raid (by extracting or dying) without ending the session for teammates still playing? `AZSGameMode::Server_ReturnPlayerToHub`'s own code comment already flags this as genuinely undecided at the architecture level, not just unbuilt.
- [ ] **OQ-BR-03 (BLOCKING)** — does a fresh raid entry mean a level reload (clean reseed, but dropped-loot persistence must survive across reloads via save data — a `B3` dependency) or does the zone stay continuously loaded (reseed needs an explicit re-roll call against live actors, no reload)? This determines the actual shape of `BR-T3`.
- [ ] `OQ-BH-01` answered — walkable-vs-menu hub determines what "return to hub" literally transitions to.

## Exit criteria

- [ ] A player can walk to an extraction point, interact, and successfully bank carried loot while returning to the hub alive.
- [ ] A player who dies has their loot dropped at the death location (already true — `Server_HandleDeathLootAndZombie`) and returns to the hub as a genuinely fresh mercenary (skills/XP reset per Decision 8; the fresh character's actual starting kit is a real, deliberate answer, not the pre-pivot `StartingHotbarLoadout` behavior left unexamined).
- [ ] Raid reseed observably reseeds something a player can notice — not just the rarity-pool reset and utilities-hazard roll that already exist in code, but at least one visible consequence (loot placement or zombie density) once `B4X` content exists to reseed against.
- [ ] Multiple extraction points exist and are all independently functional.
- [ ] `OQ-BR-01` is resolved **and implemented**, not just documented as an open gap.
- [ ] `SessionHandoff.md` shows zero "built but unverified" BR items.

---

## Task breakdown

### BR-T1 — Raid session lifecycle · **M**

| Sub-task | Definition of done |
|---|---|
| T1.1 | ✅ **Done, 2026-08-27.** `Server_RequestExtraction`, the shared `Server_LeaveRaidAndReturnToHub(bool bWasExtraction)`, and `AZSGameMode::Server_ReturnPlayerToHub` hook — currently falls back to the pre-pivot in-zone `RestartPlayer` for both death and extraction, by design, until `OQ-BR-01`/`OQ-BH-01` are answered. |
| T1.2 | Resolve `OQ-BR-01` and implement the real hub-transition mechanism once it's answered — this is the one call site (`Server_ReturnPlayerToHub`) that needs to change. |
| T1.3 | **New-mercenary-at-hub creation flow**: what does a fresh character actually start with? Ties directly into `BH`'s vendor system (buy a starting kit?) and `B6-Content`'s character-creation/background work — don't design this in isolation from either. |
| T1.4 | Raid-entry flow: choosing an entry point at the hub, leaving the hub, spawning in the zone with whatever loadout `T1.3` produced. |

### BR-T2 — Extraction points · **S**

| Sub-task | Definition of done |
|---|---|
| T2.1 | ✅ **Done, 2026-08-27.** `AZSExtractionPointActor`, reusing `UZSInteractableComponent` exactly like `AZSContainerActor` — no new interaction path. |
| T2.2 | Multiple extraction points placed and independently tested (needs at least graybox zone content — the existing small test area is sufficient before `B4X` content exists). |
| T2.3 | **`OQ-BR-02` (SEQUENCEABLE)** — should extraction have a delay/channel time? An instant, uninterruptible extract undermines the "last moments of a raid are the tensest" pattern this genre relies on. Recommend a short channel (interruptible by taking damage), but this needs a real decision, not an assumption. |
| T2.4 | Extraction-point availability/rotation — are all points always live, or does access to some require content that doesn't exist yet (a vehicle, a key)? If the latter, that's a `BV`/future hook, not this phase's scope — don't build toward it speculatively. |

### BR-T3 — Raid reseed · **M** · *shape depends on `OQ-BR-03`*

| Sub-task | Definition of done |
|---|---|
| T3.1 | ✅ **Done, 2026-08-27.** `AZSGameState::Server_StartRaidReseed` — restores the loot-rarity pool to its authored defaults and rolls a fresh per-raid utilities hazard (Decision 11). Not yet wired to anything that calls it. |
| T3.2 | Wire `Server_StartRaidReseed` to an actual "a raid just started" event — the trigger point itself depends on `OQ-BR-03`'s answer (a level-load hook vs. an explicit per-player/per-party raid-begin call on a persistently-loaded zone). |
| T3.3 | Zombie density/placement reseed — needs real spawn-volume content (`B4X`'s job); this task only needs the hook to exist, not the content behind it. |
| T3.4 | Loot-container content reseed — `AZSContainerActor` currently rolls its `ContainerSlots` once at `BeginPlay` and never again, which was correct for a single persistent world and is not correct for a reseeding raid. The fix's shape depends entirely on `OQ-BR-03`: a level-reload model reseeds this for free; a persistently-loaded zone needs an explicit re-roll entry point added to `AZSContainerActor`. Don't build this until `OQ-BR-03` is answered. |

### BR-T4 — Permadeath & hub-return · **S** · *mostly done*

| Sub-task | Definition of done |
|---|---|
| T4.1 | ✅ **Done, pre-existing + reused.** Death already drops every carried instance at the death location (`Server_HandleDeathLootAndZombie`) before routing through the same `Server_LeaveRaidAndReturnToHub(false)` extraction now shares. |
| T4.2 | Skill/XP reset on death — currently a genuine no-op, since no skill system exists yet (`B6-Sys`). Flagged here as a forward hook: whenever `B6-Sys` lands, wherever it stores skill XP must live on the character/PlayerState, never on `UZSHubSubsystem`, or Decision 8's "full reset" breaks silently. |
| T4.3 | **Re-examine `BP_ZS_PlayerCharacter::StartingHotbarLoadout`** — it currently re-grants the identical starting gear to every fresh character, which was correct under the pre-pivot "new character, same world" design and is a real open question under "new mercenary from scratch" (should a fresh mercenary start bare, or with a hub-purchased starter kit per `T1.3`?). Content/design work, not guessed here. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T2 | **Solo extraction round-trip.** Enter, loot, walk to an extraction point, interact. | Stash gains exactly what was carried; character resets cleanly. |
| **PT2** | End of T4 | **Solo death round-trip.** Die carrying loot. | Loot is on the ground at the death location on a later visit; the new mercenary starts genuinely fresh (skills/XP, per Decision 8). |
| **PT3** | End of T3 | **Reseed verification.** Note the loot/hazard state on one raid entry, extract, re-enter. | Reseed-scoped state (loot tables, hazard roll) changed; persistence-scoped state (anything dropped by a dead character) didn't get wiped by the reseed. |
| **PT4** | BR exit, needs 2 humans | **Shared-raid departure.** One player extracts or dies while another continues in the same raid. | The remaining player's session is undisturbed — this is `OQ-BR-01` tested for real, not just documented. |

## Notes

- **No historical size basis** — same caveat as `BH`; re-forecast after `BR-T1`/`T3` land for real.
- **`OQ-BR-03` is the load-bearing decision in this entire phase.** Everything in `BR-T3`, and a meaningful slice of `B3`'s save topology and `B4X`'s content architecture, waits on it. Resolve it in the same session as `OQ-BR-01`, not separately.
- **Ability/support-strike consumable *effects*** (spawning the actual airstrike/care-package actor in the raid) belong here, once `BH-T2.4` makes the item purchasable — the item existing and the item doing something in-raid are two different tasks, split across the two phases on purpose.
