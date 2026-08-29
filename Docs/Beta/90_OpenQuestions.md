# Open Questions for Debate

> **Stripped down 2026-07-26**, after two same-day answer batches resolved most of what was open. This file used to keep every option table and full "Rec:" writeup next to each question, even after it was answered — useful mid-debate, dead weight afterward. **Resolved questions below are now a short decision + the reasoning that still matters, not a debate transcript.** The original option tables and full tradeoff writeups are preserved in git history if they're ever needed again. **Still-open questions keep their full option tables** — those still need real debate, so the tradeoffs stay visible.

Every design decision not explicitly confirmed elsewhere, grouped by the phase it belongs to.

**Tags** · 🔴 **BLOCKING** — must be resolved before related implementation starts · 🟡 **SEQUENCEABLE** — can be decided in parallel with early implementation · 🟢 **LATE** — safe to defer to a polish pass or post-beta.

**When you answer one:** record the decision and *why*, dated, condensed to what a future session actually needs. Don't delete the question — a resolved question becomes a short decision record, not empty space. `00_MasterPlan.md`'s Contradiction Register exists precisely because a decision's reasoning was lost once already; the *why* is the part worth keeping, not the debate that preceded it.

---

## Cross-cutting / foundational

### OQ-X-01 — Platform commitment ✅ RESOLVED 2026-07-23
**PC only for the initial launch.** Console stays possible later (the top-down camera was partly chosen with it in mind) but is explicitly not a beta consideration — must not constrain UI, input, or performance decisions now.

### OQ-X-02 — Win condition, run length, and loss state ✅ RESOLVED 2026-07-26
**Survive indefinitely — no forced ending, no evac mechanic.** "Survive indefinitely, this ensures co-op playability." Plus a new, unshaped **quest list** players can work through over a run (content TBD, dev will supply it later) — a lightweight objective layer alongside B5's investigation arc, not a replacement for it. Feeds OQ-B5-04 once shaped.

### OQ-X-03 — Player-count ceiling ✅ RESOLVED 2026-07-26 — reverses the original rec
**4+ players**, not hard-locked 2–4. Listen-server stays primary; an optional paid dedicated-server hosting path is planned for groups who want one (OQ-B10-01). B8 must re-baseline its performance budget against 4+, not 2–4.

### OQ-X-04 — The unique selling point, one sentence 🟡
`GameDevPlan` §1 has four differentiators in long form; B12-T1.2 needs one line.
**Rec:** *a co-op survival sim with an actual mystery to solve* — the differentiator PZ structurally lacks.

### OQ-X-05 — Monetization 🟢
**Rec: premium via Early Access** — see OQ-B12-02 (resolved). Doesn't affect development, decide by B12.

### OQ-X-06 — Are there any NPCs at all? ✅ RESOLVED 2026-07-26
**Hostile human roamers, in scope, reusing zombie AI cheaply.** Friendly survivor NPCs explicitly deferred post-release. Nothing in B5's narrative should require a living person to talk to.

### OQ-X-07 — Tone target ✅ RESOLVED 2026-07-26
**Grounded, restrained horror.** "This is still correct."

### OQ-X-08 — Target release window 🟢
**Rec:** no public date until B11 is underway — the §3.2 estimate is a forecast, not a commitment.

### OQ-X-09 — Run vs. Sprint as two distinct speed tiers 🟡 *(new 2026-07-26, from `Docs/InputBindings.md`)*
The dev's keybind list gives Run (hold Left Shift) and Sprint (double-click Left Shift) as separate actions. Current design/code has only one sprint tier (`StartSprint`, stamina-gated).
| Option | Tradeoff |
|---|---|
| **Run is a free, non-stamina jog between Walk and Sprint** | Gives players a faster default pace without touching the stamina economy; Sprint keeps its scarcity. Needs a third movement-speed value and a new input path (hold vs. double-click detection). |
| Run is just a rename/remap of the existing single sprint tier, no new tier | Zero new design; but then "Sprint" (double-click) needs its own separate, actually-new behavior to justify existing at all. |

**No rec yet — needs a design call before B0-T4/T10 build against it.**

### OQ-X-10 — Toggle Safety (multiplayer PvP) 🟡 *(new 2026-07-26, from `Docs/InputBindings.md`)*
The keybind list includes a weapon-safety toggle scoped explicitly to PvP — but this project has no designed PvP mode anywhere in the plan (co-op vs. zombies/hostile roamers is the stated design).
| Option | Tradeoff |
|---|---|
| PvP is real, scoped scope (friendly-fire toggle, or a dedicated PvP mode) | Safety mechanic has a real purpose; but PvP is a meaningfully new scope item with no design anywhere yet. |
| Safety is really about accidental-discharge prevention in general (co-op friendly fire), "PvP" in the dev's note is shorthand, not a new mode | No new mode needed; the mechanic still needs its own design pass (what does "safety on" actually block, and why would a player want it off). |

**No rec yet — needs a scoping conversation, not an assumption either way.**

### OQ-X-11 — In-game text chat and push-to-talk voice ✅ RESOLVED 2026-07-30 (text half only)
**Build basic text chat.** New B10 scope: a chat widget + replicated messages, bound to the existing Enter/"Toggle Chat" key. **Voice stays unresolved as its own question** — see `OQ-B10-09`, which keeps its "rely on Discord" recommendation unchanged; this decision doesn't touch it. Push-to-Talk (V) stays unimplemented until/unless OQ-B10-09 is separately confirmed.

---

## B0 — Stabilization

