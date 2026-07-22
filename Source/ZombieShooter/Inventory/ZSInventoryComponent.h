// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ZSInventoryTypes.h"
#include "../Survival/ZSItemConfig.h"
#include "ZSInventoryComponent.generated.h"

/** Broadcast on every OnRep_ below - re-read the getters rather than diffing params, same "more than one thing can change in one server tick" reasoning as UZSHealthComponent::FZSOnBodyZonesChanged. No UI exists yet to bind this to - it's here so one exists once a UI does, per CLAUDE.md's replication convention ("never poll replicated state directly"). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnInventoryChanged);

/**
 *  P6 (Docs/GameDevPlan.md P6, Docs/Phases/P6_InventoryLoot.md): the general carry inventory -
 *  NOT P5's PrimaryHand/SecondaryHand/hotbar combat loadout slots (AZSPlayerCharacter's
 *  CurrentWeapon/HotbarSlots), which reference into this component's item data but are dispatched
 *  on by a different system (IA_Attack) for a different reason. This component owns two things:
 *
 *  1. A flat CarrySlots list (stacks of UZSItemConfig + count) - general loot, no grid/weight-
 *     independent-of-stacking complexity, matching the "roughly 1/3 of PZ's depth" pillar.
 *  2. Two equip slots (EZSEquipSlot::Back/Hip - GameDevPlan.md §7 P6, resolved 2026-07-21
 *     autonomously, dev unavailable to consult, flagged for review) for bags/clothing that grant
 *     a carry-capacity bonus (UZSItemConfig::CarryCapacityBonus) while worn - "equip-only vs.
 *     carry-only categories" per the phase file.
 *
 *  GetEncumbranceMultiplier() is the one accessor AZSPlayerCharacter wires into its movement
 *  speed (UpdateMovementSpeed, alongside UZSHealthComponent::GetMobilityMultiplier) - same
 *  "performance debuff first" philosophy as every other survival system in this project, not a
 *  hard weight cap that blocks picking things up.
 *
 *  Server-authoritative per CLAUDE.md's replication convention: every mutator is Server_-prefixed
 *  and gated by HasAuthority() (via GetOwner()->HasAuthority(), since UActorComponent itself has
 *  no HasAuthority() of its own), OnRep_ broadcasts FZSOnInventoryChanged, and this class also
 *  calls OnRep_ manually right after every authoritative mutation (OnRep never fires on the
 *  authoring machine itself - same pattern as every other component in this project).
 */
UCLASS(ClassGroup = (ZS), meta = (BlueprintSpawnableComponent))
class UZSInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UZSInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetCurrentWeight() const;

	/** BaseCarryWeight plus CarryCapacityBonus from whatever's equipped in Back/Hip. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetMaxCarryWeight() const;

	/** 1.0 at or under capacity; falls off linearly past it, bottoming out at MinEncumbranceMultiplier once weight reaches OverloadWeightRatio times capacity. Read by AZSPlayerCharacter::UpdateMovementSpeed - same multiplier-stacking pattern as UZSHealthComponent::GetMobilityMultiplier, not a hard block on carrying more. */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	float GetEncumbranceMultiplier() const;

	/** Returns a copy, not a reference - UFUNCTION-exposed container getters in this project return by value (matches how every other Blueprint-callable getter here works; not called per-tick, so the copy cost is irrelevant). */
	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	TArray<FZSInventorySlot> GetCarrySlots() const { return CarrySlots; }

	UFUNCTION(BlueprintPure, Category = "ZS|Inventory")
	UZSItemConfig* GetEquippedItem(EZSEquipSlot Slot) const;

	/** Server-authoritative: stacks onto existing partial CarrySlots entries first (respecting Item->MaxStackSize), then appends new slots for the remainder. Doesn't weight-check - a container hand-off or world pickup should still succeed content-wise; GetEncumbranceMultiplier already penalizes being overloaded instead of hard-blocking it (same philosophy as every other survival system here). No-op (returns 0) if Item is null, Count <= 0, or called off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 Server_AddItem(UZSItemConfig* Item, int32 Count);

	/** Server-authoritative: removes up to Count of Item from CarrySlots, emptiest-matching-stack-last (iterates back-to-front so partial stacks at the end get consumed before earlier ones - no gameplay significance to the order, just deterministic). Returns how many were actually removed (may be less than Count if fewer were carried). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	int32 Server_RemoveItem(UZSItemConfig* Item, int32 Count);

	/** Server-authoritative: moves one unit of Item (which the caller must already be carrying - same "caller already holds a direct reference" convention AZSPlayerCharacter::UseItem established in P2/P3) from CarrySlots into the given equip slot. Whatever was previously equipped there is returned to CarrySlots, not discarded. No-op (returns false) if Item->bIsEquippable is false, Item->EquipSlot doesn't match Slot, or Item isn't actually carried. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	bool Server_EquipToSlot(EZSEquipSlot Slot, UZSItemConfig* Item);

	/** Server-authoritative: returns whatever's equipped in Slot to CarrySlots and clears the slot. No-op if the slot is already empty. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_UnequipSlot(EZSEquipSlot Slot);

	/** Server-authoritative: removes up to Count of Item from CarrySlots and spawns an AZSWorldItemActor holding whatever was actually removed a short distance in front of the owning actor. No-op if nothing was actually carried to remove. "Dropped-item persistence in the running session" (GameDevPlan.md P6) means exactly this - a real replicated actor in the world, not a save-file-backed system (no save system exists yet, that's P7's). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Inventory")
	void Server_DropItem(UZSItemConfig* Item, int32 Count);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Inventory")
	FZSOnInventoryChanged OnInventoryChanged;

protected:

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "0"))
	float BaseCarryWeight = 25.f;

	/** Weight ratio (current / max) at which GetEncumbranceMultiplier bottoms out at MinEncumbranceMultiplier. 1.5 = 50% over capacity is as bad as it gets - no harder cap than that. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "1.01"))
	float OverloadWeightRatio = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "0", ClampMax = "1"))
	float MinEncumbranceMultiplier = 0.5f;

	/** How far in front of the owning actor Server_DropItem spawns the AZSWorldItemActor. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Inventory", meta = (ClampMin = "0"))
	float DropDistance = 100.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	TArray<FZSInventorySlot> CarrySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	TObjectPtr<UZSItemConfig> EquippedBack;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_InventoryState, Category = "ZS|Inventory")
	TObjectPtr<UZSItemConfig> EquippedHip;

	/** Shared OnRep for all three properties above - broadcasts FZSOnInventoryChanged. Manually called right after every authoritative mutation too (see class comment). */
	UFUNCTION()
	void OnRep_InventoryState();
};
