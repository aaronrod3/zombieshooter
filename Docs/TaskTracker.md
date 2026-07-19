# Task Tracker

> The live work queue — current and future tasks, outside the plan document. Complements, never replaces:
> - **[`GameDevPlan.md`](GameDevPlan.md)** — the plan of record: phases, scope contract, decisions, open questions. *Why and what.*
> - **[`SessionHandoff.md`](SessionHandoff.md)** — the session-by-session technical log. *What actually happened and what a fresh session must know.*
>
> This file is the *what's next*, at task granularity. **Update it every session**: check items off with a date, add newly-discovered work, promote items from Next → Now as phases advance. Owner tags: **[dev-editor]** = needs the human at the Unreal Editor (PIE tests, asset work MCP can't do); **[claude]** = headless-capable (C++, docs, git); **[both]** = MCP-driven editor work with dev checkpoints.

---

## Now — P0: close out & re-aim (GameDevPlan §4 P0)

- [x] Fix `SetHiddenFromOwner` LNK2019 linker error — *2026-07-18*
- [x] [claude] P0 C++ de-scope pass — FP rig removed outright (dev directive), GunCamera/Bodycam gone, springs/recoil cut, Inspect/MagCheck/grip cut, laser + physics cosmetics + weapon-owned notifies deleted, `UZSWeaponConfig` slimmed ~90 → ~22 fields. Compiled clean. — *2026-07-18*
- [x] [dev-editor] **Content cleanup pass** — *2026-07-19.* `ABP_ZS_FirstPerson`/`IA_Inspect`/`IA_MagCheck` confirmed gone. **Leftover: `IA_SwitchGrip` is still present** in `/Game/ZS/Input/` — low priority, delete whenever convenient (and its `IMC_ZS_Default` mapping if still bound).
- [x] [dev-editor] Import animation source content — *2026-07-19.* Large Lyra-style library imported into `/Game/Animation/` (see the new blocker below — not yet usable as-is).
- [x] [dev-editor] Reorganize imported content — *2026-07-19.* `Content/Characters/Mannequins/` + `Content/Characters/Heroes/` renamed/consolidated into `Content/LyraAnims/` (`Mannequins/` + `Heroes/`). Confirmed via MCP: `SK_Mannequin`/`SKM_Manny_Simple`/`SKM_Quinn_Simple` under `LyraAnims/Mannequins/Meshes/` are now real and self-consistent (the old dangling-reference problem is fixed for this folder). ~397MB, not committed (large-content policy above).
- [x] [dev-editor] **Locomotion architecture settled — 2026-07-20.** `ZS_BS_Unarmed_Idle_Walk_Run` renamed + retargeted onto `SKEL_TFA_Mannequin`, done. Animations organized into curated `Content/ZSAnims/`. **Confirmed: only 2 base locomotion blend spaces needed total (standing, crouched)** — no separate per-weapon blend spaces (the raw walk-cycle assets for those don't exist, only poses + montages), no AO layer (cursor-facing removes the need), no jump (future mounting system instead). Full architecture: GameDevPlan §5.1.
- [x] [dev-editor] `ZS_BS_UnequippedCrouchWalk` renamed + retargeted onto `SKEL_TFA_Mannequin` (1D — `GroundSpeed` only, no strafe axis, confirmed via inspection) — *2026-07-20*
- [x] [claude] **`ABP_ZS_ThirdPerson` AnimGraph rebuilt via MCP — 2026-07-20.** Cleared the stale pre-pivot graph (stance blend, `SM_AimingTransitions` state machine, breathing additive, hand-IK FABRIK — all dependent on removed C++), kept the 2 Slot nodes. Built fresh: both locomotion blend spaces → stance `BlendListByBool` → `Layered Blend Per Bone` (split `spine_02`) compositing a `bIsAiming`-selected Infima rifle idle/aim pose → existing Slot chain → Output. Compiled clean (zero warnings), saved. Reused the session-6-documented `BlendListByBool` reversed-mapping gotcha (`True`→`BlendPose_0`) rather than assume textbook ordering.
- [ ] [dev-editor] **One pin left: compile `bIsCrouched`** (Ctrl+Alt+F11 — added to `UZSAnimInstanceBase` this session, not yet live) then wire it into the stance selector's `bActiveValue`. Graph defaults safely to standing locomotion until then, not broken.
- [ ] [low priority] Assign a skeleton to the handful of wanted clips under `LyraAnims/Heroes/Mannequin/Animations/Actions/` (currently `Skeleton: None`, checked `MM_Pistol_Reload`/`MM_Rifle_Melee`) — only `Rifle_Melee` (Stage B melee swing) and the `HitReact` set (P3) are actually on our list; the rest of that folder is Lyra's out-of-scope action library.
- [ ] Pistol animations/montages blocked on Infima releasing pistol content — rifle-only for now, not a blocker for Stage A (the shared locomotion + layered-pose architecture works per-weapon regardless).
- [x] **Asset-commit policy for this and future Fab/marketplace imports, confirmed by the dev — 2026-07-19:** large downloadable game-asset content (the Lyra/ShooterGame animation import, ~318MB, and similar future packs) does **not** need to be committed if it's too large for the repo's $0 LFS budget — it's re-downloadable from its source (Fab) on a fresh machine, same treatment as `Content/InfimaGames/`. Applies to the ~1GB of other untracked content folders (VFX, SFX, footsteps, impacts, maps, hero characters) too, at the dev's discretion per-pack.
- [ ] [dev-editor] **2-client PIE verification on the slimmed build** — the surviving-actions successor to Phase 3's M7 checklist: fire/reload/aim/sprint/crouch from both clients, ammo convergence, `bIsBusy`/aim-block clearing on reload, late-join correctness. (The original M7 checklist in `CoreLoopPlan.md` includes FP/perspective items that no longer exist — skip those.)
- [x] [claude] C++ prep for Stage A: `UZSAnimInstanceBase` now pulls `GroundSpeed`/`Direction`/`bIsFalling`/`bIsCrouched` every frame (GameDevPlan §5.1) — `bIsCrouched` (added 2026-07-20) is the one **not yet compiled**, needs Ctrl+Alt+F11.
- [ ] [dev-editor] Re-place `AN_ZS_UnlockActions`/`ANS_ZS_BlockADS` on Infima's TP fire/reload montages (Stage B) — the timing system carries over untouched.
- [ ] [claude] Update `TuningReference.md` — remove dead tunables (spring configs, FP/GunCamera/Bodycam FOVs, recoil ranges); add none until new systems land.
- [x] [claude] `CLAUDE.md` + `SessionHandoff.md` P0 updates — *2026-07-18*

### Remaining cleanup checklist

- [ ] Re-save `BP_ZS_PlayerCharacter` — drops stale `FirstPersonMesh`/`FirstPersonCamera` component data; verify `CharacterMesh0` mesh + `AnimClass=ABP_ZS_ThirdPerson` intact and `StartingWeaponConfig` still set
- [ ] Re-save `DA_ZS_WeaponConfig_AssaultRifle` — drops ~70 removed fields; verify the kept set is still populated (TP_Mesh, TP_Fire, TP_Reload, TP_IdleLoop/IdlePose/AimPose, ammo block, fire modes, receiver/magazine meshes + sockets)
- [ ] Investigate untracked `Content/__ExternalActors__/ThirdPerson/Lvl_ThirdPerson/B/JX/` — commit if legitimate level edit, delete if stray
- [ ] Delete `IA_SwitchGrip` (see above)

---

## Next — P1: camera & control prototype (GameDevPlan §4 P1)

- [ ] TopDown perspective entry in `ApplyCameraPerspective` — *Door Kickers 2*-framed per the 2026-07-19 dev reference: steeper pitch (~65–75°, not classic 45° isometric), zoom tight enough to read character/weapon detail — + OverShoulder aim variant
- [ ] **Hybrid facing (confirmed 2026-07-20):** `bOrientRotationToMovement = true` (P0's existing default) stays permanent for plain WASD movement — no change needed there. Add cursor-projected aim (screen ray → ground plane → character faces aim point) as a **conditional override**, active only while aiming/attacking/interacting with the cursor. Full detail: GameDevPlan §4 P1.
- [ ] `UZSInteractableComponent` + world interaction prompt v1
- [ ] Gamepad input validation alongside mouse/keyboard (bindings exist; tune + test)
- [ ] Graybox test map
- [ ] **Go/no-go gate on Decision 1 (camera) — settle before any art purchase**

---

## Later — pointers, not duplicates

- P2–P10 backlogs live in [`GameDevPlan.md`](GameDevPlan.md) §4 — don't mirror them here until they enter Next.
- Open questions per phase: GameDevPlan §7. **Still blocking for P8:** launch meta-event count (deep handful vs. broad roster).
- Decisions 1–3 status: camera (DK2-framed top-down) and art source (dark/earthy/low-poly, dev-sourced via ArtStation, no Synty) both resolved 2026-07-19 — camera still gets its final go/no-go from P1's gate. Same-repo (Decision 2) unconfirmed but unchallenged.
- Asset sourcing: dev's own from here — GameDevPlan §5's asset needs list tracks what's required by phase; not a Claude research task anymore.
- Hostile human roamers: first post-v1 addition (Decision 5) — design questions parked in GameDevPlan §7 P4.

---

## Done log (newest first)

- **2026-07-20 (AnimGraph build):** cleared `ABP_ZS_ThirdPerson`'s stale pre-pivot AnimGraph and rebuilt it via MCP — 2 locomotion blend spaces, stance selector, rifle idle/aim pose selector, Layered Blend Per Bone, wired into the existing montage Slot chain. Compiled clean, saved. One pin (`bIsCrouched`) pending a Live Coding compile.
- **2026-07-20 (later):** hybrid facing model confirmed for P1 — movement-direction facing by default (already correct, no new work), cursor-facing only while actively aiming/attacking/interacting. Validated the animation-side `Direction`/`GroundSpeed` approach needs zero changes to support it.
- **2026-07-20:** locomotion architecture confirmed against actual asset inventory — 2 shared blend spaces + Layered Blend Per Bone for equipped poses (not per-weapon blend spaces), no AO layer (cursor-facing), no jump (future mounting system), montages confirmed as the right tool for reload/fire/interact/melee. `ZS_BS_Unarmed_Idle_Walk_Run` renamed/retargeted. Asset needs list updated with the confirmed pack roster (LowPolyWeapons, Mega_Survival_Tools, Infima rifle-only pending pistol release).
- **2026-07-19 (session 10):** editor cleanup confirmed (minus `IA_SwitchGrip` leftover) · Lyra-style animation library imported · found + diagnosed the missing-skeleton blocker (ShooterGame migration gap) via live MCP asset inspection · `UZSAnimInstanceBase` locomotion-state prep (`GroundSpeed`/`Direction`/`bIsFalling`) written, pending Live Coding compile · GameDevPlan §5.1 rewritten against verified asset state
- **2026-07-18 (session 9):** LNK2019 fix · P0 C++ de-scope (−1,896 lines, compiled clean) · GameDevPlan §5.1 standard-animation contract · this tracker created · CLAUDE.md/SessionHandoff updated for the pivot
- **2026-07-18 (session 9, planning):** GameDevPlan v0.2 revised against dev markup (DevMarkupNotes.md archived) · Decisions 4–6 + doc-name resolved · v1 skill list finalized (§3.1) · investigation-clue placement model resolved
- **2026-07-18:** GameDevPlan v0.1 + PZ reference archived · session-8 replication work committed/pushed
- **Earlier (Phases 0–3):** see `CoreLoopPlan.md` (historical plan of record) + `SessionHandoff.md`
