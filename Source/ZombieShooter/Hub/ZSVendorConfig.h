// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSVendorConfig.generated.h"

class UZSItemConfig;

/** One entry in a vendor's sell catalog - what it offers the player, and at what price. Not the same number as UZSItemConfig::SellValue (a buy-back rate) - a vendor's markup is its own authored choice per item, not derived automatically. */
USTRUCT(BlueprintType)
struct FZSVendorCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Vendor")
	TObjectPtr<UZSItemConfig> Item = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Vendor", meta = (ClampMin = "0"))
	int64 Price = 0;
};

/**
 *  BH (Docs/Beta/00_MasterPlan.md CR-13, extraction pivot 2026-08-27): per-vendor data contract -
 *  same multi-config rule as UZSZombieConfig/UZSWeaponConfig ("N vendors, zero C++ branches"). A new
 *  vendor archetype (a general trader vs. a specialized gun/medical vendor) is a new
 *  DA_ZS_VendorConfig_<Name> instance, never a new C++ branch.
 *
 *  Buying (the vendor sells to the player) and selling (the vendor buys from the player) are
 *  deliberately asymmetric, matching how a real shop works: SellCatalog is an explicit, authored
 *  list with its own per-item price; buying-from-the-player instead reads UZSItemConfig::SellValue
 *  (a property of the item itself, not the vendor) scaled by BuyPriceMultiplier, so any sellable
 *  item works with any vendor by default without needing to be listed twice.
 */
UCLASS(BlueprintType)
class UZSVendorConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Vendor")
	FText VendorName;

	/** What this vendor sells to the player - UZSHubSubsystem::BuyItemFromVendor reads this. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Vendor")
	TArray<FZSVendorCatalogEntry> SellCatalog;

	/** Fraction of UZSItemConfig::SellValue this vendor pays when buying an item from the player - 0.5 means half of full-condition value, same "vendor markup exists" shape a real shop has. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Vendor", meta = (ClampMin = "0", ClampMax = "1"))
	float BuyPriceMultiplier = 0.5f;

	/** Empty (the default) = this vendor buys anything with a nonzero SellValue. Non-empty = a specialized vendor that only buys these specific configs - e.g. a gun vendor that won't buy medical supplies. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Vendor")
	TArray<TObjectPtr<UZSItemConfig>> BuyRestrictedTo;

	/** False for a null Item, a zero/negative SellValue, or (if BuyRestrictedTo is non-empty) an Item not on that list. */
	UFUNCTION(BlueprintPure, Category = "ZS|Vendor")
	bool WillBuyItem(const UZSItemConfig* Item) const;
};
