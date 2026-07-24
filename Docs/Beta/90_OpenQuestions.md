# Open Questions for Debate

Every design decision not explicitly confirmed in `ZombieShooter_Consolidated_Changes.md`, grouped by the phase it belongs to so it can be resolved in context. Each carries 2–4 options with tradeoffs, a recommendation, and a priority tag.

**Tags** · 🔴 **BLOCKING** — must be resolved before related implementation starts · 🟡 **SEQUENCEABLE** — can be decided in parallel with early implementation · 🟢 **LATE** — safe to defer to a polish pass or post-beta.

**When you answer one:** write the answer *and the reasoning* into this file, with a date. Do not delete the question. CR-01 in the master plan exists precisely because a decision's reasoning was lost and an older version resurfaced.

---

## Cross-cutting / foundational

These predate the phase structure and several have been open since `GameDevPlan.md` §7 was written.

### OQ-X-01 — Platform commitment ✅ **RESOLVED 2026-07-23**
`GameDevPlan` §7 flagged this as blocking for P1 and it went unanswered since the pivot.

**Answer: PC only for the initial launch.** Dev decision, 2026-07-23.

**Reasoning:** focus the available part-time capacity on core features rather than platform breadth. Console remains possible later — the top-down camera was chosen partly for that (`GameDevPlan` §1, Notes §3) and nothing in this plan closes the door — but it is explicitly not a beta consideration and must not constrain UI, input, or performance decisions now.

**Consequences:** closes `GameDevPlan` §7 cross-cutting Q3 · Steam Deck verification drops from stretch goal to POST-BETA · reinforces OQ-B10-02 (Steam networking) as the sensible choice, since Steam is the only target · console-readiness is never a valid argument for scope in any B-phase.

### OQ-X-02 — Win condition, run length, and loss state 🔴
Open Questions §1 asks all three and they interact. Decision 6 already resolved that the investigation arc is an optional capstone with no forced ending, which answers part of it.
| Option | Tradeoff |
|---|---|
| **Survive indefinitely; arc is optional capstone** (current direction) | Consistent with Decision 6 and the permadeath-into-new-character loop. Risks PZ's own empty-late-game problem, which B5 exists to address. |
| Arc completion ends the run | Cleaner narrative payoff. Contradicts Decision 6 and complicates co-op persistence. |
| Both — arc completion offers an optional "evac" ending the player may decline | Best of both; slightly more state to track. |

**Rec: option 3.** It honours Decision 6 (the world keeps running) while giving players who want closure a real ending. The evac becomes the persistent world-state change Decision 6 already asks for.

### OQ-X-03 — Player-count ceiling 🟡
Confirmed 2–4. Hard-locked or flexible to 6+?
**Rec: hard-lock 2–4 for beta.** Listen-server bandwidth and B8's budget are sized for it. Raising it later is a tuning exercise; designing for it now costs performance headroom you do not have.

### OQ-X-04 — The unique selling point, stated in one sentence 🟡
`GameDevPlan` §1 has four differentiators in long form. B12-T1.2 needs one line.
**Rec:** lead with the differentiator PZ structurally lacks: *a co-op survival sim with an actual mystery to solve.* Simulation depth is table stakes in this genre; a discoverable investigation arc with a real ending is not.

### OQ-X-05 — Monetization 🟢
Premium one-time, Early Access, or free.
**Rec: premium, likely via Early Access** — see OQ-B12-02. Decide by B12; it does not affect development.

### OQ-X-06 — Are there any NPCs at all? 🟡
Open Questions §2 asks whether the world is fully NPC-less.
**Rec: fully NPC-less for beta.** Hostile human roamers are the confirmed first post-v1 addition (Decision 5) and reuse the zombie AI cheaply. Survivor NPCs are a much larger system and stay behind their own planning pass. **Nothing in B5's narrative should require a living person to talk to** — this constrains OQ-B5-01 and is the main reason to answer it early.

### OQ-X-07 — Tone target 🟡
Grounded realistic horror, pulpy action-horror, or other. Guides VO, music, and UI tone.
**Rec: grounded, restrained horror.** Consistent with Decision 3's "dark, earthy, slight realism," with the ambiguity-preserving infection design, and with B7's sparse-music recommendation. It is also the cheaper tone to execute convincingly solo.

### OQ-X-08 — Target release window 🟢
**Rec:** do not set a public date until B11 is underway. The §3.2 estimate (~10–14 months part-time) is a forecast, not a commitment, and B4 is the phase most likely to move it.

---

## B0 — Stabilization

