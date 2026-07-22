// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSInventoryTypes.h"
#include "ZSContainerActor.generated.h"

class UStaticMeshComponent;
class UZSInteractableComponent;
class UZSLootTableConfig;
class AZSPlayerCharacter;

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): a lootable container (cabinet,
 *  crate, ...). Seeds ContainerSlots from LootTable at BeginPlay (server-only,
 *  UZSLootTableConfig::RollLoot). Reuses P1's UZSInteractableComponent rather than inventing a
 *  second interaction path - interacting transfers everything into the interactor's
 *  UZSInventoryComponent in one action ("loot all"), since no Inventory UI exists yet to let a
 *  player pick individual items out one at a time - a deliberate v1 bootstrap, same "direct
 *  action before UI exists" pattern P2/P3's item-use system used before any UI did.
 */
UCLASS()
class AZSContainerActor : public AActor
{
	GENERATED_BODY()

public:

	AZSContainerActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Returns a copy, not a reference - see UZSInventoryComponent::GetCarrySlots' comment for why. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	TArray<FZSInventorySlot> GetContainerSlots() const { return ContainerSlots; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ContainerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> InteractableComponent;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory")
	TObjectPtr<UZSLootTableConfig> LootTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ContainerSlots, Category = "ZS|Inventory")
	TArray<FZSInventorySlot> ContainerSlots;

	/** Keeps InteractableComponent->bIsInteractable in sync with whether there's anything left to loot - an empty container (looted, or LootTable rolled nothing) stops showing a prompt, per UZSInteractableComponent's own "already-looted container" example. */
	UFUNCTION()
	void OnRep_ContainerSlots();

	/** Bound to InteractableComponent->OnInteracted in BeginPlay - transfers everything in ContainerSlots into Interactor's UZSInventoryComponent (Server_AddItem always fully succeeds per its own contract - no weight-blocking/partial-transfer logic needed here) and empties ContainerSlots. Only meaningfully runs server-side - OnInteract itself only ever fires there, see AZSWorldItemActor::HandleInteracted for the same reasoning. */
	UFUNCTION()
	void HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);
};
