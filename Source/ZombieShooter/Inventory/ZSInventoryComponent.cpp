// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSInventoryComponent.h"
#include "ZSWorldItemActor.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "../Weapons/ZSWeaponConfig.h"
#include "../ZombieShooter.h"

UZSInventoryComponent::UZSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	// B1-T5.0: always exactly NumLongGunMounts elements (invalid FGuid = empty mount), same "always
	// sized, index is the identity" reasoning as AZSPlayerCharacter::HotbarSlots.
	MountedLongGuns.SetNum(NumLongGunMounts);

	// 2026-08-06: same reasoning, now applied to the 11-value clothing/gear system - index 0
	// (EZSEquipSlot::None) is never written to.
	EquippedSlots.SetNum(NumEquipSlots);
}

void UZSInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZSInventoryComponent, CarrySlots);
	DOREPLIFETIME(UZSInventoryComponent, EquippedSlots);
	DOREPLIFETIME(UZSInventoryComponent, MountedLongGuns);
	DOREPLIFETIME(UZSInventoryComponent, MountedSidearm);
	DOREPLIFETIME(UZSInventoryComponent, MountedMelee);
}

float UZSInventoryComponent::GetCurrentWeight() const
{
	float Total = 0.f;

	// B0-T2 Step B: equipping never removes an instance from CarrySlots (see Server_EquipToSlot), so
	// an equipped bag's weight is already counted by this loop alone - no separate EquippedBack/Duffle
	// special-case needed anymore. GetTotalWeight() (B0-T2.9) folds in a bag's ContainedItems too.
	for (const FZSItemInstance& Instance : CarrySlots)
	{
		Total += Instance.GetTotalWeight();
	}

	return Total;
}

float UZSInventoryComponent::GetMaxCarryWeight() const
{
	float MaxWeight = BaseCarryWeight;

	// 2026-08-06: generalized from the old hardcoded EquippedBack/EquippedDuffle pair to loop every
	// real equip slot - plain clothing defaults CarryCapacityBonus to 0, so this is a no-op for them.
	for (const FGuid& Equipped : EquippedSlots)
	{
		const FZSItemInstance Instance = GetInstance(Equipped);
		if (Instance.Config)
		{
			MaxWeight += Instance.Config->CarryCapacityBonus;
		}
	}

	return MaxWeight;
}

float UZSInventoryComponent::GetEncumbranceMultiplier() const
{
	const float MaxWeight = GetMaxCarryWeight();
	if (MaxWeight <= 0.f)
	{
		return 1.f;
	}

	const float WeightRatio = GetCurrentWeight() / MaxWeight;
	if (WeightRatio <= 1.f)
	{
		return 1.f;
	}

	const float OverloadAlpha = FMath::Clamp((WeightRatio - 1.f) / FMath::Max(OverloadWeightRatio - 1.f, KINDA_SMALL_NUMBER), 0.f, 1.f);
	return FMath::Lerp(1.f, MinEncumbranceMultiplier, OverloadAlpha);
}

FZSItemInstance UZSInventoryComponent::GetInstance(FGuid InstanceId) const
{
	if (InstanceId.IsValid())
	{
		for (const FZSItemInstance& Instance : CarrySlots)
		{
			if (Instance.InstanceId == InstanceId)
			{
				return Instance;
			}
		}
	}
	return FZSItemInstance();
}

TArray<FZSItemInstance> UZSInventoryComponent::GetSlotsInLocation(EZSCarryLocation Location) const
{
	TArray<FZSItemInstance> Result;

	// Pockets/OnPerson items never leave CarrySlots (Server_StoreInBag only ever moves things INTO a
	// bag, never into OnPerson from anywhere but a top-level slot) - keep scanning the flat array.
	// 2026-08-06: excludes anything equipped or weapon-mounted - see this function's header comment
	// for why (otherwise worn gear/mounted weapons would double-count against Pockets' fixed capacity).
	if (Location == EZSCarryLocation::OnPerson)
	{
		for (const FZSItemInstance& Instance : CarrySlots)
		{
			if (Instance.Location == Location && !EquippedSlots.Contains(Instance.InstanceId) && !IsWeaponMounted(Instance.InstanceId))
			{
				Result.Add(Instance);
			}
		}
		return Result;
	}

	// 2026-08-06 bugfix: Vest/Belt/Backpack/Duffle items live nested inside the equipped bag's own
	// ContainedItems (Server_StoreInBag moves them there, out of CarrySlots entirely) - this used to
	// scan CarrySlots for these too, which could never find anything, so these 4 compartments
	// rendered empty even when the bag genuinely held items. Resolve the equipped bag for this
	// location instead and read its contents.
	EZSEquipSlot GatingSlot = EZSEquipSlot::None;
	switch (Location)
	{
	case EZSCarryLocation::Vest:     GatingSlot = EZSEquipSlot::Vest;     break;
	case EZSCarryLocation::Belt:     GatingSlot = EZSEquipSlot::Belt;     break;
	case EZSCarryLocation::Backpack: GatingSlot = EZSEquipSlot::Backpack; break;
	case EZSCarryLocation::Duffle:   GatingSlot = EZSEquipSlot::Duffle;   break;
	default: return Result; // World/Vehicle - not resolvable through equipped gear.
	}

	const FZSItemInstance Bag = GetEquippedItem(GatingSlot);
	if (!Bag.IsValid())
	{
		return Result;
	}

	for (const FZSItemInstanceBase& Contained : Bag.ContainedItems)
	{
		Result.Add(FZSItemInstance(Contained));
	}

	return Result;
}

FZSItemInstance UZSInventoryComponent::GetEquippedItem(EZSEquipSlot Slot) const
{
	return EquippedSlots.IsValidIndex((uint8)Slot) ? GetInstance(EquippedSlots[(uint8)Slot]) : FZSItemInstance();
}

