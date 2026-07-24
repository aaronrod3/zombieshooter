# B9 — Accessibility, Settings & Sandbox Options

**Size: M (8–10 dev-sessions)** · **Gate: `[PUBLIC]`** · **Depends on: B1** · **Blocks: B11**

> **This gates the internal beta, not just the public one.** Testers without a settings menu or remappable controls generate noise-bugs about hardware and preference that drown the real findings. A tester who cannot rebind a key reports "the controls are bad" instead of the actual bug you needed to hear about.
>
> Most of this can start any time after B1 and runs well as filler work when blocked on B4's content.

## Entry criteria

- [ ] B1 complete — settings need a menu framework and B1-T8.3 left the entry point stubbed for exactly this.
- [ ] B0 complete — control scheme final (perspective toggle removed, `IA_SecondaryAction` added, scroll arbitration resolved).
- [ ] **OQ-B9-01 answered** — gamepad support: required for beta or keyboard/mouse only?

## Exit criteria

- [ ] Every input is remappable, including gamepad if OQ-B9-01 says yes.
- [ ] Settings persist across sessions and apply without restart wherever technically possible.
- [ ] A colorblind player can distinguish every gameplay-critical UI state.
- [ ] Text is readable at 1080p on a couch-distance display.
- [ ] Difficulty options exist and are honest about what they change.

---

## Task breakdown

### B9-T1 — Settings framework · **S (2 sessions)**

| Sub-task | Definition of done |
|---|---|
| T1.1 | `UZSSettingsSubsystem` on top of `UGameUserSettings`. Persists to config; loads at startup. |
| T1.2 | Settings menu built into B1-T8.3's stub, reachable from both main menu and in-game. |
| T1.3 | Category structure: Video, Audio, Gameplay, Controls, Accessibility. |
| T1.4 | Apply/revert/reset-to-default per category, with a **countdown revert on video changes** so an unsupported resolution isn't a soft-lock. |

### B9-T2 — Video & audio settings · **S (2 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T2.1 | Resolution, window mode, VSync, frame limit. |
| T2.2 | Quality presets (Low/Medium/High/Epic) mapped to real scalability groups, plus individual overrides. Presets must be **validated against B8's min-spec target** — a "Low" that doesn't hit target on min spec is a lie. |
| T2.3 | Brightness/gamma. **Load-bearing here**, because B4-T4's darkness mechanic means a miscalibrated display either breaks the mechanic or makes the game unplayable. Include a calibration screen. |
| T2.4 | Master/SFX/Music/UI/Voice volume, bound to B7-T1.3's sound-class hierarchy. |
| T2.5 | Audio output device selection and a mono-audio option (accessibility). |

### B9-T3 — Control remapping · **M (3–4 sessions)** · *depends on T1*

| Sub-task | Definition of done |
|---|---|
| T3.1 | Full keyboard/mouse remapping via Enhanced Input's user-settings system (`UEnhancedInputUserSettings`) — the supported path, not a hand-rolled one. |
| T3.2 | Conflict detection and resolution UI. |
| T3.3 | **Gamepad support pass** per OQ-B9-01. The top-down twin-stick scheme was chosen partly for gamepad-friendliness (`GameDevPlan` §1), and `IA_HotbarCycle` was made first-class specifically to map to a bumper/d-pad — so most of the design work is already done. Verify P1-R9's status from B0-T0.5. |
| T3.4 | Gamepad UI navigation for every B1 screen (B1-T2.4 built the generic path; this verifies every screen honours it). |
| T3.5 | Mouse sensitivity, invert options, and hold-vs-toggle for aim, crouch, and sprint. |
| T3.6 | Input glyphs switch automatically between keyboard and gamepad. |

### B9-T4 — Accessibility · **M (2–3 sessions)** · *depends on T1, B1*

| Sub-task | Definition of done |
|---|---|
| T4.1 | **Colorblind modes** (protanopia/deuteranopia/tritanopia). Audit every gameplay-critical UI state: moodle severity, wound state, durability, hotbar selection, interaction prompts. **Colour must never be the only channel** — pair it with shape, icon, or position. |
| T4.2 | **Text size scaling** across all UI, verified for overflow at the largest setting. |
| T4.3 | **Subtitles/captions** for radio broadcasts (B5-T3) and any spoken content. If OQ-B5-05 lands on text-only radio, this is largely satisfied — but zombie/environmental audio cues still want an optional visual indicator, which matters a lot for a game where audio is the threat-detection channel. |
| T4.4 | Motion/camera-shake reduction option. |
| T4.5 | Hold-to-toggle alternatives for every held input (already partly T3.6). |
| T4.6 | UI contrast option for low-vision players. |
| T4.7 | **Screen-reader support is POST-BETA** — record the decision explicitly rather than leaving it unaddressed. |

### B9-T5 — Gameplay & difficulty options · **S (2 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | **Per-skill XP rate tunable exposed** (CONFIRMED) — the one sandbox-adjacent setting explicitly promised for v1. | X-8 |
| T5.2 | Difficulty options → OQ-B9-02. Must be **honest**: state exactly what each changes (zombie density, loot scarcity, infection chance). Hidden difficulty scaling contradicts the transparency pillar. |
| T5.3 | Where difficulty is set: new-game only, or adjustable mid-world? In a persistent-world co-op game, mid-world changes are a fairness question → OQ-B9-02. |
| T5.4 | **Full PZ-style sandbox slider suite stays POST-BETA** (`GameDevPlan` §3, CUT v1). T5.1 and T5.2 are the deliberate exceptions. |
| T5.5 | Radial quick-use menu, deferred here from B1 — the lowest-priority UI item in `UI_Plan.md`, and the hotbar already covers instant re-equip. Build only if time allows; it is a genuine cut candidate. |

---

## Playtest checkpoints

| ID | When | What is tested | Pass condition |
|---|---|---|---|
| **PT1** | End of T3 | **Full remap run**: rebind every action to something unusual, then play a full session. | Nothing is hardcoded. No conflicts slip through. Glyphs follow the rebind. |
| **PT2** | End of T3 | **Gamepad-only session** (if OQ-B9-01 = yes). Play 30 minutes with the keyboard unplugged, including inventory, container looting, and the map. | Every screen is navigable. Combat feels controllable. Hotbar cycling works as designed. |
| **PT3** | End of T4 | **Accessibility audit** — play with each colorblind mode active and with max text size. Then play with audio muted. | No gameplay-critical state is colour-only. No text overflows. Muted play is hard but possible, revealing where a visual audio cue is needed. |
| **PT4** | B9 exit | **Fresh-install first-run**: default settings on min-spec hardware. | Defaults are playable and sane. Darkness reads correctly at default gamma. Nothing requires a settings visit before playing. |

## Notes

- **Localization is B12's**, not B9's — but B9 must not hardcode UI strings. Route everything through `FText` and a string table from the start; retrofitting is far more expensive than doing it right in the same edit.
- **T4.3's visual audio cue is worth more than it looks.** In a game where audio is the primary threat-detection channel, a deaf or hard-of-hearing player is playing a fundamentally harder game. An optional directional threat indicator is the single highest-impact accessibility feature available here.
- **B9 is the best "blocked on something else" filler in the plan.** Most tasks depend only on B1 and can absorb odd sessions during B4's long content grind.
