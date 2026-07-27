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
| T2.1 | ✅ **Done, PIE-verified 2026-07-26.** `FZSItemInstance` / `FZSItemInstanceState` / `EZSCarryLocation` defined in new `Source/ZombieShooter/Inventory/ZSItemInstance.h` (replaces the deleted `ZSInventoryTypes.h`/`FZSInventorySlot`). **Deviation from the sketch above**: `FZSItemInstanceState::AttachedInstanceIds`/`EZSAttachmentSlot` omitted — Tier-2 attachments aren't scoped yet (no `EZSAttachmentSlot` enum exists, and stat-affecting attachments are explicitly out of B0), so nothing to attach to yet; add the field when that pass actually starts rather than carrying an unused placeholder now. Replication (`FGuid`/nested structs) not yet verified in 2-client PIE — that's Checkpoint A below. | Planning §5 |
| T2.2 | ✅ **Done, PIE-verified 2026-07-26.** `UZSInventoryComponent::CarrySlots` → `TArray<FZSItemInstance>`. `Server_AddItem` (Config+Count) mints a new GUID per non-stackable unit, merges into partial stacks otherwise — this is the "no pre-existing instance to preserve" entry point (e.g. equip-slot return-to-carry). New `Server_AddItemInstance(FZSItemInstance)` added alongside it for the "an instance already exists and its identity must survive" case (world pickup, container loot transfer, drop re-pickup) — merges by `StackCount` if stackable, keeps the incoming GUID/state intact if not. `Server_RemoveItem` now returns `TArray<FZSItemInstance>` (the actual instances/fragments removed) instead of a bare count, so callers needing identity (`Server_DropItem`) can get it. | P6-R3 |
| T2.3 | ✅ **Done, PIE-verified 2026-07-26.** `AZSWorldItemActor::Item`/`Count` → single `ItemInstance` (`FZSItemInstance`) field. `InitializeItem(Config, Count)` mints a fresh instance (hand-placed/pre-populated-pickup path, unchanged call sites like `BP_ZS_WorldItem_Test`); new `InitializeFromInstance(FZSItemInstance)` preserves an existing one (the drop path). `AZSContainerActor::ContainerSlots` → `TArray<FZSItemInstance>`; `UZSLootTableConfig::RollLoot` now mints instances directly (`Location = World`). `Server_DropItem` spawns one `AZSWorldItemActor` per instance/fragment `Server_RemoveItem` returns, via `InitializeFromInstance` — this is the actual mechanism behind Checkpoint A's GUID-survives-drop test. | P5-R5 |

> **✋ Checkpoint A — PASSED, dev-confirmed 2026-07-26.** Picked up a stackable item (`Canned Food`, merges/stacks correctly) and a non-stackable one (`Pistol (pickup)`) from both a hand-placed world item and a looted container - all instances got distinct, correct GUIDs. Dropped down to individual units via a temporary console command (`ZS.DebugDropFirstItem` - no real drop input exists yet, see the known gap below); the log confirmed the exact designed behavior: a still-carried stack's ID stays stable across partial drops while each dropped *fragment* gets a fresh GUID (fragments have no individual identity), and the final unit of a stack (a true whole-instance removal) **and** the non-stackable Pistol both preserved their original ID end-to-end through drop → world pickup actor → re-pickup into `CarrySlots`. **Scope note**: tested host-only (the debug console command needs authority, which only the host's own PIE window has - see the command's own code comment); general `CarrySlots`/`ContainerSlots` replication to a second client wasn't separately re-verified here, but uses the same `ReplicatedUsing` pattern already confirmed working pre-refactor. Proceeding to Step B.

