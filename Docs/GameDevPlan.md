# Survival Pivot — Full Game Development Plan (v0.2, draft for dev review)

> **Status: DRAFT — v0.2, revised 2026-07-18 after the dev's own markup pass over the PZ reference doc.** Not yet the plan of record.
> Per the dev's own instruction on that markup pass ("fully replace CoreLoopPlan once GameDevPlan is finished"): once the open questions in **§7** are resolved and this plan is confirmed, it fully replaces `CoreLoopPlan.md` as the plan of record — not just Phases 4–6 as v0.1 hedged. `CoreLoopPlan.md` then becomes a historical build log for Phases 0–3 (referenced, not edited further); `CLAUDE.md`/`SessionHandoff.md` get repointed here.
>
> **This plan's own filename is a placeholder.** The dev's markup left "Rename GameDevPlan to: ___" blank — see §7's cross-cutting questions.
>
> Companion documents: [`ProjectZomboid_DesignReference.md`](ProjectZomboid_DesignReference.md) (the PZ systems breakdown, referenced as "PZ §N") and [`DevMarkupNotes.md`](DevMarkupNotes.md) (the dev's own section-by-section notes on that reference, referenced as "Notes §N" — this plan is the synthesis of the two).
>
> Items marked **⚑ DECISION** are calls the dev should confirm or overturn. Each has a recommendation and the plan proceeds on that recommendation, so overturning one only reroutes that item, not the whole plan. **§7 is a separate, larger battery of open questions per development stage** — read it before starting the phase it's attached to, not necessarily all at once.

> **⚑ PIVOT NOTICE, 2026-08-27 — extraction/mercenary rework.** §1 (pillars) and §3/§3.1 (scope contract, skill system) below have been rewritten around a new core premise: a hub-and-raid extraction loop replacing persistent-world survival. See §1 for the full framing. **§2 and §4 below still describe the earlier 2026-07-18 FP/TP→top-down-survival pivot and are kept as history, not rewritten for this pass** — they're not contradicted by the new premise (the C++ systems they describe still carry over into "what a raid feels like" essentially unchanged), they just haven't been re-examined line-by-line against it yet. `Docs/Beta/00_MasterPlan.md` §2 CR-13 is the production-plan side of this same pivot (phase restructuring, new phases for the hub/raid/human-hostile systems).

---

## 1. What the game is

**One-liner (rewritten 2026-08-27, extraction pivot):** A low-poly co-op zombie **extraction** game — a mercenary character repeatedly enters one large quarantined zone (city/suburbs/rural districts, one seamless map) from a persistent, safe **hub**, to loot, fight zombies and a hostile human faction, and complete vendor contracts for money, in-life skill XP, and loot; a raid ends by reaching an extraction point (loot banked to the hub) or by dying (the character, its skills, and everything it's carrying are gone for good). Project Zomboid's simulation soul (needs, noise, attrition, permadeath) survives, rescoped from "your whole persistent life" to "this one raid" — real aiming, a modern 3D camera, and a dynamic contract/mission layer PZ never shipped.

**Explicitly not PZ's thesis, in a different way than the original pivot framed it**: PZ's "this is how you died" is still rejected, but not via an open-world investigation arc to chase — via a **hub that persists the things that matter** (stash, vendor relationships, hideout upgrades, currency) even though every individual mercenary's life ends in permanent loss. The player's progress lives in the hub, not in any one character surviving; a character's death is a real, stinging setback, not a full account wipe. The investigation/cure-arc content ambition (Notes §1/§22) isn't gone — see the pivot's fold-in below.

**Why this shape, not the original persistent-open-world one:**
1. **Permadeath means something again.** PZ's "new character, same world" already meant a death cost the character's build, not the world — this project's own original pillar said the same thing. The extraction structure makes that loss legible and immediate (you watch your gear sit in the zone, you feel the empty stash) instead of an abstract "the world remembers your corpse somewhere out there."
2. **Sessions have a shape.** A raid has a beginning (choose a contract/loadout at the hub), a middle (loot, fight, avoid or clear threats), and an end (extract or die) — this solves the "empty late game, no missions layer" gap PZ has (Notes §22) far more directly than an always-on open world with occasional events layered on top.
3. **The already-built replication-first architecture (Phase 3) still pays off** — co-op raids are the same multiplayer problem this codebase was built around from day one.
4. **Reseeded raids let a modest map punch above its content budget** — one seamless, well-built zone with randomized loot/zombie-density/hostile placement per entry plays differently every time, without needing PZ's 12 years of hand-authored map density.

### Design pillars (steal / rethink, per PZ §23, revised against the dev's notes)

**Kept from PZ (identity-defining):**
- Readable simulation stack — a small **moodle-style status system** (PZ §5), but **rescoped to raid length, and narrowed, 2026-08-27 extraction pivot** (superseding the "6 moodles v1" persistent-attrition framing below and in §3): **Hunger, Thirst, Stamina** are raid-duration meters that start full on raid entry and drain across that one raid's session, still performance-debuffs-first, never death spirals from neglect alone (Notes §1's original intent, now applied to a raid instead of a whole persistent life). **Fatigue is cut** — a "haven't slept in days" mechanic has no home in a raid you enter fresh every time. **Wet/Temperature are cut from the core v1 moodle set** — the 4-input persistent-exposure simulation doesn't fit a short session; may return later as an occasional per-raid hazard-roll modifier (see the reworked utilities-shutoff hazard below), not an always-on system.
- **Noise as threat currency** — every loud action pulls zombies (PZ §7.3). Unaffected by the pivot.
- **Stamina-economy melee** — swinging is a resource decision, not a reflex test (PZ §7.1). Unaffected by the pivot.
- **Infection as a visible, urgent countdown, not a guessing game** — ⚑ **REVISED 2026-07-26** (plainly shown, not ambiguous — unaffected by the 2026-08-27 pivot), **reshaped into a raid/hub two-phase system 2026-08-27**: in-raid, items (bandage/splint/disinfect) still give real, mostly-immediate relief exactly as built — but anything not fully treated **keeps progressing on real/hub time between raids, it does not pause just because the raid ended**, so re-entering the zone with an untreated bite is a real bet. The hub can offer a paid full-heal vendor service. **Emergency amputation** as a fatal-infection escape valve, at the cost of permanent capability loss for that character's remaining life, is unchanged.
- **Permadeath, rescoped 2026-08-27 — no longer "same looted world," now hub-and-raid**: PZ §18's spirit (a death costs the character's build, not the shared world) is kept, but sharpened — death now costs the character's **skills, XP, and everything currently held/carried**, full stop, with a fresh mercenary starting from scratch at the hub. What persists is no longer "the same world to walk back into" but the **hub**: stash contents, vendor rep/unlocks, hideout upgrades, and currency, all untouched by any character's death. Loot a dead character was carrying is dropped and stays exactly where it fell — a *future* raid might find it, not the same character walking back.
- Phase-transition world state — **reworked 2026-08-27**: the persistent, one-way "utilities shutoff" ratchet (PZ §10.3) doesn't fit a zone that reseeds every raid entry. It becomes a **per-raid hazard roll** instead — each raid instance rolls its own utilities state (power/water sometimes already out, sometimes not) as part of the same reseed pass that randomizes loot/zombie/hostile placement, rather than either a permanent world-spanning ratchet or a plain no-variance day/night cycle.
- Environmental storytelling + diegetic media (radio broadcasts) as tutorial and narrative (PZ §16) — **now delivered through the hub's contract system** (see below) rather than open-world wandering discovery; the investigation/cure-arc content ambition folds into a narrative contract line from one specific hub vendor (recommended, not yet dev-confirmed — see the new Decision below).

