# B0 — Stabilization & Reconciliation

**Size: L (14–18 dev-sessions)** · **Gate contribution: prerequisite for everything** · **Blocks: B1, B3, B4, and therefore all of Stage 1**

> **Why this phase exists.** Roughly four sessions of C++ shipped between 2026-07-21 and 2026-07-22 with a single PIE confirmation covering two features. Underneath it sits a data model the project's own planning doc says is wrong (`Docs/Planning/InventoryLoadoutEquipping_Plan.md` §3–§5). On top of it, the consolidated changes revise five shipped behaviours and add three subsystems. **Every one of those facts gets more expensive the longer it waits.** B0 is the cheapest this work will ever be.
>
> **The rule for this phase: no new player-facing features that aren't in the revision register.** If it isn't in `01_RevisionRegister_P0-P6.md`, it belongs in a later phase.
>
> **Rescoped 2026-07-26 for process, not just content** (`Docs/Planning/RescopeQuestionnaire.md`). Two changes apply throughout this file: **(1) checkpoint after each individual feature or fix**, not just at the six original PT milestones — the dev's explicit preference is to see/test things in small pieces, not phase-sized chunks. Every sub-task below that changes player-visible or testable behavior now ends with an explicit "stop and verify" note. **(2) Minimize chained dependency between steps** — where a task was previously "one uninterrupted block," it's now broken into independently-testable steps so a failure partway through doesn't invalidate everything built on top of it. B0-T2 (the item-instance refactor) is the main beneficiary of this; see its rewritten breakdown below. Several content decisions also changed — see the callouts inline (infection legibility reversed, melee display resolved, death/world-continuity simplified).

---

## Entry criteria

- [x] `Docs/Beta/00_MasterPlan.md` §2 Contradiction Register — **all items resolved 2026-07-26**, dev-confirmed (CR-01 through CR-12). Several reverse the original AI-guessed resolution (CR-02 vehicles, CR-06 infection legibility, CR-07 death/save rule, CR-04 camera fallback) — read the updated register before assuming anything about B0's tasks below matches what you remember.
- [x] **OQ-B0-13 answered** — do the item-instance refactor now, dev-confirmed, but as independently-testable steps (see rewritten B0-T2).
- [ ] Working tree committed. `git status` shows the large untracked content directories resolved one way or the other (`Content/Animation/`, `Content/Maps/`, `Plugins/`, etc. are currently untracked — decide gitignore vs. commit per the $0 LFS budget rule **before** a refactor makes the diff unreadable).
- [ ] A known-good build exists and is tagged (`git tag b0-baseline`), so any regression during the refactor has a bisect target.

## Exit criteria

- [ ] All of `Docs/Testing/P5_P6_CharacterSetupVerification.md` Stages A–G pass in PIE, 2-client.
- [ ] Every revision-register item marked for B0 is implemented **and PIE-verified**, not merely compiled.
- [ ] `FZSItemInstance` is the only way an item exists in the game. `grep` for `UZSWeaponConfig\*` in slot/container contexts returns nothing.
- [ ] A weapon looted from a container can be placed in the hotbar, equipped, used until it breaks, dropped at partial durability, picked back up, and still shows the same durability.
- [ ] A performance baseline exists on the fixed stress-test map, captured from a **packaged Development build**, and is committed to `Docs/Testing/`.
- [ ] `SessionHandoff.md` shows zero items in "built but unverified."
- [ ] `TuningReference.md` contains every new tunable introduced in this phase.

---

## Task breakdown

### B0-T0 — Build hygiene & refactor safety net · **S (1 session)** · *no dependencies*

The highest-C++-churn phase in the plan runs straight into `CLAUDE.md`'s Live Coding lesson. Set the guardrails first.

| Sub-task | Definition of done |
|---|---|
| T0.1 | **Standing policy recorded for B0:** full `Build.bat` rebuild over Ctrl+Alt+F11 for any header change; Live Coding only for `.cpp`-only edits. Written into `SessionHandoff.md`. |
| T0.2 | **"Compile All Blueprints" pass** run and clean. Any `is not a child class of` / `invalid target type` errors in the Output Log fixed *before* refactor work starts. |
| T0.3 | ✅ **RESOLVED 2026-07-23 — keep `BP_ZombieAIController`.** Dev decision: retained in case it is wanted later. It remains an unused Blueprint and therefore a live Live-Coding corruption surface, so **include it in every "Compile All Blueprints" pass** (T0.2) rather than assuming it is inert. Revisit in B2-T2.4's asset triage. |
| T0.4 | ✅ **Done 2026-07-23.** `git tag b0-baseline`; ~1.55 GB third-party content gitignored, ~39 MB authored content committed, `Docs/AssetSources.md` written as the reinstall record. |
| T0.5 | ✅ **RESOLVED 2026-07-23 — gamepad deferred to B9, not verified here.** Dev decision (OQ-B9-01, OQ-X-01: PC-only launch, core features first). Record as unverified and move on. **Do not test, do not fix.** The architectural hooks that prevent later rework are preserved in B1 — see OQ-B9-01's keep/defer split. |

---

### B0-T1 — Verification sweep · **M (4–5 sessions)** · *depends on T0*

