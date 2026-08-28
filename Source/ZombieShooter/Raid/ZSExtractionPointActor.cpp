// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSExtractionPointActor.h"
#include "../Interaction/ZSInteractableComponent.h"
#include "../Player/ZSPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"

AZSExtractionPointActor::AZSExtractionPointActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	ExtractionMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ExtractionMesh"));
	SetRootComponent(ExtractionMesh);

	InteractableComponent = CreateDefaultSubobject<UZSInteractableComponent>(TEXT("InteractableComponent"));
	InteractableComponent->SetupAttachment(ExtractionMesh);
	InteractableComponent->InteractionVerb = FText::FromString(TEXT("Extract"));
}

void AZSExtractionPointActor::BeginPlay()
{
	Super::BeginPlay();

	if (InteractableComponent)
	{
		InteractableComponent->OnInteracted.AddDynamic(this, &AZSExtractionPointActor::HandleInteracted);
	}
}

void AZSExtractionPointActor::HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor)
{
	if (Interactor)
	{
		Interactor->Server_RequestExtraction();
	}
}
