// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSMagazine.generated.h"

class USkeletalMeshComponent;
class UZSWeaponConfig;

/**
 *  Cosmetic magazine prop attached to the weapon. Not the source of truth for ammo count -
 *  AZSWeapon owns CurrentMagazineAmmo/CurrentReserveAmmo. This actor only represents the
 *  magazine visually. Never replicated - each machine spawns its own from
 *  AZSWeapon's cosmetic assembly.
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
	TObjectPtr<USkeletalMeshComponent> MagazineMesh;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Magazine")
	TObjectPtr<UZSWeaponConfig> WeaponConfig;
};
