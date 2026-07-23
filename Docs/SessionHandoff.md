# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-22, live session, follow-up #12) — weapon config restructure + anim rifle-pose fix (both C++ and graph wiring, saved to disk) + detailed test plan. Nothing PIE-verified yet.

Three things this round, all requested together:

**1. `UZSWeaponConfig` moved off Infima's skeletal test rifle onto static-mesh parts.** New **Setup** section (`BaseWeaponMesh`/`TriggerMesh`/`MagazineMesh`, all `UStaticMesh`) and **Attachments** section (`MuzzleMesh`/`HandguardMesh` - flashlight or laser/`GripMesh`/`OpticMesh`, each with its own socket field), replacing the old `MeshReceiver`/`MeshMagazineSK`/`MeshScope`/`MeshSightFront`/`MeshSightRear`/`MeshSilencer` fields (the three iron-sight/scope fields consolidated into one `OpticMesh` slot). `AZSWeapon`'s root component is `BaseWeaponMesh` now (`UStaticMeshComponent`, was skeletal `SK_Receiver`); `GetReceiverMesh()` renamed `GetBaseWeaponMesh()`. `AZSMagazine`'s prop mesh is static too. Compiled clean (dev-confirmed). **Every existing `DA_ZS_WeaponConfig_*` data asset needs re-authoring against the new fields** - the old ones are gone, not aliased. (Saw `DA_ZS_WeaponConfig_AssaultRifle1.uasset` and a new `Content/ZS/Weapons/Pistol/` folder appear in the working tree during this round - looks like this may already be underway, not independently verified.)

**2. Found and fixed the "stuck in idle rifle state" bug end-to-end - root cause was different from what it looked like.** The AnimGraph's *base locomotion* was already correctly wired to `BS_ZS_Unarmed_Idle_Walk_Run`/`BS_ZS_UnequippedCrouchWalk` (confirmed via `unreal-mcp` graph inspection) - that part was never broken. The actual bug: `LayeredBoneBlend_0`'s upper-body blend weight was hardcoded to `1.0`, so the rifle idle/aim pose always blended in regardless of whether a weapon was equipped. Fix: a new `bHasWeaponEquipped` bool on `UZSAnimInstanceBase` (mirrors `AZSPlayerCharacter::GetCurrentWeapon() != nullptr`, same pattern as the existing `bIsAiming`/`bIsCrouched`) gates a new `BlendListByBool` node inserted between `LayeredBoneBlend_0` and `Slot_0` in `ABP_ZS_ThirdPerson`'s AnimGraph. **Both halves done this round**: C++ compiled clean, AnimGraph wired via `unreal-mcp` (`create_node`/`connect_pins`), verified node-by-node via `get_node_infos` that the wiring is exactly right (including that `LayeredBoneBlend_0`'s pre-existing connections were undisturbed), `compile_blueprint` succeeded with no errors in the log, and `save_assets` confirmed the `.uasset` is no longer dirty and shows as modified in `git status`. **Not yet PIE-tested** - that's Stage A of the new test plan. Full technical detail: `CLAUDE.md`'s Character Skeleton & Animation section.

**3. New detailed test plan**: `Docs/Testing/P5_P6_CharacterSetupVerification.md` - a step-by-step character-setup verification (spawn state → equip → attack → unequip → weapon-switching → melee → inventory), each step with an expected result and a "if wrong, check this" note. More granular than this file's own checklist, which now just points at it for P5/P6 purposes.

**Docs also updated**: `CLAUDE.md` (Weapons/ bullet, Character Skeleton & Animation section - including a path-accuracy fix, `Content/ZSAnims/` → `Content/Animation/ZSAnims/`, and a note on which `unreal-mcp` tools actually work on AnimGraph node types vs. don't - `read_graph_dsl`/`write_graph_dsl` return empty for `AnimGraphNode_*` types, use `find_nodes`/`get_node_infos`/`create_node`/`connect_pins` instead), `Docs/Planning/InventoryLoadoutEquipping_Plan.md` (noted the cosmetic-attachment half of its Tier 2 proposal is now real, the stat-modifying half still isn't).

**Note**: `ABP_ZS_ThirdPerson.uasset` is modified on disk (saved via `unreal-mcp`, not by you in-editor) - it'll show up as a change to review/commit alongside whatever else is in your working tree, same as any other Content change this session didn't stage or push itself.

## Immediate next step

1. **Compile All Blueprints** (Content Browser bulk action) before trusting anything in PIE - standing practice, and this round touched a lot of native surface (`UZSWeaponConfig`'s field shape, `AZSWeapon`'s root component type) plus a direct AnimGraph edit.
2. **Re-author every `DA_ZS_WeaponConfig_*` data asset** against the new Setup/Attachments fields - old fields are gone, not aliased. (See the note above - some of this may already be in progress.)
3. Then work through `Docs/Testing/P5_P6_CharacterSetupVerification.md` top to bottom, starting with Stage A (which is specifically the anim fix's first real test).

## Carried forward, still current

**P5's hotbar/melee/durability/knockback and P6's whole inventory/loot system - built two rounds ago, still not PIE-tested.** Also still needed for a full pass: `IA_HotbarSelect`/`IA_HotbarCycle` (saw both as new untracked assets this round - may already be done), `BP_ZS_PlayerCharacter`'s `StartingHotbarLoadout` re-authored (saw this file modified this round too), P6 content (item/loot-table data assets, at least one container/world item placed - not yet seen). Full detail in `Docs/Testing/P5_P6_CharacterSetupVerification.md`'s own Prerequisites section - not duplicating it here.

**Two autonomous P6 design calls from two rounds ago still need your review** (bag-slot depth: `Back`+`Hip`; rarity-pool model: global per-session) - flagged in `GameDevPlan.md` §7 P6 and `Docs/Phases/P6_InventoryLoot.md`, cheap to change now, less cheap once content is authored against them.

**Two new planning docs from last round still need your read-through**: `Docs/Planning/InventoryLoadoutEquipping_Plan.md` and `Docs/Planning/UI_Plan.md` - both draft proposals, each ending in a compressed Open Questions list. The item-instance refactor they propose gets pricier the more P6 content gets built against today's shape.

**Unrelated to P5/P6:**
- First full P3+P4 integration proof, PIE-confirmed (2026-07-21): zombie chase → melee → damage → infection roll all worked end-to-end. Your character from that test is still `Incubating` on a Torso wound - treatment items and infection progression toward `Queasy` are still queued to test.
- **Known gap, flagged not fixed, deprioritized repeatedly**: `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso unconditionally - amputation's infection-clearing path (Arms/Legs only) is unreachable from a real bite. See `Docs/Phases/P3_HealthDamageMedical.md`.

## Other still-open items (lower priority, no action needed yet)
`BT_Zombie`'s wander branch has zero children; `BP_ZombieAIController`'s fate (unused) undecided; crouch pose bug untouched; temporary hit-confirmation logging still needs removing once real impact feedback exists.
