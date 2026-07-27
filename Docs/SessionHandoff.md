# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.
>
> **Away session?** If the user opened with "start the next part of the plan" or similar (they're stepping away, not working alongside you), read `Docs/AsyncSessionProtocol.md` once now and follow it for the rest of the session without re-reading it.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **Start here, not there**: `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` is the curated build-first, test-in-order, decisions-to-make companion for everything below — read it before doing anything else this session.

## Last completed (2026-07-26) — large autonomous push, nothing compiled or PIE-tested yet

The dev stepped away and asked for as much of B0 to get done solo as possible, with a compiled test list and decisions log for the return. Result: **T2 Steps B-E, T5, T6, T7, T4, T3, T9, T10 (remainder), T11, and T12.1 are all code-complete** (ten commits, `b947dd1`..`cc19659` plus the T2/T5/T6/T7 commits before them — `git log --oneline b0-baseline..main` shows the full sequence). **None of it has been compiled, Live-Coding-patched, or opened in the editor** — the editor was never touched this stretch, per standing policy (don't attempt `Build.bat` while it might be open; no way to run PIE headlessly either way).

**What's left of B0**: T0.2 (Compile All Blueprints pass), the T1 verification sweep (Stages C onward — Stage C's own blocker was already root-caused/fixed in a prior session), every checkpoint/PT listed in `B0_Stabilization.md` and the companion doc, T8.2 (a small open BT-wiring decision), and T12.2-T12.5 (packaged-build profiling) — all of these need the dev's hands (editor, PIE, or an actual judgment call), not more solo code.

## Next step

1. **Full `Build.bat` rebuild** (not Live Coding — dozens of headers changed this stretch). Regenerate IDE project files too (new files: `Player/ZSCameraDirector.h/.cpp`, `Framework/ZSElevationSubsystem.h/.cpp`).
2. Fix whatever the first real compile pass surfaces — none of this code has been type-checked by a compiler yet, only by careful reading against established patterns.
3. **"Compile All Blueprints" pass**, clean, before trusting any PIE result.
4. Then work through `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` in order — it lists content that needs authoring first (new Input Actions, a few Data Assets, two `TSubclassOf` Blueprint-class assignments), then a single ordered test checklist spanning every system touched this stretch, then a decisions log of judgment calls and documented gaps worth a second look.

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits.
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**
- **When stuck on an engine-level setup problem, check the official UE 5.8 docs site** before extended trial-and-error or engine-source spelunking.
- **After large multi-file sessions, regenerate IDE project files** (`Build.bat -projectfiles ...`) — several new files this stretch (see above); confirm Rider's project files are current before trusting IntelliSense.

## Known tooling gotchas (worth remembering)

- `unreal-mcp`'s `SkeletalMeshTools.add_socket` does not reliably honor `bone_name` for at least one bone on this project's skeleton (`weapon_r` — confirmed a genuine tool bug, not a virtual-bone limitation) — silently parents to `root` instead. Confirmed fine for `pelvis`/`hand_r`. **Workaround**: drive the real Skeleton Tree UI via `SlateInspectorToolset` instead (right-click bone → "Add Socket" → F2 to rename), then verify with `get_socket_bone`.
- **Any mesh rigidly attached to the character needs `NoCollision` explicitly set** (standing convention, `CLAUDE.md` Architecture section) — worth checking on any future attached cosmetic (clothing, held items), not just weapons.
- `unreal-mcp` was disconnected for this entire autonomous stretch — every Data Asset, Blueprint, Input Action, and level that would normally get authored alongside the C++ was instead left as an explicitly documented content gap (see the companion checklist doc's Section 1/3.3). None of it was faked or silently skipped.

## Decisions made 2026-07-23 through 2026-07-26

See `Docs/Beta/00_MasterPlan.md` §2 for the full rescope decision log (two-stage plan, infection now plainly legible not ambiguous, vehicles back in scope, 4+ players, etc.). See `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` §3 for every judgment call and documented gap from this session's solo push specifically (melee-stamina interpretation, downed-zombie BT stand-in, offhand-weapon-fire scope cut, and more) — flagged for review, not blocking.

## Verification status

**PIE-confirmed working (from before this stretch):** AnimBP rifle-pose fix, weapon placement (Stage B), hotbar unequip (Stage D), hotbar cycling all 3 slots (Stage E — note: hotbar *cycling* itself was since removed, B0-T3.4, scroll wheel is camera zoom now), melee dispatch/damage/durability break (Stage F), container loot-all + world item pickup (Stage G), zombie AI (wander/investigate/chase), ranged hitscan-turned-projectile combat vs. zombies (AssaultRifle + Pistol, single + 2-player), Item-instance Checkpoint A (drop/re-pickup GUID persistence).

**Code-complete, awaiting first compile + PIE verification:** everything listed in "Last completed" above — see `Docs/Beta/B0_ChecklistAndDecisions_2026-07-26.md` for the full ordered test list.

**Known gap, not a bug:** `AZombieCharacter::Server_MeleeAttack`'s bite-zone bug (always landing on Torso) has a code fix in place (B0-T5.1) but isn't PIE-confirmed yet.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation still needs removing before B1 (→ B0-T5.5, partially done — the `Server_Fire`/`Server_MeleeAttack` hit-confirmation logging was replaced with a real delegate hook, but the muzzle-trace debug draw and Stage G's interact/inventory logging are still in place). Three temporary `ZS.Debug*` console commands (drop/store-in-bag/list-carry-slots) exist purely for Checkpoint A/C testing without a real inventory UI — remove once one exists.