int32 UZSInventoryComponent::GetCompartmentCapacity(EZSCarryLocation Location)
{
	switch (Location)
	{
	case EZSCarryLocation::OnPerson: return 4;
	case EZSCarryLocation::Vest:     return 8;
	case EZSCarryLocation::Belt:     return 8;
	case EZSCarryLocation::Backpack: return 20; // 2026-08-07: 4x5 grid, was 4x6/24 - shrunk to fit the compartment-stack column's vertical budget
	case EZSCarryLocation::Duffle:   return 18;
	default: return 0;
	}
}

bool UZSInventoryComponent::IsContainerBearingSlot(EZSEquipSlot Slot)
{
	return Slot == EZSEquipSlot::Vest || Slot == EZSEquipSlot::Belt
		|| Slot == EZSEquipSlot::Backpack || Slot == EZSEquipSlot::Duffle;
}

int32 UZSInventoryComponent::FindFirstFreeSlotIndex(int32 Capacity, const TArray<int32>& OccupiedIndices)
{
	for (int32 Index = 0; Index < Capacity; ++Index)
	{
		if (!OccupiedIndices.Contains(Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

int32 UZSInventoryComponent::FindFirstFreeOnPersonSlotIndex() const
{
	TArray<int32> Occupied;
	for (const FZSItemInstance& Instance : CarrySlots)
	{
		if (Instance.Location == EZSCarryLocation::OnPerson && !EquippedSlots.Contains(Instance.InstanceId) && !IsWeaponMounted(Instance.InstanceId))
		{
			Occupied.Add(Instance.SlotIndex);
		}
	}
	return FindFirstFreeSlotIndex(GetCompartmentCapacity(EZSCarryLocation::OnPerson), Occupied);
}

int32 UZSInventoryComponent::FindFirstFreeBagSlotIndex(FGuid BagInstanceId) const
{
	EZSCarryLocation BagLocation;
	if (!ResolveBagLocation(BagInstanceId, BagLocation))
	{
		return INDEX_NONE;
	}

	const FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (!Bag)
	{
		return INDEX_NONE;
	}

	TArray<int32> Occupied;
	Occupied.Reserve(Bag->ContainedItems.Num());
	for (const FZSItemInstanceBase& Contained : Bag->ContainedItems)
	{
		Occupied.Add(Contained.SlotIndex);
	}
	return FindFirstFreeSlotIndex(GetCompartmentCapacity(BagLocation), Occupied);
}

void UZSInventoryComponent::RefreshOnPersonSlotIndex(FGuid InstanceId)
{
	if (FZSItemInstance* Instance = CarrySlots.FindByPredicate([InstanceId](const FZSItemInstance& Item) { return Item.InstanceId == InstanceId; }))
	{
		Instance->SlotIndex = FindFirstFreeOnPersonSlotIndex();
	}
}

bool UZSInventoryComponent::ResolveBagLocation(FGuid BagInstanceId, EZSCarryLocation& OutLocation) const
{
	const FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (!Bag || !Bag->Config || !Bag->Config->bIsEquippable || !IsContainerBearingSlot(Bag->Config->EquipSlot))
	{
		return false;
	}

	switch (Bag->Config->EquipSlot)
	{
	case EZSEquipSlot::Vest:     OutLocation = EZSCarryLocation::Vest;     return true;
	case EZSEquipSlot::Belt:     OutLocation = EZSCarryLocation::Belt;     return true;
	case EZSEquipSlot::Backpack: OutLocation = EZSCarryLocation::Backpack; return true;
	case EZSEquipSlot::Duffle:   OutLocation = EZSCarryLocation::Duffle;   return true;
	default: return false; // Unreachable - IsContainerBearingSlot already validated EquipSlot above.
	}
}

bool UZSInventoryComponent::FindItemAnywhere(FGuid InstanceId, FZSItemInstance& OutItem, FGuid& OutBagInstanceId) const
{
	if (const FZSItemInstance* TopLevel = CarrySlots.FindByPredicate([InstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == InstanceId; }))
	{
		OutItem = *TopLevel;
		OutBagInstanceId = FGuid();
		return true;
	}

	static const EZSEquipSlot ContainerSlots[] = { EZSEquipSlot::Vest, EZSEquipSlot::Belt, EZSEquipSlot::Backpack, EZSEquipSlot::Duffle };
	for (EZSEquipSlot Slot : ContainerSlots)
	{
		const FZSItemInstance Bag = GetEquippedItem(Slot);
		if (!Bag.IsValid())
		{
			continue;
		}

		if (const FZSItemInstanceBase* Contained = Bag.ContainedItems.FindByPredicate([InstanceId](const FZSItemInstanceBase& Instance) { return Instance.InstanceId == InstanceId; }))
		{
			OutItem = FZSItemInstance(*Contained);
			OutBagInstanceId = Bag.InstanceId;
			return true;
		}
	}

	return false;
}

bool UZSInventoryComponent::FindItemAtSlot(FGuid BagInstanceId, int32 SlotIndex, FZSItemInstance& OutItem) const
{
	if (!BagInstanceId.IsValid())
	{
		for (const FZSItemInstance& Instance : CarrySlots)
		{
			if (Instance.Location == EZSCarryLocation::OnPerson && Instance.SlotIndex == SlotIndex
				&& !EquippedSlots.Contains(Instance.InstanceId) && !IsWeaponMounted(Instance.InstanceId))
			{
				OutItem = Instance;
				return true;
			}
		}
		return false;
	}

	const FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (!Bag)
	{
		return false;
	}

	if (const FZSItemInstanceBase* Contained = Bag->ContainedItems.FindByPredicate([SlotIndex](const FZSItemInstanceBase& Instance) { return Instance.SlotIndex == SlotIndex; }))
	{
		OutItem = FZSItemInstance(*Contained);
		return true;
	}

	return false;
}

void UZSInventoryComponent::RemoveItemAnywhereById(FGuid InstanceId)
{
	const int32 TopLevelIndex = CarrySlots.IndexOfByPredicate([InstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == InstanceId; });
	if (TopLevelIndex != INDEX_NONE)
	{
		CarrySlots.RemoveAt(TopLevelIndex);
		return;
	}

	static const EZSEquipSlot ContainerSlots[] = { EZSEquipSlot::Vest, EZSEquipSlot::Belt, EZSEquipSlot::Backpack, EZSEquipSlot::Duffle };
	for (EZSEquipSlot Slot : ContainerSlots)
	{
		const FGuid BagId = EquippedSlots.IsValidIndex((uint8)Slot) ? EquippedSlots[(uint8)Slot] : FGuid();
		if (!BagId.IsValid())
		{
			continue;
		}

		FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagId; });
		if (!Bag)
		{
			continue;
		}

		const int32 ContainedIndex = Bag->ContainedItems.IndexOfByPredicate([InstanceId](const FZSItemInstanceBase& Instance) { return Instance.InstanceId == InstanceId; });
		if (ContainedIndex != INDEX_NONE)
		{
			Bag->ContainedItems.RemoveAt(ContainedIndex);
			return;
		}
	}
}

void UZSInventoryComponent::InsertItemInto(const FZSItemInstance& Item, FGuid BagInstanceId)
{
	if (!BagInstanceId.IsValid())
	{
		CarrySlots.Add(Item);
		return;
	}

	if (FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; }))
	{
		Bag->ContainedItems.Add(static_cast<FZSItemInstanceBase>(Item));
	}
}