**Step B — hotbar rewire & equip/unequip (T2.4–T2.8).** The single highest-value step — this is the literal fix for "you can't hotbar a looted weapon."

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.4 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `AZSPlayerCharacter::HotbarSlots` → `TArray<FGuid>`. `StartingHotbarLoadout` stays authorable as a list of configs but now seeds real instances into `CarrySlots` at `BeginPlay` and points the hotbar at their GUIDs. **Real architectural prerequisite found and resolved**: `UZSWeaponConfig` was a separate `UPrimaryDataAsset` hierarchy from `UZSItemConfig`, so `FZSItemInstance::Config` (typed `UZSItemConfig*`) couldn't legally hold a weapon config at all - a carried weapon could never exist as a real inventory item. Fixed: `UZSWeaponConfig` now extends `UZSItemConfig`, per the already-approved Planning doc §6's own table (which required this relationship to exist). Weapons inherit `Weight`/`Rarity`/`WorldMesh`/`DisplayName` for free (previously missing entirely); `bIsEquippable`/`EquipSlot`/`ItemUseType`/consumable fields are inherited but unused by weapons, same as any other not-always-meaningful field elsewhere in this project. **Content gap, not yet done**: every `DA_ZS_WeaponConfig_*` needs a real `Weight` authored (inherited default 0.5 is a placeholder, too light) - can't author Data Asset values without editor access this session. | P5-R4 |
| T2.5 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `UZSInventoryComponent::EquippedBack`/`EquippedHip` → `FGuid`. **Design simplification vs. the original sketch**: equipping (hotbar *and* gear slots alike) no longer physically removes the instance from `CarrySlots` - the slot just holds a GUID reference into an instance that stays resident there the whole time. Simpler than "remove on equip, re-add on unequip," and makes T2.9's "items stay in the bag" requirement fall out almost for free (a bag's contents are never disturbed by (un)equipping it, since the bag instance itself never moves). `GetCurrentWeight()`'s old separate `EquippedBack->Weight`/`EquippedHip->Weight` special-casing was deleted - the main `CarrySlots` loop already counts them now. | Planning §5 |
| T2.6 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Equip flow** resolves GUID → instance, seeds `AZSWeapon::CurrentDurability` via new `AZSWeapon::SeedDurabilityFromInstance()` from `InstanceState.CurrentDurability` (falling back to `Config->MaxDurabilityHits × ConditionQuality` when `-1`). | P5-R5 |
| T2.7 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Unequip flow** (new `AZSPlayerCharacter::WriteBackCurrentWeaponDurability()`) writes `CurrentDurability` back to the instance *before* `CurrentWeapon->Destroy()` - called from both the equip-a-different-weapon and unequip-to-bare-fist paths. | P5-R5 |
| T2.8 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Breaking a weapon** removes the instance from `CarrySlots` entirely (`Server_RemoveInstanceById`) and clears the hotbar GUID — the item is genuinely gone, not orphaned. | Planning §5 |

> **✋ Checkpoint B (the headline acceptance test).** Loot a weapon from a container or the world, put it on the hotbar, equip it, take it to half durability, unequip, re-equip — **durability must still show half**, not reset to full. Break a weapon and confirm it's actually gone from the inventory, not just the hotbar. 2-client PIE. This is the specific bug the whole refactor exists to fix — don't move on until it's genuinely verified, not just "looks right."

**Step C — carry locations & loot condition (T2.9–T2.10).** Separable; can slip a session without blocking anything else.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.9 | 🔧 **Code complete 2026-07-26, not yet PIE-verified. Resolved beyond the original two-option framing** — dev's answer: "if a player drops a bag with items in it, the items stay in the bag." This isn't just an unequip-time rule, it's a data-model requirement: a bag's contents had to become genuinely *nested inside* its own `FZSItemInstance` (new `ContainedItems` field, `ZSItemInstance.h`/`.cpp`, `FZSItemInstance::GetTotalWeight()` rolls contents into weight) rather than living as separate top-level `CarrySlots` entries tagged `Location=Bag`. Once contents are nested, "stays in the bag" holds automatically through equip/unequip/drop/pickup with zero special-case code, since a whole `FZSItemInstance` (bag + contents together) is what already moves through `Server_DropItem`/`AZSWorldItemActor`. New `Server_StoreInBag`/`Server_RetrieveFromBag` on `UZSInventoryComponent` are the C++ mechanism (no UI to drive them yet, same "build the mechanism, defer the UI" pattern as T11). `Bag` capacity (`CarryCapacityBonus`) still only applies while equipped (`GetMaxCarryWeight`); a bag's own weight (contents included) counts whether equipped or not, same as any other carried item - **no hard block on unequipping overloaded**, matches this project's established soft-penalty-only philosophy (`GetEncumbranceMultiplier`) rather than inventing a new hard-cap rule. | CR-09, P6-R1 |
| T2.10 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `ConditionQuality` rolled at spawn inside `UZSLootTableConfig::RollLoot`, banded by rarity tier via new `AZSGameState::RollConditionQuality`/`ConditionQualityBands` (lives on `AZSGameState`, not the loot table, same "shared/game-wide policy" reasoning as `RarityPoolEntries`) — default bands (Common 0.3–0.75, Uncommon 0.45–0.85, Rare 0.65–0.95, VeryRare 0.85–1.0) are code defaults, not dev-specified, retune freely in the Details panel. | P6-R2 |

