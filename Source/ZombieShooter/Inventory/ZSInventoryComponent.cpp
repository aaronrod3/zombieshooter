// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSInventoryComponent.h"
#include "ZSWorldItemActor.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "../ZombieShooter.h"

UZSInventoryComponent::UZSInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UZSInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UZSInventoryComponent, CarrySlots);
	DOREPLIFETIME(UZSInventoryComponent, EquippedBack);
	DOREPLIFETIME(UZSInventoryComponent, EquippedHip);
}

float UZSInventoryComponent::GetCurrentWeight() const
{
	float Total = 0.f;

	// B0-T2 Step B: equipping never removes an instance from CarrySlots (see Server_EquipToSlot), so
	// an equipped bag's weight is already counted by this loop alone - no separate EquippedBack/Hip
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

	const FZSItemInstance BackInstance = GetInstance(EquippedBack);
	if (BackInstance.Config) { MaxWeight += BackInstance.Config->CarryCapacityBonus; }

	const FZSItemInstance HipInstance = GetInstance(EquippedHip);
	if (HipInstance.Config) { MaxWeight += HipInstance.Config->CarryCapacityBonus; }

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

FZSItemInstance UZSInventoryComponent::GetEquippedItem(EZSEquipSlot Slot) const
{
	switch (Slot)
	{
	case EZSEquipSlot::Back: return GetInstance(EquippedBack);
	case EZSEquipSlot::Hip: return GetInstance(EquippedHip);
	default: return FZSItemInstance();
	}
}

int32 UZSInventoryComponent::Server_AddItem(UZSItemConfig* Item, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Count <= 0)
	{
		return 0;
	}

	const int32 StackSize = FMath::Max(Item->MaxStackSize, 1);
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
	while (Remaining > 0)
	{
		FZSItemInstance NewInstance;
		NewInstance.InstanceId = FGuid::NewGuid();
		NewInstance.Config = Item;
		NewInstance.StackCount = FMath::Min(Remaining, StackSize);
		NewInstance.Location = EZSCarryLocation::OnPerson;
		CarrySlots.Add(NewInstance);
		Remaining -= NewInstance.StackCount;
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
			CarrySlots.Add(NewInstance);
			Remaining -= NewInstance.StackCount;
		}
	}
	else
	{
		// Non-stackable: the instance keeps its own identity intact - this is what makes a dropped
		// weapon's durability survive being picked back up (B0-T2's headline fix).
		Instance.Location = EZSCarryLocation::OnPerson;
		CarrySlots.Add(Instance);
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
	if (!GetOwner() || !GetOwner()->HasAuthority() || Slot == EZSEquipSlot::None || !InstanceId.IsValid())
	{
		return false;
	}

	if (InstanceId == EquippedBack || InstanceId == EquippedHip)
	{
		// Already worn in the other gear slot - can't equip the same physical bag twice at once.
		return false;
	}

	const FZSItemInstance Instance = GetInstance(InstanceId);
	if (!Instance.IsValid() || !Instance.Config->bIsEquippable || Instance.Config->EquipSlot != Slot)
	{
		return false;
	}

	// Nothing is removed from CarrySlots - equipping just points the slot at an instance that's
	// still (and stays) resident there. See the header comment on this function.
	FGuid& TargetRef = (Slot == EZSEquipSlot::Back) ? EquippedBack : EquippedHip;
	TargetRef = InstanceId;

	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_UnequipSlot(EZSEquipSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Slot == EZSEquipSlot::None)
	{
		return;
	}

	FGuid& TargetRef = (Slot == EZSEquipSlot::Back) ? EquippedBack : EquippedHip;
	if (!TargetRef.IsValid())
	{
		return;
	}

	TargetRef = FGuid();

	OnRep_InventoryState();
}

bool UZSInventoryComponent::Server_StoreInBag(FGuid BagInstanceId, FGuid ItemInstanceId)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !BagInstanceId.IsValid() || !ItemInstanceId.IsValid() || BagInstanceId == ItemInstanceId)
	{
		return false;
	}

	FZSItemInstance* Bag = CarrySlots.FindByPredicate([BagInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == BagInstanceId; });
	if (!Bag || !Bag->Config || !Bag->Config->bIsEquippable)
	{
		return false;
	}

	const int32 ItemIndex = CarrySlots.IndexOfByPredicate([ItemInstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == ItemInstanceId; });
	if (ItemIndex == INDEX_NONE)
	{
		return false;
	}

	// Reject storing an instance currently equipped to a gear slot - GetInstance()/GetEquippedItem()
	// only resolve top-level CarrySlots, so nesting it here would silently orphan EquippedBack/Hip's
	// GUID reference (it'd resolve to an invalid instance from then on) instead of clearing it.
	// Note: this doesn't cover HotbarSlots/SecondaryHandInstanceId, which live on the owning
	// AZSPlayerCharacter, not here - closing that half needs the character to validate before
	// calling this, or a new cross-component query, which is a real design call, not a one-line fix.
	if (ItemInstanceId == EquippedBack || ItemInstanceId == EquippedHip)
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
	MovedItem.Location = EZSCarryLocation::Bag;
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

	FZSItemInstance MovedItem(Bag->ContainedItems[ItemIndex]);
	MovedItem.Location = EZSCarryLocation::OnPerson;
	Bag->ContainedItems.RemoveAt(ItemIndex);

	CarrySlots.Add(MovedItem);

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
	EquippedBack = FGuid();
	EquippedHip = FGuid();
	OnRep_InventoryState();
}

void UZSInventoryComponent::OnRep_InventoryState()
{
	OnInventoryChanged.Broadcast();
}
