# Revision Register — P0 through P6

> **Purpose.** The source prompt requires that existing phases be **REVISED, not merely appended to**, where confirmed decisions change them. P0–P6 are all built in code, so their "revision" is not a plan edit — it is real implementation work. This file is the authoritative list of those deltas. **Every item here is scheduled into B0**; `B0_Stabilization.md` is where they become tasks with acceptance criteria.
>
> `Docs/Phases/P0–P6*.md` are **not edited** by this plan — they remain build-state records of what was done at the time. This file is the diff between them and the confirmed beta design.
>
> **Legend.** `[C]` = CONFIRMED in `ZombieShooter_Consolidated_Changes.md`, non-negotiable · `[D]` = DEFERRED there, treated as an open question · `[G]` = from `GameDevPlan.md` backlog, previously unscheduled · `[N]` = new, identified by this audit · 🚩 = scope risk against the 1/3-depth pillar.

---

## P0 — Close out, clean up, re-aim

**Status:** Complete. **Revisions:** none.

The de-scope pass did its job. One note carried forward: P0's exit criterion ("2-client PIE still passes fire/reload/aim/sprint/crouch") is the **last point at which a full 2-client verification is known to have passed.** Everything since has been verified single-client at best. B0-T1 re-establishes this baseline before anything else.

---

## P1 — Camera & control prototype

Built: TopDown + ThirdPerson perspectives, hybrid cursor facing, `UZSInteractableComponent`, Enhanced Input via `IMC_ZS_Default` + `IMC_ZS_MouseLook`.

