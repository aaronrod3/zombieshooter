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
}

void AZSContainerActor::HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	// Temporary verification logging for B0-T1 Stage G re-test - remove once a world-prompt widget
	// exists and failures here are visible without the log (same note as Server_Fire).
	if (!HasAuthority() || !Interactor || ContainerSlots.Num() == 0)
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: HandleInteracted rejected - HasAuthority=%d Interactor=%s ContainerSlots=%d"),
			*GetName(), HasAuthority(), *GetNameSafe(Interactor), ContainerSlots.Num());
		return;
	}

	UZSInventoryComponent* Inventory = Interactor->GetInventoryComponent();
	if (!Inventory)
	{
		UE_LOG(LogZombieShooter, Log, TEXT("%s: HandleInteracted rejected - %s has no InventoryComponent"), *GetName(), *Interactor->GetName());
		return;
	}

	int32 ItemsTransferred = 0;
	for (const FZSItemInstance& Instance : ContainerSlots)
	{
		if (Instance.IsValid() && Instance.StackCount > 0)
		{
			Inventory->Server_AddItemInstance(Instance);
			++ItemsTransferred;

			// Temporary GUID logging for B0-T2 Checkpoint A - remove alongside the rest of this
			// session's verification logging (B0-T5.5).
			UE_LOG(LogZombieShooter, Log, TEXT("%s: transferred %s x%d, InstanceId %s"),
				*GetName(), *Instance.Config->DisplayName.ToString(), Instance.StackCount, *Instance.InstanceId.ToString());
		}
	}

	UE_LOG(LogZombieShooter, Log, TEXT("%s: loot-all transferred %d slot(s) to %s"), *GetName(), ItemsTransferred, *Interactor->GetName());

	ContainerSlots.Empty();
	OnRep_ContainerSlots();
}
