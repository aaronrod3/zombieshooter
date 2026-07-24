# Definition of Beta-Ready

Two gates, two checklists. Every item is **concrete and testable** — if you cannot tell whether it passes by performing an action, it does not belong here.

**`[INTERNAL]`** — a closed tester group can play unsupervised for a full session with no developer present.
**`[PUBLIC]`** — strangers can obtain it, play it, and you can survive the resulting support load.

---

# Gate 1 — Internal Beta Ready

Entry to **B11**.

## Core loop

- [ ] A player can spawn, scavenge, fight, get injured, treat injuries, eat, drink, sleep, and die — and repeat the cycle with a new character.
- [ ] Death is caused by a comprehensible chain of player decisions. A test where the player dies and cannot say why is a failure.
- [ ] Needs degrade performance before they kill (the pillar). **Starvation is possible but far downstream, never the default outcome of neglect.**
- [ ] Noise reliably attracts zombies, and the player can predict it well enough to make it a decision.
- [ ] The scavenge loop creates real weight/carry decisions.
- [ ] A player can survive at least one full in-game week if they play well.

## Combat

- [ ] `IA_Attack` does the right thing unarmed, with a melee weapon, and with a firearm, with no ambiguity.
- [ ] Melee is stamina-gated; swing-spam is punished by exhaustion, not by an extra mechanic.
- [ ] Firearms jam based on condition; jams are audible, visible, and clearable.
- [ ] Revolvers and bolt-actions never jam (CONFIRMED archetype).
- [ ] Weapons degrade and break; **durability persists across equip/unequip, drop, and re-pickup.**
- [ ] A standing swing never hits a downed zombie; finishing one requires a deliberate stomp.
- [ ] Hip-fire and aimed fire are meaningfully different in spread and headshot rate.

## Health & medical

- [ ] Damage lands on the correct body zone, including **zombie bites** (the blank-`FHitResult` bug is fixed).
- [ ] Both infection tiers exist and behave differently — wound infection is curable by disinfecting and never fatal alone; bite infection is not curable by disinfecting.
- [ ] **A naive player cannot tell which infection tier they have from the UI.**
- [ ] Amputation is reachable from a real bite, choreographed with a montage, and causes a blackout with a real risk window.
- [ ] Critical head bleed exists, is rare, and is impossible to miss when it happens.
- [ ] Fractures take multiple in-game days to heal.

## Survival simulation

- [ ] All 8 needs (Hunger, Thirst, Fatigue, Stamina, Injury/Pain, Infection/Sickness, Wet, Temperature) exist, replicate, and are readable in the HUD.
- [ ] Wet + cold compounds into genuine danger.
- [ ] Encumbrance penalizes stamina drain and **never hard-locks sprint.**
- [ ] Sleep works solo and in co-op, warns before an unsafe sleep, and advances the clock.

## World

- [ ] The full region is playable end-to-end in co-op.
- [ ] Multi-level buildings and basements are navigable, fightable, and lootable — including two players on different floors.
- [ ] Aim never resolves through a floor.
- [ ] Dark spaces genuinely require a light source; the flashlight is visible to other players.
- [ ] Weather changes, replicates, and drives Wet and Temperature.
- [ ] Utilities shutoff fires and visibly changes how the world plays.
- [ ] Basements vary between visits to *different* basements and are identical on revisiting the *same* one.

## UI

- [ ] Every simulated stat is readable without the console.
- [ ] Left-click means "select" over a menu and "attack" otherwise, with zero leakage either way.
- [ ] **No menu pauses the game.** Zombies keep moving and the player stays attackable.
- [ ] Full loot loop works through UI: open, inspect, take individual items, manage weight, close.
- [ ] Two players looting one container cannot duplicate items.
- [ ] Map screen shows the region, named locations, and player position.

## Persistence

- [ ] Quit → relaunch → world remembered, for both host and joining client.
- [ ] A hard kill (`taskkill`) loses at most ~10 seconds of character state and leaves a loadable world.
- [ ] Corrupting the newest save still yields a playable world from a rotating backup, with a clear message.
- [ ] A 2-hour soak shows no save hitching and no unbounded memory growth.
- [ ] Corpses and dropped items respect both cleanup limits, and their timers survive reload.

## Multiplayer

