# BR — Raid Lifecycle & Extraction

**Stage 1 — Core Playable Loop.** **Size: L (estimate, unscoped precedent — see Notes)** · **Gate: `[INTERNAL]`** · **Depends on: `BH`-T1 (stash exists to bank loot into)** · **Blocks: `B3` (save schema needs the raid/session shape settled), `B4` (world systems interact with whatever "entering a raid" actually is)**

> **New phase, CR-13 (`00_MasterPlan.md` §2, extraction pivot 2026-08-27).** The other new half of the hub-and-raid loop: how a raid starts, reseeds, and ends (by extraction or by death). First code landed the same day as the pivot — `AZSExtractionPointActor`, `AZSPlayerCharacter::Server_RequestExtraction`/`Server_LeaveRaidAndReturnToHub`, `AZSGameMode::Server_ReturnPlayerToHub`, `AZSGameState::Server_StartRaidReseed` — all additive, all deliberately stopping at an honest content/architecture gap rather than guessing past one. This file is the scoping pass, per `02_MasterWorkflow.md` §3 Step 3.
>
> **`OQ-BR-03` and `OQ-BR-01`, the two highest-leverage open questions in this phase, were both resolved 2026-08-28** (level reload each raid entry; level-streamed private sub-area for a solo leave/death) — see Entry criteria below. Neither is implemented yet; `BR-T1.2`/`T3.2` are the concrete next steps.

## Entry criteria

- [x] `BH-T1` (stash/currency backend) exists — extraction has somewhere real to bank loot.
- [x] **OQ-BR-01 (BLOCKING)** — ✅ RESOLVED and IMPLEMENTED 2026-08-28, superseding the "level-streamed private sub-area" framing below with something simpler the dev proposed directly: **a player who dies spectates the rest of the party until the raid ends (everyone else also dies or extracts); an extracted player is already pawn-less and goes straight to the (menu-driven, `OQ-BH-01`) hub UI.** No level travel needed for either case — both stay connected to the same raid level, just without a pawn in it. This also resolves `OQ-BR-03` differently than originally framed: since nobody with an active gameplay stake is ever disturbed, a real level reload becomes safe to trigger at exactly one moment — when the raid is over for everyone (`AZSGameState::IsRaidOver`/`Server_CheckRaidEndAndReset`, `AZSGameMode::Server_ReturnPlayerToHub`) — not per-player, on demand. See BR-T1.2 below for what's actually built vs. still deferred (a real `ServerTravel` reload is NOT part of this yet, on purpose).
- [x] **OQ-BR-03 (BLOCKING)** — ✅ RESOLVED 2026-08-28, reframed by the same discussion that resolved `OQ-BR-01` above: the level reload now happens once, at whole-raid-end (last living player dies or extracts), not per-player on demand. Consequence unchanged: dropped-loot persistence across raids (already a confirmed CR-13 requirement) needs real save data to survive that reload once it's actually implemented, pulling a slice of `B3` forward. `BR-T3` builds against this model. The reload itself is not yet implemented (`Server_CheckRaidEndAndReset` currently reseeds in place via `Server_StartRaidReseed`, not a real `ServerTravel` — see BR-T1.2's own note on why that's deliberately deferred).
- [x] `OQ-BH-01` answered — ✅ menu-driven, no hub level.

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
| T1.2 | ✅ **Done, 2026-08-28.** `AZSGameMode::Server_ReturnPlayerToHub` now branches on `bWasExtraction`: death calls `APlayerController::StartSpectatingOnly()` (built-in engine spectator mode, watching the rest of the party); extraction does nothing further (already pawn-less). Either way it calls the new `AZSGameState::Server_CheckRaidEndAndReset()`, which (via `IsRaidOver()` — a `PlayerArray` scan mirroring `UpdateSleepRequestState`'s own pattern) reseeds (`Server_StartRaidReseed`) and releases every spectating player back to a pawn-less "ready" `NAME_Playing` state once nobody has a living pawn left. **Deliberately NOT a real `ServerTravel` level reload yet** — untestable without a multi-client PIE session (dev-hands-only), disruptive, and no `B4X` content exists yet to actually benefit from one. `AZSPlayerCharacter::Server_RespawnAsNewCharacter` renamed to `Server_EnterSpectatorAfterDeath` to match (no longer spawns a replacement character at all). `AZSGameMode::Logout` also calls the same check, so a player disconnecting outright (not dying/extracting first) can still end the raid if they were the last one alive. New test: `ZS.Raid.IsRaidOverEdgeCases` (empty-`PlayerArray` edge case only — full multi-player coverage needs a Controller/PlayerState possession-chain harness enhancement this session didn't build, same limitation `ZS.Survival.SleepReadyCounts` already documents for itself). |
| T1.3 | **New-mercenary-at-hub creation flow**: what does a fresh character actually start with? Ties directly into `BH`'s vendor system (buy a starting kit?) and `B6-Content`'s character-creation/background work — don't design this in isolation from either. |
| T1.4 | Raid-entry flow: choosing an entry point at the hub, leaving the hub, spawning in the zone with whatever loadout `T1.3` produced. |

