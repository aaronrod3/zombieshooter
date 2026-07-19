# Dev Markup — "New Shooter Project Planning Doc"

> Verbatim transcription of the dev's own section-by-section markup pass over `ProjectZomboid_DesignReference.md`, supplied as a PDF on 2026-07-18. Archived here for the same reason the PZ reference itself is archived — so a future session with zero memory of this conversation can read the actual source instead of trusting a summary. `GameDevPlan.md` is the synthesized plan; this file is the raw input that shaped it.

**Header notes on the original doc:**
- "Fully replace CoreLoopPlan once GameDevPlan is finished."
- "Rename GameDevPlan to: ___" (left blank by the dev — pending decision, see `GameDevPlan.md` open questions).

---

## 1. Core Identity & Design Philosophy
- Tagline/thesis ("This is how you died" / inevitable death, no win condition): **REMOVE.**
- No win condition → **NOTE:** There are missions/quests to complete. Players discover these by finding items and investigating what they can be used for, or finding clues in notes/documents around the map. Create a loop of surviving, but also an end goal to survival.
- Simulation-first philosophy → **NOTES:**
  - For eating/drinking: not meant to be a main goal/struggle. Food and water last longer than most survival games. Player doesn't automatically start dying — hunger/thirst instead impact stamina, healing, aim accuracy, attack recovery, etc.
  - Wounds can be body-part specific and affect different attributes — leg wounds affect mobility/speed, arm wounds affect attack speed and reload times, for example.
  - Weather will have a strong effect; players must adjust to survive.
- Slow-burn horror → **NOTE:** Zombies need to be programmed to be CPU efficient, enabling massive hordes. Need a realistic way to keep introducing more zombies throughout the game so the player can't just clear them all.
- Player-authored narrative → **NOTE:** See the missions/quests note above. Other emergent drama is still welcome and brings immersion/tension to everyday decisions.
- Permadeath → **NOTE:** Players develop a character that, when dead, loses all progress — keeps play careful. There do need to be "second chances," such as reviving or cutting off infected body parts.
- Difficulty as identity: no note added.

## 2. Core Gameplay Loop
**NOTE:** Introduce a sleep cycle similar to Minecraft — if players are safe within a certain radius from enemies, they can sleep; if all players sleep at the same time, time lapses for the amount the player sets after entering bed.

## 3. Perspective, Camera & Controls
**NOTE:** Simplify controls; need playability on consoles later on.

## 4. Character Creation
### 4.1 Professions
**NOTE:** Like this setup, but want players to also start at a specific spawn point tied to their chosen profession. Can create co-op fun — players spawning in separate locations and having to find each other. Need to simplify/change the skills, but like the system overall.

### 4.2 Traits
**NOTE:** Instead of traits, certain actions just affect your stats differently.
**NOTE:** Need a skill points system — the more you do certain actions, the more proficient you get, which helps later gameplay.

### 4.3 Appearance & identity
(Notes above apply here too — no separate note.)

## 5. Stats, Needs & the Moodle System
No additional note beyond §1's hunger/thirst note (non-lethal-primary, performance-debuff-driven).

## 6. Skills & Progression
**NOTE:** Narrow down later by asking questions and narrowing style.

## 7. Combat
No notes added (7.1–7.4 taken as reference baseline pending later Q&A).

## 8. Zombies
No notes beyond §1's CPU-efficiency / can't-fully-clear requirement.

## 9. Health & Medical System
No note beyond §1's amputation/"second chances" note — read as directly applicable here.

## 10. World, Map & Loot
### 10.1 Setting
**NOTE:** Change location to a fictional name, but base it on the Upstate NY Adirondacks area — keep all the same location *types* though (small town, dense town, quiet suburb, mall analog, military-adjacent area, farmland belt, etc.).

### 10.2 Loot design
**NOTE:** Simple container-based loot; items take up a single spot. Items like weapons can only be equipped, so items need a category to place them into appropriate spots. Some items can only be carried (not equipped), for instance. Certain areas have good loot, others bad, others in-between — slight randomization plus common-sense placement. Also have a system where a limited total amount of certain items exists, making them rarer.

### 10.3 World persistence & erosion
No additional note.

## 11. Time, Weather, Seasons
**NOTE:** Again, change location to the Upstate NY Adirondacks area (reaffirms §10.1).

## 12. Inventory, Encumbrance & Item Systems
No separate note (covered by §10.2's category/equip-slot note).

## 13. Crafting, Building & Resource Chains
No notes added.

## 14. Food Production & Survivalist Systems
No notes added.

## 15. Vehicles
No notes added.

## 17. Meta Events & Scripted Beats
**NOTE:** Create more meta events.

## 18. Modes, Difficulty & Meta Structure
No notes added.

## 19. Multiplayer & Social
**NOTE:** Build NPCs that occasionally roam through, can encounter and kill zombies and players alike. Never friendly.

## 20. Modding & Community
No notes added.

## 21. UI/UX Summary
**NOTE:** Simplify inventory; have an easy stat system to show what items and actions do to the player. Player knowledge is mainly built on common sense and playing the game.

## 22. Explicit Design Gaps
**NOTE:** Read other notes to find answers to some of these.
Add investigative quests to find out how the infection spread. Find a cure.

## 23. Design Pillars Worth Stealing vs. Rethinking
No additional notes (final section, no markup).

---

*End of transcription.*
