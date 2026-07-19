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
- [ ] [dev-editor] **Content cleanup pass** (checklist below — do this before anything else in the next editor session)
- [ ] [dev-editor] **2-client PIE verification on the slimmed build** — the surviving-actions successor to Phase 3's M7 checklist: fire/reload/aim/sprint/crouch from both clients, ammo convergence, `bIsBusy`/aim-block clearing on reload, late-join correctness. (The original M7 checklist in `CoreLoopPlan.md` includes FP/perspective items that no longer exist — skip those.)
- [ ] [both] **Stage A base locomotion build** (GameDevPlan §5.1): repoint body mesh to UE5 Manny, fresh `ABP_ZS_ThirdPerson` on the UE5 skeleton (parent: `UZSAnimInstanceBase`), Idle/Move state machine + crouch layer + aim upper-body blend, GASP strafe set. Then retarget-or-replace Fire/Reload montages (Lyra or Infima retarget) and re-place the two timing notifies.
- [ ] [claude] Update `TuningReference.md` — remove dead tunables (spring configs, FP/GunCamera/Bodycam FOVs, recoil ranges); add none until new systems land.
- [ ] [claude] `CLAUDE.md` + `SessionHandoff.md` P0 updates — *in progress this session*

### Content cleanup checklist (next editor session, ~30 min)

- [ ] Delete `ABP_ZS_FirstPerson` (its C++ parent class no longer exists — it will fail to load/compile; expected)
- [ ] Delete `IA_Inspect`, `IA_MagCheck`, `IA_SwitchGrip` assets + their mappings in `IMC_ZS_Default`
- [ ] Re-save `BP_ZS_PlayerCharacter` — drops stale `FirstPersonMesh`/`FirstPersonCamera` component data; verify `CharacterMesh0` mesh + `AnimClass=ABP_ZS_ThirdPerson` intact and `StartingWeaponConfig` still set
- [ ] Re-save `DA_ZS_WeaponConfig_AssaultRifle` — drops ~70 removed fields; verify the kept set is still populated (TP_Mesh, TP_Fire, TP_Reload, TP_IdleLoop/IdlePose/AimPose, ammo block, fire modes, receiver/magazine meshes + sockets)
- [ ] Investigate untracked `Content/__ExternalActors__/ThirdPerson/Lvl_ThirdPerson/B/JX/` — commit if legitimate level edit, delete if stray
- [ ] Download free content for Stage A: **Game Animation Sample** (Fab), optionally **Lyra** (for the rifle anim set)

---

## Next — P1: camera & control prototype (GameDevPlan §4 P1)

- [ ] TopDown perspective entry in `ApplyCameraPerspective` (pitch ~55°, zoom, step rotation) + OverShoulder aim variant
- [ ] Cursor-projected aim (screen ray → ground plane → character faces aim point) — replaces the interim `bOrientRotationToMovement` facing
- [ ] `UZSInteractableComponent` + world interaction prompt v1
- [ ] Gamepad input validation alongside mouse/keyboard (bindings exist; tune + test)
- [ ] Graybox test map
- [ ] **Go/no-go gate on Decision 1 (camera) — settle before any art purchase**

---

## Later — pointers, not duplicates

- P2–P10 backlogs live in [`GameDevPlan.md`](GameDevPlan.md) §4 — don't mirror them here until they enter Next.
- Open questions per phase: GameDevPlan §7. **Still blocking for P8:** launch meta-event count (deep handful vs. broad roster).
- Unconfirmed recommendations: Decisions 1–3 (camera, same-repo, art source) — camera is effectively being confirmed by P1's gate; art purchase timing is a P7-adjacent call (watch for Synty Humble bundles meanwhile).
- Hostile human roamers: first post-v1 addition (Decision 5) — design questions parked in GameDevPlan §7 P4.

---

## Done log (newest first)

- **2026-07-18 (session 9):** LNK2019 fix · P0 C++ de-scope (−1,896 lines, compiled clean) · GameDevPlan §5.1 standard-animation contract · this tracker created · CLAUDE.md/SessionHandoff updated for the pivot
- **2026-07-18 (session 9, planning):** GameDevPlan v0.2 revised against dev markup (DevMarkupNotes.md archived) · Decisions 4–6 + doc-name resolved · v1 skill list finalized (§3.1) · investigation-clue placement model resolved
- **2026-07-18:** GameDevPlan v0.1 + PZ reference archived · session-8 replication work committed/pushed
- **Earlier (Phases 0–3):** see `CoreLoopPlan.md` (historical plan of record) + `SessionHandoff.md`