### BR-T2 — Extraction points · **S**

| Sub-task | Definition of done |
|---|---|
| T2.1 | ✅ **Done, 2026-08-27.** `AZSExtractionPointActor`, reusing `UZSInteractableComponent` exactly like `AZSContainerActor` — no new interaction path. |
| T2.2 | Multiple extraction points placed and independently tested (needs at least graybox zone content — the existing small test area is sufficient before `B4X` content exists). |
| T2.3 | **`OQ-BR-02` (SEQUENCEABLE)** — should extraction have a delay/channel time? An instant, uninterruptible extract undermines the "last moments of a raid are the tensest" pattern this genre relies on. Recommend a short channel (interruptible by taking damage), but this needs a real decision, not an assumption. |
| T2.4 | Extraction-point availability/rotation — are all points always live, or does access to some require content that doesn't exist yet (a vehicle, a key)? If the latter, that's a `BV`/future hook, not this phase's scope — don't build toward it speculatively. |

### BR-T3 — Raid reseed · **M** · **`OQ-BR-03` resolved 2026-08-28: level reload each raid entry**

| Sub-task | Definition of done |
|---|---|
| T3.1 | ✅ **Done, 2026-08-27.** `AZSGameState::Server_StartRaidReseed` — restores the loot-rarity pool to its authored defaults and rolls a fresh per-raid utilities hazard (Decision 11). Not yet wired to anything that calls it. |
| T3.2 | Wire `Server_StartRaidReseed` to the level-load hook (`OQ-BR-03`'s resolution) — call it on the raid level's own `BeginPlay`/game-mode-init path, once `BR-T1.4`'s raid-entry flow actually triggers a level load rather than reusing whatever level is already running. |
| T3.3 | Zombie density/placement reseed — needs real spawn-volume content (`B4X`'s job); this task only needs the hook to exist, not the content behind it. |
| T3.4 | Loot-container content reseed — **now free**, per `OQ-BR-03`'s resolution: `AZSContainerActor`'s existing `BeginPlay`-time `ContainerSlots` roll already reseeds correctly on every level reload with zero extra code, since a level reload naturally re-runs every actor's `BeginPlay`. No re-roll entry point needs building — that was only required under the persistently-loaded alternative, which wasn't chosen. |

### BR-T4 — Permadeath & hub-return · **S** · *mostly done*

| Sub-task | Definition of done |
|---|---|
| T4.1 | ✅ **Done, pre-existing + reused.** Death already drops every carried instance at the death location (`Server_HandleDeathLootAndZombie`) before routing through the same `Server_LeaveRaidAndReturnToHub(false)` extraction now shares. |
| T4.2 | Skill/XP reset on death — currently a genuine no-op, since no skill system exists yet (`B6-Sys`). Flagged here as a forward hook: whenever `B6-Sys` lands, wherever it stores skill XP must live on the character or a non-hub-persistent part of `AZSPlayerState`, never mixed into the hub stash/currency fields that survive death (`Currency`/`Stash`, `BH-T1.4`), or Decision 8's "full reset" breaks silently. |
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
- **`OQ-BR-03` was the load-bearing decision in this entire phase** — resolved 2026-08-28 (level reload). Everything in `BR-T3`, and a meaningful slice of `B3`'s save topology and `B4X`'s content architecture, can now proceed against that model rather than hedging between two.
- **Ability/support-strike consumable *effects*** (spawning the actual airstrike/care-package actor in the raid) belong here, once `BH-T2.4` makes the item purchasable — the item existing and the item doing something in-raid are two different tasks, split across the two phases on purpose.
