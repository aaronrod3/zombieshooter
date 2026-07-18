// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSMagazine.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UZSWeaponConfig;

/**
 *  Cosmetic magazine prop attached to a weapon (main and reserve). Not the source of truth
 *  for ammo count - AZSWeapon owns CurrentMagazineAmmo/CurrentReserveAmmo. This actor only
 *  represents the magazine visually, including per-round bullet meshes at sockets prefixed
 *  with the config's PrefixBulletSocket.
 */
UCLASS()
class AZSMagazine : public AActor
{
	GENERATED_BODY()

public:

	AZSMagazine();

	/** Assigns mesh/anim class from the config and spawns one bullet mesh per matching socket. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Magazine")
	void InitializeFromConfig(UZSWeaponConfig* Config);

	/** Marks MagazineMesh + every bullet mesh SetOwnerNoSee - see AZSWeapon::SetHiddenFromOwner, which calls this on the real weapon's MainMagazine/ReserveMagazine when it hides from the owner in FirstPerson view. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Magazine")
	void SetHiddenFromOwner(bool bHideFromOwner);

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> MagazineMesh;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Magazine")
	TObjectPtr<UZSWeaponConfig> WeaponConfig;

private:

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> BulletMeshComponents;
};
