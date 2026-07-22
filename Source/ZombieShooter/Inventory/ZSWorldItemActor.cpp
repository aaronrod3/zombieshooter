// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWorldItemActor.h"
#include "ZSInventoryComponent.h"
#include "../Survival/ZSItemConfig.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Player/ZSPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

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

	DOREPLIFETIME(AZSWorldItemActor, Item);
	DOREPLIFETIME(AZSWorldItemActor, Count);
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

	Item = InItem;
	Count = InCount;

	// OnRep_X never fires on the machine that has authority - apply the cosmetic assembly
	// directly here too, same pattern as every other config-driven actor in this project.
	OnRep_Item();
}

void AZSWorldItemActor::OnRep_Item()
{
	if (!Item)
	{
		return;
	}

	if (PickupMesh && Item->WorldMesh)
	{
		PickupMesh->SetStaticMesh(Item->WorldMesh);
	}

	if (InteractableComponent)
	{
		InteractableComponent->InteractionVerb = FText::FromString(FString::Printf(TEXT("Pick up %s x%d"), *Item->DisplayName.ToString(), Count));
	}
}

void AZSWorldItemActor::HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	if (!HasAuthority() || !Interactor || !Item)
	{
		return;
	}

	if (UZSInventoryComponent* Inventory = Interactor->GetInventoryComponent())
	{
		Inventory->Server_AddItem(Item, Count);
	}

	Destroy();
}
