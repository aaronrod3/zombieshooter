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
- [ ] [dev-editor] **Next: `ZS_BS_UnequippedCrouchWalk`** — same rename/retarget pass as the standing blend space.
- [ ] [dev-editor] Build the equipped-state look via **Layered Blend Per Bone** (split at upper spine) compositing the rifle/pistol idle-aim pose over the two locomotion blend spaces — not a separate blend space per weapon. This is the actual Stage A AnimGraph work once the crouch blend space above is retargeted.
- [ ] [low priority] Assign a skeleton to the handful of wanted clips under `LyraAnims/Heroes/Mannequin/Animations/Actions/` (currently `Skeleton: None`, checked `MM_Pistol_Reload`/`MM_Rifle_Melee`) — only `Rifle_Melee` (Stage B melee swing) and the `HitReact` set (P3) are actually on our list; the rest of that folder is Lyra's out-of-scope action library.
- [ ] Pistol animations/montages blocked on Infima releasing pistol content — rifle-only for now, not a blocker for Stage A (the shared locomotion + layered-pose architecture works per-weapon regardless).
- [x] **Asset-commit policy for this and future Fab/marketplace imports, confirmed by the dev — 2026-07-19:** large downloadable game-asset content (the Lyra/ShooterGame animation import, ~318MB, and similar future packs) does **not** need to be committed if it's too large for the repo's $0 LFS budget — it's re-downloadable from its source (Fab) on a fresh machine, same treatment as `Content/InfimaGames/`. Applies to the ~1GB of other untracked content folders (VFX, SFX, footsteps, impacts, maps, hero characters) too, at the dev's discretion per-pack.
- [ ] [dev-editor] **2-client PIE verification on the slimmed build** — the surviving-actions successor to Phase 3's M7 checklist: fire/reload/aim/sprint/crouch from both clients, ammo convergence, `bIsBusy`/aim-block clearing on reload, late-join correctness. (The original M7 checklist in `CoreLoopPlan.md` includes FP/perspective items that no longer exist — skip those.)
- [x] [claude] C++ prep for Stage A: `UZSAnimInstanceBase` now pulls `GroundSpeed`/`Direction`/`bIsFalling` every frame (GameDevPlan §5.1) — **needs a Live Coding compile (Ctrl+Alt+F11) next time the editor's open**, the CLI build couldn't run while Live Coding held the lock. — *2026-07-19*
- [ ] [both] **Stage A base locomotion build** (GameDevPlan §5.1): `ABP_ZS_ThirdPerson`'s Idle/Move (+ crouch branch) driven by `ZS_BS_Unarmed_Idle_Walk_Run`/`ZS_BS_UnequippedCrouchWalk`, with the rifle/pistol pose composited on top via Layered Blend Per Bone — architecture confirmed, see above. Fire/Reload montages already sourced from Infima (rifle); re-place `AN_ZS_UnlockActions`/`ANS_ZS_BlockADS` on them.
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
- [ ] Cursor-projected aim (screen ray → ground plane → character faces aim point) — replaces the interim `bOrientRotationToMovement` facing
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

- **2026-07-20:** locomotion architecture confirmed against actual asset inventory — 2 shared blend spaces + Layered Blend Per Bone for equipped poses (not per-weapon blend spaces), no AO layer (cursor-facing), no jump (future mounting system), montages confirmed as the right tool for reload/fire/interact/melee. `ZS_BS_Unarmed_Idle_Walk_Run` renamed/retargeted. Asset needs list updated with the confirmed pack roster (LowPolyWeapons, Mega_Survival_Tools, Infima rifle-only pending pistol release).
- **2026-07-19 (session 10):** editor cleanup confirmed (minus `IA_SwitchGrip` leftover) · Lyra-style animation library imported · found + diagnosed the missing-skeleton blocker (ShooterGame migration gap) via live MCP asset inspection · `UZSAnimInstanceBase` locomotion-state prep (`GroundSpeed`/`Direction`/`bIsFalling`) written, pending Live Coding compile · GameDevPlan §5.1 rewritten against verified asset state
- **2026-07-18 (session 9):** LNK2019 fix · P0 C++ de-scope (−1,896 lines, compiled clean) · GameDevPlan §5.1 standard-animation contract · this tracker created · CLAUDE.md/SessionHandoff updated for the pivot
- **2026-07-18 (session 9, planning):** GameDevPlan v0.2 revised against dev markup (DevMarkupNotes.md archived) · Decisions 4–6 + doc-name resolved · v1 skill list finalized (§3.1) · investigation-clue placement model resolved
- **2026-07-18:** GameDevPlan v0.1 + PZ reference archived · session-8 replication work committed/pushed
- **Earlier (Phases 0–3):** see `CoreLoopPlan.md` (historical plan of record) + `SessionHandoff.md`
