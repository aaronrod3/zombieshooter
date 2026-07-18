# Project Zomboid — Complete Design Reference
*A systems-level breakdown of Project Zomboid (The Indie Stone) for use as a foundation document when designing a new isometric low-poly survival game. Organized so each section can be marked KEEP / IMPROVE / REPLACE / CUT during brainstorming.*

---

## 1. Core Identity & Design Philosophy

- **Genre:** Open-world sandbox zombie survival RPG, isometric, real-time with pause (single-player).
- **Tagline/thesis:** "This is how you died." The game is explicitly about *inevitable* death — the player's story is how long they lasted and what they did, not whether they won.
- **No win condition.** There is no ending, no extraction, no cure. Survival duration and self-directed goals ARE the content.
- **Simulation-first philosophy:** Nearly every system is a simulation rather than an abstraction (nutrition has calories/protein/fat; wounds are per-body-part; temperature interacts with clothing layers and weather). Depth comes from systems interacting, not from scripted content.
- **Slow-burn horror:** Zombies (default lore) are Romero-style shamblers. Individually trivial, lethal in groups or when the player gets complacent. Tension comes from attrition, noise management, and the meta-game of risk vs. reward on every loot run.
- **Player-authored narrative:** With no quests or NPCs (as shipped), all drama is emergent — a burned safehouse, a bite on day 40, a winter with no food stores.
- **Permadeath:** Character death is final (save-per-world persists; the world continues with a new character in the same world if the player chooses).
- **Difficulty as identity:** The game is intentionally punishing and obtuse in places; the community treats mastering it as a badge. (Design tension to consider: this gatekeeps a large casual audience.)

---

## 2. Core Gameplay Loop

**Minute-to-minute:** Scavenge → manage encumbrance → avoid/fight zombies → return to base → eat/drink/rest → craft/fortify → plan next run.

**Day-to-day:** Establish safehouse → secure water & food pipeline → loot key locations (hardware stores, warehouses, gun stores, pharmacies, bookstores) → skill up (reading + practice) → expand reach with a vehicle.

**Meta arc (the "three phases"):**
1. **Early (Days 1–9):** Water and electricity are ON. Loot aggressively, stockpile water, find weapons, claim a base. TV channel "Life and Living" grants large passive XP — watching scheduled programs is a genuine early-game strategy.
2. **Mid (utilities shut off, ~day 9–30 randomized):** Water/power die. Generator + fuel logistics, rain collectors, food preservation (canning, curing), farming begins. Loot gets scarcer near spawn; vehicle range matters.
3. **Late (month+):** Full self-sufficiency — farming cycles, trapping/fishing/foraging, tailoring armor, metalworking, ammo reloading (B42), livestock (B42). The threat shifts from zombies to boredom, complacency, winter, and one bad mistake. **This phase is widely criticized as empty — the #1 design gap after NPCs.**

---

## 3. Perspective, Camera & Controls

- Fixed isometric camera, 4-direction (previously) / free-ish zoom; buildings cutaway/fade per floor so interiors are readable.
- Multi-story buildings with per-floor rendering; B42 added true basements and much taller structures plus a new lighting/depth-buffer system.
- WASD movement, mouse aim for combat; heavy reliance on right-click context menus for world interaction (chop, disassemble, wash, etc.).
- Extensive keyboard shortcuts; controller support exists but is second-class.
- **Pain point to note:** Interaction is menu-driven and clunky; many actions are buried 2–3 levels into context menus. Modern players expect radial menus / direct interaction prompts.

---

## 4. Character Creation

### 4.1 Professions (Occupations)
Each grants starting skill levels, unique XP multipliers, and a point cost/bonus. Examples (not exhaustive):
- **Unemployed** (+8 free points, no skills) — the "custom build" baseline.
- **Fire Officer** — Axe +1, Sprinting +1, Fitness/Strength friendly; popular all-rounder.
- **Carpenter** — Carpentry +3, Short Blunt +1; base-building focus.
- **Burglar** — Nimble +2, Lightfooted +2, Sneak +2, can hotwire cars from the start (signature utility perk).
- **Veteran** — Aiming +2, Reloading +2, **Desensitized trait** (immune to panic) — the guns build.
- **Park Ranger** — Foraging/Trapping/Carpentry bonuses, faster movement through trees.
- **Farmer, Fisherman, Chef/Cook, Doctor/Nurse, Electrician (can operate generators without the magazine), Mechanic, Engineer (can craft bombs/traps), Lumberjack (axe specialist), Police Officer, Security Guard (Night Owl), Construction Worker, Repairman, Metalworker, Fitness Instructor.**
- Design note: professions are essentially **starting-XP templates + 1–2 unique unlocks**. The unique unlocks (hotwire, generator use, desensitized, bomb crafting) are what make choices feel meaningful.

