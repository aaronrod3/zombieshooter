# Survival Pivot — Full Game Development Plan (v0.2, draft for dev review)

> **Status: DRAFT — v0.2, revised 2026-07-18 after the dev's own markup pass over the PZ reference doc.** Not yet the plan of record.
> Per the dev's own instruction on that markup pass ("fully replace CoreLoopPlan once GameDevPlan is finished"): once the open questions in **§7** are resolved and this plan is confirmed, it fully replaces `CoreLoopPlan.md` as the plan of record — not just Phases 4–6 as v0.1 hedged. `CoreLoopPlan.md` then becomes a historical build log for Phases 0–3 (referenced, not edited further); `CLAUDE.md`/`SessionHandoff.md` get repointed here.
>
> **This plan's own filename is a placeholder.** The dev's markup left "Rename GameDevPlan to: ___" blank — see §7's cross-cutting questions.
>
> Companion documents: [`ProjectZomboid_DesignReference.md`](ProjectZomboid_DesignReference.md) (the PZ systems breakdown, referenced as "PZ §N") and [`DevMarkupNotes.md`](DevMarkupNotes.md) (the dev's own section-by-section notes on that reference, referenced as "Notes §N" — this plan is the synthesis of the two).
>
> Items marked **⚑ DECISION** are calls the dev should confirm or overturn. Each has a recommendation and the plan proceeds on that recommendation, so overturning one only reroutes that item, not the whole plan. **§7 is a separate, larger battery of open questions per development stage** — read it before starting the phase it's attached to, not necessarily all at once.

---

## 1. What the game is

**One-liner:** A low-poly, co-op-first zombie survival game set in a fictional region modeled on Upstate NY's Adirondacks (Notes §10.1/§11) — Project Zomboid's simulation soul (needs, noise, attrition, permadeath) with a modern 3D camera, real aiming, an investigation-driven narrative arc toward the truth behind the outbreak (Notes §1/§22), and a dynamic mission/event layer PZ never shipped.

**Explicitly not PZ's thesis** (Notes §1: **REMOVE** the "this is how you died" / inevitable-death framing): survival is still hard and death is still permanent at the character level, but the game gives players something to *find out and work toward*, not just a duration to maximize. See the investigation arc below and Decision 6.

**Why this can exist next to PZ** (from the reference doc's own gap analysis, PZ §22):
1. PZ has no missions/objectives layer and an empty late game — we build a dynamic event/objective layer *and* a discoverable investigation questline as core systems, not a bolt-on (Notes §22: "add investigative quests to find out how the infection spread, find a cure").
2. PZ's multiplayer history is droughts and instability — this project is replication-first *already* (Phase 3 is built).
3. PZ's onboarding is a cliff and its UI is its most-modded surface — we ship fewer, more readable systems with modern interaction (prompts, radials, transparent stat previews — Notes §21) and a control scheme that isn't mouse-menu-dependent (Notes §3: "need playability on consoles later on").
4. PZ is 12 years of hand-built map density — we compete honestly: **small dense map, low-poly modular art, systems + events over content mass.**

### Design pillars (steal / rethink, per PZ §23, revised against the dev's notes)

**Kept from PZ (identity-defining):**
- Readable simulation stack — a small **moodle-style status system** (PZ §5, simplified — see §3 below), but tuned so **Hunger/Thirst are performance debuffs first, not death spirals** (Notes §1: food/water last longer than typical survival games; player doesn't auto-die from hunger — stamina, healing, aim accuracy, and attack recovery degrade instead; death from starvation stays possible, just far downstream, not the default outcome of neglect).
- **Noise as threat currency** — every loud action pulls zombies (PZ §7.3).
- **Stamina-economy melee** — swinging is a resource decision, not a reflex test (PZ §7.1).
- **Infection ambiguity window, extended** — "is this a cold or am I dead?" (PZ §8.1, §9) — but now with a genuine escape valve: **emergency amputation** of a bitten/infected limb can stop a fatal infection before it completes, at the cost of permanent capability loss (Notes §1/§9: "second chances... such as reviving, cutting off infected body parts"). This raises the horror-tension ceiling instead of lowering it — the player now has an agonizing choice to make, not just a death sentence to wait out.
- **Permadeath + persistent world** — new character, same looted world (PZ §18). Notes §1 confirms this reading explicitly ("lose all progress" = the character's skills/inventory/build, not the shared world state).
- Phase-transition world state — a utilities-shutoff-style clock that changes the game's rules partway in (PZ §10.3).
- Environmental storytelling + diegetic media (radio broadcasts) as tutorial and narrative (PZ §16) — now doing double duty as the mission-giver (see Phase P8).

**Explicitly cut/replaced from the "steal" list (v0.1 had these as kept-as-is; the dev's notes move them):**
- **Trait point-buy at character creation** — Notes §4.2: "instead of traits, certain actions just affect your stats differently," plus "need a skill points system, the more you do certain actions, the more proficient you get." Read together, this replaces PZ's creation-time positive/negative trait shopping list with **traits/aptitudes that emerge from play** (see §3's table and Decision 5). The classic point-buy list isn't gone forever — it's a natural "hardcore mode" sandbox option later, same shelf as other deferred PZ sandbox sliders.

**Deliberately different from PZ (our identity):**
- **3D top-down camera with direct WASD + mouse-aim control, framed like *Door Kickers 2*** (dev reference, 2026-07-19) — steeper/closer top-down than a classic 45° isometric, zoomable in tight enough to read character/room detail, not PZ's menu-driven interaction. **Important distinction from DK2 itself: this is a visual/camera reference only, not a gameplay one** — the player has full real-time direct control and normal movement at all times, no squad-command/pause-and-plan layer. Combat feels like a twin-stick/tactical shooter; survival feels like PZ. (Also more console-friendly than isometric-plus-right-click-menus — a stick-aim top-down camera translates to a gamepad far more directly, which helps Notes §3's console want land for free later.)
- **Co-op-first (2–4 players, listen server)** — every system built replicated from day one, which is already this codebase's DNA. Profession/background choice now also picks a **starting spawn location** (Notes §4.1) — see Decision 4.
- **Dynamic events, radiant objectives, and a discoverable investigation arc** — helicopter-class events (PZ §17) as an expanded *system* (Notes §17: "create more meta events"), radio-driven objectives, and a questline built from notes/documents/items scattered through the world that lets players piece together the outbreak's origin and chase a cure (Notes §1/§22).
- **Hostile human roamers** — always-hostile wandering human NPCs that fight zombies *and* players, never allies (Notes §19: "never friendly"). This is the "enemy variety Romero purism forbids" the reference doc's own §23 flags as a differentiation opportunity. **Confirmed as the first post-v1 addition, not part of the v1 slice** (Decision 5) — the dev chose to prove out the core survival loop first, even though it's cheap specifically because it reuses the zombie AI/perception/noise pipeline. Full NPC survivors/factions/dialogue/reputation systems are a different, much bigger system and stay deferred to their own planning pass, same as before.
- **Simplified simulation** — every PZ system ships here at roughly **1/3 of PZ's depth**, chosen for readability. Depth can grow later; opacity is not a feature we inherit.
- **Modern, transparent UX** — direct interaction prompts, radial quick-menu, and (Notes §21) **items/actions show their actual mechanical effect on hover/preview** rather than hidden numbers — "player knowledge is mainly built on common sense and playing the game," not a wiki.

**⚑ DECISION 1 — Camera.** Recommended: **top-down, *Door Kickers 2*-framed** (steeper pitch than a classic 45° isometric — closer to ~65–75° — zoom range tight enough to read character/weapon/room detail, wide enough for tactical awareness; yaw rotation in 45° steps). The existing `ApplyCameraPerspective` architecture makes this a *new perspective entry*, not a rewrite — cut the perspective list from 4 (FP/TP/GunCamera/Bodycam) down to **TopDown + OverShoulder** (over-shoulder kept as an aim-zoom and as a hedge). First-person, GunCamera, Bodycam: shelved (code kept in git history, perspectives removed from the cycle). **The dev's real-time-control requirement is explicit and non-negotiable: full direct player control and normal movement at all times — this decision borrows only DK2's camera angle/framing, never its squad-command/pause-and-plan gameplay layer.** Unchanged in spirit from v0.1, refined 2026-07-19 with the DK2 reference as a concrete visual touchstone.

**⚑ DECISION 2 — Same repo vs. fresh project.** Recommended: **same repo, same UE project.** Unchanged from v0.1.

**⚑ DECISION 3 — Art source.** **Resolved 2026-07-19, superseding v0.1's Synty recommendation entirely** — the dev doesn't like Synty's style and won't use it for anything. Direction instead: **dark, earthy tones with slight realism, kept low-poly** (low-poly as a production/performance choice that also keeps hand-modeling in Blender viable later, not a cartoon-toy look). This sits naturally under the *Door Kickers 2* reference above rather than in tension with it — DK2's own look is grounded and quietly detailed, not bright/flat, so the printed/decal-texture-detail note for weapons/props still holds, it just isn't contrasted against a flat Synty environment style anymore. **Sourcing is the dev's own** (ArtStation and elsewhere) — see §5, which now tracks *what's needed* rather than *where to buy it*.

**⚑ DECISION 4 (NEW) — Profession-based spawn points: solo too, or co-op-only?** Notes §4.1 wants players to start at a profession-tied spawn location, explicitly calling out the co-op "find each other" fun. Recommended: **apply to both, with a lobby-level "scatter spawns" toggle.** Solo players still get a profession-flavored starting location (keeps one system instead of two), but a co-op group can toggle scattered starts off if they'd rather just start playing together immediately. Confirm in §7.

**⚑ DECISION 5 — Hostile human roamers: timing.** *Resolved 2026-07-18, against this doc's own recommendation.* Notes §19 asks for them unconditionally, not conditionally — only *timing* was ever in question. This plan originally recommended building them alongside zombies in P4 (cheap AI reuse); **the dev chose to push them past the v1 vertical slice instead** — get the core survival loop (zombies, health, combat, loot) fully solid first, then add hostile roamers as the first post-v1 addition. See Phase P4 and Phase P10's post-v1 list — the AI-reuse argument for building them cheaply alongside zombies still holds and makes them a fast, low-risk follow-up once v1 ships, not a from-scratch system.

**⚑ DECISION 6 (NEW) — Does the investigation/cure arc end or reset the world?** Notes §1 removes "no win condition" as a philosophy but doesn't say the game should *end* on completion — it says add an "end goal to survival," which reads as something to chase, not a game-over screen. Recommended: **the investigation arc is an optional capstone with no forced ending** — reaching its conclusion unlocks a lore epilogue and a meaningful, persistent world-state change (example: a rescue/evac becomes available, or a new sandbox modifier unlocks), but the world keeps running and the character can keep playing, same continuity spirit as PZ's own permadeath-into-new-character loop. This is the single highest-leverage open question in the whole plan — it shapes world-state/save architecture (can a "completed" flag exist per-character without breaking co-op persistence?) more than any individual system does. Confirm in §7 before Phase P8 design work starts in earnest.

**Working title:** repo/project name stays "ZombieShooter" (per `CLAUDE.md`, a placeholder). This planning document's own filename is a separate, smaller open question — see §7.

---

## 2. What happens to the current project

The pivot **keeps the repo, the project, and the C++ core.** What we've built is mostly camera-agnostic infrastructure; what changes is the presentation layer and where future effort goes.

### Carries over as-is (the pivot's foundation)
| System | Why it survives |
|---|---|
| `AZSGameMode/GameState/PlayerState/PlayerController` + Enhanced Input | Camera-agnostic framework. Input actions get remapped, not rebuilt. |
| **Phase 3 replication layer** (server RPCs, `OnRep_` convention, cross-client fixes) | The single most valuable asset for a co-op-first game. |
| `UZSWeaponConfig` data-driven weapon architecture + "N weapons, zero C++ branches" rule | Extends beyond guns: the same pattern becomes `UZSItemConfig`, `UZSZombieConfig`, and now a shared config for hostile human roamers too. |
| Real ammo/magazine state on `AZSWeapon` | PZ-style scarcity needs real ammo. Already built, already replicated. |
| Notify architecture (`AN_ZS_UnlockActions`, `ANS_ZS_BlockADS`, montage-driven action flow, `bIsBusy` + fallback) | Generalizes to every timed action this game will ever have: melee swings, bandaging, amputating, barricading, crafting. |
| `BlueprintNativeEvent` policy, replication convention, naming rules, docs discipline, MCP workflow | Unchanged. |

### Simplified or repurposed
| Current | Disposition |
|---|---|
| 4-perspective camera system | Cut to TopDown + OverShoulder (Decision 1). |
| FP arms mesh/AnimBP | **Cut outright** — dev directive, stronger than "shelve" (see P0). |
| TP AnimBP (`ABP_ZS_ThirdPerson`) | Becomes the *only* character view, built on **`SKEL_TFA_Mannequin`** (Infima's skeleton, confirmed as the one shared retarget hub — not the generic UE5 mannequin originally guessed here). Rebuilt 2026-07-20: Lyra locomotion blend spaces + Infima idle/aim poses via Layered Blend Per Bone (see §5.1). |
| Infima pack | **Confirmed as the skeleton hub and the source for weapon idle/aim poses + fire/reload montages** — not a prototype placeholder. Not the sole animation source: Infima has no full directional walk-cycle assets, so locomotion itself comes from Lyra blend spaces retargeted onto Infima's skeleton (see §5.1). |
| `Docs/Infima Pack - Official Implementation Guide/` | Actively relevant — Infima is a real, load-bearing animation source (skeleton + poses/montages), not history. |

### Cut outright (the animation de-scope the dev asked for)
- **`Inspect`, `MagCheck`, `CycleGripAttachment`** — actions, input bindings, montage wiring.
- **`AZSLaserAttachment`**, grip-attachment randomization/variants.
- **Weapon-owned cosmetic notifies + `ABP_Weapon`/`ABP_Magazine`**.
- **`AZSPhysicsCasing`/`AZSPhysicsMagazine`/`AZSPhysicsObject`** cosmetic ejects.
- `FP_ReloadEmpty`/`TP_ReloadEmpty` variants, gun-camera/bodycam content, procedural ADS/recoil/crouch **spring-offset system**.

**Rule going forward:** an animation earns its place only if it's *readable at gameplay camera distance* or *gates gameplay timing*. Everything else is polish-phase-only.

---

## 3. PZ systems disposition (KEEP / SIMPLIFY / REPLACE / CUT)

| PZ system (ref §) | Disposition | Our version |
|---|---|---|
| Isometric camera, menu-driven interaction (§3) | **REPLACE** | 3D top-down, direct control, world interaction prompts + radial quick-menu. |
| Professions/occupations (§4.1) | **SIMPLIFY** | 5–7 starting "backgrounds" = stat template + 1 unique unlock + a tied starting spawn location (Decision 4). |
| Trait point-buy (§4.2) | **REPLACE** | No creation-time point-buy in v1. Traits/aptitudes emerge from play — see §3.1. |
| Moodles / needs (§5) | **KEEP, simplified** | **6 moodles v1:** Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness. |
| Nutrition micro-sim (§5) | **CUT** | Food restores Hunger; quality = bigger/longer restore. |
| Skills, learn-by-doing, books (§6) | **SIMPLIFY, revised 2026-07-19** | Attributes (Strength, Stamina, Sneak, Sprint) + per-weapon-class melee skill bars + Maintenance + Aiming/Reloading + First Aid. See §3.1. |
| Melee combat, stamina economy (§7.1) | **KEEP** | Shove + swing + stomp, stamina-gated, durability-lite. |
| Firearms as loud/scarce power (§7.2) | **KEEP** | Already built mechanically; noise system makes it PZ-honest. |
| Stealth/noise/vision model (§7.3) | **KEEP, simplified** | Crouch = quieter + slower; hearing radii; vision cones. Sneak is now a stat, not a dedicated skill tree. |
| Zombie lore + hordes + migration (§8) | **KEEP, simplified** | Shamblers, hearing/sight, door-banging, zone population + wander. |
| Per-body-part health, 17 zones (§9) | **SIMPLIFY** | 4 zones: Head, Torso, Arms, Legs. Emergency amputation as an infection-stopping "second chance." |
| Hand-built mega-map (§10.1) | **REPLACE** | A fictional Adirondacks-modeled county, one dense area + rural fringe, World Partition. |
| Container/zone loot tables (§10.2) | **KEEP, refined** | Data-asset loot tables, equip-only vs. carry-only categories, finite world-count rarity pools. |
| World persistence + erosion (§10.3) | **SIMPLIFY** | Persistence yes; erosion visuals cut v1. |
| Utilities shutoff phase transition (§10.3) | **KEEP** | Power/water die on a randomized day. |
| Seasons/weather/temperature (§11) | **SIMPLIFY** | Day/night + rain/fog v1, Adirondacks-tuned. |
| Weight encumbrance + bags (§12) | **KEEP, simplified** | Weight + bag slots, no bags-in-bags recursion. |
| Clothing protection layers (§12) | **SIMPLIFY** | Single outfit slot-set with protection values. |
| Carpentry/base-building (§13.1) | **SIMPLIFY** | v1: barricades, reinforcement, crates, rain collector. |
| Metalworking/electrical/plumbing chains (§13.2) | **CUT v1** | Generator-as-item without wiring sim. |
| B42-style deep crafting web (§13.3) | **CUT** | Events/missions/investigation are our late game instead. |
| Farming/foraging/fishing/trapping (§14) | **SIMPLIFY** | v1: farming-lite + foraging zones. Fishing/trapping deferred. |
| Vehicles (§15) | **CUT v1, plan later** | Deferred to its own planning pass. |
| TV/radio diegetic media (§16) | **KEEP, repurposed** | Tutorial + mission-giver. |
| Meta events (§17) | **KEEP, expanded ambition** | Event variety a stated priority. |
| Modes/challenge presets (§18) | **CUT v1** | Sandbox sliders post-v1. |
| MP: dedicated servers, big counts (§19) | **SIMPLIFY** | 2–4 player listen-server co-op. |
| Modding/Lua (§20) | **CUT v1** | Data-asset-driven design keeps the door open. |
| NPCs/factions (§19, §22.1) | **DEFER (past v1)** | Hostile roamers first post-v1 addition (Decision 5). |

### 3.1 — Skill system (revised 2026-07-19)

**Core attributes (not skills — passive stat pools):**
- **Strength** — increases melee damage and weight capacity. Grows from performing strength-relevant actions (melee kills, carrying heavy loads), not a separate "use strength" action.
- **Stamina** — governs the melee/sprint resource economy (Phase P2/P5). Increases both from leveling up generally and from repeatedly performing stamina-costing actions (sprinting, swinging).
- **Sneak** — reduces detection radius/noise while crouched. Grows through use.
- **Sprint** — affects sprint speed/endurance cost. Grows through use.

**Combat skills:**
- **Melee weapons** — each weapon *class* gets its own skill bar, leveled by landing hits with that class. Higher level: faster attack speed, increased damage, increased critical damage chance.
- **Maintenance** — reduces weapon wear rate; leveled by performing maintenance actions, which also become faster to perform at higher levels. Weapons gain an increased usage meter (more actions before breaking) as this skill rises.

**Firearm skills:**
- **Aiming** — increases with weapon usage (shots fired/hits landed). Affects accuracy, time-to-aim, and effective range.
- **Reloading** — increases reload speed and the speed of loading/unloading individual rounds into a magazine.

**Medical skill:**
- **First Aid** — increases with use. Higher levels unlock more effective use of medical items and faster application speed (ties into P3's bandage/splint/amputation actions).

**Deferred skills (post-v1, own planning pass):**
- Fishing, Building, Foraging, Cooking, Mechanics — confirmed direction, not part of the v1 slice. Added to the Phase P10 post-v1 backlog alongside hostile roamers.

**Design calls baked into this revision (supersedes the 2026-07-18 six-skill list):**
- **Melee splits per weapon class**, not one flat Melee skill — a real build-identity fork, matches PZ's own per-weapon-type skill model more closely than the original simplification did.
- **Maintenance is its own skill**, separated out from a generic Melee skill — weapon durability/wear management is a distinct player decision from combat proficiency.
- **Firearms splits into Aiming and Reloading** — same rationale as PZ, two different resource/skill economies (accuracy vs. logistics speed).
- **Strength and Stamina reclassified as attributes, not skills** — they're passive stat pools that grow from broad play rather than a dedicated action loop, closer to PZ's own Fitness/Strength core-stat model than a learn-by-doing skill bar.
- **Sneak and Sprint are stats, not a dedicated skill tree** — consistent with the existing §7.3 disposition ("no lightfootedness skill web in v1").
- **First Aid stands alone**, unchanged in spirit from the prior Medicine skill.
- **Carpentry and Survival move to the deferred list** as Building and Foraging/Cooking respectively — dev's stated priority is proving out the core combat/survival loop first; these return in their own planning pass alongside Fishing and Mechanics.
- **Level range 1–5** still applies where a visible level makes sense (per-weapon melee bars, Aiming, Reloading, Maintenance, First Aid) — matches the non-grind design goal (Notes §1).

**Expansion path (not built now):**
- Deferred skills (Fishing, Building, Foraging, Cooking, Mechanics) get their own planning pass once the v1 combat/survival loop is proven.
- Melee weapon-class skill bars can grow in number as weapon variety grows past P5's basic archetypes — the system already supports N classes with zero rework.
- Mechanics arrives whenever vehicles do (post-v1).

---

## 4. Development phases

### P0 — Close out, clean up, re-aim (the simplification pass)
1. **Finish Phase 3 M7** (2-client PIE verification of the existing replication layer). Verify what's built *before* surgery.
2. Commit the currently-uncommitted session-8 work (already compiled clean).
3. **De-scope pass** per §2's cut list: remove Inspect/MagCheck/SwitchGrip; strip laser/grip/physics-cosmetic paths; retire the FP spawn path; reduce perspective enum. Compile + PIE after each removal cluster.
4. Update `CLAUDE.md`, `SessionHandoff.md`; this doc becomes plan of record.
   **Exit:** clean build, 2-client PIE still passes fire/reload/aim/sprint/crouch with the slimmed action set.

### P1 — Camera & control prototype (identity test #1)
- TopDown perspective in `ApplyCameraPerspective`, movement relative to camera, OverShoulder aim-zoom toggle.
- **Hybrid facing (confirmed 2026-07-20):** WASD alone faces movement direction (`bOrientRotationToMovement = true`, already the P0 default). Cursor-projected aim only overrides facing while actively aiming/attacking/interacting — full actor rotation, not a spine-twist.
- Interaction system v1: `UZSInteractableComponent` + world prompt ("E — Open").
- Input scheme validated with both mouse+keyboard and a gamepad from day one.
- Graybox test map. Infima rifle still the stand-in weapon.
  **Exit:** moving/aiming/shooting *feels good* at top-down distance with both input methods, 2-client PIE. **This is the go/no-go gate on Decision 1.**

### P2 — Survival simulation core (identity test #2)
- `UZSNeedsComponent`: Hunger/Thirst/Fatigue/Stamina + rate curves. Consequence model: needs degrade performance before health.
- World clock (`AZSGameState`): day/night, compression, utilities-shutoff timer.
- **Sleep/time-skip** (Minecraft-style, co-op readiness check).
- Moodle UI stack (UMG, 4 severity tiers) + transparent stat-preview rule established here.
- Items exist minimally: eat/drink consumables via first `UZSItemConfig`.
  **Exit:** hunger/thirst visibly degrades performance under normal neglect; sleep-based time-skip works solo and multi-player; replicated, 2-client PIE.

### P3 — Health, damage & medical-lite
- `UZSHealthComponent`: 4 zones, wound types mapped to gameplay effects, bleed-over-time, all damage through `TakeDamage`.
- Treatment actions: bandage (cleanliness flag), disinfect, splint.
- Delayed-onset infection: bite → hidden roll → delayed queasy→fever→death arc.
- Emergency amputation: stops infection source, permanent capability loss.
- Player death → spectate/respawn-as-new-character flow.
  **Exit:** a scripted damage source can wound, infect, and kill a player who mismanages treatment; amputation-in-time survives a bite that would otherwise kill. Second client sees everything correctly.

### P4 — Zombies (the enemy, finally)
- `AZombieCharacter` + `AZombieAIController`, Behavior Tree + Blackboard, `UZSZombieConfig` data asset.
- Perception: `AIPerception` sight cone + hearing. `UZSNoiseSystem`: every loud act reports a noise event.
- Behaviors v1: wander, investigate noise, chase, attack, door-thumping.
- Zombie reintroduction: zone-based population, respawn-into-cleared-zones.
- Placeholder visuals: Mixamo/UE-mannequin zombie + Mixamo zombie animations (already imported, per §5.1 Stage C).
- Architecture built so hostile human roamers can be added cheaply post-v1 without rearchitecting.
  **Exit:** a graybox block with a profiled zombie-count budget met; a gunshot visibly drags the neighborhood onto the shooter; 2-client PIE holds up.

### P5 — Loadout & unified combat (expanded 2026-07-21 — dev request: "a proven loadout plan" + "attacking is one button, changes per weapon")
The equip-slot/hotbar machinery is combat-facing (it decides what a single Attack button *does*), so it's scoped here rather than folded into P6's inventory work — P6 still owns full inventory/weight/containers/loot tables; this phase owns the slots that reference into that inventory and the combat dispatch built on top of them.

**Loadout system** — researched against the two closest proven references for a real-time (no pause-and-plan, per Decision 1) survival game: PZ's own primary/secondary-hand equip model and DayZ's hand-slots + numbered hotbar. Recommended synthesis, **open questions resolved 2026-07-21**:
- **Two hand slots**: `PrimaryHand` + `SecondaryHand`. A two-handed item (rifle, two-handed melee) occupies both. This isn't just a genre convention — it's a **prerequisite for the amputation backlog item already recorded in §7 P3** ("arm amputation restricts weapon use to one-handed options only"): that rule is meaningless without a real one-handed/two-handed slot model to enforce it against.
- **`SecondaryHand` is independently usable, not just "the other grip"** (resolved) — a one-handed item there (offhand pistol, flashlight) can be used on its own, matching PZ/DayZ convention. **Scope note for v1**: `IA_Attack` only ever triggers `PrimaryHand`'s action; an offhand item gets its own separate action (e.g. a flashlight toggle) rather than simultaneous dual-attack dispatch — that's a real follow-on question, not solved by this decision alone.
- **Unarmed is the default, always-available state** — empty `PrimaryHand` means bare-fist melee. Satisfies "player starts unequipped." (Today's `AZSPlayerCharacter::Server_MeleeAttack`/`Unarmed*` tunables, built pre-P5 to unblock P3/P4 testing, become exactly this fallback.)
- **A small real-time hotbar** (no menu/pause) bound to specific inventory item references, for instant re-equip without opening P6's full inventory UI — matches Notes §21's "modern, transparent UX" and Decision 1's explicit rejection of a pause-and-plan layer (ruling out a Resident-Evil-style ring menu, which stops real-time play to select). **Input scheme resolved**: both number-key direct-select (1–9, classic PC survival-game convention) and scroll/cycle (maps directly to a gamepad bumper/d-pad, matching the project's own stated console-friendliness goal) are first-class, not one-or-the-other.
- **Equip/holster/switch takes real time**, choreographed through the existing notify/montage/`bIsBusy` system (same pattern as reload) — not instant, matching PZ's own equip-time model. **Timing model resolved**: an `EquipTimeSeconds`-style field on `UZSWeaponConfig`, same pattern as `FireDamage`/`FireRange` — consistent with the existing per-weapon data-driven rule, no separate weight-class system needed.
- Hotbar/hand-slots hold *references* into P6's `UZSInventoryComponent`; equipping moves/assigns a reference rather than duplicating item state — this phase depends on P6 existing for the item data to reference, even though the slot/dispatch machinery itself lives here.

**Hotbar built 2026-07-21** (same day as the design pass above, compiles pending / not yet PIE-verified): `AZSPlayerCharacter` gained a fixed 9-slot `HotbarSlots` array (seeded at `BeginPlay` from a new `StartingHotbarLoadout` array field, which replaces the old single `StartingWeaponConfig` field), `SelectHotbarSlot`/`CycleHotbar` (number-key and scroll, both built) routed through `Server_SelectHotbarSlot` → a real `EquipTimeSeconds`/`UnequipTimeSeconds`-driven delay (`bIsBusy`-gated, same choreography pattern as reload) → `CompleteHotbarSwitch`. The player now genuinely starts unarmed - `BeginPlay` no longer auto-equips anything. `CurrentWeapon` itself was **not** renamed to `PrimaryHand` (kept as-is deliberately, to avoid the exact class of Blueprint-reference corruption already logged under CLAUDE.md's Live Coding lesson) - it already *is* the primary-hand slot semantically. `SecondaryHand` is still not built - its own action-trigger mechanism is still an open question (below), so a slot nothing could use would be dead code. Needs `IA_HotbarSelect` (Axis1D, digit-key Scalar modifiers) and `IA_HotbarCycle` (Axis1D, mouse wheel) created before it's testable.

**Melee weapon variety resolved 2026-07-21**: curated 4–6 archetypes (one weapon per feel-category — e.g. blunt/edged/improvised/heavy — not PZ's full breadth of near-duplicate items), matching the project's own "roughly 1/3 of PZ's depth" design pillar and solo-dev production reality. Exact items TBD from the already-sourced placeholder packs (`Content/LowPolyWeapons/`, `Content/Mega_Survival_Tools/`, see §5's asset needs list).

**Unified attack input** — one `IA_Attack` button (already renamed from a placeholder `IA_Melee` this session) whose effect depends on what's equipped. **Partially built 2026-07-21** (ahead of the full loadout system, because `IA_Fire` and `IA_Attack` both being separately bound to the same key was actively double-triggering fire+melee on every click and blocking testing):
- `UZSWeaponConfig` gained an `EZSAttackType { Ranged, Melee }` field (no `Unarmed` value — bare-fist isn't a weapon config instance, it's `AZSPlayerCharacter`'s own fallback when nothing's equipped, same as before). Generalizes the config to cover melee weapons too, rather than a parallel melee-config hierarchy — consistent with the existing multi-weapon rule.
- `AZSPlayerCharacter::HandleAttack` (bound to `IA_Attack` alone now — `IA_Fire` is no longer separately bound) dispatches on `CurrentWeapon`'s config: `Ranged` → the existing `HandleFireStarted`/auto-fire-timer machinery (unchanged); no weapon, or `Melee` (no melee `UZSWeaponConfig` instances exist to author yet, so this branch shares the bare-fist path for now) → `Server_MeleeAttack`, using the flat `Unarmed*` character tunables.
- **Still not built**: `SecondaryHand` (see the hotbar note above), and real per-weapon melee stats on `UZSWeaponConfig` (damage/range/interval fields, montage) for whenever a melee weapon actually gets authored — `Melee`-typed configs will silently fall back to bare-fist stats until those fields exist. The hotbar/switch-timing/start-unarmed items are now built - see the note above.
- Melee weapon types through this same pipeline: swing timing via the existing notify system; shove + stomp as always-available options; stamina economy tuned against P2's `UZSNeedsComponent`.
- Firearms integration with noise + zombie mass: ammo scarcity tuning, simple hit-reaction/knockdown.
- Weapon durability-lite (melee breaks; no repair sim v1).
  **Exit:** a player starts unarmed, equips a weapon from the hotbar in real time, and one Attack button does the right thing whether they're bare-handed, swinging a bat, or holding a rifle — the PZ death loop exists on top of that (greed + noise + stamina mismanagement kills a player who had every tool to survive).

### P6 — Inventory, loot & scavenging
- `UZSInventoryComponent`, weight-based encumbrance, bag equip slots (clothing/carry capacity — **not** the `PrimaryHand`/`SecondaryHand`/hotbar combat loadout slots, which are P5's, since they're what a single Attack button dispatches on); `UZSItemConfig` equip-only vs. carry-only categories.
- Container actors + data-asset loot tables; per-zone quality tiers; finite world-count rarity pools.
- Inventory UI + radial quick-use, transparent stat-preview rule carried through.
- Dropped-item persistence.
  **Exit:** full scavenge loop in graybox: run out, loot under threat, haul back, stash; item scarcity feels intentional.

### P7 — World building & persistence
- Art integration phase: replace graybox with the chosen modular kit; build the region.
- Multiple profession-tied spawn points, scatter-spawns toggle wired.
- Enterable buildings; interior visibility solution for top-down.
- Save/persistence v1 (single "world continues" save per server).
- Utilities shutoff goes live against the real map.
  **Exit:** the real map plays end-to-end co-op, including scattered multi-spawn start; quit → relaunch → world remembered; day ~10 the lights die.

### P8 — Dynamic events, objectives & the investigation arc (the differentiator)
- `UZSEventDirector`: scheduled + random world events.
- Radio channel: scripted broadcast arc days 1–7, transitioning into dynamic events + investigation clues.
- Investigation/cure questline: guaranteed clue placement (predetermined pool, random pick, never missing). Optional capstone per Decision 6.
- Radiant objective wrappers — invitations with stakes, never mandatory.
  **Exit:** two co-op sessions on the same map play out differently; a full playthrough of the investigation arc is possible.

### P9 — Meta-loop, onboarding & difficulty
- Character creation v1: backgrounds (§3) + spawn point (Decision 4) + appearance from the art kit's modular characters. No trait point-buy — build variety from background choice and emergent play-driven attribute/skill growth (§3.1).
- Death → new character → same world flow polished.
- First-hour experience pass: radio-guided first days, interaction hints, transparent stat/action previews everywhere.
- **Skill/attribute XP hookup** (learn-by-doing across P2–P6 systems): Strength/Stamina/Sneak/Sprint attributes, per-weapon-class Melee bars, Maintenance, Aiming, Reloading, First Aid — all per §3.1's revised list, wired to their respective P2–P6 systems.
  **Exit:** a stranger survives their first 30 minutes without a wiki and dies to something they understand.

### P10 — Production hardening → public vertical slice
- Audio pass, VFX pass, performance profiling, fixed-tick save safety, crash/soak testing, packaged Windows build tested over real LAN/direct-IP.
- Trailer-able vertical slice: 20–40 minutes of tuned co-op survival on the real map, including at least one meta event and a taste of the investigation arc.
  **Exit:** shippable demo build. **First post-v1 addition (Decision 5): hostile human roamers**, built cheaply on top of P4's zombie AI architecture. Post-v1 backlog also includes the deferred skills from §3.1 (Fishing, Building, Foraging, Cooking, Mechanics), full NPC survivors/factions, vehicles, sandbox sliders, deeper seasons/temperature, Steam/EOS + dedicated server.

**Standing rules across all phases:** replication convention on every new stat/system; data-asset-driven everything; `BlueprintNativeEvent` for gameplay decisions; no magic numbers (`TuningReference.md` stays live); commit per sub-task; docs updated at phase end.

---

## 5. Asset strategy

### The style decision
**⚑ DECISION 3 — Resolved 2026-07-19.** Direction: **dark, earthy tones, slight realism, kept low-poly.** Sourcing is the dev's own.

> **Licensing/repo rule:** paid marketplace content is gitignored, never committed. Large free content that would blow the $0 LFS budget doesn't need to be committed either since it's re-downloadable from its source.

### Asset needs list (running — update as sourced)

| Category | Needed for | Status |
|---|---|---|
| Player character base mesh + rig | P1 (locomotion), ongoing | **On `SKEL_TFA_Mannequin` (Infima's own skeleton)** — confirmed direction; final mesh/look TBD |
| Locomotion animation | P1/Stage A | Retargeted from Lyra onto `SKEL_TFA_Mannequin`; Layered Blend Per Bone architecture confirmed |
| Firearms — rifle | P1 onward | Poses/montages sourced from Infima; graybox mesh, final dark/earthy model still not sourced |
| Firearms — pistol | P1 onward | Blocked on Infima — pack hasn't released pistol animations yet |
| Melee weapons | P5 | Placeholder sourced: `Content/LowPolyWeapons/`, `Content/Mega_Survival_Tools/` |
| Zombie character mesh(es) + rig | P4 | Animation set already imported; mesh/skin not yet sourced |
| Hostile human roamer visuals | Post-v1 | Not needed yet |
| Environment/building modular kit | P7 | Not sourced |
| Rural/forest/mountain biome props | P7 | Not sourced |
| Interactable world props | P6–P7 | Not sourced |
| VFX — muzzle flash / impacts | P10-final-pass | Sourced |
| Audio — bullet impacts / footsteps | P10-final-pass | Sourced |

### Blender pipeline (the DIY path, if/when hand-modeling starts)
Blender 4.x LTS, free. Model on-grid; texture toward dark/earthy/slight-realism; pick one style anchor and hold everything to it.

**Skeleton rule:** everything humanoid in the art pipeline targets **`SKEL_TFA_Mannequin`** (Infima's skeleton) as the one shared hub — imported content gets retargeted onto it, the character never moves to a different skeleton.

### 5.1 — Standard animation set (revised 2026-07-20)

**Status: real production underway.** Locomotion architecture confirmed: `ZS_BS_Unarmed_Idle_Walk_Run` drives legs for every state; Layered Blend Per Bone (split at `spine_02`) composites the equipped weapon's pose over it. Two base locomotion blend spaces total (standing, crouched). No Aim Offset layer needed — hybrid facing (movement-direction default, cursor-facing only while actively aiming/attacking) covers it. No jump verb — replaced conceptually by a future mounting system.

**AnimGraph built and compiled clean, 2026-07-20:** `ABP_ZS_ThirdPerson`'s stale pre-pivot graph cleared; new graph built on `BS_ZS_Unarmed_Idle_Walk_Run` / `BS_ZS_UnequippedCrouchWalk` + Infima's rifle idle/aim poses via Layered Blend Per Bone. Stance selector's `bIsCrouched` pin wired.

**Stage A — base locomotion:** idle, directional walk/jog, sprint, crouch idle/walk, rifle/pistol idle+aim pose layered over the walk cycle. Jump cut entirely.

**Stage B — montages:** fire, reload, generic use/channel loop, hit reaction, death, melee swing — Infima where available, TBD sourcing otherwise.

**Stage C — zombies (P4):** `/Game/Animation/Enemy/Zombie/` walk/chase/attack/scream/death/crawl, already imported.

**Explicitly out:** Lyra's full traversal-and-combat library (dodges, cover, prone, swimming, climbing, mantle/vault, finishers) — not an invitation to wire up.

---

## 6. Scope guardrails & risks

| Risk | Mitigation |
|---|---|
| **Pivot whiplash** | P0 verifies + commits a known-good baseline first. |
| **Top-down doesn't feel right** | P1 is a cheap identity gate before art spend; over-shoulder TP is the fallback. |
| **Simulation creep** | §3's table is a contract. |
| **Zombie counts vs. performance** | Low-poly + flat materials; P4 sets a profiled budget. |
| **MP save/persistence complexity** | Listen-server-host-owns-the-save, single world save. |
| **The investigation arc vs. infinite sandbox tension** | Decision 6 resolved before P8's back-end is built. |
| **NPCs are a siren song** | Full factions stay hard-gated behind a post-v1 planning pass. |
| **Solo-dev art volume** | Buy the core, one region not a county-sized map. |
| **Animation scope re-creep** | The §2 rule: readable-at-camera-distance or gameplay-gating, else polish-phase. |

**Rough shape of the calendar:** P0–P1 ≈ 2–3 weeks · P2–P3 ≈ 3–4 weeks · P4–P5 ≈ 4–6 weeks · P6 ≈ 2–3 weeks · P7 ≈ 4–6 weeks · P8 ≈ 3–4 weeks · P9–P10 ≈ 4–6 weeks. **Order-of-magnitude: ~6–9 months.**

---

## 7. Open questions by development stage

### Cross-cutting / foundational
1. Document filename — still blank.
2. Real marketing title for the game — later.
3. **Platform commitment (blocking for P1):** PC/Steam-only for v1?
4. Team-size reality check.
5. Save architecture: one world/save per server, or multiple concurrent slots?
6. **Left-click's dual meaning (dev note, 2026-07-21):** once any menu/radial/inventory UI exists (P1's radial quick-menu, P6's inventory), left-click needs to mean "select the focused UI option" while a menu has focus, and "attack/interact per IA_Attack" otherwise — the same physical input, gated by whether a menu currently has focus. **Not built yet** — no menu system exists to gate against (P1's world-prompt interaction is keybound to `E`/`IA_Interact`, not mouse click). The standard Enhanced Input mechanism for this is an input-context swap: an `IMC_ZS_UI` mapping context (left-click → UI select) that gets `AddMappingContext`'d with higher priority than `IMC_ZS_Default` while a menu is open, and removed on close — not a per-click `bIsMenuOpen` branch inside `HandleAttack`. Revisit when P1's radial menu or P6's inventory UI actually gets built.

### P0 — Close-out / re-aim
1. Is the Infima FP investment a total write-off? — **Resolved: no, Infima is the confirmed skeleton/animation source of record, not shelved.**
2. Any content the dev wants explicitly untouched during de-scope?

### P1 — Camera & control prototype
1. Controller or KBM as primary tuning target?
2. Prototype scattered profession spawn here or wait for P7?
3. Interior visibility for top-down — worth a spike here?

### P2 — Survival core
1. ~~Final skill list~~ **RESOLVED 2026-07-19** — see §3.1 (revised from the original six-skill list).
2. Solo sleep-skip without a group check?
3. Floor under Hunger/Thirst debuffs?
4. Weather — real mechanics in v1 or atmospheric-only?

### P3 — Health, damage, medical, amputation
1. Amputation tool requirement + solo vs. co-op-assist?
2. Timing window for amputation?
3. Post-amputation: permanent-only for v1, or flag for later prosthetics?
- **Post-initial-completion backlog** (dev notes, 2026-07-20 - refine once P2-4's core loop is proven, not before):
  - Amputation/cutting needs its own player animation - currently a bare C++ mutator (`Server_AmputateZone`) with no montage.
  - Amputating causes a blackout. Solo: game time accelerates ~12 real hours forward during the blackout - a real risk window (enemies can find and kill the incapacitated player), so picking a safe spot to amputate becomes a genuine tactical decision. Co-op: still a blackout, but another player can move the downed body, and a revive from a teammate shortens the blackout duration.
  - Arm amputation restricts weapon use to one-handed options only (handgun or a one-handed melee weapon) - two-handed weapons become unusable with that arm gone.
  - Medical item tier delays bite→infection conversion (extends the incubation window), giving more time to decide on amputation - a new per-tier delay field on `UZSItemConfig`'s Bandage/Disinfectant entries, not built yet.
  - **Player death, loot, and world continuity** (also touches P4/persistence): on death, dropped loot stays at the death location, and the player's character becomes a zombie (P4 - `AZombieCharacter` doesn't support death-triggered spawning yet, only placement/config-driven). Co-op: the party continues in the same world with a fresh character *unless the entire party has died*, in which case that world is over. Solo: death ends that world outright - a fresh world + fresh character, not just a fresh character in the same world. **This is a real, notable change from what `AZSPlayerCharacter::Server_RespawnAsNewCharacter` currently does** (always respawns into the same world, regardless of solo/co-op or how many players remain) - flagged here so it isn't silently lost; revisit `HandleDeath`/`Server_RespawnAsNewCharacter` when this is scheduled.

### P4 — Zombies
1. Is ~150 concurrent on-screen zombies the right target?
- Post-v1 backlog: hostile roamer spawn logic, loot drops, wound/infection system parity.

### P5 — Loadout & unified combat
**All resolved 2026-07-21:**
1. ~~Melee weapon variety~~ **RESOLVED** — curated 4–6 archetypes (one per feel-category), not PZ's full breadth.
2. ~~Durability~~ **RESOLVED** (already settled by the existing plan text) — break-only for v1, no repair sim.
3. ~~Hotbar size and key scheme~~ **RESOLVED** — both number-key direct-select (1–9) and scroll/cycle supported, not one-or-the-other; scroll/cycle is the gamepad-friendly path.
4. ~~`SecondaryHand` semantics~~ **RESOLVED** — independently usable (offhand pistol/flashlight), not just "the other grip." Follow-on question, not yet resolved: how an offhand item's own action gets triggered, since `IA_Attack`/`Server_Attack` only ever considers `PrimaryHand`.
5. ~~Equip/holster timing~~ **RESOLVED** — a per-`UZSWeaponConfig` field (e.g. `EquipTimeSeconds`), same pattern as `FireDamage`/`FireRange`, not a separate weight-class system.

### P6 — Inventory & loot
1. Bag/equip-slot depth?
2. Finite-rarity-pool model — per-world-instance or fixed design target?

### P7 — World & persistence
1. Naming pass for the fictional county/towns?
2. Map scale — still ~1×1 km?
3. Scatter-spawns default on or off?

### P8 — Dynamic events, objectives & investigation arc
1. ~~Decision 6~~ **RESOLVED** — optional capstone.
2. ~~Clue placement~~ **RESOLVED 2026-07-18.**
3. **(blocking)** How many distinct meta-events wanted at launch?

### P9 — Onboarding & meta-loop
1. Lightweight build-variety at character creation beyond background + spawn point?
2. Keep the death-recap screen?

### P10 — Hardening & vertical slice
1. Target audience for the public vertical slice?
2. Any external anchor date?

---

## 8. Immediate next steps

**Resolved as of 2026-07-19/20:** Decisions 4, 5, 6, doc filename (keep), P2 skill list (revised per §3.1), P8 clue placement, P0's Infima question (confirmed as the animation source of record, not shelved), P1's hybrid facing, P1's animation architecture (Layered Blend Per Bone).

1. Decisions 1–3 confirmed. Remaining **(blocking)** §7 question: P8 meta-event count for launch.
2. `CLAUDE.md` and `SessionHandoff.md` updated 2026-07-19/20 to reflect current state.
3. P0 de-scope pass complete; Stage A locomotion in active progress (see `SessionHandoff.md` for exact next step).
4. Asset sourcing ongoing, dev-paced.
