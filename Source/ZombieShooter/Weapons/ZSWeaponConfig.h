// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZSWeaponTypes.h"
#include "ZSWeaponConfig.generated.h"

class USkeletalMesh;
class UStaticMesh;
class UAnimMontage;
class UAnimSequence;
class UAnimSequenceBase;
class UBlendSpace;
class UBlendSpace1D;
class UAnimInstance;
class USoundBase;


/*
	Per-weapon data contract. Every weapon-facing system (AZSWeapon, AZSPlayerCharacter,
   the FP/TP AnimBPs, the AN_ZS_#1#ANS_ZS_* notify classes) reads from an instance of this
   class and never branches on which weapon is equipped. New weapon = new
   DA_ZS_WeaponConfig_<WeaponName> instance, never a new C++ branch (see CLAUDE.md's
   multi-weapon extensibility rule).
 
   Field groups mirror the Infima Tactical FPS Animations pack's own BP_TFA_BaseConfig
   (Docs/Infima Pack - Official Implementation Guide/02_Core_Data_Asset.md) so this class's
   fields can be cross-referenced 1:1 against the pack's reference content when populating
   DA_ZS_WeaponConfig_AssaultRifle. ABP_Weapon/ABP_Magazine are intentionally left unset by
   content (Phase 2 doesn't require weapon/magazine-mesh AnimBPs) but the fields exist for
   field-group completeness.
*/
 

