# Open Questions for Debate

> **Updated 2026-07-26 after a full rescope pass.** `Docs/Planning/RescopeQuestionnaire.md` took this list to the dev directly. Items below marked **✅ RESOLVED 2026-07-26 (dev-confirmed)** carry a real, dated answer — replacing whatever "Rec:" recommendation was previously doing duty as a decision. A few **reverse** the original recommendation outright (OQ-B0-07, OQ-B10-01 especially) — read the dev-answer line, not just the tag. Everything else below is unchanged and still just a recommendation awaiting a real decision, same as before.

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

### OQ-X-03 — Player-count ceiling ✅ RESOLVED 2026-07-26 (dev-confirmed) — reverses the original recommendation
**Dev answer: 4+ players**, not hard-locked to 2–4. Listen-server stays the primary mode; an **optional paid dedicated-server hosting path** is now planned for groups who want one (see OQ-B10-01, also reversed). B8's performance budget must be re-baselined against 4+ rather than 2–4 — this is a real cost increase, not a free change.

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

**✅ RESOLVED 2026-07-26 (dev-confirmed).** Do it now — confirmed. **Refined further:** do it as the independently-testable steps already laid out in `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §8, each with its own checkpoint, rather than one uninterrupted block — dev's explicit process preference is to avoid long chains of dependent, untested work. **Also new:** the dev wants real **stat-affecting weapon attachments** eventually (scopes increase accuracy, silencers decrease sound, etc.) — reversing the design doc's own §7 Tier 2 recommendation against building them. Not part of B0's refactor itself; scheduled as its own later weapon-depth pass (Stage 2) once the base item-instance model is solid, per the design doc's own migration-order step 7.

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

**✅ RESOLVED 2026-07-26 (dev-confirmed).** Aim-cone tightness: "about right" — option 1's numbers stand as dev-approved starting values, not just a recommendation. Headshot weighting (5%/25%): KEEP. Both still tune from real playtesting, but they're no longer guesses.

### OQ-B0-03 — Downed-zombie state and the stomp input 🟡
CONFIRMED: downed zombies are never in a standing swing's arc; finishing requires a deliberate stomp or targeted hit.
| Option | Tradeoff |
|---|---|
| **Stomp is contextual on `IA_Attack`** when standing over a downed target | No new binding. Reads naturally. Risks accidental stomps when a downed zombie is underfoot mid-fight — which is arguably correct behaviour anyway. |
| Dedicated `IA_Stomp` key | Unambiguous. One more binding to teach; low discoverability. |
| Stomp on `IA_SecondaryAction` | Overloads a binding B0-T11 is defining for offhand items. Muddles both. |

**Rec: option 1.** Contextual on `IA_Attack` — the game's stated philosophy is one attack button whose meaning depends on context, and this is exactly that.

**Partially resolved 2026-07-26 (dev-confirmed).** The downed-zombie state + stomp finisher itself: KEEP. **New constraint from the dev:** "find alternatives to make sure this isn't copying PZ" — the mechanic's existence is confirmed, but the specific execution (stomp-on-`IA_Attack`, the PZ-style finisher framing) needs a differentiated take before it's built, not a direct port. Treat the input-binding question above as still open pending that design pass.

### OQ-B0-04 — Temperature model scope 🔴 🚩
CONFIRMED as active scope. The question is depth, and this is the plan's highest scope-risk item.
| Option | Tradeoff |
|---|---|
| **Single body-temp scalar, four inputs** (ambient from weather+time-of-day, indoor/outdoor, wet multiplier, summed clothing insulation) | ~1 session. Genuinely readable. Fits the 1/3-depth pillar. Delivers everything CONFIRMED asks for: hot/cold, hypothermia risk, compounding with Wet. |
| Per-limb thermal + wind chill + wet-per-garment | Realistic; multiplies UI, tuning, and save state. Well past PZ's own depth, which is the opposite of the stated direction. |
| Binary cold/warm flag | Cheapest, but hypothermia risk needs a gradient to be a meaningful threat rather than a switch. |

**Rec: option 1, and hold the line on it.** Consequences route through need-severity tiers into `GetPerformanceMultiplier()` — **not** a separate damage path. Clothing contributes one `InsulationValue` per item; no layering system, per `GameDevPlan` §3's "single outfit slot-set with protection values."

**✅ RESOLVED 2026-07-26 (dev-confirmed).** Option 1 confirmed — "was part of the plan," not scope creep. Dev also reaffirmed the broader needs philosophy directly: create real worry without survival micromanagement becoming the main thing a player has to babysit. Tune generously; the failure mode to avoid is hunger/thirst upkeep becoming tedious or the primary cause of frustration.

### OQ-B0-05 — How fatigue degrades perception 🔴
Depends on CR-10 being answered "the player perceives less."
| Option | Tradeoff |
|---|---|
| **Presentation degradation** — vignette narrows, audio muffles, screen-edge clarity drops | Directly readable as "I am too tired to be doing this." Purely client-side, no replication cost. Interacts well with B4's darkness. |
| Reduce the camera's effective view distance | Mechanically strong, but fights CONFIRMED preset zoom and could read as a rendering bug. |
| Suppress HUD threat indicators | Only works if such indicators exist; B1 does not currently plan them. |

**Rec: option 1**, tuned to be noticeable at severe fatigue only, never at moderate. Accessibility caveat: it must respect B9-T4.4's motion/effects-reduction option.

**✅ RESOLVED 2026-07-26 (dev-confirmed) — see CR-10.** Reading (A) confirmed directly.

### OQ-B0-06 — Sleep vulnerability 🟡
Open Questions §4 asks whether players are attackable while sleeping. `IsSafeToSleep()` is currently a stub returning `true`.
| Option | Tradeoff |
|---|---|
| **Vulnerable, but `IsSafeToSleep()` warns first** | Preserves tension and makes barricading matter. The warning keeps it fair. Co-op: someone can keep watch, which is a genuinely good co-op moment. |
| Invulnerable while sleeping | Safe but removes all tension from a core survival act. |
| Vulnerable with no warning | Maximally tense, but a death you could not have foreseen reads as unfair. |

**Rec: option 1.** The warning is the entire design — it makes sleeping a decision instead of a gamble.

### OQ-B0-07 — Infection legibility in the UI ✅ RESOLVED 2026-07-26 (dev-confirmed) — REVERSES the original premise entirely
This question originally assumed bite infection must be "deliberately ambiguous vs. ordinary sickness," per CR-06. **That premise is gone.**

**Dev answer: "Plainly show the player if they are bitten and infected."** Distinct, legible moodles/UI states for bite status and infection status — the opposite of option 1 below, which is what this question used to recommend. See `00_MasterPlan.md` CR-06 for the full reversal and what survives unchanged (the two-tier mechanical model, the fatal timeline, amputation as the escape valve).

~~One shared "Sick" moodle driven by either tier~~ — **do not build this.** ~~Distinct moodles~~ — **this is now the correct answer**, not the rejected option. Record the new hard constraint on B1-T3.3: infection/bite status must be clearly, unambiguously readable — the opposite of the original constraint.

### OQ-B0-08 — Bite-infection fatal timeline 🟡
| Option | Tradeoff |
|---|---|
| **~2–4 in-game days**, staged across Incubating/Queasy/Fever/Critical | Long enough for the ambiguity window and for an amputation decision; short enough that a bite is genuinely terrifying. |
| Under 1 day | Removes the ambiguity window that is the whole point. |
| ~1 week | Ambiguity becomes tedium; the player forgets they were bitten. |

**Rec: 3 in-game days baseline**, extendable by medical tier (B0-T6.5). Tune from B11 telemetry — specifically, how often the amputation choice actually gets offered.

**✅ RESOLVED 2026-07-26 (dev-confirmed).** "Feels right" — 3 in-game days confirmed as the starting number.

### OQ-B0-09 — Ammo as an inventory item 🟡
Proposed by `Docs/Planning/…` §4; removes `AZSWeapon::CurrentReserveAmmo`/`MaxReserveAmmo`.
| Option | Tradeoff |
|---|---|
| **Ammo is a stackable `FZSItemInstance`** | Ammo weighs, loots, drops, and is shareable between players. Required for the ammo-scarcity pillar to mean anything. Reload draws from the stack. |
| Keep reserve ammo on the weapon actor | Simpler, but ammo is weightless, unshareable, and vanishes with the weapon — which makes "scarce ammo" a fiction. |

**Rec: option 1.** It is a small part of the refactor and it is what makes ammo scarcity real. Note the co-op consequence: sharing ammo becomes a genuine social mechanic.

**✅ RESOLVED 2026-07-26 (dev-confirmed).** Option 1 confirmed KEEP, along with the other three bundled item-instance additions (carry-location categories, loot condition variance, handedness rules) — all KEEP. **Process note:** dev wants the refactor split into the independently-testable steps `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §8 already proposes, each with its own test pass, rather than one uninterrupted 5–6 session block — see `B0_Stabilization.md`'s updated B0-T2.