bool UZSInventoryComponent::IsItemLegalForBag(const FZSItemInstance& Item, FGuid BagInstanceId) const
{
	if (!BagInstanceId.IsValid())
	{
		return true; // Pockets - no type/size gate beyond capacity, checked by the caller.
	}

	// B1-T5.0: weapons are excluded from every bag - same gate Server_StoreInBag enforces.
	if (Cast<UZSWeaponConfig>(Item.Config))
	{
		return false;
	}

	// A bag whose own ContainedItems is non-empty can't nest inside another bag.
	if (Item.ContainedItems.Num() > 0)
	{
		return false;
	}

	// Can't move an item that's currently worn/equipped into a bag - it'd orphan the equip-slot
	// reference. Same guard Server_StoreInBagChecked applies via the character wrapper.
	if (EquippedSlots.Contains(Item.InstanceId))
	{
		return false;
	}

	const FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (Bag && Bag->Config->EquipSlot == EZSEquipSlot::Backpack && Item.Config && Item.Config->ItemSize == EZSItemSize::Large)
	{
		return false;
	}

	return true;
}

bool UZSInventoryComponent::CanFitInPockets(const UZSItemConfig* Item, int32 Count) const
{
	if (!Item || Count <= 0)
	{
		return false;
	}

	const int32 StackSize = FMath::Max(Item->MaxStackSize, 1);

	// One pass: count how many OnPerson slots are already occupied (same exclusions
	// GetSlotsInLocation/FindFirstFreeOnPersonSlotIndex apply - equipped/mounted items don't count)
	// while simultaneously simulating stack absorption, in the same order the real fill loops in
	// Server_AddItem/Server_AddItemInstance use, so this can never silently drift out of sync with them.
	int32 OccupiedCount = 0;
	int32 SimRemaining = Count;
	for (const FZSItemInstance& Instance : CarrySlots)
	{
		if (Instance.Location != EZSCarryLocation::OnPerson || EquippedSlots.Contains(Instance.InstanceId) || IsWeaponMounted(Instance.InstanceId))
		{
			continue;
		}

		++OccupiedCount;
		if (SimRemaining > 0 && Instance.Config == Item && Instance.StackCount < StackSize)
		{
			SimRemaining -= FMath::Min(StackSize - Instance.StackCount, SimRemaining);
		}
	}

	const int32 NewSlotsNeeded = (SimRemaining + StackSize - 1) / StackSize;
	return OccupiedCount + NewSlotsNeeded <= GetCompartmentCapacity(EZSCarryLocation::OnPerson);
}

FZSItemInstance UZSInventoryComponent::GetMountedLongGun(int32 MountIndex) const
{
	return MountedLongGuns.IsValidIndex(MountIndex) ? GetInstance(MountedLongGuns[MountIndex]) : FZSItemInstance();
}

int32 UZSInventoryComponent::Server_AddItem(UZSItemConfig* Item, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Count <= 0)
	{
		return 0;
	}

	const int32 StackSize = FMath::Max(Item->MaxStackSize, 1);

	// 2026-08-06: atomic pre-check - if fully honoring Count would need more new Pockets slots than
	// are currently free, reject the whole call outright (dev-confirmed: a full Pockets rejects the
	// add rather than silently losing whatever didn't fit). 2026-08-10: factored into CanFitInPockets,
	// shared with Server_AddItemInstance - see its own comment.
	if (!CanFitInPockets(Item, Count))
	{
		return 0;
	}

	int32 Remaining = Count;

	// Fill existing partial stacks first.
	for (FZSItemInstance& Instance : CarrySlots)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Instance.Config == Item && Instance.StackCount < StackSize)
		{
			const int32 ToAdd = FMath::Min(StackSize - Instance.StackCount, Remaining);
			Instance.StackCount += ToAdd;
			Remaining -= ToAdd;
		}
	}

	// New instances for whatever's left - each mints its own GUID (B0-T2.2).
	FGuid FirstNewInstanceId;
	while (Remaining > 0)
	{
		FZSItemInstance NewInstance;
		NewInstance.InstanceId = FGuid::NewGuid();
		NewInstance.Config = Item;
		NewInstance.StackCount = FMath::Min(Remaining, StackSize);
		NewInstance.Location = EZSCarryLocation::OnPerson;
		NewInstance.SlotIndex = FindFirstFreeOnPersonSlotIndex();
		CarrySlots.Add(NewInstance);
		Remaining -= NewInstance.StackCount;

		if (!FirstNewInstanceId.IsValid())
		{
			FirstNewInstanceId = NewInstance.InstanceId;
		}
	}

	// 2026-08-09, dev-confirmed: a freshly-carried container-bearing gear item (Vest/Belt/Backpack/
	// Duffle) auto-equips itself if that slot is currently empty - "pick up a backpack, it's
	// immediately worn and its compartment opens," no separate manual-equip step. Only the first
	// newly-minted instance is considered (equipping is a one-item action) and only if the slot isn't
	// already occupied - never bumps something already worn there.
	if (FirstNewInstanceId.IsValid() && Item->bIsEquippable && IsContainerBearingSlot(Item->EquipSlot)
		&& !GetEquippedItem(Item->EquipSlot).IsValid())
	{
		Server_EquipToSlot(Item->EquipSlot, FirstNewInstanceId);
	}

	OnRep_InventoryState();

	// Temporary verification logging for B0-T1 Stage G re-test - remove once a real inventory UI
	// exists and this is visible without the log (same note as Server_Fire).
	UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_AddItem - weight now %.1f / %.1f (encumbrance x%.2f)"),
		*GetOwner()->GetName(), GetCurrentWeight(), GetMaxCarryWeight(), GetEncumbranceMultiplier());

	return Count;
}

