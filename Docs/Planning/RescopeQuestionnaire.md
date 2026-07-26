# Design Intent & Rescope Questionnaire

> **Status: input document, not a plan.** Written 2026-07-25 after a full read-through of `Docs/GameDevPlan.md` and every file in `Docs/Beta/` (all 13 phase files, the master plan, the open-questions log, the revision register, the beta-ready checklists) plus `Docs/Planning/`. Nothing in `Docs/Beta/` gets rewritten until you've answered what's useful here and sent it back — see [How to use this](#how-to-use-this) below.

## Why this document exists

You asked for a review of the current plan because scope felt too large and too much was happening per phase with too little of your input along the way. Having read all of it, that read is accurate, and it's worth showing you the specific evidence rather than just agreeing in the abstract:

- **The full plan is B0–B12, an estimated 195–250 dev-sessions, ~10–14 months part-time — before public beta even opens.**
- **B0, the phase you're in right now, bundles about a dozen substantial system changes into one 14–18 session block**: the item-instance data-model refactor, a full camera/aim overhaul, two new survival needs, a two-tier infection model, amputation choreography, zombie AI fixes, death/world-continuity rules, weapon jamming, a brand-new "downed zombie" AI state, and a new offhand-item system. Almost all of it is only checked at the very end (`B0-PT6`), with one mid-phase gate (the camera sign-off).
- **B4 (world content) is 45–60 sessions on its own** — bigger than B0 through B3 combined — and bundles three brand-new subsystems (multi-level buildings, darkness/light, procedural basements) in with the actual content-building work. The plan's own risk audit already calls this the phase most likely to overrun.
- **Of the ~70 open design questions logged in `90_OpenQuestions.md`, most were resolved by an AI-authored recommendation being adopted as "CONFIRMED" in the same breath** — exact aim-cone tightness, the temperature system's scope, the named character backgrounds (Park Ranger, Paramedic, etc.), the weapon roster, weight budgets, and more. A real minority have a dated decision that's clearly yours (PC-only launch, gamepad deferred to B9, keep `BP_ZombieAIController`, the weapon socket fix). Most don't.
- **The document those AI recommendations trace back to, `ZombieShooter_Consolidated_Changes.md`, doesn't exist anywhere in this repository** — it's referenced dozens of times across `Docs/Beta/` as the source of non-negotiable decisions, but I can't find it. More on this below (§1.1).
- **Some of the largest systems were built or drafted in long unsupervised sessions** — the entire P6 inventory/loot backbone, the UI plan, the inventory/loadout redesign plan — before you'd seen any of it. The verification pass you're in the middle of right now is, for a lot of this code, the first time it's ever actually run.

None of that means the work is bad — the underlying architecture is consistently solid, and some of the phasing is already good (the camera sign-off gate, deferring horde-coordination design until real profiling data exists, the B0 verification-before-refactor sequencing). The problem is concentration: too many decisions and too much unverified work piling up before you get a look, and too much creative territory — the actual plot, the tone, the names of things — getting filled in by an AI's best guess instead of your voice.

This document is how we fix that at the root instead of just re-shuffling the same content into new boxes. **Nothing gets built from this — it's pure input.** Once you send it back, I'll rebuild `Docs/Beta/` around your actual answers: smaller phases, a real checkpoint after each one, and your creative calls in place of guessed ones.

## How to use this

- **Answer what's useful, skip the rest.** Every question has a blank. Write one word, write "you decide," write nothing and leave it — all valid. This is a starting point, not an exam.
- **You don't have to finish it in one sitting.** Send back a partial version and we'll keep going, or talk through the rest live instead of writing it out.
- **Format note:** each system section below is written like a **feature design brief** — the format a design lead fills out before a system enters production: what it's for, the smallest version worth building, what's explicitly not wanted, and how we'll know it works. The quick-tag tables reuse **your own project's existing scope vocabulary** — `GameDevPlan.md` §3's KEEP / SIMPLIFY / REPLACE / CUT table — just with REPLACE swapped for **DEFER** ("good idea, not now").
- **B0-T1 (the verification pass you're running right now) doesn't need to wait for this.** It's already small, already testable, already exactly the model we want more of — keep going. Everything here is about B0-T2 onward and the shape of B1–B12.
- ⭐ marks the handful of questions in each section that matter most if you're short on time.

---

## Table of contents

- [Part 0 — How we work together](#part-0--how-we-work-together)
- [Part 1 — Foundational](#part-1--foundational)
- [Part 2 — Core systems](#part-2--core-systems)
  1. [Camera, Aiming & Combat Feel](#1-camera-aiming--combat-feel)
  2. [Survival Needs](#2-survival-needs)
  3. [Health, Wounds, Infection & Amputation](#3-health-wounds-infection--amputation)
  4. [Item-Instance Refactor & Inventory Architecture](#4-item-instance-refactor--inventory-architecture)
  5. [Loadout, Weapons & Melee](#5-loadout-weapons--melee)
  6. [Zombie AI & Horde Behavior](#6-zombie-ai--horde-behavior)
  7. [World Content — Region, Multi-Level, Darkness, Basements, Weather](#7-world-content--region-multi-level-darkness-basements-weather)
  8. [Persistence & Save](#8-persistence--save)
  9. [UI/UX](#9-uiux)
  10. [Events, Radio & the Investigation Arc](#10-events-radio--the-investigation-arc)
  11. [Progression, Skills & Backgrounds](#11-progression-skills--backgrounds)
- [Part 3 — Production & release (quick ambition check only)](#part-3--production--release-quick-ambition-check-only)
- [Part 4 — What do you want to see working first](#part-4--what-do-you-want-to-see-working-first)
- [Part 5 — Anything else](#part-5--anything-else)

---

## Part 0 — How we work together

This part matters more than any individual feature answer below — it's the process fix, not a content fix.

**1. Checkpoint size.** After how much new work do you want to stop and actually see/test it, before I build anything further on top?
 - A. After each individual feature or bug fix, even small ones
 - B. After a small cluster of related changes — a few sessions' worth
 - C. Longer stretches are fine as long as there's a real checklist I work through and you spot-check
 - D. Other: ___
 **Your answer:** A

**2. Decision-making.** For the many open "which way should this work" questions — exact numbers, names, mechanics — how involved do you want to be? ⭐
 - A. Ask me for almost everything, even small stuff
 - B. Ask me for anything that shapes how a system feels or plays; pick sensible defaults for pure implementation details and tell me what you picked
 - C. Use broad judgment, just flag anything expensive to reverse later
 - D. Other: ___
 **Your answer:** D - I will trust your judgment on most things, but please ask me before implementation for specifics that i should have input on,then procedd with steps after.

**3. Unsupervised / long autonomous sessions.** Several big pieces so far (P6's inventory/loot backbone, the UI plan, the inventory/loadout redesign plan) were built or written unsupervised, before you'd seen any of it. Going forward: ⭐
 - A. Don't do large unsupervised work anymore — keep sessions small enough that you're present or reviewing before more gets built on top
 - B. Fine for planning docs/proposals to happen unsupervised, but not a lot of shipped code
 - C. Still fine occasionally, capped in size, always verified before anything else builds on top of it
 - D. Other: ___
 **Your answer:** D - long sessions are fine, but i want you to ask me for my input on the specifics before proceeding, and then compile a list of steps you need me to complete on my end.

**4. What "tested" means from here on.**
 - A. You personally PIE-test everything before I move on
 - B. A written test script/runbook (like `Docs/Testing/P5_P6_CharacterSetupVerification.md`, which B0 is using right now) that you run through, and I don't claim something works until you've confirmed it
 - C. Trust my report for small stuff, but you test anything touching movement/camera/multiplayer/save data yourself
 - D. Other: ___
 **Your answer:** B

**5. Phase size going forward.** Given 1–4 above: are short phases (a handful of sessions, one clear testable outcome, then stop) the right shape, or are you fine with longer phases as long as there are several real checkpoints inside them?
 **Your answer:** Longer is fine, as long as there are checkpoints, or compiling a list of steps for me to complete, as long as not too many steps are done that are too dependent on each other, so if an earlier one doesn't work its not a cascading issue.

---

## Part 1 — Foundational

**1.1 — The missing source document.** `ZombieShooter_Consolidated_Changes.md` is cited constantly throughout `Docs/Beta/` as the source of "CONFIRMED, non-negotiable" decisions, but it isn't in the repo anywhere I can find. Do you know where it came from (another AI tool? your own notes elsewhere?), do you still have it, and — more importantly — do you still stand behind everything currently attributed to it? Or should everything sourced from it be treated as open again rather than settled? ⭐
 **Your answer:** It was used to compile plans for the beta planning. I still have it if I need to upload it at anytime let me know.

**1.2 — Ambition level, right now.** The full plan targets a ~10–14 month, 195–250 session public Early Access beta: investigation arc, multi-level buildings, basements, weather/temperature, ~12 named locations, 6 backgrounds, dynamic events, full audio pass, accessibility, store page. Does that match what you actually want to be working toward right now? ⭐
 - A. Yes, that's still the real goal, just needs better phasing
 - B. Too far out — I want a smaller, nearer-term playable milestone first, then reassess from there
 - C. Don't know yet — help me figure out a smaller first target
 **Your answer:** B

**1.3 — What does "real progress" look like in the next month or two?** Not the beta — the next concrete thing that would feel good to actually reach and play. (Example shape of an answer: "I can walk around a graybox map with a friend, loot a house, fight a few zombies, and survive a day/night cycle.") ⭐
 **Your answer:** I want to have a playable game, that is going to still need tuning/refining/polishing, but that way we know the features work.

**1.4 — Three flagged contradictions need a real call** (currently sitting on an AI recommendation, not a confirmed decision):
 - **CR-01 — Skill roster.** One source doc says six flat skills (Melee, Firearms, Fitness, Medicine, Carpentry, Survival). `GameDevPlan.md` §3.1 says a longer, different list (Strength/Stamina/Sneak/Sprint as passive attributes, plus per-weapon-class Melee, Maintenance, Aiming, Reloading, First Aid) and explicitly supersedes the six-skill list. Which do you actually want? **Your answer:** Use the GameDevPlan.md list (longer one)
 - **CR-02 — Vehicles.** Scope contract says CUT for v1; other places assume they exist (a container category, an auto-zoom trigger). Genuinely cut, or do you want them in? (They're a large system on their own and change how big the map should be.) **Your answer:** We will implement vehicles later on in development, should be ready for beta. 
 - **CR-10 — Does being tired make the player worse at spotting zombies (vignette/muffled audio), or does it make the player harder for zombies to spot (a stealth upside to being exhausted)?** Current assumption is the former. **Your answer:** former

**1.5 — Cadence reality check.** Plan assumes solo dev + AI assist, ~15–20 hrs/week part-time. Still accurate?
 **Your answer:** yes

---

## Part 2 — Core systems

### 1. Camera, Aiming & Combat Feel

**Current plan:** Permanent switch to a fixed top-down camera; the over-shoulder/third-person view — which was explicitly kept in earlier planning as "the fallback if top-down doesn't feel right" — gets deleted outright once a one-time playtest checkpoint (`B0-PT2`) passes. Combat accuracy becomes entirely about a hip-fire-vs-aimed spread cone (tight cones recommended) feeding into the 4-zone hit model, since a stress/panic mechanic was cut, making this the *only* source of combat difficulty.

**Your call:**

| Item | Current plan | Your call |
|---|---|---|
| Over-the-shoulder view | Delete permanently once `B0-PT2` passes | KEEP as permanent option / KEEP as fallback only / CUT now: CUT now |
| Aim-cone tightness | Tight cones, aiming clearly rewarded (pistol ~8°→2°, rifle ~5°→1°) | tighter / about right / looser and more chaotic: about right |
| Headshot bonus (aimed vs. hip) | ~5% hip / ~25% aimed | KEEP / SIMPLIFY / DEFER / CUT: KEEP |

**Open questions:**
1. ⭐ What does "feels good" actually mean to you here, beyond Door Kickers 2's camera angle? Any other games whose *combat feel* (not camera) you'd point to? **Your answer:** PZ has a good camera feel, but want it to be slightly different to avoid copying, which is why i went with DK2 style to change it up.
2. What would make you say "no, this isn't working, go back to the drawing board" on the camera during `B0-PT2`? Knowing the failure condition in advance makes that checkpoint real instead of a formality. **Your answer:** ___

---

### 2. Survival Needs

**Current plan:** Started as 6 needs (Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness). An outside source doc pushed it to 8 by adding **Wet** and **Temperature**, and the plan's own risk audit flags this as its single **highest scope-risk item** — done "properly," temperature is a weather × indoor/outdoor × clothing × wetness × time-of-day simulation. The current proposal scopes it way down (one body-temp number, four inputs, no per-limb thermal, no clothing layers) specifically to survive that risk.

**Your call:**

| Item | Current plan | Your call |
|---|---|---|
| Wet (rain/water exposure) | New, adds to footstep noise | KEEP / SIMPLIFY / DEFER / CUT: keep |
| Temperature (hot/cold, hypothermia) | New, scoped-down single-scalar model | KEEP / SIMPLIFY / DEFER / CUT: keep |
| Clothing insulation values | One flat number per item, no layering | KEEP / SIMPLIFY / DEFER / CUT: keep |

**Open questions:**
1. ⭐ Does temperature/weather survival actually matter to your vision for this game, or did it creep in from the outside source doc rather than something you asked for? **Your answer:** was part of the plan
2. Everything here is governed by "needs degrade performance before they kill" — starvation stays possible but far downstream. Does that still match what you want, or should neglect be more punishing than that? **Your answer:** i want the player to worry about survival aspects, but i dont want it to become the main thing. too many games want the player to eat/drink, but then it turns into the player being frustrated with eating/drinking too much and losing due to running out of resources.

---

### 3. Health, Wounds, Infection & Amputation

**Current plan:** 4 body zones (Head/Torso/Arms/Legs). Two separate infection tracks that must look **identical** in the UI — ordinary wound infection (curable, never fatal alone) and bite infection (not curable by disinfecting, has a fatal timeline) — so the player genuinely cannot tell which one they have. Emergency amputation stops a fatal bite infection at the cost of permanently losing the limb, plus a blackout with an in-game time skip (a real risk window if you're not somewhere safe when you do it).

This reads as one of the project's actual identity pillars (`GameDevPlan.md` calls it out by name), not filler, so it's probably closer to right than most sections here — mainly flagging it to confirm rather than to re-litigate.

**Open questions:**
1. ⭐ Confirm the ambiguity is exactly what you want: any UI element that names which infection tier you have — even a debug label — destroys the intended horror beat ("is this a cold or am I dying?"). Worth being deliberate about, since it's easy for a future UI pass to break by accident. **Your answer:** plainly show the player if they are bitten and infected.
2. Bite-to-death timeline is currently proposed at ~3 in-game days. Feels right, too short, too long? **Your answer:** feels right
3. Multi-day fracture healing, a rare-but-severe "critical head bleed" outcome — still want both? **Your answer:** yes

---

### 4. Item-Instance Refactor & Inventory Architecture

**Current plan:** Right now, "an item" means three different, disconnected things in the code depending on where you look — a shared config asset, a live weapon actor, or a bare count in a slot — which is why durability resets when you re-equip a weapon and ammo isn't a real, lootable thing yet. The fix (`Docs/Planning/InventoryLoadoutEquipping_Plan.md`) gives every carried item a stable ID that survives being looted, equipped, dropped, and re-picked-up. This is proposed as one 5–6 session block touching five core files, bundling in ammo-as-a-real-item, four carry-location categories (pockets/bag/world/vehicle-reserved), randomized loot condition, and one-handed/two-handed weapon rules all at once.

This is mostly an engineering correctness question, not a feel question — but it's a good example of the bundling problem: the design doc itself already lays out an incremental 7-step order that the current B0 plan doesn't actually preserve as separate checkpoints.

**Your call:**

| Bundled addition | Your call |
|---|---|
| Ammo becomes a real, lootable/shareable inventory item | KEEP / SIMPLIFY / DEFER / CUT: keep |
| Four carry-location categories (on-person vs. bag vs. world vs. vehicle-reserved) | KEEP / SIMPLIFY / DEFER / CUT: keep |
| Randomized loot condition (two "rare" finds can differ) | KEEP / SIMPLIFY / DEFER / CUT: keep |
| One-handed/two-handed weapon rules | KEEP / SIMPLIFY / DEFER / CUT: keep |

**Open questions:**
1. ⭐ Do you want this refactor done as one uninterrupted block (current plan, ~5–6 sessions with nothing else landing until it's done), or split into the smaller, independently-shippable steps the design doc itself already proposes, each with its own test pass? **Your answer:** independent steps for testing
2. Do you actually want weapon attachments with real stat effects (scopes/silencers that change numbers, not just look different) at some point, or is the cosmetic-only version already built enough? The design doc recommends against building the stat-changing version at all. **Your answer:** scopes increase accuracy, sliencers decrease sound, etc

---

### 5. Loadout, Weapons & Melee

**Current plan:** 9-slot hotbar, a `SecondaryHand` offhand slot (mainly for a flashlight), weapon jamming (revolvers/bolt-actions immune), a "downed zombie" AI state so a standing swing can't cheaply finish one — that needs a deliberate stomp instead. Melee weapon *display* is still an unresolved, content-blocking question — right now a crowbar temporarily uses the rifle's full-body holding pose, which looks wrong, purely to unblock testing.

**Your call:**

| Item | Current plan | Your call |
|---|---|---|
| Firearm roster | Revolver, pistol, shotgun, bolt-action (4) | right size / want more / want fewer: I will provide a full list once basic features are set |
| Melee roster | 4–6 archetypes, one per feel-category (blunt/edged/improvised/heavy) | right size / want more / want fewer: right |
| Jamming | Condition-based, with audio/UI cue | KEEP / SIMPLIFY / DEFER / CUT: keep |
| Downed-zombie + stomp finisher | New AI state + dedicated mechanic | KEEP / SIMPLIFY / DEFER / CUT: keep, find alterntives to make sure this isn't copying PZ |

**Open question:**
1. ⭐ **Melee weapon display** — a static prop on a hand socket (works for any one-handed weapon, cheap, slightly generic) vs. a unique holding pose per weapon (looks best, multiplies animation work per weapon) vs. something else? This one's been deferred three times and needs an actual answer to stop blocking content. **Your answer:** specific poses per weapon type, rifle/shotugn/lmg all the same, pistol has its own, melee has its own, etc

---

### 6. Zombie AI & Horde Behavior

**Current plan:** Good news first — the native AI migration is done and, as of this session, confirmed actually working in PIE (wander/investigate/chase all behave correctly). This is one of the few systems in the whole plan that's gone through exactly the tight build→test→fix loop you're asking for more of. What's still open: PZ-style "crowd following" (zombies drift toward each other's activity, which is most of how real hordes form) and the technical approach for handling 100+ zombies performantly are both deliberately deferred until there's real content and real performance numbers to design against — which is itself good phasing, not a gap.

**Open questions:**
1. ⭐ What do you actually want zombies to *feel* like? (PZ-style dumb-but-relentless shamblers that respond convincingly to noise, something faster/smarter, something else entirely?) **Your answer:** PZ style, but newer(fresh) zombies are faster, zombies degrade slowly and slowdown, don't do as much damage
2. How important is a genuine, visually large horde (100+ zombies moving together) to your vision, versus a smaller number that's easier to make performant, well-tested, and correctly behaved? **Your answer:** very important

---

### 7. World Content — Region, Multi-Level, Darkness, Basements, Weather

**Current plan:** This is the single largest phase in the entire project — **45–60 sessions on its own**, more than B0 through B3 combined — and it's not just content volume. It bundles in three brand-new systems that have never been built before: multi-level buildings (with aim/AI/sound all needing to respect which floor you're on), a darkness mechanic where dark interiors genuinely require a carried light source, and randomized basement layouts picked from an authored pool. The plan's own audit already names this the phase most likely to overrun a solo part-time schedule.

**This is probably the highest-value section in the whole document** — getting real signal here does more to fix the "too much at once" problem than anything else, because it's the biggest phase by a wide margin.

**Your call:**

| Item | Current plan | Your call |
|---|---|---|
| Multi-level buildings (2nd/3rd floors) | Full system: floor detection, per-floor aim resolution, AI pathing, noise attenuation | KEEP / SIMPLIFY (e.g. rare, 1–2 story cap) / DEFER / CUT: keep |
| Basements, randomized layout | Authored pool, weighted pick, most simple / a few elaborate | KEEP / SIMPLIFY (fewer, all simple) / DEFER / CUT: will stick to a fixed map for now |
| Darkness requires a light source | First-class mechanic, ties to flashlight | KEEP / SIMPLIFY (cosmetic only) / DEFER / CUT: keep |
| Weather as a real mechanic (not just visual) | Rain/fog/snow drive Wet + Temperature + noise masking | KEEP / SIMPLIFY (atmosphere-only) / DEFER / CUT: keep |
| Map scale | ~1×1 km, one dense town + rural fringe | bigger / about right / smaller-and-denser: bigger, build in phases |

**Open questions:**
1. ⭐ **If you had to cut exactly one of {multi-level buildings, basements, weather-as-a-mechanic, temperature} to meaningfully shrink this phase, which would you keep and which would go?** Forcing a real tradeoff here is more useful than asking "do you want it" about each in isolation, since the honest answer to that is almost always yes. **Your answer:** don't see the need to cut, but can work through them in phases
2. Named locations (a town center, hospital, sheriff's station, hardware store, school, ranger station, etc.) still need actual names and flavor — is this something you want to write yourself, brainstorm together live, or are you fine with a first draft from me to react to? **Your answer:** generic names at first, will change later, worried about mechanics and features first

---

### 8. Persistence & Save

**Current plan:** Mostly a correctness/infrastructure question rather than a feel question — layered saves (frequent character-state, periodic full-world, chunk-on-unload, save-on-quit), rotating backups so a corrupted save doesn't lose a world. One real design question buried in here: an old backlog note says **solo character death should end that entire world**, not just respawn a new character into it, while co-op continues as long as one player survives.

**Open questions:**
1. ⭐ Does "solo death ends the whole world, start a new one" still match what you want, or is that too harsh? The alternative is a single continuous world regardless of player count. **Your answer:** start a new character on death, but world persists, player can find loot and safe house again.
2. Multiple save slots (several worlds at once) vs. one world per install — preference? **Your answer:** world saves, but is overwritten so player cant load an old save to fix a mistake

---

### 9. UI/UX

**Current plan:** Needs, wounds, hotbar, and the entire inventory are currently invisible in-game — this phase is the instrument panel for everything already built, not a "nice to have." Recommended direction: a single scrollable inventory list (not a Project-Zomboid-style dual-pane drag grid, which the game's own design reference calls out as PZ's most complained-about surface), per-item "what does this actually do" previews on hover, and a UI input mode that never pauses the game.

**Open questions:**
1. Confirm the single-scrollable-list direction over a grid — yes, or did you picture something else? **Your answer:** i will design this later
2. Any UI you've used (any game or app) whose *feel* you'd want this to borrow from? **Your answer:** I like the idea of separate containers for inventory, and equipment slots that the player drags items into to assign

---

### 10. Events, Radio & the Investigation Arc

**Current plan:** This is called out in your own design doc as the actual differentiator from Project Zomboid — dynamic world events (flyovers, alarms, convoys, horde migrations) plus a discoverable mystery about the outbreak's origin, delivered through radio and environmental storytelling, with an optional (never forced) capstone ending. **The actual plot — origin, story beats, what's knowable, the final revelation — currently exists only as an AI-authored placeholder shape** ("something happened → someone knew in advance → a response was attempted and failed"), never a real answer from you. This is flagged in the plan itself as its largest single content dependency.

**Open questions:**
1. ⭐ Do you have your own idea for what actually happened / what the investigation uncovers, even roughly? Or would you rather brainstorm this together when we actually get there, rather than guess now? Either is fine — just tell me which. **Your answer:** still planning, skip for now
2. Tone — grounded/restrained horror (current assumption) vs. something pulpier/more action-horror vs. something else? **Your answer:** 
3. Roughly how many distinct event types feel right for a full playthrough — a handful (~6, risk of feeling scripted) or a wider variety (~10+, more authoring cost)? **Your answer:** ___

---

### 11. Progression, Skills & Backgrounds

**Current plan:** Learn-by-doing skills (no point-buy at creation) — attributes like Strength/Stamina grow from play, combat skills like per-weapon-class Melee, Aiming, Reloading, Maintenance, First Aid level 1–5 from use. Six starting "backgrounds" (Park Ranger, Paramedic, Mechanic, Sheriff's Deputy, Line Cook, Hunter) were drafted by an AI as flavor examples, tied to Adirondacks-setting starting locations — never actually confirmed by you. Blocked on **CR-01** above (which skill list is real).

**Open questions:**
1. Do the six drafted backgrounds resonate as a starting point, or do you want to write your own list/theme? **Your answer:** will compile a full list later, not important for specifics, but need ability to create and assign starting values to the starting types
2. Should backgrounds be purely additive (safe, no wrong choices) or carry a real tradeoff (e.g., better-armed background starts in a more dangerous location)? **Your answer:** carry a real tradeoff

---

## Part 3 — Production & release (quick ambition check only)

These are all genuinely far off, and asking for real decisions on them now would repeat the exact "too much at once" problem this whole exercise exists to fix. Just flag if your ambition differs from the current draft assumption — we'll run a focused, short version of this same questionnaire again when each of these actually comes up.

- **Audio.** Assumption: UE's built-in audio tools (no paid middleware), sparse/event-driven music rather than a continuous score. Different ambition? **Your answer:** sounds good
- **Accessibility & settings.** Assumption: full remapping, gamepad support (built but not verified until late), colorblind modes, difficulty presets. Different ambition? **Your answer:** sounds good
- **Multiplayer hardening & release engineering.** Assumption: 2–4 players, listen-server only (no dedicated servers), Steam networking so testers don't need to port-forward. Still matches? **Your answer:** 4+ players, option for dedicated servers if players want to pay for it, but primarily listen-server
- **Art direction & pipeline.** Open budget question: is there money for a marketplace environment kit / sound library, or is this free-assets-and-Blender only? This changes a lot downstream. **Your answer:** mostly free, cheaper assets, want Door Kickers 2 level of detail so I can create assets myself later.
- **Performance & minimum spec.** Assumption target: roughly a GTX 1060 / RX 580 class machine, 16GB RAM, ~150 zombies on screen. Do you personally have (or care about supporting) hardware below that? **Your answer:** want to keep the game light
- **Business & release.** Pricing, Early Access vs. single launch, store presence — genuinely not needed yet. Anything you already know you want here? **Your answer:** probably around the 9.99 range to encourage people to buy it, early access for sure, on steam only at first.

---

## Part 4 — What do you want to see working first?

Independent of whatever order makes the most *engineering* sense, what do you most want to actually experience playable soonest? Pick/rank as many as you want, or name something not on this list:

- [ ] Full solo loop: needs, day/night, one zombie fight, feels tense and complete even in graybox
- [ ] Two people playing together (even on a placeholder map) — co-op just working and feeling good
- [ ] One building, fully realized (lit, lootable, a zombie encounter in it) — a taste of the real art direction
- [ ] The scavenge loop: loot a house under threat, haul it home, feel the weight decision
- [ ] A weapon in your hands feeling genuinely good to fire/swing
- [ ] The amputation/infection horror beat, actually experienced once
- [ ] The first hint of the investigation arc (one clue, one radio broadcast)
- [ ] Other: ___

**Your answer / ranking:** I ultimately first want a full working beta, but I want several test phases that combine or test individual features together/separately to ensure they work. Need to build off each other in a smart way to properly test.

---

## Part 5 — Anything else

Anything this document didn't ask about, or any reaction to the diagnostic at the top you want on record before I start rebuilding the plan.

**Your answer:** ___

---

## What happens after you send this back

I'll rebuild `Docs/Beta/` around your answers: cut what you tagged CUT, shrink what you tagged SIMPLIFY, pull DEFER items out of near-term phases entirely, replace the AI-guessed creative content with your actual answers (or an open placeholder if you'd rather brainstorm live later), and restructure the phase breakdown so each phase is sized to your Part 0 answers with a real playtest checkpoint at the end of it — not one giant checkpoint at the end of a 15-session phase. `GameDevPlan.md`'s pillars and scope-contract format stay as the base; `Docs/Beta/` gets rebuilt on top of it.
