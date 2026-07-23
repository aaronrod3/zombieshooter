# P5/P6 Character Setup — Step-by-Step Verification

> Written 2026-07-22, at the dev's request for something more thorough than the quick checklist in `Docs/SessionHandoff.md` — this one is specifically about verifying the **character/weapon/mesh setup is actually correct** at every stage of the loadout flow, not just "does the feature technically work." Each step says what to do, exactly what you should see if it's right, and what a wrong result most likely means. Run top to bottom the first time; after that, treat each stage as independently re-runnable when you touch something in that area.
>
> This supersedes the P5/P6 section of `Docs/SessionHandoff.md`'s testing checklist for setup-correctness purposes — that file still owns the quick pass/fail list and any carried-forward P3/P4 items.

## 0. Prerequisites — same as before, restated precisely for this round

This round changed more than usual, so be precise about what needs to happen before Stage A:

1. **Full compile** (Rider/VS `ZombieShooterEditor Win64 Development`, or `Build.bat` via PowerShell — not Ctrl+Alt+F11, not Bash). This round touched:
   - `UZSWeaponConfig`: removed `MeshReceiver`/`MeshMagazineSK`/`MeshScope`/`MeshSightFront`/`MeshSightRear`/`MeshSilencer`/`SocketScope`/`SocketSightFront`/`SocketSightRear`; added `BaseWeaponMesh`/`TriggerMesh`/`MagazineMesh`/`MuzzleMesh`/`GripMesh`/`OpticMesh`/`SocketTrigger`/`SocketGrip`/`SocketOptic` (all `UStaticMesh`, no skeletal weapon mesh anymore).
   - `AZSWeapon`: root component renamed `SK_Receiver`(`USkeletalMeshComponent`) → `BaseWeaponMesh`(`UStaticMeshComponent`); `GetReceiverMesh()` renamed `GetBaseWeaponMesh()`, return type changed.
   - `AZSMagazine`: mesh component skeletal → static.
   - `UZSAnimInstanceBase`: new `bHasWeaponEquipped` bool property.
2. **Compile All Blueprints** pass after — standing practice, doubly important this round given the property/class-shape changes above.
3. **Every existing `DA_ZS_WeaponConfig_*` data asset needs re-authoring.** The old skeletal mesh fields are gone - any weapon config authored before this round has lost its mesh assignments and needs the new `BaseWeaponMesh`/`TriggerMesh`/`MagazineMesh` (Setup) and optionally `MuzzleMesh`/`HandguardMesh`/`GripMesh`/`OpticMesh` (Attachments) fields set from static mesh sources (`Content/LowPolyWeapons/`, `Content/Mega_Survival_Tools/`). **A weapon config with `BaseWeaponMesh` unset will equip successfully (no error) but render as an invisible weapon** - don't mistake that for a bug, it's a straightforward "forgot to assign the mesh" content gap.
4. **The anim graph fix itself may still be pending** depending on when you're reading this — check with me directly before Stage A if you're not sure it's actually wired in yet (see the note at the end of this doc).
5. `IA_HotbarSelect`/`IA_HotbarCycle` created, `BP_ZS_PlayerCharacter`'s `StartingHotbarLoadout` re-authored with at least one (ideally two) fully-set-up weapon configs.

## Stage A — Spawn state (before touching any input)

This is the stage the dev's original bug report was about — verify it first, in isolation, before doing anything else.

| # | Action | Expected | If wrong |
|---|---|---|---|
| A1 | Enter PIE, don't press anything | Character stands idle. | — |
| A2 | Look at the character's upper body/arms | **No rifle-holding pose** — arms should read as relaxed/idle, not gripping an invisible weapon. | If still rifle-posed: the anim graph fix (`bHasWeaponEquipped` gating `LayeredBoneBlend_0`) either wasn't compiled, wasn't wired in yet, or `Compile All Blueprints` wasn't run after it was. Check with me. |
| A3 | Walk forward/back/strafe | Legs use the unarmed walk cycle (`BS_ZS_Unarmed_Idle_Walk_Run`) - this was already correct before this round's fix, shouldn't have changed. | If legs look wrong, that's a *different*, previously-unreported bug — the fix this round only touched the upper-body layer's weight, not the base locomotion blend spaces. |
| A4 | Crouch (still no weapon) | Legs switch to `BS_ZS_UnequippedCrouchWalk`. | Same note as A3. |
| A5 | Check the body mesh visually | Should be whatever `AZSPlayerCharacter::UnarmedBodyMesh` cached at `BeginPlay` (the CDO/BP-authored default skeletal mesh) - not a leftover gun-holding mesh from a previous session's testing. | If it looks like a weapon-holding body mesh already: `BP_ZS_PlayerCharacter`'s own default `SkeletalMesh` on the mesh component may itself be set to a weapon-pose mesh - check the Mesh component's defaults directly, not just `StartingHotbarLoadout`. |

