// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWorldItemActor.h"
#include "ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Player/ZSPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"
#include "../ZombieShooter.h"

AZSWorldItemActor::AZSWorldItemActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Never moves independently once spawned/placed (no physics-drop-and-settle in v1) - same
	// "no absolute position/velocity that ever needs replicating" reasoning as AZSWeapon.
	SetReplicateMovement(false);

	PickupMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PickupMesh"));
	SetRootComponent(PickupMesh);

	InteractableComponent = CreateDefaultSubobject<UZSInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->SetupAttachment(PickupMesh);
	InteractableComponent->InteractionVerb = FText::FromString(TEXT("Pick up"));
}

void AZSWorldItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSWorldItemActor, ItemInstance);
}

void AZSWorldItemActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteracted.AddDynamic(this, &AZSWorldItemActor::HandleInteracted);
	}
}

void AZSWorldItemActor::InitializeItem(UZSItemConfig* InItem, int32 InCount)
{
	if (!HasAuthority())
	{
		return;
	}

	FZSItemInstance NewInstance;
	NewInstance.InstanceId = FGuid::NewGuid();
	NewInstance.Config = InItem;
	NewInstance.StackCount = InCount;
	InitializeFromInstance(NewInstance);
}

void AZSWorldItemActor::InitializeFromInstance(const FZSItemInstance& InInstance)
{
	if (!HasAuthority())
	{
		return;
	}

	ItemInstance = InInstance;

	// OnRep_X never fires on the machine that has authority - apply the cosmetic assembly
	// directly here too, same pattern as every other config-driven actor in this project.
	OnRep_ItemInstance();
}

void AZSWorldItemActor::OnRep_ItemInstance()
{
	if (!ItemInstance.Config)
	{
		return;
	}

	if (PickupMesh && ItemInstance.Config->WorldMesh)
	{
		PickupMesh->SetStaticMesh(ItemInstance.Config->WorldMesh);
	}

	if (InteractableComponent)
	{
		InteractableComponent->InteractionVerb = FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *ItemInstance.Config->DisplayName.ToString(), ItemInstance.StackCount));
	}
}

void AZSWorldItemActor::HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	// Temporary verification logging for B0-T1 Stage G re-test - remove once a world-prompt widget
	// exists and failures here are visible without the log (same note as Server_Fire).
	if (!HasAuthority() || !Interactor || !ItemInstance.IsValid())
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: pickup rejected - Interactor=%s Item=%s"), *GetName(), *GetNameSafe(Interactor), *GetNameSafe(ItemInstance.Config));
		return;
	}

	if (UZSInventoryComponent* Inventory = Interactor->GetInventoryComponent())
	{
		// 2026-08-06, dev-confirmed: a full Pockets (OnPerson) compartment rejects the pickup outright
		// - Server_AddItemInstance returns false and mutates nothing, so the world item must stay put
		// rather than being destroyed out from under a pickup that never actually happened.
		if (!Inventory->Server_AddItemInstance(ItemInstance))
		{
			UE_LOG(LogZombieShooter, Log, TEXT("%s: pickup rejected - %s's Pockets are full"), *Interactor->GetName(), *ItemInstance.Config->DisplayName.ToString());
			return;
		}

		// InstanceId logged here (B0-T2 Checkpoint A) so a dropped-then-repicked-up item's GUID can
		// be diffed against the one Server_DropItem logged when it was dropped.
		UE_LOG(LogZombieShooter, Log, TEXT("%s: picked up %s x%d, InstanceId %s"),
			*Interactor->GetName(), *ItemInstance.Config->DisplayName.ToString(), ItemInstance.StackCount, *ItemInstance.InstanceId.ToString());

		// 2026-08-09, dev-confirmed: a picked-up weapon auto-mounts, same "no separate manual-equip
		// step" spirit as the container-bearing-gear auto-equip above it. World-pickup path only,
		// deliberately - a container-loot take stays drag-and-drop only. No-op for a non-weapon item.
		Interactor->Server_TryAutoMountWeapon(ItemInstance.InstanceId);
	}

	Destroy();
}
