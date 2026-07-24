# B3 — Persistence, Save Architecture & Streaming Backbone

**Size: L (16–20 dev-sessions)** · **Gate: `[INTERNAL]`** · **Depends on: B0** · **Blocks: B4 (hard)**

> **Systems only, no content.** This phase builds and de-risks save/load and World Partition streaming **against the graybox map**, before a single production asset is placed. Doing it the other way round means revisiting every streaming cell once saves arrive.
>
> **Highest data-integrity risk in the project.** A save bug in a permadeath game with a persistent shared world is not a bug, it is a lost campaign. Rotating backups are CONFIRMED precisely because of this.

## Entry criteria

- [ ] B0 complete — `FZSItemInstance` gives every item a stable `FGuid`, which is the serialization key. Serializing the old model would mean writing the save layer twice.
- [ ] B0-T4 complete — the needs list is settled at 8. Serializing 6 then adding 2 means a save-version migration during beta.
- [ ] **OQ-B3-01 answered (BLOCKING)** — save topology + world-lifetime rules. This is the largest unresolved architectural question remaining in the project.

## Exit criteria

- [ ] Quit → relaunch → world remembered, verified across a host and a joining client.
- [ ] A hard kill (`taskkill`) mid-session loses at most ~10 seconds of character state and leaves a loadable world.
- [ ] Corrupting the newest save file still yields a playable world from a rotating backup.
- [ ] A 2-hour soak with continuous streaming shows no save-thread hitching and no unbounded memory growth.
- [ ] Save format carries a version stamp and an explicit unknown-version failure path.

---

## Task breakdown

### B3-T1 — Save topology & architecture spike · **M (3–4 sessions)** · *blocked by OQ-B3-01*