**Explicitly cut/replaced from the "steal" list (v0.1 had these as kept-as-is; the dev's notes move them):**
- **Trait point-buy at character creation** — Notes §4.2: "instead of traits, certain actions just affect your stats differently," plus "need a skill points system, the more you do certain actions, the more proficient you get." Read together, this replaces PZ's creation-time positive/negative trait shopping list with **traits/aptitudes that emerge from play** (see §3's table and Decision 5). The classic point-buy list isn't gone forever — it's a natural "hardcore mode" sandbox option later, same shelf as other deferred PZ sandbox sliders.

**Deliberately different from PZ (our identity):**
- **3D top-down camera with direct WASD + mouse-aim control, framed like *Door Kickers 2*** (dev reference, 2026-07-19) — steeper/closer top-down than a classic 45° isometric, zoomable in tight enough to read character/room detail, not PZ's menu-driven interaction. **Important distinction from DK2 itself: this is a visual/camera reference only, not a gameplay one** — the player has full real-time direct control and normal movement at all times, no squad-command/pause-and-plan layer. Combat feels like a twin-stick/tactical shooter; survival feels like PZ. (Also more console-friendly than isometric-plus-right-click-menus — a stick-aim top-down camera translates to a gamepad far more directly, which helps Notes §3's console want land for free later.)
- **Co-op-first (4+ players, listen server — raised from the original 2–4 per OQ-X-03, 2026-07-26 rescope)** — every system built replicated from day one, which is already this codebase's DNA. Profession/background choice now also picks a **starting spawn location** (Notes §4.1) — see Decision 4.
- **Dynamic events, radiant objectives, and a discoverable investigation arc — reframed 2026-08-27 into the contract system below**, not open-world wandering discovery. Helicopter-class events (PZ §17) and radio-driven objectives still exist as in-raid flavor/pressure, but the questline built from notes/documents/items that lets players piece together the outbreak's origin and chase a cure (Notes §1/§22) is now delivered as a narrative contract line from a specific hub vendor — same content ambition, different delivery mechanism.
- **Hostile human roamers → promoted to a core, v1 system, 2026-08-27 (supersedes Decision 5's deferral below).** Always-hostile human NPCs (Notes §19: "never friendly") that fight zombies *and* players are no longer a post-v1 nice-to-have — heist-style contracts and "high risk zones with amazing loot" need guarded/contested loot to justify the risk, so this faction ships alongside zombies from the start. Still reuses the zombie AI/perception/noise pipeline (`AZombieCharacter`'s sibling pattern) as the original deferral argument assumed, just built sooner. Full NPC survivor/dialogue/reputation systems beyond "hostile faction to fight" stay deferred, unchanged.
- **The hub** — a persistent, safe space outside the zone: secure stash (survives character death), vendor NPCs (sell gear/ammo/consumables/abilities, buy loot, hand out contracts), hideout upgrade stations (replaces PZ's in-world carpentry/base-building — see §3), and loadout prep before entering a raid. This is genuinely new, no PZ or prior-pivot equivalent.
- **Vendor contracts as the mission layer** — scavenging, recon, heist-style high-risk/high-reward, and document/valuables-retrieval contracts, given and paid out at the hub (money + skill XP + sometimes item rewards), replacing PZ's absent late-game objective layer entirely rather than patching it with events alone.
- **Extraction** — a raid ends one of two ways: reach a defined extraction point (loot banked to the stash, character returns to the hub alive) or die (character + carried gear gone, dropped loot persists in the zone for a future raid). No PZ equivalent — this is the structural core of the new loop.
- **Raid reseeding** — every raid entry rerolls loot-container contents, zombie density/placement, human-hostile placement, and the utilities-hazard state, while anything a dead character dropped stays put across raids. Existing-world loot persistence + fresh-encounter variance, without needing a fully continuous persistent simulation.
- **Purchasable support abilities** — airstrikes, care packages, and similar vendor-bought consumables, carried into a raid as ordinary items (reuses the existing `Server_UseItem` dispatch pattern). No PZ equivalent.
- **Simplified simulation** — every PZ system ships here at roughly **1/3 of PZ's depth**, chosen for readability. Depth can grow later; opacity is not a feature we inherit.
- **Modern, transparent UX** — direct interaction prompts, radial quick-menu, and (Notes §21) **items/actions show their actual mechanical effect on hover/preview** rather than hidden numbers — "player knowledge is mainly built on common sense and playing the game," not a wiki.

**⚑ DECISION 1 — Camera.** Recommended: **top-down, *Door Kickers 2*-framed** (steeper pitch than a classic 45° isometric — closer to ~65–75° — zoom range tight enough to read character/weapon/room detail, wide enough for tactical awareness; yaw rotation in 45° steps). The existing `ApplyCameraPerspective` architecture makes this a *new perspective entry*, not a rewrite — cut the perspective list from 4 (FP/TP/GunCamera/Bodycam) down to **TopDown + OverShoulder** (over-shoulder kept as an aim-zoom and as a hedge). First-person, GunCamera, Bodycam: shelved (code kept in git history, perspectives removed from the cycle). **The dev's real-time-control requirement is explicit and non-negotiable: full direct player control and normal movement at all times — this decision borrows only DK2's camera angle/framing, never its squad-command/pause-and-plan gameplay layer.** Unchanged in spirit from v0.1, refined 2026-07-19 with the DK2 reference as a concrete visual touchstone.

**⚑ DECISION 2 — Same repo vs. fresh project.** Recommended: **same repo, same UE project.** Unchanged from v0.1.

**⚑ DECISION 3 — Art source.** **Resolved 2026-07-19, superseding v0.1's Synty recommendation entirely** — the dev doesn't like Synty's style and won't use it for anything. Direction instead: **dark, earthy tones with slight realism, kept low-poly** (low-poly as a production/performance choice that also keeps hand-modeling in Blender viable later, not a cartoon-toy look). This sits naturally under the *Door Kickers 2* reference above rather than in tension with it — DK2's own look is grounded and quietly detailed, not bright/flat, so the printed/decal-texture-detail note for weapons/props still holds, it just isn't contrasted against a flat Synty environment style anymore. **Sourcing is the dev's own** (ArtStation and elsewhere) — see §5, which now tracks *what's needed* rather than *where to buy it*.

**⚑ DECISION 4 (NEW) — Profession-based spawn points: solo too, or co-op-only?** Notes §4.1 wants players to start at a profession-tied spawn location, explicitly calling out the co-op "find each other" fun. Recommended: **apply to both, with a lobby-level "scatter spawns" toggle.** Solo players still get a profession-flavored starting location (keeps one system instead of two), but a co-op group can toggle scattered starts off if they'd rather just start playing together immediately. Confirm in §7.

**⚑ DECISION 5 — Hostile human roamers: timing.** *Resolved 2026-07-18, against this doc's own recommendation.* Notes §19 asks for them unconditionally, not conditionally — only *timing* was ever in question. This plan originally recommended building them alongside zombies in P4 (cheap AI reuse); **the dev chose to push them past the v1 vertical slice instead** — get the core survival loop (zombies, health, combat, loot) fully solid first, then add hostile roamers as the first post-v1 addition. See Phase P4 and Phase P10's post-v1 list — the AI-reuse argument for building them cheaply alongside zombies still holds and makes them a fast, low-risk follow-up once v1 ships, not a from-scratch system.
**⚑ SUPERSEDED 2026-08-27** — the extraction pivot promotes hostile human roamers to a core v1 system (see the "Deliberately different from PZ" bullet above). Heist-style contracts need guarded/contested loot to justify their risk; the AI-reuse argument for building them alongside zombies still holds, it just no longer waits for a "v1 vertical slice" that this doc's own premise no longer describes.

**⚑ DECISION 6 — Does the investigation/cure arc end or reset the world?** Notes §1 removes "no win condition" as a philosophy but doesn't say the game should *end* on completion — it says add an "end goal to survival," which reads as something to chase, not a game-over screen. Recommended: **the investigation arc is an optional capstone with no forced ending** — reaching its conclusion unlocks a lore epilogue and a meaningful, persistent world-state change (example: a rescue/evac becomes available, or a new sandbox modifier unlocks), but the world keeps running and the character can keep playing, same continuity spirit as PZ's own permadeath-into-new-character loop.
**Reframed, not resolved, 2026-08-27** — "the world keeps running" now means "the hub keeps running," and the capstone is recommended (not yet dev-confirmed, see Decision 10 below) to be a narrative contract line rather than open-world discovery. The core question — does completing it meaningfully change anything persistent — still applies, now scoped to hub/save state (a hideout unlock, a new vendor, a sandbox modifier) instead of world state. Confirm in §7 before the contract system's narrative-content work starts in earnest.

**⚑ DECISION 7 (NEW, 2026-08-27) — Hub-side secure stash.** Confirmed by the dev this session: a **secure stash exists at the hub, never tied to character death.** On death, a character loses its skills, XP, and everything currently held/carried — the stash is untouched. This is what makes the vendor/currency economy function at all (there has to be somewhere to bank gear/currency between raids that isn't at risk every single raid).

**⚑ DECISION 8 (NEW, 2026-08-27) — Skill XP fully resets on character death.** Confirmed by the dev this session: **no persistent skill-like progression survives a character's death** — only money, hideout upgrades, vendor rep/unlocks, and stash contents do. "Skill experience," named as one of the loop's three core rewards, is real but scoped to a single character's life. This is why §3.1's skill system was reshaped (not just retuned) around fast, milestone-based growth rather than PZ's slow-burn model — a system that needs dozens of hours to pay off can't, if most characters won't live that long.

**⚑ DECISION 9 (NEW, 2026-08-27) — One seamless raid zone, not multiple maps.** Confirmed by the dev this session: the quarantine zone is **one large seamless map** (city/suburbs/rural as districts of it), reusing the already-planned World Partition district-by-district build-out (`Docs/Beta/B4X`) almost entirely, with multiple entry/extraction points rather than several discrete raid maps to choose between.

**⚑ DECISION 10 (NEW, 2026-08-27, flagged recommendation — not yet dev-confirmed) — Investigation/cure-arc capstone folds into the contract system.** Recommended: deliver the same content ambition as Decision 6 through a **narrative contract line from one specific hub vendor**, rather than open-world environmental-storytelling discovery. Lower-stakes than Decisions 7–9/11 (nothing has been built against it yet, so it reverts cheaply) — flagged explicitly rather than silently assumed.

**⚑ DECISION 11 (NEW, 2026-08-27) — Utilities-shutoff becomes a per-raid hazard, not a persistent ratchet.** Confirmed by the dev this session: the old permanent, one-way "power/water die on a randomized day" world-clock (§3, §1's "Phase-transition world state" bullet) doesn't fit a zone that reseeds every raid entry. Each raid instance now rolls its own utilities state as part of the same reseed pass that randomizes loot/zombie/hostile placement — sometimes power's already out, sometimes not — rather than either a permanent world-spanning countdown or a fixed always-on cycle with no variance.

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
| Moodles / needs (§5) | **KEEP, reshaped 2026-08-27** | **Raid-duration meters, not persistent-life attrition:** Hunger, Thirst, Stamina start full each raid and drain across that raid's session. Fatigue **cut** (no fit for a raid entered fresh). Wet/Temperature **cut from core** (may return as an occasional per-raid hazard-roll modifier, not an always-on system). Injury/Pain/Infection kept as a two-phase raid/hub system — see §1's "Infection as a visible, urgent countdown" bullet. |
| Nutrition micro-sim (§5) | **CUT** | Food restores Hunger; quality = bigger/longer restore. |
| Skills, learn-by-doing, books (§6) | **SIMPLIFY, reshaped 2026-08-27 (extraction pivot, Decision 8)** | Fast, milestone-based growth within a single character's life (full reset on death — no persistent skill-like progression). Narrowed roster: Attributes (Strength, Stamina, Sneak) + per-weapon-class melee skill bars + Aiming/Reloading + First Aid + Lockpicking. Maintenance cut (→ hub vendor repair service); Sprint folded into Stamina. See §3.1 (rewritten). |
| Melee combat, stamina economy (§7.1) | **KEEP** | Shove + swing + stomp, stamina-gated, durability-lite. |
| Firearms as loud/scarce power (§7.2) | **KEEP** | Already built mechanically; noise system makes it PZ-honest. |
| Stealth/noise/vision model (§7.3) | **KEEP, simplified** | Crouch = quieter + slower; hearing radii; vision cones. Sneak is now a stat, not a dedicated skill tree. |
| Zombie lore + hordes + migration (§8) | **KEEP, simplified** | Shamblers, hearing/sight, door-banging, zone population + wander. **New 2026-07-26 (dev call):** a "freshness" axis — recently-turned zombies are faster and hit harder; the longer a zombie has been undead, the more it slows down and weakens. Genuine large hordes (100+, visually distinct, not just a number) are explicitly **important to the vision**, not a nice-to-have — do not water this down as a performance shortcut without checking first. |
| Per-body-part health, 17 zones (§9) | **SIMPLIFY** | 4 zones: Head, Torso, Arms, Legs. Emergency amputation as an infection-stopping "second chance." |
| Hand-built mega-map (§10.1) | **REPLACE** ⚑ *revised 2026-08-27, extraction pivot (Decision 9)* | The same fictional Adirondacks-modeled region, built the same phased/district-by-district way, **now framed as "the quarantine zone" — one large seamless raid map** (city/suburbs/rural districts) entered and exited from the hub, not a persistent world the player permanently lives in. World Partition district plan carries over unchanged; multiple entry/extraction points replace the old single-spawn framing. |
| Container/zone loot tables (§10.2) | **KEEP, refined** | Data-asset loot tables, equip-only vs. carry-only categories, finite world-count rarity pools. |
| World persistence + erosion (§10.3) | **SIMPLIFY, reshaped 2026-08-27** | Erosion visuals still cut v1. Persistence itself splits in two: the **hub** (stash, vendor rep/unlocks, hideout upgrades, currency) persists across every character's death; the **zone** persists dropped-loot/corpse state exactly as left, but reseeds loot-container contents/zombie density/human-hostile placement on every raid entry. No single "the world" persistence model anymore — see §1's "Raid reseeding" bullet. |
| Utilities shutoff phase transition (§10.3) | **KEEP, reworked 2026-08-27 (Decision 11)** | No longer a permanent one-way ratchet — each raid instance rolls its own utilities state (power/water sometimes already out) as part of the raid-reseed pass. |
| Seasons/weather/temperature (§11) | **SIMPLIFY** | Day/night + rain/fog v1, Adirondacks-tuned. |
| Weight encumbrance + bags (§12) | **KEEP, simplified** | Weight + bag slots, no bags-in-bags recursion. |
| Clothing protection layers (§12) | **SIMPLIFY** | Single outfit slot-set with protection values. |
| Carpentry/base-building (§13.1) | **SIMPLIFY, relocated 2026-08-27** | Moves from in-zone construction to **hub-side hideout upgrade stations** (Tarkov-hideout-station pattern) — doesn't fit a raid session the character might not survive. Barricades/reinforcement may still exist as a minor in-raid tactic (e.g. slow a horde at a doorway) but are no longer a persistent building investment. |
| Metalworking/electrical/plumbing chains (§13.2) | **CUT v1** | Generator-as-item without wiring sim. |
| B42-style deep crafting web (§13.3) | **CUT** | Events/missions/investigation are our late game instead. |
| Farming/foraging/fishing/trapping (§14) | **CUT/MINIMIZE, 2026-08-27** | A raid-session structure doesn't fit long-cycle farming or fishing/trapping — cut outright. Foraging survives only as in-raid loot spawns (a container/zone type), not a distinct gathering mechanic. A hub-only crop mechanic (grows on real/game time between raids) is a possible later minor addition, not scoped now. |
| Vehicles (§15) | **DEFER — later in development, before beta** ⚑ *revised 2026-07-26* | Not v1/Stage-1 work, but no longer an indefinite deferral: dev wants a real vehicle system landed before the game reaches beta. Own planning pass when Stage 1 (the core playable loop) is done; changes the map-scale math in §3's world-scale row above once scheduled. |
| TV/radio diegetic media (§16) | **KEEP, repurposed** | Tutorial + mission-giver. |
| Meta events (§17) | **KEEP, expanded ambition** | Event variety a stated priority. |
| Modes/challenge presets (§18) | **CUT v1** | Sandbox sliders post-v1. |
| MP: dedicated servers, big counts (§19) | **SIMPLIFY** ⚑ *revised 2026-07-26, extended 2026-08-27* | **4+ player co-op**, listen-server as the primary/default mode; an optional paid dedicated-server hosting path for groups who want one, added later. **New 2026-08-27**: friendly fire is always on, but a shared/hosted world is a trusted invited group (no stranger PvP/matchmaking to build). Later dedicated multiplayer worlds need **character/stash save data keyed per world**, so gear farmed in a private/solo world can't cross into a shared one — an extension of the existing save-topology plan (`Docs/Beta/B3`), not a new save system. Raises B8's network/perf budget math versus the original 2–4 assumption; re-baseline before locking numbers. |
| Modding/Lua (§20) | **CUT v1** | Data-asset-driven design keeps the door open. |
| NPCs/factions (§19, §22.1) | **PROMOTED TO CORE, 2026-08-27** ⚑ *supersedes Decision 5's deferral* | Hostile human roamers ship alongside zombies from v1 — heist/guarded-loot contracts need them. Full NPC survivor/dialogue/reputation systems beyond "hostile faction to fight" stay deferred. |

### 3.1 — Skill system (reshaped 2026-08-27, extraction pivot, Decision 8 — supersedes the 2026-07-19 revision below in shape, not just numbers)

**Why this changed from a retune to a reshape**: skill XP now fully resets to zero on every character death, with no persistent skill-like progression at the hub/account level (Decision 8). PZ's slow-burn, learn-by-doing-over-dozens-of-hours model assumes a long single life — under permadeath-every-few-raids, most characters will never live long enough to feel it. The system below is narrower (fewer tracked progressions) and faster (milestone-based, not granular slow-incrementing bars), so a character's whole skill arc can be felt within a raid or two, not a full persistent playthrough.

**Core attributes (not skills — passive stat pools):**
- **Strength** — increases melee damage and weight capacity. Directly rewards extraction's own core tension (how much loot can you carry to an extraction point). Grows from strength-relevant actions (melee kills, carrying heavy loads).
- **Stamina** — governs the melee/sprint/movement resource economy. **Absorbs the old Sprint attribute's effects** (sprint speed/endurance cost) — one stat instead of two covering overlapping ground. Grows from stamina-costing actions (sprinting, swinging) and from general use.
- **Sneak** — reduces detection radius/noise while crouched. Directly rewards avoiding a fight you don't need (a horde or a human-hostile squad you can route around) — arguably *more* central to extraction than to open-world survival, where avoidance was one option among many. Grows through use.

**Combat skills:**
- **Melee weapons** — each weapon *class* still gets its own skill bar, leveled by landing hits with that class. Unchanged shape (higher level: faster attack speed, increased damage, increased critical chance) — already fast-payoff, kept as-is.

**Firearm skills:**
- **Aiming** — increases with weapon usage (shots fired/hits landed). Affects accuracy, time-to-aim, effective range. Unchanged.
- **Reloading** — increases reload speed and per-round load/unload speed. Unchanged.

**Medical skill:**
- **First Aid** — increases with use. Higher levels unlock more effective/faster medical-item use — matters more now, not less, given the raid/hub two-phase wound system (§1) makes fast, effective in-raid treatment a real decision under time pressure.

**Utility skill:**
- **Lockpicking** — a quieter alternative to breaching locked doors, pure success-chance roll gated by level (no minigame), failed attempts generate noise. **Kept and arguably more relevant under the pivot** — heist/high-risk-zone contracts plausibly gate their best loot behind locked doors/containers, making this a real access skill, not just a stealth-flavor option.

**Cut from the tracked-skill roster, 2026-08-27:**
- **Maintenance** — folded into a hub vendor repair service instead of a trained skill. Weapon-wear management as a slow-burn mastery skill doesn't pay off fast enough to matter within a short character life; paying a vendor to maintain gear between raids is a cleaner fit for the hub-economy loop anyway.
- **Sprint** — folded into Stamina above; a separate attribute covering nearly the same ground added a tracked stat without adding a real decision.
- **Fishing, Building, Foraging, Cooking** — cut outright, not deferred. These assumed a long-horizon, self-sufficiency survival life; a raid-based mercenary loop has no use for them (Foraging exists only as an in-raid loot-container type now, not a skill — see §3's Farming/foraging row).
- **Mechanics** — stays deferred, tied to whenever Vehicles (`BV`) lands, unchanged from the prior plan.

**Milestone-based leveling (shape locked in, exact thresholds still open — see `⚑ DECISION 10`-adjacent tuning pass, not blocking):** instead of a granular XP bar that creeps upward, each tracked skill advances at clear, telegraphed thresholds tied to raid-relevant actions (e.g. "land 15 hits with Blunt weapons this life → Melee(Blunt) level 2") — level range 1–5 unchanged, but the curve is tuned so level 2–3 is reachable within a raid or two of focused use, not requiring a long uninterrupted life. Exact thresholds/curve numbers land in `Docs/TuningReference.md` when this phase is actually built, not here.

**Net v1 roster**: 3 attributes (Strength, Stamina, Sneak) + 5 skills (Melee per-weapon-class, Aiming, Reloading, First Aid, Lockpicking) — down from the prior 4 attributes + 6 skills, narrower on purpose so what's left pays off fast and matters for raid performance specifically.

**Expansion path (not built now):**
- Melee weapon-class skill bars can still grow in number as weapon variety grows — the system already supports N classes with zero rework, unaffected by the pivot.
- Mechanics arrives whenever Vehicles does (post-v1), unchanged.

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

**Unified attack input** — one `IA_Attack` button (already renamed from a placeholder `IA_Melee` this session) whose effect depends on what's equipped. **Built 2026-07-21 across two rounds** (first the dispatch skeleton, to unblock testing; then real per-weapon stats the same night as the P5/P6 overnight push):
- `UZSWeaponConfig` gained an `EZSAttackType { Ranged, Melee }` field (no `Unarmed` value — bare-fist isn't a weapon config instance, it's `AZSPlayerCharacter`'s own fallback when nothing's equipped). Generalizes the config to cover melee weapons too, rather than a parallel melee-config hierarchy — consistent with the existing multi-weapon rule.
- `AZSPlayerCharacter::HandleAttack` (bound to `IA_Attack` alone — `IA_Fire` is no longer separately bound) dispatches on `CurrentWeapon`'s config: `Ranged` → the existing `HandleFireStarted`/auto-fire-timer machinery; `Melee` → `Server_WeaponMeleeAttack` (real per-weapon stats, see below); no weapon → `Server_MeleeAttack`, the flat `Unarmed*` character tunables.
- **Real per-weapon melee stats, durability-lite, and simple hit knockback**: `UZSWeaponConfig` gained `MeleeDamage`/`MeleeRange`/`MeleeAttackInterval`/`MeleeDamageTypeClass`/`MeleeMontage` (mirrors the `Unarmed*` fields), `MaxDurabilityHits` (0 = unbreakable default), and `MeleeKnockbackStrength`/`FireKnockbackStrength`. The bare-fist and weapon-melee overlap/hit logic was deduplicated into a shared `PerformMeleeSwing` helper. Breaking a weapon (`AZSWeapon::Server_ConsumeDurabilityHit` hitting 0) auto-unequips it **and** clears it from its own hotbar slot - durability lives on the `AZSWeapon` actor instance, not on any per-item state the hotbar itself tracks, so leaving the slot filled would let re-selecting it spawn a fresh full-durability weapon from the same config; a known limitation of not having P6's inventory give items real per-instance state yet. Knockback (`ApplyHitKnockback`) is a plain `LaunchCharacter` impulse - physical only, no AI stagger/interrupt (would need `BT_Zombie` editing, deliberately not attempted in an unsupervised session).
- **Melee weapon display — RESOLVED 2026-07-26 (dev decision, rescope pass).** Not a single generic held-prop pose and not a unique pose per weapon — a middle ground, grouped by weapon *category*: one shared `TP_Mesh` pose for long-guns (rifle/shotgun/LMG all share it), a separate shared pose for pistols, and a separate shared pose for melee weapons. This unblocks authoring a real melee `UZSWeaponConfig` (previously only a temporary rifle-pose reuse existed, per B0-T1.1/OQ-B0-11 in `Docs/Beta/`).
- **Still not built**: `SecondaryHand` (see the hotbar note above); shove/stomp (shove needs an input-action decision, stomp needs a "downed zombie" AI state - neither exists); stamina cost on melee; ammo scarcity tuning (needs real playtest data); hit-reaction/knockdown is knockback-only (see above), not a real AI stagger state.
  **Exit:** a player starts unarmed, equips a weapon from the hotbar in real time, and one Attack button does the right thing whether they're bare-handed, swinging a bat, or holding a rifle — the PZ death loop exists on top of that (greed + noise + stamina mismanagement kills a player who had every tool to survive).

### P6 — Inventory, loot & scavenging
**Core backbone built 2026-07-21** (unsupervised overnight session - see `Docs/SessionHandoff.md`; nothing compiled or PIE-tested yet): `UZSInventoryComponent` (replicated flat `CarrySlots` + two equip slots resolved that night, see §7 below), weight-based encumbrance (`GetEncumbranceMultiplier`, folded into `AZSPlayerCharacter::UpdateMovementSpeed` alongside P3's mobility multiplier - **not** the `PrimaryHand`/hotbar combat loadout slots, which stay P5's); `UZSItemConfig` gained equip-only vs. carry-only fields (`Weight`/`MaxStackSize`/`bIsEquippable`/`EquipSlot`/`CarryCapacityBonus`/`Rarity`/`WorldMesh`). `AZSContainerActor` + `UZSLootTableConfig` (weighted rolls, "loot all" on interact - a UI-less v1 bootstrap) and a finite world-count rarity pool (`AZSGameState::RarityPoolEntries`, resolved that night, see §7) are also built. `AZSWorldItemActor` (a real replicated pickup actor) covers "dropped-item persistence in the running session" literally, not via a save system (none exists - that's P7's).
- Container actors + data-asset loot tables ✅ (see above); **per-zone quality tiers genuinely not built** - no zone system exists anywhere in the project to key off of, not just an unwritten feature; finite world-count rarity pools ✅ (see above).
- Inventory UI + radial quick-use, transparent stat-preview rule carried through - **not built**, deliberately deferred: UMG/Blueprint content work wasn't attempted unsupervised (Live Coding/Blueprint corruption risk with nobody present to catch it, per `CLAUDE.md`'s lesson).
- Dropped-item persistence ✅ (see above).
- **Known gap**: P5's `HotbarSlots` still holds direct `UZSWeaponConfig*` references rather than referencing items actually held in `UZSInventoryComponent` - wiring "you can only hotbar a weapon you're actually carrying" is unbuilt follow-up work.
  **Exit:** full scavenge loop in graybox: run out, loot under threat, haul back, stash; item scarcity feels intentional. **Not reachable yet** - no Inventory UI, no authored content, nothing tested.

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
- **Skill/attribute XP hookup** (learn-by-doing across P2–P6 systems): Strength/Stamina/Sneak/Sprint attributes, per-weapon-class Melee bars, Maintenance, Aiming, Reloading, First Aid, Lockpicking — all per §3.1's revised list, wired to their respective P2–P6 systems.
  **Exit:** a stranger survives their first 30 minutes without a wiki and dies to something they understand.

### P10 — Production hardening → public vertical slice
- Audio pass, VFX pass, performance profiling, fixed-tick save safety, crash/soak testing, packaged Windows build tested over real LAN/direct-IP.
- Trailer-able vertical slice: 20–40 minutes of tuned co-op survival on the real map, including at least one meta event and a taste of the investigation arc.
  **Exit:** shippable demo build. **First post-v1 addition (Decision 5): hostile human roamers**, built cheaply on top of P4's zombie AI architecture — **reconfirmed 2026-07-26** (OQ-X-06). Post-v1 backlog also includes the deferred skills from §3.1 (Fishing, Building, Foraging, Cooking, Mechanics), **friendly survivor NPCs** (explicitly deferred post-release, dev-confirmed 2026-07-26), vehicles (now DEFER not CUT — see §3), sandbox sliders, deeper seasons/temperature, Steam/EOS + dedicated server (now an optional paid path, not cut — see §3).

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
  - **Player death, loot, and world continuity** (also touches P4/persistence). ⚑ **REVISED 2026-07-26, dev decision (rescope pass) — simpler and unified across solo and co-op:** on death, dropped loot stays at the death location, and the player's character becomes a zombie (P4 - `AZombieCharacter` doesn't support death-triggered spawning yet, only placement/config-driven). **Death always respawns a fresh character into the same, persistent world — solo included.** The earlier idea floated here ("solo death ends that world outright, party-wipe ends the co-op world") is **superseded** — there is no asymmetric solo-only world-ending rule. The world keeps running, your loot and any base/safehouse you built stay findable, and you just start over as a new survivor in it. This is actually a smaller change from what `AZSPlayerCharacter::Server_RespawnAsNewCharacter` already does (always respawns into the same world) than the earlier backlog note was — mainly confirms the *current* behavior's world-continuity direction is right and removes the solo-specific exception that would have required extra special-casing.

### P4 — Zombies
1. ~~Is ~150 concurrent on-screen zombies the right target?~~ **RESOLVED 2026-07-26** — yes, and treat it as a floor, not a stretch goal: genuine large-horde presence is confirmed important to the dev's vision, not a cuttable nice-to-have if performance gets tight. Optimize the tech to hit the number before considering lowering the number.
2. **NEW 2026-07-26 — zombie "freshness" mechanic**, undefined in detail: recently-turned zombies faster/stronger, degrading toward slower/weaker over (in-game?) time. Needs a decision on what drives the timer (time-since-turned vs. some other clock) and where the tuning knobs live (`UZSZombieConfig` per-type curve is the obvious fit, consistent with the multi-config rule) — schedule alongside the existing zombie AI depth pass rather than as a separate system.
- Post-v1 backlog: hostile roamer spawn logic, loot drops, wound/infection system parity.

### P5 — Loadout & unified combat
**All resolved 2026-07-21:**
1. ~~Melee weapon variety~~ **RESOLVED** — curated 4–6 archetypes (one per feel-category), not PZ's full breadth.
2. ~~Durability~~ **RESOLVED** (already settled by the existing plan text) — break-only for v1, no repair sim.
3. ~~Hotbar size and key scheme~~ **RESOLVED** — both number-key direct-select (1–9) and scroll/cycle supported, not one-or-the-other; scroll/cycle is the gamepad-friendly path.
4. ~~`SecondaryHand` semantics~~ **RESOLVED** — independently usable (offhand pistol/flashlight), not just "the other grip." Follow-on question, not yet resolved: how an offhand item's own action gets triggered, since `IA_Attack`/`Server_Attack` only ever considers `PrimaryHand`.
5. ~~Equip/holster timing~~ **RESOLVED** — a per-`UZSWeaponConfig` field (e.g. `EquipTimeSeconds`), same pattern as `FireDamage`/`FireRange`, not a separate weight-class system.

### P6 — Inventory & loot
1. ~~Bag/equip-slot depth~~ **RESOLVED autonomously 2026-07-21** (dev unavailable to consult - flagged for review, not a confident call the way the P5 questions were): two equip slots, `Back` (large capacity bonus) and `Hip` (small, quick-access) - matches the "weight + bag slots, no bags-in-bags recursion" line already in §3's scope table, and stays proportionate to the project's "roughly 1/3 of PZ's depth" pillar. PZ/DayZ both go deeper (multiple clothing layers, vest+backpack+pouches); this is a deliberately smaller v1, easy to extend by adding more `EZSEquipSlot` enum values later without a rearchitect.
2. ~~Finite-rarity-pool model~~ **RESOLVED autonomously 2026-07-21** (same caveat as above): a single global per-server-session counter per rarity tier, tracked on `AZSGameState` (`RarityPoolEntries`) - not per-zone (no zone system exists to key off yet - per-zone quality tiers are tracked as P6-R7 in `Docs/Beta/01_RevisionRegister_P0-P6.md`, scheduled for B4X once real zones exist) and not persisted across server restarts (no save system exists - that's P7's). Matches the phase file's own "genuinely rare items stay rare across a whole session" wording literally - a per-roll percentage without a world cap wouldn't be "finite" at all.

### P7 — World & persistence
1. Naming pass for the fictional county/towns?
2. Map scale — still ~1×1 km?
3. Scatter-spawns default on or off?

### P8 — Dynamic events, objectives & investigation arc
1. ~~Decision 6~~ **RESOLVED** — optional capstone. **Reconfirmed and extended 2026-07-26 (OQ-X-02):** survive indefinitely, no forced ending, no evac mechanic. New, unshaped **quest list** players can work through over a run — content TBD, dev will supply it later — a lightweight objective layer alongside this investigation arc, not a replacement for it.
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
