# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Full history lives in git commit log, not here.
>
> **Plan of record has two halves now.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **B0-T0 complete. B0-T8 (zombie AI) complete and PIE-verified.** **B0-T1 (verification sweep) is in progress** — pass 1 found 4 bugs (fixed, PIE-confirmed by the dev except the weapon socket/collision fix); the dev's own pass-2 testing found 2 more real bugs (fixed, needs a rebuild + re-test before continuing).

## Last completed (2026-07-26) — root-caused the weapon movement/placement bug; fixed container interaction; revised CLAUDE.md

### The real bug behind "character moves erratically / camera in body / weapon in wrong spot"
The dev's pass-2 test (after the socket fix from the previous handoff) still showed the weapon spawning underneath the character with the character then moving upward indefinitely — and a different socket the dev tried by hand (`RightHandSocket` on `ik_hand_gun`) produced the same class of symptom, just in a different direction. That "wrong regardless of socket" pattern was the tell: **`AZSWeapon::BaseWeaponMesh` (the weapon's root component) never had its collision explicitly disabled.** Every other cosmetic mesh on the weapon (trigger, muzzle, attachments) gets `NoCollision` via `AssignNewStaticMesh` — `BaseWeaponMesh` is built directly in the constructor and that call was simply missing. Left at the default blocking collision profile, the mesh overlaps the character's own capsule the instant it's attached, and `CharacterMovementComponent`'s penetration-resolution logic fights that overlap forever — that's the "moving upward indefinitely" / "moved in another direction" / "camera in the body" symptom, all from the same root cause. Fixed in `Source/ZombieShooter/Weapons/ZSWeapon.cpp` (constructor): `BaseWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision)`. **This needs a rebuild before it takes effect** — it's `.cpp`-only (no header signature change), so Live Coding (Ctrl+Alt+F11) is fine per the standing B0 policy, or a full `Build.bat` if you'd rather.

### Socket choice — new dedicated `SocketGunAttachment` socket on `weapon_r`
The dev's own `RightHandSocket` (on `ik_hand_gun`) still didn't look right "even after adjustments." Likely reason: `ik_hand_gun` is an IK *control* bone — on a rig with no active IK solver driving it (this project's AnimGraph doesn't have one), it just sits at a fixed reference-pose location, not tracking the actual animated hand. First attempt to fix this reused Infima's own pre-existing `weapon_r_muzzle` socket (zero offset on `weapon_r`, a bone that's part of the real skinned animation chain — confirmed via the Skeleton Tree UI, not just the scripted tool, after the dev correctly pushed back on an earlier unverified claim). Per dev request, **that was replaced with a new, dedicated socket** instead of reusing the muzzle one: `SocketGunAttachment`, also on `weapon_r`, zero offset, created via the real Skeleton Tree UI (right-click `weapon_r` → Add Socket → rename) rather than the scripted `add_socket` tool, since that tool is confirmed broken for this bone (see the tooling gotcha below). All 3 weapon configs (AssaultRifle, Pistol, Crowbar) now point at `SocketGunAttachment`.

**Known side effect, not fully explained**: the dev's `RightHandSocket` socket disappeared from the skeleton during this same UI session — likely an Escape-key/rename-flow interaction while automating the Skeleton Tree, exact cause not pinned down. No functional impact (nothing referenced it anymore), but flagging it since it wasn't intentional.

**Content-tracking gap, worth knowing**: `SKM_Manny_Simple` lives under the gitignored `Content/InfimaGames/` tree, so this new `SocketGunAttachment` socket is **not captured anywhere in git** — a fresh clone that reinstalls Infima's pack from Fab won't have it. If that ever happens, recreate it: right-click `weapon_r` in `SKM_Manny_Simple`'s Skeleton Tree → Add Socket → rename to `SocketGunAttachment`, leave the offset at zero.

**Test this together** — the collision fix and the new socket — as one clean pass, since the collision bug alone could account for placement looking wrong in every attempt so far.

### Container interact — real bug found and fixed
Walking up to `BP_ZS_Container_Test` and pressing interact did nothing (no log, no visual change). `UpdateNearestInteractable`'s detection scan finds interactable actors via their *physical* collision (`ContainerMesh`, not `UZSInteractableComponent` itself — that component has no collision of its own, it's just a marker). `ContainerMesh`'s collision *profile* was already correct (`BlockAllDynamic`, `ECC_WorldDynamic`), but the static mesh asset (`SM_Small_Wood_Box_Closed`) may not have shipped with real collision geometry, which would make the profile setting moot — no collision primitives to actually trace against. Generated fresh convex hull collision on that mesh (`StaticMeshTools.generate_convex_collisions`) to guarantee it's detectable regardless. Also double-checked the loot table itself (3 entries, weights 5/3/1, `numRolls=3`) and the container's code path (`BeginPlay` → `RollLoot` → `bIsInteractable = ContainerSlots.Num() > 0`) — both look correct, so this was very likely the actual fix. Needs re-test.

### CLAUDE.md revised
Per dev request: the Architecture section had accumulated into a session-by-session build log (dated incidents, "same night" narration, deferred-feature laundry lists) instead of a stable reference. Rewritten to describe current-state responsibility only — what each folder/class does *now*, not the history of how it got there. Also: added `Docs/Beta/` as a first-class reference throughout (it was previously unmentioned despite being half the plan of record), updated "Development Order" to point at the Beta plan instead of the now-historical P0-P10 narrative, consolidated the Character Skeleton & Animation section (moved its still-useful AnimGraph-editing tooling note into the MCP lessons section, cut the play-by-play bug narrative), and added two new durable lessons: the cosmetic-attachment-collision rule (Architecture) and the `add_socket` virtual-bone bug (MCP tooling). Detailed incident history for anything cut lives in git log, same convention as `SessionHandoff.md` itself.

