// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSHubSubsystem.generated.h"

class UZSVendorConfig;
class UZSItemConfig;

/** Broadcast whenever Currency or Stash changes - lets a future hub/loadout-prep UI bind instead of polling, same replication-convention spirit as everywhere else in this project (this isn't itself a replicated property, since a GameInstanceSubsystem has no network identity of its own - see this class's own header comment). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnHubStateChanged);

/**
 *  BH (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): the persistent-across-
 *  character-death half of the new hub-and-raid loop - secure stash + currency. On a character's
 *  death its skills/XP/carried gear are gone for good (BR's job); this subsystem is what survives
 *  that regardless. Deliberately a GameInstanceSubsystem, same "the one object that survives a
 *  level travel" reasoning UZSGameInstance's own header comment already gives for itself - a
 *  character's pawn/PlayerState are destroyed on death, this never is.
 *
 *  Real disk persistence is NOT implemented here - no UZSSaveGameSubsystem exists anywhere in this
 *  project yet (B3 "Persistence & Save Backbone" hasn't started), so everything below is in-memory
 *  only for the lifetime of one running game instance. Same "content gap, no-op gracefully"
 *  pattern as every other not-yet-built system in this project (e.g. AZombieCharacter::ZombieConfig
 *  unset, UZSZombieConfig::BehaviorTree unset) - wire real save/load here once B3 exists rather
 *  than guessing a save-file format blind.
 *
 *  Per-player vs. shared-world stash: deliberately per-GAME-INSTANCE (there's exactly one of these
 *  per running instance - the host's, in a listen-server game), not per-connecting-client - matches
 *  the "listen-server-host-owns-the-save" convention Docs/Beta/00_MasterPlan.md's CR-07 already
 *  established for the pre-pivot world save. Whether a shared hosted raid should eventually give
 *  each connected player their own separate stash (rather than every caller on this machine
 *  sharing the one below) is an open BH scoping-pass question, not resolved here - correct as-is
 *  for the initial local-save-only launch target and for a solo host.
 */
UCLASS()
class UZSHubSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** Seeds Currency from StartingCurrency - the one-time "a fresh game instance/save starts with this much money" grant. Real persistence (so this only ever applies once per save, not once per launch) is B3's job - see this class's own header comment. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UFUNCTION(BlueprintPure, Category = "ZS|Hub")
	int64 GetCurrency() const { return Currency; }

	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void AddCurrency(int64 Amount);

	/** False (no-op, nothing spent) if Amount > GetCurrency() - spending never goes negative. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool TrySpendCurrency(int64 Amount);

	/** Returns a copy, not a reference - matches every other Blueprint-exposed container getter in this project (e.g. UZSInventoryComponent::GetCarrySlots). */
	UFUNCTION(BlueprintPure, Category = "ZS|Hub")
	TArray<FZSItemInstance> GetStashContents() const { return Stash; }

	/** BR's extraction entry point (AZSPlayerCharacter::Server_RequestExtraction): appends every extracted instance to the stash verbatim, preserving InstanceId/InstanceState - same identity-preservation rule as every other item transfer in this project (UZSInventoryComponent::Server_AddItemInstance, AZSWorldItemActor::InitializeFromInstance). No capacity limit yet - a real stash-size system is a BH scoping-pass question, not guessed here. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void DepositItemsToStash(const TArray<FZSItemInstance>& Items);

	/** Removes and returns the stash entry matching InstanceId - the "take this from the stash into my raid loadout" entry point a future hub/loadout-prep UI calls. False (OutItem left default, nothing removed) if not found. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool WithdrawFromStash(FGuid InstanceId, FZSItemInstance& OutItem);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hub")
	FZSOnHubStateChanged OnHubStateChanged;

	// =====================================================================
	// BH-T2 (Docs/Beta/BH_HubHideoutEconomy.md): vendor economy - buying and selling both operate
	// on the stash directly (there's no separate "vendor inventory" to move items through first),
	// same "the hub has no in-between step" spirit as everywhere else in this subsystem.
	// =====================================================================

	/** Removes InstanceId from the stash and credits its sell value (UZSItemConfig::SellValue, scaled by StackCount if stackable or ConditionQuality if not, then by Vendor->BuyPriceMultiplier) - the reverse of DepositItemsToStash's "loot comes in," this is "loot goes out for currency." False (nothing changed) if InstanceId isn't in the stash, Vendor is null, or Vendor->WillBuyItem rejects the item's config (a zero SellValue or a specialized vendor's restriction list). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool SellStashItemToVendor(FGuid InstanceId, const UZSVendorConfig* Vendor, int64& OutCurrencyEarned);

	/** Spends Count × the vendor's catalog price for Item (TrySpendCurrency's own insufficient-funds guard applies) and mints Count units into the stash, filling existing partial stacks first then minting new instances for the remainder - same fill-order as UZSInventoryComponent::Server_AddItem. False (nothing changed, currency not spent) if Vendor/Item are invalid, Item isn't in Vendor->SellCatalog, or currency is insufficient. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool BuyItemFromVendor(const UZSVendorConfig* Vendor, UZSItemConfig* Item, int32 Count, FZSItemInstance& OutPurchasedInstance);

protected:

	/** BH-T1.3: starting currency for a fresh save (OQ-BH-03) - a code default, not dev-specified, retune freely; add to TuningReference.md once a real number is chosen. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Hub", meta = (ClampMin = "0"))
	int64 StartingCurrency = 500;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|Hub")
	int64 Currency = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ZS|Hub")
	TArray<FZSItemInstance> Stash;
};