bool UZSInventoryComponent::Server_AddItemInstance(FZSItemInstance Instance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Instance.IsValid() || Instance.StackCount <= 0)
	{
		return false;
	}

	const int32 StackSize = FMath::Max(Instance.Config->MaxStackSize, 1);

	// 2026-08-06: same atomic capacity pre-check as Server_AddItem - see its comment. This is the
	// mechanism behind AZSWorldItemActor::HandleInteracted's "reject the pickup, item stays in the
	// world" behavior when Pockets is full. 2026-08-10: factored into CanFitInPockets, shared with
	// Server_AddItem - see its own comment.
	if (!CanFitInPockets(Instance.Config, Instance.StackCount))
	{
		return false;
	}

	if (StackSize > 1)
	{
		// Stackable: merge into existing partial stacks first, same splitting logic as Server_AddItem
		// - the incoming instance's own InstanceId/InstanceState are discarded on merge, since a
		// merged stack has no sub-identity (FZSItemInstance's own invariant comment).
		int32 Remaining = Instance.StackCount;
		for (FZSItemInstance& Existing : CarrySlots)
		{
			if (Remaining <= 0)
			{
				break;
			}

			if (Existing.Config == Instance.Config && Existing.StackCount < StackSize)
			{
				const int32 ToAdd = FMath::Min(StackSize - Existing.StackCount, Remaining);
				Existing.StackCount += ToAdd;
				Remaining -= ToAdd;
			}
		}

		while (Remaining > 0)
		{
			FZSItemInstance NewInstance;
			NewInstance.InstanceId = FGuid::NewGuid();
			NewInstance.Config = Instance.Config;
			NewInstance.StackCount = FMath::Min(Remaining, StackSize);
			NewInstance.Location = EZSCarryLocation::OnPerson;
			NewInstance.SlotIndex = FindFirstFreeOnPersonSlotIndex();
			CarrySlots.Add(NewInstance);
			Remaining -= NewInstance.StackCount;
		}
	}
	else
	{
		// Non-stackable: the instance keeps its own identity intact - this is what makes a dropped
		// weapon's durability survive being picked back up (B0-T2's headline fix). SlotIndex is NOT
		// preserved the same way, though - wherever it sat before (a different compartment, possibly
		// even a now-invalid index) has no bearing on where it's landing now, so it's freshly assigned
		// here same as everything else that lands in Pockets.
		Instance.Location = EZSCarryLocation::OnPerson;
		Instance.SlotIndex = FindFirstFreeOnPersonSlotIndex();
		CarrySlots.Add(Instance);

		// 2026-08-09: same auto-equip-on-pickup as Server_AddItem (see its comment) - this is the path
		// a real world-item pickup (AZSWorldItemActor::HandleInteracted) and container loot transfers
		// actually go through, so this is where "pick up a backpack, it's immediately worn" happens
		// for the case that matters most.
		if (Instance.Config->bIsEquippable && IsContainerBearingSlot(Instance.Config->EquipSlot)
			&& !GetEquippedItem(Instance.Config->EquipSlot).IsValid())
		{
			Server_EquipToSlot(Instance.Config->EquipSlot, Instance.InstanceId);
		}
	}

	OnRep_InventoryState();

	// Temporary verification logging for B0-T1 Stage G re-test - remove once a real inventory UI
	// exists and this is visible without the log (same note as Server_Fire).
	UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_AddItemInstance - weight now %.1f / %.1f (encumbrance x%.2f)"),
		*GetOwner()->GetName(), GetCurrentWeight(), GetMaxCarryWeight(), GetEncumbranceMultiplier());

	return true;
}

TArray<FZSItemInstance> UZSInventoryComponent::Server_RemoveItem(UZSItemConfig* Item, int32 Count)
{
	TArray<FZSItemInstance> Removed;

	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Count <= 0)
	{
		return Removed;
	}

	int32 Remaining = Count;

	for (int32 Index = CarrySlots.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FZSItemInstance& Instance = CarrySlots[Index];
		if (Instance.Config != Item)
		{
			continue;
		}

		const int32 ToRemove = FMath::Min(Instance.StackCount, Remaining);

		if (ToRemove >= Instance.StackCount)
		{
			// Whole instance consumed - preserve its identity (GUID/InstanceState) in the output.
			Removed.Add(Instance);
			CarrySlots.RemoveAt(Index);
		}
		else
		{
			// Partial stack split-off - the remainder has no individual identity to preserve, so it
			// gets a fresh GUID rather than reusing the still-carried stack's.
			Instance.StackCount -= ToRemove;

			FZSItemInstance Fragment;
			Fragment.InstanceId = FGuid::NewGuid();
			Fragment.Config = Instance.Config;
			Fragment.StackCount = ToRemove;
			Fragment.Location = Instance.Location;
			Removed.Add(Fragment);
		}

		Remaining -= ToRemove;
	}

	if (Removed.Num() > 0)
	{
		OnRep_InventoryState();
	}
	return Removed;
}

