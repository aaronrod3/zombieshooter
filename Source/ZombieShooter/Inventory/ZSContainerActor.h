// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSItemInstance.h"
#include "ZSContainerActor.generated.h"

class UStaticMeshComponent;
class UZSInteractableComponent;
class UZSLootTableConfig;
class AZSPlayerCharacter;

/** B1-T6: fires whenever ContainerSlots changes for any reason (a take, a take-all, initial roll) - a T6 loot-screen widget binds this instead of polling GetContainerSlots(), per T2.2's convention. Re-read GetContainerSlots() rather than diffing params, same "more than one thing can change in one tick" reasoning as other *SlotsChanged delegates in this project. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnContainerSlotsChanged);

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): a lootable container (cabinet,
 *  crate, ...). Seeds ContainerSlots from LootTable at BeginPlay (server-only,
 *  UZSLootTableConfig::RollLoot). Reuses P1's UZSInteractableComponent rather than inventing a
 *  second interaction path.
 *
 *  B1-T6.2, 2026-08-01: per-item take (Server_TakeItem) now exists alongside the original "loot
 *  all" bulk transfer (renamed Server_TakeAllItems, same behavior, now reusable by a future "Take
 *  All" UI button) - the v1 bootstrap this class used to only offer. HandleInteracted (bound to
 *  interact) still calls Server_TakeAllItems unchanged for now: whether interacting with a
 *  container should keep auto-looting everything or instead open a real two-pane loot screen is
 *  an open UX question the design session explicitly left unresolved (Docs/Planning/
 *  B1_UIDesignSession_2026-07-30.md: "container-loot screen was not mocked up this session") -
 *  not guessed past here, see B1_UI_UX.md's Manual setup steps.
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
	TArray<FZSItemInstance> GetContainerSlots() const { return ContainerSlots; }

	/** B1-T6.2/T6.3: finds InstanceId in ContainerSlots by GUID-exact match, removes it, and hands it to Requester's UZSInventoryComponent - one atomic server-authoritative call, so two players racing for the same item resolve correctly (the second call's lookup simply fails once the first has already removed it - no separate locking needed, RPCs execute serially on the server). Returns false (no-op) if InstanceId isn't currently in ContainerSlots, Requester has no inventory, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_TakeItem(FGuid InstanceId, AZSPlayerCharacter* Requester);

	/** B1-T6.2: the "Take All" convenience button's entry point - same transfer HandleInteracted has always done, now named and reusable. No-op off a non-authoritative machine or if Requester has no inventory. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_TakeAllItems(AZSPlayerCharacter* Requester);

	/** Adds Instance directly to ContainerSlots, bypassing LootTable - for a hand-placed guaranteed item (e.g. a quest-relevant pickup a designer wants in a specific container, not left to a random roll). No-op (false) if Instance is invalid or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_AddItemToContainer(FZSItemInstance Instance);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Inventory")
	FZSOnContainerSlotsChanged OnContainerSlotsChanged;

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ContainerMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> InteractableComponent;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory")
	TObjectPtr<UZSLootTableConfig> LootTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ContainerSlots, Category = "ZS|Inventory")
	TArray<FZSItemInstance> ContainerSlots;

	/** Keeps InteractableComponent->bIsInteractable in sync with whether there's anything left to loot, and broadcasts OnContainerSlotsChanged for a T6 loot-screen widget. */
	UFUNCTION()
	void OnRep_ContainerSlots();

	/** Bound to InteractableComponent->OnInteracted in BeginPlay - calls Server_TakeAllItems(Interactor). Only meaningfully runs server-side - OnInteract itself only ever fires there, see AZSWorldItemActor::HandleInteracted for the same reasoning. */
	UFUNCTION()
	void HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);
};