UCLASS(BlueprintType)
class UZSWeaponConfig : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	// ---- Meshes / Classes ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> MeshReceiver;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> MeshMagazineSK;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshMagazine;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshBullet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshBulletCasing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshHandguard;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshScope;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshSightFront;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshSightRear;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshLaserAttachment;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshGripVertical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshGripAngled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<UStaticMesh> MeshSilencer;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> FP_Mesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TObjectPtr<USkeletalMesh> TP_Mesh;

	/** Left unset by content in Phase 2 — a mesh with no AnimClass still plays montages fine. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TSubclassOf<UAnimInstance> ABP_Weapon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Meshes")
	TSubclassOf<UAnimInstance> ABP_Magazine;

	// ---- Sockets ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketGunAttachment = TEXT("SocketGunAttachment");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketMagazineAttachment = TEXT("SocketMagazineAttachment");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketMagazineReserveAttachment = TEXT("SocketMagazineReserveAttachment");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketLaserAttachment = TEXT("SocketLaserAttachment");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketGripAttachment = TEXT("SocketGripAttachment");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketCasingEject = TEXT("SocketCasingEject");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketLaserStart = TEXT("SocketLaserStart");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketHandguard = TEXT("SocketHandguard");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketMuzzle = TEXT("SocketMuzzle");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketBulletChambered = TEXT("SocketBulletChambered");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketCasingJammed = TEXT("SocketCasingJammed");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketScope = TEXT("SocketScope");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketSightFront = TEXT("SocketSightFront");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketSightRear = TEXT("SocketSightRear");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketGunCamera = TEXT("SocketGunCamera");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketHelmetCamera = TEXT("SocketHelmetCamera");

	/**
	 *  Must default to a distinct value from SocketHelmetCamera — the Infima pack ships these
	 *  equal by default, which collapses Bodycam's helmet/chest variation into one angle.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sockets")
	FName SocketChestCamera = TEXT("SocketChestCamera");

	// ---- FP Character Montages ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_Equip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_FireSemi;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_FireAuto;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_FireEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_ReloadEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_ReloadQuick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_Inspect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_InspectEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_MagCheck;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_GrenadeThrowQuick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_FireMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_JumpFull;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Character Montages")
	TObjectPtr<UAnimMontage> FP_Randomization;

	// ---- TP Character Montages ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_Equip;

	/** Singular — the pack has no TP_FireSemi/TP_FireAuto split, unlike the FP set. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_FireEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_ReloadEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_ReloadQuick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_Inspect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_InspectEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_MagCheck;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_GrenadeThrowQuick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_FireMode;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Character Montages")
	TObjectPtr<UAnimMontage> TP_Randomization;

	// ---- FP Weapon-Mesh Montages ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_Equip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_ReloadEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_ReloadQuick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_Inspect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Weapon Montages")
	TObjectPtr<UAnimMontage> FP_WEP_MagCheck;

	// ---- TP Weapon-Mesh Montages ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_Equip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_Fire;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_Reload;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_ReloadEmpty;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_ReloadQuick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_Inspect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Weapon Montages")
	TObjectPtr<UAnimMontage> TP_WEP_MagCheck;

	// ---- FP Locomotion / Poses / Transitions ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UBlendSpace> FP_Locomotion_Standing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UBlendSpace> FP_Locomotion_Crouching;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UBlendSpace> FP_Locomotion_Aiming;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_RunLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_SprintLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_IdlePose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_AimPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_IdlePoseGripAngled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_IdlePoseGripVertical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_AimPoseGripAngled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_AimPoseGripVertical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_Transition_WalkEnd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_Transition_RunEnd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_Transition_CrouchStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FP Locomotion")
	TObjectPtr<UAnimSequence> FP_Transition_CrouchEnd;

	// ---- TP Poses / Transitions ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_IdleLoop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_IdlePose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_AimPose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_IdlePoseGripAngled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_IdlePoseGripVertical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_AimPoseGripAngled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_AimPoseGripVertical;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_Transition_AimStart;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TP Locomotion")
	TObjectPtr<UAnimSequenceBase> TP_Transition_AimEnd;

	// ---- Weapon-Side Poses ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Poses")
	TObjectPtr<UAnimSequence> WEP_ReferencePose;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Poses")
	TObjectPtr<UBlendSpace1D> WEP_FireModeStates;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon Poses")
	TObjectPtr<UBlendSpace1D> WEP_MagazineDepletion;

	// ---- Grouped / Randomized Arrays ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> FP_Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> TP_Melee;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> FP_Malfunctions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> TP_Malfunctions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> FP_WEP_Malfunctions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> TP_WEP_Malfunctions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> FP_Interactions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> TP_Interactions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> FP_Healing;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Randomized")
	TArray<TObjectPtr<UAnimMontage>> TP_Healing;

	// ---- Offsets ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Offsets")
	FTransform OffsetAimDownSights;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Offsets")
	FTransform OffsetCrouch;

	// ---- Cosmetic / Legacy ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	FName PrefixBulletSocket = TEXT("Bullet_");

	/** Cosmetic-only starting fill for the magazine's visual bullet count — NOT the real ammo source of truth. See MagazineCapacity/StartingReserveAmmo below. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cosmetic")
	int32 TotalAmmoCount = 0;

	// ---- Sounds ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds")
	TObjectPtr<USoundBase> SoundCue_WEP_MagDrop;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds")
	TObjectPtr<USoundBase> SoundCue_WEP_CasingEject;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Sounds")
	TObjectPtr<USoundBase> SoundCue_WEP_AimIn;

	// ---- Real Gameplay: Ammo (project addition — Infima's AmmoCount above is cosmetic-only) ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo", meta = (ClampMin = "1"))
	int32 MagazineCapacity = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 StartingReserveAmmo = 90;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ammo")
	int32 MaxReserveAmmo = 180;

	// ---- Real Gameplay: Fire Modes ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Modes")
	TArray<EZSFireMode> SupportedFireModes = { EZSFireMode::Semi, EZSFireMode::Auto };

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Fire Modes")
	float RoundsPerMinute = 600.f;

	// ---- Recoil Feel (per-weapon — see CoreLoopPlan.md Phase 2 §"Key architecture decisions") ----

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	FVector2D RecoilPitchRange = FVector2D(0.5f, 1.5f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	FVector2D RecoilYawRange = FVector2D(-0.5f, 0.5f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "1"))
	int32 RecoilRampMinShots = 5;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil", meta = (ClampMin = "1"))
	int32 RecoilRampMaxShots = 25;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Recoil")
	float RecoilRecoverySpeed = 22.f;
};
