# Input Bindings Reference

> **Living reference, like `TuningReference.md` but for controls — update this whenever a binding changes or a new action is added.** Source of record for the dev's actual intended keymap; `IMC_ZS_Default`/`IMC_ZS_MouseLook` (and whatever vehicle/UI mapping contexts follow) should track this, not the other way around. First drafted 2026-07-26 from the dev's own keybind list, given directly in chat — **not yet implemented**, this is the target scheme.

**Key column format:** default PC binding. Every input is rebindable via `B9_Accessibility_Settings.md` B9-T3 (Enhanced Input's `UEnhancedInputUserSettings`) — this table is the shipped default, not a hardcoded lock.

## Movement

| Action | Key |
|---|---|
| Forward / Backward / Left / Right | W / S / A / D |
| Run | Left Shift (hold) |
| Sprint | Double-click Left Shift |
| Sneak (Crouch) | Left Ctrl |
| Interact | F |
| Cancel Action | Move, or start another action (no dedicated key) |

## Combat

| Action | Key |
|---|---|
| Aim | Right Mouse Button (toggle **and** hold both work) |
| Fire / Attack | Left Mouse Button |
| Melee Shove / Stomp / Mount / Climb | Space |
| Reload Weapon | R |
| Rack Firearm (chamber / clear jam) | Alt + R |
🔧 **B0-T10.1/T10.2/T10.6 implemented 2026-07-26**: only the confirmed halves — clear-jam (`AZSPlayerCharacter::RackAction`/`StartRackFirearm`) and the finisher (`FinisherAction`/`HandleFinisher`, stomp/weapon-strike over a downed zombie only). Shove and Mount/Climb (bundled on the same Space input) remain unimplemented — genuinely undesigned, not an oversight. Both `IA_Rack`/`IA_Finisher` still need manual creation in-editor as `.uasset`s.
| Toggle Safety (multiplayer PvP) | X |

## Camera

| Action | Key |
|---|---|
| Zoom In | `=` / Mouse Wheel Up |
| Zoom Out | `-` / Mouse Wheel Down |

## UI

| Action | Key |
|---|---|
| Toggle Inventory | Tab |
| Toggle Map | M |
| Main Menu | Escape |
| UI Select | Left Mouse Button |
| UI Cancel | Escape |
| UI Navigate (focus movement within a menu) | Arrow Keys |
| Show Scoreboard / Player List | `P` (proposed default, added 2026-07-30 — `OQ-B1-07`, dev-adjustable) |

🔧 **B1-T1 implemented 2026-07-30**: `UZSUIManager` (`ULocalPlayerSubsystem`, `Source/ZombieShooter/UI/`) owns the modal stack that adds/removes `IMC_ZS_UI` at a higher Enhanced Input priority than `IMC_ZS_Default`. **Content gap**: `IMC_ZS_UI` and its three actions (`IA_UISelect`, `IA_UICancel`, `IA_UINavigate`) don't exist as `.uasset`s yet — same graceful-if-missing pattern as every other input asset in this project, needs manual creation in-editor:
- `IA_UISelect` (Digital bool) — Left Mouse Button, Pressed.
- `IA_UICancel` (Digital bool) — Escape, Pressed.
- `IA_UINavigate` (Axis2D) — Arrow Keys (Up/Down/Left/Right as +Y/-Y/-X/+X via the standard WASD-style modifier setup already used on `IA_Move`). Deliberately **not** WASD — movement must keep working while a menu is open (Decision 1, no pause layer), so UI focus navigation needs its own keys.
- `IMC_ZS_UI` — add all three above. Left Mouse Button here is what's meant to out-prioritize `IMC_ZS_Default`'s own Left Mouse Button → `IA_Attack` binding once a modal is open; `UZSUIManager::PushModal` adds this context at priority 10 (`IMC_ZS_Default` is added at priority 0 in `AZSPlayerController::SetupInputComponent`).
`IA_UINavigate`'s actual focus-cycling logic is T2.4's job (generic base-class focus navigation), not T1's — T1 only needs the mapping context/actions to exist so T2 has something to bind to.

🔧 **B1, 2026-08-05 implemented**: `AZSPlayerCharacter::ToggleInventoryScreen`/`TogglePauseMenuScreen` (C++ written, not yet compiled — see `SessionHandoff.md`). **Content gap**: `IA_ToggleInventory` and `IA_TogglePauseMenu` don't exist as `.uasset`s yet, same graceful-if-missing pattern:
- `IA_ToggleInventory` (Digital bool) — Tab, Pressed, in `IMC_ZS_Default`.
- `IA_TogglePauseMenu` (Digital bool) — Escape, Pressed, in `IMC_ZS_Default` — deliberately not `IMC_ZS_UI`, since this action must also fire with no modal open (to open Pause in the first place); the open-vs-close branch is decided in C++ (`TryCloseTopmostScreen`), not by which mapping context is layered on top.
Escape's dual role above ("Main Menu" and "UI Cancel" are the same key) is handled by `TryCloseTopmostScreen()` closing whichever of Inventory/SleepPrompt/PauseMenu is open instead of opening Pause on top of it — `IA_UICancel` itself still has no bound handler (T2.4's focus-navigation-only scope), that's unrelated to this.