### 4.2 Traits (positive cost points; negative grant points)
A large point-buy list creating enormous build variety. Representative set:
- **Positive:** Athletic/Strong (stat boosts), Keen Hearing, Cat's Eyes (night vision), Dextrous (fast inventory transfer), Fast Learner (+30% XP), Lucky (better loot), Organized (+30% container capacity), Wakeful, Iron Gut, Thick Skinned, Brave, Outdoorsman, Fast Reader, Amateur Mechanic, Handy, Gymnast, Low Thirst/Light Eater.
- **Negative:** Overweight/Obese/Underweight, Weak, High Thirst, Hearty Appetite, Slow Healer, Prone to Illness, Smoker (must smoke to relieve stress — famously a "free points" pick), Short Sighted, Hard of Hearing, Deaf, Clumsy, Conspicuous, Cowardly, Agoraphobic/Claustrophobic (panic conditions), Restless Sleeper, Insomniac, Pacifist, All Thumbs, Slow Reader, Illiterate (cannot read at all), Thin Skinned, Hemophobic, Sunday Driver, Weak Stomach.
- Several traits are **dynamic in B42/later builds**: weight, fitness, and some traits can be gained/lost through play (e.g., losing Overweight by dieting; gaining Desensitized-like panic reduction over months of kills).
- **Design insight:** negative traits as a points economy is a beloved system — players min-max "free points" picks; builds are a huge part of replayability and community content.

### 4.3 Appearance & identity
Cosmetic customization (gender, hair, beard, clothing start). Clothing matters mechanically later (protection/insulation), not at creation.

---

## 5. Stats, Needs & the Moodle System

**Moodles** are the iconic stacked status icons (4 severity tiers each) that communicate internal state without numbers:
- **Hunger** — reduces strength/damage, eventually health drain.
- **Thirst** — faster onset than hunger; lethal sooner.
- **Fatigue (sleep)** — slows movement, massive combat penalties, forced micro-sleeps at extremes.
- **Endurance (stamina)** — drained by running, swinging, hauling; recovered by resting. Governs the melee combat economy; over-swinging = death.
- **Heavy Load** — encumbrance over capacity: slower movement, faster endurance drain, tripping risk.
- **Injured/Pain** — from wounds; pain reduces accuracy/speed and prevents sleep (painkillers).
- **Panic** — proximity/sight of zombies; wider misses, worse crits, tunnel vision. Reduced by Brave/Desensitized, beta blockers, alcohol, and *months of survival* (habituation).
- **Stress/Unhappiness** — from blood, darkness (phobias), boredom, reading grisly things, unwashed clothes; slows actions; relieved by comfort activities (books, TV, cigarettes for smokers, good food).
- **Boredom** — indoor idleness; feeds unhappiness/depression which slows action speed dramatically.
- **Sick (Queasy→Fever)** — food poisoning, corpse proximity, cold/flu, or **the** infection; ambiguity ("is this a cold or am I dead?") is a deliberate and brilliant tension mechanic.
- **Cold/Hot** — temperature vs. clothing insulation/layers; hypothermia/heatstroke; wetness accelerates cold; seasons matter.
- **Drunk**, **Tired from muscle strain (B42: separate muscle fatigue per activity)**, **Wet**, **Windchill**.

**Underlying simulation (mostly hidden numbers):**
- **Nutrition:** calories, carbs, protein, fat; weight changes over weeks; weight affects fitness/traits. Under-eating tanks strength; overeating → overweight penalties.
- **Fitness & Strength** are slow-moving "core stats" trained by exercise (with muscle soreness as a pacing gate) and affected by weight/food.
- **Design insight:** Moodles compress ~15 interacting simulations into readable iconography. This is the single most copied system in the genre and is core to "PZ-like" feel.

