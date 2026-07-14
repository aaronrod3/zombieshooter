// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSWeaponTypes.h"
#include "ZSWeapon.generated.h"

class USkeletalMeshComponent;
class UStaticMeshComponent;
class UStaticMesh;
class UZSWeaponConfig;
class AZSMagazine;
class AZSPhysicsMagazine;
class AZSPhysicsCasing;
class AZSLaserAttachment;

/**
 *  The equipped weapon actor. One instance per equipped weapon - re-parented between the
 *  character's first-person and third-person mesh components on perspective switch (see
 *  AZSPlayerCharacter), never duplicated. Every field this class reads comes from
 *  CurrentConfig; no C++ branch is specific to any one weapon (multi-weapon extensibility
 *  rule, CLAUDE.md).
 */
UCLASS()
class AZSWeapon : public AActor
{
	GENERATED_BODY()

public:

	AZSWeapon();

	/** Assembles the receiver mesh, optional attachment parts, grip, starting ammo, and magazines from the config. Call right after SpawnActor - not BeginPlay, since SpawnActor invokes BeginPlay synchronously once the world has already begun play (the common case for EquipWeapon), which left this config-dependent setup unreachable when it lived in BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void InitializeFromConfig(UZSWeaponConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	AZSMagazine* SpawnMagazine(FName SocketName);

	/** Socket-driven, not name-driven: toggles whichever magazine (main or reserve) this actor spawned at that role. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void SetMagazineVisibility(bool bVisible, bool bIsReserve);

	/** Gameplay execution point - overridable in a per-weapon Blueprint child (see CLAUDE.md's tech stack convention) to add drop VFX/behavior without a C++ recompile. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	AZSPhysicsMagazine* SpawnDroppedMagazine(float ImpulseForce, float RotationForce);

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	AZSPhysicsCasing* EjectCasing(FRotator RotationOffset, float MinEjectForce, float MaxEjectForce, float RotationSpeed);

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void SetGripAttachment(EZSGripAttachment NewGrip);

	/** Cycles CurrentGrip: None -> Vertical -> Angled -> None. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void RandomizeGripAttachment();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool CanFire() const;

	/** Decrements CurrentMagazineAmmo by one. Returns false (no-op) if the magazine is already empty. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	bool ConsumeAmmoRound();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool CanReload() const;

	/** Transfers ammo reserve -> magazine synchronously. The reload montage that follows is purely cosmetic (see CoreLoopPlan.md Phase 2 "Key architecture decisions"). Gameplay execution point - overridable per-weapon. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void PerformReload();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void CycleFireMode();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	UZSWeaponConfig* GetConfig() const { return CurrentConfig; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	USkeletalMeshComponent* GetReceiverMesh() const { return SK_Receiver; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	EZSGripAttachment GetCurrentGrip() const { return CurrentGrip; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	EZSFireMode GetCurrentFireMode() const { return CurrentFireMode; }

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SK_Receiver;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Weapon")
	TObjectPtr<UZSWeaponConfig> CurrentConfig;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Weapon")
	EZSGripAttachment CurrentGrip = EZSGripAttachment::None;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Weapon")
	EZSFireMode CurrentFireMode = EZSFireMode::Semi;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Weapon")
	int32 CurrentMagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Weapon")
	int32 CurrentReserveAmmo = 0;

private:

	UStaticMeshComponent* AssignNewStaticMesh(const FName& SocketName, UStaticMesh* Mesh, const FName& ComponentName);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HandguardMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> SilencerMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> ScopeMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> FrontSightMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> RearSightMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GripMesh;

	UPROPERTY()
	TObjectPtr<AZSMagazine> MainMagazine;

	UPROPERTY()
	TObjectPtr<AZSMagazine> ReserveMagazine;

	UPROPERTY()
	TObjectPtr<AZSLaserAttachment> LaserAttachment;
};
