# BF — Human Hostile AI Faction

**Stage 2 — Content, Depth & Release.** **Size: M (estimate — has a real precedent, see Notes)** · **Gate: `[PUBLIC]`** · **Depends on: `B4X`** *(for real guard-point/spawn content — the character/controller/combat code itself needs only the existing small test area)* · **Blocks: `B5`'s heist-style contracts (need something to guard the loot)**

> **New phase, CR-13 (`00_MasterPlan.md` §2, extraction pivot 2026-08-27) — but not a new idea.** `GameDevPlan.md`'s original Decision 5 already scoped hostile human roamers as "the first post-v1 addition," explicitly because it's cheap AI reuse on top of the zombie pipeline. CR-13 simply promotes the timing from "after v1" to core, since heist/guarded-loot contracts need something to guard the loot. First code landed the same day as the pivot — `AZSHostileCharacter`/`AZSHostileAIController`/`UZSHostileConfig` (`Source/ZombieShooter/Hostiles/`), a deliberate sibling to `AZombieCharacter`/`AZombieAIController`/`UZSZombieConfig`, same multi-config rule, same `TakeDamage`-only-mutator convention. Perception and a ranged hitscan attack work today; no guard/patrol Behavior Tree exists yet — same "content gap, no-op gracefully" state `BT_Zombie` itself was in for a long stretch of this project's history (see `OQ-B4-12`).

## Entry criteria