- [ ] 4 players complete a 2-hour session over real internet with no desync or disconnection.
- [ ] Late-join works and the joining player receives full world state.
- [ ] Disconnect and reconnect preserves the character.
- [ ] Sleep-readiness aggregation never deadlocks on a disconnected player.

## Performance

- [ ] Target frame rate and 1%-low held at target zombie density on min-spec hardware, packaged Development build, 4 players.
- [ ] Before/after numbers exist on the fixed B0-T12 stress scenario.
- [ ] No frame spike above threshold during streaming, saving, or a horde event.

## Settings & accessibility

- [ ] Every input is remappable.
- [ ] Gamepad fully supported, including every menu. **All of this is verified in B9 and nowhere earlier** (OQ-B9-01) — treat it as untested until that pass runs.
- [ ] Colorblind modes exist and no gameplay-critical state is colour-only.
- [ ] Text scales without overflowing.
- [ ] Brightness calibration exists — **required**, because the darkness mechanic depends on it.

## Build & process

- [ ] Packaged build runs on a machine that has never had the editor.
- [ ] Build version is visible in-game and embedded in saves and crash reports.
- [ ] Crashes reach the developer automatically with a readable stack trace.
- [ ] Testers can obtain and update builds without hand-holding.
- [ ] `SessionHandoff.md` shows **zero** "built but unverified" items.

---

# Gate 2 — Public Beta Ready

Everything above, **plus**:

## Content completeness

- [ ] The investigation arc is completable start to finish, verified across **at least 3 world seeds**.
- [ ] No clue is unreachable under any seed.
- [ ] The radio arc runs days 1–7 and transitions into dynamic content with no seam.
- [ ] 8–10 distinct event types fire and feel varied over a long session.
- [ ] All named locations are dressed to the B2 quality bar and have a mechanical identity, not just a name.
- [ ] The full weapon roster is authored and balanced.
- [ ] Loot tables cover every zone and rarity tier.

## Progression

- [ ] Every skill and attribute gains from play with visible feedback.
- [ ] No skill has a dominant grind that beats playing normally.
- [ ] All backgrounds are authored, differentiated, and tied to a starting location.
- [ ] Character creation and new-game setup are complete.

## Audio

- [ ] A player with the screen off can estimate threat direction and rough count.
- [ ] What the player hears matches what zombies hear — audio radius equals `FireNoiseRadius`.
- [ ] **No gameplay state is silent**: jams, breaks, infection onset, critical bleed all have audio.
- [ ] Ambient beds cover every zone × time-of-day × weather × interior combination.

## The stranger tests

- [ ] **A person who has never seen the game survives 30 minutes without narration and understands their death.** (B6-PT4)
- [ ] **A stranger buys/downloads, installs, launches, configures, plays for an hour, and hosts co-op for a friend — with zero contact with the developer.** (B12-PT3)
- [ ] A cold reader of the store page can say what the game is and who it is for.

## Retention signal

- [ ] Internal testers voluntarily returned for repeat sessions without being asked.
- [ ] Day 10 of a world has something to do — the late-game emptiness this project exists to avoid.
- [ ] Co-op is meaningfully better than solo, not just solo with a spectator.

## Store & support

- [ ] Store page live: capsule art, screenshots from the real game, trailer, honest copy, system requirements from the measured min spec.
- [ ] Real title chosen (not "ZombieShooter").
- [ ] Pricing and Early Access decision published.
- [ ] Public bug-report pipeline live and monitored.
- [ ] Known-issues list published.
- [ ] Launch-day runbook written; rollback **tested**, not just documented.
- [ ] All UI text routed through `FText` + string tables.

---

## Explicitly NOT required for either gate

Recorded so they cannot be mistaken for gaps late in the process:

Vehicles · dedicated servers · cross-platform play · voice chat · host migration · NPC survivors and factions · hostile human roamers (Decision 5 — confirmed *first post-v1 addition*) · deferred skills (Fishing, Building, Foraging, Cooking, Mechanics) · full sandbox slider suite · seasons · farming (foraging zones only, per OQ-B4-06) · stat-modifying weapon attachments · voice acting · localization beyond English · screen-reader support · Nanite · a demo build · console support · adaptive music · dismemberment/gore system · ranged zombie threats · special zombie archetypes beyond standard + Crawler.
