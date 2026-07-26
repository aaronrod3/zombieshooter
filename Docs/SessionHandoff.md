# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` describes architecture/current-state design, not status, to avoid the same fact needing edits in multiple places. Full history lives in git commit log, not here. (`Docs/Phases/` — the old pre-Beta-plan status stubs — was deleted 2026-07-26, fully superseded by `Docs/Beta/`.)
>
> **Plan of record has two halves now.** `Docs/GameDevPlan.md` = design (pillars, §3 scope contract, Decisions). `Docs/Beta/` = production plan to beta (phases B0–B12, tasks, gates). Start at `Docs/Beta/README.md`. Conventions: `CLAUDE.md`.

## Current phase: B0 — Stabilization & Reconciliation

`Docs/Beta/B0_Stabilization.md`. **B0-T0 complete. B0-T8 (zombie AI) complete and PIE-verified.** **B0-T1 (verification sweep) is in progress** — pass 3 (Stages B–G) just ran, dev-tested in PIE. **Not clean**: Stage C failed (new bug — see below), Stage G's drop/re-pickup/encumbrance sub-test couldn't run at all (no input binding exists for drop). Everything else passed. **B0-T2 stays gated until pass 4 re-verifies Stage C** — do not start B0-T2 yet.

## ⚑ Full plan rescoped 2026-07-26 — read `Docs/Beta/00_MasterPlan.md` §2 before assuming anything about design intent

The dev asked for a full review of the plan because scope felt too concentrated and too many decisions were being made without real input. Claude reviewed every file in `Docs/GameDevPlan.md` and `Docs/Beta/`, wrote `Docs/Planning/RescopeQuestionnaire.md`, and the dev answered it. Every `Docs/Beta/*.md` file, `GameDevPlan.md`, and the two `Docs/Planning/` design docs were then revised against those answers. **Full detail lives in `00_MasterPlan.md`'s Contradiction Register — read it, don't rely on this summary alone.** Headlines, several of which **reverse** what the docs said before this session:

- **Process, not just content:** checkpoint after each individual feature/fix from here on (not phase-end only), ask before implementing anything design-shaping, minimize chained dependency between steps, written test scripts the dev runs personally before anything is claimed done.
- **Two-stage plan:** Stage 1 = "Core Playable Loop" (B0, B1, B3, B4-systems-only-on-graybox, B6-Sys) — get every core system rough-but-working and tested before content volume. Stage 2 = everything else (content, narrative, art, audio, release). See `00_MasterPlan.md` §3.2.
- ⚑ **Infection is now plainly legible, not ambiguous.** The "can't tell if it's a cold or a bite" horror pillar is reversed — show the player clearly when they're bitten/infected. Biggest single content reversal from this session; nothing was built against the old version yet.
- **Vehicles are back in scope** (later in dev, ready for beta) · **4+ players, not hard-locked 2–4** · **an optional paid dedicated-server path is planned** · **stat-affecting weapon attachments are wanted** (scopes/silencers with real effects) · **map is bigger, built in phases** (region content is now a continuous track, `T_ContinuousTracks.md` T7, not a single 45–60 session phase) · **procedural/randomized basements are cut** (fixed authored map for now) · **death always respawns into the same persistent world**, no asymmetric solo-ends-the-world rule · **melee weapon display resolved** (grouped poses: long-gun/pistol/melee) · **genuinely large zombie hordes (100+) confirmed important**, plus a new zombie "freshness" mechanic to design (faster/stronger when freshly turned, degrading over time).
- `ZombieShooter_Consolidated_Changes.md` (cited throughout the old `Docs/Beta/` as the source of "CONFIRMED" decisions) **does not exist in this repo** — the dev has it and can supply it if a specific gap needs it, but treat anything sourced from it that the rescope didn't directly touch as still worth double-checking, not gospel.
- `Docs/Planning/ZombieShooter_Open_Questions_For_Beta.md` was deleted — fully superseded by the answered `RescopeQuestionnaire.md` and the updated `90_OpenQuestions.md` (recoverable from git history if ever needed).
- **Second answer batch, same day (still going):** the dev worked through most of the remaining X/B0–B4 open questions directly in chat (not via the questionnaire file this time), plus a few from B5/B10. All folded into `90_OpenQuestions.md`, `B0_Stabilization.md`, `B4_WorldContent.md`, `T_ContinuousTracks.md`, and `GameDevPlan.md` §3.1 (new **Lockpicking** skill, per OQ-B4-08). **`90_OpenQuestions.md` was also stripped down** at the dev's request — resolved questions are now a short decision record instead of the full option-table debate; still-open questions keep their tables. See that file's Summary section for the current tally.
- **`OQ-B3-01` conflict resolved:** dev confirmed "one continuously-overwritten world stays" — no save-slot rollback, as originally recorded.
- **One open ask for Claude's input, answered but not yet confirmed:** `OQ-B10-04` (disconnect handling) — the dev asked for help designing an anti-combat-log system; a proposal (safe-logout reusing the sleep-safety gate + a server-tracked in-combat flag) is drafted in `90_OpenQuestions.md`, dev said they'll look it over later.
- **New: `Docs/InputBindings.md` created** — the dev's full target keymap (movement/combat/camera/UI/hotbar/vehicles/multiplayer), not yet implemented against. It resolved two open questions (light-source key is `T` not `F`; `Alt+R` "Rack Firearm" is the jam-clear input) and **changed one already-written task**: the downed-zombie finisher moved from contextual-on-`IA_Attack` to a dedicated `Space` key, bundled with two new, not-yet-designed moves (Shove, Mount/Climb) — `B0_Stabilization.md` T10.6 updated accordingly. It also surfaced three genuinely new, undesigned mechanics, now tracked as `OQ-X-09` (Run vs. Sprint as two speed tiers), `OQ-X-10` (a PvP weapon-safety toggle — no PvP mode exists in the plan anywhere), and `OQ-X-11` (in-game text chat + push-to-talk voice, which needs `OQ-B10-09`'s "no voice chat" rec re-examined, not silently kept).