**This runs before any refactor.** You cannot tell whether the refactor broke something if you never knew it worked.

Work through `Docs/Testing/P5_P6_CharacterSetupVerification.md` Stages B–G. Two content prerequisites must be met first — do them as part of this task:

**Bonus fix, 2026-07-25**: `StartingHotbarLoadout` slot 2 was pointing at `DA_ZS_WeaponConfig_AssaultRifle1` — an orphaned duplicate (near-identical tunables to the real `AssaultRifle` config, strongly suggesting an accidental in-editor duplicate) with no `BaseWeaponMesh` set, which per the testing doc's own warning would have equipped successfully but rendered invisible. Stage E ("switching between the two authored weapon configs") would have failed on this immediately, for content reasons unrelated to any real bug. Fixed: `DA_ZS_WeaponConfig_Pistol` (previously unauthored, zero referencers) got its meshes properly assigned from `SM_Pistols1_01` and distinct tunables from the rifle (shorter range/damage, faster equip, quieter — so Stage E's "confirm nothing was accidentally shared/cached" is actually testable); slot 2 now points at it; the stray `AssaultRifle1` asset was deleted after confirming zero referencers.

| Sub-task | Definition of done |
|---|---|
| T1.1 | ✅ **Done 2026-07-25.** `DA_ZS_WeaponConfig_Crowbar` authored (`Content/ZS/Weapons/Melee/`) — `SM_Crowbar` mesh, `AttackType=Melee`, real melee stats (22 dmg / 150 range / 0.9s interval / 350 knockback / 15-hit durability so Stage F2's break test is actually exercisable), reusing the rifle's `TP_Mesh` per OQ-B0-11's stated temporary measure. Added as `StartingHotbarLoadout` slot 3. **Still temporary** — OQ-B0-11 itself is unresolved, this only unblocks testing. |
| T1.2 | ✅ **Done 2026-07-25.** 3 `DA_ZS_ItemConfig_*` (`CannedFood` consumable, `Bandage`, `PistolPickup` weapon-as-item), 1 `DA_ZS_LootTableConfig_Basic` (3 rolls, weighted), all in `Content/ZS/Items/`. Container/world-item placement needed a wrinkle: `AZSContainerActor::LootTable` is `EditDefaultsOnly` and `AZSWorldItemActor::Item`/`Count` are `VisibleAnywhere` (code-set only, via `InitializeItem`) — neither is instance-editable on a raw placed actor, by design. Fixed by creating two trivial Blueprint children (`BP_ZS_Container_Test`, `BP_ZS_WorldItem_Test`) — the container's CDO has `LootTable` set to `DA_ZS_LootTableConfig_Basic`; the world item's `EventBeginPlay` calls `InitializeItem(CannedFood, 2)`. Both placed in `Lvl_ThirdPerson` near the PlayerStarts (300,300,302) and compiled clean. |
| T1.3 | **Stage B** — equip delay is visibly non-instant; each attachment mesh (Muzzle/Handguard/Grip/Optic) appears at its correct socket; magazine actor appears; `TP_Mesh` body swap occurs; rifle upper-body pose *re-appears* on equip. |
| T1.4 | **Stage C** — ranged hitscan originates from `SocketMuzzle`, applies `FireDamage` at `FireRange`, damages a zombie. |
| T1.5 | **Stage D** — re-pressing the equipped slot unequips; the AnimBP rifle-pose fix works in the equipped→unarmed direction. |
| T1.6 | **Stage E** — switching between the two authored weapon configs works; each shows its own meshes. |
| T1.7 | **Stage F** — `IA_Attack` dispatches to melee for a `Melee`-typed config; per-weapon melee stats apply; durability decrements; breaking unequips and clears the slot. |
| T1.8 | **Stage G** — container "loot all" transfers into `CarrySlots`; world item pickup works; drop spawns a world item; encumbrance affects movement speed. |
| T1.9 | **2-client re-baseline (PT1)** — see Playtest Checkpoints below. |
| T1.10 | Every failure found is filed as a discrete bug task, **not fixed inline**, unless it blocks the rest of the sweep. A verification pass that turns into a debugging pass never finishes. |

> **Expect failures.** This is 4+ sessions of unrun code. Budget the higher end of the session estimate and treat a clean sweep as the surprise, not the default.

---

### B0-T2 — Item-instance refactor · **L (5–6 sessions)** · *depends on T1; blocked by OQ-B0-13*

The architectural core of B0. Follows `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §5–§8, extended with the consolidated changes' container categories (CR-09) and condition variance (P6-R2).

**Data structures** (final target shape — extends the Planning doc's proposal):

```cpp
// Source/ZombieShooter/Inventory/ZSItemInstance.h  (new file)

UENUM(BlueprintType)
enum class EZSCarryLocation : uint8   // CR-09's four categories
{
    OnPerson,   // pockets/worn — always available, small, no bag required
    Bag,        // granted by an equipped Back/Hip container
    World,      // inside an AZSContainerActor
    Vehicle     // RESERVED — forward-compat only, see CR-02. No implementation.
};

USTRUCT(BlueprintType)
struct FZSItemInstanceState
{
    GENERATED_BODY()

    // -1 = uninitialised; resolve from Config->MaxDurabilityHits on first read.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentDurability = -1;

    // P6-R2 loot condition variance. 0..1 multiplier rolled within the rarity
    // tier's band at spawn. Scales effective durability and jam chance (P5-R1).
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float ConditionQuality = 1.0f;

    // Tier-2 attachments only. Empty unless/until OQ approves them.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EZSAttachmentSlot, FGuid> AttachedInstanceIds;
};

USTRUCT(BlueprintType)
struct FZSItemInstance
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid InstanceId;                       // stable for the item's whole life

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UZSItemConfig> Config = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackCount = 1;                   // meaningful only if MaxStackSize > 1

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EZSCarryLocation Location = EZSCarryLocation::OnPerson;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FZSItemInstanceState InstanceState;     // meaningful only if StackCount == 1
};
```

**Invariant to hold throughout:** stackable and per-instance-stateful are mutually exclusive by construction. `MaxStackSize > 1` ⇒ `InstanceState` is never read. This is what makes the model tractable.

**Restructured 2026-07-26 into independently-testable steps, per dev preference (`Docs/Planning/RescopeQuestionnaire.md` Part 0 + item-instance section).** All four bundled additions below are dev-confirmed KEEP; the change is sequencing, not scope. Each step ends with a real checkpoint — don't start the next step until the current one is PIE-verified. `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §8 is the source order.

