// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "../Inventory/ZSItemInstance.h"
#include "ZSPlayerState.generated.h"

class UZSVendorConfig;
class UZSItemConfig;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnCurrencyChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnStashChanged);

/**
 *  Per-player replicated data: health, ammo, kills. Health may move into a
 *  dedicated UZSHealthComponent once Phase 4 (Damage and Health) is reached —
 *  not decided yet, see docs/SessionHandoff.md.
 *
 *  BH (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27), moved here 2026-08-28: the
 *  persistent-across-character-death hub stash/currency (`Currency`/`Stash` below) used to live on
 *  `UZSHubSubsystem`, a `UGameInstanceSubsystem` - correct-by-accident for the listen-server host
 *  (whose own local UI and the server share one process) but silently WRONG for any other connected
 *  player, since a `UGameInstanceSubsystem`'s state never replicates across the network at all
 *  (`UGameInstance` is a per-process concept, not a networked one). `AZSPlayerState` is the right
 *  home instead: it already replicates to its owning connection (confirmed via engine source -
 *  `AController::InitPlayerState` sets `SpawnInfo.Owner = this`, so `PlayerState->GetNetConnection()`
 *  correctly chains through to the real client connection, which is also what makes the `Server`
 *  RPCs below correctly routable when called from the owning client, not just from other
 *  server-side code). Replicated `COND_OwnerOnly` - no other connected player needs to see this
 *  one's stash contents or balance.
 *
 *  Real disk persistence is NOT implemented here - no `UZSSaveGameSubsystem` exists anywhere in this
 *  project yet (B3 "Persistence & Save Backbone" hasn't started), so this is in-memory only for the
 *  lifetime of one connection (a reconnect currently gets a fresh `AZSPlayerState`, and with it a
 *  fresh `StartingCurrency` grant and an empty stash - no real account/login system exists to
 *  recognize "the same player" across a reconnect yet, same limitation the lack of disk persistence
 *  already implies).
 */
UCLASS()
class AZSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	AZSPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:

	virtual void BeginPlay() override;

public:

	// =====================================================================
	// BH: persistent hub stash & currency - see this class's own header comment for why it lives
	// here instead of UZSHubSubsystem.
	// =====================================================================

	UFUNCTION(BlueprintPure, Category = "ZS|Hub")
	int64 GetCurrency() const { return Currency; }

	/** Returns a copy, not a reference - matches every other Blueprint-exposed container getter in this project (e.g. UZSInventoryComponent::GetCarrySlots). */
	UFUNCTION(BlueprintPure, Category = "ZS|Hub")
	TArray<FZSItemInstance> GetStashContents() const { return Stash; }

	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ZS|Hub")
	void Server_AddCurrency(int64 Amount);

	/** No-op (nothing spent) if Amount > GetCurrency() - spending never goes negative. Server RPCs can't return a value - call this, then read GetCurrency() once the resulting OnRep_Currency/OnCurrencyChanged fires, same pattern UZSInventoryComponent::Server_StoreInBagChecked already established for this project. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ZS|Hub")
	void Server_TrySpendCurrency(int64 Amount);

	/** BR's extraction entry point (AZSPlayerCharacter::Server_RequestExtraction): appends every extracted instance to the stash verbatim, preserving InstanceId/InstanceState - same identity-preservation rule as every other item transfer in this project. No capacity limit yet - a real stash-size system is a BH scoping-pass question, not guessed here. Already-authoritative callers (Server_RequestExtraction included) can call this directly - a Server RPC executes its body immediately, with no network round-trip, when called from code that already has authority, same as every other Server-tagged function in this project. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ZS|Hub")
	void Server_DepositItemsToStash(const TArray<FZSItemInstance>& Items);

	/** Removes the stash entry matching InstanceId - the "take this from the stash into my raid loadout" entry point a future hub/loadout-prep UI calls. No-op (nothing removed) if not found - check GetStashContents() afterward for the result, same reasoning as Server_TrySpendCurrency above. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ZS|Hub")
	void Server_WithdrawFromStash(FGuid InstanceId);

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hub")
	FZSOnCurrencyChanged OnCurrencyChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Hub")
	FZSOnStashChanged OnStashChanged;

	// =====================================================================
	// BH-T2: vendor economy - buying and selling both operate on the stash directly (there's no
	// separate "vendor inventory" to move items through first).
	// =====================================================================

	/** Removes InstanceId from the stash and credits its sell value (UZSItemConfig::SellValue, scaled by StackCount if stackable or ConditionQuality if not, then by Vendor->BuyPriceMultiplier). No-op if InstanceId isn't in the stash, Vendor is null, or Vendor->WillBuyItem rejects the item's config (a zero SellValue or a specialized vendor's restriction list) - check GetCurrency()/GetStashContents() afterward for the result. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ZS|Hub")
	void Server_SellStashItemToVendor(FGuid InstanceId, UZSVendorConfig* Vendor);

	/** Spends Count x the vendor's catalog price for Item (Server_TrySpendCurrency's own insufficient-funds guard applies internally) and mints Count units into the stash, filling existing partial stacks first then minting new instances for the remainder - same fill-order as UZSInventoryComponent::Server_AddItem. No-op (nothing changed, currency not spent) if Vendor/Item are invalid, Item isn't in Vendor->SellCatalog, or currency is insufficient. */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "ZS|Hub")
	void Server_BuyItemFromVendor(UZSVendorConfig* Vendor, UZSItemConfig* Item, int32 Count);

protected:

	/** BH-T1.3: starting currency for a fresh save (OQ-BH-03) - a code default, not dev-specified, retune freely; add to TuningReference.md once a real number is chosen. Granted once, in BeginPlay. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|Hub", meta = (ClampMin = "0"))
	int64 StartingCurrency = 500;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Currency, Category = "ZS|Hub")
	int64 Currency = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_Stash, Category = "ZS|Hub")
	TArray<FZSItemInstance> Stash;

	UFUNCTION()
	void OnRep_Currency();

	UFUNCTION()
	void OnRep_Stash();
};
