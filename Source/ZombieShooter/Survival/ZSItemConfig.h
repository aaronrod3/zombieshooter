// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSItemConfig.generated.h"

class UTexture2D;

/**
 *  Which action AZSPlayerCharacter::Server_UseItem routes an item to (P3, Docs/Phases/P3_HealthDamageMedical.md
 *  - extends P2's eat/drink-only item slice to cover the medical treatment actions). Zero C++
 *  branches per new item, same rule as weapons/needs: a new bandage/disinfectant/splint is a new
 *  DA_ZS_ItemConfig_<Name> instance with this enum set, never a new class.
 */
UENUM(BlueprintType)
enum class EZSItemUseType : uint8
{
	/** Routes to UZSNeedsComponent::Server_ConsumeItem (HungerRestore/ThirstRestore). */
	Consumable,
	/** Routes to UZSHealthComponent::Server_ApplyBandage(TargetZone, bIsCleanBandage). */
	Bandage,
	/** Routes to UZSHealthComponent::Server_Disinfect(TargetZone). */
	Disinfectant,
	/** Routes to UZSHealthComponent::Server_Splint(TargetZone). */
	Splint
};

/*
	P2's "items exist minimally" slice (Docs/GameDevPlan.md P2), extended in P3 to cover the
	medical treatment actions (Docs/Phases/P3_HealthDamageMedical.md: "bandage/disinfect/splint").
	Not a general inventory item yet - no weight, stacking, or equip-slot fields; those belong to
	P6's UZSInventoryComponent pass once a real inventory exists. New item = new
	DA_ZS_ItemConfig_<Name> instance, same one-data-asset-per-content-instance pattern as
	UZSWeaponConfig. AZSPlayerCharacter::Server_UseItem is the single dispatch point - it reads
	ItemUseType and routes to the right component, so neither NeedsComponent nor HealthComponent
	need to know about item data directly.
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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
	EZSItemUseType ItemUseType = EZSItemUseType::Consumable;

	/** Shown in the transparent stat-preview rule established here (GameDevPlan.md P2) - "eat this, gain +X Hunger" on hover, not a hidden number. Only meaningful when ItemUseType == Consumable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::Consumable"))
	float HungerRestore = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Consumable", meta = (ClampMin = "0", EditCondition = "ItemUseType == EZSItemUseType::Consumable"))
	float ThirstRestore = 0.f;

	/** Only meaningful when ItemUseType == Bandage - a clean bandage clears the wound's dirty flag too, a dirty rag stops the bleed but leaves it dirty. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Medical", meta = (EditCondition = "ItemUseType == EZSItemUseType::Bandage"))
	bool bIsCleanBandage = true;
};