### OQ-B0-13 — Item-instance refactor: go / no-go 🔴 **ANSWER FIRST**
The single highest-consequence question in B0. `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §5 proposes replacing three incompatible notions of "an item" with one `FZSItemInstance` + `FGuid` model. It touches five files of code shipped days ago.
| Option | Tradeoff |
|---|---|
| **Do it now, in B0** | ~5–6 sessions. Unblocks durability persistence, ammo-as-item, loot condition variance, hotbar-references-inventory, and save serialization. Cost is lowest it will ever be — no content is authored against the current shape yet. |
| Defer to after content authoring | Every `DA_ZS_ItemConfig_*`, loot table, and container placed increases migration cost. B3's save layer would be written against a model that then changes, meaning writing it twice. |
| Don't do it; patch symptoms individually | Durability persistence alone needs most of the plumbing anyway. You end up with the refactor's cost and none of its coherence. |

**Rec: do it now.** Four separate CONFIRMED features (loot condition variance, durability, ammo economy, four container categories) all reduce to "items need per-instance identity." Without it they are individually hacky and collectively incoherent. **Nothing else in B0 should start until this is answered.**

### OQ-B0-01 — Scroll-wheel arbitration 🔴
`IA_HotbarCycle` is on the mouse wheel; CONFIRMED preset zoom now also wants it.
| Option | Tradeoff |
|---|---|
| **Scroll = zoom; hotbar cycles on a modifier or keys** | Zoom is the more frequent, more continuous action, and CONFIRMED auto-zoom means manual zoom must be immediately reachable. Hotbar already has 1–9 direct-select. |
| Scroll = hotbar; zoom on +/− or a modifier | Preserves the gamepad-bumper analogy the hotbar design leaned on — but that analogy holds on gamepad regardless of what the mouse wheel does. |
| Context-dependent | Ambiguous inputs are exactly the class of bug `GameDevPlan` §7 Q6 already warns about. |

**Rec: option 1.** Scroll = zoom. Hotbar keeps 1–9 on keyboard and bumpers on gamepad, which is where cycling actually earns its place.

### OQ-B0-02 — Aim-cone and headshot-weighting values 🔴
CR-11 makes this the **sole** source of combat accuracy pressure, since Panic stays deferred. That raises it from tuning detail to core design.
| Option | Tradeoff |
|---|---|
| **Tight cones, big hip/aim delta** (e.g. pistol 8°→2°, rifle 5°→1°) | Aiming is clearly worth it. Hip-fire stays viable at melee range. Rewards the deliberate play the pillars want. |
| Wide cones, small delta | More chaotic, more "survival horror miss." Risks feeling broken rather than tense — players read misses as bugs. |
| Distance-scaled cone | Most realistic; hardest to read and hardest to tune. |

**Rec: option 1**, with headshot weighting roughly 5% hip-fire / 25% aimed. **Prototype before authoring any weapon content** (B0-T3.5 flags this) — the numbers change every weapon's data.

### OQ-B0-03 — Downed-zombie state and the stomp input 🟡
CONFIRMED: downed zombies are never in a standing swing's arc; finishing requires a deliberate stomp or targeted hit.
| Option | Tradeoff |
|---|---|
| **Stomp is contextual on `IA_Attack`** when standing over a downed target | No new binding. Reads naturally. Risks accidental stomps when a downed zombie is underfoot mid-fight — which is arguably correct behaviour anyway. |
| Dedicated `IA_Stomp` key | Unambiguous. One more binding to teach; low discoverability. |
| Stomp on `IA_SecondaryAction` | Overloads a binding B0-T11 is defining for offhand items. Muddles both. |

**Rec: option 1.** Contextual on `IA_Attack` — the game's stated philosophy is one attack button whose meaning depends on context, and this is exactly that.

### OQ-B0-04 — Temperature model scope 🔴 🚩
CONFIRMED as active scope. The question is depth, and this is the plan's highest scope-risk item.
| Option | Tradeoff |
|---|---|
| **Single body-temp scalar, four inputs** (ambient from weather+time-of-day, indoor/outdoor, wet multiplier, summed clothing insulation) | ~1 session. Genuinely readable. Fits the 1/3-depth pillar. Delivers everything CONFIRMED asks for: hot/cold, hypothermia risk, compounding with Wet. |
| Per-limb thermal + wind chill + wet-per-garment | Realistic; multiplies UI, tuning, and save state. Well past PZ's own depth, which is the opposite of the stated direction. |
| Binary cold/warm flag | Cheapest, but hypothermia risk needs a gradient to be a meaningful threat rather than a switch. |

**Rec: option 1, and hold the line on it.** Consequences route through need-severity tiers into `GetPerformanceMultiplier()` — **not** a separate damage path. Clothing contributes one `InsulationValue` per item; no layering system, per `GameDevPlan` §3's "single outfit slot-set with protection values."

### OQ-B0-05 — How fatigue degrades perception 🔴
Depends on CR-10 being answered "the player perceives less."
| Option | Tradeoff |
|---|---|
| **Presentation degradation** — vignette narrows, audio muffles, screen-edge clarity drops | Directly readable as "I am too tired to be doing this." Purely client-side, no replication cost. Interacts well with B4's darkness. |
| Reduce the camera's effective view distance | Mechanically strong, but fights CONFIRMED preset zoom and could read as a rendering bug. |
| Suppress HUD threat indicators | Only works if such indicators exist; B1 does not currently plan them. |

**Rec: option 1**, tuned to be noticeable at severe fatigue only, never at moderate. Accessibility caveat: it must respect B9-T4.4's motion/effects-reduction option.

### OQ-B0-06 — Sleep vulnerability 🟡
Open Questions §4 asks whether players are attackable while sleeping. `IsSafeToSleep()` is currently a stub returning `true`.
| Option | Tradeoff |
|---|---|
| **Vulnerable, but `IsSafeToSleep()` warns first** | Preserves tension and makes barricading matter. The warning keeps it fair. Co-op: someone can keep watch, which is a genuinely good co-op moment. |
| Invulnerable while sleeping | Safe but removes all tension from a core survival act. |
| Vulnerable with no warning | Maximally tense, but a death you could not have foreseen reads as unfair. |

**Rec: option 1.** The warning is the entire design — it makes sleeping a decision instead of a gamble.

### OQ-B0-07 — Preserving infection ambiguity in the UI 🔴
CONFIRMED that bite infection must be "deliberately ambiguous vs. ordinary sickness." Easy to destroy accidentally in B1.
| Option | Tradeoff |
|---|---|
| **One shared "Sick" moodle** driven by either tier, with identical symptom progression | Ambiguity fully preserved. Requires discipline: no tooltip, log line, or debug UI may leak the tier. |
| Distinct moodles | Destroys the CONFIRMED design intent entirely. |
| Shared moodle that diverges at a late stage | Preserves early ambiguity; gives late certainty when it is too late to matter, which is arguably the horror payoff. |

**Rec: option 1 for beta, option 3 as a post-beta refinement.** Record it as a hard constraint on B1-T3.3 — this is the kind of intent a well-meaning UI pass quietly breaks.

### OQ-B0-08 — Bite-infection fatal timeline 🟡
| Option | Tradeoff |
|---|---|
| **~2–4 in-game days**, staged across Incubating/Queasy/Fever/Critical | Long enough for the ambiguity window and for an amputation decision; short enough that a bite is genuinely terrifying. |
| Under 1 day | Removes the ambiguity window that is the whole point. |
| ~1 week | Ambiguity becomes tedium; the player forgets they were bitten. |

**Rec: 3 in-game days baseline**, extendable by medical tier (B0-T6.5). Tune from B11 telemetry — specifically, how often the amputation choice actually gets offered.

### OQ-B0-09 — Ammo as an inventory item 🟡
Proposed by `Docs/Planning/…` §4; removes `AZSWeapon::CurrentReserveAmmo`/`MaxReserveAmmo`.
| Option | Tradeoff |
|---|---|
| **Ammo is a stackable `FZSItemInstance`** | Ammo weighs, loots, drops, and is shareable between players. Required for the ammo-scarcity pillar to mean anything. Reload draws from the stack. |
| Keep reserve ammo on the weapon actor | Simpler, but ammo is weightless, unshareable, and vanishes with the weapon — which makes "scarce ammo" a fiction. |

**Rec: option 1.** It is a small part of the refactor and it is what makes ammo scarcity real. Note the co-op consequence: sharing ammo becomes a genuine social mechanic.

### OQ-B0-10 — `IA_SecondaryAction` binding 🟡
| Option | Tradeoff |
|---|---|
| **Middle mouse button** | Free, adjacent to `IA_Attack`/`IA_Aim`, maps to a gamepad face button. Awkward on some mice. |
| A letter key (e.g. `F`) | Comfortable; `F` is conventionally "use/flashlight," which fits the primary use case. |
| Right-click with a modifier | Muddles `IA_Aim`. |

**Rec: `F` for keyboard, gamepad face button.** `F` matches player expectation for a flashlight toggle, which is the offhand item B4's darkness mechanic makes load-bearing.

### OQ-B0-11 — Melee weapon display/attachment 🔴 **content-blocking**
`TP_Mesh` is a full-body skeletal swap authored for a rifle-holding pose; a bat or pipe needs something different. **No melee config has ever been authored because of this.**
| Option | Tradeoff |
|---|---|
| **Held-prop socket attachment + a shared one-handed melee `TP_Mesh`** | One body pose covers all one-handed melee; the weapon is a static mesh on a hand socket. Cheap, scales to N weapons with zero new C++ — consistent with the multi-weapon rule. |
| A `TP_Mesh` per melee weapon | Best-looking; multiplies authoring per weapon. Directly violates the multi-weapon rule. |
| Reuse the rifle pose | Wrong-looking, but unblocks testing today. Acceptable only as B0-T1.1's stated-temporary measure. |

**Rec: option 1**, plus a second shared pose for two-handed melee later if needed. `UZSWeaponConfig` gains a socket field alongside the existing attachment sockets.

### OQ-B0-12 — Weapon roster 🟡
Resolved as "4–6 melee archetypes, one per feel-category." Firearms roster is undefined. Open Questions §6 asks for the exact list.
**Rec — melee (4):** blunt light (bat), edged (machete/axe), improvised fragile (pipe/plank, low durability), heavy two-handed (sledge, slow/high damage). **Firearms (4):** revolver (jam-immune backup, CONFIRMED archetype), pistol, shotgun, bolt-action rifle (jam-immune). This gives both jam-immune archetypes a home and covers the range/noise/ammo spread. Source from `Content/LowPolyWeapons/` and `Content/Mega_Survival_Tools/`.

### OQ-B0-14 — Review the two autonomous P6 design calls 🟡
Both were made unsupervised on 2026-07-21 and flagged for review by the assistant that made them.
| Call | Assessment |
|---|---|
| **Bag-slot depth: `Back` + `Hip`** | Sound. Matches §3's "weight + bag slots, no bags-in-bags," and extends by adding enum values later. **Rec: keep.** |
| **Rarity pool: global per-server-session** | Sound *given no zone system existed*. B4-T1.5 now builds one. **Rec: keep global for beta**, revisit per-zone post-beta — per-zone rarity is a meaningfully different feel and should be a deliberate choice, not a side effect. |

### OQ-B0-15 — Weight budget and rarity tier numbers 🟡
Blocks T4 content authoring.
**Rec:** base on-person capacity ~8kg (pockets only, deliberately restrictive so bags matter), `Hip` +5kg, `Back` +20kg. Encumbrance penalty begins at 100% and scales to a hard stamina-drain multiplier at 150%. Rarity tiers: Common (unpooled), Uncommon (unpooled), Rare (pool ~30/session), VeryRare (pool ~8/session). **All provisional — these are B11-T4.3's tuning targets, and the point is to have numbers to test, not to get them right first.**

---

## B1 — UI/UX

### OQ-B1-01 — UI art timing 🟡
| Option | Tradeoff |
|---|---|
| **Functional-grey now, restyle after B2** | B1 is not blocked on art. B1-T2.1's style asset is the single restyle surface. |
| Wait for B2's art lock | Delays B1 and therefore everything downstream, for a purely cosmetic gain. |

**Rec: option 1.** Hard requirement: no colour literals outside the style asset.

### OQ-B1-02 — HUD density 🟡
| Option | Tradeoff |
|---|---|
| **Contextual** — moodles appear on state change and fade; health/ammo persist | Clean screen, matches the top-down tactical framing. Risk: a slow-building need goes unnoticed. |
| Always-on everything | Nothing missed; cluttered, and clutter at max zoom-out is a readability problem. |
| Player-configurable | Best outcome, more work. Naturally belongs with B9's settings. |

**Rec: contextual for beta, configurable in B9 if time allows.** Exception: anything that can kill you in under a minute (critical head bleed) is always-on regardless.

### OQ-B1-03 — Solo pause 🟢
Co-op cannot pause. Solo could.
**Rec: no pause in solo either.** One code path, one set of assumptions, and it preserves the tension pillar identically in both modes. Offer a "quit to menu saves immediately" affordance instead, which covers the real need (having to stop playing).

---

## B2 — Art & Pipeline

### OQ-B2-01 — Asset budget 🔴
Open Questions §14 asks whether there is money for marketplace/contracted art.
| Option | Tradeoff |
|---|---|
| **Modest budget for a core kit + audio libraries** | Buys the two most time-expensive categories. `GameDevPlan` §6's own mitigation is literally "buy the core." |
| Free/self-made only | Zero cost, significant time cost, and a real risk of visual incoherence across mismatched free packs. |
| Contracted art | Best quality, highest cost, adds coordination overhead to a solo schedule. |

**Rec: option 1.** A modular environment kit and a sound library are where money converts most directly into months.

### OQ-B2-02 — Nanite 🟡
| Option | Tradeoff |
|---|---|
| **No Nanite; traditional LODs** | Predictable cost on min spec, and low-poly assets are exactly the case Nanite helps least. Better fit for a high-actor-count game. |
| Nanite on environment only | Saves LOD authoring; raises the GPU floor and hurts min-spec targets. |

**Rec: no Nanite.** The project is low-poly by decision and CPU/AI-bound by nature (zombie count is the primary budget metric). Nanite solves a problem this project does not have.

---

## B3 — Persistence

### OQ-B3-01 — Save topology and world lifetime 🔴 **largest remaining architectural question**
Combines `GameDevPlan` §7 cross-cutting Q5 with the §7 P3 backlog's world-termination rules (CR-07/CR-12).
| Option | Tradeoff |
|---|---|
| **One world per save slot; multiple slots; host owns the save** | Players can run several worlds. Matches "host-owns-the-save." Late-join means joining the host's active world. Slightly more UI. |
| Single world per install | Simplest. Brutal — one bad death and there is nothing to return to. Poor fit for a beta where testers want to retry. |
| Per-character saves within a shared world | Closest to PZ's model; significantly more complex in co-op and conflicts with host-owned saves. |

**Rec: option 1.** Then, on world lifetime, per the §7 P3 backlog: **co-op continues on a fresh character unless the entire party is dead; solo death ends that world outright.** Multiple slots make the solo rule survivable rather than punitive — you lose a world, not the game. Note this **contradicts what `Server_RespawnAsNewCharacter` does today** and must be implemented in B10-T1.5.

### OQ-B3-02 — Serialization format 🟡
| Option | Tradeoff |
|---|---|
| **`USaveGame` + `FArchive`** | Engine-native, versioning support, least custom code. Can get large; needs care with `FGuid`-keyed maps. |
| Custom binary | Compact and fast; all versioning, endianness, and tooling is yours to write and maintain. |
| JSON/text | Debuggable and diffable — genuinely valuable during beta bug triage. Large and slow at world scale. |

**Rec: `USaveGame` + `FArchive`, with a debug JSON export path.** The export is a few hours and turns "tester says their save is broken" from guesswork into inspection.

---

## B4 — World Content

### OQ-B4-01 — Region scale 🔴
`GameDevPlan` §7 P7 Q2 asks "still ~1×1 km?"
| Option | Tradeoff |
|---|---|
| **~1×1 km: one dense town + rural fringe** | Matches §3's REPLACE line and "compete honestly: small dense map." Achievable solo. Dense is more interesting than large. |
| 2×2 km, multiple settlements | More variety and exploration; roughly 4× the content, and B4 is already XXL. Would likely need vehicles (CR-02). |
| Under 1×1 km | Very achievable; risks feeling small once players know it, which is a late-game emptiness problem B5 would then have to carry alone. |

**Rec: option 1, and validate it against B2-T4.5's measured per-room build time before committing.** If a single room takes 3 sessions, even 1×1 km is too ambitious and the answer is fewer, denser buildings.

### OQ-B4-02 — Named locations 🔴 **deferred twice**
Blocks B4 content and B5's investigation arc.
**Rec:** author 8–12 named locations, each with a mechanical identity as well as a name, so they are destinations rather than labels. Suggested spread: a small town centre, a hospital/clinic (medical loot + a clue site), a police/sheriff station (firearms), a hardware store (tools/materials), a school (shelter archetype), a research or ranger station (**the investigation arc's anchor**), a lakeside camp, a farm, and a highway rest stop. **Do this as a writing session during B4's blocked time** — it is not engineering work and it unblocks two phases.

### OQ-B4-03 — Interior visibility solution 🔴
Old P7 named the problem and never solved it.
| Option | Tradeoff |
|---|---|
| **Roof/floor-above fade on entry** | Standard, readable, cheap. Must be per-player in co-op (T2.3), which is the real work. |
| Camera-relative cutaway plane | Great for multi-level; more complex, can be disorienting when the plane moves. |
| Per-room reveal (fog-of-war style) | Strong for tension and exploration. Highest complexity; interacts badly with co-op sightlines. |
| Dithered occlusion of blocking geometry | Cheapest; visually noisy at max zoom-out. |

**Rec: option 1, evaluated against option 2 in a spike.** Prototype both against B2's reference room before committing — B4-T2 explicitly budgets for this because the wrong choice is a rebuild.

### OQ-B4-04 — Floor detection method 🟡
| Option | Tradeoff |
|---|---|
| **Authored floor volumes** | Reliable, explicit, and designer-controlled. Adds an authoring step per building. |
| Downward trace + Z-bucketing | Zero authoring; fails on ramps, stairs, mezzanines, and open stairwells — exactly where it matters. |
| Hybrid: volumes with trace fallback | Robust; two systems to reason about. |

**Rec: authored volumes.** Multi-level correctness is a hard requirement for aim resolution; predictability beats convenience. The authoring cost folds into B4-T10's per-building pass.

### OQ-B4-05 — Zombie repopulation in cleared areas 🟡
| Option | Tradeoff |
|---|---|
| **Slow repopulation via migration from adjacent zones** | Clearing feels meaningful but not permanent. Ties naturally to horde migration events (B5-T2.4). |
| Permanently cleared | Strong sense of progress; the map trends toward empty, which is the late-game emptiness problem again. |
| Fast respawn | Preserves threat; makes clearing pointless and player effort feel disrespected. |

**Rec: option 1.** Migration-based, so repopulation is diegetic rather than a spawn timer, and noise pulls it faster.

### OQ-B4-06 — Farming/foraging 🟡
`GameDevPlan` §3 commits to "v1: farming-lite + foraging zones." **This plan does not schedule it** — deliberately, pending this answer. It is the one §3 commitment left unplaced.
| Option | Tradeoff |
|---|---|
| **Foraging zones only for beta** | Cheap: marked areas yield food on a timer. Gives a renewable food source without a growth simulation. Farming moves post-beta with the Foraging skill (already deferred in §3.1). |
| Farming-lite too | Honours §3 fully; adds plots, growth stages, water, and season interaction — a real system in an already-XXL phase. |
| Neither | Food is finite-loot only, which makes long-term survival a countdown rather than a stable state. |

**Rec: option 1.** Foraging zones deliver the renewable-food need at a fraction of the cost. Farming returns post-beta with the Building/Foraging skills it belongs with.

### OQ-B4-07 — Does light attract zombies? 🟡
Unstated but obvious interaction between B4's darkness mechanic and the noise-as-threat pillar.
| Option | Tradeoff |
|---|---|
| **Yes — light extends effective sight radius against the holder** | Consistent: the pillar is "every advantage has a cost." Makes the flashlight a real decision instead of a free upgrade. |
| No | Simpler; makes light strictly beneficial, which is out of character for this game. |
| Only at night/outdoors | Realistic, more conditional logic, harder to read. |

**Rec: option 1.** It converts the darkness mechanic from an inconvenience into a genuine risk/reward choice, which is the difference between a chore and a mechanic.

### OQ-B4-08 — Locked doors, keys, lockpicking 🟡
| Option | Tradeoff |
|---|---|
| **Locked doors + breaching (break the door, make noise)** | No new skill or minigame. Ties directly into the noise pillar: the cost of forcing entry is attention. |
| Keys to find | Adds a loot category and search motivation; risks a key-hunt that blocks content. |
| Lockpicking skill/minigame | New skill (contradicts §3.1's settled roster) and a minigame that stops real-time play — against Decision 1. |

**Rec: option 1.** Breaching is the answer most consistent with every existing pillar and requires no new systems.

### OQ-B4-09 — Does rain mask noise? 🟡
**Rec: yes**, as a modest reduction to effective noise radius. It gives weather a tactical dimension and rewards players for reading conditions — a strong, nearly-free interaction between two existing systems.

### OQ-B4-10 — Day/night cycle length 🟡
| Option | Tradeoff |
|---|---|
| **~1.5–2 real hours per in-game day** | A session spans 1–2 days. Night is a real event, not a nuisance. Aligns with the compressed clock that needs decay already assumes. |
| Real-time 24h | Realistic; unplayable for a game with a day-based utilities shutoff. |
| ~30–45 min | Very eventful; needs and infection timelines would need retuning, and day counts inflate fast. |

**Rec: ~2 real hours**, exposed as a tunable. Night should be roughly 1/3 of it — long enough to matter, short enough not to be an endurance test given B4-T4's darkness.

### OQ-B4-11 — Map discovery and teammate positions 🟡
**Rec: map revealed by exploration; teammates shown only when nearby or when a location is manually shared.** Both preserve tension and make B4-T9.3's markers a real co-op communication tool rather than decoration. Full teammate tracking would remove most of the reason to coordinate.

### OQ-B4-12 — Zombie AI depth pass: PZ-style behavioral fidelity 🟡 *(new, 2026-07-23; must resolve before B4-T7)*

Surfaced while triaging `BT_Zombie`'s compile errors during B0-T0.2 (see revision register P4-R1/P4-R3). The current AI is a ShooterGame-derived chase→melee loop — functional, PIE-confirmed, but a long way from Project Zomboid's actual zombie identity, which the project's own pitch (`GameDevPlan` §1) claims as its simulation-fidelity foundation. Dev call, 2026-07-23: **don't patch this incrementally — hold the redesign for one deliberate pass**, timed so it lands once real zombie population/zone content makes the behavior worth tuning against (B4-T7), rather than twice (a stopgap now, a rebuild later).

**Known building blocks already authored, currently disconnected** (found during this investigation, not by design — record so this pass doesn't re-derive it): `BTTask_Wander`, `BTTask_GetInvestigationPoint`, `BTTask_ClearLastKnownLocation`, `BTTask_StartIdleDwell`, `BTTask_StartInvestigationTimer`, `BTTask_MeleeAttack` (wired), plus the unused `BP_ZombieAIController`. Whether any of these survive the redesign or get replaced outright is exactly what this pass should decide — their existence is a starting inventory, not a constraint.

**PZ traits worth deliberately evaluating**, per `ProjectZomboid_DesignReference.md` §8 (cross-reference when this pass starts):
- **Ambient wandering with no target** — PZ zombies drift even absent a stimulus; the current BT only activates on perception. Affects whether "clearing an area" ever visually reads as clear.
- **Bounded memory at the last-known location** — PZ zombies give up and return to ambient wander after a tunable search window, rather than tracking forever. `BTTask_ClearLastKnownLocation` suggests this was already the intent once.
- **Crowd-following / migration** — zombies drift toward other zombies' activity, which is most of how PZ hordes actually form without explicit coordination logic. Directly relevant to **OQ-B7-01**'s horde-coordination approach — resolve this pass first, since it may make Rally-Leader-style coordination unnecessary rather than just unwanted.
- **Sandbox-style "zombie lore" tunables** — PZ exposes speed/toughness/cognition/transmission as world-creation options. `UZSZombieConfig` already supports per-*type* variation (CONFIRMED, P4-R2); whether any axis becomes a **per-world** dial is a question for **OQ-B9-02** (difficulty options), not this one — flag the dependency, don't merge the questions.
- **Door/obstacle destruction over time** — feeds directly into B4-T5.2's door-thumping task; this pass should specify the behavior, B4-T5 implements it.

| Option | Tradeoff |
|---|---|
| **Dedicated design + implementation pass at the start of B4, before B4-T7** | Timed exactly when zone/population content makes it worth tuning against. Delays any wander/investigate behavior until B4 — acceptable, since B0-T8.4 confirms nothing in B0/B1/B2/B3 needs it. |
| Patch incrementally now (B0-T8) and refine later | Rejected by the dev — risks building twice, and risks the redesign anchoring on the stopgap's shape instead of PZ's actual behavior. |
| Fold into B7-T5 (horde coordination) instead of B4 | Too late — B4-T7's zone population and B5's event pacing (e.g. horde migration events) both assume zombies already behave like PZ zombies, not like a placeholder. |

**Rec: option 1.** Scope as its own task at the top of B4, before T7 — likely **M (3–4 sessions)**: audit the existing disconnected assets, decide what's kept vs. rebuilt, implement, verify against a PZ-familiar playtester if one is available. Update `Docs/Beta/B4_WorldContent.md` T7 with a concrete sub-task once this lands, rather than leaving it implicit in this question.

---

## B5 — Events & Investigation

### OQ-B5-01 — The actual plot 🔴 **largest content dependency in the plan**
Open Questions §2 asks for the outbreak origin, story beats, the final revelation, and how much is knowable.
**Rec — shape rather than content:** an origin that is **discoverable but never fully explained**, delivered entirely through environmental storytelling, documents, and radio — consistent with OQ-X-06 (no NPCs to explain it) and OQ-X-07 (grounded tone). Three acts: *something happened here* → *someone knew in advance* → *there was an attempt at a response, and it failed*. The capstone is reaching wherever that response was coordinated from — the research/ranger station of OQ-B4-02. **This is a writing task, not engineering; do it during B4's blocked sessions.** Also resolve `GameDevPlan` §7 P8 Q3 (event count at launch) at the same time — it has been flagged BLOCKING since 2026-07-19.

### OQ-B5-02 — Ambient event locatability 🟡 (DEFERRED item)
| Option | Tradeoff |
|---|---|
| **Ambient events are unlocatable; tangible events are investigable** | Clear player-facing rule. Ambient events do atmospheric work without generating false leads; tangible ones reward investigation. |
| All events investigable | Every sound is a lead; consistent but expensive, and it makes the world feel like a checklist. |
| All ambient, none investigable | Cheapest; wastes the event system's potential entirely. |

**Rec: option 1**, and make the distinction *audibly* learnable — ambient events should sound distant and directionless by design, so players learn the rule without being told it.

### OQ-B5-03 — Event escalation over time 🟡
**Rec: yes, weighted by `DayCount`.** Early days favour ambient and opportunity events; later days favour horde and threat events. It gives the world an arc independent of the investigation arc, which is what keeps day 20 from feeling like day 3.

### OQ-B5-04 — Event roster count 🔴
`GameDevPlan` §7 P8 Q3, blocking since 2026-07-19.
**Rec: 8–10 distinct event types for beta**, spread across ambient / opportunity / threat / narrative. Below ~6 the world feels scripted within one session; above ~12 the authoring cost lands squarely in the phase that is already carrying the investigation arc.

### OQ-B5-05 — Voice acting 🟡
| Option | Tradeoff |
|---|---|
| **Text-only radio transcripts and notes** | No talent dependency, no re-record cost when writing changes, and it is the scope-safe default. Subtitles are free by construction (B9-T4.3). |
| Voiced radio only | Radio is the most atmospheric candidate and the most bounded scope. Costs money and locks the script. |
| Full VO | Out of scope for a solo beta. |

**Rec: text-only for beta.** Radio VO is a strong post-beta upgrade once the script has stopped moving.

### OQ-B5-06 — Clue/journal UI 🟡
**Rec: a journal listing discovered clues, with map-pin integration via B4-T9.3.** No quest markers and no objective arrows — the diegetic framing is the point, and a floating waypoint would undo it. Clues state what was found and where; the player draws the connection.

---

## B6 — Progression & Onboarding

### OQ-B6-01 — XP curves 🟡
**Rec:** level 1→2 fast enough to be felt within a session; 4→5 slow enough to represent real investment. Roughly geometric with a ~2.2× step. All values in `UZSSkillConfig` and `TuningReference.md`, tuned from B11-T4.7 telemetry.

### OQ-B6-02 — Practice loops per skill 🟡 (DEFERRED item)
CONFIRMED as open for exploration.
**Rec, one per skill:** *Melee* — practise swings on a fixed object (diminishing returns). *Aiming* — dry-aim tracking, plus real shots. *Reloading* — manual unload/reload cycling. *Maintenance* — maintenance actions on any owned weapon. *First Aid* — treating minor self-inflicted or teammate wounds. *Fitness/attributes* — emerge from normal play, no dedicated loop needed. **Governing rule (B6-T3.3): practice must always be worse XP than real use**, or the game teaches people to hit a wall in a basement.

### OQ-B6-03 — Perks and skill cap 🟢
**Rec: no perks; 1–5 is a hard cap for everyone.** Passive improvements only. Consistent with §3.1's non-grind goal and with the "no point-buy, build variety from background + emergent play" direction. Perks are a good post-beta expansion if progression turns out to feel thin.

### OQ-B6-04 — Background roster 🔴 (DEFERRED item)
CONFIRMED: backgrounds grant higher starting proficiency, not unique items. Must suit the setting and not mirror another game's occupation list.
**Rec — 6 backgrounds, each tied to a starting location and 1–2 skills:** *Park Ranger* (Survival-adjacent + Firearms, ranger station) · *Paramedic* (First Aid, clinic) · *Mechanic* (Maintenance, hardware store) · *Sheriff's Deputy* (Firearms + Aiming, station) · *Line Cook* (no combat skill; starts with the best food/shelter position — the deliberately non-combat option) · *Hunter* (Aiming + Sneak, rural camp). Deliberately Adirondacks-flavoured rather than generic occupations.

### OQ-B6-05 — Background tradeoffs 🟡 (DEFERRED item)
| Option | Tradeoff |
|---|---|
| **Purely additive** | Simple; no "trap" choices for new players. Risks the strongest combat background dominating. |
| Minor drawbacks | Better balance and more identity; adds a balancing burden and can punish uninformed first choices. |
| Additive skills + differentiated starting *location* difficulty | Balance through world placement rather than stat penalties — the Deputy starts well-armed but in a dense, dangerous town. |

**Rec: option 3.** It creates real tradeoffs without stat penalties, reuses Decision 4's spawn system, and is entirely tunable through content rather than code.

### OQ-B6-06 — Radio tutorial pacing 🟡 (DEFERRED item)
**Rec:** days 1–2 survival basics (needs, noise, looting) · days 3–4 combat and injury · days 5–6 the utilities-shutoff warning, turning tutorial into narrative · day 7 the transition into the investigation arc. Each broadcast teaches by *describing what is happening in the world*, never by naming a control.

### OQ-B6-07 — Death recap screen 🟢
**Rec: yes, minimal** — cause, day survived, and one or two notable stats. It directly serves B6-PT4's "dies to something they understand" criterion. Keep it short; it is a moment of frustration, not a place for a report.

### OQ-B6-08 — Appearance customization 🟢
**Rec: minimal — a small set of preset heads/bodies plus clothing colour, independent of background.** Character art is not where a solo project's time converts to player value, and clothing will be partly gameplay-driven by insulation anyway.

### OQ-B6-09 — New-game setup flow 🟡
**Rec:** world name → seed (optional, defaults random) → difficulty (OQ-B9-02) → background (which implies spawn) → scatter-spawns toggle for co-op → appearance. Everything on one screen where possible. A long wizard before a permadeath game is friction at exactly the wrong moment.

---

## B7 — Audio & Horde AI

### OQ-B7-01 — Horde coordination approach 🔴
CONFIRMED that performance drives this decision, and the measurement is B0-T12/B8's.
| Option | Tradeoff |
|---|---|
| **AI tick LOD + shared target grouping** | Distant zombies tick rarely; nearby ones behave fully. Groups share one pathing target. Preserves individual behaviour where it is visible; biggest win for least redesign. |
| Flow-field pathfinding for hordes | Excellent at very high counts; a substantial system, and individual behaviour becomes harder to express. |
| Rally leader | Explicitly not committed in CONFIRMED. Cheap, but coordination becomes visible and gamey. |
| Full ECS-style AI rewrite | Best ceiling; completely out of scope. |

**Rec: option 1, decided on B8-T2's measurements, not this recommendation.** Tick LOD is worth doing regardless of what else is chosen, so start there.

### OQ-B7-02 — Audio middleware 🟡
**Rec: UE built-in + MetaSounds.** No licensing, no extra build complexity, and the project's needs (attenuation, occlusion, concurrency limits, a few dynamic layers) are all natively supported. Wwise/FMOD would be justified by adaptive music, which B7-T4.4 recommends against anyway.

### OQ-B7-03 — Zombie roster for beta 🟡
CONFIRMED: no special archetypes; standard + later Crawlers.
**Rec: two types for beta — standard shambler and Crawler.** Crawlers are cheap (they reuse everything, differing in speed, height, and detection profile) and they make the downed-zombie mechanic (B0-T10.4) meaningful, since a "downed" zombie that can still crawl toward you is far more interesting than one that is simply prone.

### OQ-B7-04 — Music direction 🟢
**Rec: sparse and event-driven, not continuous.** A persistent score masks the audio cues the noise pillar depends on. Music should mark moments — a horde arriving, an event firing, a death — and otherwise leave the ambient bed exposed. This is also dramatically cheaper than a full adaptive score.

---

## B8 — Performance

### OQ-B8-01 — Performance budget numbers 🔴 (DEFERRED item, now decidable)
**Rec, pending B8-T2's measurements:** 60 FPS average / 45 FPS 1%-low at 1080p on min spec, with 150 concurrent zombies in view. If measurement says that is unreachable, **lower the zombie count before lowering the frame rate** — CONFIRMED guidance makes zombie count the primary budget metric, and a stuttering survival game is worse than a slightly emptier one.

### OQ-B8-02 — Minimum hardware target 🔴 (DEFERRED item)
CONFIRMED reference note: PZ's min spec is ~quad-core 2.77GHz / 8GB / 2GB VRAM, community "smooth" spec is i5-9600K / Ryzen 5600-class + RTX 3060 + 16GB — **but PZ is a 2D sprite engine and is not a valid baseline for 3D UE5.**
**Rec:** target roughly i5-8400 / Ryzen 2600, GTX 1060 6GB / RX 580, 16GB RAM. This is a realistic UE5 floor and it captures most of the surviving Steam hardware base. **Remember the listen-server host pays both server and client costs** — the host machine is the real min-spec case and should be what you measure.

---

## B9 — Accessibility & Settings

### OQ-B9-01 — Gamepad support for beta ✅ **RESOLVED 2026-07-23 (deferred to B9)**

**Answer: all gamepad work and testing deferred to B9.** Dev decision, 2026-07-23 — core features first, PC-only launch (OQ-X-01). Gamepad is **not cut**, just not verified or polished until B9.

**Reasoning:** gamepad support is a *verification and polish* cost, not a *design* cost, so deferring it buys real capacity now without foreclosing anything. Enhanced Input handles gamepad natively; the top-down twin-stick scheme was already chosen with it in mind.

**⚠ The one caveat that makes this safe — keep the architecture, drop the testing.** Deferring gamepad *testing* is free. Deferring gamepad *architecture* is not: if B1 ships eight UI screens with no focus-navigation path, retrofitting one in B9 means reopening all eight. The split adopted:

| Keep now (cheap, prevents rework) | Defer to B9 (real cost, no rework risk) |
|---|---|
| Generic focus navigation at B1-T2.4's **widget base class** — one implementation, inherited by every screen | Per-screen gamepad navigation verification |
| Input actions stay gamepad-mappable in Enhanced Input (already true — costs nothing) | Gamepad-specific bindings and tuning |
| Don't hardcode mouse-only interactions (e.g. drag-drop must always have a keyboard/gamepad path — already a B1-T5.3 requirement for accessibility reasons anyway) | Input glyph switching (B9-T3.6) |
| `IA_HotbarCycle` stays a first-class action | Stick sensitivity, deadzones, aim assist |

**Consequences:** gamepad removed from B1's exit criteria and from every playtest checkpoint before B9 · B0-T0.5 becomes "record as unverified," not "verify" · B9-T3.3 becomes a genuine build-and-verify task rather than a check · **OQ-B0-01's scroll-arbitration reasoning must be re-examined**, since it partly rested on the hotbar mapping to a gamepad bumper — the recommendation still holds on keyboard grounds alone (zoom is the more frequent continuous action, hotbar has 1–9), but the gamepad half of the argument is now deferred, not load-bearing.

### OQ-B9-02 — Difficulty options 🟡
**Rec: three presets** (zombie density, loot scarcity, infection chance) **plus the CONFIRMED per-skill XP rate tunable**, set at world creation and **locked for that world's lifetime.** Locking avoids the co-op fairness problem of mid-world changes, and it protects the permadeath framing. Each preset must state exactly what it changes — hidden scaling contradicts the transparency pillar.

---

## B10 — Multiplayer & Release

### OQ-B10-01 — Dedicated servers 🟡
**Rec: listen-server only for beta.** `GameDevPlan` §3, §6, and `CLAUDE.md` all commit to it. Dedicated servers are a post-beta feature that also implies always-on world persistence — a different product shape, not just a deployment option.

### OQ-B10-02 — Steam/EOS networking 🔴 **biggest swing item in B10**
| Option | Tradeoff |
|---|---|
| **Steam networking (Steam Sockets)** | No port forwarding — which is the single biggest practical barrier to testers actually playing together. Friends-list invites. Ties the build to Steam and adds real integration work. |
| Direct IP only | What exists today; zero new work. Port forwarding will materially reduce B11 participation, and "we couldn't connect" is the least useful feedback possible. |
| EOS | Platform-agnostic, free; more integration work than Steam and less relevant if OQ-X-01 says Steam-only. |

**Rec: Steam networking**, given OQ-X-01's Steam-only recommendation. The connection barrier is the difference between testers playing co-op and testers playing solo and reporting on the wrong game.

### OQ-B10-03 — Late-join model 🟡
**Rec: full late-join into a running world.** Anything less makes co-op scheduling-dependent, which for a 2–4 player game between friends is a serious practical constraint. It is real work (B10-T1.1) and it is worth it.

### OQ-B10-04 — Disconnect handling 🟡
| Option | Tradeoff |
|---|---|
| **Character remains in-world briefly (~60s), then despawns with items intact** | Prevents combat-log escapes without punishing a genuine crash. |
| Vanish immediately | Safe for the player; a free escape from any dangerous situation. |
| Remain indefinitely, vulnerable | Realistic and harsh; a router blip becomes a lost character in a permadeath game. |

**Rec: option 1.** It threads the needle between anti-exploit and not punishing bad connections — and in a beta, bad connections are guaranteed.

### OQ-B10-05 — Host migration 🟢
**Rec: not supported.** Communicate clearly that the host leaving ends the session, and **save cleanly before closing it** (B10-T1.4). Host migration in a streamed persistent world is disproportionately expensive.

### OQ-B10-06 — Tester distribution channel 🟡
**Rec: Steam playtest** if OQ-B10-02 lands on Steam networking — one system for distribution, updates, and connection. Otherwise itch.io.

### OQ-B10-07 — Telemetry scope 🟡
**Rec: minimal and opt-in** — session length, death cause/location/day, zombie kills, items looted, skill levels at death. That set is exactly what B11-T4's tuning needs and nothing more. Anything beyond it is data you will not use and will have to justify.

### OQ-B10-08 — Branching strategy 🟡
**Rec: `main` for development, `release/*` for tester builds, cherry-pick hotfixes.** Minimal ceremony, and it makes tester builds reproducible while work continues. Never force-push `main` (standing rule).

### OQ-B10-09 — Voice chat 🟢
**Rec: none — rely on Discord.** For a 2–4 player co-op game whose players are almost certainly already in a call, building voice chat is poor value.

---

## B11 / B12 — Beta Program

### OQ-B11-01 — Feedback channel 🟡
**Rec: Discord for unstructured impressions + a structured form for bugs.** Both matter — the structured channel gets you repro steps, the unstructured one gets you the reason someone stopped playing.

### OQ-B11-02 — Tester recruiting 🟡
**Rec: 8–10 testers**, mixed between survival-genre veterans and newcomers, recruited from the community channel T3 starts at B4. Veterans find balance and depth problems; newcomers find onboarding problems. You need both, and they rarely overlap.

### OQ-B12-01 — Pricing 🟢
**Rec: decide against comparable titles at B12.** Genre and scope place it in the usual indie survival band; the number does not affect development and should be set with launch-window information, not now.

### OQ-B12-02 — Early Access vs. single launch 🟡
| Option | Tradeoff |
|---|---|
| **Early Access** | Revenue during continued development, a real feedback loop, and the POST-BETA backlog (roamers, vehicles, deferred skills) becomes a visible roadmap rather than a cut list. Commits you to ongoing updates. |
| Single full launch | Cleaner; needs everything finished, which for a solo survival sim is a very distant date. |

**Rec: Early Access.** It matches the project's scope reality and gives the deferred-features backlog a legitimate home.

### OQ-B12-03 — Demo 🟡
**Rec: no separate demo for beta.** A demo is a separately balanced, separately supported build — a real scope commitment. Reconsider for a Steam Next Fest after Early Access launch, when the content it would draw from is stable.

### OQ-B12-04 — Localization 🟢
**Rec: English-only for beta, with the pipeline in place** (`FText` + string tables from B9 onward, per B9's standing note). Adding a language then becomes a translation cost, not an engineering project.

### OQ-B12-05 — Public bug pipeline 🟡
**Rec: GitHub Issues** — the repo is already public with the labels and Projects board set up. Add a Discord channel for the reports that never make it to a tracker, which is most of them.

---

## Summary — questions by priority

**✅ RESOLVED (2)**

| Question | Answer | Date |
|---|---|---|
| **OQ-X-01** Platform commitment | **PC only** for initial launch. Console/Steam Deck → POST-BETA, never a scope argument. | 2026-07-23 |
| **OQ-B9-01** Gamepad support | **In scope, all work deferred to B9.** Architecture hooks kept in B1 (base-class focus navigation, no mouse-only interactions); testing and tuning deferred. | 2026-07-23 |

**🔴 BLOCKING (15)** — resolve before the named phase starts. Batch each phase's set into one design session.

| Phase | Questions |
|---|---|
| Before B0 | OQ-B0-13 (**answer first**), OQ-B0-01, OQ-B0-02, OQ-B0-04, OQ-B0-05, OQ-B0-07, OQ-B0-11 · plus CR-01, CR-02, CR-10 |
| Cross-cutting | OQ-X-02 |
| Before B2 | OQ-B2-01 |
| Before B3 | OQ-B3-01 |
| Before B4 | OQ-B4-01, OQ-B4-02, OQ-B4-03 |
| Before B5 | OQ-B5-01, OQ-B5-04 |
| Before B6 | OQ-B6-04 |
| Before B7 | OQ-B7-01 |
| Before B8 | OQ-B8-01, OQ-B8-02 |
| Before B10 | OQ-B10-02 |

**🟡 SEQUENCEABLE (39)** — decide in parallel with early implementation on that phase. Notable addition: **OQ-B4-12** (zombie AI PZ-fidelity redesign) must resolve before B4-T7 specifically, not before B4 as a whole.

**🟢 LATE (11)** — OQ-X-05, OQ-X-08, OQ-B1-03, OQ-B6-03, OQ-B6-07, OQ-B6-08, OQ-B7-04, OQ-B10-05, OQ-B10-09, OQ-B12-01, OQ-B12-04.
