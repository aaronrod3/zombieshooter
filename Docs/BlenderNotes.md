# Blender Notes

> **Living document — created 2026-08-12.** Update sections in place each session; this is not a log, don't append dated entries to the bottom the way `SessionHandoff.md` avoids doing too. If something here turns out wrong, correct it in place and note the correction inline, don't leave the stale version alongside a new one.
>
> Broader item-authoring convention (archetypes, required fields, the low-poly triangle/material budget, and a category-level tracker of what's sourced vs. what needs modeling) lives in `Docs/Planning/ItemSetupStandard_2026-08-06.md` (§4b, §9) — that's maintained from the coding session against the C++ source directly. This file is narrower: the tactical, easy-to-forget specifics a Blender session needs on hand before touching anything.

## 1. Purpose

This file is the memory bridge for Blender sessions on this project — the Blender MCP connector (via Claude Desktop) doesn't carry context between sessions the way a Claude Code session does, so read this file first, every session, before modeling anything.

## 2. Naming convention

Verified directly against what's actually sitting in `Content/` (not assumed) — no static mesh anywhere in the project uses a `ZS`-prefixed filename (`Content/**/SM_ZS_*` returns nothing). `CLAUDE.md`'s `ZS` prefix rule is scoped to **classes, Blueprints, Data Assets, and Widget Blueprints** (`AZSPlayerCharacter`, `BP_ZS_*`, `DA_ZS_*`, `WBP_ZS_*`) — raw art (meshes/textures/materials) does not take it.

- **Static mesh**: `SM_<Name>`, or `SM_<Abbrev>_<Part>` for a multi-part item. Example already live and correctly wired: the assault rifle's actual in-use parts are `SM_AR_Rifle` (body), `SM_AR_Trigger`, `SM_AR_Magazine` — these match `DA_ZS_WeaponConfig_AssaultRifle`'s `BaseWeaponMesh`/`TriggerMesh`/`MagazineMesh` fields one-for-one. Use this clean form for anything newly modeled.
- **Wart worth knowing about, not replicating**: raw, not-yet-renamed source-pack imports keep their *original pack filenames* until someone renames them — e.g. hundreds of `SM_AssaultRifle1_01_3`-style names sit alongside the real `SM_AR_*` ones in `Content/ZS/Items/Weapons/Meshes/AssaultRifle/`, and the medical pack has `SM_Bandage_01` / `SM_bandage_02` (inconsistent capitalization — not a convention, just an existing typo) / `SM_Bandage_03` / `SM_med_box_01`. Don't take these as the pattern to match.
- **Texture**: `T_<Name>_BaseColor` / `_Normal` / `_OcclusionRoughnessMetallic` — confirmed from the `Content/FirstAidCabinet/Textures/` pack's own convention, already precedent in the project. For a hand-authored low-poly item, per `ItemSetupStandard_2026-08-06.md` §4b, only `BaseColor` is actually required — skip `_Normal`/`_ORM` unless a specific item needs the extra read (unlikely at this poly count and camera distance).
- **Material**: `M_<Name>` (master), `MI_<Name>` (instance).
- **Data asset** (not authored in Blender, but this is what a finished mesh eventually feeds into on the Unreal side): `DA_ZS_ItemConfig_<Name>` / `DA_ZS_WeaponConfig_<Name>`.

## 3. Export settings

**Not yet empirically confirmed — do not treat the numbers below as verified.** This note was written from a session with no Blender and no live-Unreal-editor access, so the one thing the dev explicitly asked to be checked by hand hasn't been checked by hand yet. Treat this whole section as a to-do with a starting guess attached, not a resolved answer.

**What actually needs checking**: Unreal is Z-up, centimeters. Blender's default scene is also Z-up, meters — so the axis mapping is less likely to be the problem than the *unit-scale conversion* Blender's FBX exporter applies. That's the one number that silently produces a correctly-shaped-but-wrong-sized import if it's off.

**Likely-correct starting point** (standard Blender FBX exporter defaults, unverified on this project specifically):
- Forward: `-Z Forward`, Up: `Y Up` (Blender-exporter-space axis knobs — this is the combination that typically lines up with UE's expectation after the exporter's internal conversion).
- Scale: `1.00`, Apply Scale: `FBX All`.
- Apply all object transforms in Blender before export (`Ctrl+A` → All Transforms) rather than relying on the exporter to bake them — an un-applied rotation/scale is the most common cause of a rotated or wrong-sized import.

**The actual empirical test to run, first time Blender + Unreal are both in reach in the same session:**
1. In Blender, with the scene unit set to Meters, model a plain cube exactly 1×1×1m (= 100×100×100cm).
2. Export FBX with the candidate settings above.
3. Import into this project, open the resulting Static Mesh in the Static Mesh Editor, and read its bounding box.
4. It should read **100×100×100** (cm). If it reads 1×1×1, or 10000×10000×10000, or anything non-cubic, the Scale/Apply-Scale setting is wrong — adjust and re-test before trusting any asset made with these settings.
5. Write the confirmed, working settings back into this section, replacing the "unverified" framing above.

(`Docs/Planning/ItemSetupStandard_2026-08-06.md` §4b previously noted the Forward/Up pair above as a working default without flagging it as unverified — treat *this* file as the canonical, empirically-checked source once step 5 above actually happens, and correct that doc's note to match or point here.)

## 4. Collision

Which exported meshes are "attachment-type" (rigidly attached to the character or a weapon, needs `NoCollision`):
- **`WornMesh`** — the on-character visual for the 5 physical **Gear** slots only: Helmet, Vest, Belt, Backpack, Duffle. (The 6 **Clothing** slots — Head/Eyes/Mask/Shirt/Pants/Shoes — don't get a mesh at all, planned as a texture swap instead; see §5/§6 below. Don't model a `WornMesh` for those.)
- Weapon attachment meshes — Muzzle, Handguard, Grip, Optic — and a weapon's own `BaseWeaponMesh`/`TriggerMesh`.

**Verified nuance, worth knowing before doing this by hand**: for anything going through the two established systems above, collision is already force-disabled automatically, in code, at runtime — confirmed by reading `ZSPlayerCharacter.cpp` directly: every `WornMeshComponents` entry gets `SetCollisionEnabled(ECollisionEnabled::NoCollision)` unconditionally at creation, regardless of what the imported mesh itself carries. `AZSWeapon::AssignNewStaticMesh` does the same for weapon attachments. **So in the normal case, no manual per-asset Unreal-side toggle is actually required** — just flag in a handoff which exported mesh is intended as a `WornMesh`/attachment so nobody has to rediscover this, rather than assuming it needs a manual fix.

A manual `NoCollision` toggle (Static Mesh Editor, or on the Blueprint/component doing the attaching) is only needed if a mesh is attached through some *other*, not-yet-existing pathway that doesn't go through `RefreshWornMeshes`/`AssignNewStaticMesh` — flag explicitly if that situation ever comes up.

`WorldMesh` (the dropped/ground-pickup visual) doesn't need this at all — interaction is handled by a separate sphere-overlap component (`UZSInteractableComponent`), not the mesh's own collision. Normal auto-generated collision (or none) is fine.

## 5. Folder destinations

**Verified current structure** (checked directly, not from an old plan doc — see the note below on why that matters):

```
Content/ZS/Items/
  Weapons/
    Ammo/Blueprints/, Ammo/DataAssets/
    Melee/    <- DA_ZS_WeaponConfig_Crowbar.uasset (mesh sourcing not confirmed this pass)
    Pistol/   <- DA_ZS_WeaponConfig_Pistol.uasset (mesh sourcing not confirmed this pass)
    Rifle/    <- DA_ZS_WeaponConfig_AssaultRifle.uasset, T_AssaultRifle.uasset, BP_ZS_WorldItem_AR_Rifle.uasset
    Meshes/AssaultRifle/  <- 900+ mesh parts (mix of the real SM_AR_* ones in use, and unused raw pack imports)
  Medical/Meshes/   <- SM_Bandage_01/02/03, SM_med_box_01, etc. (FirstAidCabinet-pack names, not yet renamed)
  Textures/         <- T_CanofBeans.uasset (flat, not yet split into a per-category subfolder)
  ItemDataAssets/   <- DA_ZS_ItemConfig_PistolPickup, DA_ZS_ItemConfig_Flashlight
```

**Note the inconsistency, don't copy it**: the DA-holding folder is named `Rifle/` but the mesh-parts folder is `Meshes/AssaultRifle/` — different name, different nesting depth. This isn't a deliberate pattern, just where things landed (the mesh-parts pack import was large enough to warrant its own subfolder; Pistol/Melee weren't). `Content/ZS/Weapons/` (the older top-level path this doc's original 2026-08-06 plan and an earlier session-handoff note both described) **no longer exists** — confirmed empty; everything now lives under `Content/ZS/Items/Weapons/` as shown above. If you see either path referenced elsewhere in the docs, trust this file's directly-verified version over an older note.

**Two untracked asset packs still sitting at the repo root, not yet organized in:**
- `Content/FirstAidCabinet/` — 128 assets, medical (bandages, alcohol, spray, medicine box, scissors, syringe, iodine, painkiller/vitamin textures).
- `Content/Poly-MegaSurvivalFood/` — 242 assets, food (raw/cooked/rotten produce, canned goods, drinks, burger/pizza/sandwich modular kits).

Per the coding session's plan (`ItemSetupStandard_2026-08-06.md` §4a), these are meant to eventually feed `Content/ZS/Items/Food/` and `Content/ZS/Items/Medical/`, but that move hasn't happened yet. Don't treat their current root-level location as a real destination, and don't assume an asset existing in one of these packs means the item is "done" — most of it has no `DA_ZS_ItemConfig` wired up yet either (that's a data-authoring task, separate from anything Blender needs to do).

**For genuinely new Blender output**: put mesh + texture + material instance together in `Content/ZS/Items/<Category>/` (non-weapon) or `Content/ZS/Items/Weapons/<Category>/` (weapon), matching the `Rifle/` folder's flat pattern — not the separate `Meshes/` subfolder, which looks like a one-off for an unusually large raw part count rather than a rule to repeat.

## 6. Open questions

Surface these to the coding session / dev rather than guessing past them:

- **FBX export scale/axis — unverified.** See §3. Needs the empirical cube test run once, by hand, before anything exported so far can be fully trusted dimensionally.
- **Icon pipeline status unclear.** `Content/ZS/Items/IconCreator/BP_IconGenerator.uasset` and `M_Icon.uasset` exist and had in-progress edits as of the coding session's last check — this looks like an icon-auto-generation-from-mesh system (render-capture a `WorldMesh` into an icon texture) rather than something requiring hand-drawn 2D icon art. **Unconfirmed.** If true, Blender sessions never need to produce separate icon textures — just the 3D mesh. Big enough workflow implication that it's worth the dev confirming either way before assuming either.
- **`DA_Bag`'s mesh sourcing and its `EquipSlot` value are both flagged unconfirmed** (coding-session note, since the `Back`→`Backpack` enum rename) — relevant since Backpack/Duffle gear is the current best first-modeling-target candidate.
- **No canonical real-world size reference exists for items in any doc.** Recommendation until told otherwise: model at real-world scale using Blender's normal meter-based units (a bandage roughly hand-sized, a backpack roughly torso-sized) rather than inventing a size chart — simpler, and avoids confusing this with `UZSItemConfig::ItemSize` (`Small`/`Medium`/`Large`), which is a **gameplay compartment-fit classification, not a physical dimension** (`Source/ZombieShooter/Survival/ZSItemConfig.h`).
- **`ProjectileMesh` is a confirmed small content gap** (`Docs/TuningReference.md`: currently an engine placeholder Sphere, "needs a real bullet mesh per weapon before this is presentable") — trivial geometry, could double as a low-stakes first test of the export pipeline (§3) before committing to a bigger gear piece.
- Melee (Crowbar) and Pistol mesh sourcing not confirmed this pass — worth a quick in-editor check of whether they already have a real mesh assigned or are still on a pack placeholder.

## 7. Status

**Last done**: file created 2026-08-12 — naming/folder/collision sections grounded directly against current repo state. Dev picked **Backpack** as the first modeling target the same session.

**Next / active target — Backpack:**
- Socket: `EZSEquipSlot::Backpack` → `SocketBack` (confirmed in `AZSPlayerCharacter::GetSocketForEquipSlot`, `ZSPlayerCharacter.cpp`) — `Duffle` maps to the same socket, so the two are likely meant to be mutually exclusive in practice, not layered together.
- **Unconfirmed since 2026-08-06**: `CLAUDE.md` flagged `SocketBack` as not yet existing on `SKEL_TFA_Mannequin` as of that date — worth a quick check in the Skeleton Tree before assuming it's there. Without it, the mesh still attaches (no crash) but at a default transform, not the right spot on the back.
- Budget: 500–1200 tris ("Worn gear, large," §4b above).
- Texture: 256×256, BaseColor only, one `MI_Backpack` instance. Note: the shared master material this depends on (`M_ZS_Item`, per §4b) doesn't exist yet either — needs creating once, then every future item instances off it.
- Scale reference (generic real-world reference, not a game-specific spec — none exists in the project yet): roughly 45–55cm tall × 30–35cm wide × 15–20cm deep, day-pack proportions.
- Naming/destination: `SM_Backpack` / `T_Backpack_BaseColor` / `MI_Backpack`, into `Content/ZS/Items/Containers/` (per `ItemSetupStandard.md` §4a's proposed layout — doesn't exist yet, this would be the first thing to land there).
- **Out of scope for the Blender session itself**: wiring the finished mesh into a data asset. `DA_Bag` already exists but its mesh sourcing and `EquipSlot` value are both flagged unconfirmed (§6) — sorting that out is coding-session/in-editor follow-up once the mesh is ready, not a Blender task.