### OQ-B0-10 — `IA_SecondaryAction` binding 🟡
| Option | Tradeoff |
|---|---|
| **Middle mouse button** | Free, adjacent to `IA_Attack`/`IA_Aim`, maps to a gamepad face button. Awkward on some mice. |
| A letter key (e.g. `F`) | Comfortable; `F` is conventionally "use/flashlight," which fits the primary use case. |
| Right-click with a modifier | Muddles `IA_Aim`. |

**Rec: `F` for keyboard, gamepad face button.** `F` matches player expectation for a flashlight toggle, which is the offhand item B4's darkness mechanic makes load-bearing.

### OQ-B0-11 — Melee weapon display/attachment 🔴 **content-blocking** *(temporary unblock applied 2026-07-25)*
`TP_Mesh` is a full-body skeletal swap authored for a rifle-holding pose; a bat or pipe needs something different. **No real melee config existed before 2026-07-25** — `DA_ZS_WeaponConfig_Crowbar` now exists (`Content/ZS/Weapons/Melee/`, `SM_Crowbar` weapon mesh, rifle `TP_Mesh` reused) purely to unblock B0-T1's Stage F testing, per option 3 below. This does **not** resolve the question — the crowbar still shows the rifle-holding pose, which is the exact wrong-looking outcome option 3 accepts as temporary. T10.7 replaces it with a real answer.
| Option | Tradeoff |
|---|---|
| **Held-prop socket attachment + a shared one-handed melee `TP_Mesh`** | One body pose covers all one-handed melee; the weapon is a static mesh on a hand socket. Cheap, scales to N weapons with zero new C++ — consistent with the multi-weapon rule. |
| A `TP_Mesh` per melee weapon | Best-looking; multiplies authoring per weapon. Directly violates the multi-weapon rule. |
| Reuse the rifle pose | Wrong-looking, but unblocks testing today. Acceptable only as B0-T1.1's stated-temporary measure. |

