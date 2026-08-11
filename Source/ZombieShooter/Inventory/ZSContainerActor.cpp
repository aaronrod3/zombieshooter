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

		// OnRep_X never fires on the machine that has authority - apply directly here too, same
		// pattern as every other config-driven actor in this project.
		OnRep_ContainerSlots();
	}
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

	FZSItemInstance Removed;
	if (!Inventory->Server_RemoveInstanceByIdAnywhere(ItemInstanceId, Removed))
	{
		return false;
	}

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