---

## 6. Skills & Progression

### 6.1 Skill list (Build 41 baseline; B42 expands)
- **Passive:** Fitness, Strength.
- **Agility:** Sprinting, Lightfooted, Nimble (combat-stance movement), Sneak.
- **Combat:** Axe, Long Blunt, Short Blunt, Long Blade, Short Blade, Spear, Maintenance (weapon condition preservation).
- **Firearms:** Aiming, Reloading.
- **Crafting:** Carpentry, Cooking, Farming, First Aid, Electrical, Metalworking, Mechanics, Tailoring; **B42 adds:** Welding split, Masonry, Pottery, Glassmaking, Knapping/flint, Carving, Animal Care/Husbandry, Butchering, Tracking, and a much deeper crafting web.
- **Survivalist:** Fishing, Trapping, Foraging (B41 reworked into a "search mode" investigation radius).

### 6.2 XP model
- **Learn-by-doing:** every action grants XP in its skill. No XP for kills per se — you get Axe XP by swinging axes.
- **Skill books multiply XP** (Vol 1–5 map to levels 1–2, 3–4, 5–6, 7–8, 9–10) — up to a massive multiplier band; reading is a core loop activity (and time sink, balanced by boredom/fatigue).
- **Recipe magazines** gate specific recipes (generator connection, ammo types, cooking recipes) — loot-driven knowledge unlocks.
- **Occupation/trait XP multipliers** (+75% for profession-favored skills etc.).
- **"Life and Living" TV** broadcasts scheduled shows for the first ~9 days granting large Cooking/Carpentry/etc. XP — a clever diegetic tutorial + early-game scheduling pressure. VHS tapes found in the world do the same on-demand (finite uses).
- **Design tension:** grind is real; several skills (Fishing pre-rework, Electrical, First Aid) level painfully. B42 rebalanced many. Slow, mostly-invisible progression is repeatedly cited as a friction point for new players.

---

## 7. Combat

### 7.1 Melee (the primary mode)
- Weapon classes with distinct speed/reach/damage/durability/endurance cost: axes (high dmg, slow), long blunt (crowbar durability meme, baseball bat = beloved all-rounder), short blunt (hammers), long blade (katana = rare grail), short blade (knives = jaw-stab insta-kill animation but high risk/low durability), spears (crafted, long reach, fragile, jaw-stab specialists).
- **Push/shove + stomp:** always-available no-weapon options; shove-to-ground + boot-stomp is the canonical low-risk technique early.
- **Hit mechanics:** swing timing, weapon reach, multi-hit (sandbox: max targets per swing), knockback/knockdown chances, crit chance influenced by skill/panic/fatigue/moodles.
- **Endurance economy:** every swing costs stamina; fighting >5–10 zombies without pacing is how mid-game characters die. Combat is resource management, not reflexes.
- **Weapon condition & Maintenance skill:** weapons degrade and break; repairs are limited; scarcity drives loot loops.
- **Muscle strain (B42):** using a weapon class strains those muscles over a day, punishing marathon clearing; forces rotation/rest. Controversial but deepens pacing.
- **Space management is the real skill:** funneling through doors/windows/fences, never getting surrounded, using tall fences for free lunge-stomp kills, backpedal-swing rhythm.

### 7.2 Ranged
- Firearms are loud (huge attraction radius — a gunshot pulls a neighborhood), ammo-scarce, and skill-gated (low Aiming = miss city). Late-game power spike once trained; shotgun = crowd killer + dinner bell.
- Attachments (scopes, slings), reloading skill/magazine management; B42 adds ammo crafting/reloading benches.

### 7.3 Stealth & noise
- Sound is a first-class system: running, combat, breaking glass, alarms, engines, gunfire all emit radii that attract zombies. Crouch-walking (Sneak/Lightfooted) reduces detection; line-of-sight model with vision cones; darkness helps (and hurts you).
- **House alarms** on random buildings — the jump-scare loot gamble.
- Distraction tools: thrown noisemakers, alarm clocks, (car alarms), later bombs.

