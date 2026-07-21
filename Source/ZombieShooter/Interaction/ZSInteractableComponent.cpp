// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSInteractableComponent.h"

UZSInteractableComponent::UZSInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UZSInteractableComponent::OnInteract_Implementation(AZSPlayerCharacter* Interactor)
{
	OnInteracted.Broadcast(this, Interactor);
}
