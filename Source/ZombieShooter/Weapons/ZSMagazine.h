// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSMagazine.generated.h"

class UStaticMeshComponent;
class UZSWeaponConfig;

/**
 *  Cosmetic magazine prop attached to the weapon. Not the source of truth for ammo count -
 *  AZSWeapon owns CurrentMagazineAmmo/CurrentReserveAmmo. This actor only represents the
 *  magazine visually. Never replicated - each machine spawns its own from
 *  AZSWeapon's cosmetic assembly. Static mesh (2026-07-22, moved off Infima's skeletal
 *  magazine prop alongside the rest of AZSWeapon's static-mesh assembly).
 */
UCLASS()
class AZSMagazine : public AActor
{
	GENERATED_BODY()

public:

	AZSMagazine();

	/** Assigns the magazine mesh from the config. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Magazine")
	void InitializeFromConfig(UZSWeaponConfig* Config);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> MagazineMesh;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Magazine")
	TObjectPtr<UZSWeaponConfig> WeaponConfig;
};