## Hotbar / Items

| Action | Key |
|---|---|
| Hotbar Slots 1–9 | 1–9 |
| Equip / Toggle Light Source | `T` (assignable) — 🔧 implemented 2026-07-26 as `AZSPlayerCharacter::SecondaryAction`/`HandleSecondaryAction` (B0-T11.2) |

## Vehicles (`BV` phase — first pass at this keymap, vehicles not built yet)

| Action | Key |
|---|---|
| Start Engine | F |
| Toggle Headlights | T |
| Vehicle Heater | O |
| Vehicle Info | U |
| Horn | Space |
| Vehicle Radial Menu | V |
| Switch Seat | Z |

## Multiplayer / Social

| Action | Key |
|---|---|
| Toggle Chat | Enter |
| Push-to-Talk | V |
| Ping Wheel | Middle Mouse Button, hold (proposed default, added 2026-07-30 — `OQ-B10-11`, dev-adjustable) |

---

## Notes — what this changes or adds versus the current plan

This list resolves a couple of already-open questions and surfaces a few genuinely new mechanics that weren't previously scoped anywhere. Flagging both kinds so neither gets lost.

**Resolves existing open questions:**
- **OQ-B0-01 (scroll-wheel arbitration)** — consistent with the existing resolution: scroll wheel is zoom, `=`/`-` are the keyboard alternative. 🔧 **Implemented 2026-07-26** (`Docs/Beta/B0_Stabilization.md` T3.4): `AZSPlayerCharacter::ZoomAction` (Axis1D) → `HandleZoom` → `UZSCameraDirector::ApplyManualZoom`. `IA_HotbarCycle`/`CycleHotbar` were removed rather than rebound. **Content gap**: `IA_Zoom` doesn't exist as a `.uasset` yet — needs manual creation in-editor (Axis1D, mouse wheel + `=`/`-` Scalar modifiers in `IMC_ZS_Default`).
- **OQ-B0-10 (`IA_SecondaryAction` / light source binding)** — settles on **`T`** for light-source toggle. Note `F` is spoken for by **Interact**, not this action — the earlier assumption that `F` would double as the light toggle no longer holds now that Interact has its own dedicated key. Update `B0_Stabilization.md` T11.2 to bind `T`, not `F`.

**Changes an already-written task — needs a doc update, not just tracking:**
- **OQ-B0-03 / `B0_Stabilization.md` T10.6 (downed-zombie finisher)** — this list puts the finisher on **Space**, bundled with Shove/Mount/Climb as one context-aware action, **not** contextual on `IA_Attack` as the prior resolution said. Also reveals two mechanics not previously scoped at all: a **Shove** action and a **Mount/Climb** action, sharing the same input as the stomp/swing-down finisher. T10.6 needs updating to Space, and Shove/Mount/Climb need their own task entries once designed.
- **B0-T10.1/T10.2 (jamming)** — "clear the jam" now has a real name and key: **Rack Firearm, Alt+R**. This also implies **racking is a distinct action from reloading** (chambering a round manually), not just a jam-clear button — worth confirming whether racking has any use outside clearing a jam (e.g. required after reloading certain weapon types) before implementing.

**Genuinely new mechanics, not yet designed anywhere — flagged for a design pass, not implemented from this table alone:**
- **Run vs. Sprint as two distinct speed tiers** (Run = hold Shift, Sprint = double-click Shift). Current code/design only has one sprint tier (`StartSprint`, stamina-gated). Needs a design decision: is Run a free, non-stamina-costing jog between walk and sprint, or something else? Affects `UZSNeedsComponent`'s stamina model.
- **Toggle Safety (PvP)** — a weapon-safety mechanic with no prior design anywhere in the plan. Needs scoping: what does "safety on" actually prevent (accidental discharge? friendly-fire specifically?), and is it PvP-only given this project has no designed PvP mode yet.
- **Text chat (Enter) and in-game Push-to-Talk voice (V)** — neither exists in the plan. **This directly contradicts OQ-B10-09's standing recommendation** ("no voice chat, rely on Discord") — that question was never dev-confirmed, just a recommendation, so this isn't a reversal of a decision, but it does mean OQ-B10-09 needs a real answer now rather than defaulting to the old rec. Text chat as a system is new scope for B10 either way.

Added these as open items to `Docs/Beta/90_OpenQuestions.md` (new OQ-X-09 through OQ-X-11) rather than leaving them only in this note, so they surface in the normal open-questions sweep.

**Update 2026-07-30:** `OQ-X-11` is now resolved — text chat (Enter) is being built, voice (Push-to-Talk, V) stays unimplemented pending its own separate answer under `OQ-B10-09`. Also added this session: a Scoreboard/Player List key (UI table) and a Ping Wheel key (this table) — see `OQ-B1-07` and `OQ-B10-11`.