## Last completed (2026-07-26) — B0-T1 pass 3, dev-tested in PIE: Stages B/D/E/F pass, Stage C fails, Stage G partially blocked

The `ZSWeapon.cpp` collision fix (from the prior handoff) was confirmed already baked into the running editor's binary before this session started (DLL timestamp newer than the source edit) — Live Coding reported "no code changes detected," which was expected, not a failure.

### Stage-by-stage results
- **Stage B — PASS**, re-confirmed again after the `RightHandSocket`/`hand_r` switch and a full rebuild: all 3 weapons now spawn correctly and the socket is properly adjustable (previously stuck).
- **Stage C — FAIL, new bug.** Ranged hitscan (`Server_Fire`) consistently hit a `StaticMeshActor` instead of a nearby zombie, **with the dev's cursor confirmed on the zombie** at the time of firing. Not yet investigated (occlusion vs. a trace start/direction error are both plausible) — do not assume a cause without checking. Needs its own debugging pass before pass 4.
- **Stage D — PASS.** Unequip clean, re-confirmed with the collision fix in place.
- **Stage E — PASS.** Hotbar cycling (1/2/3) works, all three weapons now look right on the character.
- **Stage F — PASS.** Crowbar melee dispatch, damage, and durability break all confirmed.
- **Stage G — PARTIAL.**
  - Container loot-all: **PASS**, confirmed via new log instrumentation (see below) — `BP_ZS_Container_Test` transferred all 3 slots to the player's `CarrySlots`. The dev's initial "not seeing any change" was expected: there's no inventory UI yet, so a successful loot has nothing to visually confirm it by.
  - World item pickup: **PASS**, same log-based confirmation (`AZSWorldItemActor::HandleInteracted`).
  - Drop / re-pickup / encumbrance: **could not be tested at all.** `UZSInventoryComponent::Server_DropItem` exists but is never called from anywhere — no Input Action is wired to it, and `Docs/InputBindings.md` doesn't list a drop key either. Dev's call: **defer intentionally** — drop will be designed as part of upcoming inventory-management work, not bolted on ad hoc here. Not filed as a "bug," just an acknowledged gap to fold into that later work.

### Diagnostic logging added (temporary, needs removal later)
No part of the interact → loot/pickup chain had any logging, which is why Stage G was initially unverifiable. Added `UE_LOG(LogZombieShooter, ...)` at each decision point, same style/convention as the existing temporary `Server_Fire`/`Server_MeleeAttack` hit-confirmation logs (comment-tagged the same way):
- `AZSPlayerCharacter::TryInteract` / `Server_Interact_Implementation` (`Source/ZombieShooter/Player/ZSPlayerCharacter.cpp`) — logs why an interact was rejected, or confirms it reached the target.
- `AZSContainerActor::HandleInteracted` (`Source/ZombieShooter/Inventory/ZSContainerActor.cpp`) — logs rejection reasons and the number of slots transferred.
- `AZSWorldItemActor::HandleInteracted` (`Source/ZombieShooter/Inventory/ZSWorldItemActor.cpp`) — logs the item/count picked up.
- `UZSInventoryComponent::Server_AddItem` / `Server_DropItem` (`Source/ZombieShooter/Inventory/ZSInventoryComponent.cpp`) — logs resulting weight vs. max weight and the encumbrance multiplier.