> **✋ Checkpoint C — not yet run.** Equip a bag, confirm capacity rises. Use the new `Server_StoreInBag`/`Server_RetrieveFromBag` (no input bound - call via a temporary debug hook, same pattern as Checkpoint A's `ZS.DebugDropFirstItem`, or a Blueprint test node) to put an item in a bag, then drop the bag and confirm the dropped `AZSWorldItemActor` still holds it, then pick the bag back up and confirm the item's still inside. Loot the same item twice from a rare-tier table and confirm condition genuinely varies. **Also verify nested-struct replication** (`ContainedItems` inside `CarrySlots`) actually reaches a second client, not just the host - same caveat Checkpoint A's own T2.1 row flagged for the outer struct, now one level deeper.

**Step D — ammo as a real item (T2.11).** Independent of B/C — could be done earlier if it's ever more convenient.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.11 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Ammo becomes a real inventory item.** Removed `AZSWeapon::CurrentReserveAmmo` and `UZSWeaponConfig::StartingReserveAmmo`/`MaxReserveAmmo` entirely; new `UZSWeaponConfig::AmmoItemConfig` names which `UZSItemConfig` a weapon reloads from, `CanReload()`/`PerformReload()` check/draw from the owning player's `UZSInventoryComponent::CarrySlots` directly. Ammo now weighs, loots, drops, and can be shared, same as any other stackable item. **Content gap, not yet done**: no `DA_ZS_ItemConfig_Ammo_*` instances exist and no weapon config has `AmmoItemConfig` assigned yet - every weapon's `CanReload()` returns false until this is authored (can't do it myself without editor access). | P5-R6, OQ-B0-09 |

> **✋ Checkpoint D — not yet run.** Needs `AmmoItemConfig` authored and assigned first (see the content gap above). Then: fire a weapon empty, reload from a carried ammo stack, confirm the stack count drops by the right amount. Drop ammo, have a second player pick it up and reload from it.

**Step E — handedness fields (T2.12).** Cheap, pure data-classification.

| Sub-task | Definition of done | Ref |
|---|---|---|
| T2.12 | ✅ **Done 2026-07-26** (pure data-classification, no PIE test needed beyond a compile - nothing consumes these fields yet). `EZSWeaponHandedness { OneHanded, TwoHanded }` (`ZSWeaponTypes.h`) + `Handedness`/`bUsableInSecondaryHand` added to `UZSWeaponConfig`. Two-handed blocks `SecondaryHand`. Arm amputation restricts to `OneHanded`. Defaults (`TwoHanded`/`false`) match the common case rather than requiring every existing weapon config to be revisited. | P5-R7, P3-R8 |