bool UZSInventoryComponent::Server_RemoveInstanceById(FGuid InstanceId, FZSItemInstance& OutRemoved)
{
	OutRemoved = FZSItemInstance();

	if (!GetOwner() || !GetOwner()->HasAuthority() || !InstanceId.IsValid())
	{
		return false;
	}

	for (int32 Index = 0; Index < CarrySlots.Num(); ++Index)
	{
		if (CarrySlots[Index].InstanceId == InstanceId)
		{
			OutRemoved = CarrySlots[Index];
			CarrySlots.RemoveAt(Index);
			OnRep_InventoryState();
			return true;
		}
	}
	return false;
}

bool UZSInventoryComponent::Server_RemoveInstanceByIdAnywhere(FGuid InstanceId, FZSItemInstance& OutRemoved)
{
	OutRemoved = FZSItemInstance();

	if (!GetOwner() || !GetOwner()->HasAuthority() || !InstanceId.IsValid())
	{
		return false;
	}

	if (EquippedSlots.Contains(InstanceId) || IsWeaponMounted(InstanceId))
	{
		return false;
	}

	FGuid BagInstanceId;
	if (!FindItemAnywhere(InstanceId, OutRemoved, BagInstanceId))
	{
		return false;
	}

	RemoveItemAnywhereById(InstanceId);
	OnRep_InventoryState();
	return true;
}

bool UZSInventoryComponent::Server_UpdateInstanceState(FGuid InstanceId, const FZSItemInstanceState& NewState)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !InstanceId.IsValid())
	{
		return false;
	}

	for (FZSItemInstance& Instance : CarrySlots)
	{
		if (Instance.InstanceId == InstanceId)
		{
			Instance.InstanceState = NewState;
			OnRep_InventoryState();
			return true;
		}
	}
	return false;
}

bool UZSInventoryComponent::Server_EquipToSlot(EZSEquipSlot Slot, FGuid InstanceId)
{
	// 2026-08-11: logged on every rejection path - this function has been implicated in two
	// unexplained automation failures (CompartmentCapacityAndStoreRegression's Backpack auto-equip,
	// EquipHelmetForceUnequipsHead) that both read correct by inspection. Silent no-ops made both
	// undiagnosable from a log; this doesn't fix either, but the next run will say exactly which
	// guard clause rejected the call instead of leaving it a mystery.
	if (!GetOwner() || !GetOwner()->HasAuthority() || Slot == EZSEquipSlot::None || !InstanceId.IsValid())
	{
		UE_LOG(LogZombieShooter, Warning, TEXT("Server_EquipToSlot(%s, %s) REJECTED - no owner/authority, Slot==None, or invalid InstanceId"),
			*UEnum::GetValueAsString(Slot), *InstanceId.ToString());
		return false;
	}

	if (!EquippedSlots.IsValidIndex((uint8)Slot))
	{
		UE_LOG(LogZombieShooter, Warning, TEXT("Server_EquipToSlot(%s, %s) REJECTED - Slot index %d not valid in EquippedSlots (Num=%d)"),
			*UEnum::GetValueAsString(Slot), *InstanceId.ToString(), (int32)Slot, EquippedSlots.Num());
		return false;
	}

	if (EquippedSlots.Contains(InstanceId))
	{
		// Already worn in some other gear slot (generalized 2026-08-06 from the old Back-vs-Duffle-
		// only check) - can't equip the same physical item twice at once.
		UE_LOG(LogZombieShooter, Warning, TEXT("Server_EquipToSlot(%s, %s) REJECTED - InstanceId already occupies some EquippedSlots entry"),
			*UEnum::GetValueAsString(Slot), *InstanceId.ToString());
		return false;
	}

	const FZSItemInstance Instance = GetInstance(InstanceId);
	if (!Instance.IsValid() || !Instance.Config->bIsEquippable || Instance.Config->EquipSlot != Slot)
	{
		UE_LOG(LogZombieShooter, Warning, TEXT("Server_EquipToSlot(%s, %s) REJECTED - Instance.IsValid=%d bIsEquippable=%d Config's own EquipSlot=%s"),
			*UEnum::GetValueAsString(Slot), *InstanceId.ToString(), Instance.IsValid(),
			Instance.IsValid() && Instance.Config ? Instance.Config->bIsEquippable : false,
			Instance.IsValid() && Instance.Config ? *UEnum::GetValueAsString(Instance.Config->EquipSlot) : TEXT("N/A"));
		return false;
	}

	// Dev-confirmed, one-way only: equipping into Helmet force-unequips whatever's in Head.
	// Equipping into Head while Helmet is worn does not reciprocally unequip Helmet.
	// 2026-08-10: this bypassed Server_UnequipSlot, so it never called RefreshOnPersonSlotIndex on the
	// outgoing Head item - reproducing exactly the stale-SlotIndex collision RefreshOnPersonSlotIndex
	// exists to prevent everywhere else an item goes from invisible-to-Pockets back to visible.
	if (Slot == EZSEquipSlot::Helmet && EquippedSlots[(uint8)EZSEquipSlot::Head].IsValid())
	{
		RefreshOnPersonSlotIndex(EquippedSlots[(uint8)EZSEquipSlot::Head]);
		EquippedSlots[(uint8)EZSEquipSlot::Head] = FGuid();
	}

	// Nothing is removed from CarrySlots - equipping just points the slot at an instance that's
	// still (and stays) resident there. See the header comment on this function.
	EquippedSlots[(uint8)Slot] = InstanceId;

	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_UnequipSlot(EZSEquipSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !EquippedSlots.IsValidIndex((uint8)Slot))
	{
		return;
	}

	if (!EquippedSlots[(uint8)Slot].IsValid())
	{
		return;
	}

	// 2026-08-09: the unequipped item becomes a visible Pockets item again - give it a fresh slot
	// rather than reusing its stale one, see RefreshOnPersonSlotIndex's own comment for why.
	RefreshOnPersonSlotIndex(EquippedSlots[(uint8)Slot]);
	EquippedSlots[(uint8)Slot] = FGuid();

	OnRep_InventoryState();
}

