// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Survival/ZSItemConfig.h"
#include "ZSMagazineConfig.generated.h"

/**
 *  2026-08-11: a real, carryable, per-instance-stateful item - not the old flat reserve-ammo-pool
 *  reload (UZSWeaponConfig::AmmoItemConfig still exists and is unrelated; that's loose rounds, this
 *  is a loaded magazine). Compatibility with a weapon is by ammo type, not an explicit per-weapon
 *  link: a magazine fits any weapon whose UZSWeaponConfig::AmmoItemConfig matches this magazine's
 *  own CompatibleAmmoConfig - same "data classification, not a C++ branch" reasoning as everything
 *  else in this project. MaxStackSize should be authored as 1 (inherited from UZSItemConfig, not
 *  overridden here) - a magazine is stateful (FZSItemInstanceState::CurrentAmmoCount), and per
 *  FZSItemInstance's own invariant, stackable and stateful are mutually exclusive.
 */
UCLASS(BlueprintType)
class UZSMagazineConfig : public UZSItemConfig
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/** How many rounds this magazine type holds when full. A freshly-created instance (loot roll, debug grant) starts at this - see FZSItemInstanceState::CurrentAmmoCount's own comment for the lazy-resolve pattern. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magazine", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 30;

	/** Which loose-ammo UZSItemConfig this magazine's caliber matches - a magazine is legal to load into a weapon iff this equals that weapon's own UZSWeaponConfig::AmmoItemConfig. Unset means this magazine fits nothing (same "unset = content gap, not a crash" pattern as every other required-but-optional reference in this project). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Magazine")
	TObjectPtr<UZSItemConfig> CompatibleAmmoConfig;
};
