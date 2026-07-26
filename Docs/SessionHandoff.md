# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves now.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **B0-T0 complete. B0-T8 (zombie AI) complete and PIE-verified** — the navmesh blocker is resolved. **B0-T1 (verification sweep) is in progress**: the first PIE pass found 4 real bugs, all fixed tonight but **not yet re-verified** — needs a second PIE pass.

## Last completed (2026-07-26) — navmesh fixed by the dev; first B0-T1 PIE pass found and fixed 4 bugs

### Navmesh — resolved, zombie AI now verified working
The dev fixed `Lvl_ThirdPerson`'s stuck navigation build manually, following Epic's official docs (dev.epicgames.com "Basic Navigation in Unreal Engine," §2). **Zombie AI now moves around and behaves correctly in PIE** — wander, investigate, and chase all confirmed. `memory/project_navmesh_dynamic_workaround.md` and `Docs/Beta/B0_Stabilization.md`'s T8.5 are updated to reflect this. Lesson recorded for future sessions: check the official UE 5.8 docs site first on engine-level setup problems, not just engine-source spelunking — see `memory/feedback_consult_official_ue_docs.md`.

### B0-T1 runbook, pass 1 — 4 real bugs found, all fixed (not yet re-tested)

1. **Weapon attachment socket was wrong.** `SocketGunAttachment` was set to `"ik_hand_gun"` — an IK control bone, not a weapon-carry bone, and not a real socket (attaching to a bare bone name falls back to that bone's raw, unoffset transform). This is why the rifle spawned mispositioned/misrotated on equip. Root-caused by inspecting the skeleton directly: `SKM_Manny_Simple` has a dedicated `weapon_r` bone with an existing pack-authored socket, `weapon_r_muzzle`, sitting at **zero local offset** on it — strong evidence `weapon_r` is the actual intended weapon-carry point. Tried creating a new socket there via `unreal-mcp`'s `add_socket` — **that tool has a bug/limitation**: it silently fails to parent to `weapon_r` (confirmed likely a virtual bone, since parenting to a real bone like `pelvis` worked fine in a side-by-side test) and defaults the new socket to `root` instead, with no error surfaced except an unrelated-looking `SetSocketParent... bone named 'None'` log line. Worked around by pointing `SocketGunAttachment` at the existing, already-correct `weapon_r_muzzle` socket instead (confirmed unused anywhere else in our C++). All 3 weapon configs (AssaultRifle, Pistol, Crowbar) updated. **The "movement becomes unpredictable while equipped" symptom is very likely a visual side-effect of this same bug** (a wrong-angled weapon rigidly attached to the body makes turning look erratic) — checked `UpdateMovementSpeed`/`bUseControllerRotationYaw`/aiming code directly, nothing in there reacts to `CurrentWeapon` being set, so there's no real movement-logic bug found. Re-check this specifically once the socket fix is verified in PIE — flag it again if it's still happening with the gun correctly placed.
2. **Fire trace/damage confirmed working** (`shot hit ... for 25.0 damage` in the log) — likely was firing from the wrong angle only because of bug 1 above, not a separate bug. Re-verify muzzle origin once the socket fix is confirmed.
3. **Hotbar keys 2–9 never worked — only key 1 did anything.** Root cause: `IMC_ZS_Default`'s per-digit-key `Scalar` modifiers were only ever authored for the "One" key mapping; "Two" through "Nine" had empty modifier arrays, so every digit key sent the Enhanced Input system's raw press value of `1.0` regardless of which key was pressed — `HandleHotbarSelect` always resolved to slot 1. This is why pressing 1/2/3 just toggled the rifle on/off (re-pressing the *same resolved slot* triggers the documented unequip-toggle behavior) instead of cycling. Fixed: added `InputModifierScalar` instances (X = 2..9) to each of the "Two"–"Nine" key mappings, mirroring "One"'s existing (X=1) modifier. Stage D (re-press-1-to-unequip) is confirmed still working — that logic was never the bug.
4. **`ZSContainer_TestLoot` (now `BP_ZS_Container_Test`) had no visible mesh.** The Blueprint child I built last session for T1.2 never got a `ContainerMesh` assignment — functional (loot table was wired) but invisible, so undiscoverable in the level. Fixed: assigned `SM_Small_Wood_Box_Closed` (`Content/Mega_Survival_Tools/`).
5. Crowbar/Stage F untested — blocked on bug 3 (hotbar cycling), should be reachable now.

**None of tonight's 4 fixes have been PIE-verified yet** — they're logically sound and compile/save clean, but need a fresh run to confirm.

## Runbook — B0-T1 Stages B–G, pass 2 (do this next)

Same loadout as before — **AssaultRifle (1) / Pistol (2) / Crowbar (3, melee)** — now with the socket, hotbar, and container fixes applied.

1. **Stage B** — press `1`. Confirm the rifle now sits correctly on the character (not off to the side/pointing up), and that equipping no longer makes movement/turning look erratic.
2. **Stage C** — fire. Confirm the shot originates from the now-correctly-placed muzzle.
3. **Stage D** — press `1` again to unequip (already confirmed working, quick re-check only).
4. **Stage E** — press `1`, then `2` (should now genuinely switch to the Pistol, not just toggle), then `3`. Confirm real cycling across all three slots, and that scroll-wheel cycling also works.
5. **Stage F** — with the Crowbar (`3`) equipped, melee a zombie. Confirm crowbar stats apply (22 dmg), land ~15 hits to break it, confirm it unequips and slot 3 stays empty on re-press.
6. **Stage G** — the container near spawn (~450,300) should now be visible (a wood box) and lootable. The world item pickup (~300,450) should still work as before. Loot, drop, re-pick-up, check encumbrance.

**File failures as discrete notes, don't fix inline** (T1.10). Report back pass/fail per stage and I'll fold results into `B0_Stabilization.md`.

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits. (Tonight's work was content-only again — no rebuild needed.)
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**
- **When stuck on an engine-level setup problem, check the official UE 5.8 docs site** (dev.epicgames.com) before extended trial-and-error or engine-source spelunking — see `memory/feedback_consult_official_ue_docs.md`.
- **After large multi-file sessions, regenerate IDE project files** (`Build.bat -projectfiles ...`) — not needed yet this stretch (no new C++ files/header changes since the last regen).

## Known tooling gotcha (worth remembering)

`unreal-mcp`'s `SkeletalMeshTools.add_socket` does not reliably honor its `bone_name` argument for at least one bone on this project's skeleton (`weapon_r`, likely a virtual bone) — it silently parents the new socket to `root` instead, with only an oblique log line (`SetSocketParent... bone named 'None'`) as a clue, no thrown error. Always verify with `get_socket_bone` right after `add_socket`, don't trust the call succeeding at face value. Reusing an existing, correctly-parented socket is a safe workaround when the target bone is virtual/IK-only.

## Decisions made 2026-07-23 through 2026-07-26

- **T0.3 — keep `BP_ZombieAIController`**, in case it is wanted later.
- **T0.5 / OQ-B9-01 — all gamepad work and testing deferred to B9.**
- **OQ-X-01 — PC only for the initial launch.**
- **Zombie AI native migration + navmesh fix — done, PIE-verified 2026-07-26.**
- **OQ-B0-11 temporary unblock** — a real (if wrong-looking) melee config exists for testing; question itself still open.

## Blocking decisions needed before B0-T2 (not before T0/T1)

- **OQ-B0-13 — item-instance refactor go/no-go.** The hard blocker; ~5–6 sessions of B0-T2 depend on it, and half is unrecoverable if the direction changes mid-way. Design doc: `Docs/Planning/InventoryLoadoutEquipping_Plan.md`.
- Also blocking B0, in the same design session: **OQ-B0-01** (scroll arbitration), **OQ-B0-02** (aim cone), **OQ-B0-04** (temperature scope), **OQ-B0-05** (fatigue/perception), **OQ-B0-07** (infection ambiguity in UI), **OQ-B0-11** (melee weapon display — temporarily unblocked for testing, real answer still needed).
- **Three contradictions need your call**: `Docs/Beta/00_MasterPlan.md` §2 — **CR-01** (skill roster), **CR-02** (vehicles), **CR-10** (fatigue/perception reading).
- **`UI_Plan.md`'s own §7 open questions** are an unchecked entry-criterion for B1 — not just background reading.

## Verification status — carried forward, still current

**PIE-confirmed working:** the AnimBP rifle-pose fix and basic hotbar switching (2026-07-22, Stage A). **Zombie AI** — wander, investigate, chase (2026-07-26). **Ranged hitscan damage** — confirmed applying, muzzle origin needs re-check after the socket fix.

**Still unverified, staged for pass 2 above:** Stage B (equip visuals, attachment sockets, magazine, `TP_Mesh` swap, rifle pose reappearing), Stage D re-check, Stage E (real 3-way switching), Stage F (melee dispatch, durability break), Stage G (container now visible, re-verify loot-all/drop/encumbrance).

**Known gap, still unfixed:** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso — amputation's Arms/Legs infection-clearing path is unreachable from a real bite. Scheduled as **B0-T5.1**.

## Other still-open items (lower priority)

Crouch pose bug untouched; temporary hit-confirmation logging still needs removing (→ B0-T5.5).