**Step A — foundation (T2.1–T2.3).** The one part that genuinely can't be split further — the codebase is non-compiling mid-way through.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.1 | `FZSItemInstance` / `FZSItemInstanceState` / `EZSCarryLocation` defined; replicates correctly (`FGuid` and nested structs both replicate; verify in 2-client PIE, not by inspection). | Planning §5 |
| T2.2 | `UZSInventoryComponent::CarrySlots` → `TArray<FZSItemInstance>`. `Server_AddItem` mints a new GUID for `MaxStackSize == 1` items, merges stacks otherwise. | P6-R3 |
| T2.3 | `AZSWorldItemActor` and `AZSContainerActor` carry a full `FZSItemInstance`, not `Config + Count`. | P5-R5 |

> **✋ Checkpoint A.** Compile, 2-client PIE: pick up a stackable item and a non-stackable item, confirm both get correct GUIDs, confirm a dropped item's GUID survives being picked back up. Don't proceed to Step B until this passes.

**Step B — hotbar rewire & equip/unequip (T2.4–T2.8).** The single highest-value step — this is the literal fix for "you can't hotbar a looted weapon."

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.4 | `AZSPlayerCharacter::HotbarSlots` → `TArray<FGuid>`. `StartingHotbarLoadout` stays authorable as a list of configs but now seeds real instances into `CarrySlots` at `BeginPlay` and points the hotbar at their GUIDs. | P5-R4 |
| T2.5 | `UZSInventoryComponent::EquippedBack`/`EquippedHip` → `FGuid`. | Planning §5 |
| T2.6 | **Equip flow** resolves GUID → instance, seeds `AZSWeapon::CurrentDurability` from `InstanceState.CurrentDurability` (falling back to `Config->MaxDurabilityHits × ConditionQuality` when `-1`). | P5-R5 |
| T2.7 | **Unequip flow** writes `CurrentDurability` back to the instance *before* `CurrentWeapon->Destroy()`. | P5-R5 |
| T2.8 | **Breaking a weapon** removes the instance from `CarrySlots` entirely and clears the hotbar GUID — the item is genuinely gone, not orphaned. | Planning §5 |

> **✋ Checkpoint B (the headline acceptance test).** Loot a weapon from a container or the world, put it on the hotbar, equip it, take it to half durability, unequip, re-equip — **durability must still show half**, not reset to full. Break a weapon and confirm it's actually gone from the inventory, not just the hotbar. 2-client PIE. This is the specific bug the whole refactor exists to fix — don't move on until it's genuinely verified, not just "looks right."

**Step C — carry locations & loot condition (T2.9–T2.10).** Separable; can slip a session without blocking anything else.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.9 | **`EZSCarryLocation` enforcement**: `OnPerson` capacity is a small fixed base; `Bag` capacity exists only while a bag is equipped in `Back`/`Hip`. Unequipping a bag with items in it must have defined behaviour (drop to world / block the unequip — pick one and state it). | CR-09, P6-R1 |
| T2.10 | **`ConditionQuality` rolled at spawn** inside `UZSLootTableConfig`'s weighted roll, banded by rarity tier. | P6-R2 |

> **✋ Checkpoint C.** Equip a bag, confirm capacity rises; unequip it with items in the bag-only space and confirm the defined behavior actually happens. Loot the same item twice from a rare-tier table and confirm condition genuinely varies.

**Step D — ammo as a real item (T2.11).** Independent of B/C — could be done earlier if it's ever more convenient.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.11 | **Ammo becomes a real inventory item.** Remove `AZSWeapon::CurrentReserveAmmo`/`MaxReserveAmmo`; reload draws from a matching ammo `FZSItemInstance` stack. Ammo now weighs, loots, drops, and can be shared. | P5-R6, OQ-B0-09 |