## Stage B — Equipping the first weapon

| # | Action | Expected | If wrong |
|---|---|---|---|
| B1 | Press the hotbar number for your first authored weapon | Nothing happens instantly - there should be a visible pause matching that weapon's `EquipTimeSeconds`. | Instant equip = `Server_SelectHotbarSlot`'s timer isn't running, or `EquipTimeSeconds` is 0/unset on that config. |
| B2 | After the delay | The weapon actor appears, attached at the character body's `SocketGunAttachment`. | Nothing appears = `BaseWeaponMesh` unset on the config (see Prerequisites #3), or `SocketGunAttachment` doesn't exist on the body mesh being used. |
| B3 | Look at the weapon itself | `BaseWeaponMesh`'s static mesh renders correctly at the weapon actor's root. | — |
| B4 | Check each authored attachment (whichever of Muzzle/Handguard/Grip/Optic you set on this config) | Each attachment mesh appears at its own socket (`SocketMuzzle`/`SocketHandguard`/`SocketGrip`/`SocketOptic`) on `BaseWeaponMesh`, not floating in space or at the origin. | Attachment missing entirely = that socket doesn't exist on the static mesh asset (check in the Static Mesh Editor) - `AssignNewStaticMesh` silently no-ops if the socket's missing, by design (same "unset/missing = no-op" convention as everywhere else), so this fails silently rather than erroring. Attachment floating at the wrong spot = the socket exists but its transform wasn't authored correctly on the mesh asset. |
| B5 | Check the magazine | A separate `AZSMagazine` actor appears at `SocketMagazineAttachment`, showing `MagazineMesh`. | Missing = `MagazineMesh` unset on the config, or the socket's missing on `BaseWeaponMesh`. |
| B6 | Check the character's upper body now | Rifle idle pose should now show (the `bHasWeaponEquipped`-gated layer is active again - this is the "equipped" half of the same fix verified in A2). | If it *doesn't* show a rifle pose while equipped: the new gate is inverted, or `TP_IdlePose`/`TP_AimPose`/whatever feeds `SequencePlayer_3`/`_4` isn't assigned on this weapon's config. |
| B7 | Check the character's body mesh | Should have swapped to this weapon's `TP_Mesh` (a full skeletal mesh swap, separate from the weapon actor itself). | If unchanged: `TP_Mesh` unset on the config, or `RefreshBodyMeshFromWeapon` isn't running (check logs). |

## Stage C — Ranged attack dispatch (only if this weapon's `AttackType` is `Ranged`)

| # | Action | Expected | If wrong |
|---|---|---|---|
| C1 | `IA_Attack` (fire) at a target | Hitscan fires from the weapon's `SocketMuzzle` location (not eye height) - this works **independent of whether `MuzzleMesh` is actually assigned**, since `SocketMuzzle` is read directly off `BaseWeaponMesh` regardless of what (if anything) is attached there. | If the trace still originates from eye height: `BaseWeaponMesh`'s static mesh asset doesn't actually have a socket named exactly `SocketMuzzle` (or whatever `Config->SocketMuzzle` was changed to) - check `DoesSocketExist` isn't silently failing due to a name mismatch. |
| C2 | Watch for the existing on-screen hit confirmation | Green "Shot hit X for Y" message, plus a log line - both still temporary/unremoved from P4, unrelated to this round's changes. | — |
| C3 | Hit a zombie a few times | Target visibly shoves backward (`FireKnockbackStrength`, built two rounds ago) - not new this round, just a good moment to re-confirm nothing regressed. | — |

## Stage D — Unequip (re-press the same hotbar number)

This is Stage A's fix verified from the *other* direction — equipped-to-unarmed, not just spawn-time.