All `.cpp`-only, Live Coding-compatible. **Fold this into the same cleanup pass as the existing temporary hit-confirmation logging (B0-T5.5)** — don't track it as a separate task.

## Next step — pass 4, gated on the Stage C bug

**Update:** dev did the full rebuild, could now adjust `RightHandSocket` directly (previously stuck), and confirmed **Stage B passes cleanly — all 3 weapons spawn correctly.** Stage C (shot direction) still needs verification, now with the socket fix confirmed in place.

Added temporary muzzle trace visualization to `Server_Fire_Implementation` (`Source/ZombieShooter/Player/ZSPlayerCharacter.cpp`) to make Stage C directly verifiable in PIE: a yellow debug sphere at `TraceStart` (the actual muzzle origin used) and a debug line along the trace — green to the impact point on a hit, red to `TraceEnd` on a clean miss, both persisting 8 seconds. `.cpp`-only, Live Coding-compatible. Same temporary-instrumentation convention as the rest of this session's logging — fold into the B0-T5.5 cleanup pass.

**Update, same session:** dev confirmed via the debug trace line that shot *direction* is correct — the trace visibly points at the zombie. That ruled out rotation/socket-direction entirely and pointed straight at collision instead. Checked `AZombieCharacter_C`'s capsule (`CollisionCylinder`) via `ObjectTools.get_properties`: profile `Pawn`, but with an **explicit custom response override — `Visibility: ECR_Ignore`**. `Server_Fire`'s hitscan trace uses `ECC_Visibility` (`Source/ZombieShooter/Player/ZSPlayerCharacter.cpp`), so the trace was passing straight through every zombie's capsule and continuing on to hit whatever *did* block Visibility further down the line — matching every symptom seen since Stage C started (hits landing on level props instead of the zombie, regardless of aim). Confirmed **not** C++-sourced (`AZombieCharacter`'s constructor sets no collision responses at all — checked `Source/ZombieShooter/Zombies/ZombieCharacter.cpp`), so this was a Blueprint-level override, likely leftover/accidental rather than intentional. **This is very likely the actual root cause of Stage C all along** — the socket work earlier in this session was a real, separate bug worth fixing, but wasn't what was breaking ranged combat.

Fixed: set `Visibility` response to `ECR_Block` on `AZombieCharacter_C`'s `CollisionCylinder`, verified via re-read, saved, Blueprint recompiled. Content-only change (Blueprint CDO property) — **no rebuild needed**, testable immediately.

1. Fresh PIE, fire at a zombie in open space — should now register hits directly on the zombie. Also worth a spot-check that this didn't change anything else observable (melee already used a separate `ECC_Pawn` object-type overlap query, not this Visibility channel, so Stage F shouldn't be affected — but confirm).
2. If Stage C still fails after this, both the socket and collision fixes are ruled out — look at the trace logic itself in `Server_Fire`, or a same-frame race between `bOrientRotationToMovement` and `UpdateCursorFacing` (replication ruled out; testing is standalone).
3. **Only then** does B0-T2 (item-instance refactor) become unblocked, per the dev's original sequencing ("once pass 3 is clean").

## B0-T0.1 — build policy for this phase (standing, for the duration of B0)

- **Full `Build.bat` rebuild for any header change.** Live Coding (Ctrl+Alt+F11) only for `.cpp`-only edits — tonight's `ZSWeapon.cpp` fix qualifies.
- **"Compile All Blueprints" pass after every patch cluster**, before trusting any PIE result.
- When something that "should just work" behaves wrong after a recompile, **check the Output Log for `is not a child class of` or `invalid target type` before anything else.**
- **When stuck on an engine-level setup problem, check the official UE 5.8 docs site** before extended trial-and-error or engine-source spelunking.
- **After large multi-file sessions, regenerate IDE project files** (`Build.bat -projectfiles ...`) — no header/new-file changes yet this stretch, not needed.

## Known tooling gotchas (worth remembering)