- [x] The zombie AI pipeline exists as the pattern to mirror — confirmed low-risk reuse, not a new architecture.
- [ ] **`OQ-BF-01` (BLOCKING for `BF-T2`, not for `BF-T1`)** — what does "guard" behavior actually mean here? Hold a fixed point and never move? Patrol a short route? Investigate noise like a zombie does? This needs its own design pass, the same way `OQ-B4-12` was a dedicated design+implementation session for zombie behavior, not something to guess into a BT.
- [ ] **`OQ-BF-02` (SEQUENCEABLE)** — is one hostile archetype enough for beta, or does the roster need variety (a lightly-armed patrol vs. a heavily-armed heist guard)? Mirrors `OQ-B7-03`'s zombie-roster question.
- [ ] **`OQ-BF-03` (SEQUENCEABLE)** — do hostiles drop loot on death? Given the "guarded loot" framing this is almost certainly yes (their weapon/ammo, at minimum) — recommend confirming rather than assuming, since it changes `BF-T3`'s scope.
- [ ] **`OQ-BF-04` (SEQUENCEABLE)** — do hostiles and zombies fight each other, or does each faction only ever target the player? A genuine three-way fight is a much more interesting heist scenario than two separate one-sided threats, but both AI controllers currently detect "Friendlies/Neutrals/Hostiles" equally (the same v1 simplification `AZombieAIController`'s own header comment already documents) — real faction-affiliation work if the answer is yes.

## Exit criteria

- [ ] A hostile can perceive a player, fire on them, and be killed, fully working on the existing small test area (no `B4X` content required for this bar).
- [ ] At least one real guard/patrol behavior exists beyond "stand still and shoot whatever it perceives" — `OQ-BF-01`'s answer, implemented.
- [ ] Hostiles resolve `OQ-BF-03` in actual behavior (drop loot or explicitly don't, on purpose, not by omission).
- [ ] At least one hostile archetype is content-authored (a real `DA_ZS_HostileConfig_*` + placeholder mesh) and placed in a level.
- [ ] Automation coverage exists for the pure-logic parts (damage/death, ranged-attack range/cooldown gating), mirroring the zombie test suite's own coverage shape.
- [ ] `SessionHandoff.md` shows zero "built but unverified" BF items.

---

## Task breakdown

### BF-T1 — Core character/controller · **M** · *mostly done*

| Sub-task | Definition of done |
|---|---|
| T1.1 | ✅ **Done, 2026-08-27.** `AZSHostileCharacter` (flat health, `TakeDamage`, corpse-cleanup timer, no downed-state — deliberately narrower v1 scope than `AZombieCharacter`, see the class's own header comment) + `AZSHostileAIController` (sight/hearing perception, `TargetActor` blackboard key) + `UZSHostileConfig` (health/ranged-combat/senses/AI fields, multi-config rule). `Server_RangedAttack` mirrors `AZSPlayerCharacter::FireWeapon`'s hitscan shape (`VRandCone` spread, real trace against the target's mesh so zone inference works for free, `UZSNoiseSystem::ReportNoise`). |
| T1.2 | **Minimal native "combat without a BT" fallback**, so a hostile is functional today even before `BF-T2`'s guard/patrol tree exists — e.g. a Tick-driven check on `AZSHostileAIController` (mirrors `AZombieAIController::Tick`'s own `bIsInMeleeRange` computation) that calls `TriggerRangedAttack()` whenever a target is currently perceived and in range. Stationary defense only, no movement — an honest placeholder for "stands its ground and shoots," not a guess at real guard behavior. |
| T1.3 | Automation-test coverage for `TakeDamage`/`Die`/`Server_RangedAttack`'s range-and-cooldown gating, mirroring `AZombieCharacter`'s own existing test coverage shape (`ZSAutomationTests.cpp`). |

### BF-T2 — Guard/patrol behavior · **M** · *depends on `OQ-BF-01`*

| Sub-task | Definition of done |
|---|---|
| T2.1 | `BT_Hostile` authored (dev-hands-only, no editor/MCP access confirmed this session — same content gap `BT_Zombie` itself sat in for a long stretch). Native BT task nodes mirroring `Zombies/AI/`'s pattern (`BTTask_*` C++ classes sharing key names via a `ZSHostileBlackboardKeys.h`-style header, already stubbed) so the actual content-authoring side is thin once this starts. |
| T2.2 | Guard-point / patrol-route behavior around heist-relevant loot — `OQ-BF-01`'s answer, implemented. This is the actual differentiator from a zombie, not just a reskinned wander. |
| T2.3 | Investigate-noise behavior, reusing `UZSNoiseSystem` the same way zombies already do — a hostile should react to gunfire/noise, not only to direct sight. |

### BF-T3 — Content & death consequences · **S**

| Sub-task | Definition of done |
|---|---|
| T3.1 | At least one real `UZSHostileConfig` instance authored with a placeholder mesh (dev-hands-only content). |
| T3.2 | **Death loot, per `OQ-BF-03`.** Recommended mechanism: reuse `UZSLootTableConfig::RollLoot` directly on death, spawning world items the same way `AZSContainerActor::BeginPlay` already does — a hostile that only ever *loses* items on death has no need for a full carry-inventory component, so this stays as cheap as the rest of `BF-T1`'s reuse. |
| T3.3 | Roster: at least one archetype beyond the base for beta, if `OQ-BF-02` calls for it. |

### BF-T4 — Human-vs-zombie interaction · **S** · *depends on `OQ-BF-04`*

| Sub-task | Definition of done |
|---|---|
| T4.1 | If `OQ-BF-04` resolves "yes, they fight each other": real faction-affiliation work on both `AZombieAIController` and `AZSHostileAIController` (today both simply detect everyone). If "no": explicitly document that both factions are player-only threats and move on — a real decision either way, not silence. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T1 | **Solo firefight.** Get shot by a hostile, kill it. | Damage/zone inference works correctly; death and (once `T3.2` lands) loot drop both fire correctly. |
| **PT2** | End of T2 | **Guard behavior**, once `BT_Hostile` exists. | The hostile behaves like a guard per `OQ-BF-01`'s answer, not like a wandering zombie in a different skin. |
| **PT3** | BF exit | **Heist scenario** — a guarded loot cache with 2+ hostiles, approached under fire. | Reads as a real tactical threat distinct from a zombie horde, not a reskinned one. |

## Notes

- **This phase has a real precedent, unlike `BH`/`BR`.** `GameDevPlan.md` Decision 5 always intended this as cheap AI reuse — the **M** estimate reflects that head start, not a guess from nothing.
- **No `BT_Hostile` content exists yet.** Perception and the T1.2 stationary-defense fallback work today independent of any tree — don't block `BF-T1`'s exit on content that's explicitly `BF-T2`'s job.
- **A visible held weapon (a real `AZSWeapon`-shaped mesh, not just an invisible hitscan) is a content/polish follow-up**, not blocking — see `UZSHostileConfig.h`'s own comment on why `Server_RangedAttack` doesn't spawn a real weapon actor today.
