// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSItemConfig.generated.h"

class UTexture2D;

/*
	P2's "items exist minimally" slice (Docs/GameDevPlan.md P2): just enough of an item data
	contract to eat/drink through UZSNeedsComponent::Server_ConsumeItem. Not a general inventory
	item yet - no weight, stacking, or equip-slot fields; those belong to P6's UZSInventoryComponent
	pass once a real inventory exists. New consumable = new DA_ZS_ItemConfig_<Name> instance, same
	one-data-asset-per-content-instance pattern as UZSWeaponConfig.
*/

UCLASS(BlueprintType)
class UZSItemConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UTexture2D> Icon;

	/** Shown in the transparent stat-preview rule established here (GameDevPlan.md P2) - "eat this, gain +X Hunger" on hover, not a hidden number. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0"))
	float HungerRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0"))
	float ThirstRestore = 0.f;

	/** True for a real consumable (food/drink); false reserves this config for a future non-consumable item type without a separate class yet. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable")
	bool bIsConsumable = true;
};
