# ZombieShooter — Open Questions & Gaps Document
For Claude: use this to build a refined phase plan toward a playable beta
Compiled July 2026 — companion to ZombieShooter_Consolidated_Changes.md

Instructions for use: This document lists every unresolved/undecided area needed to take
the project from its current state to a feature-complete, playable beta. Items are grouped
by system, matching the structure of the Consolidated Changes doc. Use these gaps to propose
a phase plan, flag dependencies between systems, and identify which decisions block which
implementation work.

---

## 1. Core Vision & Scope

- What is the definitive one-sentence pitch/logline for the game?
- What is the target platform (PC only? Steam? Console later?)
- What is the target player count ceiling (confirmed 2-4 co-op — is this hard-locked or could it flex to 6+ later?)
- What is the target playtime for a single "run" — hours? days survived? Is there a win condition, or is it purely survive-as-long-as-possible?
- Is there a definitive "loss" state (permadeath only) or are there difficulty/save-scum options?
- What separates this game from Project Zomboid in the player's mind — what is the actual unique selling point once all systems are built?
- What is the monetization plan (premium one-time purchase, early access, free)?
- What is the target release window / is there a target date for a playable beta?

## 2. Narrative & Setting

- What are the actual named locations in the fictional Adirondacks-inspired region? (deferred from previous session)
- What is the outbreak's specific origin story, and how much of it is the player meant to uncover vs. never fully explain?
- Who are the key named NPCs/factions (if any), and are there any survivor NPCs at all, or is this a fully NPC-less zombie-only world?
- What is the actual mystery/plot of the investigation arc (P8)? What are the story beats, and what is the final revelation/ending?
- Is there an ending state at all, or is the investigation arc a repeatable/ambient narrative layer with no final resolution?
- What is the tone target — grounded/realistic horror, pulpy action-horror, or something else? (Useful for guiding VO, music, and UI tone.)
- Will there be any voice acting, or text-only narrative delivery (radio transcripts, notes, etc.)?

## 3. Camera & Control (mostly resolved — remaining gaps)

- Exact preset zoom range values (min/max distance) — what do these numbers need to be?
- Exact aim-cone tightening values for hip-fire vs. aimed (numeric spread values per weapon type)?
- Exact headshot-chance weighting differential between hip-fire and aimed?
- Controller support — is gamepad input a requirement for v1, or keyboard/mouse only?
- Are there any accessibility considerations (colorblind modes, remappable keys, subtitle/text-size options)?

## 4. Needs & Status Simulation (remaining gaps)

- Exact severity thresholds/stages for each need (e.g., how many hunger stages, what triggers each)?
- Exact health-drain rates for critical neglect states?
- Temperature system specifics: what determines cold/hot exposure (weather, indoor/outdoor, season, clothing insulation value)?
- Does clothing provide meaningful thermal insulation values, and is there a clothing-layering system?
- Should Panic, Stress, or Boredom ever be reconsidered for a post-v1 update, or permanently cut?
- Is there a sleep mechanic (in-game time skip while resting), and if so, are players vulnerable to zombie attacks while sleeping?

## 5. Health, Damage & Medical (remaining gaps)

- Full list of specific wound types beyond the general model (lacerations, punctures, burns, fractures) — which are in v1?
- Full progression/tiering of medical items (bandage tiers, disinfectant, splints, antibiotics for wound infection)?
- Is there a "field surgery"/amputation UI flow designed yet? What are the mechanical steps and requirements (tools, sterility, skill level)?
- What is the actual fatal timeline for bite infection (how many in-game hours/days from bite to death if untreated)?
- Does character death end the save permanently (permadeath), or can a new character continue in the same world?
- Are there status effects from illness unrelated to zombie infection (food poisoning, common cold, etc.)?

## 6. Combat (remaining gaps)

- Full weapon roster — what specific melee weapons and firearms are planned for v1 (exact list, not just categories)?
- Ammo types and crafting/finding balance — is ammo scarce by design, and what's the target ammo economy?
- Weapon attachment/modification system — is this planned at all, or are weapons static once found?
- Explosives — are grenades/molotovs/traps planned, and if so what's the design for their use?
- Are there ranged zombie threats (spitters, etc.) or is all zombie damage melee/contact-based?
- Multi-hit weapon rules — which weapon types can hit multiple zombies in one swing, and what's the arc/range?
- Dismemberment/gore system — is this planned, and does it affect zombie combat difficulty (e.g., removing a limb slows a zombie)?
- Vehicle combat — can vehicles be used to run over zombies, and does this have a designed risk/reward (damage to vehicle, noise generated)?

## 7. Inventory & Loot (remaining gaps)

- Full weight budget numbers — what's the base carry weight, and how much do bags/backpacks add?
- Exact rarity tiers and the finite world-count pool per tier — what are the actual numbers per item type?
- Crafting system — what's the full crafting recipe list planned for v1, and what stations/tools are required?
- Is there a base-building/fortification system (barricading windows, building walls), and if so what's the scope?
- Food spoilage — do perishable foods degrade over time, and is there refrigeration/preservation as a mechanic?
- Are there container lock/key mechanics (locked doors, safes) requiring lockpicking or key-finding?