**Rec: option 1**, plus a second shared pose for two-handed melee later if needed. `UZSWeaponConfig` gains a socket field alongside the existing attachment sockets.

**✅ RESOLVED 2026-07-26 (dev-confirmed) — neither option above, a third way.** "Specific poses per weapon type, rifle/shotgun/LMG all the same, pistol has its own, melee has its own, etc." — grouped by weapon *category*, not one universal pose (option 1 above) and not a unique pose per individual weapon (option 2 above). Three shared `TP_Mesh` poses total for the current roster: long-guns, pistols, melee. This is genuinely content-blocking no longer — author the real melee `UZSWeaponConfig` (B0-T10.7) against this.

### OQ-B0-12 — Weapon roster 🟡
Resolved as "4–6 melee archetypes, one per feel-category." Firearms roster is undefined. Open Questions §6 asks for the exact list.
**Rec — melee (4):** blunt light (bat), edged (machete/axe), improvised fragile (pipe/plank, low durability), heavy two-handed (sledge, slow/high damage). **Firearms (4):** revolver (jam-immune backup, CONFIRMED archetype), pistol, shotgun, bolt-action rifle (jam-immune). This gives both jam-immune archetypes a home and covers the range/noise/ammo spread. Source from `Content/LowPolyWeapons/` and `Content/Mega_Survival_Tools/`.

