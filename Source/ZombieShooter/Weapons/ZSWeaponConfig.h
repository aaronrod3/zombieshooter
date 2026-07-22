// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSWeaponTypes.h"
#include "ZSWeaponConfig.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UAnimMontage;
class UAnimSequenceBase;
class AZSWeapon;
class UDamageType;

/*
	Per-weapon data contract. Every weapon-facing system (AZSWeapon, AZSPlayerCharacter,
   the TP AnimBP, the AN_ZS_/ANS_ZS_ notify classes) reads from an instance of this class and
   never branches on which weapon is equipped. New weapon = new DA_ZS_WeaponConfig_<WeaponName>
   instance, never a new C++ branch (see CLAUDE.md's multi-weapon extensibility rule).

   Slimmed hard in the P0 de-scope (Docs/GameDevPlan.md section 2): the original Infima-mirroring
   field catalog (~90 fields: FP montage/locomotion sets, weapon-mesh montages, grip/laser/casing
   cosmetics, procedural spring offsets, gun/body camera sockets) is in git history if a future
   phase resurrects any of it. What remains is what the third-person survival game actually
   consumes today. New animation fields get added per the standard-animation plan in
   Docs/GameDevPlan.md, not by restoring the old catalog wholesale.
*/

UCLASS(BlueprintType)
class UZSWeaponConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	/**
	 *  Which AZSWeapon (sub)class AZSPlayerCharacter::EquipWeapon spawns for this config. Left
	 *  unset by default (falls back to plain AZSWeapon) - only needs setting if a specific
	 *  weapon wants its own Blueprint child overriding one of AZSWeapon's BlueprintNativeEvent
	 *  functions (PerformReload, CycleFireMode) without touching C++. See CLAUDE.md's tech
	 *  stack convention.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Class")
	TSubclassOf<AZSWeapon> WeaponClass;

	// ---- Meshes ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> MeshReceiver;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> MeshMagazineSK;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshHandguard;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshScope;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshSightFront;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshSightRear;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshSilencer;

	/** The character body mesh used while this weapon is equipped. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> TP_Mesh;

	// ---- Sockets ----

	/** On the character body mesh - where the weapon actor attaches. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketGunAttachment = TEXT("SocketGunAttachment");

	/** On the receiver mesh - where the magazine prop attaches. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketMagazineAttachment = TEXT("SocketMagazineAttachment");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketHandguard = TEXT("SocketHandguard");

	/** Also the future muzzle-flash/trace origin once firing gets real ballistics (P1+). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketMuzzle = TEXT("SocketMuzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketScope = TEXT("SocketScope");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketSightFront = TEXT("SocketSightFront");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketSightRear = TEXT("SocketSightRear");

	// ---- Montages (third-person character mesh) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
	TObjectPtr<UAnimMontage> TP_Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Montages")
	TObjectPtr<UAnimMontage> TP_Reload;

	// ---- Poses (consumed by the TP AnimBP's dynamic pins) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poses")
	TObjectPtr<UAnimSequenceBase> TP_IdleLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poses")
	TObjectPtr<UAnimSequenceBase> TP_IdlePose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Poses")
	TObjectPtr<UAnimSequenceBase> TP_AimPose;

	// ---- Ammo (real gameplay state - lives on AZSWeapon, seeded from here) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 StartingReserveAmmo = 90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 MaxReserveAmmo = 180;

	// ---- Attack dispatch (P5) ----

	/** Which behavior IA_Attack triggers while this is equipped - AZSPlayerCharacter::HandleAttack reads this instead of a weapon/melee item having its own separate input. Defaults to Ranged since every config authored before P5 is a gun. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack Dispatch")
	EZSAttackType AttackType = EZSAttackType::Ranged;

	// ---- Loadout (P5: real-time hotbar, AZSPlayerCharacter::Server_SelectHotbarSlot) ----

	/** How long switching the hotbar to this weapon takes (SetBusy(true) for the duration, same choreography pattern as reload) - GameDevPlan.md P5's "equip/holster/switch takes real time, not instant". No separate holster-then-equip phases yet (v1 simplification, see Docs/Phases/P5_CombatCompletion.md) - this single delay covers both directions. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loadout", meta = (ClampMin = "0"))
	float EquipTimeSeconds = 0.75f;

	// ---- Fire Modes ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Modes")
	TArray<EZSFireMode> SupportedFireModes = { EZSFireMode::Semi, EZSFireMode::Auto };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Modes")
	float RoundsPerMinute = 600.f;

	/** P4: how far a shot's noise event (UZSNoiseSystem::ReportNoise, called from AZSPlayerCharacter::Server_Fire) reaches - "every loud act reports a noise event with a radius" (GameDevPlan.md P4). Per-weapon, not a global constant: a shotgun should carry further than a suppressed pistol without a C++ branch. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Noise", meta = (ClampMin = "0"))
	float FireNoiseRadius = 3000.f;

	// ---- Damage (P4: hitscan fire damage, Server_Fire) ----

	/** Per-shot hitscan damage, applied via UGameplayStatics::ApplyPointDamage. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0"))
	float FireDamage = 25.f;

	/** Hitscan trace distance from the weapon's SocketMuzzle (falls back to eye height if the socket isn't found). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage", meta = (ClampMin = "0"))
	float FireRange = 5000.f;

	/** Which EZSWoundType a hit applies to a player target, via AZSPlayerCharacter::WoundTypeFromDamageTypeClass (ZSDamageTypes.h). Unset falls back to UZSDamageType_Laceration at the call site (Server_Fire) - same "unset = generic marker" pattern as UZSZombieConfig::AttackDamageTypeClass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Damage")
	TSubclassOf<UDamageType> FireDamageTypeClass;
};
