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
 *  All" UI button) - the v1 bootstrap this class used to only offer.
 *
 *  2026-08-05, resolved (dev call): interacting with a container opens the real WBP_ZS_ContainerLoot
 *  screen (HandleInteracted -> AZSPlayerCharacter::Client_OpenContainerLoot) rather than
 *  auto-looting everything - the open UX question the design session originally left unresolved
 *  (Docs/Planning/B1_UIDesignSession_2026-07-30.md). Server_TakeAllItems is unchanged and still
 *  reachable - it's what the loot screen's own "Take All" button calls.
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

	/** B1-T6.2/T6.3: finds InstanceId in ContainerSlots by GUID-exact match, removes it, and hands it to Requester's UZSInventoryComponent - one atomic server-authoritative call, so two players racing for the same item resolve correctly (the second call's lookup simply fails once the first has already removed it - no separate locking needed, RPCs execute serially on the server). Returns false (no-op) if InstanceId isn't currently in ContainerSlots, Requester has no inventory, or called off a non-authoritative machine. 2026-08-06: also returns false (item stays in the container, nothing removed) if the transfer itself is rejected - UZSInventoryComponent::Server_AddItemInstance now rejects a full-Pockets add outright, so this only removes from ContainerSlots once the add has actually succeeded. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_TakeItem(FGuid InstanceId, AZSPlayerCharacter* Requester);

	/** B1-T6.2: the "Take All" convenience button's entry point - same transfer HandleInteracted has always done, now named and reusable. No-op off a non-authoritative machine or if Requester has no inventory. 2026-08-06: only removes whichever items actually transferred - if Pockets fills up partway through, whatever didn't fit is left behind in ContainerSlots (still lootable) rather than silently destroyed. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_TakeAllItems(AZSPlayerCharacter* Requester);

	/** Adds Instance directly to ContainerSlots, bypassing LootTable - for a hand-placed guaranteed item (e.g. a quest-relevant pickup a designer wants in a specific container, not left to a random roll). No-op (false) if Instance is invalid or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_AddItemToContainer(FZSItemInstance Instance);

	/** 2026-08-09, dev-confirmed (container deposit system): the reverse of Server_TakeItem - removes ItemInstanceId from Depositor's own inventory (wherever it currently is - Pockets, or nested in an equipped bag; rejects a mounted/equipped item, since a deposit is meant to remove it from the inventory entirely, see Server_RemoveInstanceByIdAnywhere's own comment) and adds it to ContainerSlots, preserving its identity/InstanceState. Unlike a person-carried bag (Server_StoreInBag), there's no weapon-type exclusion here - this is exactly the "weapons can be brought to an external container" case. No-op (false, nothing moved) if ItemInstanceId isn't found in Depositor's inventory, Depositor has no inventory component, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_DepositItem(FGuid ItemInstanceId, AZSPlayerCharacter* Depositor);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Inventory")
	FZSOnContainerSlotsChanged OnContainerSlotsChanged;

	/** 2026-08-11: fixed grid dimensions for this container's loot display - EditDefaultsOnly so a
	 *  per-archetype Blueprint (BP_ZS_Container_Kitchen, etc.) can size its own container differently,
	 *  same "per-instance data asset/Blueprint, not a C++ branch" spirit as everything else in this
	 *  project. Mirrors UZSInventoryComponent::GetCompartmentCapacity's fixed-capacity pattern, just
	 *  per-actor-instance instead of per-EZSCarryLocation since a container has only one storage area. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory")
	int32 GridColumns = 6;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory")
	int32 GridRows = 4;

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	int32 GetGridColumns() const { return GridColumns; }

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	int32 GetGridRows() const { return GridRows; }

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	int32 GetContainerCapacity() const { return FMath::Max(GridColumns * GridRows, 0); }

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

private:

	/** 2026-08-11: first free grid cell in [0, GetContainerCapacity()), or INDEX_NONE if full - same pattern as UZSInventoryComponent::FindFirstFreeSlotIndex, kept separate rather than shared since the two classes have no common base to hang a shared helper off. */
	int32 FindFirstFreeSlotIndex() const;
};
