// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSInventoryComponent.h"
#include "ZSWorldItemActor.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"

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

	for (const FZSInventorySlot& Slot : CarrySlots)
	{
		if (Slot.Item)
		{
			Total += Slot.Item->Weight * Slot.Count;
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
	for (FZSInventorySlot& Slot : CarrySlots)
	{
		if (Remaining <= 0)
		{
			break;
		}

		if (Slot.Item == Item && Slot.Count < StackSize)
		{
			const int32 ToAdd = FMath::Min(StackSize - Slot.Count, Remaining);
			Slot.Count += ToAdd;
			Remaining -= ToAdd;
		}
	}

	// New slots for whatever's left.
	while (Remaining > 0)
	{
		FZSInventorySlot NewSlot;
		NewSlot.Item = Item;
		NewSlot.Count = FMath::Min(Remaining, StackSize);
		CarrySlots.Add(NewSlot);
		Remaining -= NewSlot.Count;
	}

	OnRep_InventoryState();
	return Count;
}

int32 UZSInventoryComponent::Server_RemoveItem(UZSItemConfig* Item, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Item || Count <= 0)
	{
		return 0;
	}

	int32 Remaining = Count;

	for (int32 Index = CarrySlots.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		FZSInventorySlot& Slot = CarrySlots[Index];
		if (Slot.Item != Item)
		{
			continue;
		}

		const int32 ToRemove = FMath::Min(Slot.Count, Remaining);
		Slot.Count -= ToRemove;
		Remaining -= ToRemove;

		if (Slot.Count <= 0)
		{
			CarrySlots.RemoveAt(Index);
		}
	}

	const int32 ActuallyRemoved = Count - Remaining;
	if (ActuallyRemoved > 0)
	{
		OnRep_InventoryState();
	}
	return ActuallyRemoved;
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

	if (Server_RemoveItem(Item, 1) <= 0)
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

	const int32 Removed = Server_RemoveItem(Item, Count);
	if (Removed <= 0)
	{
		return;
	}

	const FVector DropLocation = OwnerActor->GetActorLocation() + OwnerActor->GetActorForwardVector() * DropDistance;

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	if (AZSWorldItemActor* WorldItem = GetWorld()->SpawnActor<AZSWorldItemActor>(AZSWorldItemActor::StaticClass(), DropLocation, OwnerActor->GetActorRotation(), SpawnParams))
	{
		WorldItem->InitializeItem(Item, Removed);
	}
}

void UZSInventoryComponent::OnRep_InventoryState()
{
	OnInventoryChanged.Broadcast();
}
