# Survival Pivot — Full Game Development Plan (v0.1, draft for dev review)

> **Status: DRAFT — written 2026-07-18 for the dev to review and adjust. Not yet the plan of record.**
> Once reviewed/adjusted, this supersedes `CoreLoopPlan.md`'s Phase 4–6 scope (Phases 0–3 stand as built), and `CLAUDE.md` + `SessionHandoff.md` get updated to point here.
>
> Companion document: [`ProjectZomboid_DesignReference.md`](ProjectZomboid_DesignReference.md) (the PZ systems breakdown this plan works against — section references like "PZ §5" point into it).
>
> Items marked **⚑ DECISION** are calls the dev should confirm or overturn. Each has a recommendation and the plan proceeds on that recommendation, so overturning one only reroutes that item, not the whole plan.

---

## 1. What the game is

**One-liner:** A low-poly, co-op-first zombie survival game — Project Zomboid's simulation soul (needs, noise, attrition, permadeath) with a modern 3D camera, real aiming, readable systems, and a dynamic mission/event layer PZ never shipped.

**Why this can exist next to PZ** (from the reference doc's own gap analysis, PZ §22):
1. PZ has no missions/objectives layer and an empty late game — we build a dynamic event/objective layer as a core system, not a bolt-on.
2. PZ's multiplayer history is droughts and instability — this project is replication-first *already* (Phase 3 is built).
3. PZ's onboarding is a cliff and its UI is its most-modded surface — we ship fewer, more readable systems with modern interaction (prompts, radials, clear feedback).
4. PZ is 12 years of hand-built map density — we compete honestly: **small dense map, low-poly modular art, systems + events over content mass.**

### Design pillars (steal / rethink, per PZ §23)

**Kept from PZ (identity-defining):**
- Readable simulation stack — a small **moodle-style status system** (PZ §5, simplified — see §3 below).
- **Noise as threat currency** — every loud action pulls zombies (PZ §7.3). This is the system that makes guns a real decision, and it's where our already-built shooting mechanics stop being "just a shooter."
- **Stamina-economy melee** — swinging is a resource decision, not a reflex test (PZ §7.1).
- **Infection ambiguity window** — "is this a cold or am I dead?" (PZ §8.1, §9).
- **Permadeath + persistent world** — new character, same looted world (PZ §18).
- Phase-transition world state — a utilities-shutoff-style clock that changes the game's rules partway in (PZ §10.3).
- Environmental storytelling + diegetic media (radio broadcasts) as tutorial and narrative (PZ §16).

**Deliberately different from PZ (our identity):**
- **3D fixed-angle top-down camera with direct WASD + mouse-aim control** — not PZ's menu-driven isometric interaction. Combat feels like a twin-stick/tactical shooter; survival feels like PZ.
- **Co-op-first (2–4 players, listen server)** — every system built replicated from day one, which is already this codebase's DNA.
- **Dynamic events & radiant objectives** — helicopter-class events (PZ §17) as a *system*, plus radio-driven objectives ("military drop at the church, 20 min") that give sessions shape. Light NPC presence later, gated behind its own planning pass.
- **Simplified simulation** — every PZ system ships here at roughly **1/3 of PZ's depth**, chosen for readability. Depth can grow later; opacity is not a feature we inherit.
- **Modern UX** — direct interaction prompts, radial quick-menu, no wiki-required mechanics.

**⚑ DECISION 1 — Camera.** Recommended: **fixed-angle top-down** (pitch ~55°, zoom 8–20 m, yaw rotation in 45° steps), matching the reference doc's isometric thesis. The existing `ApplyCameraPerspective` architecture makes this a *new perspective entry*, not a rewrite — and we cut the perspective list from 4 (FP/TP/GunCamera/Bodycam) down to **TopDown + OverShoulder** (over-shoulder kept as an aim-zoom and as a hedge: if top-down doesn't feel right in the graybox phase, over-shoulder third-person survival is the fallback identity, and we lose nothing). First-person, GunCamera, Bodycam: shelved (code kept in git history, perspectives removed from the cycle).

