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

	for (const FZSItemInstance& Instance : CarrySlots)
	{
		if (Instance.Config)
		{
			Total += Instance.Config->Weight * Instance.StackCount;
		}
	}

	if (EquippedBack) { Total += EquippedBack->Weight; }
	if (EquippedHip) { Total += EquippedHip->Weight; }

	return Total;
}

float UZSInventoryComponent::GetMaxCarryWeight() const
{
	float MaxWeight = BaseCarryWeight;

	if (EquippedBack) { MaxWeight += EquippedBack->CarryCapacityBonus; }
	if (EquippedHip) { MaxWeight += EquippedHip->CarryCapacityBonus; }

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

UZSItemConfig* UZSInventoryComponent::GetEquippedItem(EZSEquipSlot Slot) const
{
	switch (Slot)
	{
	case EZSEquipSlot::Back: return EquippedBack;
	case EZSEquipSlot::Hip: return EquippedHip;
	default: return nullptr;
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

bool UZSInventoryComponent::Server_EquipToSlot(EZSEquipSlot Slot, UZSItemConfig* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Slot == EZSEquipSlot::None)
	{
		return false;
	}

	if (!Item->bIsEquippable || Item->EquipSlot != Slot)
	{
		return false;
	}

	if (Server_RemoveItem(Item, 1).IsEmpty())
	{
		// Not actually carried - nothing to equip.
		return false;
	}

	TObjectPtr<UZSItemConfig>& TargetRef = (Slot == EZSEquipSlot::Back) ? EquippedBack : EquippedHip;
	if (TargetRef)
	{
		// Whatever was previously equipped goes back to the carry list, not discarded.
		Server_AddItem(TargetRef, 1);
	}
	TargetRef = Item;

	OnRep_InventoryState();
	return true;
}

void UZSInventoryComponent::Server_UnequipSlot(EZSEquipSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || Slot == EZSEquipSlot::None)
	{
		return;
	}

	TObjectPtr<UZSItemConfig>& TargetRef = (Slot == EZSEquipSlot::Back) ? EquippedBack : EquippedHip;
	if (!TargetRef)
	{
		return;
	}

	Server_AddItem(TargetRef, 1);
	TargetRef = nullptr;

	OnRep_InventoryState();
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

void UZSInventoryComponent::OnRep_InventoryState()
{
	OnInventoryChanged.Broadcast();
}
