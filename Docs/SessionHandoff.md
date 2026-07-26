# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **B0-T0 complete. B0-T8 (zombie AI) complete and PIE-verified.** **B0-T1 (verification sweep) still in progress, pass 4 not yet run.** Pass 3's blocker (Stage C — ranged hits landing on the wrong actor) is root-caused and fixed (see below), and combat itself has since moved onto a bigger change (the projectile system, T10.8) that pass 3 never tested. **B0-T2 stays gated until a pass-4 sweep re-confirms all stages, including ranged combat via the new projectile path** — do not start B0-T2 yet.

## Last completed (2026-07-26) — Stage C root-caused + fixed, projectile system built and rolled out to AR + Pistol, T10.9 bug found and code-fixed (untested)

**Stage C root cause (pass 3's blocker): `AZombieCharacter_C`'s capsule had an explicit `Visibility: ECR_Ignore` override**, letting the hitscan trace (`ECC_Visibility`) pass straight through every zombie to whatever level prop was behind it — unrelated to the `weapon_r`→`RightHandSocket` socket work done earlier (that was a real, separate bug: `weapon_r` abandoned entirely, new `RightHandSocket` created on `hand_r`, all 3 weapon configs updated). Fixed: capsule's `Visibility` response set to `ECR_Block`. Both fixes are content-only (Blueprint CDO / Data Asset properties), no rebuild was needed, and both were PIE-confirmed by the dev before combat moved on to the bigger change below.

**Projectile system (T10.8), dev-approved "full projectile system, go ahead":** new `AZSProjectile` actor (`Source/ZombieShooter/Weapons/ZSProjectile.h`/`.cpp`) — sphere collision root, cosmetic `NoCollision` mesh child, `UProjectileMovementComponent` (no gravity/bounce). New `UZSWeaponConfig` fields `ProjectileClass`/`ProjectileMesh`/`ProjectileSpeed` (opt-in per weapon — unset keeps the old hitscan path). `Server_Fire_Implementation` branches on `Config->ProjectileClass` right after computing the muzzle `TraceStart`.
- **AssaultRifle**: PIE-confirmed hitting zombies correctly, **2-player replication check passed** (projectile travel visible on both the firing client and the host).
- **Pistol**: config wired up identically (`ProjectileClass`/`ProjectileMesh` set, same values as AR) — **not yet PIE-confirmed by the dev**, first thing to check next session.
- `ProjectileMesh` is the engine placeholder Sphere for both weapons — needs a real bullet mesh per weapon eventually (content task, dev's to source).
- Deferred, not scoped: bullet casing ejection (dev's idea, noted in T10.8, pick up once the core system is solid).

**T10.9 — new bug found during the AR 2-player check, now code-fixed but NOT yet compiled or PIE-tested:** Player 2 (non-host client)'s cursor-facing rotation (`UpdateCursorFacing`'s `SetActorRotation()`) never reached the server — it's a local-only call, outside `CharacterMovementComponent`'s replication path. Affects every cursor-facing-gated action for non-host clients (ranged/melee/interact), not just fire. **Fix**: new `Server_UpdateCursorFacingRotation(FRotator)` — `Unreliable` server RPC (`ZSPlayerCharacter.h`/`.cpp`), called from `UpdateCursorFacing` alongside the existing local `SetActorRotation`. **This is a header change — needs a full `Build.bat` rebuild, not Live Coding**, before it can be tested.

**Weapon-attachment sockets — answered, no code change:** `SocketMuzzle`/`SocketTrigger`/`SocketMagazineAttachment`/`SocketHandguard`/`SocketGrip`/`SocketOptic` all live on each **weapon's own Static Mesh asset** (create via that mesh's Static Mesh Editor → Socket Manager), not on the character skeleton — separate from `RightHandSocket` (the one skeletal socket, on `hand_r`, for attaching the whole weapon to the hand). The projectile spawn point already reuses `Config->SocketMuzzle` (same socket the hitscan trace and `MuzzleMesh` attachment use) via `BaseWeaponMesh->GetSocketLocation()`, falling back to eye height if the socket doesn't exist — so no new/separate socket is needed for the projectile specifically. Collision with the weapon itself is a non-issue regardless of spawn point: `BaseWeaponMesh` and all attachment sub-meshes are already `NoCollision`. Dev is creating/verifying `SocketMuzzle` on the AR/Pistol meshes themselves next session — worth a spot-check that projectiles now spawn from the actual barrel tip rather than eye height.

**Committed and pushed** (`63588a6`): Pistol projectile config, zombie collision fix, T10.9 RPC code, `TuningReference.md`/`B0_Stabilization.md` doc updates. `Content/Poly-MegaSurvivalFood/` (dev's own Fab assets) intentionally left untracked, not committed.

## Next step

1. **Full `Build.bat` rebuild** — required for T10.9's header change before anything else below can be tested.
2. **Confirm Pistol projectile in PIE** (equip, fire at a zombie — same pattern as AR).
3. **Re-test T10.9's exact repro**: Player 2 shooting in a direction that requires rotating, confirm the server now sees the correct facing (was previously silently wrong, no visible error).
4. **Pass 4 — full B0-T1 stage re-sweep** (Stages B–G), now that Stage C's root cause is fixed and combat has moved onto the projectile path. This is what actually unblocks B0-T2 (item-instance refactor) — the dev's original sequencing ("once the sweep is clean").
5. Dev is separately setting up `SocketMuzzle` on the AR/Pistol static meshes (see above) — not blocking, but re-verify projectile spawn origin once done.

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits.
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**
- **When stuck on an engine-level setup problem, check the official UE 5.8 docs site** before extended trial-and-error or engine-source spelunking.
- **After large multi-file sessions, regenerate IDE project files** (`Build.bat -projectfiles ...`) — new files this session (`ZSProjectile.h`/`.cpp`); confirm Rider's project files are current before next session if not already done.

## Known tooling gotchas (worth remembering)

- `unreal-mcp`'s `SkeletalMeshTools.add_socket` does not reliably honor `bone_name` for at least one bone on this project's skeleton (`weapon_r` — confirmed a genuine tool bug, not a virtual-bone limitation) — silently parents to `root` instead. Confirmed fine for `pelvis`/`hand_r`. **Workaround**: drive the real Skeleton Tree UI via `SlateInspectorToolset` instead (right-click bone → "Add Socket" → F2 to rename), then verify with `get_socket_bone`.
- **Any mesh rigidly attached to the character needs `NoCollision` explicitly set** (standing convention, `CLAUDE.md` Architecture section) — worth checking on any future attached cosmetic (clothing, held items), not just weapons.

## Decisions made 2026-07-23 through 2026-07-26

See `Docs/Beta/00_MasterPlan.md` §2 for the full rescope decision log (two-stage plan, infection now plainly legible not ambiguous, vehicles back in scope, 4+ players, etc. — too many individual decisions to duplicate here). Combat-specific:
- **Weapon/equip socket — `RightHandSocket` on `hand_r`**, not `weapon_r` (abandoned) or `ik_hand_gun` (rejected, IK-control bone). General right-hand equip point, not weapon-specific.
- **Ranged weapons move to a full simulated projectile system** — dev-approved, AssaultRifle + Pistol done, see above.
- **T0.3 — keep `BP_ZombieAIController`**, in case it's wanted later.
- **T0.5 / OQ-B9-01 — all gamepad work and testing deferred to B9.**
- **Zombie AI native migration + navmesh fix — done, PIE-verified.**

## Verification status

**PIE-confirmed working:** AnimBP rifle-pose fix, weapon placement (Stage B), hotbar unequip (Stage D), hotbar cycling all 3 slots (Stage E), melee dispatch/damage/durability break (Stage F), container loot-all + world item pickup (Stage G, log-confirmed), zombie AI (wander/investigate/chase), ranged hitscan-turned-projectile combat vs. zombies (AssaultRifle, single + 2-player).

**Not yet PIE-confirmed:** Pistol projectile rollout; T10.9's aim-sync RPC fix (needs rebuild first).

**Known gap, not a bug:** item drop has no input binding anywhere (`Server_DropItem` is never called) — deferred intentionally, to be designed alongside future inventory-management work.

**Known gap, still unfixed:** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso. Scheduled as **B0-T5.1**.

## Other still-open items (lower priority)

Crouch pose bug untouched. Temporary debug instrumentation still needs removing before B1 (→ **B0-T5.5**): muzzle-trace debug draw and `UE_LOG` hit-confirmation in `Server_Fire`/`Server_MeleeAttack`/`AZSProjectile::HandleHit`, plus Stage G's interact/inventory logging (`TryInteract`, `ZSContainerActor`/`ZSWorldItemActor`/`ZSInventoryComponent`).
