// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSItemInstance.h"
#include "ZSWorldItemActor.generated.h"

class UZSItemConfig;
class UZSInteractableComponent;
class UStaticMeshComponent;
class AZSPlayerCharacter;

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): a physical, interactable,
 *  pickup-able world representation of an inventory stack - spawned by
 *  UZSInventoryComponent::Server_DropItem, or placed directly in a level for hand-authored loot.
 *  "Dropped-item persistence in the running session" means exactly this: a real replicated actor
 *  sitting in the world, not a save-file-backed system (no save system exists yet - that's P7's).
 *
 *  Reuses P1's UZSInteractableComponent rather than inventing a second interaction path -
 *  HandleInteracted is bound to its OnInteracted delegate in BeginPlay (fires server-side only,
 *  same as every other interactable in this project - see UZSInteractableComponent's own doc
 *  comment), adds ItemInstance to the interactor's UZSInventoryComponent (preserving its
 *  InstanceId/InstanceState - see ZSItemInstance.h), and destroys this actor.
 */
UCLASS()
class AZSWorldItemActor : public AActor
{
	GENERATED_BODY()

public:

	AZSWorldItemActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-only: mints a fresh FZSItemInstance for InItem/InCount and assembles the cosmetic mesh - the hand-authored/placed-loot path (CLAUDE.md's "Blueprint subclass calling InitializeItem in BeginPlay" pattern for pre-populated pickups), where there's no pre-existing instance to preserve. Delegates to InitializeFromInstance. Safe to call right after SpawnActor - mirrors AZSWeapon::InitializeFromConfig's "call right after spawn, not BeginPlay" reasoning. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void InitializeItem(UZSItemConfig* InItem, int32 InCount);

	/** Server-only: as above, but for the drop/relocate path where an instance already exists (UZSInventoryComponent::Server_DropItem) and its InstanceId/InstanceState (durability, condition) must survive the round trip - B0-T2's headline exit criterion. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void InitializeFromInstance(const FZSItemInstance& InInstance);

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	const FZSItemInstance& GetItemInstance() const { return ItemInstance; }

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	UZSItemConfig* GetItem() const { return ItemInstance.Config; }

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	int32 GetCount() const { return ItemInstance.StackCount; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> InteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_ItemInstance, Category = "ZS|Inventory")
	FZSItemInstance ItemInstance;

	/** Cosmetic mesh assembly + interaction-prompt text from ItemInstance - called from InitializeFromInstance on the server (OnRep never fires on the authoring machine itself) and via replication on clients. */
	UFUNCTION()
	void OnRep_ItemInstance();

	/** Bound to InteractableComponent->OnInteracted in BeginPlay. Only meaningfully runs server-side - OnInteract itself (and therefore this broadcast) only ever fires on the server, per AZSPlayerCharacter::Server_Interact/UZSInteractableComponent's own doc comment - so no HasAuthority() gate is needed on the binding itself, only inside the handler as a defensive check. */
	UFUNCTION()
	void HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);
};
