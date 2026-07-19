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
- **3D fixed-angle top-down camera with direct WASD + mouse-aim control** — not PZ's menu-driven isometric interaction. Combat feels like a twin-stick/tactical shooter; survival feels like PZ. (This also happens to be the more console-friendly of the two — a stick-aim top-down camera translates to a gamepad far more directly than isometric-plus-right-click-context-menus does, which helps Notes §3's console want land for free later.)
- **Co-op-first (2–4 players, listen server)** — every system built replicated from day one, which is already this codebase's DNA. Profession/background choice now also picks a **starting spawn location** (Notes §4.1) — see Decision 4.
- **Dynamic events, radiant objectives, and a discoverable investigation arc** — helicopter-class events (PZ §17) as an expanded *system* (Notes §17: "create more meta events"), radio-driven objectives, and a questline built from notes/documents/items scattered through the world that lets players piece together the outbreak's origin and chase a cure (Notes §1/§22).
- **Hostile human roamers** — always-hostile wandering human NPCs that fight zombies *and* players, never allies (Notes §19: "never friendly"). This is the "enemy variety Romero purism forbids" the reference doc's own §23 flags as a differentiation opportunity, and it's cheap here specifically because it reuses the zombie AI/perception/noise pipeline (see Phase P4 and Decision 5). Full NPC survivors/factions/dialogue/reputation systems are a different, much bigger system and stay deferred to their own planning pass.
- **Simplified simulation** — every PZ system ships here at roughly **1/3 of PZ's depth**, chosen for readability. Depth can grow later; opacity is not a feature we inherit.
- **Modern, transparent UX** — direct interaction prompts, radial quick-menu, and (Notes §21) **items/actions show their actual mechanical effect on hover/preview** rather than hidden numbers — "player knowledge is mainly built on common sense and playing the game," not a wiki.

**⚑ DECISION 1 — Camera.** Recommended: **fixed-angle top-down** (pitch ~55°, zoom 8–20 m, yaw rotation in 45° steps). The existing `ApplyCameraPerspective` architecture makes this a *new perspective entry*, not a rewrite — cut the perspective list from 4 (FP/TP/GunCamera/Bodycam) down to **TopDown + OverShoulder** (over-shoulder kept as an aim-zoom and as a hedge). First-person, GunCamera, Bodycam: shelved (code kept in git history, perspectives removed from the cycle). Unchanged from v0.1 — nothing in the dev's notes contradicts it, and the console-friendliness argument above is a point in its favor.

**⚑ DECISION 2 — Same repo vs. fresh project.** Recommended: **same repo, same UE project.** Unchanged from v0.1.

