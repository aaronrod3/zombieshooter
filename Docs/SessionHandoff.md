# Session Handoff

> Read this first, every session. This file is rewritten every session, not appended to — it holds only the last completed task and what's needed next. This is the **sole owner of verification status** (compiled? PIE-tested?) — `CLAUDE.md` and `Docs/Phases/P<N>_*.md` describe architecture/checklist state, not status, to avoid the same fact needing edits in three places. Plan of record: `Docs/GameDevPlan.md`. Conventions: `CLAUDE.md`. Full history lives in git commit log, not here.

## Last completed (2026-07-22, live session, follow-up #13) — PIE-confirmed: anim rifle-pose fix works, hotbar switching works

**Stage A of `Docs/Testing/P5_P6_CharacterSetupVerification.md` passed**: the character no longer shows a rifle-holding pose while unarmed - the `bHasWeaponEquipped`-gated `BlendListByBool` fix (including the branch-inversion correction from the round before this) is confirmed working in PIE, not just compiled clean. This is the **first real PIE confirmation of anything built across the last several unsupervised/live rounds** (P5 hotbar, P5 melee/durability/knockback, P6 inventory, the weapon-config restructure) - everything up to now had only been reviewed/compiled, never actually run.

**Hotbar switching also confirmed working**: pressing number keys switches the equipped weapon. **Not yet independently verified at the granularity `Docs/Testing/P5_P6_CharacterSetupVerification.md`'s Stage B asks for** - specifically: the equip-time delay being visibly present (not instant), each attachment mesh (Muzzle/Handguard/Grip/Optic) appearing at the right socket, the magazine actor appearing, the body mesh (`TP_Mesh`) swap, and the rifle upper-body pose re-appearing on equip (Stage A only confirmed it *disappears* correctly when unarmed, not that it *reappears* correctly when equipped again). Worth a closer look at Stage B specifically before treating the whole loadout loop as done - the basic mechanism clearly works, but the detail-level checks haven't all been confirmed individually yet.

## Immediate next step

Continue through `Docs/Testing/P5_P6_CharacterSetupVerification.md` from Stage B onward (Stage A is done):
1. **Stage B** - equip delay timing, attachment sockets, magazine, body mesh swap, rifle pose reappearing.
2. **Stage C** - ranged attack dispatch (hitscan from `SocketMuzzle`).
3. **Stage D** - unequip (re-press same slot) - confirms the anim fix from the equipped-to-unarmed direction too, not just spawn-time.
4. **Stage E** - switching between two different weapons, if you have two authored.
5. **Stage F** - melee weapon dispatch (needs a `Melee`-typed config - none authored yet, may need to temporarily flip one).
6. **Stage G** - P6 inventory/loot (needs content: item configs, a loot table, a placed container/world item - none of this exists yet as far as I've seen).

## Carried forward, still current

**P6's whole inventory/loot system (`UZSInventoryComponent`, containers, world items, loot tables) - built two rounds ago, still not PIE-tested at all.** No content authored for it yet (no `DA_ZS_ItemConfig_*`/`DA_ZS_LootTableConfig_*`, no containers/pickups placed) - that's Stage G's prerequisite.

**Every existing `DA_ZS_WeaponConfig_*` needs re-authoring against the new static-mesh Setup/Attachments fields** (old `MeshReceiver`/`MeshMagazineSK`/etc. fields are gone). Two configs now exist in the working tree - `DA_ZS_WeaponConfig_AssaultRifle1` and a new `DA_ZS_WeaponConfig_Pistol` - which is exactly what Stage E (switching between two weapons) needs; worth putting both in `StartingHotbarLoadout` if not already there. Still worth confirming attachments specifically (Stage B) since an unset attachment just silently doesn't appear, no error.

**Two autonomous P6 design calls still need your review** (bag-slot depth: `Back`+`Hip`; rarity-pool model: global per-session) - flagged in `GameDevPlan.md` §7 P6 and `Docs/Phases/P6_InventoryLoot.md`, cheap to change now, less cheap once content is authored against them.

**Two planning docs still need your read-through**: `Docs/Planning/InventoryLoadoutEquipping_Plan.md` and `Docs/Planning/UI_Plan.md` - both draft proposals, each ending in a compressed Open Questions list. The item-instance refactor they propose (fixing durability-doesn't-persist and hotbar-doesn't-reference-inventory) gets pricier the more P6 content gets built against today's shape - worth deciding before Stage G content authoring goes very far.

**Unrelated to P5/P6:**
- First full P3+P4 integration proof, PIE-confirmed (2026-07-21): zombie chase → melee → damage → infection roll all worked end-to-end. Your character from that test is still `Incubating` on a Torso wound - treatment items and infection progression toward `Queasy` are still queued to test.
- **Known gap, flagged not fixed, deprioritized repeatedly**: `AZombieCharacter::Server_MeleeAttack` passes a blank `FHitResult`, so every zombie bite lands on Torso unconditionally - amputation's infection-clearing path (Arms/Legs only) is unreachable from a real bite. See `Docs/Phases/P3_HealthDamageMedical.md`.

## Other still-open items (lower priority, no action needed yet)
`BT_Zombie`'s wander branch has zero children; `BP_ZombieAIController`'s fate (unused) undecided; crouch pose bug untouched; temporary hit-confirmation logging still needs removing once real impact feedback exists.