> **✋ Checkpoint E.** Confirm a two-handed weapon config genuinely blocks `SecondaryHand` (now built, see B0-T11 below) and that the field is readable/settable per weapon.

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
| T4.1 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `Wet` added to `UZSNeedsComponent` as a replicated binary flag (`bIsWet`) + a server-only `WetElapsedGameHours` dry-out timer (`TickWet`, counts up toward `NeedsConfig->WetDryOutGameHours` then auto-clears). `Server_SetWet(bool)` is the debug/entry-point setter — no real weather system exists yet to call it from (B4's job). | P2-R1 |
| T4.2 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** New `AZSPlayerCharacter::TickWetFootstepNoise`, called from `Tick()`: while `NeedsComponent->IsWet()` and the character has ground velocity but isn't sprinting, reports a `UZSNoiseSystem::ReportNoise` event every `WetFootstepNoiseIntervalSeconds` (0.6s default) at `WetFootstepNoiseRadius` (600 default, vs. `SprintNoiseRadius`'s 1200) — approximates a footstep cadence with no real footstep-audio-cue system to key off of. Dry footsteps report nothing at all, so a wet player becomes "audibly distinct" by being the only one that reports anything while walking. Not layered on top of sprint's own one-shot report (avoids a sprinting-while-wet double-report) — a deliberate scope-narrowing call, not an oversight. **PT4's "verify the radius change actually pulls zombies differently" still needs the dev's hands in PIE.** | P2-R1 |
| T4.3 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `Temperature` added to `UZSNeedsComponent` as a single replicated 0-100 scalar (`NeutralTemperature` = comfortable, 0/100 = hypothermic/hyperthermic extremes). `TickTemperature` computes a target from exactly the four confirmed inputs (`NeedsConfig->NeutralTemperature - (bIsWet ? WetTemperaturePenalty : 0) + (bIsIndoors ? IndoorTemperatureBonus : 0) + InsulationSum`, clamped 0-100) and interpolates `Temperature` toward it via `FMath::FInterpConstantTo` at `TemperatureChangeRatePerGameHour` per game-hour. No per-limb thermal, no layering — scope discipline held. | P2-R2, OQ-B0-04 |
| T4.4 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `UZSItemConfig::InsulationValue` added (only meaningful on an equippable item). `TickTemperature` sums `InsulationValue` from whatever's currently equipped in the `Back`/`Hip` gear slots (`UZSInventoryComponent::GetEquippedItem`) into the temperature target — proxy scope, no dedicated clothing-slot system exists yet, so the two general gear slots stand in for a real wardrobe. **Content gap**: no clothing `DA_ZS_ItemConfig_*` instances authored with a non-zero `InsulationValue` yet. | P2-R2 |
| T4.5 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** New `UZSNeedsComponent::GetTemperaturePerformanceMultiplier()` — linear falloff from 1.0 at `HypothermiaThreshold`/`HyperthermiaThreshold` down to `TemperatureExtremePerformanceMultiplier` (0.5 default) at the 0/100 extreme, folded as a fourth multiplicative term into `GetPerformanceMultiplier()` alongside Hunger/Thirst/Fatigue. No separate damage path — temperature only ever debuffs performance, same "performance-debuff-first" model as every other need. Linear, not curve-driven, so it's testable without new content (see the field's own doc comment for why this one differs from the other three). | OQ-B0-04 |
| T4.6 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** New `UZSNeedsComponent::GetPerceptionMultiplier()` — evaluates `NeedsConfig->FatiguePerceptionCurve` at inverted Fatigue (100 - Fatigue, same convention `GetPerformanceMultiplier()`'s own FatigueMult term uses), returns 1 (no degradation) if the curve is unset. Deliberately distinct from `GetPerformanceMultiplier()` per CR-10/OQ-B0-05's resolution — this is a presentation-only stub (vignette/audio) for a future camera/audio pass (B1) to consume, never touches gameplay math itself. **Content gap**: `FatiguePerceptionCurve` not authored, and no B1 visual/audio consumer exists yet — both expected, out of scope for B0. | P2-R3 |
| T4.7 | ✅ **Confirmed already true, 2026-07-26 — no code change needed.** `GetPerformanceMultiplier()` multiplies four independently-[0,1]-clamped terms (`EvaluatePerformanceCurve` clamps its return, and `GetTemperaturePerformanceMultiplier()` is itself an `FMath::Lerp` between 1.0 and a sub-1.0 floor) — the product can never exceed 1.0 by construction, regardless of how sated/comfortable the four inputs are. Verified by reading, not assuming. | P2-R4 |
| T4.8 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `TickStamina`'s sprint-drain branch now scales `StaminaDrainPerSecondSprinting` by `1 / GetEncumbranceMultiplier()` (clamped to `[1, MaxEncumbranceStaminaDrainMultiplier]`, code default ceiling 2×) — heavier load drains stamina faster. Confirmed `StartSprint_Implementation`/`Server_StartSprint_Implementation` were already gated on `Stamina > 0` only, no encumbrance check to remove. | P2-R5 |
| T4.9 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** Severity-tier getters added for all four `UZSNeedsComponent`-owned needs on the shared 4-tier scale (`GetHungerSeverityTier`/`GetThirstSeverityTier`/`GetFatigueSeverityTier` reuse `NeedsConfig->GetSeverityTier` directly or inverted; `GetStaminaSeverityTier` added this pass for the same "higher = better" shape; `GetTemperatureSeverityTier` transforms Temperature into a "comfort value" — 100 at `NeutralTemperature`, falling to 0 at either extreme — before feeding the same thresholds, since temperature is bad in *both* directions unlike the other three). **Scope interpretation**: of the doc's "8 needs," `Wet` is binary and doesn't have a 4-tier shape to author (documented here, not silently skipped); Injury/Pain and Infection/Sickness severity already exist as their own concepts on `UZSHealthComponent` (per-zone wound flags, `EZSInfectionStage`) rather than this shared 0-100/4-tier scale — flagged as a decision for the dev to confirm, not assumed correct. | P2-R8 |
| T4.10 | 🔧 **Partially code-complete 2026-07-26, not yet PIE-verified — honest gap, not faked.** `IsSafeToSleep()` now checks condition (1), the aggro-cooldown: new `Server_NotifyHostileDetection()` (called from `AZombieAIController::HandleTargetPerceptionUpdated` on every successful sense) stamps `LastHostileDetectionTime`, and `IsSafeToSleep()` fails while `GetWorld()->GetTimeSeconds() - LastHostileDetectionTime < HostileDetectionCooldownSeconds`. Condition (2), "real shelter" (barricaded room/locked door/vehicle), is **stubbed `true`** — no barricade/indoor-detection system exists in B0 to check against. This is a genuine content/system gap, not a design walk-back: the gate is real and testable for condition (1) today, and condition (2) becomes a one-line swap once B4's indoor/shelter system exists. | P2-R7, OQ-B0-06 |

---

### B0-T5 — Wound model revision · **S (2–3 sessions)** · *depends on T1*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T5.1 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Fix the blank `FHitResult` bite bug.** `AZombieCharacter::Server_MeleeAttack` now traces (`ECC_Visibility` from zombie to target, falling back to a direct `LineTraceComponent` against the target's skeletal mesh if the first trace found no bone - a capsule hit has no `BoneName`, so this covers whichever collision setup the target has) instead of passing a blank `FHitResult()`. **Amputation's Arms/Legs infection-clearing path becomes reachable from a real bite for the first time.** | P3-R6 |
| T5.2 | ✅ **Confirmed already true, 2026-07-26 - no code change needed.** There was never a bandage-decay mechanism in the codebase at all (`Server_ApplyBandage` just clears `bBleeding` with no timer to re-trigger it) - grepped to confirm before assuming. A bandage already stays effective until the wound heals; `bClean` (renamed from the doc's `bDirty` framing - same flag, existing name) still drives infection risk. "Higher-tier heal item supersedes" isn't applicable yet - only one bandage tier (clean/dirty) exists. | P3-R3, CR-05 |
| T5.3 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Critical head bleed** — new `FZSBodyZoneWound::bCriticalBleed`, rolled in `Server_ApplyDamage` on a fresh bleeding Head-zone hit (`UZSHealthConfig::CriticalHeadBleedChance`, code default 8%), overrides the normal wound-type bleed rate with a steep `BleedDamagePerSecond_CriticalHead` (code default 4/s) in `TickBleed`. Cleared by any bandage. "Distinct and urgent feedback" is the same `OnDamageImpact` stub as T5.5 for now (no dedicated UI hook - B1's job). | P3-R4 |
| T5.4 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Multi-day fracture recovery** on the `AZSGameState` game-hour clock (new `UZSHealthComponent::TickFractureRecovery`, same conversion `TickInfection` uses) — new `FZSBodyZoneWound::FractureRecoveryProgressGameHours` + `UZSHealthConfig::FractureRecoveryDurationGameHours`/`SplintedFractureRecoveryDurationGameHours` (code defaults: 240h unsplinted, 96h splinted - splint shortens by 60%, doesn't trivialize). A fresh fracture-causing hit resets progress, same as it already reset `bSplinted`. | P3-R5 |
| T5.5 | ✅ **Done 2026-07-26.** Removed the temporary `Server_ApplyDamage` hit-confirmation `UE_LOG`/on-screen message; replaced with `UZSHealthComponent::OnDamageImpact(Zone, WoundType, DamageAmount)`, a `BlueprintImplementableEvent` stub with no default implementation - bind cosmetic VFX/SFX to it in a Blueprint subclass whenever B7's real pass happens. `Server_RollForInfection`'s own temporary logging left alone - that one's T6's concern (infection legibility), not T5's. | P5-R11 |

---

### B0-T6 — Two-tier infection · **S (2–3 sessions)** · *depends on T5*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T6.1 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **`EZSWoundInfectionState`** (`None`/`Infected`) per zone, distinct from `EZSInfectionStage`. New `UZSHealthComponent::TickWoundInfection` progresses `FZSBodyZoneWound::WoundInfectionProgressGameHours` on the game-hour clock while a wound is dirty and untreated, escalating to `Infected` past `WoundInfectionOnsetGameHours` (code default 24h). Slows fracture recovery (`WoundInfectionFractureRecoverySlowMultiplier`, 0.5x) and worsens bleed (`WoundInfectionBleedMultiplier`, 1.3x, stacks with the existing dirty multiplier) - **never touches `CurrentHealth` directly, never fatal alone.** | P3-R1 |
| T6.2 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** `Server_Disinfect` and a clean `Server_ApplyBandage` both cure wound infection immediately (reset `WoundInfectionState`/progress); **explicitly does nothing to bite infection** (`InfectionStage` untouched by either). | P3-R1 |
| T6.3 | ✅ **Done 2026-07-26 at the code/doc level - the actual HUD display is still B1's job, not built here.** Updated `EZSInfectionStage`'s own header comment (was "deliberately UI-ambiguous... should not display this enum name verbatim") to record the reversed requirement directly at the type definition, so a future B1 pass reads the correct constraint at the source rather than a doc buried elsewhere. Both `InfectionStage` and the new `WoundInfectionState` already broadcast via existing `OnRep_`/delegate machinery - nothing further needed on the data side for B1 to build against. | P3-R2, CR-06 |
| T6.4 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Bite-infection fatal timeline** — dev-confirmed range (2-4 in-game days) now lives as `MinBiteInfectionDurationGameHours`/`MaxBiteInfectionDurationGameHours` (48/96) on `UZSHealthConfig`; `Server_RollForInfection` rolls a random total within that range per-infection and scales the 4 existing per-stage durations (now base proportional weights, retuned to sum to 72h/3 days as a readable midpoint) to fit it, preserving relative stage pacing while the actual total varies run-to-run. Rolled values live in a new server-only `RolledInfectionStageDurationsGameHours[4]` on the component, not the config (config only holds the base weights + range). | P3-R12, OQ-B0-08 |
| T6.5 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** **Medical-tier incubation delay** — new `UZSItemConfig::MedicalIncubationDelayGameHours` (Bandage/Disinfectant only, default 0 = no effect) + new `UZSHealthComponent::Server_DelayInfection(Zone, GameHours)` (no-op unless `Zone` is the actual bite-infection source), wired into `Server_UseItem_Implementation`'s Bandage/Disinfectant dispatch. **Content gap**: no bandage/disinfectant `DA_ZS_ItemConfig_*` instances have a non-zero value authored yet (a "better" medical tier is what would set one) - can't author Data Asset values without editor access. | P3-R9 |

---

### B0-T7 — Amputation choreography & blackout · **S (2–3 sessions)** · *depends on T6*

| Sub-task | Definition of done | Ref |
|---|---|---|
| T7.1 | 🔧 **Code complete 2026-07-26, not yet PIE-verified.** Amputation gets a **montage + `bIsBusy` gate** (`Server_AmputateZone`→`CompleteAmputation`, real timer via `AmputationDurationSeconds`, not montage-notify-driven since no montage is authored yet - content gap). | P3-R10 |
| T7.2 | 🔧 **Code complete.** **Blackout state** (`bIsBlackedOut`) — incapacitated, vulnerable after a successful amputation. Not death (collision/damageability stay on - a blacked-out player is still a valid target), not normal play (movement disabled via `DisableMovement`, `CanAttack`/`CanFire`/`CanSwitchLoadout` all gated off). | P3-R7 |
| T7.3 | 🔧 **Code complete.** **Solo:** `EnterBlackout` jumps the world clock forward `BlackoutTimeSkipGameHours` (12) via the existing `AZSGameState::Server_AdvanceTimeByGameHours` (a single lump-sum jump, not a sustained clock-rate multiplier - the world clock is shared across every connected player). Player stays vulnerable/immobile in real time for `BlackoutDurationSeconds` (60) before auto-recovering. | P3-R7 |
| T7.4 | 🔧 **Code complete, partial scope.** **Co-op:** new `ReviveInteractable` component (only interactable while blacked out) lets a teammate revive early, ending the blackout immediately. `UpdateNearestInteractable`'s overlap scan widened to also query `ECC_Pawn` (was WorldStatic/WorldDynamic only - no interactable had ever lived on a Pawn before) plus a self-exclusion check. **"Move the downed body" not built** - a real drag/carry system is bigger scope than this pass; revive-shortens-blackout is the concrete mechanic delivered. | P3-R7 |
| T7.5 | 🔧 **Code complete.** Arm amputation blocks equipping a `TwoHanded` weapon (`Server_SelectHotbarSlot_Implementation` checks `HealthComponent->GetZoneWound(Arms).bAmputated`). | P3-R8 |

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
| T10.8 | ⚑ **NEW, decided 2026-07-26 (dev-confirmed) — ranged weapons move off instant hitscan onto a real simulated projectile system.** New `AZSProjectile` actor (replicated, `UProjectileMovementComponent`, damage/knockback on first blocking hit) plus new `ProjectileClass`/`ProjectileMesh`/`ProjectileSpeed` fields on `UZSWeaponConfig` — unset `ProjectileClass` keeps the old hitscan path, so this is opt-in per weapon, not a forced migration. **Status**: AssaultRifle wired up, PIE-confirmed hitting zombies correctly, and 2-player replication check passed (projectile travel is visible on both the firing client and the host). Pistol rollout in progress. `ProjectileMesh` is currently the engine placeholder Sphere for every weapon — needs an actual bullet mesh per weapon before this is presentable. Still needed: cosmetic polish (tracer/impact VFX). **Deferred sub-idea, not yet scoped**: bullet casing ejection (a small physics-simulated casing spawned off a fire anim-notify, matching the existing `AN_ZS_UnlockActions`/`ANS_ZS_BlockADS` notify-driven-VFX pattern) — dev flagged this as worth adding to the plan, pick up once the core projectile system itself is solid. | — |
| T10.9 | 🛑 **NEW BUG, found 2026-07-26 during T10.8's 2-player replication check — Player 2's (non-host client's) aim/facing rotation never reaches the server.** `UpdateCursorFacing` (`Source/ZombieShooter/Player/ZSPlayerCharacter.cpp`) sets rotation via a direct `SetActorRotation()` call on whichever machine has the pawn locally controlled - fine for the host (that machine *is* the server), but for a real remote client this is purely local/cosmetic: `CharacterMovementComponent`'s normal replication only carries rotation the server independently re-derives from replicated *input* (`bOrientRotationToMovement`), not an out-of-band `SetActorRotation` call. Dev-confirmed symptom: "when as player 2... shooting in a different direction so that the character aim has to rotate, that is not replicated to server side." **This affects every cursor-facing-gated action for non-host clients, not just ranged fire** - the same gate (`IsCursorFacingActive()`) covers aiming, attacking, *and* interacting, per its own doc comment - so melee and interact are suspected to have the identical bug, just never surfaced before this was the first real (non-standalone, non-host) multiplayer test of a cursor-facing action this session. **Fix implemented 2026-07-26, not yet compiled/PIE-tested**: `Server_UpdateCursorFacingRotation(FRotator)` (`ZSPlayerCharacter.h`/`.cpp`) - `Unreliable` server RPC, called from `UpdateCursorFacing` right after the existing local `SetActorRotation`, applies the same rotation authoritatively on the server's copy of the pawn. Needs a full rebuild (header change) and a 2-player re-test of the exact repro (P2 shooting in a direction that requires rotating) before this can be marked resolved. | — |

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