**⚑ DECISION 3 — Art source.** Recommended: **hybrid — buy the Synty POLYGON core, fill gaps in Blender.** Unchanged from v0.1 — see §5. (The new Adirondacks setting, if anything, strengthens this: Synty's rural/wasteland and forest-biome packs and any low-poly winter/pine-forest kits fit a mountain-and-lakes region better than the original flat-Kentucky placeholder would have.)

**⚑ DECISION 4 (NEW) — Profession-based spawn points: solo too, or co-op-only?** Notes §4.1 wants players to start at a profession-tied spawn location, explicitly calling out the co-op "find each other" fun. Recommended: **apply to both, with a lobby-level "scatter spawns" toggle.** Solo players still get a profession-flavored starting location (keeps one system instead of two), but a co-op group can toggle scattered starts off if they'd rather just start playing together immediately. Confirm in §7.

**⚑ DECISION 5 (NEW) — Hostile human roamers: ship in P4 alongside zombies, or a later stretch goal?** Notes §19 asks for them unconditionally, not conditionally — the open question is *timing*, not *whether*. Recommended: **ship a lightweight version in Phase P4**, reusing the Behavior Tree/Blackboard/`AIPerception`/noise-system infrastructure zombies already need — the incremental cost is a new pawn + "hostile to everyone" faction rule, not new AI architecture. Depth (varied loadouts, ranged AI behavior, camp setpieces) can still grow post-v1. Confirm timing in §7.

**⚑ DECISION 6 (NEW) — Does the investigation/cure arc end or reset the world?** Notes §1 removes "no win condition" as a philosophy but doesn't say the game should *end* on completion — it says add an "end goal to survival," which reads as something to chase, not a game-over screen. Recommended: **the investigation arc is an optional capstone with no forced ending** — reaching its conclusion unlocks a lore epilogue and a meaningful, persistent world-state change (example: a rescue/evac becomes available, or a new sandbox modifier unlocks), but the world keeps running and the character can keep playing, same continuity spirit as PZ's own permadeath-into-new-character loop. This is the single highest-leverage open question in the whole plan — it shapes world-state/save architecture (can a "completed" flag exist per-character without breaking co-op persistence?) more than any individual system does. Confirm in §7 before Phase P8 design work starts in earnest.

**Working title:** repo/project name stays "ZombieShooter" (per `CLAUDE.md`, a placeholder). This planning document's own filename is a separate, smaller open question — see §7.

---

## 2. What happens to the current project

The pivot **keeps the repo, the project, and the C++ core.** What we've built is mostly camera-agnostic infrastructure; what changes is the presentation layer and where future effort goes.

### Carries over as-is (the pivot's foundation)
| System | Why it survives |
|---|---|
| `AZSGameMode/GameState/PlayerState/PlayerController` + Enhanced Input | Camera-agnostic framework. Input actions get remapped, not rebuilt. Enhanced Input already abstracts keyboard/mouse vs. gamepad bindings, which is most of what "console-playable later" (Notes §3) actually needs at the input layer — the remaining console work is UI navigation and platform certification, both later/separate concerns. |
| **Phase 3 replication layer** (server RPCs, `OnRep_` convention, cross-client fixes) | The single most valuable asset for a co-op-first game. Finish M7 verification before pivoting — see Phase P0. |
| `UZSWeaponConfig` data-driven weapon architecture + "N weapons, zero C++ branches" rule | Extends beyond guns: the same pattern becomes `UZSItemConfig`, `UZSZombieConfig`, and now a shared config for hostile human roamers too. |
| Real ammo/magazine state on `AZSWeapon` | PZ-style scarcity needs real ammo. Already built, already replicated. |
| Notify architecture (`AN_ZS_UnlockActions`, `ANS_ZS_BlockADS`, montage-driven action flow, `bIsBusy` + fallback) | Generalizes to every timed action this game will ever have: melee swings, bandaging, amputating, barricading, crafting. This is *not* wasted animation work — it's the action system. |
| `BlueprintNativeEvent` policy, replication convention, naming rules, docs discipline, MCP workflow | Unchanged. |

### Simplified or repurposed
| Current | Disposition |
|---|---|
| 4-perspective camera system | Cut to TopDown + OverShoulder (Decision 1). `ToggleCameraPerspective` stays as the toggle between the two. |
| FP arms mesh/AnimBP (`ABP_ZS_FirstPerson`, FABRIK, additive stacks, camera/head toggle) | **Shelved, not deleted** — disconnect from the spawn path. No further investment. |
| TP AnimBP (`ABP_ZS_ThirdPerson`) | Becomes the *only* character view. Gets rebuilt against the new art skeleton (see §5) using Epic's free Game Animation Sample locomotion — replacing the Infima-skeleton-specific graph. |
| Infima pack | **Demoted from "reference of record" to "prototype placeholder."** Keep using its weapons/animations in graybox until low-poly art lands, then it exits the runtime entirely (it stays gitignored either way). What we permanently keep from Infima is what we already extracted: the config-driven weapon architecture, notify concepts, and real-reload flow. |
| `Docs/Infima Pack - Official Implementation Guide/` | Kept for history; no longer drives new work. |

### Cut outright (the animation de-scope the dev asked for)
- **`Inspect`, `MagCheck`, `CycleGripAttachment`** — actions, input bindings, montage wiring. (The montages/notify classes they exercised stay; they're generic.)
- **`AZSLaserAttachment`**, grip-attachment randomization/variants.
- **Weapon-owned cosmetic notifies + `ABP_Weapon`/`ABP_Magazine`** (`AN_ZS_DropMagazine`, `AN_ZS_EjectCasing`, `ANS_ZS_HideMainMag`, `ANS_ZS_ShowReserveMag`) — already deferred in M9; now formally cut. At top-down camera distance nobody sees a magazine hide/show swap.
- **`AZSPhysicsCasing`/`AZSPhysicsMagazine`/`AZSPhysicsObject`** cosmetic ejects — cut from the runtime path (class files can stay until they're in the way).
- `FP_ReloadEmpty`/`TP_ReloadEmpty` variants, gun-camera/bodycam content, procedural ADS/recoil/crouch **spring-offset system** (top-down recoil is a crosshair/spread concern, not a skeletal-pose concern).

**Rule going forward:** an animation earns its place only if it's *readable at gameplay camera distance* or *gates gameplay timing* (reload lockout, swing timing, bandage/amputation channel). Everything else is polish-phase-only.

---

## 3. PZ systems disposition (KEEP / SIMPLIFY / REPLACE / CUT)

The reference doc asks for exactly this markup; the dev's own notes refine several rows further. This is the scope contract — anything marked CUT stays cut until a dedicated planning pass revives it.

| PZ system (ref §) | Disposition | Our version |
|---|---|---|
| Isometric camera, menu-driven interaction (§3) | **REPLACE** | 3D top-down, direct control, world interaction prompts + radial quick-menu. Controller-compatible from P1 onward (Notes §3). |
| Professions/occupations (§4.1) | **SIMPLIFY** | 5–7 starting "backgrounds" = stat template + 1 unique unlock + **a tied starting spawn location** (Notes §4.1, Decision 4). Data-asset-driven (`DA_ZS_Background_*`). |
| Trait point-buy (§4.2) | **REPLACE** | No creation-time point-buy in v1. Traits/aptitudes emerge from play instead (Notes §4.2) — see the new skills/attributes system below. Classic point-buy could return later as an optional sandbox/"hardcore" toggle. |
| Moodles / needs (§5) | **KEEP, simplified** | **6 moodles v1:** Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness. Hunger/Thirst explicitly tuned as **debuff-first, not death-spiral-first** (Notes §1) — see Phase P2. Panic/Stress/Boredom/Temperature: deferred pool. Same 4-severity-tier readable iconography. |
| Nutrition micro-sim (calories/protein/fat) (§5) | **CUT** | Food restores Hunger; quality = bigger/longer restore. |
| Skills, learn-by-doing, books (§6) | **SIMPLIFY, still narrowing** | Dev's own note (§6): "narrow down later by asking questions." Working recommendation: ~8 skills (Melee, Firearms, Sprinting/Fitness, Carpentry, Medicine, Cooking, Scavenging, Mechanics-lite), learn-by-doing + magazine-style one-shot unlocks, no Vol 1–5 book grind. Treat as unconfirmed — see §7. |
| Melee combat, stamina economy (§7.1) | **KEEP** | Core feel target. Shove + swing + stomp, stamina-gated, weapon durability-lite (break, no repair micro-sim). |
| Firearms as loud/scarce power (§7.2) | **KEEP** | Already built mechanically; noise system makes it PZ-honest. |
| Stealth/noise/vision model (§7.3) | **KEEP, simplified** | Crouch = quieter + slower; AI hearing radii per action; line-of-sight vision cones. No lightfootedness skill web in v1. |
| Zombie lore + hordes + migration (§8) | **KEEP, simplified** | Shamblers, hearing/sight, door-banging, crawlers (later), zone population + wander. **Explicit performance requirement** (Notes §1): zombie AI must be CPU-cheap enough to support massive hordes, and the game needs a believable ongoing zombie-reintroduction mechanism so an area can't be permanently cleared. Full sandbox lore sliders (§8.3): CUT for v1, revisit as a cheap post-launch win. |
| Per-body-part health, 17 zones (§9) | **SIMPLIFY** | **4 zones:** Head, Torso, Arms, Legs. Wound types: scratch/bite/laceration/fracture, each mapped to a concrete gameplay effect (Notes §1: leg wounds → mobility/speed, arm wounds → attack speed/reload time). Bandage cleanliness + the two-tier infection dread: KEPT. **New:** emergency amputation as an infection-stopping "second chance" (Notes §1/§9) — see Phase P3. |
| Hand-built mega-map (§10.1) | **REPLACE** | A fictional county modeled on Upstate NY's Adirondacks (Notes §10.1/§11) — mountains, lakes, forest, small towns — keeping the same *location archetypes* PZ uses (small town, dense town, quiet suburb, mall analog, military-adjacent area, farm fringe). One dense playable area + rural fringe (~1×1 km v1, size TBD — see §7), modular-kit built, World Partition. Final naming is its own open question, not yet settled. |
| Container/zone loot tables (§10.2) | **KEEP, refined** | Data-asset loot tables per building/container archetype. **Item categories** (Notes §10.2): equip-only slots vs. carry-only items, one item per container slot. **Scarcity via a finite world-count pool per rarity tier**, layered under per-zone quality tiers (good/bad/in-between areas) with slight randomization plus common-sense placement — not just a percentage roll per table. Annotated-map stash hints: KEEP. |
| World persistence + erosion (§10.3) | **SIMPLIFY** | Persistence: yes (saves are Phase P7). Erosion visuals: CUT v1. |
| Utilities shutoff phase transition (§10.3) | **KEEP** | Power/water die on a randomized day — our mid-game bell too. |
| Seasons/weather/temperature (§11) | **SIMPLIFY** | Day/night + rain/fog v1, tuned to the Adirondacks setting (colder, more snow) rather than the original Kentucky climate. Weather has real mechanical teeth (Notes §1: "players must adjust to survive"), scope TBD — see §7. Full seasonal calendar + temperature-layer survival: deferred pool. |
| Weight encumbrance + bags (§12) | **KEEP, simplified** | Weight + bag slots. No bags-in-bags recursion, no clothing-layer insulation matrix. |
| Clothing protection layers (§12) | **SIMPLIFY** | Single outfit slot-set with protection values. No layering sim. |
| Carpentry/base-building (§13.1) | **SIMPLIFY** | v1: barricades, door/window reinforcement, crates, rain collector. Freeform wall-building: deferred (big system, PZ's answer to late-game — ours is events + the investigation arc). |
| Metalworking/electrical/plumbing chains (§13.2) | **CUT v1** | Generator-as-item (fuel logistics) without the wiring sim. |
| B42-style deep crafting web (§13.3) | **CUT** | Explicitly the direction we *don't* chase — events/missions/investigation are our late game. |
| Farming/foraging/fishing/trapping (§14) | **SIMPLIFY** | v1: farming-lite (plant/water/harvest) + foraging zones. Fishing/trapping: deferred pool. |
| Vehicles (§15) | **CUT v1, plan later** | Big, physics-fiddly, and PZ+HumanitZ's shared weak spot. Deferred to its own planning pass post-v1. Map is sized to be walkable. |
| TV/radio diegetic media (§16) | **KEEP, repurposed** | The radio is our tutorial *and* our mission-giver — broadcast schedule teaches systems early, then becomes the dynamic-objective and investigation-arc-clue channel. |
| Meta events (helicopter etc.) (§17) | **KEEP, expanded ambition** | Notes §17: "create more meta events" — treat event *variety* as a primary content lever, not an afterthought. See Phase P8. |
| Modes/challenge presets (§18) | **CUT v1** | One tuned default. Sandbox sliders post-v1. |
| MP: dedicated servers, big counts (§19) | **SIMPLIFY** | 2–4 player listen-server co-op. Dedicated/Steam sockets: own planning pass later (per existing `CLAUDE.md` rule). |
| Modding/Lua (§20) | **CUT v1** | Data-asset-driven design keeps the door open; actual mod support is post-launch. |
| NPCs/factions (§19, §22.1) | **SIMPLIFY (partial, retimed)** | **In scope for v1, but narrowly:** always-hostile wandering human roamers that fight zombies and players alike, never allies (Notes §19). Ships alongside zombies in Phase P4 (Decision 5), reusing the same AI pipeline. Full NPC survivors/factions/dialogue/reputation/economy systems remain **deferred to a dedicated planning pass** — that's a fundamentally different, much larger system than a hostile roamer variant. |

---

## 4. Development phases

Same working style as `CoreLoopPlan.md`: numbered phases, milestone tables, PIE-verified exit criteria, commit per sub-task, docs updated at phase end. Phases are ordered so **every phase ends in something playable** and the risky/identity-defining systems come early. **Read §7's questions for a phase before starting it** — several phases have decisions embedded that change their own scope.

### P0 — Close out, clean up, re-aim (the simplification pass)
1. **Finish Phase 3 M7** (2-client PIE verification of the existing replication layer, dev-driven, checklist already in `CoreLoopPlan.md`). Verify what's built *before* surgery, so post-surgery breakage has a known-good baseline.
2. Commit the currently-uncommitted session-8 work (already compiled clean).
3. **De-scope pass** per §2's cut list: remove Inspect/MagCheck/SwitchGrip input actions + bindings; strip the actions and laser/grip/physics-cosmetic paths from `AZSPlayerCharacter`/`AZSWeapon`; retire the FP spawn path (keep `FirstPersonMesh` component dormant or remove — smallest safe diff wins); reduce perspective enum. Compile + PIE after each removal cluster.
4. Update `CLAUDE.md`, `SessionHandoff.md`; this doc becomes plan of record.
   **Exit:** clean build, 2-client PIE still passes fire/reload/aim/sprint/crouch with the slimmed action set.

### P1 — Camera & control prototype (identity test #1)
- TopDown perspective in `ApplyCameraPerspective` (pitch/zoom/step-rotation), cursor-projected aim (screen ray → ground plane → character aims at point), movement relative to camera, OverShoulder aim-zoom toggle.
- Interaction system v1: `UZSInteractableComponent` + world prompt ("E — Open").
- Input scheme validated with both mouse+keyboard and a gamepad from day one (Enhanced Input already supports the dual bindings; this is a tuning/testing task here, not new plumbing) — real console porting/certification stays a later, separate decision.
- Graybox test map. Infima rifle still the stand-in weapon.
  **Exit:** moving/aiming/shooting *feels good* at top-down distance with both input methods, 2-client PIE. **This is the go/no-go gate on Decision 1 — settle the camera before any art money is spent.**

### P2 — Survival simulation core (identity test #2)
- `UZSNeedsComponent` (ActorComponent, replicated, data-asset-tuned): Hunger/Thirst/Fatigue/Stamina + rate curves. **Consequence model per Notes §1:** Hunger/Thirst degrade stamina regen, healing rate, aim accuracy, and attack recovery *before* they ever threaten health directly — health drain only at sustained, deep neglect, not the default outcome of forgetting to eat for a day.
- World clock (`AZSGameState`): day/night, configurable compression, the utilities-shutoff timer.
- **Sleep/time-skip** (Notes §2, Minecraft-style): sleeping requires being safe within a radius of hostiles; in co-op, time only advances once every player is asleep/ready, for a duration the initiating player sets.
- Moodle UI stack (UMG, 4 severity tiers) + first-pass HUD, with the transparent stat-preview rule (Notes §21) established here for the first time — every consumable/action shows its actual effect on hover, not a hidden number.
- Items exist minimally: eat/drink consumables via a first `UZSItemConfig`.
  **Exit:** a character's hunger/thirst visibly degrades performance (not health) under normal neglect, and sleep-based time-skip works solo and with a multi-player readiness check; replicated, 2-client PIE.

### P3 — Health, damage & medical-lite
- `UZSHealthComponent`: 4 zones, wound records (scratch/bite/laceration/fracture) each mapped to a real gameplay effect (leg wounds → move speed/mobility, arm wounds → attack speed/reload time — Notes §1), bleed-over-time, all damage through `TakeDamage`.
- Treatment actions (montage + notify-gated, reusing the action system): bandage (cleanliness flag), disinfect, splint.
- **Knox-style infection:** bite → hidden infection roll → delayed queasy→fever→death arc; deliberately UI-ambiguous vs. ordinary sickness.
- **Emergency amputation** (Notes §1/§9, new mechanic): removing a bitten/infected limb before the infection timer completes stops that infection source outright, at the cost of permanent capability loss to that limb (mobility/attack-speed penalties from the zone mapping above, now permanent rather than healing). Scope questions (tool required? solo-capable or co-op-assist-only? timing window?) are in §7 — build the simplest version first (any bladed/tool item, solo-capable, works any time before the infection timer expires) and refine once played.
- Player death → spectate/respawn-as-new-character flow (permadeath groundwork).
  **Exit:** a scripted damage source can wound (with the correct gameplay-effect mapping), infect, and kill a player who mismanages treatment — and a player who amputates in time survives a bite that would otherwise have killed them. Second client sees everything correctly.

### P4 — Zombies & hostile roamers (the enemy, finally)
- `AZombieCharacter` + `AZombieAIController`, classic **Behavior Tree + Blackboard** (per the standing `CLAUDE.md` decision), `UZSZombieConfig` data asset (speed/health/senses/damage — N zombie types, zero C++ branches, same rule as weapons). **Explicit performance target from the outset** (Notes §1): profile early, don't retrofit efficiency later.
- Perception: `AIPerception` sight cone + hearing. **`UZSNoiseSystem`:** every loud act (gunshot, sprint, breaking glass) reports a noise event with a radius; this is the load-bearing system of the whole game.
- Behaviors v1: wander, investigate noise, chase, attack (melee hit → P3 damage/infection), door-thumping (destructible door HP).
- **Zombie reintroduction:** zone-based population from data, server-authoritative, respawn-into-cleared-zones on a slow timer — the concrete answer to Notes §1's "player can't just clear them all forever."
- **Hostile human roamers** (Decision 5, Notes §19): a second AI variant sharing the same Behavior Tree/perception/noise pipeline, `UZSHostileConfig` data asset, always-hostile faction tag (attacks zombies and players alike, never allies with either). Simplest version first: melee-only or basic ranged, no camp/loadout variety yet — depth is a post-v1 lever.
- Placeholder visuals: Mixamo/UE-mannequin zombie + Mixamo zombie animations (free) until the art phase; hostile roamers reuse the same base mannequin with a different outfit/color read.
  **Exit:** a graybox block with a profiled zombie-count budget met; a gunshot visibly drags the neighborhood onto the shooter; a hostile roamer will fight either zombies or the player depending on who's closer; 2-client PIE holds up.

### P5 — Combat completion (melee + the full loop feel)
- **Melee weapon type** through the *same* `UZSWeaponConfig` pipeline (a melee config specifies swing montages, reach, stamina cost, damage, durability — the multi-weapon rule pays off here).
- Swing timing via the existing notify system; shove + stomp as always-available options; stamina economy tuned against P2.
- Firearms integration with noise + zombie mass: ammo scarcity tuning, simple hit-reaction/knockdown.
- Weapon durability-lite (melee breaks; no repair sim v1).
  **Exit:** the PZ death loop exists — greed + noise + stamina mismanagement kills a player who had every tool to survive.

### P6 — Inventory, loot & scavenging
- `UZSInventoryComponent` (replicated), weight-based encumbrance, bag equip slots; `UZSItemConfig` grows into the general item contract with **equip-only vs. carry-only categories** (Notes §10.2) so weapons/armor claim dedicated slots while general loot doesn't.
- Container actors + data-asset **loot tables** keyed by building/container archetype, one item per container slot; **per-zone quality tiers** (good/bad/in-between areas) with slight randomization, plus a **finite world-count pool per rarity tier** so genuinely rare items stay rare across a whole session, not just per-roll (Notes §10.2).
- Inventory UI (list-based dual-pane, bulk actions, favorite/junk — beat PZ's UI, don't copy it) + radial quick-use, with the transparent stat-preview rule from P2 carried through every item tooltip.
- Dropped-item persistence in the running session.
  **Exit:** full scavenge loop in graybox: run out, loot under threat, haul back, stash; item scarcity feels intentional, not just random.

### P7 — World building & persistence
- **Art integration phase** (see §5): replace graybox with the chosen modular kit; build the region (World Partition) — residential streets, main-street commercial row (hardware/pharmacy/gun store/grocery — the loot-archetype anchors), gas station, church, farm fringe, mountain/forest terrain, at least one larger "dense town" analog. Naming and exact scale are open questions — see §7.
- **Multiple profession-tied spawn points** (Decision 4) placed across the map, with the "scatter spawns" co-op toggle wired.
- Enterable buildings as the rule; interior visibility solution for top-down (roof fade/cutaway — prototype early in P1 if it worries us).
- **Save/persistence v1** (single "world continues" save per server): world item/container/door state, character sheets, clock, zombie population coarse state. Host-side SaveGame; permadeath = character deleted, world persists.
- Utilities shutoff goes live against the real map (powered lights/fridges/pumps flip off).
  **Exit:** the real map plays end-to-end co-op, including a scattered multi-spawn start; quit → relaunch → world remembered; day ~10 the lights die.

### P8 — Dynamic events, objectives & the investigation arc (the differentiator)
- `UZSEventDirector` (server): scheduled + random world events from data assets — helicopter flyover (drags hordes), distant gunshots/screams (ambient migration), crashed convoy/supply drop (timed loot beacon = risk/reward), house alarms. **Expanded roster relative to PZ's own set** (Notes §17) — event variety is a stated priority, not a nice-to-have.
- **Radio channel:** scripted broadcast arc for days 1–7 (diegetic tutorial, PZ §16's trick) that transitions into dynamic event/objective announcements *and* the first investigation-arc clues.
- **Investigation/cure questline** (Notes §1/§22): notes, documents, and items scattered through the world let players piece together how the outbreak started and pursue leads toward a cure. Resolve Decision 6 (does completion end/reset the world, or is it a persistent capstone?) before designing this system's back-end — the answer changes whether "completed" is per-character state, per-world state, or purely cosmetic/lore.
- Radiant objective wrappers ("reach the drop before it's swarmed," "restore the station generator") — objectives are *invitations with stakes*, never mandatory quests.
  **Exit:** two co-op sessions on the same map play out differently because the director dealt different beats; a full playthrough of the investigation arc is possible and its ending behaves per Decision 6's resolution.

### P9 — Meta-loop, onboarding & difficulty
- Character creation v1: backgrounds (§3) + spawn point (Decision 4) + appearance from the art kit's modular characters. No trait point-buy in v1 (§3) — build variety comes from background choice and emergent play-driven aptitudes instead.
- Death → new character → same world flow polished (find your old corpse — keep PZ's beloved beat).
- First-hour experience pass: radio-guided first days, interaction hints, transparent stat/action previews everywhere (Notes §21 as a hard onboarding requirement, not just a UI nicety).
- Skill XP hookup (learn-by-doing across P2–P6 systems) — finalize the actual skill list here if it wasn't already nailed down in P2 (§7).
  **Exit:** a stranger survives their first 30 minutes without a wiki and dies to something they understand.

### P10 — Production hardening → public vertical slice
- Audio pass (see §5 asset list), VFX pass (low-poly-friendly: flat-shaded blood/muzzle/impact), performance (zombie + hostile-roamer count profiling, LODs/HISM on the kit, net relevancy), fixed-tick save safety, crash/soak testing, packaged Windows build tested over real LAN/direct-IP.
- Trailer-able vertical slice: 20–40 minutes of tuned co-op survival on the real map, including at least one meta event and a taste of the investigation arc.
  **Exit:** shippable demo build. Post-v1 planning pass picks from: full NPC survivors/factions, vehicles, sandbox sliders, deeper seasons/temperature, Steam/EOS + dedicated server, deferred-pool systems.

**Standing rules across all phases** (inherited, still binding): replication convention on every new stat/system; data-asset-driven everything (`N` of a thing, zero C++ branches); `BlueprintNativeEvent` for gameplay decisions; no magic numbers (`TuningReference.md` stays live); commit per sub-task; docs updated at phase end.

---

## 5. Asset strategy

### The style decision
**⚑ DECISION 3 — Art source.** Recommended: **hybrid — buy the Synty POLYGON core, fill gaps in Blender to match it.** Unchanged from v0.1.

Whatever is chosen: **pick one style anchor and make everything else conform to it** (proportions, texel-less flat/gradient-atlas texturing, palette). Mixed low-poly styles read worse than consistent mediocre ones.

> **Licensing/repo rule (existing `CLAUDE.md` pattern, applies to all of these):** paid marketplace content is **gitignored, never committed** to the public repo — same as `Content/InfimaGames/`. CC0 content may be committed.

### Paid core (Fab / Synty Store — watch for Humble Synty bundles, they recur and are drastically cheaper)
| Pack | What it covers | Notes |
|---|---|---|
| **[POLYGON Apocalypse](https://syntystore.com/products/polygon-apocalypse-pack)** | The bulk of the game: 1,800+ prefabs, **modular buildings with enterable interiors**, modular bunker/quarantine walls, **modular gun system** (build our weapon variety from parts), 86 complete weapons incl. melee | The single highest-value purchase; UE-native version on Fab. Its modular gun system slots straight into `UZSWeaponConfig`-per-weapon. |
| **[POLYGON City Zombies](https://syntystore.com/products/polygon-city-zombies-pack)** | 50 zombie characters w/ color variants | Rigged compatible with the Synty/UE-mannequin-style skeleton → retargets to the MoCap Online / Mixamo zombie sets. Also the obvious base for hostile-human-roamer visuals (recolor/re-outfit variants). |
| [POLYGON Apocalypse Wasteland](https://syntystore.com/products/polygon-apocalypse-wasteland) | Rural/outskirts biome variety | Fits the new Adirondacks-region setting well — mountain/forest fringe, not just urban decay. |
| POLYGON City / Town packs (Synty store) | Extra civilian building/prop variety | Optional, only if Apocalypse's coverage feels thin after the P7 blockout. |
| [Zombie Starter / Basic / Pro — MoCap Online](https://mocaponline.com/products/zombie) ([Fab listing](https://www.fab.com/listings/c4ed6ca8-f8b1-438c-98cb-66f8a4783b91)) | 26 / 119 / 265 pro-mocap zombie animations (walks, chases, attacks, deaths, crawls) | Start with **Starter or Basic**; Pro only if zombie variety becomes a focus. Mixamo (below) may be enough for v1 — defer this purchase until P4 shows the gap. |

### Free / CC0
| Source | What it covers |
|---|---|
| **[Game Animation Sample](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016)** (Epic, free) | 500+ AAA locomotion animations for the UE5 mannequin skeleton — the new TP locomotion base (walk/run/crouch/jump). Use its animations with a simple state machine first; its motion-matching setup is optional depth later. |
| **[Mixamo](https://www.mixamo.com)** (free, Adobe account) | The classic zombie animation set (walk/attack/scream/death) + human fillers (also usable for hostile roamers); auto-rigs too. v1 zombie animation plan. |
| [Kenney](https://kenney.nl/assets/survival-kit) (CC0) | Survival Kit (80 modular survival props), [modular characters w/ 17 anims + accessories](https://www.kaylousberg.com/work/kenney-character-assets), blaster/prop kits |
| [Quaternius](https://quaternius.com/) (CC0) | Thousands of low-poly models incl. rigged/animated characters, buildings, props — worth checking specifically for forest/mountain/winter sets given the setting change |
| [Poly Pizza](https://poly.pizza/bundles) (CC0 aggregator) | Search engine over Kenney/Quaternius/Google Poly-era packs, FBX/GLTF |
| [itch.io low-poly + post-apocalyptic tags](https://itch.io/game-assets/tag-low-poly/tag-post-apocalyptic), [OpenGameArt CC0](https://opengameart.org/content/cc0-assets-3d-low-poly) | Gap-filling; check licenses per pack on itch |
| Audio: Sonniss GDC bundles (free GB of pro SFX), freesound.org (CC0 filter), Kenney audio packs | Gunshots/zombies/ambience for P10; a paid horror-ambience pack can wait |

### Blender pipeline (the fill-the-gaps plan)
Blender 4.x LTS, free. The workflow that matches Synty-style art:

1. **Model on-grid:** modular kit pieces authored to a strict grid (1 m / 0.5 m increments, matching UE's grid) with pivots at floor-corner — this is what makes "modular" actually snap in-editor.
2. **Texture with a gradient/flat-color atlas:** one shared 256–1024 px palette texture for the *whole game*; UV islands are just dropped onto color blocks. No baking, no per-asset materials, automatic style consistency with Synty (theirs works the same way), and it keeps draw calls trivial.
3. **Match the anchor's proportions** (import a Synty building into Blender as reference scale before modeling anything).

**Addons — essential (all free):**
- **[Send to Unreal](https://epicgames.github.io/BlenderTools/send2ue/)** (Epic official) — one-click Blender→open-UE-project push, correct scale/axes/LODs, batch animation export.
- **[UE to Rigify](https://addons.cgdive.com/tools/ue2rigify)** (same Epic [BlenderTools repo](https://github.com/EpicGames/BlenderTools)) — full Rigify control rig over UE mannequin skeletons; the path for authoring/adjusting any custom character animation that stays retarget-compatible.
- **Game Rig Tools** — lightweight deform-rig generation for game export where full Rigify is overkill.
- **TexTools** — UV layout/align tools that make atlas-palette UV work fast.
- **Machin3tools** — general modeling QoL (mirror, align, focus) that speeds low-poly work disproportionately.

**Addons — worth paying for only if Blender becomes a main lane:** UVPackmaster (best-in-class packing), Hard Ops/Boxcutter (hard-surface speed; overkill for flat-shaded low-poly), Auto-Rig Pro (alternative rig+UE export path if UE to Rigify frustrates).

**Skeleton rule (important, learned the hard way with `SKEL_TFA_Mannequin`):** everything humanoid in the new art pipeline targets the **UE5 mannequin skeleton** (or a Synty rig retarget-mapped to it once, via IK Retargeter). One skeleton family, one retarget hub — never again a system built against a pack-specific skeleton.

---

## 6. Scope guardrails & risks

| Risk | Mitigation |
|---|---|
| **Pivot whiplash** — rebuilding presentation while systems half-exist | P0 verifies + commits a known-good baseline first; cuts are surgical and each compile/PIE-gated. |
| **Top-down doesn't feel right** | P1 is a cheap identity gate *before* art spend; over-shoulder TP is the pre-agreed fallback (Decision 1). |
| **Simulation creep** (PZ gravity: every system invites 3 more) | §3's table is a contract; "deferred pool" items need a planning pass to enter scope, same rule `CLAUDE.md` already uses for missions/economy. |
| **Zombie counts vs. performance** | Low-poly + flat materials is half the answer; P4 sets a profiled budget (target: 60fps with ~150 active on-screen zombies on mid hardware, tune from there — confirm this number, see §7); crowd anim tricks (anim sharing/URO) if needed. |
| **MP save/persistence complexity** | Listen-server-host-owns-the-save (PZ's own co-op model), single world save, no per-client saves. |
| **The investigation arc vs. infinite sandbox tension** | This is PZ's own unresolved #1 design fork (per its reference doc's §22), and it's exactly what this project is now attempting to answer. Getting Decision 6 right matters more than any single system — it determines whether "completed the investigation" is per-character save state, per-world save state, or purely cosmetic, and that determination has to happen before P8's back-end is built, not after. |
| **NPCs are a siren song** | Full factions/dialogue/allied-NPC systems stay hard-gated behind a post-v1 planning pass. Only the narrow, cheap hostile-roamer variant (Decision 5) is in v1 scope. |
| **Solo-dev art volume** | Buy the core (Decision 3), one region not a county-sized map, prop variety via palette recolors. |
| **Animation scope re-creep** | The §2 rule: readable-at-camera-distance or gameplay-gating, else polish-phase. |

**Rough shape of the calendar** (solo dev + Claude sessions, part-time cadence like the last two weeks — adjust freely): P0–P1 ≈ 2–3 weeks · P2–P3 ≈ 3–4 weeks · P4–P5 ≈ 4–6 weeks · P6 ≈ 2–3 weeks · P7 ≈ 4–6 weeks (art-heavy) · P8 ≈ 3–4 weeks · P9–P10 ≈ 4–6 weeks. **Order-of-magnitude: a tuned co-op vertical slice in ~6–9 months.** Estimates are for pacing honesty, not commitments.

---

## 7. Open questions by development stage

The dev asked to "come up with questions for each stage of development" — this is that list. Not every question needs an answer before its phase starts; some are fine to resolve mid-phase through play-testing. The ones marked **(blocking)** genuinely shape the phase's architecture and are worth settling first.

### Cross-cutting / foundational
1. **This document's filename/title** — the dev's own markup left "rename GameDevPlan to: ___" blank. Real working title for the *document*, distinct from the game's eventual name?
2. Is a real marketing/working title for the *game itself* wanted now, or is that a later-phase problem (current lean: later)?
3. **Platform commitment (blocking for P1):** PC/Steam-only for v1 with console as an explicit-but-later goal, confirmed? If so, do we design UI navigation (menus, radial wheel, inventory) gamepad-first from the start, or keyboard/mouse-first with a gamepad pass retrofitted before P10?
4. Team-size reality check: should phase-length estimates in §4/§6 assume solo + AI-assisted sessions throughout, or could an artist/contractor/composer join at some point (changes the P7/P10 estimates significantly)?
5. Save architecture: one world/save per hosted server (matches PZ, matches the plan as written), or do we also want multiple concurrent save slots per install (host manages several distinct worlds)?

### P0 — Close-out / re-aim
1. Is the current Infima-based FP animation investment a total write-off once shelved, or is there a version of it (e.g., over-shoulder ADS) still worth keeping alive as a fallback rather than fully dormant?
2. Any specific content/code the dev wants explicitly untouched during the de-scope pass (e.g., don't touch the weapon config data assets yet, even the ones tied to cut features)?

### P1 — Camera & control prototype
1. Controller or KBM as the *primary* tuning target for aim-assist curves and sensitivity defaults?
2. Prototype the "scattered profession spawn" co-op flow here (cheap, camera-adjacent) or wait for the real map in P7?
3. Interior visibility for top-down — roof fade, hard cutaway, or a camera-ducks-inside solution? Worth a quick spike here even though the real answer lands in P7.

### P2 — Survival core (needs/moodles/skills/time)
1. **(blocking, per Notes §6)** Final skill list — is the working ~8-skill cut right, or does the dev want a different split (e.g., merge Firearms+Melee into one Combat skill; fewer/more crafting sub-skills)?
2. Can a solo player sleep-skip time alone (no group check needed), with the "everyone must be ready" rule only applying in co-op?
3. Is there a floor under Hunger/Thirst debuffs (never fully incapacitating), or should sustained, total neglect still be a real death path, just a much slower one than PZ's default?
4. Weather (Notes §1: "players must adjust to survive") — real temperature/insulation mechanics in v1, or atmospheric/visibility-only for now with survival-temperature systems deferred to the pool?

### P3 — Health, damage, medical, amputation
1. Does amputation require a specific tool (hatchet/saw/machete), and is it solo-capable or does it need another player's help in co-op?
2. Is there a timing window (must amputate within X minutes of infection) or is any time before the infection timer completes valid?
3. Post-amputation: permanent capability loss only for v1, or is a later prosthetic/adaptation crafting chain worth flagging now so the data model doesn't need reworking to support it?
4. Do hostile human roamers use the same wound/infection system as zombies (bleeding/fractures, no Knox infection), or are their attacks mechanically simpler (flat damage only)?

### P4 — Zombies & hostile roamers
1. Is ~150 concurrent on-screen zombies (this plan's placeholder performance target) the right ballpark, or does the dev have a different number in mind?
2. Hostile roamer spawn logic — pure ambient/random encounters, or tied to specific setpieces (e.g., a raider camp location type)?
3. Do hostile roamers drop loot (weapons/ammo), creating a deliberate risk/reward incentive to fight them?

### P5 — Combat completion
1. Melee weapon variety for v1 — a curated set (4–6 archetypes: blunt/edged/axe/improvised) or closer to PZ's full breadth from the start?
2. Durability: break-only with no repair (matches PZ's scarcity pressure) or is a basic repair mechanic wanted sooner than PZ shipped one?

### P6 — Inventory & loot
1. Bag/equip-slot depth — flat capacity plus a couple of bag-upgrade tiers, or PZ's fuller bags-in-bags nesting?
2. Is the finite-rarity-pool model per-world-instance (each server tracks its own scarcity) or a fixed design target validated per building type (simpler, fully table-driven)?

### P7 — World & persistence
1. Naming pass — does the dev want to brainstorm the fictional county/town names together, or already have names in mind?
2. Map scale — is ~1×1 km still right, or does having multiple profession spawn points (Decision 4) argue for something larger so starts feel meaningfully separated?
3. Confirm Decision 4's "scatter spawns" toggle as a lobby-level co-op setting, on by default or off by default?

### P8 — Dynamic events, objectives & investigation arc
1. **(blocking) Decision 6** — does completing the investigation/cure arc end or meaningfully alter the save, or is it a pure capstone/epilogue with the sandbox continuing unchanged?
2. Should the investigation arc's clues/locations be roughly fixed per playthrough (like a authored questline), or shuffle per world seed for replayability?
3. How many distinct meta-events (helicopter-class beats) are wanted at launch — a handful (3–5) tuned deeply, or a broader roster from day one given Notes §17's "create more meta events"?

### P9 — Onboarding & meta-loop
1. Any lightweight build-variety system wanted at character creation beyond background + spawn point, now that full trait point-buy is deferred (e.g., a small number of simple perk picks)?
2. Keep the "how you died" death-recap screen as a fun PZ callback even though the underlying philosophy line was removed, or should that framing go too?

### P10 — Hardening & vertical slice
1. What's the target audience for the public vertical slice — a Steam Early Access launch candidate, a demo/playtest build, or a portfolio/pitch piece? Changes what "done" means for P10.
2. Any external date (convention, publisher pitch, community playtest) that should anchor or compress the ~6–9 month estimate?

---

## 8. Immediate next steps (first session after the dev reviews this)

1. Dev resolves the ⚑ decisions (1–6) and the §7 questions marked **(blocking)** at minimum — everything else in §7 can be answered as its phase approaches.
2. Update `CLAUDE.md` (identity section, dev-order table → this doc, animation-scope rule) and `SessionHandoff.md`.
3. Run P0 step 1: the already-pending Phase 3 M7 two-client PIE verification (checklist in `CoreLoopPlan.md`).
4. Begin the P0 de-scope pass.
5. (Parallel, dev-paced) Wishlist/watch the Synty packs on Fab; grab the free Game Animation Sample and Kenney/Quaternius kits so P1's graybox has stand-ins; start a naming brainstorm for the Adirondacks-region setting (P7 §7 question 1) whenever it's fun to think about, no rush.