- `unreal-mcp`'s `SkeletalMeshTools.add_socket` does not reliably honor `bone_name` for at least one bone on this project's skeleton (`weapon_r` — confirmed a real, non-virtual bone via the Skeleton Tree UI, so this is a genuine tool bug, not a virtual-bone limitation) — silently parents to `root` instead, only a stray log line as a clue. Works fine for ordinary bones like `pelvis`. **Confirmed workaround**: use `SlateInspectorToolset` to drive the real Skeleton Tree UI instead — right-click the target bone row → "Add Socket" → rename via F2. This creates the socket correctly (verified via `get_socket_bone` after). The read-only query tools (`get_socket_names`/`get_socket_bone`/`get_socket_transform`) are reliable — it's specifically the scripted `add_socket` creation path that's broken.
- **Any mesh rigidly attached to the character needs `NoCollision` explicitly set.** This is now also recorded as a standing convention in `CLAUDE.md`'s Architecture section — worth checking on any *future* attached cosmetic (clothing, held items) too, not just weapons.
- **`weapon_r` abandoned entirely, 2026-07-26.** A zero-offset reset of `SocketGunAttachment` (the socket was found with an unexplained 209-unit/2.09m local Z offset from `weapon_r`, despite being documented as zero-offset) did **not** fix Stage C after a clean re-test away from level clutter — so `weapon_r` itself was the problem, not just a bad offset. Dev's call: **stop using `weapon_r`, move to `hand_r` instead** (a real chain bone, same family as the already-rejected `ik_hand_gun` IK-control bone but not itself an IK control — should track the actual animated hand). New socket **`RightHandSocket`** created on `hand_r`, zero offset, intended as **the general attachment point for anything equipped in the player's right hand, not weapon-specific**. Old `SocketGunAttachment` socket removed from `SKM_Manny_Simple` entirely.
  - Verified via `get_socket_bone` (correctly parented to `hand_r`, not silently to `root` — the known `add_socket` bug is apparently `weapon_r`-specific, didn't reproduce here) and `get_socket_transform` (zero offset confirmed).
  - The C++ **property** `UZSWeaponConfig::SocketGunAttachment` (`Source/ZombieShooter/Weapons/ZSWeaponConfig.h`) keeps its name (avoids an asset-wide rename) but now defaults to `TEXT("RightHandSocket")` — a header change, needs a **full `Build.bat` rebuild**, not Live Coding.
  - All 3 weapon configs (`DA_ZS_WeaponConfig_AssaultRifle`/`_Pistol`, `DA_ZS_WeaponConfig_Crowbar`) had this as an **explicit serialized override**, not just inheriting the C++ default — updated and saved individually via `ObjectTools.set_properties`, verified by re-reading each one back.
  - **The content changes alone (skeleton socket + 3 weapon config values) are sufficient to test right now, no rebuild required** — the header default change only affects hypothetical future weapon configs, not these three. The rebuild is a "keep it in sync" nicety, not a blocker.
  - **Needs a fresh PIE re-test of Stage B (weapon placement) and Stage C (ranged accuracy)** — this was a design decision by the dev in response to the zero-offset fix not working, not yet independently confirmed to fix Stage C itself.
  - Same untracked-in-git situation as before: `SKM_Manny_Simple` lives under gitignored `Content/InfimaGames/`. If a fresh clone reinstalls Infima's pack, recreate `RightHandSocket` on `hand_r` (zero offset) via the Skeleton Tree UI.

## Decisions made 2026-07-23 through 2026-07-26

- **T0.3 — keep `BP_ZombieAIController`**, in case it is wanted later.
- **T0.5 / OQ-B9-01 — all gamepad work and testing deferred to B9.**
- **OQ-X-01 — PC only for the initial launch.**
- **Zombie AI native migration + navmesh fix — done, PIE-verified 2026-07-26.**
- ~~OQ-B0-11 temporary unblock~~ — **now fully resolved** (not just unblocked): melee display is grouped poses by weapon category. Update the temporary crowbar config's pose grouping when T10.7 is actually implemented.
- **Weapon/equip socket — `RightHandSocket` on `hand_r`, not `weapon_r` (abandoned after the zero-offset fix didn't resolve Stage C), not `ik_hand_gun` (rejected earlier, IK-control bone with no active solver), not the reused `weapon_r_muzzle`.** Intended as a general right-hand equip point, not weapon-specific. See above.
- **Ranged weapons move to a full simulated projectile system, decided 2026-07-26 — dev confirmed "go ahead," Step 1 in progress.** Dev's call, made after confirming hitscan (`Server_Fire`'s instant `ECC_Visibility` line trace) is what's actually resolving hits today and works correctly post the collision fix above.
  - **Step 1 (in progress):** new `AZSProjectile` actor (`Source/ZombieShooter/Weapons/ZSProjectile.h`/`.cpp`) — sphere collision root (blocks WorldStatic/WorldDynamic/Pawn, ignores everything else including Visibility), cosmetic `UStaticMeshComponent` child (NoCollision, per the standing convention), `UProjectileMovementComponent` (no gravity, no bounce, straight-line). Reuses the existing `FireDamage`/`FireDamageTypeClass`/`FireKnockbackStrength` config fields for the hit contract rather than duplicating them - same values mean the same thing whether hitscan or projectile resolves them. New `UZSWeaponConfig` fields: `ProjectileClass` (unset = keep hitscan, the default/fallback for every weapon not yet migrated), `ProjectileMesh`, `ProjectileSpeed` (defaults to 6000 cm/s - deliberately slow/visible, not realistic bullet velocity, since legibility was the whole point of this change). `Server_Fire_Implementation` branches on `Config->ProjectileClass` right after computing the muzzle `TraceStart`, before the hitscan-only code. This is the one actor in `Weapons/` that needs real movement replication (`SetReplicateMovement(true)`) - everything else there is static-once-attached or spawn-and-forget.
  - **Needs a full `Build.bat` rebuild** (new class in new files - not Live Coding-safe) before anything can be tested or before the AssaultRifle config can be wired to a `ProjectileClass` (can't touch that Data Asset property until the class exists in the compiled binary).
  - **Not yet done:** assigning `DA_ZS_WeaponConfig_AssaultRifle`'s `ProjectileClass`/`ProjectileMesh`/`ProjectileSpeed` (next step after rebuild), PIE test, 2-player replication check, Pistol rollout, cosmetic polish (tracer/impact VFX). Pistol and Crowbar untouched for now - proving the concept on one weapon first, per the standing "minimize chained dependency between steps" rule.
- **Full plan rescope, 2026-07-26** — see the section above and `00_MasterPlan.md` §2 for the complete list; too many individual decisions to duplicate here.

## Blocking decisions before B0-T2 — resolved 2026-07-26, cleared by the rescope pass

All of the below are now answered (see `00_MasterPlan.md` §2 for full detail, `Docs/Beta/B0_Stabilization.md`'s rewritten B0-T2 for how it changes the task breakdown):

- ~~OQ-B0-13~~ — item-instance refactor: **go**, but as independently-testable steps, not one 5–6 session block.
- ~~OQ-B0-02, OQ-B0-04, OQ-B0-05, OQ-B0-07, OQ-B0-11~~ — aim-cone/headshot values, temperature scope, fatigue/perception, infection legibility (⚑ **reversed** — plain, not ambiguous), melee weapon display (grouped poses by category) — all resolved.
- ~~CR-01, CR-02, CR-10~~ — skill roster (longer list), vehicles (in scope, later), fatigue/perception reading (confirmed as assumed) — all resolved.
- **Still genuinely open, not blocking B0-T2 specifically**: `OQ-B0-01` (scroll-wheel arbitration between zoom and hotbar cycle) and `OQ-B0-03`'s specific stomp-finisher execution (mechanic confirmed wanted, needs a non-PZ-clone design pass before building — see `B0_Stabilization.md` T10.6). `UI_Plan.md`'s own §7 open questions are mostly still open too (not a B0 blocker, but check before B1 starts).

## Verification status — carried forward, still current

**PIE-confirmed working:** AnimBP rifle-pose fix, weapon placement + no movement/camera side-effects (Stage B, collision fix confirmed resolved), hotbar unequip (Stage D), hotbar cycling across all 3 slots with correct visuals (Stage E), melee dispatch/damage/durability break (Stage F), container loot-all (Stage G, log-confirmed), world item pickup (Stage G, log-confirmed), zombie AI (wander/investigate/chase).

**Known bug, not yet root-caused:** ranged hitscan (`Server_Fire`) hits a `StaticMeshActor` instead of a cursor-targeted zombie even with the cursor confirmed on the zombie (Stage C, pass 3, 2026-07-26). Blocks B0-T2 until fixed and re-verified.

**Known gap, not a bug:** item drop has no input binding anywhere (`Server_DropItem` is never called) — deferred intentionally, to be designed alongside future inventory-management work, not this sweep.

**Known gap, still unfixed:** `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso. Scheduled as **B0-T5.1**.

## Other still-open items (lower priority)

Crouch pose bug untouched; temporary hit-confirmation logging (`Server_Fire`, `Server_MeleeAttack`) plus this session's new Stage G interact/inventory logging both still need removing (→ B0-T5.5).