**Partially resolved 2026-07-26.** Melee roster size confirmed "right" (4–6 archetypes) — the recommendation above stands. **Firearm roster deferred by the dev** — "will provide a full list once basic features are set." Treat the 4-firearm recommendation above as a placeholder for content-authoring purposes only (T4), not a locked roster; revisit once the dev's own list arrives.

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

**✅ RESOLVED 2026-07-26 (dev-confirmed) — leans toward option 2, not option 1.** "Mostly free, cheaper assets, want Door Kickers 2 level of detail so I can create assets myself later." Modest/free-first budget, DK2 as the fidelity benchmark, and the dev expects to hand-model some assets himself over time (Blender pipeline, already documented in `GameDevPlan.md` §5). Don't assume a marketplace-kit-sized budget when scoping B2.

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

**✅ RESOLVED 2026-07-26 (dev-confirmed) — simpler than either option above.** **One continuously-overwritten world** (not multiple slots) — "so player can't load an old save to fix a mistake," no player-facing rollback. **Death rule: no asymmetric solo/co-op split.** Death always respawns a fresh character into the same persistent world, solo included; loot and any base/safehouse remain reachable. `Server_RespawnAsNewCharacter`'s current behavior was already right — this removes a planned special-case rather than requiring new work. Rotating backups for crash/corruption recovery are unaffected and still planned (a different concern from save-scumming). See `00_MasterPlan.md` CR-07 for the full resolution.

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

**✅ RESOLVED 2026-07-26 (dev-confirmed) — bigger than option 1.** "Bigger, build in phases." Driven partly by vehicles coming back into scope (CR-02) — a 1×1 km map is small once driving is real. **The "build in phases" half is now structural, not just advice**: region content moved from a single B4 phase into `B4X`, a continuous track built district-by-district (see `00_MasterPlan.md` §3.2, `T_ContinuousTracks.md` T7). The per-room build-time validation this question already recommended stays just as important — arguably more so, since the map just got bigger.