| # | Revision | Src | Impact | B0 task |
|---|---|---|---|---|
| P1-R1 | **Delete the perspective system entirely.** Remove `ToggleCameraPerspective`, `IA_ToggleView` (`V`), the `EZSCameraPerspective` enum, `ApplyCameraPerspective`'s branching, and the `ThirdPerson`/OverShoulder camera setup. Single fixed top-down camera. | `[C]` §1 | **Removes P1's own risk hedge** — see Contradiction CR-04. Gated behind the B0-PT2 camera sign-off; do not delete until that passes. | B0-T3 |
| P1-R2 | **Fixed preset zoom range.** Min/max camera distance as tunables, not a free-scroll continuum. | `[C]` §1 | Conflicts with `IA_HotbarCycle` currently bound to the mouse wheel — **two systems now want the scroll input.** Needs an input-arbitration decision. | B0-T3, OQ-B0-01 |
| P1-R3 | **Auto-zoom on context triggers** — entering building, exiting building, driving (reserved). Needs a context-detection mechanism (volume overlap or the elevation system's floor detection). | `[C]` §1 | New subsystem: `UZSCameraDirector` or equivalent, tracking a context stack. | B0-T3 |
| P1-R4 | **Manual zoom overrides and fully disengages auto-zoom, no cooldown.** Lock persists until the next *distinct* auto-zoom-triggering context change, then auto-zoom resumes. | `[C]` §1 | Requires storing "the context that was active when the player took manual control" and comparing against new contexts — not a simple bool. | B0-T3 |
| P1-R5 | **Aim-cone accuracy model.** Hip-fire resolves within a spread cone with lower headshot weighting; aim-held narrows the cone and raises headshot weighting. No camera change on aim. | `[C]` §1 | Replaces today's perfect-accuracy hitscan in `Server_Fire`. Headshot weighting must integrate with the 4-zone health model, i.e. the cone resolves to a **zone**, not just a point. | B0-T3, OQ-B0-02 |
| P1-R6 | **Elevation handling** — aim ray resolves against the character's current floor/Z-plane, auto-detected. Player has no manual elevation control. | `[C]` §1 🚩 | Genuinely new subsystem. B0 builds the **interface and a single-floor stub**; B4 builds real multi-level support once geometry exists. Splitting it this way stops B0 from silently absorbing a B4-sized problem. | B0-T3 (stub), B4-T3 (full) |
| P1-R7 | **Interaction world-prompt widget still does not exist.** `OnNearestInteractableChanged` is the C++ hook; nothing consumes it. | `[N]` | P1's stated deliverable was "`UZSInteractableComponent` + world prompt ('E — Open')". Half shipped. | B1-T4 |
| P1-R8 | **Left-click's dual meaning is unresolved** and now urgent — B1 introduces the first modal UI. Standard fix is an `IMC_ZS_UI` context swap at higher priority, not a `bIsMenuOpen` branch inside `HandleAttack`. | `[G]` §7 x-cut Q6 | Blocks every modal screen in B1. | B1-T1 |
| P1-R9 | **Gamepad support was a P1 deliverable** ("validated with both mouse+keyboard and a gamepad from day one") and is not evidenced anywhere in the code or handoff notes. | `[N]` | ✅ **Resolved 2026-07-23 — deferred to B9, deliberately unverified.** Dev decision (OQ-B9-01): PC-only launch, core features first. Assume it does not work. B1 keeps the cheap architectural hooks so B9 is a verification pass, not a retrofit. | B9-T3.3 |

---

## P2 — Survival simulation core

Built: `UZSNeedsComponent` (Hunger/Thirst/Fatigue/Stamina), `UZSNeedsConfig`, world clock on `AZSGameState`, sleep/time-skip readiness aggregation, `UZSItemConfig` eat/drink via `Server_ConsumeItem`.

| # | Revision | Src | Impact | B0 task |
|---|---|---|---|---|
| P2-R1 | **Add `Wet` need** — binary flag from rain/water exposure, ties into footstep noise (wet footsteps louder/distinct). | `[C]` §2 | Needs a rain source to read from — B0 ships the flag + noise hookup with a debug setter; B4 wires real weather. | B0-T4 |
| P2-R2 | **Add `Temperature` need** — hot/cold, hypothermia risk, compounds with Wet. | `[C]` §2 🚩 | **Highest scope-risk item in the register.** Requires a deliberately minimal model or it grows a weather × clothing × interior × season matrix. Proposal in OQ-B0-04. | B0-T4, OQ-B0-04 |
| P2-R3 | **Fatigue at high severity reduces zombie-detection perception.** | `[C]` §2 | ⚠ Ambiguous — see CR-10. Proceeding on "the player perceives less," implemented as presentation degradation. **Confirm before building.** | B0-T4, OQ-B0-05 |
| P2-R4 | **Hunger/Thirst are penalty-only** — no "well-fed" bonus state. | `[C]` §2 | Verify no bonus path exists in `GetPerformanceMultiplier()`; if the curve returns >1.0 when sated, clamp to 1.0. Cheap. | B0-T4 |
| P2-R5 | **Encumbrance must not hard-lock sprinting** — it heavily penalizes stamina drain rate instead. | `[C]` §2 | Today `GetEncumbranceMultiplier()` folds into movement speed only. Needs to also scale `UZSNeedsComponent`'s sprint stamina drain, and `StartSprint`'s gate must not consult encumbrance. | B0-T4 |
| P2-R6 | **Moodle UI does not exist.** `OnHungerChanged`/etc. are hooks with no consumer. P2's exit criterion ("hunger/thirst *visibly* degrades performance") is therefore unmet. | `[N]` | Design the moodle container for **N needs**, not 8 — the list has already grown once. | B1-T3 |
| P2-R7 | **`IsSafeToSleep()` is a stub returning `true`**, pending zombies. Zombies now exist. | `[N]` | Real implementation: no hostile within X of the sleeper, or a safety check the player can read *before* committing. Also needs the vulnerability answer — OQ-B0-06. | B0-T4, OQ-B0-06 |
| P2-R8 | **Severity thresholds/stages are undefined** for every need. | `[N]` | Moodles need discrete tiers to render; the plan says 4 severity tiers. Must be authored in `UZSNeedsConfig` before B1's moodle work. | B0-T4, `TuningReference.md` |

---

## P3 — Health, damage & medical-lite

Built: `UZSHealthComponent`, 4 zones with `FZSBodyZoneWound`, damage-type marker classes, bite→infection roll, `EZSInfectionStage` progression, `Server_AmputateZone`, treatment actions, death→respawn.

| # | Revision | Src | Impact | B0 task |
|---|---|---|---|---|
| P3-R1 | **Add wound infection as a second, distinct tier** — any injury can develop it, curable by disinfecting, slows healing if neglected, **never fatal alone.** | `[C]` §3 | New `EZSWoundInfectionState` per zone, separate from `EZSInfectionStage`. Additive — see CR-06. | B0-T6 |
| P3-R2 | **Preserve ambiguity between the two tiers.** Bite infection must not be distinguishable from ordinary sickness by inspecting the UI. | `[C]` §3 | Non-obvious and easy to break: a naive moodle labelled "Infected" destroys the entire design intent. **Constrains B1's HUD design.** | B0-T6, B1-T3, OQ-B0-07 |
| P3-R3 | **Remove dirty-bandage decay.** A bandage persists and stays effective until the wound is healed; no re-bandage bleeding risk. Bandage is superseded when a higher-tier heal item is applied. | `[C]` §3 | Note the nuance in CR-05: the wound's own `bDirty` flag **stays** (it drives wound-infection risk and is what `Server_Disinfect` acts on). What goes is the bandage-degrades-over-time behaviour. | B0-T5 |
| P3-R4 | **Critical head-zone bleed** — rare outcome on head wounds, fast and dangerous, layered onto the existing 4 zones without adding a fifth. | `[C]` §3 | New flag/state on the Head `FZSBodyZoneWound` + a much steeper bleed rate. Needs distinct, urgent UI/audio feedback or it is an invisible death. | B0-T5 |
| P3-R5 | **Fracture recovery is multi-day**, reinforcing injury permanence. | `[C]` §3 | Recovery ticks on `AZSGameState`'s game-hour clock (same pattern as needs decay and infection). Splinting should shorten but not trivialize it. | B0-T5 |
| P3-R6 | **Zombie bites always land on Torso.** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so the zone is never inferred — **amputation's infection-clearing path (Arms/Legs only) is unreachable from a real bite.** | `[G]` handoff | This is a **shipped, known, repeatedly-deprioritized bug that makes a headline mechanic unreachable.** Fix in B0, not later. | B0-T5 |
| P3-R7 | **Amputation blackout + ~12h time skip.** Solo: a real risk window where enemies can find the incapacitated player. Co-op: teammate can move the body; a revive shortens the blackout. | `[G]` §7 P3 | Previously unscheduled. Needs an incapacitated state, a time-acceleration path, and a co-op revive action. | B0-T7 |
| P3-R8 | **Arm amputation restricts to one-handed weapons.** | `[G]` §7 P3 | Unenforceable until `EZSWeaponHandedness` exists — depends on the item-instance refactor. | B0-T2 |
| P3-R9 | **Medical item tier delays bite→infection conversion**, extending the incubation window and the amputation decision time. | `[G]` §7 P3 | New per-tier delay field on `UZSItemConfig`'s Bandage/Disinfectant entries. | B0-T6 |
| P3-R10 | **Amputation has no animation** — bare C++ mutator, no montage, no timing gate. | `[G]` §7 P3 | Violates the project's own "actions are choreographed through montage + `bIsBusy`" convention. | B0-T7 |
| P3-R11 | **Death→loot→world continuity rules unimplemented**: loot stays at death location; the dead player becomes a zombie; co-op continues unless the whole party dies; **solo death ends the world.** | `[G]` §7 P3 | `Server_RespawnAsNewCharacter` currently always respawns into the same world regardless. The world-lifetime half depends on save topology (OQ-B3-01); the loot + become-a-zombie half can ship in B0. | B0-T9, B3 |
| P3-R12 | **Bite-infection fatal timeline is unspecified** in in-game hours. | `[N]` | Needed to tune the amputation decision window. Author into `UZSHealthConfig` + `TuningReference.md`. | B0-T6, OQ-B0-08 |

---

## P4 — Zombies

Built: `AZombieCharacter`, `AZombieAIController`, `UZSZombieConfig`, `BT_Zombie`/`BB_Zombie`, AIPerception sight+hearing, `UZSNoiseSystem::ReportNoise` wired to fire and sprint.

| # | Revision | Src | Impact | B0 task |
|---|---|---|---|---|
| P4-R1 | **Add search-last-known-location** to the BT. Full loop: wander → investigate noise → chase → **search last location** → resume wander. | `[C]` §6 | ⚠ **Revised 2026-07-23.** `LastKnownLocation` Blackboard key exists and is written by `HandleTargetPerceptionUpdated`, as originally noted — but investigating `BT_Zombie`'s compile errors found a **`BTTask_ClearLastKnownLocation` asset already authored and sitting disconnected**, not just an unconsumed Blackboard key. This was likely built once and abandoned, not never started. **Deferred, not built in B0** — dev decision, in favor of a proper PZ-style redesign rather than wiring in what may be discarded anyway. | OQ-B4-12 |
| P4-R2 | **Confirm senses stay fixed per TYPE, never randomized per-individual.** | `[C]` §6 | Already true — `UZSZombieConfig` radii are read at `OnPossess`. **Verify no randomization creeps in later**; record as a standing rule — including through whatever OQ-B4-12 eventually builds. | B0-T8 |
| P4-R3 | **`BT_Zombie`'s wander branch has zero children** — zombies have no idle behaviour at all. | `[N]` handoff | ⚠ **Revised 2026-07-23 — this description was slightly wrong.** The branch isn't empty; it has two nodes referencing classes that no longer exist under those names (`BTTask_WanderToPoint`, `BTTask_InvestigateWander`). Matching assets exist under different names (`BTTask_Wander`, `BTTask_GetInvestigationPoint`, plus `BTTask_StartIdleDwell`/`BTTask_StartInvestigationTimer`) — reads as an unredirected rename, not a from-scratch gap. **Split in B0-T8**: repointing the stale references is hygiene (done now, T8.1); designing real PZ-style wander/investigate behavior is deferred (OQ-B4-12, gates B4-T7). | B0-T8 (hygiene) / OQ-B4-12 (design) |
| P4-R4 | **Noise-system stress-test criteria** added to two-client PIE verification — specific scenarios for melee/gunfire noise radius accuracy, not general pass/fail. | `[C]` §11 | Becomes a named checkpoint, not a vibe check. Scenarios specified in `B0_Stabilization.md` §PT4. | B0-T8, B0-PT4 |
| P4-R5 | **Horde coordination deferred to whatever performs best** — no Rally Leader committed. | `[C]` §6 | Explicitly subordinates design to profiling. Design work cannot start until B0's baseline exists. → B7. | B7, OQ-B7-01 |
| P4-R6 | **No special variants** (no Screamer etc.). Base roster = standard + later Crawlers. Data-driven architecture keeps the door open. | `[C]` §6 | Constrains B4/B7 — resist adding archetypes. Crawlers still need a v1/post-beta decision. | OQ-B7-03 |
| P4-R7 | **Zone-based population and respawn-into-cleared-zones is unbuilt** — no zone system exists anywhere in the project. | `[N]` | P4's stated deliverable. Also blocks P6's per-zone loot quality tiers. Needs the real map → B4. | B4-T7 |
| P4-R8 | **Door-thumping is unbuilt** (P4 deliverable: "wander, investigate noise, chase, attack, door-thumping"). | `[N]` | Needs interactable doors, which need real interiors. → B4. | B4-T5 |
| P4-R9 | **`BP_ZombieAIController`** — unused Blueprint. | `[N]` handoff | ✅ **Resolved 2026-07-23 — keep it**, dev decision, in case it is wanted later. Still a live Blueprint-corruption surface per `CLAUDE.md`'s Live Coding lesson, so **it must be included in every "Compile All Blueprints" pass** rather than treated as inert. Revisit in B2-T2.4's asset triage. | closed |

---

## P5 — Loadout & unified combat

Built: `IA_Attack` dispatch on `EZSAttackType`, 9-slot `HotbarSlots`, `Server_SelectHotbarSlot` + `EquipTimeSeconds` choreography, per-weapon melee stats, durability-lite, `ApplyHitKnockback`, static-mesh weapon assembly.

> **Verification status is the dominant fact here.** Per `SessionHandoff.md`, only the AnimBP fix and basic hotbar switching are PIE-confirmed. Stages B–G of `Docs/Testing/P5_P6_CharacterSetupVerification.md` are unrun.

| # | Revision | Src | Impact | B0 task |
|---|---|---|---|---|
| P5-R1 | **Condition-based jamming.** Jam chance scales off weapon condition/durability. Revolvers and bolt-actions are jam-immune by weapon type. | `[C]` §4 | New `bJamImmune` + jam-chance curve on `UZSWeaponConfig`; a jammed state on `AZSWeapon`; a clear-jam action (montage + `bIsBusy`, same pattern as reload). Needs UI + audio to be legible. | B0-T10 |
| P5-R2 | **No separate melee strain penalty** — stamina drain alone governs swing-spam. | `[C]` §4 | Confirms the simpler path. But **melee currently costs no stamina at all** (listed as "still not built" in P5) — so this is "implement stamina cost, and *only* stamina cost." | B0-T10 |
| P5-R3 | **Downed/staggered zombies are never auto-included in a standing swing's arc.** Finishing a downed zombie requires a deliberate stomp or targeted hit. | `[C]` §4 | Requires a **downed zombie state**, which does not exist — `ApplyHitKnockback` is a physical `LaunchCharacter` impulse with no AI stagger. Needs a BT/Blackboard downed state + `PerformMeleeSwing` filtering by it + a stomp input. | B0-T10, OQ-B0-03 |
| P5-R4 | **`HotbarSlots` holds bare `UZSWeaponConfig*`**, not references to carried items. You cannot hotbar a weapon you actually looted. | `[N]` P6 file | The headline gap. Fixed by the item-instance refactor. | B0-T2 |
| P5-R5 | **Durability does not persist across equip/unequip** — it lives on the `AZSWeapon` actor, destroyed on unequip. Re-selecting a slot spawns a fresh full-durability weapon. | `[N]` Planning | Makes durability-lite meaningless in practice, and jamming (P5-R1) inherits the same bug since it reads the same value. | B0-T2 |
| P5-R6 | **Ammo is not an inventory item** — `AZSWeapon::CurrentReserveAmmo`/`MaxReserveAmmo` are actor state. | `[N]` Planning §4 | Breaks the ammo-scarcity pillar: reserve ammo neither weighs anything nor can be shared, looted, or dropped. | B0-T2, OQ-B0-09 |
| P5-R7 | **`EZSWeaponHandedness` + `bUsableInSecondaryHand` do not exist.** | `[N]` Planning §7 | Prerequisite for P3-R8 (arm amputation) and for `SecondaryHand`. Pure data classification, cheap. | B0-T2 |
| P5-R8 | **`SecondaryHand` is unbuilt**; its trigger mechanism has been deferred three times. `Docs/Planning/…§6` proposes `IA_SecondaryAction` + an `bIsToggleable` item concept. | `[N]` | Needed for the offhand flashlight, which B4's darkness mechanic `[C]` makes load-bearing rather than optional. | B0-T11, OQ-B0-10 |
| P5-R9 | **No melee `UZSWeaponConfig` has ever been authored** — blocked on deciding how a melee weapon attaches/displays (`TP_Mesh` is a full-body skeletal swap built for a rifle pose). | `[N]` | Blocks Stage F verification and the entire melee half of combat. **Content-blocking.** | B0-T10, OQ-B0-11 |
| P5-R10 | **Melee weapon roster undefined.** Resolved as "4–6 archetypes, one per feel-category"; exact items TBD from `Content/LowPolyWeapons/` + `Content/Mega_Survival_Tools/`. | `[N]` | Needed before T4 content authoring can start. | OQ-B0-12 |
| P5-R11 | **Temporary hit-confirmation logging still in place**, pending real impact feedback. | `[N]` handoff | Remove once B0-T10's impact VFX/SFX stub exists. | B0-T10 |

---

## P6 — Inventory, loot & scavenging

Built: `UZSInventoryComponent` (flat `CarrySlots`, `Back`/`Hip` equip slots), `UZSLootTableConfig`, `AZSContainerActor`, `AZSWorldItemActor`, rarity pool on `AZSGameState`.

> **Never compiled-and-run, never PIE-tested, zero authored content.** Built unsupervised overnight.

| # | Revision | Src | Impact | B0 task |
|---|---|---|---|---|
| P6-R1 | **Four container/carry categories** — on-person (no bag needed), bags/backpacks, vehicle (reserved), world containers. | `[C]` §5 | Today capacity is a single scalar and a bag only raises it. The new model needs item *location* to matter. Land it inside the refactor where it is cheapest. | B0-T2 |
| P6-R2 | **Loot condition variance** — items roll a randomized condition within their rarity tier on spawn; two "rare" finds differ meaningfully. | `[C]` §5 | Requires per-instance state, i.e. **directly depends on `FZSItemInstance`.** Cannot be built before the refactor. | B0-T2 |
| P6-R3 | **Item-instance/GUID model** — `FZSItemInstance` + `FZSItemInstanceState`, GUID plumbing through carry slots, equip slots, hotbar, world items, containers. | `[N]` Planning §5 | **The single highest-value change in B0.** Unblocks P5-R4/R5/R6, P6-R1/R2, and save serialization (B3). Cost rises with every authored asset. Needs your go/no-go — OQ-B0-13. | B0-T2 |
| P6-R4 | **No inventory UI** — containers do "loot all" on interact as a UI-less bootstrap. | `[N]` | P6's exit criterion is unreachable without it. | B1-T5, B1-T6 |
| P6-R5 | **Zero authored content** — no `DA_ZS_ItemConfig_*`, no `DA_ZS_LootTableConfig_*`, no placed containers or pickups. | `[N]` handoff | Blocks Stage G verification entirely. Becomes track T4. | T4 |
| P6-R6 | **Two autonomous design calls still unreviewed**: bag-slot depth (`Back` + `Hip`) and the rarity-pool model (global per-session). | `[N]` §7 P6 | Flagged by the assistant that made them. Cheap to change now; expensive after T4 authoring. | OQ-B0-14 |
| P6-R7 | **Per-zone loot quality tiers unbuilt** — no zone system exists to key off. | `[N]` | Same missing zone system as P4-R7. Resolve once, in B4. | B4-T7 |
| P6-R8 | **Rarity pool does not persist** across server restarts (no save system). | `[N]` | Becomes a B3 save-payload item. | B3-T5 |
| P6-R9 | **Weight budget numbers undefined** — base carry weight, bag bonuses, per-item weights. | `[N]` OQ §7 | Blocks T4 authoring and encumbrance tuning (P2-R5). | OQ-B0-15, `TuningReference.md` |
| P6-R10 | **Rarity tier counts and finite pool numbers undefined.** | `[N]` OQ §7 | Same. | OQ-B0-15 |

---

## Cross-cutting items with no P-phase home

| # | Item | Src | Home |
|---|---|---|---|
| X-1 | **Corpse/item cleanup** — dual-limit (time decay + area count cap, whichever first), oldest-first despawn, object pooling over destroy/respawn, timers persisted by time-of-death. | `[C]` §11 | **B3-T6** (needs save + streaming) |
| X-2 | **Darkness/lighting mechanic** — dark interiors and underground require a light source. First-class, ties to interior-visibility work. | `[C]` §7 | **B4-T4**; makes `SecondaryHand` (P5-R8) load-bearing |
| X-3 | **Basement/underground randomized layout selection** — most simple single rooms, rare elaborate bunker/tunnel complexes. | `[C]` §7 🚩 | **B4-T6** |
| X-4 | **Profiling methodology** — `stat unit`/`stat fps` triage first, then `stat ai`/`anim`/`physics`, then `stat gpu`/`drawcount`. Packaged Development/Test builds only, never Debug or raw PIE. | `[C]` §12 | **T5**, baseline at **B0-T12** |
| X-5 | **Fixed reusable stress-test map/scenario** for before/after comparison project-wide. | `[C]` §12 | **B0-T12** (build it early so every later comparison shares a baseline) |
| X-6 | **Event repeatability** — most events repeatable per session; investigation story beats may be one-time. | `[C]` §8 | **B5-T2** |
| X-7 | **Per-event warning treatment** — some events warn via radio in advance, others don't, varying by type. | `[C]` §8 | **B5-T3** |
| X-8 | **No skill decay** (CONFIRMED cut) + **per-skill XP rate as an exposed tunable.** | `[C]` §9 | **B6-T2**, surfaced in **B9** |
| X-9 | **Backgrounds grant higher starting proficiency**, not unique items, as the primary differentiator. | `[C]` §10 | **B6-T4** |
| X-10 | **Ambient event locatability** — unlocatable flavor vs. investigable tangible events. | `[D]` §8 | OQ-B5-02 |
| X-11 | **Named locations** for the region. | `[D]` §7 | OQ-B4-02 (**BLOCKING** for B4) |
| X-12 | **Skill practice loops** per skill. | `[D]` §9 | OQ-B6-02 |
| X-13 | **Background roster, tradeoffs, radio tutorial pacing.** | `[D]` §10 | OQ-B6-04/05/06 |
| X-14 | **Performance budget numbers + minimum hardware spec.** | `[D]` §12 | OQ-B8-01/02 |