bool UZSInventoryComponent::Server_StoreInBag(FGuid BagInstanceId, FGuid ItemInstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BagInstanceId.IsValid() || !ItemInstanceId.IsValid() || BagInstanceId == ItemInstanceId)
	{
		return false;
	}

	FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	// B1-T5.0: EquipSlot must actually be a container-bearing slot, not just bIsEquippable - a
	// mounted sidearm (bIsEquippable + EquipSlot == None, since weapon mounts don't use this enum at
	// all) or a plain clothing item shouldn't be treated as storage. Generalized 2026-08-06 from the
	// old Back/Duffle-only check to all 4 container-bearing slots (IsContainerBearingSlot).
	if (!Bag || !Bag->Config || !Bag->Config->bIsEquippable || !IsContainerBearingSlot(Bag->Config->EquipSlot))
	{
		return false;
	}

	const int32 ItemIndex = CarrySlots.IndexOfByPredicate([ItemInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == ItemInstanceId; });
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	// B1-T5.0: weapons are excluded from all three compartments entirely - they use the mount slots
	// instead. A UZSWeaponConfig instance would otherwise pass every check above (a rifle could be
	// bIsEquippable == false by default, so this is mostly a safety net, not the primary gate).
	if (Cast<UZSWeaponConfig>(CarrySlots[ItemIndex].Config))
	{
		return false;
	}

	// 2026-08-06: compartment-size privilege (EZSItemSize's own comment, Docs/Planning/
	// B1_UIDesignSession_2026-07-30.md) - Backpack only accepts Small/Medium; Vest/Belt/Duffle accept
	// everything (Duffle: "largest capacity bonus" per EZSEquipSlot::Duffle's own comment). Pockets
	// (OnPerson) is deliberately NOT gated anywhere - it's the always-available fallback, not
	// something a player "stores into" the way a bag is: a fresh pickup
	// (Server_AddItem/Server_AddItemInstance) always defaults to OnPerson regardless of size, and
	// Server_RetrieveFromBag below always lands an item back on OnPerson unconditionally too.
	if (Bag->Config->EquipSlot == EZSEquipSlot::Backpack
		&& CarrySlots[ItemIndex].Config && CarrySlots[ItemIndex].Config->ItemSize == EZSItemSize::Large)
	{
		return false;
	}

	// Map the bag's EquipSlot to the FZSItemInstance::Location value it grants storage under - the
	// two enums deliberately share names (Vest/Belt/Backpack/Duffle) for exactly this 1:1 mapping.
	EZSCarryLocation TargetLocation;
	switch (Bag->Config->EquipSlot)
	{
	case EZSEquipSlot::Vest:     TargetLocation = EZSCarryLocation::Vest;     break;
	case EZSEquipSlot::Belt:     TargetLocation = EZSCarryLocation::Belt;     break;
	case EZSEquipSlot::Backpack: TargetLocation = EZSCarryLocation::Backpack; break;
	case EZSEquipSlot::Duffle:   TargetLocation = EZSCarryLocation::Duffle;   break;
	default: return false; // Unreachable - IsContainerBearingSlot already validated EquipSlot above.
	}

	// 2026-08-06: capacity check that didn't exist before this rework - a container-bearing bag's
	// fixed-grid compartment (GetCompartmentCapacity) can't accept more than it holds.
	if (Bag->ContainedItems.Num() >= GetCompartmentCapacity(TargetLocation))
	{
		return false;
	}

	// Reject storing an instance currently equipped to a gear slot - GetInstance()/GetEquippedItem()
	// only resolve top-level CarrySlots, so nesting it here would silently orphan that slot's GUID
	// reference (it'd resolve to an invalid instance from then on) instead of clearing it.
	// Note: this doesn't cover HotbarSlots/SecondaryHandInstanceId/weapon mounts, which live on the
	// owning AZSPlayerCharacter or are checked above - Closed 2026-07-30 via
	// AZSPlayerCharacter::Server_StoreInBagChecked, which validates against that character-side state
	// before calling this function - callers should go through that wrapper, not this function
	// directly, wherever a character is available.
	if (EquippedSlots.Contains(ItemInstanceId))
	{
		return false;
	}

	// A bag whose own ContainedItems is non-empty can't nest inside another bag - ContainedItems is
	// typed FZSItemInstanceBase precisely so this can't be represented, so reject explicitly here
	// rather than silently truncating the inner bag's contents on the slice below.
	if (CarrySlots[ItemIndex].ContainedItems.Num() > 0)
	{
		return false;
	}

	FZSItemInstance MovedItem = CarrySlots[ItemIndex];
	MovedItem.Location = TargetLocation;
	MovedItem.SlotIndex = FindFirstFreeBagSlotIndex(BagInstanceId);
	CarrySlots.RemoveAt(ItemIndex);

	// Re-find Bag - CarrySlots.RemoveAt above may have reallocated/shifted the array, invalidating
	// the earlier pointer.
	Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (!Bag)
	{
		// Shouldn't happen (Bag != Item, so removing Item can't have removed Bag too), but don't
		// silently drop the item if it somehow does - put it back rather than lose it.
		CarrySlots.Add(MovedItem);
		OnRep_InventoryState();
		return false;
	}
	Bag->ContainedItems.Add(static_cast<FZSItemInstanceBase>(MovedItem));

	OnRep_InventoryState();
	return true;
}

bool UZSInventoryComponent::Server_RetrieveFromBag(FGuid BagInstanceId, FGuid ItemInstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BagInstanceId.IsValid() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (!Bag)
	{
		return false;
	}

	const int32 ItemIndex = Bag->ContainedItems.IndexOfByPredicate([ItemInstanceId](const FZSItemInstanceBase& Instance) { return Instance.InstanceId == ItemInstanceId; });
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	// 2026-08-09: pre-existing gap - retrieving into an already-full Pockets used to silently add a
	// 5th+ CarrySlots entry the fixed 4-slot grid would never render (SlotWidgets.Num() caps the
	// display loop), a "phantom carried but invisible" item. Reject outright instead, same atomic
	// capacity gate every other Pockets-landing path (Server_AddItem/Server_AddItemInstance) already
	// enforces - the item stays in the bag rather than becoming untrackable.
	if (GetSlotsInLocation(EZSCarryLocation::OnPerson).Num() >= GetCompartmentCapacity(EZSCarryLocation::OnPerson))
	{
		return false;
	}

	FZSItemInstance MovedItem(Bag->ContainedItems[ItemIndex]);
	MovedItem.Location = EZSCarryLocation::OnPerson;
	MovedItem.SlotIndex = FindFirstFreeOnPersonSlotIndex();
	Bag->ContainedItems.RemoveAt(ItemIndex);

	CarrySlots.Add(MovedItem);

	OnRep_InventoryState();
	return true;
}