> **✋ Checkpoint D.** Fire a weapon empty, reload from a carried ammo stack, confirm the stack count drops by the right amount. Drop ammo, have a second player pick it up and reload from it.

**Step E — handedness fields (T2.12).** Cheap, pure data-classification.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.12 | **`EZSWeaponHandedness { OneHanded, TwoHanded }`** + `bool bUsableInSecondaryHand` added to `UZSWeaponConfig`. Two-handed blocks `SecondaryHand`. Arm amputation restricts to `OneHanded`. | P5-R7, P3-R8 |

> **✋ Checkpoint E.** Confirm a two-handed weapon config genuinely blocks `SecondaryHand` (once B0-T11 exists) and that the field is readable/settable per weapon.

**Step F — full-cycle regression (T2.13).**

| Sub-task | Definition of done |
|---|---|
| T2.13 | Full 2-client PIE pass on the loot→hotbar→equip→break→drop→repick cycle, all steps above together. |

**Deliberately not in B0:** stat-affecting weapon attachments (scopes/silencers with real gameplay effects). The dev confirmed he wants these eventually — reversing the design doc's own recommendation against building them — but they're scoped as their own later weapon-depth pass (Stage 2), once this foundation is solid, per `Docs/Planning/InventoryLoadoutEquipping_Plan.md` §8's own step 7.

---

### B0-T3 — Camera, aiming & elevation revision · **M (4–5 sessions)** · *depends on T1; gated by PT2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T3.1 | **`UZSCameraDirector`** (new component or `AZSPlayerController` subobject) owns camera distance. Fixed preset min/max zoom range from tunables. | P1-R2 |
| T3.2 | **Auto-zoom context stack** — `EZSCameraContext { Outdoor, Interior, Underground, Driving(reserved) }`, each with a preset distance. Context changes drive a smooth interpolation, not a snap. | P1-R3 |
| T3.3 | **Manual override** immediately and fully disengages auto-zoom, no cooldown. Store the context that was active at override time; auto-zoom resumes only on a transition to a *different* context. | P1-R4 |
| T3.4 | **Scroll-input arbitration** resolved and implemented — `IA_HotbarCycle` is currently on the mouse wheel and zoom now wants it. | OQ-B0-01 |
| T3.5 | **Aim-cone model.** `Server_Fire` resolves within a spread cone rather than a perfect ray. Per-weapon `HipFireSpreadDegrees` / `AimedSpreadDegrees` on `UZSWeaponConfig`. | P1-R5 |
| T3.6 | **Headshot weighting** — the cone resolves to a **body zone**, not just a point, feeding the existing 4-zone model. Separate hip-fire vs. aimed weighting values. | P1-R5, OQ-B0-02 |
| T3.7 | **Elevation interface + single-floor stub.** `IZSElevationProvider` (or a `UZSElevationSubsystem`) answers "what floor/Z-plane is this actor on?" Aim rays resolve against that plane. B0 ships a stub that always returns the ground plane; B4 implements real multi-level. **Building the interface now means B4 is a swap, not a retrofit.** | P1-R6 |
| T3.8 | **PT2 camera feel/tuning checkpoint passes** (below). ⚑ **Changed 2026-07-26**: this is no longer a go/no-go gate on whether to delete the perspective code — the dev confirmed top-down permanently, no fallback wanted. A bad PT2 result now means *re-tune* (cone width, zoom presets, headshot weighting), not *revert*. | — |
| T3.9 | Delete `ToggleCameraPerspective`, `IA_ToggleView`, `EZSCameraPerspective`, and the ThirdPerson camera path **without waiting on PT2's result** — the dev already confirmed this direction. Update `CLAUDE.md`'s architecture section. | P1-R1, CR-04 |

---

### B0-T4 — Needs simulation revision · **M (3–4 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T4.1 | **`Wet`** added to `UZSNeedsComponent` as a replicated binary flag (+ a dry-out timer). Debug setter for testing; real weather source wired in B4. | P2-R1 |
| T4.2 | Wet feeds footstep noise — louder or audibly distinct via `UZSNoiseSystem`. Verify the radius change actually pulls zombies differently (PT4). | P2-R1 |
| T4.3 | **`Temperature`** added as a single replicated body-temperature scalar. **Scope discipline: four inputs only** — ambient (weather/time-of-day), indoor/outdoor, wet multiplier, clothing insulation sum. No per-limb thermal, no layering system. | P2-R2, OQ-B0-04 |
| T4.4 | `UZSItemConfig` gains `InsulationValue`. Equipped clothing sums into the temperature model. | P2-R2 |
| T4.5 | Hypothermia/hyperthermia consequences defined as need-severity tiers feeding `GetPerformanceMultiplier()` — **not** a separate damage path. | OQ-B0-04 |
| T4.6 | **Fatigue → perception degradation** implemented per the resolution of CR-10/OQ-B0-05. | P2-R3 |
| T4.7 | **Hunger/Thirst penalty-only** — verify `GetPerformanceMultiplier()` never exceeds 1.0 from being sated. | P2-R4 |
| T4.8 | **Encumbrance penalizes stamina drain, never hard-locks sprint.** Remove any encumbrance gate in `StartSprint`; scale sprint stamina drain by `GetEncumbranceMultiplier()`. | P2-R5 |
| T4.9 | **Severity thresholds authored** for all 8 needs, 4 tiers each, in `UZSNeedsConfig` + `TuningReference.md`. B1's moodle UI depends on these existing. | P2-R8 |
| T4.10 | **`IsSafeToSleep()` implemented for real.** ⚑ **Expanded 2026-07-26 (dev-confirmed)** — this is a gate on whether sleep can be *initiated* at all, not just a vulnerability warning. Two conditions, both required: (1) no recent hostile detection/pursuit (an aggro-cooldown state, not just "no hostile within radius right now"), and (2) the player's immediate location provides real shelter — barricaded room, behind a locked door, or inside a vehicle. Minecraft-style: you can't just lie down in the open street. Sleeping fast-forwards time, so the gate is what keeps that meaningful. | P2-R7, OQ-B0-06 |

