# B1 UI Design Session — 2026-07-30

> Captures design decisions from a planning conversation held ahead of actual B1 implementation (dev is finishing B0 hands-on first). Supplements `Docs/Planning/UI_Plan.md` and `Docs/Beta/B1_UI_UX.md` — read those first for the baseline plan; this doc records what changed or was newly decided since. Mockups were sketched interactively during the session (not saved as image assets); the descriptions below are the source of record.

## Default HUD (locked)

Minimal, milsim-inspired direction: the player is expected to rely on remembering their own condition, plus audio/visual character cues, rather than an always-on stat readout. E.g. "I need to eat/drink" or "clear my jam" should be picked up by feel/cue, not a persistent bar or icon.

- **Hotbar** — always visible, 9 slots (existing).
- **Stamina bar** — contextual: appears only when below 100%, disappears at full. The one exception to the "no meters" rule, since it directly gates sprinting decisions moment-to-moment.
- **Critical head bleed alert** — always-on when active (matches the already-resolved OQ-B1-02: "anything that can kill you in under a minute stays always-on regardless").
- **Interaction prompt** — screen-space, fixed position (not world-space/floating), bound to `F` (not `E`).
- **Body-zone wound diagram** — small, corner-placed, non-obstructing. Contextual: appears only while any zone carries an active wound, hidden at full health (same reveal-on-change pattern as the stamina bar).
- **Ammo counter and other HUD readouts** — built as real, on-by-default elements now, but wired through a settings toggle so they can be switched off later after testing. Pulls a slice of B9's existing "HUD density, player-configurable" plan forward as a simple toggle rather than waiting for a full settings screen.

Everything else (needs levels, detailed wound state) is deliberately *not* on the default HUD — it lives in the Tab menu below.

## Tab menu (opens on `Tab`)

- **Full-tab-swap structure**: a **Loadout / Stats / Skills** strip. Each tab replaces the entire window content — Loadout is never fragmented; it's always the complete Player + Gear + Weapon-mounts + Inventory set shown together. (Rejected alternative: keeping Inventory permanently visible while only the Player/Equipment panel swaps — full-swap was chosen for simpler widget architecture and because Loadout's job is fast drag-and-drop management, while Stats/Skills is a deliberate, slower destination that shouldn't compete for the same space.)
- **Needs** (hunger/thirst/fatigue/temperature/etc.) live inside the Loadout tab's player panel — not their own tab, not on the main HUD.
- **Stats and Skills tabs are placeholders until B6** (skill/progression system) actually exists. This is a sequencing dependency for B1, not something to build content for now.
- **Loadout layout**: player silhouette with Gear slots (Backpack / Hip / Flashlight) arranged beside it to the right; Weapon-mount slots below that row; Needs bars in between. Inventory compartments fill the right side of the window.
- **Gear slot visibility rule**: a Backpack or Duffle gear slot/section should not render at all unless that container is actually equipped — no empty placeholder shown for an unequipped container slot.

## Inventory / carry model

- **Three compartments**:
  - **Pockets** (on-body) — small items only, always available, quickest access.
  - **Backpack** — medium capacity, quicker to access than the Duffle.
  - **Duffle** — largest capacity, adds a movement-speed penalty while carried, and cannot be opened without stopping — its own equip slot, gated by the same `bIsBusy` mechanism already used for reload/amputation (can't move or fight while its panel is open, released when closed).
- **Compartment eligibility rule**: item size tier — **Small / Medium / Large**, a new field on `UZSItemConfig` — decides which compartments accept an item (Pockets: Small only; Backpack: Small+Medium; Duffle: everything). Chosen over a category-based whitelist (dev had no preference between the two) and over a full spatial/Tetris grid (see rejection below).
- **Weapons are excluded from all three compartments entirely** — they use dedicated **weapon-mount slots** instead: 2 long-gun slots + 1 sidearm holster, using the same `FGuid`-reference equip-slot pattern already established for Back/Hip/SecondaryHand. This is what actually satisfies "a rifle can't go in a pocket," structurally, with zero new grid engineering. **Resolved 2026-07-30**: mount slots are the real weapon-carry capacity, not cosmetic — a weapon must occupy a mount slot to be carried at all (max 2 long-guns + 1 sidearm on your person). `HotbarSlots` becomes a quick-select pointer into a mounted weapon (or other item) for the active loadout, not an independent capacity check for weapons. This also happens to resolve a wishlist item already sitting unscheduled in `B0_Stabilization.md`'s decisions log — carried weapons currently just `Destroy()` on unequip rather than staying visibly holstered; mount slots are that holster.
- **Full Tetris-style spatial-grid inventory was explicitly considered and rejected.** Estimated cost: 6–9 sessions beyond B1-T5's existing 4–5 session estimate (new per-item footprint/rotation data, placement/collision validation, a drag-rotate grid widget, nested-bag sub-grids, 2-client replication of grid position) — comparable in size to all of B0-T2, the item-instance refactor. Judged not worth the ongoing player friction for this game's co-op-under-time-pressure design center (noise-as-threat, permadeath) versus the far cheaper weapon-mount-slot alternative, which solves the actual stated problem without the recurring "repack the grid" cost that even genre-defining games (Tarkov, DayZ) are frequently criticized for by more casual players.

## Open gap: item icon authoring has no scheduled task yet

Checked `Docs/Beta/B1_UI_UX.md`, `Docs/Beta/B2_ArtPipeline.md`, and `Docs/Beta/T_ContinuousTracks.md` — none schedule real per-item icon art as its own task. The only related decision is **OQ-B1-01** ("functional-grey now, restyle after B2"), which covers general UI chrome/style, not item-icon content specifically. **Recommendation**: build B1-T5 against placeholder/generic category icons (or plain text) first, so the inventory-grid mechanism itself isn't blocked waiting on art; track real per-item icon authoring as its own explicit task — either a B2 sub-task or an ongoing `T_ContinuousTracks.md` content-authoring item that ramps up once the grid exists and keeps pace as new items are added. This needs an actual decision when B1 is scheduled to start; not resolved here.

## Still open / not yet designed

- Stats and Skills tab layouts — deferred until B6 exists.
- Whether weapon jam state (or other combat feedback) gets any visual HUD cue at all, or stays purely audio/animation-driven per the milsim direction.
- Exact size-tier cutoffs (what counts as Small vs. Medium vs. Large) — not yet assigned to any real item.
- Death/respawn screen, sleep prompt, and container-loot screen were not mocked up this session.
