// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSHubSubsystem.generated.h"

class UZSVendorConfig;
class UZSItemConfig;
class APlayerState;

/** Broadcast whenever a specific player's Currency or Stash changes - lets a future hub/loadout-prep UI bind instead of polling, same replication-convention spirit as everywhere else in this project (this isn't itself a replicated property, since a GameInstanceSubsystem has no network identity of its own - see this class's own header comment). Carries the affected player so a UI bound to one player's own stash doesn't spuriously refresh when a different connected player's stash changes (OQ-BH-02, per-player stash). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnHubStateChanged, APlayerState*, Player);

/** One connected player's hub-side persistent state (OQ-BH-02, resolved 2026-08-28: per-player, not shared). */
USTRUCT()
struct FZSPlayerHubData
{
	GENERATED_BODY()

	UPROPERTY()
	int64 Currency = 0;

	UPROPERTY()
	TArray<FZSItemInstance> Stash;
};

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
 *  only for the lifetime of one running game instance. Same "content gap, no-op gracefully" pattern
 *  as every other not-yet-built system in this project - wire real save/load here once B3 exists.
 *
 *  Per-player stash (OQ-BH-02, resolved 2026-08-28): a hosted co-op raid gives each connected
 *  player their own separate stash/currency, not one shared pool. There is still exactly one
 *  UZSHubSubsystem per running game instance (the host's, in a listen-server game) - "per-player"
 *  means this one subsystem instance keys its state internally by APlayerState, not that each
 *  player gets a separate subsystem object. APlayerState is the right anchor (not APawn/AController)
 *  because it's this project's own existing "survives a pawn's death/respawn, tied to one connected
 *  player for the session" identity object - matches how AZSPlayerState is already described in
 *  CLAUDE.md's Framework/ section. Keyed by TWeakObjectPtr so a disconnected player's stale entry
 *  doesn't keep a destroyed PlayerState referenced; a reconnecting player currently gets a fresh
 *  entry (no real account/login system exists to recognize "the same player" across a reconnect -
 *  same limitation the lack of real disk persistence above already has).
 */
UCLASS()
class UZSHubSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	/** BlueprintPure (no exec pins) but deliberately not a const C++ method - a never-before-seen Player lazily gets their StartingCurrency entry created here on first touch (GetOrCreatePlayerData), same as every other function below, so "what's my balance" reads StartingCurrency correctly even before that player has done anything else at the hub yet, rather than reporting a misleading 0 until their first mutation. */
	UFUNCTION(BlueprintPure, Category = "ZS|Hub")
	int64 GetCurrency(APlayerState* Player);

	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void AddCurrency(APlayerState* Player, int64 Amount);

	/** False (no-op, nothing spent) if Amount > GetCurrency(Player) - spending never goes negative. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool TrySpendCurrency(APlayerState* Player, int64 Amount);

	/** Returns a copy, not a reference - matches every other Blueprint-exposed container getter in this project (e.g. UZSInventoryComponent::GetCarrySlots). Same lazy-seed-on-first-touch reasoning as GetCurrency above (an empty Stash needs no seeding, but routes through the same GetOrCreatePlayerData for one consistent lookup path, not two that could drift). */
	UFUNCTION(BlueprintPure, Category = "ZS|Hub")
	TArray<FZSItemInstance> GetStashContents(APlayerState* Player);

	/** BR's extraction entry point (AZSPlayerCharacter::Server_RequestExtraction): appends every extracted instance to Player's stash verbatim, preserving InstanceId/InstanceState - same identity-preservation rule as every other item transfer in this project (UZSInventoryComponent::Server_AddItemInstance, AZSWorldItemActor::InitializeFromInstance). No capacity limit yet - a real stash-size system is a BH scoping-pass question, not guessed here. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	void DepositItemsToStash(APlayerState* Player, const TArray<FZSItemInstance>& Items);

	/** Removes and returns Player's stash entry matching InstanceId - the "take this from the stash into my raid loadout" entry point a future hub/loadout-prep UI calls. False (OutItem left default, nothing removed) if not found. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool WithdrawFromStash(APlayerState* Player, FGuid InstanceId, FZSItemInstance& OutItem);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hub")
	FZSOnHubStateChanged OnHubStateChanged;

	// =====================================================================
	// BH-T2 (Docs/Beta/BH_HubHideoutEconomy.md): vendor economy - buying and selling both operate
	// on Player's stash directly (there's no separate "vendor inventory" to move items through
	// first), same "the hub has no in-between step" spirit as everywhere else in this subsystem.
	// =====================================================================

	/** Removes InstanceId from Player's stash and credits its sell value (UZSItemConfig::SellValue, scaled by StackCount if stackable or ConditionQuality if not, then by Vendor->BuyPriceMultiplier) - the reverse of DepositItemsToStash's "loot comes in," this is "loot goes out for currency." False (nothing changed) if InstanceId isn't in Player's stash, Vendor is null, or Vendor->WillBuyItem rejects the item's config (a zero SellValue or a specialized vendor's restriction list). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool SellStashItemToVendor(APlayerState* Player, FGuid InstanceId, const UZSVendorConfig* Vendor, int64& OutCurrencyEarned);

	/** Spends Count x the vendor's catalog price for Item (TrySpendCurrency's own insufficient-funds guard applies) and mints Count units into Player's stash, filling existing partial stacks first then minting new instances for the remainder - same fill-order as UZSInventoryComponent::Server_AddItem. False (nothing changed, currency not spent) if Vendor/Item are invalid, Item isn't in Vendor->SellCatalog, or currency is insufficient. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Hub")
	bool BuyItemFromVendor(APlayerState* Player, const UZSVendorConfig* Vendor, UZSItemConfig* Item, int32 Count, FZSItemInstance& OutPurchasedInstance);

protected:

	/** BH-T1.3: starting currency for a fresh save (OQ-BH-03) - a code default, not dev-specified, retune freely; add to TuningReference.md once a real number is chosen. Granted once per player, the first time that player's entry is created (GetOrCreatePlayerData), not once per game instance - see this class's own header comment on why per-player replaces the old per-instance Initialize()-time seed. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Hub", meta = (ClampMin = "0"))
	int64 StartingCurrency = 500;

	/** The one lookup path every function above routes through (getters included, see their own comments on why) - looks up Player's entry, creating and seeding one with StartingCurrency if this is the first time this player's been seen. Returns nullptr only for a null/invalid Player. */
	FZSPlayerHubData* GetOrCreatePlayerData(APlayerState* Player);

	UPROPERTY()
	TMap<TWeakObjectPtr<APlayerState>, FZSPlayerHubData> PerPlayerData;
};