### OQ-B0-13 — Item-instance refactor: go / no-go ✅ RESOLVED 2026-07-26
**Do it now, in B0** — but as the independently-testable steps in `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §8, each with its own checkpoint, not one long uninterrupted block. Also: real **stat-affecting weapon attachments** wanted eventually (scopes/silencers with a mechanical effect) — its own later Stage-2 weapon-depth pass once the base item-instance model is solid, not part of B0 itself.

### OQ-B0-01 — Scroll-wheel arbitration ✅ RESOLVED 2026-07-26
**Scroll = zoom.** Hotbar drops scroll-cycling entirely, keeps 1–9 direct-select.

### OQ-B0-02 — Aim-cone and headshot-weighting values ✅ RESOLVED 2026-07-26
**Tight cones, big hip/aim delta** (pistol 8°→2°, rifle 5°→1° starting values) — "about right." Headshot weighting ~5% hip-fire / 25% aimed. Both still tune from real playtesting, but they're dev-approved starting numbers, not guesses.

### OQ-B0-14 — Damage feedback model: headshot-instakill + no on-screen numbers ✅ RESOLVED 2026-08-09
**Body shots stay multi-hit (unchanged); a headshot is an instant kill unconditionally** — no per-weapon exception, no per-zombie-type resistance, flat universal rule regardless of what dealt the hit. No on-screen numeric damage feedback — relies on `UZSHealthComponent::OnDamageImpact` (already exists as a VFX/SFX hook) plus zombie reaction/stagger/death animation for player-facing feedback, matching the already-planned removal of the temporary debug hit-confirmation text. Reuses the existing bone→zone hit detection and headshot-weighting fields as-is, no new systems needed.

### OQ-B0-03 — Downed-zombie state and the finisher ✅ RESOLVED 2026-07-26
**KEEP the downed state.** Finisher stays contextual on `IA_Attack`, but branches on loadout: bare-handed over a downed target → stomp; melee weapon equipped → a downward swing/strike instead. That equipped-dependent branch — not one universal finisher animation regardless of loadout — is the deliberate difference from a direct PZ port.

### OQ-B0-04 — Temperature model scope ✅ RESOLVED 2026-07-26 🚩
**Single body-temp scalar, four inputs** (ambient weather+time-of-day, indoor/outdoor, wet multiplier, summed clothing insulation) — "was part of the plan," not scope creep. Routes through need-severity tiers into `GetPerformanceMultiplier()`, never a separate damage path; no per-limb thermal, no clothing-layering system. Broader needs philosophy reaffirmed directly: create real worry without survival micromanagement becoming the main thing a player has to babysit — tune generously.

### OQ-B0-05 — How fatigue degrades perception ✅ RESOLVED 2026-07-26
**Presentation degradation** — vignette narrows, audio muffles — noticeable at severe fatigue only, never moderate. See CR-10. Must respect B9-T4.4's motion/effects-reduction option.

### OQ-B0-06 — Sleep vulnerability ✅ RESOLVED 2026-07-26 — goes further than originally scoped
**Sleep is gated, not just risky.** `IsSafeToSleep()` requires both: (1) no recent hostile detection/pursuit (an aggro-cooldown state, not just "nothing nearby right now"), and (2) real shelter at the immediate location — barricaded room, locked door, or inside a vehicle. Minecraft-style: you can't just lie down in the open street. See `B0_Stabilization.md` T4.10.

### OQ-B0-07 — Infection legibility in the UI ✅ RESOLVED 2026-07-26 — REVERSES the original premise
**Plainly show bite/infection status** — distinct, legible moodles for both tiers. The opposite of the "deliberate ambiguity vs. ordinary sickness" this question originally assumed (CR-06). The two-tier mechanical model, the fatal timeline, and amputation as the escape valve all survive unchanged — only the UI legibility rule flipped.

### OQ-B0-08 — Bite-infection fatal timeline ✅ RESOLVED 2026-07-26
**2–4 in-game day window** (loosened from an initial flat-3-day answer) — long enough to give the player a real choice about *where* to be when it runs out. Pairs directly with T9.1/T9.2's loot-stays-at-death-location + dead-player-becomes-a-zombie mechanic, confirmed to apply cleanly to infection deaths: "it would be cool if the character becomes a zombie, holding on to loot and clothing it had on when it died."

### OQ-B0-09 — Ammo as an inventory item ✅ RESOLVED 2026-07-26
**KEEP** — ammo is a stackable `FZSItemInstance`, along with the other three bundled item-instance additions (carry-location categories, loot condition variance, handedness rules).

### OQ-B0-10 — `IA_SecondaryAction` binding ✅ RESOLVED 2026-07-26
Stays flexible — a dedicated key (`F` default) or a context-aware dispatch, either is fine to build. Full rebinding is already covered by `B9_Accessibility_Settings.md` B9-T3, so "make it assignable" needs no new work.

### OQ-B0-11 — Melee weapon display/attachment ✅ RESOLVED 2026-07-26 — a third way, not either original option
**Grouped poses by weapon category** — long-guns (rifle/shotgun/LMG) share one `TP_Mesh` pose, pistols their own, melee their own. Three shared poses total for the current roster. Real per-category animations are still pending on the dev's own content-gathering timeline; near-term priority is verifying correct spawn/socket attachment against this three-pose model, not finishing final animations.

### OQ-B0-12 — Weapon roster 🟡 partially resolved
**Melee roster size confirmed** ("right," 4–6 archetypes: blunt/edged/improvised-fragile/heavy-two-handed). **Firearm roster still open** — dev will provide a full list once basic features are set; any placeholder 4-gun list is content-authoring scaffolding only, not a locked roster.

### OQ-B0-14 — Review the two autonomous P6 design calls 🟡
**Rec: keep both** — Back/Hip bag-slot depth as-is; global-per-session rarity pool for beta, revisit per-zone post-beta (B4-T1.5 now has a zone system that didn't exist when this was first decided).

### OQ-B0-15 — Weight budget and rarity tier numbers 🟡
**Rec (provisional, B11-T4.3's tuning targets):** ~8kg on-person capacity, `Hip` +5kg, `Back` +20kg, encumbrance penalty scaling to a hard stamina-drain multiplier at 150%. Rarity pool sizes: Rare ~30/session, VeryRare ~8/session.

### OQ-B0-16 — Base-building/barricading sequencing ✅ RESOLVED 2026-07-30
**Build minimal barricading first** — board up doors/windows via `UZSInteractableComponent`, no full construction — as the first slice of `GameDevPlan.md` §3's already-confirmed SIMPLIFY scope (barricades, reinforcement, crates, rain collector). This isn't a re-scoping down; reinforcement/crates/rain-collector stay in scope for later within the same SIMPLIFY line, just sequenced after the minimal version. Directly feeds `OQ-B0-06`'s "real shelter" half of `IsSafeToSleep()`, currently stubbed `true`. Home for the actual task: `B4X` (world interactable content), once that track reaches door/window dressing.

### OQ-B0-17 — Multi-hit melee ✅ RESOLVED 2026-07-30 (direction), implementation deferred
**Blunt/bladed weapon categories get multi-hit** — an arc sweep that can land on 2-3 zombies per swing; improvised-fragile and heavy-two-handed categories keep the current single-nearest-target resolution. New field on `UZSWeaponConfig` (multi-config rule applies), plus a melee-damage rebalance pass. **Not blocking anything now** — implement whenever the melee weapon roster/animations get their real content pass (B2/content authoring), not before.

### OQ-B0-18 — Player-side crouch stealth ✅ RESOLVED 2026-07-30
**Add a crouch noise-reduction multiplier**, symmetric with the zombie-side `SightRadius`/`HearingRange` model already in `UZSZombieConfig`. Feeds `UZSNoiseSystem` — crouched movement should measurably shrink the effective noise radius zombies react to, not just look different. Makes crouching tactically meaningful, not only visual.

---

## B1 — UI/UX

### OQ-B1-01 — UI art timing ✅ RESOLVED 2026-07-26
**Functional-grey now, restyle after B2.** Hard requirement: no colour literals outside the style asset.

### OQ-B1-02 — HUD density ✅ RESOLVED 2026-07-26
**Contextual by default, made player-configurable in B9.** Exception: anything that can kill you in under a minute (critical head bleed) stays always-on regardless.

### OQ-B1-03 — Solo pause ✅ RESOLVED 2026-08-03
**No pause, ever** — solo behaves identically to co-op. One code path, one set of assumptions, same tension pillar (Decision 1) in both modes. `T8.2`'s in-game menu doesn't need a solo-only branch. Issue #5 closed.

### OQ-B1-04 — Notifications/toast system ✅ RESOLVED 2026-07-30
**Build a lightweight, queued toast system** — pickup confirmation, horde-approaching alert, player joined/left, and future needs, all through one reusable widget rather than a bespoke UI per event type. New sub-task: `B1-T3.10`.

### OQ-B1-05 — Loading screens / level-transition UX ✅ RESOLVED 2026-07-30
**Simple placeholder now, real art later.** A tip/lore-text loading screen ships as soon as it's built, styled functional-grey like the rest of B1 per `OQ-B1-01`'s precedent — swap in real art once B2 locks direction, don't wait to build the mechanism itself. New line: `B1-T8`.

### OQ-B1-06 — Save/autosave indicator ✅ RESOLVED 2026-07-30
**Add a small HUD icon that flashes on autosave**, reading off B3-T2.1's ~10s character-save cadence. Reassures players nothing is lost, especially given B3's hard-kill/corruption hardening work is a standing concern. New sub-task: `B1-T3.11`.

### OQ-B1-07 — Scoreboard/player-list key ✅ RESOLVED 2026-07-30
**Add a dedicated key and HUD screen** showing connected players — absent from `InputBindings.md` until now. Doubles as the target-selection UI for the new host admin tools (`OQ-B10-12`). New sub-task: `B1-T3.9`.

### OQ-B1-08 — Container interact UX ✅ RESOLVED 2026-08-03
**Option B — a real loot screen**, not auto-loot-all. Matches what's already in progress: `UZSContainerLootWidget` (C++, compiled) + `WBP_ZS_ContainerLoot` (T6, Designer-tab build in progress). Per-item take (`Server_TakeContainerItem`) plus a "Take All" convenience button (`Server_TakeAllContainerItems`) — auto-loot-everything-on-interact is retired once this screen ships. Issue #4 closed.

### OQ-B1-09 — SecondaryHand offhand item action trigger ✅ RESOLVED (already shipped, not newly decided)
**Option A — one binding, dispatches on slot content.** `IA_SecondaryAction` (B0-T11) already dispatches on the resolved config's type — toggle for items, full fire/melee for weapons (B0-T11.2, mirrors `CurrentWeapon`'s lifecycle) — confirmed against `CLAUDE.md`'s Player/ section 2026-08-03. Issue #3 closed as already-answered, not a fresh decision.

---

## BH — Hub/Hideout & Economy *(new, CR-13, extraction pivot 2026-08-27)*

### OQ-BH-01 — Walkable hub or menu-driven screen flow? ✅ RESOLVED 2026-08-28 (dev-confirmed)
**Menu-driven screens — no hub level content.** "Returning to hub" opens a screen flow (stash/vendor/contract UI, reusing B1's `UZSUserWidgetBase`/`UZSUIManager` modal stack) directly, not a walkable space. `BH-T6` ("the hub space itself") is now scoped to `T6.1-alt` only — `T6.1`'s walkable-hub-level path is cut, not just deprioritized. See OQ-BR-01's resolution below for how this reconciles with a level-streamed transition still being needed mechanically.

### OQ-BH-02 — Per-player or shared stash in a hosted co-op world? ✅ RESOLVED 2026-08-28 (dev-confirmed)
**Per-player stash.** Each connected client gets its own separate stash/currency, not one shared pool per game instance. Real engineering work still needed: `UZSHubSubsystem`'s `Currency`/`Stash` need to key off the requesting player (`PlayerState` or equivalent) instead of being one flat `GameInstanceSubsystem`-wide pool — not built yet, tracked as the concrete next step on `BH-T1`. `BH-T3.2`'s contract-tracking scope question ("scoped per the same per-player-vs-shared question") is resolved the same way: per-player.

### OQ-BH-03 — Starting currency and first vendor's price scale 🟡
**Rec:** pick a provisional number once `BH-T2`'s vendor catalog exists, tune from there — not blocking `BH-T1`/`T2`'s construction, only their content.

---

## BR — Raid Lifecycle & Extraction *(new, CR-13, extraction pivot 2026-08-27)*

### OQ-BR-01 — Can one player leave a shared raid without ending it for teammates? ✅ RESOLVED 2026-08-28 (dev-confirmed)
**Level-streamed private sub-area.** A player who extracts or dies travels into a private streamed level while the shared raid level keeps running for everyone else on the same server. **Reconciled with OQ-BH-01's "menu-driven, no hub level" answer**: the private sub-area this streams into doesn't need to be a rich walkable space — it can be a minimal/empty holding level whose only job is to host the departing player's pawn while the menu-driven hub UI runs on top of it. The level-streaming mechanism and the hub's own content are two separate questions; this resolution is my synthesis of the two answers, not independently dev-confirmed as a single statement — flag if that reconciliation isn't what's intended. `AZSGameMode::Server_ReturnPlayerToHub` is the one call site that needs to change from its current `RestartPlayer`-in-place fallback to this real mechanism (`BR-T1.2`).

### OQ-BR-02 — Should extraction have a delay/channel time? 🟡
**Rec: yes, a short interruptible channel** (broken by taking damage) — an instant, uninterruptible extract undermines the genre's "the last moments of a raid are the tensest" pattern. Needs a real decision, not an assumption either way.

### OQ-BR-03 — Does entering the zone reload the level, or does it stay persistently loaded? ✅ RESOLVED 2026-08-28 (dev-confirmed)
**Level reload each raid entry.** Reseeds loot/zombie/hostile state for free via a fresh level load. **Consequence, already flagged when this was open:** dropped-loot persistence now needs to survive across reloads via real save data — this pulls a slice of `B3` (Persistence) forward sooner than `B3` would otherwise need to exist, since "loot stays where it fell across raids" (already a confirmed CR-13 requirement) can't be true across a level reload without saving it first. `BR-T3`'s raid-reseed task should build against this model, not the persistent-zone alternative.

---

## B2 — Art & Pipeline

### OQ-B2-01 — Asset budget ✅ RESOLVED 2026-07-26 — leans opposite the original rec
**Mostly free/cheap assets**, Door Kickers 2 as the fidelity benchmark; the dev expects to hand-model some assets himself over time (Blender pipeline, `GameDevPlan.md` §5). Don't assume a marketplace-kit-sized budget when scoping B2.

### OQ-B2-02 — Nanite ✅ RESOLVED 2026-07-26
**No Nanite.** Traditional LODs.

---

## B3 — Persistence

### OQ-B3-01 — Save topology and world lifetime ✅ RESOLVED 2026-07-26, ⚠ then a conflicting answer arrived
**One continuously-overwritten world** (not multiple save slots) — "so player can't load an old save to fix a mistake," no player-facing rollback. **Death always respawns into the same persistent world**, solo included — no asymmetric solo-ends-the-world rule; `Server_RespawnAsNewCharacter`'s existing behavior already matches this, nothing further to build. Rotating crash/corruption backups are unaffected (a different concern from save-scumming). See `00_MasterPlan.md` CR-07.

**Conflict resolved 2026-07-26.** Dev confirmed directly: "one continuously-overwritten world stays." The multi-save-slot answer above was not what was meant; no change to the decision.

### OQ-B3-02 — Serialization format ✅ RESOLVED 2026-07-26
**`USaveGame` + `FArchive`**, with a debug JSON export path for bug triage.

---

## B4 — World Content

### OQ-B4-01 — Region scale ✅ RESOLVED 2026-07-26 — bigger than the original rec
**Bigger than the original ~1×1 km proposal, built in phases** — driven partly by vehicles returning to scope (CR-02). Multi-biome: **urban, rural, wooded, and suburban** areas, not one uniform density. Structural consequence: region content is `B4X`, a continuous district-by-district track, not a single phase (`T_ContinuousTracks.md` T7) — validate per-district build time against B2-T4.5 as it goes.

### OQ-B4-02 — Named locations ✅ RESOLVED 2026-07-26
**Generic/functional names for now** ("Town Center," "Hospital") — mechanics first, confirmed twice. Real naming/flavor pass comes later, closer to B5. The mechanical-identity spread (medical site, firearms site, etc.) still governs placement.

### OQ-B4-03 — Interior visibility solution 🟡 partially resolved
**Approach confirmed: spike roof/floor-fade vs. camera-relative cutaway plane** against B2's reference room before committing — the wrong choice is a rebuild. Which technique wins is still open until the spike actually runs.

### OQ-B4-04 — Floor detection method ✅ RESOLVED 2026-07-26
**Authored floor volumes.**

### OQ-B4-05 — Zombie repopulation in cleared areas ✅ RESOLVED 2026-07-26
**Slow migration-based repopulation** from adjacent zones — diegetic, not a spawn timer; noise pulls it faster.

### OQ-B4-06 — Farming/foraging ✅ RESOLVED 2026-07-26
**Foraging zones only for beta.** Farming-lite stays post-beta, alongside the deferred Foraging skill.

### OQ-B4-07 — Does light attract zombies? ✅ RESOLVED 2026-07-26
**Yes.** "Zombies are attracted to light and sound" — light extends effective detection radius against the holder, same principle as noise.

### OQ-B4-08 — Locked doors, keys, lockpicking ✅ RESOLVED 2026-07-26 — adds a new skill
**Hybrid: breaching (forced entry, generates noise) *plus* a new levelable Lockpicking skill.** Lockpicking is a pure success-chance roll by skill level, no minigame; failed attempts generate noise (so spamming picks is a real risk); higher levels grant more speed and stealth. This reverses the original rec's own reasoning that a new skill wasn't warranted here. Add **Lockpicking** to `GameDevPlan.md` §3.1's skill list.

### OQ-B4-09 — Does rain mask noise? 🟡
**Rec: yes**, a modest reduction to effective noise radius.

### OQ-B4-10 — Day/night cycle length ✅ RESOLVED 2026-07-26
**Confirmed as a tunable.** ~2 real hours (night ~1/3) stands as the starting value to test from, not a locked number.

### OQ-B4-11 — Map discovery and teammate positions 🟡
**Rec:** map revealed by exploration; teammates shown only when nearby or a location is manually shared.

### OQ-B4-12 — Zombie AI depth pass: PZ-style behavioral fidelity 🟡 partially resolved
**Scope confirmed: a dedicated design+implementation pass at the start of B4, before B4-T7** (S–M, 2–3 sessions) — covering `BTTask_ClearLastKnownLocation` wiring, crowd-following/migration (feeds OQ-B7-01), sandbox-style per-world tunables (feeds OQ-B9-02), and door/obstacle destruction (feeds B4-T5.2). **New scope added 2026-07-26:** a zombie **"freshness" mechanic** — recently-turned zombies faster/stronger, degrading toward slower/weaker over time, likely a per-type curve on `UZSZombieConfig`. Confirmed zombie feel: "PZ style, but newer zombies are faster, zombies degrade slowly and slow down, don't do as much damage." Genuine large hordes (100+) confirmed important, not a cuttable stretch goal — see CR-08. The pass itself still needs to actually run.

---

## BF — Human Hostile AI Faction *(new, CR-13, extraction pivot 2026-08-27 — promotes `GameDevPlan.md` Decision 5 from deferred to core)*

### OQ-BF-01 — What does "guard" behavior actually mean? ✅ RESOLVED 2026-08-28 (dev-confirmed)
**Investigate noise like a zombie does.** A hostile reacts to `UZSNoiseSystem` events (gunfire, sprinting) by moving to investigate, the same behavior shape `BT_Zombie` already has — not a fixed patrol route. Consequence for `BF-T2`'s task breakdown: `T2.3` (investigate-noise) is now the primary defining "guard" behavior, not a secondary addition alongside patrol; `T2.2`'s "guard-point/patrol-route" framing is superseded by this answer rather than run alongside it — a guard's default state is holding position (already built, `BF-T1.2`'s stationary fallback) until noise or sight gives it something to investigate, mirroring zombie AI's own wander/investigate/chase shape rather than inventing a separate patrol system.

### OQ-BF-02 — One hostile archetype for beta, or roster variety? 🟡
Mirrors `OQ-B7-03`'s zombie-roster question. **Rec: start with one, add a second (heavier heist-guard variant) only if content time allows** — the multi-config rule makes a second archetype cheap once the first is proven.

### OQ-BF-03 — Do hostiles drop loot on death? 🟡
**Rec: yes** — at minimum their weapon/ammo, reusing `UZSLootTableConfig::RollLoot` the same way `AZSContainerActor` already does at `BeginPlay`. Matches the "guarded loot" framing directly; confirm rather than assume, since it changes `BF-T3`'s scope.

### OQ-BF-04 — Do hostiles and zombies fight each other? 🟡
**No rec yet.** A genuine three-way fight (player/zombie/hostile) is a more interesting heist scenario than two separate one-sided threats, but both `AZombieAIController` and `AZSHostileAIController` currently detect "Friendlies/Neutrals/Hostiles" equally (the existing v1 simplification) — saying yes means real faction-affiliation work on both classes, not a flag flip.

---

## B5 — Events & Investigation

### OQ-B5-01 — The actual plot 🟡 partially resolved
**Confirmed: brainstorm the actual plot together live, when B5 starts** — "still planning, skip for now." An earlier three-act placeholder shape floated here was never adopted; treat it as gone, not a draft. Tone (pulpy vs. grounded) and event roster count are also still genuinely open.

### OQ-B5-02 — Ambient event locatability ✅ RESOLVED 2026-07-26
**Ambient events are unlocatable; tangible events are investigable.** Make the distinction *audibly* learnable — ambient events should sound distant/directionless by design, so players learn the rule without being told it.

### OQ-B5-03 — Event escalation over time 🟡
**Rec: yes, weighted by `DayCount`** — early days favour ambient/opportunity events, later days favour horde/threat events.

### OQ-B5-04 — Event roster count 🔴
**Rec: 8–10 distinct event types for beta**, spread across ambient/opportunity/threat/narrative. Blocking since 2026-07-19.

### OQ-B5-05 — Voice acting 🟡
**Rec: text-only for beta.** Radio VO is a strong post-beta upgrade once the script stops moving.

### OQ-B5-06 — Clue/journal UI 🟡
**Rec:** a journal listing discovered clues, with map-pin integration. No quest markers, no objective arrows — diegetic framing is the point.

---

## B6 — Progression & Onboarding

### OQ-B6-01 — XP curves 🟡
**Rec:** 1→2 fast enough to feel within a session, 4→5 slow enough to represent real investment, ~2.2× geometric step. Tuned from B11 telemetry.

### OQ-B6-02 — Practice loops per skill 🟡
**Rec, one per skill** — Melee (practice swings, diminishing returns), Aiming (dry-aim + real shots), Reloading (manual cycling), Maintenance (maintenance actions), First Aid (treating wounds); Fitness/attributes emerge from normal play. **Governing rule:** practice must always be worse XP than real use.

### OQ-B6-03 — Perks and skill cap 🟢
**Rec: no perks; 1–5 is a hard cap for everyone.** Passive improvements only.

### OQ-B6-04 — Background roster 🟡 partially resolved
**Confirmed: a generic, data-driven background system** (`UZSBackgroundConfig`-style, new background = new data asset, zero C++) — build the system now, in Stage 1 (`B6-Sys`). **Actual roster/names still open** — dev will compile a full list later for `B6-Content`; an earlier six-name placeholder floated here was never adopted.

### OQ-B6-05 — Background tradeoffs ✅ RESOLVED 2026-07-26
**Must carry a real tradeoff, not purely additive** — e.g. additive skills paired with a harder starting-location risk (the Deputy starts well-armed but in a dense, dangerous town), reusing Decision 4's spawn system. Avoid designing a background with no wrong choice.

### OQ-B6-06 — Radio tutorial pacing 🟡
**Rec:** days 1–2 survival basics, 3–4 combat/injury, 5–6 the utilities-shutoff warning, day 7 transition into the investigation arc. Teach by describing what's happening in the world, never by naming a control.

### OQ-B6-07 — Death recap screen 🟢
**Rec: yes, minimal** — cause, day survived, one or two notable stats.

### OQ-B6-08 — Appearance customization 🟢
**Rec: minimal** — a small set of preset heads/bodies plus clothing colour, independent of background.

### OQ-B6-09 — New-game setup flow 🟡
**Rec:** world name → seed (optional) → difficulty (OQ-B9-02) → background (implies spawn) → scatter-spawns toggle for co-op → appearance, one screen where possible.

### OQ-B6-10 — Weapon repair mechanic ✅ RESOLVED 2026-07-30
**Ties to the existing Maintenance skill, not a new skill.** `B6-Content-T2.4` already has Maintenance reducing wear rate and jam chance passively; this adds an *active* repair action — consume a repair item/perform a maintenance action to restore durability, effectiveness scaled by Maintenance level — routed through the existing `Server_UseItem`/`EZSItemUseType` dispatch pattern. Without this, broken weapons just vanish; attrition alone was never the intended design, just an accidental byproduct of the gap.

### OQ-B6-11 — Skill books/magazines (PZ-style XP multiplier items) ✅ RESOLVED 2026-07-30 — considered and rejected
**Cut. Practice-driven XP only**, per the existing `B6-Content-T3` practice-loop design — no readable item grants a mechanical XP bonus. Recorded explicitly (rather than left silent) so this doesn't get re-raised as an unexamined gap; it was looked at against the PZ reference and turned down, not overlooked.

---

## B7 — Audio & Horde AI

### OQ-B7-01 — Horde coordination approach 🔴
**Rec: AI tick LOD + shared target grouping**, decided on B8-T2's measurements, not this recommendation. Ambition raised 2026-07-26: genuine large hordes (100+, visually distinct) confirmed important to the vision — CR-08 — which raises the bar for what counts as an acceptable answer, but the decision stays measurement-driven.

### OQ-B7-02 — Audio middleware ✅ RESOLVED 2026-07-26
**UE built-in + MetaSounds.** No paid middleware.

### OQ-B7-03 — Zombie roster for beta 🟡
**Rec: two types** — standard shambler and Crawler (reuses everything, differs in speed/height/detection profile; makes the downed-zombie mechanic meaningful).

### OQ-B7-04 — Music direction ✅ RESOLVED 2026-07-26
**Sparse and event-driven, not continuous.** A persistent score would mask the noise pillar's audio cues.

### OQ-B7-05 — Audio ducking rules ✅ RESOLVED 2026-07-30
**Define explicit priority rules in `B7-T1.3`'s mix hierarchy**: combat ducks ambience; critical-alert cues (critical head bleed, jam, low-health) duck both radio and ambience; radio ducks ambience but yields to critical alerts. Avoids leaving ducking behavior to implementation-time guesswork against a mix hierarchy that was otherwise silent on it.

---

## B8 — Performance

### OQ-B8-01 — Performance budget numbers 🔴 (re-baselined, numbers still pending measurement)
**Rec:** 60 FPS average / 45 FPS 1%-low at 1080p min spec, 150 concurrent zombies in view. Measure against **4-player** concurrent load (not 2), per OQ-X-03. Lower zombie count before lowering frame rate — but per CR-08's horde priority, treat that as a last resort and exhaust tick-LOD/optimization options first.

### OQ-B8-02 — Minimum hardware target 🔴
**Rec:** i5-8400 / Ryzen 2600, GTX 1060 6GB / RX 580, 16GB RAM. The listen-server host is the real min-spec case — it pays both server and client cost.

### OQ-B8-03 — VFX/particle budget ✅ RESOLVED 2026-07-30
**Add a VFX/particle budget to `B8-T1`, parallel to B7's audio concurrency budget** — draw-call and particle-count limits per effect type (blood, muzzle flash, horde-scale death/impact effects), especially load-bearing at 100+ zombie density (`CR-08`). B8 had rendering/gameplay/network sub-budgets but nothing for particles specifically; this closes that gap.

---

## B9 — Accessibility & Settings

### OQ-B9-01 — Gamepad support for beta ⚑ OVERTURNED 2026-07-30 — was ✅ RESOLVED 2026-07-23
**Gamepad support is cut for v1**, reversing the 2026-07-23 decision below. Dev call, made during a full plan gap-review pass: reduce scope rather than carry gamepad through B9. **What survives:** `B1-T2.4`'s generic focus-navigation base class stays — it's still required for keyboard-only accessibility (B1's own exit criteria: no screen may hardcode a mouse-only interaction), independent of gamepad. **What's cut:** `B9-T3.3`/`T3.4`'s gamepad-specific verification/binding work, PT2's gamepad-only playtest, and the "gamepad included" clause of B9's remap exit criterion. B9's size estimate likely shrinks as a result (not recomputed here). Console stays a possible later platform per `OQ-X-01`, unaffected by this — that's a distinct question from gamepad-on-PC support now.

*Original 2026-07-23 resolution, superseded above:* All gamepad work and testing deferred to B9 — not cut, just unverified/unpolished until then. Keep now (cheap): generic focus navigation on B1's widget base class, gamepad-mappable input actions, no mouse-only interactions. Defer to B9: per-screen navigation verification, gamepad-specific bindings/tuning, input glyph switching.

### OQ-B9-02 — Difficulty options 🟡
**Rec: three presets** (zombie density, loot scarcity, infection chance) plus the per-skill XP rate tunable, set at world creation and **locked for that world's lifetime.**

### OQ-B9-03 — Photo mode ✅ RESOLVED 2026-07-30
**Add a simple photo mode** (free camera + UI-hide toggle) as `B9-T5.6`, filler work per B9's own "best blocked-on-something-else filler" note. Cheap, and useful for `T_ContinuousTracks.md` T3's devlog/marketing track.

---

## B10 — Multiplayer & Release

### OQ-B10-01 — Dedicated servers ✅ RESOLVED 2026-07-26 — reverses the original rec
**Listen-server stays the default/free path; an optional paid dedicated-server hosting path is planned** for groups who want one. Real new B10 scope (hosting infrastructure, likely third-party or self-hosted — needs its own design pass, not assumed free).

### OQ-B10-02 — Steam/EOS networking ✅ RESOLVED 2026-07-26
**Steam networking (Steam Sockets).** No port forwarding — the single biggest practical barrier to testers actually connecting — plus friends-list invites.

### OQ-B10-03 — Late-join model 🟡
**Rec: full late-join into a running world.** Anything less makes co-op scheduling-dependent.

### OQ-B10-04 — Disconnect handling 🟡 — dev asked for design help; proposal below, not yet confirmed
Dev's stated goals: a safe voluntary way to leave, a safe respawn back in, and **no exploit path to escape a losing fight by disconnecting.** Proposed direction (needs a one-line go/no-go, not yet built):
- **Two distinct exits.** A deliberate "log out safely" action reuses OQ-B0-06's `IsSafeToSleep()` gate (not recently hunted + real shelter) — if it passes, the character despawns cleanly and reappears at that exact spot on rejoin.
- **Server-tracked "in combat" flag** (set on dealing/taking damage, clears after N seconds of no combat) blocks the safe-logout action while active — the same anti-combat-log idea most co-op/MMO games use, built on state the server already tracks.
- **Any non-graceful disconnect while flagged in-combat** (alt-F4, crash, network drop) falls back to the original rec: character remains in-world and vulnerable for a short grace window (~60s), then despawns with items intact. A genuine crash isn't punished; pulling the plug mid-fight isn't a clean escape either.
- Ties into T7.4's existing "teammate can move a downed body" mechanic for the co-op case.

### OQ-B10-05 — Host migration 🟢
**Rec: not supported.** Host leaving ends the session; save cleanly before closing it.

### OQ-B10-06 — Tester distribution channel 🟡
**Rec: Steam playtest** — OQ-B10-02 already lands on Steam networking.

### OQ-B10-07 — Telemetry scope 🟡
**Rec: minimal and opt-in** — session length, death cause/location/day, kills, loot, skill levels at death.

### OQ-B10-08 — Branching strategy 🟡
**Rec:** `main` for development, `release/*` for tester builds, cherry-pick hotfixes. Never force-push `main`.

### OQ-B10-09 — Voice chat 🟢
**Rec: none — rely on Discord.**

### OQ-B10-10 — Trading/economy ✅ RESOLVED 2026-07-30
**Drop the `B10-T2.4` dupe-test reference to "trading."** No dedicated trade UI/mechanism exists or is being built — the reference was an inconsistency (something to test that was never actually scoped). Players trade informally by dropping items on the ground; existing drop/pickup already covers it.

### OQ-B10-11 — Non-verbal comms (ping wheel) ✅ RESOLVED 2026-07-30
**Add a simple ping wheel** — map/world-space ping with a few canned callouts ("here", "help", "zombie") — given voice isn't guaranteed for 4+ player co-op even with text chat (`OQ-X-11`) now in scope. New B10 sub-task, minimal HUD investment.

### OQ-B10-12 — Admin/moderation tools ✅ RESOLVED 2026-07-30 — broader than originally scoped
**Build host-level kick/ban/whitelist now**, via host-gated console commands (same pattern as `ZS.SpawnZombies`), for **any** listen-server host — not just the paid dedicated-server path. Rationale (dev-stated): a listen-server host inviting strangers into their world needs the same moderation capability a dedicated-server admin would. New `B10-T1` sub-task; uses the scoreboard/player-list UI (`OQ-B1-07`) as its target-selection surface.

### OQ-B10-13 — Anti-cheat posture ✅ RESOLVED 2026-07-30
**Rely on the existing server-authoritative architecture; no dedicated anti-cheat system.** All damage/inventory mutation already routes through `Server_` RPCs gated on `HasAuthority()` — treated as sufficient given the project's scale and co-op-not-competitive design. `B10-T2.3`'s authority audit is the relevant verification step, not a new system.

### OQ-B10-14 — Achievements ✅ RESOLVED 2026-07-30
**Add Steam achievements**, now that Steam is the confirmed launch platform (`OQ-B10-02`) — straightforward Steamworks API integration, real store-page value. New B10 sub-task alongside the Steam networking work.

---

## B11 / B12 — Beta Program

### OQ-B11-01 — Feedback channel 🟡
**Rec: Discord for unstructured impressions + a structured form for bugs.**

### OQ-B11-02 — Tester recruiting 🟡
**Rec: 8–10 testers**, mixed survival-genre veterans and newcomers.

### OQ-B12-01 — Pricing ✅ RESOLVED 2026-07-26
**~$9.99**, "to encourage people to buy it" — a target/anchor, not a final locked number.

### OQ-B12-02 — Early Access vs. single launch ✅ RESOLVED 2026-07-26
**Early Access, Steam-only at first.** Also settles part of OQ-B10-02/B10-06.

### OQ-B12-03 — Demo 🟡
**Rec: no separate demo for beta.** Reconsider for a Steam Next Fest after Early Access launch.

### OQ-B12-04 — Localization 🟢
**Rec: English-only for beta**, with the `FText` + string-table pipeline in place from B9 onward.

### OQ-B12-05 — Public bug pipeline 🟡
**Rec: GitHub Issues** (repo already public, labels/Projects board set up) + a Discord channel for the reports that never make it to a tracker.

### OQ-B12-06 — EULA / ToS / privacy policy ✅ RESOLVED 2026-07-30
**Add before any public release** (B12). Needed for the Steam store listing and to disclose the opt-in telemetry (`OQ-B10-07`). New `B12-T1` sub-task.

### OQ-B12-07 — Age rating ✅ RESOLVED 2026-07-30
**Plan for it pre-release.** Steam requires at minimum an IARC questionnaire before the store page can go live. New `B12-T1` sub-task, quick but must land before `B12-T2`'s store-page work finishes.

### OQ-B12-08 — Credits screen 🟢
**Not urgent — address if necessary later.** Most current assets are placeholders for testing; a real attribution/credits pass only matters once real licensed/third-party content is locked. No task added yet, just this reminder so it isn't forgotten when that happens.

---

## Summary — questions by priority

> Full per-question reasoning lives above; this table is the fast-scan index.

**✅ RESOLVED, dev-confirmed (44)**

| Question | Answer | Date |
|---|---|---|
| **OQ-X-01** Platform commitment | PC only for initial launch. | 2026-07-23 |
| **OQ-X-02** Win condition | Survive indefinitely; new quest-list idea, content TBD. | 2026-07-26 |
| **OQ-X-03** Player-count ceiling | **4+**, not hard-locked 2–4 (reverses original rec). | 2026-07-26 |
| **OQ-X-06** NPCs | Hostile roamers confirmed in scope; friendly survivor NPCs stay post-release. | 2026-07-26 |
| **OQ-X-07** Tone | Grounded, restrained horror reconfirmed as-is. | 2026-07-26 |
| **OQ-B9-01** Gamepad support | In scope, all work deferred to B9. | 2026-07-23 |
| **CR-01** Skill roster | `GameDevPlan.md` §3.1's longer list. | 2026-07-26 |
| **CR-02** Vehicles | Not cut — later in dev, ready for beta (reverses original rec). | 2026-07-26 |
| **CR-03 / OQ-B0-04** Temperature/Wet scope | Keep all three, scoped-down model. | 2026-07-26 |
| **CR-04** Camera fallback | Cut now, not gated on a sign-off. | 2026-07-26 |
| **CR-06 / OQ-B0-07** Infection legibility | Plain/clear feedback, **not** ambiguous (reverses original rec). | 2026-07-26 |
| **CR-07/12 / OQ-B3-01** Save topology & death rule | One continuously-overwritten world; death always → new character, world persists. Confirmed twice, conflict resolved. | 2026-07-26 |
| **CR-08** Horde ambition | Genuinely large hordes (100+) confirmed important, not a cuttable stretch goal. | 2026-07-26 |
| **CR-10 / OQ-B0-05** Fatigue perception | Presentation degradation — player's own perception degrades. | 2026-07-26 |
| **OQ-B0-01** Scroll-wheel arbitration | Scroll = zoom; hotbar drops scroll-cycling entirely. | 2026-07-26 |
| **OQ-B0-02** Aim-cone & headshot values | "About right" / KEEP — dev-approved starting numbers. | 2026-07-26 |
| **OQ-B0-03** Downed-zombie/stomp | KEEP; non-PZ-clone finisher: bare-handed → stomp, weapon equipped → swing-down strike. | 2026-07-26 |
| **OQ-B0-06** Sleep vulnerability | Gated on real prep (barricade/locked door/vehicle) + not currently hunted. | 2026-07-26 |
| **OQ-B0-08** Bite-infection timeline | 2–4 in-game day window; pairs with the corpse-becomes-zombie-holding-loot mechanic. | 2026-07-26 |
| **OQ-B0-09** Ammo as inventory item | KEEP, with all 4 bundled item-instance additions. | 2026-07-26 |
| **OQ-B0-10** `IA_SecondaryAction` binding | Stays flexible; full rebinding already covered by B9-T3. | 2026-07-26 |
| **OQ-B0-11** Melee weapon display | Grouped poses by weapon category (long-gun / pistol / melee). | 2026-07-26 |
| **OQ-B0-13** Item-instance refactor | Do it now, as independently-testable steps, not one block. | 2026-07-26 |
| **OQ-B1-01** UI art timing | Functional-grey now, restyle after B2. | 2026-07-26 |
| **OQ-B1-02** HUD density | Contextual, made player-configurable in B9. | 2026-07-26 |
| **OQ-B2-01** Asset budget | Mostly free/cheap, DK2 fidelity bar, self-modeled assets expected later. | 2026-07-26 |
| **OQ-B2-02** Nanite | No Nanite. | 2026-07-26 |
| **OQ-B3-02** Serialization format | `USaveGame` + `FArchive`, with debug JSON export. | 2026-07-26 |
| **OQ-B4-01** Region scale | Bigger than 1×1 km, built in phases (`B4X`), multi-biome. | 2026-07-26 |
| **OQ-B4-02** Named locations | Generic/functional names now; real naming pass later. | 2026-07-26 |
| **OQ-B4-04** Floor detection | Authored floor volumes. | 2026-07-26 |
| **OQ-B4-05** Zombie repopulation | Slow migration-based repopulation from adjacent zones. | 2026-07-26 |
| **OQ-B4-06** Farming/foraging | Foraging zones only for beta; farming-lite stays post-beta. | 2026-07-26 |
| **OQ-B4-07** Light attracts zombies | Yes — same principle as noise. | 2026-07-26 |
| **OQ-B4-08** Locked doors/lockpicking | Hybrid: breaching **plus** a new levelable Lockpicking skill. | 2026-07-26 |
| **OQ-B4-10** Day/night cycle length | Confirmed as a tunable; ~2 real hours the starting value. | 2026-07-26 |
| **OQ-B5-02** Ambient event locatability | Ambient unlocatable, tangible investigable, audibly learnable. | 2026-07-26 |
| **OQ-B6-05** Background tradeoffs | Real tradeoffs, not purely additive. | 2026-07-26 |
| **OQ-B7-02** Audio middleware | UE built-in + MetaSounds confirmed. | 2026-07-26 |
| **OQ-B7-04** Music direction | Sparse/event-driven confirmed. | 2026-07-26 |
| **OQ-B10-01** Dedicated servers | Optional paid path, not cut (reverses original rec). | 2026-07-26 |
| **OQ-B10-02** Steam/EOS networking | Steam networking (Steam Sockets). | 2026-07-26 |
| **OQ-B12-01** Pricing | ~$9.99 target. | 2026-07-26 |
| **OQ-B12-02** Early Access vs. single launch | Early Access confirmed, Steam-only at first. | 2026-07-26 |

**🟡 Partially resolved — direction confirmed, specifics still deferred (6)**

| Question | What's resolved | What's still open |
|---|---|---|
| **OQ-B0-12** Weapon roster | Melee roster size ("right," 4–6). | Firearm roster — dev will provide once basic features are set. |
| **OQ-B4-03** Interior visibility | Approach confirmed: spike roof-fade vs. camera-relative cutaway. | Which technique wins — the spike hasn't run yet. |
| **OQ-B4-12** Zombie AI depth pass | Scope confirmed, plus a new "freshness" mechanic added. | The pass itself (crowd-following, `ClearLastKnownLocation` wiring) still needs to run. |
| **OQ-B5-01** The plot | Confirmed: brainstorm together live when B5 starts, not now. | The actual plot — genuinely still nothing, by design. |
| **OQ-B6-04** Background roster | Confirmed: generic, data-driven system, build now (`B6-Sys`). | The actual roster/names — dev's own list, later (`B6-Content`). |
| **OQ-B10-04** Disconnect handling | Dev's goals stated (safe exit/respawn, no combat-log exploit); a proposal is drafted above. | Needs a go/no-go on the proposed anti-exploit design. |

**🔴 Still BLOCKING (5)** — resolve before the named phase starts. All 5 CR-13 (BH/BR/BF) BLOCKING items were resolved 2026-08-28 — see `OQ-BH-01`/`OQ-BH-02`/`OQ-BR-01`/`OQ-BR-03`/`OQ-BF-01` above.

| Phase | Questions |
|---|---|
| Before B5 | OQ-B5-04 (event roster count — genuinely still open, tone also open) |
| Before B7 | OQ-B7-01 (horde-coordination *approach* — still gated on profiling measurements, ambition is confirmed but the technical answer isn't) |
| Before B8 | OQ-B8-01, OQ-B8-02 (budget numbers — re-baselined for 4+ players, but still pending actual measurement) |

**🟡 SEQUENCEABLE (~36)** — decide in parallel with early implementation on that phase. Includes three items from `Docs/InputBindings.md` (OQ-X-09 Run/Sprint tiers, OQ-X-10 Toggle Safety/PvP, OQ-X-11 chat/voice) and five new CR-13 items (`OQ-BH-03`, `OQ-BR-02`, `OQ-BF-02`, `OQ-BF-03`, `OQ-BF-04`).

**🟢 LATE (10)** — OQ-X-05, OQ-X-08, OQ-B6-03, OQ-B6-07, OQ-B6-08, OQ-B10-05, OQ-B10-09, OQ-B12-03, OQ-B12-04, OQ-B12-05.

**2026-07-30 gap-review pass** — a full sweep of the plan against what a shipped game needs turned up 28 items, each given a real decision that session (see the individual entries above, not re-tabulated here to avoid drift): `OQ-B0-16/17/18`, `OQ-B1-04/05/06/07`, `OQ-B6-10/11`, `OQ-B7-05`, `OQ-B8-03`, `OQ-B9-03`, `OQ-B10-10/11/12/13/14`, `OQ-B12-06/07/08`. One reversal: `OQ-B9-01` (gamepad, cut). One partial resolution: `OQ-X-11` (text chat built, voice half stays open under `OQ-B10-09`).

**2026-08-03 weekly-scoping pass** — 3 more issues closed: `OQ-B1-03` (solo pause → no pause ever), `OQ-B1-08`/issue #4 (container interact → real loot screen), `OQ-B1-09`/issue #3 (SecondaryHand action trigger → already shipped via `IA_SecondaryAction`'s existing dispatch, not a fresh decision).