## 8. Zombies & AI (remaining gaps)

- Full zombie type roster for v1 launch (exact count and named types, not just "standard + Crawlers")?
- What determines zombie spawn density and distribution across the map — fixed population, or dynamic based on noise/events?
- Do zombies respawn/repopulate over time in cleared areas, or do cleared areas stay permanently cleared?
- Is there a day/night behavior difference for zombies (faster at night, different sensory range, etc.)?
- What is the actual chosen solution for horde/large-group performance (the deferred decision from the Multiplayer section)?
- Are there any non-zombie hostile threats (wildlife, hostile survivors/NPCs)?

## 9. World & Persistence (remaining gaps)

- What is the actual map size/scope target (number of towns, approximate square km, or comparable reference)?
- Named locations (deferred — see Narrative section above, duplicated dependency)
- Is there a season/weather cycle, and if so what's the full scope (rain, snow, fog, storms)?
- Is there a day/night cycle length, and does it match real-time or run on a compressed clock?
- Vehicle system — what's the full scope (fuel, repair, damage model, finding/hotwiring)?
- Is there a farming/agriculture system planned, and if so what's the scope?
- Animal/wildlife system — hunting, livestock, or ambient wildlife only?

## 10. Dynamic Events & Investigation Arc (remaining gaps)

- Full roster of event types planned for v1 (flyovers, convoys, alarms, supply drops, etc. — exact list)?
- Ambient event locatability decision (deferred from previous session) — resolve this
- What is the actual clue/evidence system UI — how are clues tracked, stored, and referenced by the player (journal, map pins, etc.)?
- Are dynamic events balanced/tuned differently based on how many days have passed (escalating difficulty over time)?

## 11. Skills & Progression (remaining gaps)

- Exact XP curve/thresholds for each of the six v1 skills — how much XP per level, and what actions grant XP?
- Low-friction practice loop designs (deferred from previous session) — resolve this for each skill
- Are there any perks/unlockable abilities tied to reaching skill milestones, beyond passive stat improvements?
- Is there a skill cap that can be exceeded with rare items/traits, or is 1-5 a hard ceiling for everyone?

## 12. Character Creation & Onboarding (remaining gaps — largest gap area)

- Full background roster (deferred from previous session) — resolve this
- Background tradeoffs (deferred from previous session) — resolve this
- Radio tutorial arc pacing (deferred from previous session) — resolve this
- Character appearance customization — is there a visual character creator, or are backgrounds tied to fixed character models?
- Is there a name/gender/appearance choice independent of background selection?
- What is the actual new-game setup flow (world seed selection, difficulty options, starting location choice)?

## 13. Multiplayer & Persistence Architecture (remaining gaps)

- Is dedicated server hosting planned, or listen-server (host-and-play) only for v1?
- What is the plan for players joining a session already in progress mid-game (late-join flow)?
- What happens to a player's character/items if they disconnect — does the character persist in the world, or vanish until reconnect?
- Is there voice chat integration, or reliance on external tools (Discord)?
- Cross-platform play — is this a goal, or single-platform only (PC)?

## 14. Production & Technical Foundation (remaining gaps)

- Performance budget numbers (deferred from previous session) — resolve this
- Minimum hardware target spec (deferred from previous session) — resolve this
- What is the actual current team size — solo dev only, or are collaborators/contractors planned for art, sound, etc.?
- Is there a budget for paid assets (marketplace assets, sound libraries, contracted art) or is everything self-made/free-asset-based?
- What engine version is locked in, and is there a policy on engine version upgrades mid-development?
- Is there a version control/build pipeline already established (Git workflow, branching strategy)?
- What is the testing/QA plan beyond solo playtesting — closed beta group, public beta, etc.?

## 15. Art, Audio & UI (not yet covered in any previous session — full gap)

- What is the target art style (realistic, stylized, low-poly) and is there a mood board/reference set?
- Character model source — custom modeled, marketplace assets, or a mix?
- Full UI/HUD design — what's actually on-screen at all times (health bar, needs moodles, minimap, ammo counter)?
- Sound design scope — ambient audio, zombie vocalizations, weapon sound design, music score direction?
- Is there a full UI wireframe/mockup for menus (main menu, inventory screen, character sheet, map screen)?
- Localization — is the game English-only for v1, or are other languages planned?

## 16. Marketing & Release (not yet covered — full gap)

- Is there a planned marketing/community-building timeline (Steam page, trailers, devlogs)?
- Is a demo planned before full beta/launch?
- What is the pricing strategy?
- Is there a planned Early Access period, or a single full-release launch?

---

## Priority Flag for Claude
When building the revised phase plan, please explicitly flag which of the above items are:
(a) BLOCKING — must be answered before implementation of a related system can start,
(b) SEQUENCEABLE — can be answered in parallel with early implementation without blocking it,
(c) POLISH/LATE — safe to leave until beta polish pass or post-beta.

This will help prioritize which open questions need to be brought back to the design conversation
soonest versus which can wait.