| Sub-task | Definition of done |
|---|---|
| T1.1 | OQ-B3-01 resolved and written up: one world per server vs. multiple slots; what a "world" is identified by; how party-wipe and solo-death world termination work (`GameDevPlan` §7 P3 backlog, CR-07/CR-12). |
| T1.2 | `UZSSaveGameSubsystem` (`UGameInstanceSubsystem`) skeleton — owns all save orchestration. Single entry point; no system writes save files directly. |
| T1.3 | **Save payload inventory** — an explicit, exhaustive written list of every piece of state that must persist, produced by walking each component. Anything not on the list is intentionally transient. See §Payload below. |
| T1.4 | Serialization format decided (`USaveGame` + `FArchive`, or a custom binary/record format) → OQ-B3-02. |
| T1.5 | **Version stamp from the first write.** Retrofitting versioning after testers have saves is painful and entirely avoidable. |
| T1.6 | Spike proves round-trip of one non-trivial slice (a player's full inventory with instance state) before the rest is built on it. |

### B3-T2 — Layered save implementation · **M (4–5 sessions)** · *depends on T1*

All four layers are CONFIRMED (Consolidated §7).

| Sub-task | Definition of done |
|---|---|
| T2.1 | **Character-state save, ~10s interval.** Async, off the game thread. Must not hitch — profile it, don't assume. |
| T2.2 | **Periodic full-world save** on its own longer timer, interval tunable. |
| T2.3 | **Chunk/area save on unload**, hooked to World Partition cell unload — the layer that makes a large streamed world tractable. |
| T2.4 | **Clean full save on graceful shutdown**, covering both quit-to-menu and quit-to-desktop. |
| T2.5 | **Rotating backup slots (2–3), never a single overwritten file** (CONFIRMED). Rotation is atomic: write to temp, fsync, rename. A crash mid-write must never destroy the previous good save. |
| T2.6 | Corruption detection (checksum) with automatic fallback to the next-newest backup, and a clear player-facing message when it happens. |

### B3-T3 — World Partition & streaming · **M (3–4 sessions)** · *depends on T2*

| Sub-task | Definition of done |
|---|---|
| T3.1 | World Partition enabled on the graybox map; cell size and streaming distance chosen and **measured against the B0-T12 baseline**, not guessed. |
| T3.2 | Streaming policy decided for a top-down camera — the view distance is fixed and known, which makes this more tractable than a free-camera game. Document the reasoning. |
| T3.3 | **Zombie behaviour across streaming boundaries** defined: what happens to a chasing zombie whose cell unloads, and to noise events fired near a boundary (PT4 in B0 tests this scenario for exactly this reason). |
| T3.4 | **Data layers** for the elevation/multi-level system (B4-T3) reserved now — retrofitting layers into a built region is expensive. |
| T3.5 | Server-authority model under streaming confirmed for listen-server: the host streams by their own position; clients must not desync when the host's cells differ from theirs. |

### B3-T4 — Corpse & item lifetime management · **S (2–3 sessions)** · *depends on T2*

CONFIRMED dual-limit system (Consolidated §11). Directly feeds B8's performance work.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | **Dual limit**: per-corpse decay timer **and** an area count cap, whichever triggers first. Oldest despawns first when the cap is exceeded. | X-1 |
| T4.2 | **Object pooling, not destroy/respawn** — CONFIRMED as the GC-friendly path at scale. Applies to corpses, world items, and zombie actors. | X-1 |
| T4.3 | **Timers tracked by time-of-death, not runtime elapsed**, so they survive save/reload correctly — a corpse from 3 in-game days ago must not reappear fresh on load. | X-1 |
| T4.4 | Loot on a corpse (B0-T9.1) persists with the corpse and is not lost to cleanup while a player is en route back to it. Define the interaction between "corpse despawns" and "the loot on it." |

### B3-T5 — Persisting existing systems · **M (3–4 sessions)** · *depends on T2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | **World clock** — `TimeOfDayHours`, `DayCount`, utilities-shutoff timer. | P2 |
| T5.2 | **Rarity pool** — `AZSGameState::RarityPoolEntries` must persist across restarts or "finite world-count rarity" is meaningless the moment anyone reloads. | P6-R8 |
| T5.3 | **Container contents** — including which have been looted, so a cleared house stays cleared. |
| T5.4 | **Dropped world items** with full instance state (`InstanceId`, durability, `ConditionQuality`). |
| T5.5 | **Player character state** — needs (all 8), health/wounds/infection (both tiers), inventory, hotbar assignments, equipped slots, skills (schema reserved for B6 even though unpopulated). |
| T5.6 | **Zombie population state** per zone/cell — cleared areas and repopulation timers (OQ-B4-05 decides the repopulation rule; B3 provides the storage). |
| T5.7 | **Reserved, unpopulated schema** for B5's event/investigation flags and B6's character-creation data. Adding fields later is a version migration; reserving them now is free. |

### B3-T6 — Failure-mode hardening · **M (2–3 sessions)** · *depends on all above*

| Sub-task | Definition of done |
|---|---|
| T6.1 | **Hard-kill test** — `taskkill /F` on the host at 20 random moments including mid-save. Every resulting save set must load. |
| T6.2 | **Disk-full and read-only-directory** handling: fail loudly and safely, never silently discard a save. |
| T6.3 | **Client disconnect** during a save — character state must not be lost or half-written. |
| T6.4 | **2-hour soak** with continuous movement across streaming boundaries. Watch memory, save duration drift, and file-size growth. |
| T6.5 | **Unknown save version** path: refuse to load with a clear message rather than loading garbage. |

---

## Save payload inventory (T1.3 starting list)

| Owner | State | Layer |
|---|---|---|
| `AZSGameState` | `TimeOfDayHours`, `DayCount`, utilities-shutoff timer, `RarityPoolEntries` | World |
| `AZSPlayerState` | Identity, skills (B6), stats | Character (10s) |
| `UZSNeedsComponent` | All 8 needs + severity states | Character (10s) |
| `UZSHealthComponent` | `CurrentHealth`, 4× `FZSBodyZoneWound`, `EZSInfectionStage`, `EZSWoundInfectionState`, fracture recovery timers | Character (10s) |
| `UZSInventoryComponent` | `CarrySlots` (full `FZSItemInstance`), equip slots | Character (10s) |
| `AZSPlayerCharacter` | Hotbar GUIDs, `SecondaryHand`, transform, blackout state | Character (10s) |
| `AZSContainerActor` | Contents, looted flag | Chunk |
| `AZSWorldItemActor` | Instance + transform | Chunk |
| Corpses | Instance list, time-of-death | Chunk |
| `AZombieCharacter` | Population counts per zone, cleared/repopulation timers | Chunk |
| B5 (reserved) | Event flags, clue discovery, radio arc progress | World |
| B6 (reserved) | Background, appearance, world seed, difficulty settings | World |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | Round-trip spike: a full inventory with instance state saves and loads identically. | Durability and `ConditionQuality` survive byte-for-byte. |
| **PT2** | End of T2 | Quit → relaunch, 2-client. Then hard-kill the host mid-session. | World remembered. Hard kill loses ≤10s. Backup rotation demonstrably works. |
| **PT3** | End of T3 | Streaming stress: run laps across cell boundaries with zombies chasing and noise events firing at boundaries. | No desync, no lost AI, no duplicated actors, no hitching. |
| **PT4** | End of T4 | Kill 100+ zombies in one area over an in-game day; save, reload, return. | Cleanup respects both limits. No corpse resurrects with a reset timer. No memory growth. |
| **PT5** | B3 exit | **2-hour co-op soak, then deliberate save corruption.** | Soak clean. Corrupted newest save falls back to a backup with a clear message. |

## Notes

- **Do not start B4 before PT5.** Every hour of region content built on an unproven save layer is an hour that may need redoing.
- **Late-join is B10's**, not B3's — but B3's topology decision (T1.1) determines whether late-join is even possible, so make the call with B10 in mind.
- **Cloud saves, Steam Cloud, and save sharing are POST-BETA.** Local files only.