**Working title:** stays "ZombieShooter" as repo/project name (per `CLAUDE.md`, it's a placeholder). Real name is a marketing-pass problem, not a now problem.

---

## 2. What happens to the current project

The pivot **keeps the repo, the project, and the C++ core.** What we've built is mostly camera-agnostic infrastructure; what changes is the presentation layer and where future effort goes.

### Carries over as-is (the pivot's foundation)
| System | Why it survives |
|---|---|
| `AZSGameMode/GameState/PlayerState/PlayerController` + Enhanced Input | Camera-agnostic framework. Input actions get remapped, not rebuilt. |
| **Phase 3 replication layer** (server RPCs, `OnRep_` convention, cross-client fixes) | The single most valuable asset for a co-op-first game. Finish M7 verification before pivoting — see Phase P0. |
| `UZSWeaponConfig` data-driven weapon architecture + "N weapons, zero C++ branches" rule | Extends beyond guns: the same pattern becomes `UZSItemConfig`, `UZSZombieConfig`, etc. |
| Real ammo/magazine state on `AZSWeapon` | PZ-style scarcity needs real ammo. Already built, already replicated. |
| Notify architecture (`AN_ZS_UnlockActions`, `ANS_ZS_BlockADS`, montage-driven action flow, `bIsBusy` + fallback) | Generalizes to every timed action this game will ever have: melee swings, bandaging, barricading, crafting. This is *not* wasted animation work — it's the action system. |
| `BlueprintNativeEvent` policy, replication convention, naming rules, docs discipline, MCP workflow | Unchanged. |

### Simplified or repurposed
| Current | Disposition |
|---|---|
| 4-perspective camera system | Cut to TopDown + OverShoulder (Decision 1). `ToggleCameraPerspective` stays as the toggle between the two. |
| FP arms mesh/AnimBP (`ABP_ZS_FirstPerson`, FABRIK, additive stacks, camera/head toggle) | **Shelved, not deleted** — disconnect from the spawn path. If Decision 1 flips to over-shoulder-with-ADS someday, it's in git history. No further investment. |
| TP AnimBP (`ABP_ZS_ThirdPerson`) | Becomes the *only* character view. Gets rebuilt against the new art skeleton (see §5) using Epic's free Game Animation Sample locomotion — replacing the Infima-skeleton-specific graph. |
| Infima pack | **Demoted from "reference of record" to "prototype placeholder."** Keep using its weapons/animations in graybox until low-poly art lands, then it exits the runtime entirely (it stays gitignored either way). What we permanently keep from Infima is what we already extracted: the config-driven weapon architecture, notify concepts, and real-reload flow. |
| `Docs/Infima Pack - Official Implementation Guide/` | Kept for history; no longer drives new work. |

### Cut outright (the animation de-scope the dev asked for)
- **`Inspect`, `MagCheck`, `CycleGripAttachment`** — actions, input bindings, montage wiring. (The montages/notify classes they exercised stay; they're generic.)
- **`AZSLaserAttachment`**, grip-attachment randomization/variants.
- **Weapon-owned cosmetic notifies + `ABP_Weapon`/`ABP_Magazine`** (`AN_ZS_DropMagazine`, `AN_ZS_EjectCasing`, `ANS_ZS_HideMainMag`, `ANS_ZS_ShowReserveMag`) — already deferred in M9; now formally cut. At top-down camera distance nobody sees a magazine hide/show swap.
- **`AZSPhysicsCasing`/`AZSPhysicsMagazine`/`AZSPhysicsObject`** cosmetic ejects — cut from the runtime path (class files can stay until they're in the way).
- `FP_ReloadEmpty`/`TP_ReloadEmpty` variants, gun-camera/bodycam content, procedural ADS/recoil/crouch **spring-offset system** (top-down recoil is a crosshair/spread concern, not a skeletal-pose concern).

**Rule going forward:** an animation earns its place only if it's *readable at gameplay camera distance* or *gates gameplay timing* (reload lockout, swing timing, bandage channel). Everything else is polish-phase-only.

**⚑ DECISION 2 — Same repo vs. fresh project.** Recommended: **same repo, same UE project.** The C++ core is the pivot's head start; a fresh project throws away working replicated systems to avoid deleting some content folders. Cleanup is Phase P0. (If the dev prefers a clean break: fresh repo, migrate `Source/`, and the plan below is unchanged from Phase P1 on.)

---

## 3. PZ systems disposition (KEEP / SIMPLIFY / REPLACE / CUT)

The reference doc asks for exactly this markup. This is the scope contract — anything marked CUT stays cut until a dedicated planning pass revives it.

| PZ system (ref §) | Disposition | Our version |
|---|---|---|
| Isometric camera, menu-driven interaction (§3) | **REPLACE** | 3D top-down, direct control, world interaction prompts + radial quick-menu. |
| Professions/occupations (§4.1) | **SIMPLIFY** | 5–7 starting "backgrounds" = stat template + 1 unique unlock each. Data-asset-driven (`DA_ZS_Background_*`). |
| Trait point-buy (§4.2) | **SIMPLIFY (later)** | ~10 positive / ~10 negative traits, points economy kept (it's beloved and cheap to build once stats exist). Not in the first playable. |
| Moodles / needs (§5) | **KEEP, simplified** | **6 moodles v1:** Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness. Panic/Stress/Boredom/Temperature: deferred pool. Same 4-severity-tier readable iconography. |
| Nutrition micro-sim (calories/protein/fat) (§5) | **CUT** | Food restores Hunger; quality = bigger/longer restore. |
| Skills, learn-by-doing, books (§6) | **SIMPLIFY** | ~8 skills (Melee, Firearms, Sprinting/Fitness, Carpentry, Medicine, Cooking, Scavenging, Mechanics-lite). Learn-by-doing + magazine-style one-shot unlocks. No Vol 1–5 book grind. |
| Melee combat, stamina economy (§7.1) | **KEEP** | Core feel target. Shove + swing + stomp, stamina-gated, weapon durability-lite (break, no repair micro-sim). |
| Firearms as loud/scarce power (§7.2) | **KEEP** | Already built mechanically; noise system makes it PZ-honest. |
| Stealth/noise/vision model (§7.3) | **KEEP, simplified** | Crouch = quieter + slower; AI hearing radii per action; line-of-sight vision cones. No lightfootedness skill web in v1. |
| Zombie lore + hordes + migration (§8) | **KEEP, simplified** | Shamblers, hearing/sight, door-banging, crawlers (later), zone population + wander. Full sandbox lore sliders (§8.3): CUT for v1, revisit as a cheap post-launch win. |
| Per-body-part health, 17 zones (§9) | **SIMPLIFY** | **4 zones:** Head, Torso, Arms, Legs. Wound types: scratch/bite/laceration/fracture. Bandage cleanliness + the two-tier infection dread: KEPT (it's the horror engine). |
| Hand-built mega-map (§10.1) | **REPLACE** | One small dense town + rural fringe (~1×1 km playable v1), modular-kit built, World Partition. |
| Container/zone loot tables (§10.2) | **KEEP** | Data-asset loot tables per building/container archetype. Annotated-map stash hints: KEEP (cheap, brilliant). |
| World persistence + erosion (§10.3) | **SIMPLIFY** | Persistence: yes (saves are Phase P7). Erosion visuals: CUT v1. |
| Utilities shutoff phase transition (§10.3) | **KEEP** | Power/water die on a randomized day — our mid-game bell too. |
| Seasons/weather/temperature (§11) | **SIMPLIFY** | Day/night + rain/fog v1. Full seasonal calendar + temperature survival: deferred pool. |
| Weight encumbrance + bags (§12) | **KEEP, simplified** | Weight + bag slots. No bags-in-bags recursion, no clothing-layer insulation matrix. |
| Clothing protection layers (§12) | **SIMPLIFY** | Single outfit slot-set with protection values. No layering sim. |
| Carpentry/base-building (§13.1) | **SIMPLIFY** | v1: barricades, door/window reinforcement, crates, rain collector. Freeform wall-building: deferred (big system, PZ's answer to late-game — ours is events). |
| Metalworking/electrical/plumbing chains (§13.2) | **CUT v1** | Generator-as-item (fuel logistics) without the wiring sim. |
| B42-style deep crafting web (§13.3) | **CUT** | Explicitly the direction we *don't* chase — events/missions are our late game. |
| Farming/foraging/fishing/trapping (§14) | **SIMPLIFY** | v1: farming-lite (plant/water/harvest) + foraging zones. Fishing/trapping: deferred pool. |
| Vehicles (§15) | **CUT v1, plan later** | Big, physics-fiddly, and PZ+HumanitZ's shared weak spot. Deferred to its own planning pass post-v1. Map is sized to be walkable. |
| TV/radio diegetic media (§16) | **KEEP, repurposed** | The radio is our tutorial *and* our mission-giver — broadcast schedule teaches systems early, then becomes the dynamic-objective channel. |
| Meta events (helicopter etc.) (§17) | **KEEP, expanded** | This is our headline system — see Phase P8. |
| Modes/challenge presets (§18) | **CUT v1** | One tuned default. Sandbox sliders post-v1. |
| MP: dedicated servers, big counts (§19) | **SIMPLIFY** | 2–4 player listen-server co-op. Dedicated/Steam sockets: own planning pass later (per existing `CLAUDE.md` rule). |
| Modding/Lua (§20) | **CUT v1** | Data-asset-driven design keeps the door open; actual mod support is post-launch. |
| NPCs/factions (§22.1) | **DEFER** | The biggest differentiator is also the biggest risk. v1 ships *events without humans*; NPC survivors get a dedicated planning pass only after the event system proves out. |

---

## 4. Development phases

Same working style as `CoreLoopPlan.md`: numbered phases, milestone tables, PIE-verified exit criteria, commit per sub-task, docs updated at phase end. Phases are ordered so **every phase ends in something playable** and the risky/identity-defining systems come early.

### P0 — Close out, clean up, re-aim (the simplification pass)
1. **Finish Phase 3 M7** (2-client PIE verification of the existing replication layer, dev-driven, checklist already in `CoreLoopPlan.md`). We verify what's built *before* surgery, so post-surgery breakage has a known-good baseline.
2. Commit the currently-uncommitted session-8 work (already compiled clean).
3. **De-scope pass** per §2's cut list: remove Inspect/MagCheck/SwitchGrip input actions + bindings; strip the actions and laser/grip/physics-cosmetic paths from `AZSPlayerCharacter`/`AZSWeapon`; retire the FP spawn path (keep `FirstPersonMesh` component dormant or remove — smallest safe diff wins); reduce perspective enum. Compile + PIE after each removal cluster.
4. Update `CLAUDE.md`, `SessionHandoff.md`; this doc becomes plan of record.
   **Exit:** clean build, 2-client PIE still passes fire/reload/aim/sprint/crouch with the slimmed action set.

### P1 — Camera & control prototype (identity test #1)
- TopDown perspective in `ApplyCameraPerspective` (pitch/zoom/step-rotation), cursor-projected aim (screen ray → ground plane → character aims at point), movement relative to camera, OverShoulder aim-zoom toggle.
- Interaction system v1: `UZSInteractableComponent` + world prompt ("E — Open").
- Graybox test map. Infima rifle still the stand-in weapon.
  **Exit:** moving/aiming/shooting *feels good* at top-down distance, 2-client PIE. **This is the go/no-go gate on Decision 1 — settle the camera before any art money is spent.**

### P2 — Survival simulation core (identity test #2)
- `UZSNeedsComponent` (ActorComponent, replicated, data-asset-tuned): Hunger/Thirst/Fatigue/Stamina + rate curves; consequences (empty hunger → weakness debuff → HP drain; stamina gates sprint/melee).
- World clock (`AZSGameState`): day/night, configurable compression, sleep-to-skip (co-op vote), the utilities-shutoff timer.
- Moodle UI stack (UMG, 4 severity tiers) + first-pass HUD.
- Items exist minimally: eat/drink consumables via a first `UZSItemConfig`.
  **Exit:** a character can starve to death in PIE with legible moodle escalation, replicated.

### P3 — Health, damage & medical-lite
- `UZSHealthComponent` (the old Phase 4, rebuilt to the new scope): 4 zones, wound records (scratch/bite/laceration/fracture), bleed-over-time, all damage through `TakeDamage`.
- Treatment actions (montage + notify-gated, reusing the action system): bandage (cleanliness flag), disinfect, splint.
- **Knox-style infection:** bite → hidden infection roll → delayed queasy→fever→death arc; deliberately UI-ambiguous vs. ordinary sickness.
- Player death → spectate/respawn-as-new-character flow (permadeath groundwork).
  **Exit:** a scripted damage source can wound, infect, and kill a player who mismanages treatment; second client sees everything correctly.

### P4 — Zombies (the enemy, finally)
- `AZombieCharacter` + `AZombieAIController`, classic **Behavior Tree + Blackboard** (per the standing `CLAUDE.md` decision), `UZSZombieConfig` data asset (speed/health/senses/damage — N zombie types, zero C++ branches, same rule as weapons).
- Perception: `AIPerception` sight cone + hearing. **`UZSNoiseSystem`:** every loud act (gunshot, sprint, breaking glass) reports a noise event with a radius; this is the load-bearing system of the whole game.
- Behaviors v1: wander, investigate noise, chase, attack (melee hit → P3 damage/infection), door-thumping (destructible door HP).
- Spawning: zone-based population from data, server-authoritative, respawn-into-cleared-zones on a slow timer.
- Placeholder visuals: Mixamo/UE-mannequin zombie + Mixamo zombie animations (free) until the art phase.
  **Exit:** a graybox block with 30+ zombies; a gunshot visibly drags the neighborhood onto the shooter; 2-client PIE holds up.

### P5 — Combat completion (melee + the full loop feel)
- **Melee weapon type** through the *same* `UZSWeaponConfig` pipeline (a melee config specifies swing montages, reach, stamina cost, damage, durability — the multi-weapon rule pays off here).
- Swing timing via the existing notify system; shove + stomp as always-available options; stamina economy tuned against P2.
- Firearms integration with noise + zombie mass: ammo scarcity tuning, simple hit-reaction/knockdown.
- Weapon durability-lite (melee breaks; no repair sim v1).
  **Exit:** the PZ death loop exists — greed + noise + stamina mismanagement kills a player who had every tool to survive.

### P6 — Inventory, loot & scavenging
- `UZSInventoryComponent` (replicated), weight-based encumbrance, bag equip slots; `UZSItemConfig` grows into the general item contract (food/med/ammo/material/tool).
- Container actors + data-asset **loot tables** keyed by building/container archetype; timed transfer actions (looting under pressure = tension, per PZ §12).
- Inventory UI (list-based dual-pane, bulk actions, favorite/junk — beat PZ's UI, don't copy it) + radial quick-use.
- Dropped-item persistence in the running session.
  **Exit:** full scavenge loop in graybox: run out, loot under threat, haul back, stash.

### P7 — World building & persistence
- **Art integration phase** (see §5): replace graybox with the chosen modular kit; build the town (World Partition, ~1×1 km v1): residential streets, main-street commercial row (hardware/pharmacy/gun store/grocery — the loot-archetype anchors), gas station, church, farm fringe, woods.
- Enterable buildings as the rule; interior visibility solution for top-down (roof fade/cutaway — prototype early in P1 if it worries us).
- **Save/persistence v1** (single "world continues" save per server): world item/container/door state, character sheets, clock, zombie population coarse state. Host-side SaveGame; permadeath = character deleted, world persists.
- Utilities shutoff goes live against the real map (powered lights/fridges/pumps flip off).
  **Exit:** the real map plays end-to-end co-op; quit → relaunch → world remembered; day ~10 the lights die.

### P8 — Dynamic events & objectives (the differentiator)
- `UZSEventDirector` (server): scheduled + random world events from data assets — helicopter flyover (drags hordes), distant gunshots/screams (ambient migration), crashed convoy/supply drop (timed loot beacon = risk/reward), house alarms.
- **Radio channel:** scripted broadcast arc for days 1–7 (diegetic tutorial, PZ §16's trick) that transitions into dynamic event/objective announcements.
- Radiant objective wrappers ("reach the drop before it's swarmed," "restore the station generator") — objectives are *invitations with stakes*, never mandatory quests. No NPCs yet.
  **Exit:** two co-op sessions on the same map play out differently because the director dealt different beats.

### P9 — Meta-loop, onboarding & difficulty
- Character creation v1: backgrounds (§3) + appearance from the art kit's modular characters; trait point-buy if budget allows, else deferred.
- Death → new character → same world flow polished (find your old corpse — keep PZ's beloved beat).
- First-hour experience pass: radio-guided first days, interaction hints, a "how you died" death recap card.
- Skill XP hookup (learn-by-doing across P2–P6 systems).
  **Exit:** a stranger survives their first 30 minutes without a wiki and dies to something they understand.

### P10 — Production hardening → public vertical slice
- Audio pass (see §5 asset list), VFX pass (low-poly-friendly: flat-shaded blood/muzzle/impact), performance (zombie count profiling, LODs/HISM on the kit, net relevancy), fixed-tick save safety, crash/soak testing, packaged Windows build tested over real LAN/direct-IP.
- Trailer-able vertical slice: 20–40 minutes of tuned co-op survival on the town map.
  **Exit:** shippable demo build. Post-v1 planning pass picks from: NPCs/factions, vehicles, sandbox sliders, seasons, Steam/EOS + dedicated server, deferred-pool systems.

**Standing rules across all phases** (inherited, still binding): replication convention on every new stat/system; data-asset-driven everything (`N` of a thing, zero C++ branches); `BlueprintNativeEvent` for gameplay decisions; no magic numbers (`TuningReference.md` stays live); commit per sub-task; docs updated at phase end.

---

## 5. Asset strategy

### The style decision
**⚑ DECISION 3 — Art source.** Recommended: **hybrid — buy the Synty POLYGON core, fill gaps in Blender to match it.** Buying the core saves months; Blender covers the game-specific 20% (our own props, kit pieces Synty lacks, hero items). All alternatives below if the dev prefers pure-free or pure-handmade.

Whatever is chosen: **pick one style anchor and make everything else conform to it** (proportions, texel-less flat/gradient-atlas texturing, palette). Mixed low-poly styles read worse than consistent mediocre ones.

> **Licensing/repo rule (existing `CLAUDE.md` pattern, applies to all of these):** paid marketplace content is **gitignored, never committed** to the public repo — same as `Content/InfimaGames/`. CC0 content may be committed.

### Paid core (Fab / Synty Store — watch for Humble Synty bundles, they recur and are drastically cheaper)
| Pack | What it covers | Notes |
|---|---|---|
| **[POLYGON Apocalypse](https://syntystore.com/products/polygon-apocalypse-pack)** | The bulk of the game: 1,800+ prefabs, **modular buildings with enterable interiors**, modular bunker/quarantine walls, **modular gun system** (build our weapon variety from parts), 86 complete weapons incl. melee | The single highest-value purchase; UE-native version on Fab. Its modular gun system slots straight into `UZSWeaponConfig`-per-weapon. |
| **[POLYGON City Zombies](https://syntystore.com/products/polygon-city-zombies-pack)** | 50 zombie characters w/ color variants | Rigged compatible with the Synty/UE-mannequin-style skeleton → retargets to the MoCap Online / Mixamo zombie sets. |
| [POLYGON Apocalypse Wasteland](https://syntystore.com/products/polygon-apocalypse-wasteland) | Rural/outskirts biome variety | Optional; nice for the farm-fringe ring of the map. |
| POLYGON City / Town packs (Synty store) | Extra civilian building/prop variety | Optional, only if Apocalypse's coverage feels thin after the P7 blockout. |
| [Zombie Starter / Basic / Pro — MoCap Online](https://mocaponline.com/products/zombie) ([Fab listing](https://www.fab.com/listings/c4ed6ca8-f8b1-438c-98cb-66f8a4783b91)) | 26 / 119 / 265 pro-mocap zombie animations (walks, chases, attacks, deaths, crawls) | Start with **Starter or Basic**; Pro only if zombie variety becomes a focus. Mixamo (below) may be enough for v1 — defer this purchase until P4 shows the gap. |

### Free / CC0
| Source | What it covers |
|---|---|
| **[Game Animation Sample](https://www.fab.com/listings/880e319a-a59e-4ed2-b268-b32dac7fa016)** (Epic, free) | 500+ AAA locomotion animations for the UE5 mannequin skeleton — the new TP locomotion base (walk/run/crouch/jump). Use its animations with a simple state machine first; its motion-matching setup is optional depth later. |
| **[Mixamo](https://www.mixamo.com)** (free, Adobe account) | The classic zombie animation set (walk/attack/scream/death) + human fillers; auto-rigs too. v1 zombie animation plan. |
| [Kenney](https://kenney.nl/assets/survival-kit) (CC0) | Survival Kit (80 modular survival props), [modular characters w/ 17 anims + accessories](https://www.kaylousberg.com/work/kenney-character-assets), blaster/prop kits |
| [Quaternius](https://quaternius.com/) (CC0) | Thousands of low-poly models incl. rigged/animated characters, buildings, props |
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
| **Zombie counts vs. performance** | Low-poly + flat materials is half the answer; P4 sets a profiled budget (target: 60fps with ~150 active on-screen zombies on mid hardware, tune from there); crowd anim tricks (anim sharing/URO) if needed. |
| **MP save/persistence complexity** | Listen-server-host-owns-the-save (PZ's own co-op model), single world save, no per-client saves. |
| **NPCs are a siren song** | Hard-gated behind post-v1 planning. Events ship first and are the differentiator on their own. |
| **Solo-dev art volume** | Buy the core (Decision 3), one town not a county, prop variety via palette recolors. |
| **Animation scope re-creep** | The §2 rule: readable-at-camera-distance or gameplay-gating, else polish-phase. |

**Rough shape of the calendar** (solo dev + Claude sessions, part-time cadence like the last two weeks — adjust freely): P0–P1 ≈ 2–3 weeks · P2–P3 ≈ 3–4 weeks · P4–P5 ≈ 4–6 weeks · P6 ≈ 2–3 weeks · P7 ≈ 4–6 weeks (art-heavy) · P8 ≈ 3–4 weeks · P9–P10 ≈ 4–6 weeks. **Order-of-magnitude: a tuned co-op vertical slice in ~6–9 months.** Estimates are for pacing honesty, not commitments.

---

## 7. Immediate next steps (first session after the dev reviews this)

1. Dev marks up the ⚑ decisions (1: camera, 2: repo, 3: art source) and any §3 table overrides.
2. Update `CLAUDE.md` (identity section, dev-order table → this doc, animation-scope rule) and `SessionHandoff.md`.
3. Run P0 step 1: the already-pending Phase 3 M7 two-client PIE verification (checklist in `CoreLoopPlan.md`).
4. Commit pending session-8 work; then begin the P0 de-scope pass.
5. (Parallel, dev-paced) Wishlist/watch the Synty packs on Fab; grab the free Game Animation Sample and Kenney/Quaternius kits so P1's graybox has stand-ins.
