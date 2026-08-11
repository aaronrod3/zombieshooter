// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSContainerActor.h"
#include "ZSInventoryComponent.h"
#include "ZSLootTableConfig.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Player/ZSPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "../ZombieShooter.h"

AZSContainerActor::AZSContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	ContainerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ContainerMesh"));
	SetRootComponent(ContainerMesh);

	InteractableComponent = CreateDefaultSubobject<UZSInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->SetupAttachment(ContainerMesh);
	InteractableComponent->InteractionVerb = FText::FromString(TEXT("Loot"));
}

void AZSContainerActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSContainerActor, ContainerSlots);
}

void AZSContainerActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteracted.AddDynamic(this, &AZSContainerActor::HandleInteracted);
	}

	if (HasAuthority() && LootTable)
	{
		ContainerSlots = LootTable->RollLoot(GetWorld());

		// 2026-08-11: RollLoot doesn't know about grid capacity (it's a UZSLootTableConfig concern-free
		// roll), so slots are assigned here - sequential, since the container starts genuinely empty.
		// A NumRolls that exceeds GetContainerCapacity() leaves the overflow at INDEX_NONE (won't
		// render, won't crash) rather than silently dropping loot - logged so it reads as a content
		// mistake (NumRolls too high for this archetype's grid) rather than a mystery.
		const int32 Capacity = GetContainerCapacity();
		for (int32 Index = 0; Index < ContainerSlots.Num(); ++Index)
		{
			if (Index < Capacity)
			{
				ContainerSlots[Index].SlotIndex = Index;
			}
			else
			{
				UE_LOG(LogZombieShooter, Warning, TEXT("%s: BeginPlay rolled %d item(s) but the grid only holds %d - %d won't display, raise GridColumns/GridRows or LootTable->NumRolls"),
					*GetName(), ContainerSlots.Num(), Capacity, ContainerSlots.Num() - Capacity);
				break;
			}
		}

		// OnRep_X never fires on the machine that has authority - apply directly here too, same
		// pattern as every other config-driven actor in this project.
		OnRep_ContainerSlots();
	}
}

int32 AZSContainerActor::FindFirstFreeSlotIndex() const
{
	TArray<int32> Occupied;
	Occupied.Reserve(ContainerSlots.Num());
	for (const FZSItemInstance& Instance : ContainerSlots)
	{
		Occupied.Add(Instance.SlotIndex);
	}

	const int32 Capacity = GetContainerCapacity();
	for (int32 Index = 0; Index < Capacity; ++Index)
	{
		if (!Occupied.Contains(Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

void AZSContainerActor::OnRep_ContainerSlots()
{
	if (InteractableComponent)
	{
		InteractableComponent->bIsInteractable = ContainerSlots.Num() > 0;
	}

	OnContainerSlotsChanged.Broadcast();
}

bool AZSContainerActor::Server_TakeItem(FGuid InstanceId, AZSPlayerCharacter* Requester)
{
	if (!HasAuthority() || !Requester || !InstanceId.IsValid())
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Requester->GetInventoryComponent();
	if (!Inventory)
	{
		return false;
	}

	const int32 Index = ContainerSlots.IndexOfByPredicate([InstanceId](const FZSItemInstance& Instance) { return Instance.InstanceId == InstanceId; });
	if (Index == INDEX_NONE)
	{
		// Already taken by someone else (or never existed) - this IS the dupe-safety guarantee, not
		// a failure worth logging every time two players race for the same item.
		return false;
	}

	// 2026-08-06: don't remove from ContainerSlots until the transfer into the requester's own
	// Pockets actually succeeds - Server_AddItemInstance now rejects a full-Pockets add outright, and
	// removing first would silently destroy the item rather than leaving it in the container.
	if (!Inventory->Server_AddItemInstance(ContainerSlots[Index]))
	{
		return false;
	}

	ContainerSlots.RemoveAt(Index);
	OnRep_ContainerSlots();

	return true;
}

void AZSContainerActor::Server_TakeAllItems(AZSPlayerCharacter* Requester)
{
	if (!HasAuthority() || !Requester)
	{
		return;
	}

	UZSInventoryComponent* Inventory = Requester->GetInventoryComponent();
	if (!Inventory)
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: Server_TakeAllItems rejected - %s has no InventoryComponent"), *GetName(), *Requester->GetName());
		return;
	}

	// 2026-08-06: only remove what actually transferred - Server_AddItemInstance now rejects a
	// full-Pockets add outright, and removing unconditionally would silently destroy whatever didn't
	// fit instead of leaving it behind, still lootable. Iterates back-to-front so RemoveAt doesn't
	// invalidate the indices still to be visited.
	int32 ItemsTransferred = 0;
	for (int32 Index = ContainerSlots.Num() - 1; Index >= 0; --Index)
	{
		const FZSItemInstance& Instance = ContainerSlots[Index];
		if (Instance.IsValid() && Instance.StackCount > 0 && Inventory->Server_AddItemInstance(Instance))
		{
			ContainerSlots.RemoveAt(Index);
			++ItemsTransferred;
		}
	}

	UE_LOG(LogZombieShooter, Log, TEXT("%s: take-all transferred %d slot(s) to %s"), *GetName(), ItemsTransferred, *Requester->GetName());

	OnRep_ContainerSlots();
}

bool AZSContainerActor::Server_AddItemToContainer(FZSItemInstance Instance)
{
	if (!HasAuthority() || !Instance.IsValid())
	{
		return false;
	}

	const int32 FreeIndex = FindFirstFreeSlotIndex();
	if (FreeIndex == INDEX_NONE)
	{
		return false;
	}

	Instance.SlotIndex = FreeIndex;
	ContainerSlots.Add(Instance);
	OnRep_ContainerSlots();

	return true;
}

bool AZSContainerActor::Server_DepositItem(FGuid ItemInstanceId, AZSPlayerCharacter* Depositor)
{
	if (!HasAuthority() || !Depositor || !ItemInstanceId.IsValid())
	{
		return false;
	}

	UZSInventoryComponent* Inventory = Depositor->GetInventoryComponent();
	if (!Inventory)
	{
		return false;
	}

	// 2026-08-11: capacity checked before anything is removed from the depositor's own inventory -
	// same atomic-reject pattern as Server_AddItemInstance's Pockets check. Removing first and only
	// then discovering the container's full would strand the item in neither place.
	const int32 FreeIndex = FindFirstFreeSlotIndex();
	if (FreeIndex == INDEX_NONE)
	{
		return false;
	}

	FZSItemInstance Removed;
	if (!Inventory->Server_RemoveInstanceByIdAnywhere(ItemInstanceId, Removed))
	{
		return false;
	}

	Removed.SlotIndex = FreeIndex;
	ContainerSlots.Add(Removed);
	OnRep_ContainerSlots();
	return true;
}

void AZSContainerActor::HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	if (!HasAuthority() || !Interactor || ContainerSlots.Num() == 0)
	{
		return;
	}

	// 2026-08-05: resolved (real loot screen, not auto-loot-all) - opens WBP_ZS_ContainerLoot on the
	// interacting client instead of transferring everything server-side. Server_TakeAllItems is
	// unchanged and still reachable - it's what the loot screen's own "Take All" button calls, via
	// AZSPlayerCharacter::Server_TakeAllContainerItems.
	Interactor->Client_OpenContainerLoot(this);
}