---

### B0-T5 — Wound model revision · **S (2–3 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | **Fix the blank `FHitResult` bite bug.** `AZombieCharacter::Server_MeleeAttack` must pass a real hit result so zone inference works. **Amputation's Arms/Legs infection-clearing path becomes reachable from a real bite for the first time.** | P3-R6 |
| T5.2 | **Remove bandage decay.** A bandage stays effective until the wound heals. Keep the wound's own `bDirty` flag (it drives wound-infection risk and is `Server_Disinfect`'s target). Higher-tier heal item supersedes and removes the bandage. | P3-R3, CR-05 |
| T5.3 | **Critical head bleed** — rare outcome on Head-zone wounds, steep bleed rate, distinct and urgent feedback. Chance + rate as tunables. | P3-R4 |
| T5.4 | **Multi-day fracture recovery** on the `AZSGameState` game-hour clock. Splint shortens but does not trivialize. | P3-R5 |
| T5.5 | Remove the temporary hit-confirmation logging; replace with a minimal impact VFX/SFX stub (real pass in B7). | P5-R11 |

---

### B0-T6 — Two-tier infection · **S (2–3 sessions)** · *depends on T5*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T6.1 | **`EZSWoundInfectionState`** per zone, distinct from `EZSInfectionStage`. Progresses on the game-hour clock when a wound is dirty and untreated. Slows healing. **Never fatal alone.** | P3-R1 |
| T6.2 | `Server_Disinfect` cures wound infection; **explicitly does nothing to bite infection.** | P3-R1 |
| T6.3 | ⚑ **REVERSED 2026-07-26 (dev-confirmed) — was "preserve ambiguity," is now the opposite.** Both tiers must be **plainly, legibly shown** — the player should know when they've been bitten and know when an infection (either tier) is active. No more "identical signals, no UI element names the tier." This constrains B1 the same way the old rule did, just in the other direction — record it as a design requirement on the HUD (show it clearly), not a suppression rule. | P3-R2, CR-06 |
| T6.4 | **Bite-infection fatal timeline** authored in in-game hours per stage into `UZSHealthConfig` + `TuningReference.md`. ⚑ **Range widened 2026-07-26 (dev-confirmed)**: 2–4 in-game days, not a flat 3 — the window's purpose is giving the player time to make a deliberate choice about where to be when it runs out (see T9.1/T9.2). | P3-R12, OQ-B0-08 |
| T6.5 | **Medical-tier incubation delay** — per-tier delay field on `UZSItemConfig`'s Bandage/Disinfectant entries, extending the amputation decision window. | P3-R9 |

---