bool UZSInventoryComponent::Server_RetrieveFromAnyEquippedBag(FGuid ItemInstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemInstanceId.IsValid())
	{
		return false;
	}

	// Only the 4 container-bearing slots (Vest/Belt/Backpack/Duffle) can hold nested items at all.
	static const EZSEquipSlot ContainerSlots[] = { EZSEquipSlot::Vest, EZSEquipSlot::Belt, EZSEquipSlot::Backpack, EZSEquipSlot::Duffle };
	for (EZSEquipSlot Slot : ContainerSlots)
	{
		const FZSItemInstance Bag = GetEquippedItem(Slot);
		if (!Bag.IsValid())
		{
			continue;
		}

		if (Bag.ContainedItems.ContainsByPredicate([ItemInstanceId](const FZSItemInstanceBase& Instance) { return Instance.InstanceId == ItemInstanceId; }))
		{
			return Server_RetrieveFromBag(Bag.InstanceId, ItemInstanceId);
		}
	}

	return false;
}

bool UZSInventoryComponent::Server_MoveToSlot(FGuid ItemInstanceId, FGuid TargetBagInstanceId, int32 TargetSlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !ItemInstanceId.IsValid() || TargetSlotIndex < 0)
	{
		return false;
	}

	// 2026-08-11: reject a currently-equipped or weapon-mounted instance outright - unreachable
	// through the normal UI flow (GetSlotsInLocation already excludes both from ever populating a
	// draggable UZSItemSlotWidget), but a raw RPC call bypassing the UI previously wasn't caught
	// server-side, which would silently orphan the EquippedSlots/mount-array GUID reference the same
	// way the fixed Server_StoreInBag bug used to. Same combined check as ResolveMountableWeapon.
	if (IsWeaponMounted(ItemInstanceId) || EquippedSlots.Contains(ItemInstanceId))
	{
		return false;
	}

	// Resolve the target compartment and its capacity.
	EZSCarryLocation TargetLocation = EZSCarryLocation::OnPerson;
	if (TargetBagInstanceId.IsValid() && !ResolveBagLocation(TargetBagInstanceId, TargetLocation))
	{
		return false;
	}
	if (TargetSlotIndex >= GetCompartmentCapacity(TargetLocation))
	{
		return false;
	}

	// Locate and copy out the moving item - nothing is mutated yet, everything below validates
	// first so a rejected swap leaves both items exactly where they started.
	FZSItemInstance MovingItem;
	FGuid OriginBagInstanceId;
	if (!FindItemAnywhere(ItemInstanceId, MovingItem, OriginBagInstanceId))
	{
		return false;
	}

	const int32 OriginSlotIndex = MovingItem.SlotIndex;
	if (OriginBagInstanceId == TargetBagInstanceId && OriginSlotIndex == TargetSlotIndex)
	{
		return false; // Dropped back onto its own current slot - no-op.
	}

	if (!IsItemLegalForBag(MovingItem, TargetBagInstanceId))
	{
		return false;
	}

	// Whatever currently occupies the target slot, if anything, needs to swap into MovingItem's old
	// spot rather than being displaced - validate that placement is legal too before touching
	// anything, so a swap either fully succeeds or nothing moves at all.
	FZSItemInstance Occupant;
	const bool bHasOccupant = FindItemAtSlot(TargetBagInstanceId, TargetSlotIndex, Occupant);
	if (bHasOccupant && !IsItemLegalForBag(Occupant, OriginBagInstanceId))
	{
		return false;
	}

	// --- Validated - mutate now. ---
	RemoveItemAnywhereById(ItemInstanceId);
	if (bHasOccupant)
	{
		RemoveItemAnywhereById(Occupant.InstanceId);
	}

	MovingItem.Location = TargetLocation;
	MovingItem.SlotIndex = TargetSlotIndex;
	InsertItemInto(MovingItem, TargetBagInstanceId);

	if (bHasOccupant)
	{
		EZSCarryLocation OriginLocation = EZSCarryLocation::OnPerson;
		if (OriginBagInstanceId.IsValid())
		{
			ResolveBagLocation(OriginBagInstanceId, OriginLocation); // Already validated above via IsItemLegalForBag's caller context.
		}
		Occupant.Location = OriginLocation;
		Occupant.SlotIndex = OriginSlotIndex;
		InsertItemInto(Occupant, OriginBagInstanceId);
	}

	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_DropItem(UZSItemConfig* Item, int32 Count)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || !Item || Count <= 0)
	{
		return;
	}

	const TArray<FZSItemInstance> Removed = Server_RemoveItem(Item, Count);
	if (Removed.Num() == 0)
	{
		return;
	}

	const FVector DropLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * DropDistance;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// One AZSWorldItemActor per removed instance/fragment (in practice almost always just one - see
	// Server_RemoveItem) so each preserves its own InstanceId/InstanceState (e.g. a weapon's
	// durability) rather than collapsing distinct instances into a single pile - B0-T2's headline
	// "durability survives drop/re-pickup" fix, verified at Checkpoint A with the GUID alone.
	int32 TotalRemoved = 0;
	for (const FZSItemInstance& Instance : Removed)
	{
		TotalRemoved += Instance.StackCount;
		if (AZSWorldItemActor* WorldItem = GetWorld()->SpawnActor<AZSWorldItemActor>(AZSWorldItemActor::StaticClass(), DropLocation, OwnerActor->GetActorRotation(), SpawnParams))
		{
			WorldItem->InitializeFromInstance(Instance);
		}

		// Temporary GUID logging for B0-T2 Checkpoint A - remove alongside the rest of this
		// session's verification logging (B0-T5.5).
		UE_LOG(LogZombieShooter, Log, TEXT("%s: dropped instance InstanceId %s (x%d)"),
			*OwnerActor->GetName(), *Instance.InstanceId.ToString(), Instance.StackCount);
	}

	// Temporary verification logging for B0-T1 Stage G re-test - remove once a real inventory UI
	// exists and this is visible without the log (same note as Server_Fire).
	UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_DropItem - dropped %s x%d, weight now %.1f / %.1f"),
		*OwnerActor->GetName(), *Item->DisplayName.ToString(), TotalRemoved, GetCurrentWeight(), GetMaxCarryWeight());
}

