// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ZSInteractableComponent.generated.h"

class AZSPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FZSOnInteracted, UZSInteractableComponent*, Interactable, AZSPlayerCharacter*, Interactor);

/**
 *  P1 interaction system v1 (Docs/GameDevPlan.md P1, Docs/Phases/P1_CameraControl.md): attach
 *  to any actor (door, container, generator switch, ...) to make it interactable. Detection and
 *  range are driven by AZSPlayerCharacter::UpdateNearestInteractable each tick, not by this
 *  component polling - keeps the interactable itself passive/cheap, matching how there could be
 *  many of these in a scene at once.
 *
 *  Server-authoritative per CLAUDE.md's replication convention: AZSPlayerCharacter::TryInteract
 *  is the client-callable entry point, routed through Server_Interact - OnInteract only actually
 *  runs with HasAuthority(). BlueprintNativeEvent so per-interactable-type Blueprints (a door, a
 *  loot container, a generator) can override behavior with zero C++ branching, same tech-stack
 *  convention as AZSPlayerCharacter/AZSWeapon's gameplay execution points.
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class UZSInteractableComponent : public USceneComponent
{
	GENERATED_BODY()

public:

	UZSInteractableComponent();

	/** Shown in the world prompt as "<bound key> - <Verb>", e.g. "F - Open". The key itself comes from IA_Interact's binding, not this component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Interaction")
	FText InteractionVerb = FText::FromString(TEXT("Interact"));

	/** How close a character's UpdateNearestInteractable sphere check needs to be to consider this component in range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Interaction")
	float InteractionRadius = 150.f;

	/** Toggle to temporarily disable (e.g. an already-looted container, a door that's already open) without destroying the component. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ZS|Interaction")
	bool bIsInteractable = true;

	UFUNCTION(BlueprintPure, Category = "ZS|Interaction")
	bool CanInteract() const { return bIsInteractable; }

	/** Gameplay execution point - override per interactable type in Blueprint (see CLAUDE.md's tech-stack convention). Only meaningfully runs server-side; see AZSPlayerCharacter::Server_Interact. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Interaction")
	void OnInteract(AZSPlayerCharacter* Interactor);
	virtual void OnInteract_Implementation(AZSPlayerCharacter* Interactor);

	/** Broadcast after a successful OnInteract - for Blueprint-side FX/sound/UI without a C++ override. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|Interaction")
	FZSOnInteracted OnInteracted;
};