### OQ-B4-02 — Named locations 🔴 **deferred twice**
Blocks B4 content and B5's investigation arc.
**Rec:** author 8–12 named locations, each with a mechanical identity as well as a name, so they are destinations rather than labels. Suggested spread: a small town centre, a hospital/clinic (medical loot + a clue site), a police/sheriff station (firearms), a hardware store (tools/materials), a school (shelter archetype), a research or ranger station (**the investigation arc's anchor**), a lakeside camp, a farm, and a highway rest stop. **Do this as a writing session during B4's blocked time** — it is not engineering work and it unblocks two phases.

**Partially resolved 2026-07-26.** The *mechanical identity* per location (spread above) can proceed as a placeholder — dev wants generic names for now ("generic names at first, will change later, worried about mechanics and features first"). Build `B4X` locations with functional/generic labels (e.g. "Town Center," "Hospital"); do the real naming/flavor writing pass later, closer to B5, without blocking content on it now.

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

### OQ-B4-12 — Zombie AI depth pass: PZ-style behavioral fidelity 🟡 *(new 2026-07-23, trimmed 2026-07-25; must resolve before B4-T7)*

**Trimmed 2026-07-25** — a chunk of what this question originally covered turned out to be bug-fixing on the *existing* design rather than genuine redesign, and got done in B0 instead of waiting for B4. See `Docs/SessionHandoff.md` and revision register P4-R1/P4-R3 for the full account: all 6 BT tasks are now native C++, two real bugs are fixed (`GetInvestigationPoint`'s unset keys + double-roll, `StartInvestigationTimer`'s InProgress-forever block on `Wander`), and a genuine ambient-wander branch now exists on the root selector. **Behaviorally unverified in PIE** — blocked on an unrelated navmesh issue in the test level (dev fixing manually; see `memory/project_navmesh_dynamic_workaround.md`). This question now covers only what's genuinely still open: real PZ-fidelity *additions*, not fixes to what's already there.

**What's already done, no longer part of this question**: ambient wandering exists (root-level branch) · stale references fixed · the tasks are native, not Blueprint · `MeleeAttack`/`Wander`/`GetInvestigationPoint`/`StartInvestigationTimer` all wired and working code-side.

**What's still genuinely open:**
- **`BTTask_ClearLastKnownLocation` wiring** — exists as a native class, still not placed in the tree. Ambiguous because `StartInvestigationTimer`'s own expiry already clears `LastKnownLocation` on a longer timer; `ClearLastKnownLocation`'s own short (2s) independent delay may have been meant for a narrower case (e.g. "give up immediately if `GetInvestigationPoint` can't find anywhere to go") rather than a duplicate give-up path. Needs a design call, not a guess.
- **Crowd-following / migration** — zombies drift toward other zombies' activity, which is most of how PZ hordes actually form without explicit coordination logic. Directly relevant to **OQ-B7-01**'s horde-coordination approach — resolve this pass first, since it may make Rally-Leader-style coordination unnecessary rather than just unwanted.
- **Sandbox-style "zombie lore" tunables** — PZ exposes speed/toughness/cognition/transmission as world-creation options. `UZSZombieConfig` already supports per-*type* variation (CONFIRMED, P4-R2); whether any axis becomes a **per-world** dial is a question for **OQ-B9-02** (difficulty options), not this one — flag the dependency, don't merge the questions.
- **Door/obstacle destruction over time** — feeds directly into B4-T5.2's door-thumping task; this pass should specify the behavior, B4-T5 implements it.
- **A structural oddity worth a look, not necessarily a bug**: the `Wait`→`GetInvestigationPoint` branch sits at root-selector priority, a sibling of Attack/Chase/Investigate, rather than nested inside the Investigate branch. It happens to work correctly as-is (fails cleanly for a "cold" zombie, falls through to the new ambient-wander branch), but it's the kind of leftover worth a second look when this pass runs.

| Option | Tradeoff |
|---|---|
| **Dedicated design + implementation pass at the start of B4, before B4-T7** | Timed exactly when zone/population content makes it worth tuning against. Now a smaller pass than originally scoped, since the bug-fixing half is already done. |
| Patch incrementally now (B0) | Rejected — same reasoning as before: crowd-following/tunables genuinely need real zone/population content to design against. |
| Fold into B7-T5 (horde coordination) instead of B4 | Too late for the crowd-following piece specifically — B4-T7's zone population and B5's event pacing both assume it. |

**Rec: option 1.** Now likely **S–M (2–3 sessions)**, down from the original 3–4 — the `ClearLastKnownLocation` wiring call and crowd-following/tunables scoping are what's left, not a from-scratch behavior rebuild.

**New scope, added 2026-07-26 (dev-confirmed), fold into this same pass:** a zombie **"freshness" mechanic** — recently-turned zombies are faster and hit harder; the longer a zombie has been undead, the more it slows and weakens. Also, the dev confirmed zombie feel overall as "PZ style, but newer zombies are faster, zombies degrade slowly and slow down, don't do as much damage" — this pass should design the freshness curve alongside the crowd-following/tunables work already scoped here, likely on the same `UZSZombieConfig` per-type-curve substrate. **Also confirmed: genuine large hordes (100+) are important to the vision, not a cuttable stretch goal** — see CR-08 in `00_MasterPlan.md`. This raises the stakes on this pass and on B7-T5/OQ-B7-01's horde-coordination work, but doesn't change this question's own scope beyond adding the freshness mechanic.

---

## B5 — Events & Investigation

### OQ-B5-01 — The actual plot 🔴 **largest content dependency in the plan**
Open Questions §2 asks for the outbreak origin, story beats, the final revelation, and how much is knowable.
**Rec — shape rather than content:** an origin that is **discoverable but never fully explained**, delivered entirely through environmental storytelling, documents, and radio — consistent with OQ-X-06 (no NPCs to explain it) and OQ-X-07 (grounded tone). Three acts: *something happened here* → *someone knew in advance* → *there was an attempt at a response, and it failed*. The capstone is reaching wherever that response was coordinated from — the research/ranger station of OQ-B4-02. **This is a writing task, not engineering; do it during B4's blocked sessions.** Also resolve `GameDevPlan` §7 P8 Q3 (event count at launch) at the same time — it has been flagged BLOCKING since 2026-07-19.

**Dev response 2026-07-26: "Still planning, skip for now."** The three-act shape above was always just an AI-authored placeholder, never adopted — don't treat it as a working draft. **Brainstorm the actual plot together when B5 actually starts**, per the dev's own preference (stated directly in the rescope pass). Tone (pulpy vs. grounded) and event roster count are also still genuinely open — not addressed in the rescope pass — leave both blank until that live session.

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

**Partially resolved 2026-07-26.** Dev: "will compile a full list later, not important for specifics" — the six names above are still just an AI-authored placeholder, not adopted. **What the dev does want built now:** a genuinely generic, data-driven background system that lets him **create backgrounds and assign starting stat/skill values per type** (`UZSBackgroundConfig`-style, per the multi-config rule — new background = new data asset, zero C++). Build the system in Stage 1 (`B6-Sys`); leave the actual roster/names for `B6-Content` in Stage 2, from the dev's own list.

### OQ-B6-05 — Background tradeoffs 🟡 (DEFERRED item)
| Option | Tradeoff |
|---|---|
| **Purely additive** | Simple; no "trap" choices for new players. Risks the strongest combat background dominating. |
| Minor drawbacks | Better balance and more identity; adds a balancing burden and can punish uninformed first choices. |
| Additive skills + differentiated starting *location* difficulty | Balance through world placement rather than stat penalties — the Deputy starts well-armed but in a dense, dangerous town. |

**Rec: option 3.** It creates real tradeoffs without stat penalties, reuses Decision 4's spawn system, and is entirely tunable through content rather than code.

**✅ RESOLVED 2026-07-26 (dev-confirmed).** "Carry a real tradeoff" — confirms option 3 (or option 2; either is compatible with "a real tradeoff," option 3 specifically via starting-location risk rather than stat penalties is still the recommended mechanism). Not purely additive — avoid designing backgrounds where there's no wrong choice.

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

**Ambition raised 2026-07-26 (dev-confirmed) — approach unchanged.** Genuine large hordes (100+, visually distinct) are confirmed important to the vision, not a number to trade away if performance is tight. This doesn't change *how* the decision gets made (still measurement-driven, still gated on B0/B8 profiling) — it raises the bar for what counts as an acceptable answer. See CR-08 in `00_MasterPlan.md`.

### OQ-B7-02 — Audio middleware 🟡
**Rec: UE built-in + MetaSounds.** No licensing, no extra build complexity, and the project's needs (attenuation, occlusion, concurrency limits, a few dynamic layers) are all natively supported. Wwise/FMOD would be justified by adaptive music, which B7-T4.4 recommends against anyway.

**✅ RESOLVED 2026-07-26 (dev-confirmed).** "Sounds good" — UE built-in + MetaSounds confirmed, no paid middleware.

### OQ-B7-03 — Zombie roster for beta 🟡
CONFIRMED: no special archetypes; standard + later Crawlers.
**Rec: two types for beta — standard shambler and Crawler.** Crawlers are cheap (they reuse everything, differing in speed, height, and detection profile) and they make the downed-zombie mechanic (B0-T10.4) meaningful, since a "downed" zombie that can still crawl toward you is far more interesting than one that is simply prone.

### OQ-B7-04 — Music direction 🟢
**Rec: sparse and event-driven, not continuous.** A persistent score masks the audio cues the noise pillar depends on. Music should mark moments — a horde arriving, an event firing, a death — and otherwise leave the ambient bed exposed. This is also dramatically cheaper than a full adaptive score.

**✅ RESOLVED 2026-07-26 (dev-confirmed).** "Sounds good" — sparse/event-driven confirmed over a continuous score.

---

## B8 — Performance

### OQ-B8-01 — Performance budget numbers 🔴 (DEFERRED item, now decidable)
**Rec, pending B8-T2's measurements:** 60 FPS average / 45 FPS 1%-low at 1080p on min spec, with 150 concurrent zombies in view. If measurement says that is unreachable, **lower the zombie count before lowering the frame rate** — CONFIRMED guidance makes zombie count the primary budget metric, and a stuttering survival game is worse than a slightly emptier one.

**Re-baselined 2026-07-26 (dev-confirmed context, numbers still pending measurement).** Dev wants to "keep the game light" (performance-conscious, not chasing max fidelity) but also confirmed 4+ players (not 2–4) and a genuinely large horde as important — these two pull in different directions and both apply. Measure against 4-player concurrent load, not 2, when B8-T2 actually runs. The "lower zombie count before lowering frame rate" guidance stays, but per CR-08, treat that as a last resort given the dev's stated horde priority — exhaust tick-LOD/optimization options first.

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

### OQ-B10-01 — Dedicated servers ✅ RESOLVED 2026-07-26 (dev-confirmed) — reverses the original recommendation
**Rec (superseded): listen-server only for beta.** `GameDevPlan` §3, §6, and `CLAUDE.md` all committed to it.

**Dev answer: not listen-server-only anymore.** "Option for dedicated servers if players want to pay for it, but primarily listen-server." Listen-server stays the default/free path; a paid dedicated-server hosting option is now planned for groups who want one. This is real new scope for B10 (hosting infrastructure, likely a third-party relationship or self-hosted panel — needs its own design pass, not assumed free) and it changes the "always-on world persistence" implication the original recommendation flagged — that implication is now accepted, not avoided.

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

### OQ-B12-01 — Pricing ✅ RESOLVED 2026-07-26 (dev-confirmed)
**Dev answer: ~$9.99**, "to encourage people to buy it." Treat as a target/anchor, not a final locked number — still fine to revisit against comparables closer to B12, but this is a real number now, not a placeholder.

### OQ-B12-02 — Early Access vs. single launch 🟡
| Option | Tradeoff |
|---|---|
| **Early Access** | Revenue during continued development, a real feedback loop, and the POST-BETA backlog (roamers, vehicles, deferred skills) becomes a visible roadmap rather than a cut list. Commits you to ongoing updates. |
| Single full launch | Cleaner; needs everything finished, which for a solo survival sim is a very distant date. |

**Rec: Early Access.** It matches the project's scope reality and gives the deferred-features backlog a legitimate home.

**✅ RESOLVED 2026-07-26 (dev-confirmed).** "Early access for sure, on Steam only at first." Confirms Early Access and also answers part of OQ-B10-02/B10-06 — Steam is the first/only storefront target, consistent with OQ-X-01's PC-only launch decision.

### OQ-B12-03 — Demo 🟡
**Rec: no separate demo for beta.** A demo is a separately balanced, separately supported build — a real scope commitment. Reconsider for a Steam Next Fest after Early Access launch, when the content it would draw from is stable.

### OQ-B12-04 — Localization 🟢
**Rec: English-only for beta, with the pipeline in place** (`FText` + string tables from B9 onward, per B9's standing note). Adding a language then becomes a translation cost, not an engineering project.

### OQ-B12-05 — Public bug pipeline 🟡
**Rec: GitHub Issues** — the repo is already public with the labels and Projects board set up. Add a Discord channel for the reports that never make it to a tracker, which is most of them.

---

## Summary — questions by priority

> **Rewritten 2026-07-26.** A full rescope pass (`Docs/Planning/RescopeQuestionnaire.md`) resolved most of the previously-BLOCKING set directly with the dev — see each question above for the dated answer. This section now reflects what's actually still open.

**✅ RESOLVED, dev-confirmed (28)**

| Question | Answer (see full entry above for detail) | Date |
|---|---|---|
| **OQ-X-01** Platform commitment | PC only for initial launch. | 2026-07-23 |
| **OQ-X-03** Player-count ceiling | **4+**, not hard-locked 2–4 (reverses original rec). | 2026-07-26 |
| **OQ-B9-01** Gamepad support | In scope, all work deferred to B9. | 2026-07-23 |
| **CR-01** Skill roster | `GameDevPlan.md` §3.1's longer list. | 2026-07-26 |
| **CR-02** Vehicles | Not cut — later in dev, ready for beta (reverses original rec). | 2026-07-26 |
| **CR-03 / OQ-B0-04** Temperature/Wet scope | Keep all three, scoped-down model. | 2026-07-26 |
| **CR-04** Camera fallback | Cut now, not gated on a sign-off. | 2026-07-26 |
| **CR-06 / OQ-B0-07** Infection legibility | Plain/clear feedback, **not** ambiguous (reverses original rec — the biggest content change). | 2026-07-26 |
| **CR-07/12 / OQ-B3-01** Save topology & death rule | One continuously-overwritten world; death always → new character, world always persists (no asymmetric solo rule). | 2026-07-26 |
| **CR-10 / OQ-B0-05** Fatigue perception | Reading (A) — player's own perception degrades. | 2026-07-26 |
| **CR-08** Horde ambition | Genuinely large hordes (100+) confirmed important, not a cuttable stretch goal. | 2026-07-26 |
| **OQ-B0-02** Aim-cone & headshot values | "About right" / KEEP — dev-approved starting numbers. | 2026-07-26 |
| **OQ-B0-03** Downed-zombie/stomp | KEEP the mechanic, but needs a non-PZ-clone variant before building. | 2026-07-26 |
| **OQ-B0-08** Bite-infection timeline | ~3 in-game days confirmed. | 2026-07-26 |
| **OQ-B0-09** Ammo as inventory item | KEEP, along with all 4 bundled item-instance additions. | 2026-07-26 |
| **OQ-B0-11** Melee weapon display | Grouped poses by weapon category (long-gun / pistol / melee). | 2026-07-26 |
| **OQ-B0-13** Item-instance refactor | Do it now, but as independently-testable steps, not one block. | 2026-07-26 |
| **OQ-B2-01** Asset budget | Mostly free/cheap, DK2 as the fidelity bar, self-modeled assets expected later. | 2026-07-26 |
| **OQ-B4-01** Region scale | Bigger than 1×1 km, built in phases (now a continuous track, `B4X`). | 2026-07-26 |
| **OQ-B6-05** Background tradeoffs | Real tradeoffs, not purely additive. | 2026-07-26 |
| **OQ-B7-02** Audio middleware | UE built-in + MetaSounds confirmed. | 2026-07-26 |
| **OQ-B7-04** Music direction | Sparse/event-driven confirmed. | 2026-07-26 |
| **OQ-B10-01** Dedicated servers | Now planned as an optional paid path, not cut (reverses original rec). | 2026-07-26 |
| **OQ-B12-01** Pricing | ~$9.99 target. | 2026-07-26 |
| **OQ-B12-02** Early Access vs. single launch | Early Access confirmed, Steam-only at first. | 2026-07-26 |

**🟡 Partially resolved — system/direction confirmed, specifics still deferred (5)**

| Question | What's resolved | What's still open |
|---|---|---|
| **OQ-B0-12** Weapon roster | Melee roster size ("right," 4–6). | Firearm roster — dev will provide once basic features are set. |
| **OQ-B4-02** Named locations | Mechanical-identity spread still a reasonable placeholder plan. | Real names/flavor — generic labels for now, dev wants to focus on mechanics first. |
| **OQ-B4-12** Zombie AI depth pass | Scope confirmed, **plus a new "freshness" mechanic added.** | The pass itself (crowd-following, `ClearLastKnownLocation` wiring) still needs to actually run. |
| **OQ-B5-01** The plot | Confirmed: brainstorm together live when B5 starts, not now. | The actual plot — genuinely still nothing, by design. |
| **OQ-B6-04** Background roster | Confirmed: generic, data-driven system, build now (`B6-Sys`). | The actual roster/names — dev's own list, later (`B6-Content`). |

**🔴 Still BLOCKING (5)** — resolve before the named phase starts.

| Phase | Questions |
|---|---|
| Before B4X (region content) | OQ-B4-03 (interior visibility solution — needs a spike, not just a decision) |
| Before B5 | OQ-B5-04 (event roster count — genuinely still open, tone also open) |
| Before B7 | OQ-B7-01 (horde-coordination *approach* — still gated on profiling measurements, ambition is confirmed but the technical answer isn't) |
| Before B8 | OQ-B8-01, OQ-B8-02 (budget numbers — re-baselined for 4+ players, but still pending actual measurement) |

**🟡 SEQUENCEABLE (~30, reduced from 39)** — decide in parallel with early implementation on that phase. Unaffected by the rescope pass unless listed above.

**🟢 LATE (11)** — OQ-X-05, OQ-X-08, OQ-B1-03, OQ-B6-03, OQ-B6-07, OQ-B6-08, OQ-B10-05, OQ-B10-09, OQ-B12-03, OQ-B12-04, OQ-B12-05.