void UZSInventoryComponent::Server_DropAllItems(FVector DropLocation)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	// Same one-AZSWorldItemActor-per-instance reasoning as Server_DropItem - a death drop should
	// preserve every individual instance's identity (durability, a bag's own nested contents) just
	// as much as a deliberate single-item drop does, not collapse everything into one pile.
	for (const FZSItemInstance& Instance : CarrySlots)
	{
		if (AZSWorldItemActor* WorldItem = GetWorld()->SpawnActor<AZSWorldItemActor>(AZSWorldItemActor::StaticClass(), DropLocation, OwnerActor->GetActorRotation(), SpawnParams))
		{
			WorldItem->InitializeFromInstance(Instance);
		}
	}

	CarrySlots.Empty();
	for (FGuid& EquippedSlot : EquippedSlots)
	{
		EquippedSlot = FGuid();
	}
	// B1-T5.0: mounted weapons are just CarrySlots instances too (dropped in the loop above) - their
	// mount-slot GUID references need clearing the same way EquippedBack/Duffle's do.
	for (FGuid& MountSlot : MountedLongGuns)
	{
		MountSlot = FGuid();
	}
	MountedSidearm = FGuid();
	MountedMelee = FGuid();
	OnRep_InventoryState();
}

bool UZSInventoryComponent::Server_MountMelee(FGuid InstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const UZSWeaponConfig* WeaponConfig = ResolveMountableWeapon(InstanceId);
	// OneHanded + Melee only - the exact converse of Server_MountSidearm's gate, see the header
	// comment on this function for why.
	if (!WeaponConfig || WeaponConfig->Handedness != EZSWeaponHandedness::OneHanded || WeaponConfig->AttackType != EZSAttackType::Melee)
	{
		return false;
	}

	MountedMelee = InstanceId;
	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_UnmountMelee()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !MountedMelee.IsValid())
	{
		return;
	}

	// 2026-08-09: the unmounted weapon becomes a visible Pockets item again - see
	// RefreshOnPersonSlotIndex's own comment for why it needs a fresh slot, not its stale one.
	RefreshOnPersonSlotIndex(MountedMelee);
	MountedMelee = FGuid();
	OnRep_InventoryState();
}

bool UZSInventoryComponent::IsWeaponMounted(FGuid InstanceId) const
{
	if (!InstanceId.IsValid())
	{
		return false;
	}
	return MountedSidearm == InstanceId || MountedMelee == InstanceId || MountedLongGuns.Contains(InstanceId);
}

const UZSWeaponConfig* UZSInventoryComponent::ResolveMountableWeapon(FGuid InstanceId) const
{
	if (!InstanceId.IsValid() || IsWeaponMounted(InstanceId) || EquippedSlots.Contains(InstanceId))
	{
		return nullptr;
	}

	const FZSItemInstance Instance = GetInstance(InstanceId);
	return Instance.IsValid() ? Cast<UZSWeaponConfig>(Instance.Config) : nullptr;
}

bool UZSInventoryComponent::Server_MountLongGun(int32 MountIndex, FGuid InstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !MountedLongGuns.IsValidIndex(MountIndex))
	{
		return false;
	}

	const UZSWeaponConfig* WeaponConfig = ResolveMountableWeapon(InstanceId);
	if (!WeaponConfig || WeaponConfig->Handedness != EZSWeaponHandedness::TwoHanded)
	{
		return false;
	}

	MountedLongGuns[MountIndex] = InstanceId;
	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_UnmountLongGun(int32 MountIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !MountedLongGuns.IsValidIndex(MountIndex) || !MountedLongGuns[MountIndex].IsValid())
	{
		return;
	}

	// 2026-08-09: the unmounted weapon becomes a visible Pockets item again - see
	// RefreshOnPersonSlotIndex's own comment for why it needs a fresh slot, not its stale one.
	RefreshOnPersonSlotIndex(MountedLongGuns[MountIndex]);
	MountedLongGuns[MountIndex] = FGuid();
	OnRep_InventoryState();
}

bool UZSInventoryComponent::Server_MountSidearm(FGuid InstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}

	const UZSWeaponConfig* WeaponConfig = ResolveMountableWeapon(InstanceId);
	// OneHanded + Ranged only - excludes a one-handed melee weapon (e.g. a knife) from being
	// "mounted as a sidearm," see the header comment on this function.
	if (!WeaponConfig || WeaponConfig->Handedness != EZSWeaponHandedness::OneHanded || WeaponConfig->AttackType != EZSAttackType::Ranged)
	{
		return false;
	}

	MountedSidearm = InstanceId;
	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_UnmountSidearm()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !MountedSidearm.IsValid())
	{
		return;
	}

	// 2026-08-09: the unmounted weapon becomes a visible Pockets item again - see
	// RefreshOnPersonSlotIndex's own comment for why it needs a fresh slot, not its stale one.
	RefreshOnPersonSlotIndex(MountedSidearm);
	MountedSidearm = FGuid();
	OnRep_InventoryState();
}

void UZSInventoryComponent::OnRep_InventoryState()
{
	OnInventoryChanged.Broadcast();
}
