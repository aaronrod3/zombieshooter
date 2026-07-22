# Inventory, Loadout & Equipping — Unified Design Plan

> **Status: DRAFT PROPOSAL, not plan of record.** Written 2026-07-22, unsupervised (dev away), at the dev's request ("thoroughly plan the inventory, loadout, and player equipping items, tie everything together, setup modularity for weapons"). This is deliberately kept **out of `GameDevPlan.md`/`Docs/Phases/`** so it doesn't get mistaken for settled decisions — everything here is a recommendation pending your review, not something that's been built or agreed to. Where I made a judgment call instead of just listing options, I've said so and marked it `RECOMMENDATION` — override freely. See the **Open Questions** section (§9) for the compressed list of everything that actually needs your yes/no.
>
> Grounded in: the actual C++ built as of commit `65f53e4` (P5's hotbar/melee/durability, P6's inventory/loot), `Docs/GameDevPlan.md`, `Docs/DevMarkupNotes.md` §10.2 and §21 (your own annotations — quoted directly below, they matter more than anything I infer), and `Docs/ProjectZomboid_DesignReference.md` §7/§12/§21.

## 1. Why this doc exists

P5 (loadout/combat) and P6 (inventory/loot) were built in the same overnight session but **as two separate systems that happen to sit next to each other**, not one integrated system. That was a deliberate scope call at the time (P6's own phase file flags it: *"P5's `HotbarSlots` still holds direct `UZSWeaponConfig*` references, not yet backed by items actually held in this component"*). This doc is the follow-up: figure out what "actually tied together" looks like, and what it takes to get there.

Three things came out of re-reading the source docs closely for this pass that I think matter more than they've been given credit for so far:

1. **`Docs/DevMarkupNotes.md` §10.2 (your own note, verbatim)**: *"Simple container-based loot; items take up a single spot. Items like weapons can only be equipped, so items need a category to place them into appropriate spots. Some items can only be carried (not equipped), for instance."* — this is the actual original spec for the category/equip-slot system. P6 built the category concept (`bIsEquippable`/`EquipSlot`) but only applied it to bags/clothing — **weapons never got folded into it**, they still equip through a completely separate parallel mechanism (P5's hotbar). That's the central gap this doc addresses.
2. **§21 (your own note)**: *"Simplify inventory; have an easy stat system to show what items and actions do to the player. Player knowledge is mainly built on common sense and playing the game."* — this is a constraint on everything below, not just the UI doc. Whatever gets built here needs to stay legible without a wiki, the way `ProjectZomboid_DesignReference.md` §12 explicitly warns PZ's own inventory *didn't* ("the most-modded, most-complained-about surface in the game... treat as a must-beat benchmark, not a template").
3. **Ammo isn't actually in the inventory system at all right now.** This wasn't on anyone's radar as a gap until I traced the data flow for this doc — see §4. It's a real hole, not a nice-to-have.

## 2. Current state, audited line-by-line (as of `65f53e4`)

### The three separate ways an "item" exists in the codebase today

| Representation | Where | What it tracks | What it *can't* track |
|---|---|---|---|
| `UZSItemConfig*` | A `UPrimaryDataAsset` — shared, immutable, one instance per item *type* (e.g. one `DA_ZS_ItemConfig_Bandage` asset shared by every bandage anyone ever carries) | Static per-type data: `Weight`, `MaxStackSize`, `bIsEquippable`/`EquipSlot`/`CarryCapacityBonus`, `Rarity`, `WorldMesh`, `ItemUseType` | Anything specific to *one particular* carried item — there is no such thing as "this specific bandage" today, only "a bandage" |
| `AZSWeapon` | A live, replicated actor, spawned fresh by `EquipWeapon(Config)` every time a weapon is equipped, destroyed every time it's unequipped | Live, mutable per-instance state: `CurrentDurability`, `CurrentMagazineAmmo`, `CurrentReserveAmmo`, `CurrentFireMode` | Nothing persists across an equip/unequip cycle — see §3, this is the actual bug |
| `FZSInventorySlot` | An entry in `UZSInventoryComponent::CarrySlots` | Just `{ UZSItemConfig* Item; int32 Count; }` — "you have N of these" | Any per-instance state at all. A stack of 3 identical bandages is fine (they really are interchangeable); a "stack" of 1 rifle silently pretending to be interchangeable with every other rifle of the same config is not fine, and is exactly what's happening today |

**None of these three talk to each other.** `HotbarSlots` (P5) holds raw `UZSWeaponConfig*` references that were seeded once from `StartingHotbarLoadout` at `BeginPlay` and never touch `CarrySlots` (P6) at all. Picking up a weapon via `AZSWorldItemActor` adds it to `CarrySlots` like any other loot — there's currently no path from "I picked up a rifle" to "that rifle is now selectable on my hotbar." You'd have to have authored it into `StartingHotbarLoadout` at design time for it to ever be equippable, which defeats the entire point of a loot-driven survival game.

### What already works and shouldn't be disturbed
- The weight/encumbrance math (`GetCurrentWeight`/`GetMaxCarryWeight`/`GetEncumbranceMultiplier`) is self-contained and doesn't care what representation items use underneath — safe to build on top of.
- The `EZSAttackType`/`HandleAttack` dispatch (Ranged → `HandleFireStarted`, Melee → `Server_WeaponMeleeAttack`, none → `Server_MeleeAttack`) is sound and shouldn't need to change shape — it just needs its inputs (which weapon, what state) to come from a richer source.
- The replication convention (`Server_X` + `OnRep_X` + delegate broadcast) is applied consistently everywhere already — everything proposed below follows the same pattern, nothing new to learn.

## 3. The core problem, stated precisely

**A carried item that needs per-instance state (a weapon's durability/ammo, eventually an attachment) has nowhere to keep that state while it's sitting in the inventory, and loses it entirely when unequipped.**

Concretely, today:
- Break a weapon mid-fight → `Server_WeaponMeleeAttack` nulls it out of `HotbarSlots` and destroys the `AZSWeapon` actor. The `UZSWeaponConfig*` reference is just gone from the hotbar; nothing in `CarrySlots` ever knew the weapon existed as a "thing you have," so there's nothing to remove from there either, and there's no broken-item husk left behind to represent "you're still carrying a ruined rifle."
- Swap from Rifle A (half durability, half a magazine loaded) to Rifle B and back → Rifle A respawns as a *brand new* `AZSWeapon` from the same `UZSWeaponConfig`, at full durability, full starting reserve ammo. Nothing was preserved. This was flagged as a known limitation when durability shipped last round; it's real and worth fixing now rather than letting more systems get built on top of the gap.
- Pick up two different rifles of the same type → they're currently indistinguishable in `CarrySlots` (same `Item` pointer, `Count` just increments) even though in a real game one might be pristine and one might be nearly broken. `MaxStackSize` on weapon configs is already `1` by convention (never explicitly enforced, but every weapon example so far implies it), so this specific case probably wouldn't actually collide in practice — but it's a sign the data model has no way to *express* "these two are different" even when it matters.

## 4. Ammo is not integrated with the inventory — a gap nobody flagged until now

Tracing `AZSWeapon::PerformReload`: it transfers `CurrentReserveAmmo → CurrentMagazineAmmo`, both plain `int32`s living on the weapon actor, seeded once from `UZSWeaponConfig::StartingReserveAmmo` and capped at `MaxReserveAmmo`. **There is no mechanism anywhere for ammo to be picked up, looted, dropped, or carried as a real item.** "Reserve ammo" today is a fake resource that resets to `StartingReserveAmmo` every time you re-equip the weapon (see §3) and can never be replenished by looting — it's a fixed props value, not a game resource.

This blocks two things already on the books:
- P5's own flagged-unbuilt "ammo scarcity tuning" — you can't tune scarcity of a resource that isn't actually finite or lootable.
- The whole "run out, loot under threat, haul back" loop `Docs/Phases/P6_InventoryLoot.md`'s exit criteria describes — that loop needs ammo to be real inventory.

**`RECOMMENDATION`**: ammo becomes a normal, stackable `UZSItemConfig` item (e.g. `DA_ZS_ItemConfig_Ammo_9mm`, `MaxStackSize` in the hundreds), and `UZSWeaponConfig` gains an `AmmoItemConfig` reference saying which ammo item this weapon reloads from. `PerformReload` changes from "decrement an internal counter" to "ask the owning player's `UZSInventoryComponent` to remove up to `AmmoNeeded` units of `AmmoItemConfig` from `CarrySlots`, add whatever was actually available to `CurrentMagazineAmmo`." `CurrentReserveAmmo` and `MaxReserveAmmo` go away as concepts entirely — the inventory's stack count for that ammo type *is* the reserve, full stop, no duplicate bookkeeping. This is a clean simplification, not just a feature add.

This also means ammo scarcity becomes "just loot table tuning" for free, the same way any other item's rarity works — no special-case code needed.

## 5. Proposed unified data model: the Item Instance

This is the actual "tie everything together" piece. Recommend introducing a real per-instance record, and routing every system that currently touches a bare `UZSItemConfig*` through it instead.

```cpp
USTRUCT(BlueprintType)
struct FZSItemInstance
{
    GENERATED_BODY()

    // Stable identity - survives being moved between CarrySlots, an equip slot, a hotbar slot,
    // a container, or the world. Generated once (FGuid::NewGuid()) when an instance is first
    // created (looted, crafted, purchased - whatever future acquisition paths exist) and never
    // changes again, even as the instance moves between every other kind of slot in the game.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGuid InstanceId;

    // The shared, immutable "template" - unchanged from today's UZSItemConfig.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TObjectPtr<UZSItemConfig> Config = nullptr;

    // Only meaningful when Config->MaxStackSize > 1 (ammo, food, meds, materials). Weapons/bags/
    // tools (MaxStackSize == 1 by convention) always have StackCount == 1 - the "stack" concept
    // and the "per-instance state" concept are mutually exclusive by construction, which is
    // exactly right: two things that can meaningfully differ from each other were never
    // supposed to be interchangeable in a stack anyway.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 StackCount = 1;

    // Only meaningful when StackCount == 1. Everything a specific instance needs remembered
    // across equip/unequip/drop/pickup cycles.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FZSItemInstanceState InstanceState;
};

USTRUCT(BlueprintType)
struct FZSItemInstanceState
{
    GENERATED_BODY()

    // -1 = "not yet initialized, use Config's fresh-spawn default" (MaxDurabilityHits, etc.) -
    // this sentinel exists so a stackable item's instance (which never touches this struct
    // meaningfully) doesn't need special-casing, and so a freshly-looted weapon's first-ever
    // read initializes correctly without a separate "seed on creation" pass.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 CurrentDurability = -1;

    // Tier 2 (see §7) - which attachment instance (by its own InstanceId, living in the SAME
    // CarrySlots pool logically "moved into" this weapon) occupies each of this weapon's
    // attachment slots. Empty/unused unless attachments are actually built.
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TMap<EZSAttachmentSlot, FGuid> AttachedInstanceIds;
};
```

### What changes to plug this in

- **`UZSInventoryComponent::CarrySlots`**: `TArray<FZSInventorySlot>` → `TArray<FZSItemInstance>`. `Server_AddItem`/`Server_RemoveItem` keep the same stacking behavior for `MaxStackSize > 1` items; for `MaxStackSize == 1` items, `Server_AddItem` always creates a new `FZSItemInstance` with a fresh `InstanceId` (never merges) — this is the "picking up a genuinely new/found item" path, distinct from "restoring an item that already had a GUID" (dropping and re-picking-up your *own* weapon should preserve its existing `InstanceId`/`InstanceState`, not mint a new one — `Server_DropItem`/`AZSWorldItemActor` need to carry the instance through, not just `Item + Count`).
- **`AZSPlayerCharacter::HotbarSlots`**: `TArray<TObjectPtr<UZSWeaponConfig>>` → `TArray<FGuid>` (instance IDs; `FGuid()` = empty slot). `StartingHotbarLoadout` (currently `TArray<UZSWeaponConfig*>`, authored directly) stays as a *seed* concept but now needs to also seed matching `FZSItemInstance` entries into `CarrySlots` at `BeginPlay`, then point `HotbarSlots` at their new GUIDs — slightly more setup at spawn time, no change to the authoring experience (still just a list of weapon configs on the BP).
- **`UZSInventoryComponent::EquippedBack`/`EquippedHip`**: `TObjectPtr<UZSItemConfig>` → `FGuid`, same reasoning.
- **Equip flow** (`Server_SelectHotbarSlot` → `CompleteHotbarSwitch`): resolve the GUID to an `FZSItemInstance` in `CarrySlots`, read `Config` (as today) **and** `InstanceState.CurrentDurability`. `EquipWeapon` needs a new parameter (or overload) to seed `AZSWeapon::CurrentDurability` from that saved value instead of always resetting to `Config->MaxDurabilityHits`.
- **Unequip flow**: before `CurrentWeapon->Destroy()`, write `CurrentWeapon->GetCurrentDurability()` back into the matching `FZSItemInstance::InstanceState.CurrentDurability` in `CarrySlots`. This is the actual fix for "durability resets on re-equip."
- **Breaking a weapon** (`Server_WeaponMeleeAttack`, durability hits 0): instead of nulling the `HotbarSlots` entry's config reference (today's stopgap), remove the `FZSItemInstance` from `CarrySlots` entirely via its `InstanceId` (the item is genuinely destroyed/consumed) and clear the `HotbarSlots` GUID. Net effect is the same as today (weapon's gone) but now it's *actually* gone from the inventory too, not just orphaned from the hotbar while still technically "owned."

This is a real refactor of already-shipped-tonight code, not a greenfield add. It touches `ZSInventoryComponent.h/.cpp`, `ZSPlayerCharacter.h/.cpp` (hotbar section), `ZSWeapon.h/.cpp`, `ZSWorldItemActor.h/.cpp`, and `ZSContainerActor.h/.cpp`. Scope it as its own pass, not a drive-by change alongside something else.

## 6. Player equipping items — one verb, two slot families

Today there are two `Server_Equip*`-shaped code paths that don't know about each other:

1. **Combat loadout slots** (P5): `PrimaryHand` (= `CurrentWeapon`), future `SecondaryHand`. Equipping takes real time (`EquipTimeSeconds`, `bIsBusy`-gated), and what's equipped here drives `IA_Attack`'s dispatch. Holds weapon/melee configs (anything with an `AttackType`).
2. **Gear slots** (P6): `Back`, `Hip`. Equipping is instant today, grants passive `CarryCapacityBonus`, doesn't affect combat dispatch at all. Holds `bIsEquippable` configs (bags/clothing).

**`RECOMMENDATION`**: keep these as two genuinely different *families* with different rules (that's a real gameplay distinction, not an accident) but make them both operate on the same underlying `FZSItemInstance` pool from §5, so "equip" always means the same thing structurally: *move an instance's reference from `CarrySlots` into a named slot elsewhere on the character, governed by that slot family's own rules.* Concretely:

| | Combat loadout slots | Gear slots |
|---|---|---|
| Owned by | P5 | P6 |
| Members | `PrimaryHand` (hotbar), future `SecondaryHand` | `Back`, `Hip`, future clothing |
| Equip time | Real (`EquipTimeSeconds`/`UnequipTimeSeconds`, `bIsBusy`) | **Open question — see §9.** PZ itself does have real equip time for some clothing swaps; today's P6 code made bags instant with no stated reasoning, just because that's simplest. Worth an actual decision rather than an accident. |
| Drives `IA_Attack`? | Yes (`PrimaryHand` only, per the already-resolved P5 design pass) | No |
| Legal item types | `AttackType`-bearing `UZSWeaponConfig` | `bIsEquippable` `UZSItemConfig` |

Both read/write the same `CarrySlots` pool. This is the actual "tied together" — not a merger into one slot type (they have genuinely different rules and shouldn't be forced into identical code paths), but a shared substrate underneath.

### Resolving `SecondaryHand` (currently a standing open question, twice deferred)

`GameDevPlan.md` §7 P5 resolved *that* `SecondaryHand` should be independently usable, but explicitly left *how its own action triggers* unanswered, twice, because nobody could confirm an input binding without you present. Since this doc is meant to actually tie the system together, here's a concrete proposal rather than deferring a third time:

**`RECOMMENDATION`**: a new `IA_SecondaryAction` input (suggest a dedicated key rather than overloading an existing one — `LMB`/`RMB` are already `IA_Attack`/`IA_Aim`; a middle-mouse-button or a free letter key both work mechanically). `AZSPlayerCharacter::HandleSecondaryAction()` dispatches on `SecondaryHandWeapon`'s config the same way `HandleAttack` dispatches on `PrimaryHand`'s — reusing `PerformMeleeSwing`/`Server_Fire`-shaped logic, not a parallel implementation. This only cleanly covers offhand items that *are* `UZSWeaponConfig`s (an offhand pistol). A flashlight or similar toggle-item doesn't fit a `UZSWeaponConfig`/attack-dispatch model at all — that's a separate, smaller "activatable item" concept (`UZSItemConfig` gains an optional `bIsToggleable` flag and an on/off cosmetic hook) that `IA_SecondaryAction` would also need to check for before falling back to weapon-attack dispatch. Both pieces are new, neither is large, but this is genuinely new surface, not just wiring — scope it as its own small task, not a rider on the item-instance refactor.

## 7. Weapon modularity

This is the part of the ask I want to be most careful not to over-scope. Two very different things could be meant by "modularity for weapons," and I think both are worth doing, at different depths:

### Tier 0 — already true, worth stating explicitly
"New weapon = new `UZSWeaponConfig` data asset instance, zero C++ branches" (the existing multi-weapon rule, `CLAUDE.md`) already covers ranged and melee (as of tonight). This tier doesn't need any new work — it needs to be *kept* true as everything else below gets added, i.e. nothing in §5/§6/§8 should introduce a C++ branch keyed on a specific weapon.

### Tier 1 — `RECOMMENDATION`: build this, it's cheap and closes a real gap
Two fields are missing from `UZSWeaponConfig` that are prerequisites for rules already written down elsewhere in the docs:
- `EZSWeaponHandedness { OneHanded, TwoHanded }` — a two-handed weapon should occupy (block) `SecondaryHand`. This is the literal prerequisite `GameDevPlan.md` §4 P5 already names for the amputation backlog item ("arm amputation restricts weapon use to one-handed options only") — that rule is unenforceable without this field existing.
- `bool bUsableInSecondaryHand` — whether this config is legal to place in `SecondaryHand` at all (a rifle: no, even before considering handedness; a pistol: yes).

This is pure data-classification work, no attachment system, no stat-modifier math — just the fields needed to make slot-legality rules actually enforceable. Low risk, directly unblocks two already-written-down design intentions.

### Tier 2 — attachments — **flagging as optional, recommend against building yet**
This is probably what "modularity" evokes most readily (scopes, silencers, magazines, the Tarkov/Escape-from-Tarkov mental model), so I want to address it directly rather than skip it, but the research grounding doesn't support going deep here:

- PZ's own attachments (the game this project is deliberately targeting ~1/3 the depth of) are shallow — scopes and slings, minor/cosmetic, not a stat-modifier tree (`ProjectZomboid_DesignReference.md` §7.2).
- Nothing in `GameDevPlan.md`, `DevMarkupNotes.md`, or any dev note anywhere asks for weapon customization depth. It's not a stated pillar.
- A full modular system (interchangeable barrels/stocks/magazines that change effective stats, à la Tarkov) is a meaningfully large scope addition for a solo-dev project already targeting a smaller footprint than its own reference game.

**If you do want attachments**, here's the shallow version that would fit the existing architecture without a parallel class hierarchy:
- `UZSWeaponConfig` gains `TArray<EZSAttachmentSlot>` (`Optic`, `Muzzle`, `Underbarrel` — only the slots that exist on *that* weapon; melee weapons have an empty array, i.e. no attachment support at all, which is correct and requires zero extra code).
- `UZSItemConfig` gains a few optional fields (`bool bIsAttachment`, `EZSAttachmentSlot AttachmentSlot`, flat modifiers like `FireRangeBonus`/`NoiseRadiusMultiplier`) rather than inventing a new `UZSAttachmentConfig` class — keeps it inside the existing item-instance model from §5 (an attachment is just another `FZSItemInstance`, "attached" means its GUID lives in the weapon's `InstanceState.AttachedInstanceIds` map instead of a `CarrySlots` entry).
- Effective stats = base config values + sum of attached modifiers, computed once when attachments change (on equip, or on attach/detach) into a cached struct on `AZSWeapon`, not recomputed per-shot.
- Explicitly **not recommended**: anything that changes a weapon's fundamental archetype (barrel swaps that turn a carbine into a different weapon class, etc.) — Tier 3, out of scope, not researched further here because nothing points at wanting it.

**This whole tier needs your go/no-go before any of it gets built** — see §9.

## 8. Migration order (if this plan is approved)

Ordered so each step is independently shippable and testable, and nothing later depends on something not yet decided:

1. **`FZSItemInstance`/`FZSItemInstanceState` + GUID plumbing** (§5) — the foundation everything else sits on. Touches `ZSInventoryComponent`, `ZSWorldItemActor`, `ZSContainerActor`.
2. **Rewire `HotbarSlots` to reference instances by GUID** (§5/§6) — makes "loot a weapon, then equip it from the hotbar" actually possible for the first time. This is the single highest-value step — it's the literal thing P6's own phase file already flagged as the known gap.
3. **Durability persistence through equip/unequip** (§5) — now cheap, since the plumbing from steps 1-2 already carries the state around.
4. **Ammo as a real inventory item** (§4) — independent of steps 1-3, could actually be done first if preferred; ordered here because it's a good "prove the item-instance model works for something other than weapons" checkpoint.
5. **Weapon handedness/secondary-hand-legality fields** (§7 Tier 1) — cheap, unblocks the amputation-restricts-weapon-use design intent.
6. **`SecondaryHand` + `IA_SecondaryAction`** (§6) — now has somewhere real to plug into.
7. **Attachments** (§7 Tier 2) — only if approved; last, because everything above should be solid before adding a system that layers on top of it.

## 9. Open questions — the compressed list

Everything below is a place I made a call so the doc could be complete, but where the actual decision should be yours:

1. **The item-instance/GUID model itself (§5)** — this is the biggest architectural change proposed here, and it touches code shipped just last round. Confirm you want this direction before it's built, since it's a real refactor, not an additive change.
2. **Ammo-as-inventory-item (§4)** — removes `AZSWeapon::CurrentReserveAmmo`/`MaxReserveAmmo` as concepts entirely in favor of inventory stack counts. Confirm.
3. **`SecondaryHand` trigger (§6)** — proposed a dedicated new input action; need your actual key-binding preference (or "not now" if you'd rather this stay deferred a third time).
4. **Gear-slot equip timing (§6)** — instant (as shipped) vs. real-time like combat slots. No stated reasoning exists for today's "instant" choice; worth an actual decision.
5. **Attachments — build at all? (§7 Tier 2)** — my read of the source docs is "no, not yet, stay Tier 1." Flag if you disagree.
6. **Slot-count cap vs. weight-only (from `DevMarkupNotes.md` §10.2's "items take up a single spot")** — today's model is pure weight + stacking, no cap on the *number* of distinct carry slots. Your original note's wording ("items take up a single spot") could mean container loot specifically (each loot roll = one item, which is already true) or could mean the player's own inventory should also have a slot-count limit alongside weight. Worth clarifying since it changes `UZSInventoryComponent`'s shape.