| # | Action | Expected | If wrong |
|---|---|---|---|
| D1 | Press the currently-equipped slot's number again | Delay (`UnequipTimeSeconds`), then the weapon actor and its magazine both disappear. | — |
| D2 | Check the body mesh | Reverts to `UnarmedBodyMesh` (Stage A's mesh), not left on the weapon's `TP_Mesh`. | — |
| D3 | Check the upper body pose | **Rifle pose should disappear entirely** - back to Stage A2's relaxed-arms state, not stuck mid-transition or still showing the rifle layer. | If it's stuck rifle-posed after unequip specifically (but was fine at initial spawn): `bHasWeaponEquipped` isn't being re-evaluated every tick, or `CurrentWeapon` isn't actually going null on the client's local view (check replication - `OnRep_CurrentWeapon` should fire and `RefreshBodyMeshFromWeapon`/`AttachWeaponToBodyMesh` should both run). |

## Stage E — Switching between two different weapons (needs ≥2 authored in `StartingHotbarLoadout`)

| # | Action | Expected | If wrong |
|---|---|---|---|
| E1 | Equip weapon A, then without unequipping, select weapon B's hotbar slot | Weapon A's actor (and all its attachments/magazine) is fully destroyed before weapon B's is spawned - no visual overlap, no leftover attachment meshes from A lingering after B appears. | Since `EquipWeapon` always destroys the old `AZSWeapon` actor before spawning a new one, and attachment components are owned by that actor (destroyed automatically with it), a leftover-attachment bug here would indicate something is holding a stale component reference outside the actor's own lifetime - worth flagging directly if seen, this shouldn't be possible given how the code's structured. |
| E2 | Check weapon B's own attachments | Match weapon B's own config, not weapon A's (i.e. confirm nothing was accidentally shared/cached between configs). | — |
| E3 | Cycle through slots with the scroll wheel a few times rapidly | Each switch completes (or is cleanly ignored if attempted mid-switch, per `bIsBusy` gating) - no stuck/partial states. | — |

## Stage F — Melee weapon dispatch (only if a `Melee`-typed config exists to test — none is authored yet as of this doc; you may need to temporarily flip an existing config's `AttackType` to `Melee`)

| # | Action | Expected | If wrong |
|---|---|---|---|
| F1 | Equip a `Melee`-typed weapon, `IA_Attack` near a target | Uses that config's own `MeleeDamage`/`MeleeRange`/`MeleeAttackInterval` (visible in the on-screen hit log's damage number), not the bare-fist `Unarmed*` values. | Bare-fist numbers showing while a melee weapon's equipped = `HandleAttack`'s dispatch fell through incorrectly - check `CurrentWeapon->GetConfig()->AttackType` is actually `Melee` on the asset. |
| F2 | Land enough hits to exceed `MaxDurabilityHits` | Weapon breaks: auto-unequips, disappears from its hotbar slot, and re-selecting that (now empty) slot does **not** bring it back. | If it comes back at full durability: the hotbar slot wasn't actually cleared (`HotbarSlots[ActiveHotbarIndex] = nullptr` didn't run) - check the break-handling branch in `Server_WeaponMeleeAttack`. |
| F3 | Watch the target on a landed hit | Knockback per `MeleeKnockbackStrength`. | — |

## Stage G — P6 inventory interplay (needs content: at least one `DA_ZS_ItemConfig_*`, ideally a loot table + container/world item placed)

| # | Action | Expected | If wrong |
|---|---|---|---|
| G1 | Interact with a placed/dropped `AZSWorldItemActor` | Item added to your carry list, actor disappears. | — |
| G2 | Drop an item | New `AZSWorldItemActor` spawns in front of you holding it, item leaves your carry list. | — |
| G3 | Loot a container | Everything transfers in one action; container stops being interactable once empty. | — |
| G4 | Carry enough weight to exceed capacity | Movement speed visibly drops (`GetEncumbranceMultiplier` folded into `UpdateMovementSpeed`). | — |
| G5 | Equip a `bIsEquippable` item into `Back`/`Hip` | Max carry weight increases by that item's `CarryCapacityBonus`. | — |
| G6 | **Cross-check with P5**: does picking up a weapon via G1 make it selectable on the hotbar? | **No — this is a known, already-documented gap** (`Docs/Phases/P6_InventoryLoot.md`, `Docs/Planning/InventoryLoadoutEquipping_Plan.md`). `HotbarSlots` still only reflects what's authored in `StartingHotbarLoadout`, not what's actually carried. Don't treat this as a new bug - it's the exact problem the item-instance refactor proposal in the planning doc exists to fix. | — |

## A note on the anim graph fix specifically

Both halves are done: the C++ (`UZSAnimInstanceBase::bHasWeaponEquipped`) and the AnimGraph wiring (a new `BlendListByBool` node between `LayeredBoneBlend_0` and `Slot_0`, gated on that bool) - wired via `unreal-mcp`, compiled clean (`compile_blueprint` + a log check for errors on `ABP_ZS_ThirdPerson`, none found), and saved to disk (`ABP_ZS_ThirdPerson.uasset` confirmed no longer dirty, shows as modified in `git status`). **Not yet PIE-verified** - Stage A above is the first real test of it. If Stage A still shows the rifle-pose bug despite all that, something more specific is wrong (worth a fresh look, not just re-checking whether the wiring happened - it did).