## Runbook — B0-T1 Stages B–G, pass 3 (do this next)

Same loadout — **AssaultRifle (1) / Pistol (2) / Crowbar (3, melee)**. **First, rebuild** (Live Coding or full, per above) and confirm the Output Log is clean.

1. **Stage B** — press `1`. Confirm the rifle now sits correctly on the character, and that equipping no longer causes any movement/camera weirdness at all (this was the real bug — if it's still happening after the rebuild, the collision fix didn't take, not a new issue).
2. **Stage C** — fire, confirm the shot originates from the now-correctly-placed muzzle.
3. **Stage D** — press `1` again to unequip (already confirmed working).
4. **Stage E** — press `1`, `2`, `3` in sequence, confirm real cycling (already confirmed working from pass 2) and that all three now look right.
5. **Stage F** — Crowbar (`3`) melee a zombie, confirm stats (22 dmg), land ~15 hits to break it.
6. **Stage G** — container near spawn (~450,300): interact, confirm loot-all now actually works. World item pickup (~300,450): loot, drop, re-pick-up, check encumbrance.

**File failures as discrete notes, don't fix inline** (T1.10). Report back pass/fail per stage.

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits — tonight's `ZSWeapon.cpp` fix qualifies.
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**
- **When stuck on an engine-level setup problem, check the official UE 5.8 docs site** before extended trial-and-error or engine-source spelunking.
- **After large multi-file sessions, regenerate IDE project files** (`Build.bat -projectfiles ...`) — no header/new-file changes yet this stretch, not needed.

## Known tooling gotchas (worth remembering)

- `unreal-mcp`'s `SkeletalMeshTools.add_socket` does not reliably honor `bone_name` for at least one bone on this project's skeleton (`weapon_r` — confirmed a real, non-virtual bone via the Skeleton Tree UI, so this is a genuine tool bug, not a virtual-bone limitation) — silently parents to `root` instead, only a stray log line as a clue. Works fine for ordinary bones like `pelvis`. **Confirmed workaround**: use `SlateInspectorToolset` to drive the real Skeleton Tree UI instead — right-click the target bone row → "Add Socket" → rename via F2. This creates the socket correctly (verified via `get_socket_bone` after). The read-only query tools (`get_socket_names`/`get_socket_bone`/`get_socket_transform`) are reliable — it's specifically the scripted `add_socket` creation path that's broken.
- **Any mesh rigidly attached to the character needs `NoCollision` explicitly set.** This is now also recorded as a standing convention in `CLAUDE.md`'s Architecture section — worth checking on any *future* attached cosmetic (clothing, held items) too, not just weapons.

## Decisions made 2026-07-23 through 2026-07-26

- **T0.3 — keep `BP_ZombieAIController`**, in case it is wanted later.
- **T0.5 / OQ-B9-01 — all gamepad work and testing deferred to B9.**
- **OQ-X-01 — PC only for the initial launch.**
- **Zombie AI native migration + navmesh fix — done, PIE-verified 2026-07-26.**
- **OQ-B0-11 temporary unblock** — a real melee config exists for testing; question itself still open.
- **Weapon socket — new dedicated `SocketGunAttachment` on `weapon_r`, not `ik_hand_gun`/`RightHandSocket` or the reused `weapon_r_muzzle`** — see above; revisit if it turns out wrong too.

## Blocking decisions needed before B0-T2 (not before T0/T1)

- **OQ-B0-13 — item-instance refactor go/no-go.** The hard blocker; ~5–6 sessions of B0-T2 depend on it. Design doc: `Docs/Planning/InventoryLoadoutEquipping_Plan.md`.
- Also blocking B0, same design session: **OQ-B0-01** (scroll arbitration), **OQ-B0-02** (aim cone), **OQ-B0-04** (temperature scope), **OQ-B0-05** (fatigue/perception), **OQ-B0-07** (infection ambiguity in UI), **OQ-B0-11** (melee weapon display).
- **Three contradictions need your call**: `Docs/Beta/00_MasterPlan.md` §2 — **CR-01** (skill roster), **CR-02** (vehicles), **CR-10** (fatigue/perception reading).
- **`UI_Plan.md`'s own §7 open questions** are an unchecked entry-criterion for B1.

## Verification status — carried forward, still current

**PIE-confirmed working:** AnimBP rifle-pose fix, basic hotbar switching, hotbar cycling across all 3 slots (dev confirmed pass 2), Stage D unequip, zombie AI (wander/investigate/chase), ranged hitscan damage.

**Still unverified, staged for pass 3 above:** weapon placement + no movement/camera side-effects (fix applied, needs rebuild + re-test), Stage B full detail (attachment sockets, magazine, `TP_Mesh` swap), Stage F (melee dispatch, durability break), Stage G (container interact — fix applied, needs re-test).

**Known gap, still unfixed:** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso. Scheduled as **B0-T5.1**.

## Other still-open items (lower priority)

Crouch pose bug untouched; temporary hit-confirmation logging still needs removing (→ B0-T5.5).
