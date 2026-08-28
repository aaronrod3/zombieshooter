// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSExtractionPointActor.generated.h"

class UStaticMeshComponent;
class UZSInteractableComponent;
class AZSPlayerCharacter;

/**
 *  BR (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): a raid-zone extraction
 *  point - interact to end the raid successfully instead of by dying. Reuses UZSInteractableComponent
 *  exactly like AZSContainerActor (ZSContainerActor.h) - no new interaction path, same "OnInteract
 *  only ever meaningfully fires server-side" reasoning that class's own header documents.
 *
 *  All the actual consequence (banking carried loot to the hub stash instead of dropping it, then
 *  routing back to the hub) lives on AZSPlayerCharacter::Server_RequestExtraction - this class is
 *  just the world-placed trigger, same "thin trigger, real logic lives on the character/component"
 *  split ReviveInteractable already uses on AZSPlayerCharacter.
 */
UCLASS()
class AZSExtractionPointActor : public AActor
{
	GENERATED_BODY()

public:

	AZSExtractionPointActor();

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ExtractionMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> InteractableComponent;

	/** Bound to InteractableComponent->OnInteracted in BeginPlay - calls Interactor->Server_RequestExtraction(). */
	UFUNCTION()
	void HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);
};