### 7.4 Death causes (the honest list)
Bites (infection), scratches/lacerations (lower infection chance), overwhelmed while greedy/overencumbered, falls, fire (very lethal, spreads), car crashes, hypothermia, starvation in winter, complacency ~week 4. "Complacency kills" is effectively a design pillar.

---

## 8. Zombies

### 8.1 Default lore ("Knox Infection")
- Romero shamblers: slow, weak senses individually, dangerous in hordes. 100% airborne infection is OFF by default misconception — default is: **everyone reanimates on death** (all humans are infected), and **bites are ~100% lethal infection, scratches ~7%, lacerations ~25%**. The infection kills over ~2–3 days (queasy → fever → death) with no cure. This "am I infected?" ambiguity window is core horror design.
- Zombies hear, see (cone), and track; they **migrate** (meta-wander), form ambient hordes, bang through doors/windows/fences, and **thump in groups** (buildings are not safe forever).
- **Memory/decomposition options:** how long they remember prey; whether they weaken over months (decomposition setting can make the late game slowly easier).
- **Crawlers:** legless zombies under cars/in grass — the classic ankle-bite ambush.
- Corpse management matters: bodies emit sickness radius; burning/burying corpses is a real chore loop (and fire risk).

### 8.2 Population simulation
- Region-based population density (urban cores dense, rural sparse), **respawn/migration settings** (default: zombies respawn into cleared cells over days and redistribute), event-driven surges.
- **Meta events:** distant gunshots, screams, dog barks, jet flyovers periodically pull zombie crowds across the map — keeps the world's threat topology shifting while you sleep.
- **The Helicopter Event:** ~day 3–10, a helicopter follows YOU for hours, dragging every zombie in earshot toward your position. The single most famous scripted beat in the game; a masterclass in one-time event design.