### B0-T7 — Amputation choreography & blackout · **S (2–3 sessions)** · *depends on T6*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T7.1 | Amputation gets a **montage + `bIsBusy` gate**, matching the project's own timed-action convention instead of a bare mutator. | P3-R10 |
| T7.2 | **Blackout state** — an incapacitated, vulnerable state after amputation. Not death, not normal play. | P3-R7 |
| T7.3 | **Solo:** game time accelerates ~12 in-game hours during blackout. Enemies can find and kill the incapacitated player, making location choice a real tactical decision. | P3-R7 |
| T7.4 | **Co-op:** teammate can move the downed body; a revive action shortens the blackout. | P3-R7 |
| T7.5 | Arm amputation enforces `OneHanded`-only weapon use (needs T2.12's handedness field). | P3-R8 |

---

### B0-T8 — Zombie AI: native migration + hygiene fixes, PIE-verified · **M (grew from S)** · *depends on T1*

> **History.** Rescoped 2026-07-23 from "design a full PZ-fidelity redesign" down to "hygiene only, repoint two stale references, defer everything else to OQ-B4-12" — see the superseded framing this replaces. Investigating those two stale references turned into a full native-C++ migration of all 6 BT tasks on 2026-07-24/25 (ahead of B0-T2's heavy C++ churn, to remove a corruption-risk surface before the highest-risk phase in the plan), which in turn surfaced two real, previously-invisible bugs and one genuine missing-behavior gap. **2026-07-26: the navmesh blocker is resolved (dev fixed it manually via Epic's official navigation docs) and zombie AI is now confirmed working in PIE** — wander, investigate, and chase all behave correctly. This sub-phase is not "redesign," it's bug-fixing the *existing* design; OQ-B4-12 still owns the actual PZ-fidelity redesign (crowd-following, sandbox tunables, etc.) and has been trimmed accordingly.

| Sub-task | Definition of done | Ref | Status |
|---|---|---|---|
| T8.1 | Native C++ migration of all 6 `BT_Zombie` tasks (`Zombies/AI/BTTask_*`), replacing content-Blueprint wrappers. Includes the two originally-stale reference repoints. | P4-R3 | ✅ Done, verified clean (log scan, `list_nodes`) |
| T8.1a | Bug fix: `BTTask_GetInvestigationPoint` had unset key selectors (evaluated to `"None"`, never worked) and rolled its random point twice independently. Fixed to a single roll, both keys default to `LastKnownLocation`. | `[N]` found during migration | ✅ Done |
| T8.1b | Bug fix: `BTTask_StartInvestigationTimer` returned `InProgress` forever, permanently blocking its `Wander` sequence-sibling. Fixed to return `Succeeded` immediately. | `[N]` found during migration | ✅ Done |
| T8.1c | Added a genuine ambient-wander branch — undecorated, lowest-priority child of the root selector — since `Wander` was previously reachable only from inside the Investigate branch. | P4-R3 | ✅ Done, placed in-editor, verified via `get_children` |
| T8.2 | `BTTask_ClearLastKnownLocation` wiring decision — still open, folded into the trimmed OQ-B4-12 (genuinely ambiguous: may overlap with `StartInvestigationTimer`'s own expiry-driven clear). | P4-R1 (partial) | ⏳ Open, small — not the full redesign this line originally deferred |
| T8.3 | Standing rule: senses are fixed per `UZSZombieConfig` type, **never randomized per-individual.** No PZ-fidelity redesign should quietly reintroduce per-individual randomization. | P4-R2 | ✅ Holds |
| T8.4 | **Noise stress-test scenarios** (PT4). | P4-R4 | ⏳ Blocked with everything else below |
| T8.5 | **PIE behavioral verification of T8.1–T8.1c** — does ambient wander fire, does investigate path correctly, does the timer→wander sequencing feel right. | `[N]` | ✅ **Done 2026-07-26.** Navmesh blocker resolved by the dev (Epic's official navigation docs); zombie moves and behaves correctly in PIE. Full trail in `memory/project_navmesh_dynamic_workaround.md`. |

> **Two different failure classes were found in the original triage — still worth keeping distinct.** `BP_ZombieAIController`'s "missing or NULL parent class" *was* the Live Coding corruption pattern `CLAUDE.md` documents (fixed: reparented to `ZombieAIController`). `BT_Zombie`'s original "class missing" errors were **not** that pattern — stale references between two *content* Blueprints from an unredirected rename (also fixed, then superseded entirely by the native migration). Keep the distinction in mind next time an Output Log error needs triaging.

---

### B0-T9 — Death, loot & world continuity (partial) · **S (2 sessions)** · *depends on T2, T5*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T9.1 | **Loot stays at the death location** — the player's `CarrySlots` instances spawn as `AZSWorldItemActor`s (or a corpse container) on death, preserving `InstanceId` and state. Directly enabled by T2. | P3-R11 |
| T9.2 | **The dead player becomes a zombie** — `AZombieCharacter` gains a death-triggered spawn path (it currently only supports placement/config-driven spawning). ⚑ **Reconfirmed 2026-07-26 (dev-confirmed) specifically for infection deaths**: "it would be cool if the character becomes a zombie, holding on to loot and clothing it had on when it died" — T9.1's death-location loot and this task are the same mechanic already, applied consistently to every death type including the now-2–4-day-windowed bite-infection death (T6.4). No new work beyond T9.1/T9.2 themselves; this just confirms they cover the infection case too. | P3-R11 |
| T9.3 | Respawn-as-new-character flow re-verified against the new loot rules — **solo and co-op alike**, no special-casing needed. | — |
| T9.4 | ⚑ **Simplified 2026-07-26 (dev-confirmed) — no longer deferred, because there's nothing left to defer.** The old plan deferred an asymmetric "solo death ends the world" rule to B3. That rule is gone: death always respawns into the **same persistent world**, solo included. `Server_RespawnAsNewCharacter`'s existing always-same-world behavior already matches this — nothing further to build here. B3 still owns save *topology* (one continuously-overwritten world, corruption-recovery backups), just not a death-triggered world-ending rule. | CR-07 |

---

### B0-T10 — Combat revision · **M (3–4 sessions)** · *depends on T2*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T10.1 | **Jamming.** `UZSWeaponConfig` gains `bJamImmune` (true for revolvers/bolt-actions) + a jam-chance curve scaling off `InstanceState.CurrentDurability` × `ConditionQuality`. Jammed state on `AZSWeapon`; clear-jam action via montage + `bIsBusy`. ⚑ **Named and bound 2026-07-26 per `Docs/InputBindings.md`: "Rack Firearm," `Alt+R`** — confirm whether racking has any use outside clearing a jam (e.g. required after certain reloads) before implementing, since the name implies a manual-chamber action, not just a jam-clear button. | P5-R1 |
| T10.2 | Jam has **legible feedback** — distinct audio cue + a HUD indicator hook for B1. A silent jam is a bug report. | P5-R1 |
| T10.3 | **Melee costs stamina.** Per-weapon `MeleeStaminaCost` on `UZSWeaponConfig` + an `UnarmedStaminaCost` tunable. **No separate strain mechanic** — stamina alone governs swing-spam. | P5-R2 |
| T10.4 | **Downed zombie state.** A real AI state (Blackboard key + BT branch), entered from knockback/damage thresholds — not just the current physical `LaunchCharacter` impulse. **Dev-confirmed KEEP 2026-07-26**, with one added constraint: "find alternatives to make sure this isn't copying PZ" — the state itself is fine, but see T10.6 for the finisher-mechanic requirement. | P5-R3, OQ-B0-03 |
| T10.5 | **`PerformMeleeSwing` excludes downed targets** from a standing swing's arc, unconditionally. | P5-R3 |
| T10.6 | **Stomp/finisher action.** ⚑ **RESOLVED 2026-07-26 (dev-confirmed), binding updated same day per `Docs/InputBindings.md`** — the differentiated take: execution branches on what's equipped (bare-handed → stomp; melee weapon equipped → a downward swing/strike using that weapon instead of a generic stomp animation), but the **input is `Space`**, not contextual on `IA_Attack` as first resolved — bundled as one context-aware action alongside two new, not-yet-designed moves: **Shove** and **Mount/Climb**. That equipped-dependent finisher branch is still what keeps this from reading as a direct PZ port; the binding just moved. Shove and Mount/Climb need their own design/task entries before this can be considered fully scoped. | P5-R3, OQ-B0-03 |
| T10.7 | ✅ **RESOLVED 2026-07-26 (dev-confirmed).** Melee weapon display: grouped poses by weapon *category*, not one universal pose and not one per individual weapon — long-guns (rifle/shotgun/LMG) share a `TP_Mesh` pose, pistols share their own, melee weapons share their own. Author the real melee `UZSWeaponConfig` against this (replacing T1.1's temporary rifle-pose reuse). | P5-R9, OQ-B0-11 |
| T10.8 | ⚑ **NEW, decided 2026-07-26 (dev-confirmed) — ranged weapons move off instant hitscan onto a real simulated projectile system.** New `AZSProjectile` actor (replicated, `UProjectileMovementComponent`, damage/knockback on first blocking hit) plus new `ProjectileClass`/`ProjectileMesh`/`ProjectileSpeed` fields on `UZSWeaponConfig` — unset `ProjectileClass` keeps the old hitscan path, so this is opt-in per weapon, not a forced migration. **Status**: AssaultRifle wired up and PIE-confirmed hitting zombies correctly; Pistol still on hitscan pending rollout; `ProjectileMesh` is currently the engine placeholder Sphere for every weapon — needs an actual bullet mesh per weapon before this is presentable. Still needed: 2-player replication check (does the projectile visibly travel on a non-host client, not just the firing player's own screen), Pistol rollout, cosmetic polish (tracer/impact VFX). **Deferred sub-idea, not yet scoped**: bullet casing ejection (a small physics-simulated casing spawned off a fire anim-notify, matching the existing `AN_ZS_UnlockActions`/`ANS_ZS_BlockADS` notify-driven-VFX pattern) — dev flagged this as worth adding to the plan, pick up once the core projectile system itself is solid. | — |

---

### B0-T11 — SecondaryHand & activatable items · **S (2 sessions)** · *depends on T2*

Scheduled in B0 rather than later because **B4's darkness mechanic is CONFIRMED**, which makes a held light source load-bearing rather than a nice-to-have.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T11.1 | `SecondaryHand` slot on `AZSPlayerCharacter`, honouring `EZSWeaponHandedness` (two-handed primary blocks it) and `bUsableInSecondaryHand`. | P5-R8 |
| T11.2 | **`IA_SecondaryAction`** input + `HandleSecondaryAction()` dispatching on the secondary's config, reusing `PerformMeleeSwing`/`Server_Fire` rather than a parallel implementation. ⚑ **Binding settled 2026-07-26 per `Docs/InputBindings.md`: `T`**, not `F` — `F` is spoken for by Interact. | OQ-B0-10 |
| T11.3 | **Activatable-item concept**: `UZSItemConfig` gains `bIsToggleable` + an on/off cosmetic hook, so a flashlight works without being a `UZSWeaponConfig`. `IA_SecondaryAction` checks this before falling through to attack dispatch. | Planning §6 |
| T11.4 | A working flashlight item — the thing B4's darkness mechanic will be designed against. | X-2 |

---

### B0-T12 — Profiling baseline & stress-test map · **S (2 sessions)** · *depends on T8*

CONFIRMED requirement (Consolidated §12): a **single, reusable** stress-test scenario used for all before/after comparison project-wide. Build it early so every later measurement shares a baseline.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T12.1 | `Lvl_ZS_StressTest` — a fixed graybox map with deterministic zombie spawn counts, scriptable via console (`ZS.SpawnZombies <n>`). | X-5 |
| T12.2 | **Packaged Development build** produced. Never profile Debug or raw PIE — CONFIRMED methodology. | X-4 |
| T12.3 | Triage sequence run and recorded: `stat unit` / `stat fps` (Game/Draw/RHI/GPU-bound), then `stat ai` / `stat anim` / `stat physics`, then `stat gpu` / `stat drawcount`. | X-4 |
| T12.4 | Baseline captured at 25 / 50 / 100 / 150 / 250 zombies. **Zombie count is the primary budget metric** (CONFIRMED). Results committed to `Docs/Testing/PerfBaseline_B0.md`. |
| T12.5 | The baseline informs — but does not yet decide — OQ-B8-01 (budget numbers) and OQ-B7-01 (horde coordination). Recording the numbers is the deliverable; setting the budget is B8's. | X-14 |

---

## Playtest checkpoints

| ID | When | What is specifically being tested | Pass condition |
|---|---|---|---|
| **PT1** | End of B0-T1 | **2-client baseline re-established.** Fire, reload, aim, sprint, crouch, hotbar switch, melee, loot, drop — from *both* clients, with each observing the other. | The last verified 2-client state (P0's exit) is matched or exceeded. Every divergence between what a client sees locally and what the other client sees is logged. |
| **PT2** | B0-T3 | **⚑ CAMERA FEEL/TUNING CHECKPOINT** (reframed 2026-07-26 — no longer gates the perspective-code deletion, which already happened per the dev's confirmed direction). Play 20+ minutes: navigate an interior, fight 3+ zombies at range and in melee, loot a container, read your own character's state at both zoom extremes. | Aim-cone, zoom range, and headshot weighting feel right at the dev's own stated bar (PZ-like feel, DK2 framing). **If this fails, re-tune the numbers** — cone width, zoom presets, headshot split — within top-down. There's no fallback camera to revert to, so failure here means another tuning pass, not a design reopening. |
| **PT3** | End of B0-T4 | **Survival needs.** Run a compressed-clock session through hunger/thirst/fatigue decay into severe tiers. Get wet in the debug rain, get cold, stack them. Sprint while encumbered to exhaustion. | Needs degrade performance without killing outright (the pillar). Wet+cold compounds. Encumbrance never hard-blocks sprint. All 8 needs reach and leave every severity tier. |
| **PT4** | End of B0-T8 | **Noise stress test (CONFIRMED requirement).** Specific scenarios, not general pass/fail: (a) unsuppressed gunshot at a known distance — verify exactly the zombies inside `FireNoiseRadius` respond and those outside do not; (b) melee swing at close range vs. a sleeping/wandering group; (c) sprint-start noise; (d) **wet vs. dry footsteps at the same distance produce measurably different response radii**; (e) two clients firing simultaneously — verify noise events do not double-count or drop; (f) a noise event fired at the edge of a streaming boundary. | Each scenario's actual response radius matches its configured radius within tolerance, **measured**, from both clients. |
| **PT5** | End of B0-T10 | **Full combat loop.** Loot a weapon → hotbar it → equip → fight → weapon degrades → jams → clear the jam → weapon breaks. Melee to exhaustion. Knock a zombie down and finish it — bare-handed (stomp) and with a melee weapon equipped (swing-down) both. | Durability persists across an unequip/re-equip cycle (the refactor's headline fix). Jams are legible. A standing swing never hits a downed zombie. |
| **PT6** | B0 exit | **Full stage sweep A–G, 2-client, plus a 30-minute unscripted co-op session.** | All exit criteria met. `SessionHandoff.md` has zero "built but unverified" items. |

---

## Prototyping vs. stable-systems guidance

| Good early-prototype candidates | Wait for stable systems |
|---|---|
| **T3.5–T3.7 aim cone + elevation interface** — cheap to spike, and the *feel* answer changes the tuning of every weapon. Prototype before authoring any weapon content. | **T10.4 downed-zombie state** — depends on BT structure that T8 is actively changing. |
| **T12.1 stress-test map** — deliberately built before anything is optimized, so it measures the un-optimized case. | **T2.9–T2.11 carry locations, condition variance, ammo** — build on T2.1–T2.8's settled foundation. |
| **T4.3 temperature model** — spike the four-input math in isolation; it is much easier to reason about before it's entangled with weather. | **T11 SecondaryHand** — its real justification (darkness) doesn't exist until B4; build the mechanism, defer the tuning. |

## What B0 explicitly does NOT do

Inventory UI · moodle UI · HUD (all B1) · real multi-level geometry (B4 builds it on graybox; B4X builds it at content scale) · weather visuals · procedural basements (**cut entirely, dev-confirmed 2026-07-26** — fixed authored map instead) · save/load (B3) · horde-coordination redesign (B7, though the ambition behind it is now confirmed high) · **stat-affecting weapon attachments (confirmed wanted, but scoped as its own later weapon-depth pass in Stage 2 — not "maybe never," just "not here")** · zone system (B4) · door-thumping (B4, needs doors) · vehicles (own Stage-2 phase, `BV`) · zombie "freshness" mechanic (scheduled with the zombie AI depth pass, `OQ-B4-12`) · any new weapon or item content beyond the minimum needed to run the verification stages (T4).
