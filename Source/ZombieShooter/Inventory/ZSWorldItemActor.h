// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
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
 *  comment), adds Item/Count to the interactor's UZSInventoryComponent, and destroys this actor.
 */
UCLASS()
class AZSWorldItemActor : public AActor
{
	GENERATED_BODY()

public:

	AZSWorldItemActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Server-only: sets the replicated Item/Count this pickup represents and assembles its cosmetic mesh. Safe to call right after SpawnActor - mirrors AZSWeapon::InitializeFromConfig's "call right after spawn, not BeginPlay" reasoning. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void InitializeItem(UZSItemConfig* InItem, int32 InCount);

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	UZSItemConfig* GetItem() const { return Item; }

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	int32 GetCount() const { return Count; }

protected:

	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PickupMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UZSInteractableComponent> InteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Item, Category = "ZS|Inventory")
	TObjectPtr<UZSItemConfig> Item;

	/** No OnRep needed - always set in the same InitializeItem call as Item, and nothing client-side reacts to Count on its own (the interaction prompt text is assembled from Item + Count together in OnRep_Item). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "ZS|Inventory")
	int32 Count = 0;

	/** Cosmetic mesh assembly + interaction-prompt text from Item - called from InitializeItem on the server (OnRep never fires on the authoring machine itself) and via replication on clients. */
	UFUNCTION()
	void OnRep_Item();

	/** Bound to InteractableComponent->OnInteracted in BeginPlay. Only meaningfully runs server-side - OnInteract itself (and therefore this broadcast) only ever fires on the server, per AZSPlayerCharacter::Server_Interact/UZSInteractableComponent's own doc comment - so no HasAuthority() gate is needed on the binding itself, only inside the handler as a defensive check. */
	UFUNCTION()
	void HandleInteracted(UZSInteractableComponent* Interactable, AZSPlayerCharacter* Interactor);
};