### 8.3 Full sandbox lore customization
Speed (sprinters/fast shamblers/shamblers), strength, toughness, cognition (can open doors?), senses, memory, hearing, sight, infection transmission (blood+saliva / saliva / none / everyone's infected), mortality timeline, reanimation time, environmental attacks, day/night activity split, population multiplier & respawn. **"Customizable apocalypse" is a headline feature — many players' "real" game is a personal ruleset. CDDA/sprinters runs are community content genres.**

---

## 9. Health & Medical System

- **Per-body-part damage model:** ~17 zones (head, neck, torso, each arm/forearm/hand, thighs/shins/feet). Each can carry distinct wound types: scratch, laceration, bite, deep wound (glass), bullet, burn, fracture.
- **Treatments:** bandages (cleanliness matters — dirty bandage → wound infection, separate from zombie infection), disinfectant, sutures/needle for deep wounds, splints for fractures (weeks-long limps), tweezers for glass/bullets, painkillers/antibiotics.
- **Wound infection vs. Knox infection:** two-tier dread — treatable bacterial infection vs. the untreatable one; the UI intentionally doesn't tell you which you have at first.
- **First Aid skill** speeds/improves treatment (widely considered underpowered — a known gap).
- Sickness sources: rotten food, corpse proximity, rain/cold (colds reduce sneak via sneezing!), and infection. Colds have a full symptom arc.
- Falls/fractures from heights; carrying weight worsens landing; sheet-ropes as risky escape/entry.
- **Design insight:** health as a *logistics and knowledge* system (carry a med kit, know what each wound needs) rather than a health bar creates constant low-level tension and rewards preparation.

---

## 10. World, Map & Loot

### 10.1 Setting
- **Knox Country, Kentucky, July 1993** — fictionalized real towns: **Muldraugh** (classic start, suburban strip), **West Point** (dense, dangerous, great loot), **Riverside** (quiet, country club), **Rosewood** (calm starter, fire station/prison), **Louisville** (huge dense city, endgame loot destination, gated by highway checkpoints), plus March Ridge, Valley Station, mall (Crossroads Mall), military base (Fort Knox area roads), farmland belts between towns. B42 adds **Ekron and large new southwestern map regions**, muddying "safe rural" assumptions.
- One enormous **contiguous hand-built map** (no procedural generation) — thousands of unique enterable buildings, all with full interiors. Community map project ethos: real road trips take in-game hours by car.
- 1993 period setting quietly justifies no cell phones, VHS/TV/radio media loops, analog everything.

### 10.2 Loot design
- **Container-based, zone-weighted loot tables:** building type determines loot (hardware store → tools/nails; pharmacy → meds; bookstore → skill books; gun store → guarded arsenal; warehouses → bulk materials; residential → food/clothes/random).
- **"Prestige" loot locations** are community-known pilgrimage sites (gun stores, the mall, Louisville) — knowledge-of-the-map IS progression.
- Loot rarity fully sandbox-tunable (Insanely Rare → Abundant per category).
- **Annotated maps:** found map items with survivor scribbles hinting at stashes — the closest thing to quests in the base game, and a fan-favorite feature to expand on.
- Physical map items + map UI that must be found/read; no magic minimap by default.

### 10.3 World persistence & erosion
- Everything persists: dropped items, corpses, smashed windows, player constructions, burned buildings (fire permanently destroys structures).
- **Erosion system:** over months, grass grows through roads, windows crack, vegetation swallows the world — visual storytelling of civilization's decay tied to elapsed time.
- Utilities: **water & power shut off** on a randomized timer (~day 9–30 default), the single most important world-state transition (see phases in §2).

---

## 11. Time, Weather, Seasons

- Configurable day length (real-time to 15-min days; default 1 hr = 1 day-hour compression); sleep skips time (with interruption risks); fast-forward in SP.
- Full **seasonal climate model** for Kentucky: July start = heat; autumn rains; **winter is a survival wall** (crops die, temperature lethal without insulated clothing layers, food logistics peak). Fog, rain (wetness→cold), thunderstorms, snow accumulation, wind chill; B42 deepened seasonal foraging/wildlife.
- Darkness is genuinely dark; light sources (flashlights, lamps on generators, candles) vs. attracting attention through windows — curtains/sheets on windows as a core early ritual.

---

## 12. Inventory, Encumbrance & Item Systems

- **Weight-based encumbrance** (capacity from Strength + traits + bags). Bags: pockets → fanny packs → school bags → hiking bags → military rucks; bags-in-bags with equip slots (back/primary/secondary); containers reduce carried weight when worn properly.
- **Grid-less list inventory with drag-drop transfer**, loot-all, transfer-all-of-type; timed transfer actions (moving heavy items takes real seconds — interruption risk while looting is deliberate tension).
- **Clothing layer system:** ~10+ body slots with layers (underwear→shirt→sweater→jacket); each garment has insulation, water resistance, **bite/scratch protection values**, condition (rips patched via Tailoring — armor crafting), blood/dirt (washing affects stress), and temperature tradeoffs (leather jacket = armor but summer heatstroke).
- Item condition/durability across tools, weapons, clothing; batteries for devices; food freshness states (fresh → stale → rotten; rotten = poison, or compost).
- **Favorite/junk tagging, container naming** exist but bulk-management is weak — the inventory UI is the most-modded, most-complained-about surface in the game. Treat as a must-beat benchmark, not a template.

---

## 13. Crafting, Building & Resource Chains

### 13.1 Carpentry & base building
- Plank+nails construction tiers: walls (3 levels), doors, window frames, stairs, floors, roofs(via floors), furniture, crates (storage), rain collectors, composters; **barricades** (planks/metal sheets on doors/windows, both sides); sheet ropes; log walls (axe+logs, low-skill perimeter). Level-gated quality (build HP scales with skill).
- Disassembly of world furniture for materials/XP — "eat the world" loop.
- **Safehouse design is player-expressive:** kill corridors, fence mazes, rooftop farms, moat-of-fences; the community shares base blueprints like architecture.

### 13.2 Metalworking, electrical, plumbing
- Metalworking: propane torch + welding mask; metal walls/doors/bars, repairs; historically undercooked vs carpentry (B42 expands into real smithing/forging chains from ore/scrap).
- Electrical: generators (magazine/profession-gated), wiring lamps, hotwiring (with Mechanics), making remote bombs (Engineer); device disassembly for parts.
- Plumbing: connect sinks to rain barrels for infinite washing/cooking water post-shutoff.

### 13.3 B42 crafting overhaul (direction of travel)
- Massive deepening toward **long-term civilization rebuilding**: masonry, pottery/kilns, glassblowing, blacksmithing from scavenged/forged metal, knapping, carving, animal husbandry chains (see §14), with the stated goal of making "year 2+" self-sufficiency genuinely playable without loot dependence.
- Design reading: TIS is answering "late game is empty" with *deeper crafting*, NOT missions/NPCs — this is exactly the space a competitor can attack differently.

---

## 14. Food Production & Survivalist Systems

- **Farming:** seed→till→sow→water→disease-manage (mildew/flies)→harvest; seasonal viability windows; crops: cabbages, potatoes, carrots, tomatoes, broccoli, radishes, strawberries + B42 expansion (grain, etc.); farming is the late-game caloric backbone; indoor/rooftop farming viable with light.
- **Foraging ("Search Mode"):** zone-based (forest/deep forest/vegetation/urban) investigation radius affected by skill, traits, weather, light; yields berries/mushrooms (poison ID risk!), medicinal herbs, worms, stones, twigs — herbalist niche.
- **Fishing:** rod/line/lures or crafted spear; time-of-day windows; fish size scaling with skill; net traps (B42 reworked into a less grindy, more tactile system).
- **Trapping:** trap types per animal (mouse/rat/rabbit/bird), baits, placement rules (away from player activity), check cycles.
- **Cooking:** recipe system with ingredient substitution (soups/stews/salads/sandwiches), hunger+happiness values per ingredient, spices, baking (flour/yeast era-goods), canning/jarring for preservation, curing, freezer/fridge dependency on power, campfires/BBQs/antique ovens (fire risk, smoke attraction).
- **B42 Animal Husbandry:** chickens/eggs, cows/milk, sheep/wool, pigs; feeding, enclosures, breeding genetics, butchering chains; wildlife (deer etc.) + hunting/tracking.
- **Design insight:** food is a *calendar problem* (seasons, spoilage, power) not an inventory problem — this is what makes PZ's survival feel different from craft-o-mania survival games.

---

## 15. Vehicles

- Found-in-world cars with per-part condition (engine, brakes, tires, battery, muffler, windows, doors, gas tank, trunk); **Mechanics skill + tools** to swap parts between wrecks; car keys/hotwiring; fuel from stations (power-dependent pumps → siphoning post-blackout) and jerry cans.
- Driving model: acceleration/handling per model class (sports/family/vans/trucks/police/ambulance/fire), off-road penalties, crash damage (to car, occupants, and zombies — vehicular slaughter degrades the car), noise attraction (engine quality/muffler), headlights.
- Vehicles as **mobile bases**: trunk storage builds, van-life playstyle; towing (B41.66+); B42 adds animal trailers, more vehicle types & interactions.
- Known weak spots: physics feel floaty/deadly at speed; mechanics UI is opaque; repair grind is steep. (Also HumanitZ's weak spot — differentiation opportunity.)

---

## 16. Media, Knowledge & Diegetic Systems

- **TV/Radio broadcast schedule:** Life and Living (XP), news channels narrating the outbreak's collapse timeline (world-building through static media), automated emergency broadcast; radios (portable/car/ham) with channels, batteries, volume-as-noise-risk; **the broadcasts ARE the narrative** in vanilla.
- **VHS tapes + camcorder home videos:** found media granting XP or story flavor; player must find a working TV/VCR — media as loot.
- Skill books/magazines/newspapers (see §6); boredom/stress relief via literature — comfort as a resource.
- **Design insight:** PZ delivers story without NPCs via environmental storytelling + scheduled media. A mission-driven competitor can keep this texture AND add live actors.

---

## 17. Meta Events & Scripted Beats

- Helicopter event (§8.2), phantom gunshots/screams/dogs, jets, house alarms, random **zombie "zones"** (crash sites, military blockades, survivor-house setpieces: barricaded homes with corpses+loot — micro-stories).
- B41/42 sandbox "events" sliders; modded ecosystems (Expanded Helicopter Events) show huge appetite for MORE dynamic events — direct evidence for your "missions/things to do" thesis.

---

## 18. Modes, Difficulty & Meta Structure

- **Presets:** Apocalypse (default: stealth-lean, deadly), Survivor (stronger combat, classic B40 feel), Builder (gentler, base-focus), **Custom Sandbox** (everything tunable — 100+ options), **Challenge modes:** CDDA ("A Really CD DA" — start injured in a burning world), One Week Later, Winter Is Coming, Studio, Opening Hours.
- **Character-death → new character in same world** (optionally find/loot your old zombified self — beloved emergent moment).
- No difficulty curve management beyond initial settings — the game gets *easier* as player knowledge grows (meta-progression lives in the player, not the save).

---

## 19. Multiplayer & Social

- Dedicated servers + host-a-server co-op (B41); up to large player counts on public servers; PvE co-op base building is the dominant mode; PvP servers with factions/safehouse claims exist (roleplay server scene is significant, with heavy modding).
- Split-screen local co-op (rare + loved feature).
- B42 shipped **single-player first**; multiplayer for B42 arrived later/staged (unstable branch mid-2026) — long MP droughts have been a major community pain point.
- No official NPC survivors in any build (the eternal promise — B43 target per roadmap statements).

---

## 20. Modding & Community

- Steam Workshop with enormous ecosystem (tens of thousands of mods): QoL/UI fixes, maps, vehicles, guns, **NPC frameworks** (Superb Survivors etc. — janky but massively downloaded = demand signal), Expanded Helicopter Events, They Knew, Brita's weapons, RV Interiors.
- Lua-scriptable game logic + map tools; modding friendliness is a pillar of longevity and effectively outsourced QoL R&D.
- **Lesson:** the mod ecosystem is a ranked list of what vanilla lacks. Top downloads ≈ your feature roadmap.

---

## 21. UI/UX Summary (friction inventory)

- Health panel (body diagram), moodle stack, inventory dual-pane, crafting menus (B42 reworked into a searchable crafting UI), context menus, map UI, vehicle radial (one of the few radials).
- Chronic complaints: inventory micromanagement, opaque mechanics (nothing explains infection odds, nutrition, or muscle strain in-game), tutorial covers ~2% of systems, information lives on the wiki. **"The wiki is the real tutorial" is both charm and market ceiling.**

---

## 22. Explicit Design Gaps (the openings for a new game)

1. **No NPCs/human element** — 12+ years promised; single largest unmet demand.
2. **No missions/objectives layer** — annotated maps + challenge modes are the entire "content" surface; late game = empty checklist of self-set goals.
3. **Late-game emptiness** — TIS's answer is deeper crafting (B42); nobody is answering with *narrative/dynamic events/factions*.
4. **Onboarding cliff** — brutal opacity; huge churn of curious buyers.
5. **UI friction** — inventory/menus; most-modded surface.
6. **Vehicle feel** — functional but clunky.
7. **Combat monotony over hundreds of hours** — one optimal rhythm (shove/stomp, backpedal-swing); no enemy variety by design (pure Romero) — variety would need to come from *situations*, not monsters, unless lore diverges.
8. **No economy/trading/social systems** — nothing to spend surplus on; hoarding is its own reward.
9. **Multiplayer instability/droughts** around major builds.

---

## 23. Design Pillars Worth Stealing vs. Rethinking (brainstorm seed)

**Steal (identity-defining, low-risk):**
- Moodle-style readable simulation stack; per-body-part health; infection ambiguity window; endurance-economy melee; noise-as-threat-currency; utilities-shutoff phase transition; seasonal calendar pressure; permadeath + persistent world; sandbox sliders; annotated-map treasure hints; meta events (helicopter-class beats); environmental storytelling & diegetic media; negative-trait point economy.

**Rethink/differentiate (your thesis: missions + NPCs + modern UX):**
- Replace "empty late game + deeper crafting" with **faction/NPC-driven dynamic missions** (radiant scavenge/rescue/defend + handcrafted questlines + reputation/economy).
- Onboarding: diegetic guided first days (radio handler? Life-and-Living-style broadcast tutorialization is *already* the elegant in-fiction answer).
- Inventory/UX: modern bulk actions, radial quick-use, direct world prompts.
- Combat: keep stamina economy; add situational variety via NPC threats (bandits change stealth/noise/ranged math completely — human enemies with guns are the "enemy variety" Romero purism forbids).
- Scope guardrails: PZ's map density (thousands of hand-built interiors) is 12 years of content — a low-poly art style + smaller, denser map + mission layer is the honest competitive scope.

*End of reference document.*
