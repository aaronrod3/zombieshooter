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

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnConfigChanged, UZSWeaponConfig*, NewConfig);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnGripChanged, EZSGripAttachment, NewGrip);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnFireModeChanged, EZSFireMode, NewFireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnAmmoChanged, int32, NewAmmo);

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

	/** Phase 3: server-authoritative state replicated to all clients - see CoreLoopPlan.md's Phase 3 state classification. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Assembles the receiver mesh, optional attachment parts, grip, starting ammo, and magazines from the config. Call right after SpawnActor - not BeginPlay, since SpawnActor invokes BeginPlay synchronously once the world has already begun play (the common case for EquipWeapon), which left this config-dependent setup unreachable when it lived in BeginPlay. Server-only (Phase 3) - gated by HasAuthority(); clients assemble the same cosmetics from OnRep_CurrentConfig instead. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void InitializeFromConfig(UZSWeaponConfig* Config);

	/** Phase 3: initializes this instance as a purely cosmetic, never-replicated FirstPerson-only
	 *  visual twin of the real (replicated) weapon - see AZSPlayerCharacter::RefreshFirstPersonWeaponVisual.
	 *  Sets bIsFirstPersonVisual, disables replication, and assembles only the receiver mesh +
	 *  static-mesh cosmetics + initial grip (not magazines/laser - those would need their own
	 *  owner-only-visibility support on AZSMagazine/AZSLaserAttachment, deferred; see
	 *  CoreLoopPlan.md's Phase 3 notes on this fix). Call immediately after SpawnActor. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void InitializeAsFirstPersonVisual(UZSWeaponConfig* Config);

	/** Marks every visual component (receiver + static-mesh cosmetics) SetOnlyOwnerSee - used by
	 *  the FirstPerson-only visual twin so it never renders for any client but this pawn's own
	 *  owner. No-op on components that don't currently exist (e.g. no grip attachment yet). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void SetOwnerOnlyVisible(bool bOwnerOnly);

	/** Marks every visual component (receiver + static-mesh cosmetics) SetOwnerNoSee - used on the
	 *  real weapon so it hides from this pawn's own owner specifically whenever GetMesh() itself
	 *  does (FirstPerson view - see AZSPlayerCharacter::AttachWeaponToActiveMesh), while staying
	 *  visible to every other client. Deliberately does not extend to MainMagazine/
	 *  ReserveMagazine/LaserAttachment (would need their own SetOwnerNoSee support) - a known,
	 *  documented gap, not an oversight; explicit non-recursive component list only. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void SetHiddenFromOwner(bool bHideFromOwner);

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

	/** Decrements CurrentMagazineAmmo by one. Returns false (no-op) if the magazine is already empty or if called off a non-authoritative machine (Phase 3). Named Server_ per CLAUDE.md's convention - the one plain (non-BlueprintNativeEvent) mutator touching replicated ammo state. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	bool Server_ConsumeAmmoRound();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool CanReload() const;

	/** Transfers ammo reserve -> magazine synchronously. The reload montage that follows is purely cosmetic (see CoreLoopPlan.md Phase 2 "Key architecture decisions"). Gameplay execution point - overridable per-weapon. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void PerformReload();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void CycleFireMode();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon", meta = (BlueprintThreadSafe))
	UZSWeaponConfig* GetConfig() const { return CurrentConfig; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	USkeletalMeshComponent* GetReceiverMesh() const { return SK_Receiver; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	EZSGripAttachment GetCurrentGrip() const { return CurrentGrip; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	EZSFireMode GetCurrentFireMode() const { return CurrentFireMode; }

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnConfigChanged OnConfigChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnGripChanged OnGripChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnFireModeChanged OnFireModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnAmmoChanged OnMagazineAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnAmmoChanged OnReserveAmmoChanged;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SK_Receiver;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentConfig, Category = "ZS|Weapon")
	TObjectPtr<UZSWeaponConfig> CurrentConfig;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentGrip, Category = "ZS|Weapon")
	EZSGripAttachment CurrentGrip = EZSGripAttachment::None;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentFireMode, Category = "ZS|Weapon")
	EZSFireMode CurrentFireMode = EZSFireMode::Semi;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentMagazineAmmo, Category = "ZS|Weapon")
	int32 CurrentMagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentReserveAmmo, Category = "ZS|Weapon")
	int32 CurrentReserveAmmo = 0;

	/** Runs the same cosmetic assembly InitializeFromConfig() runs server-side (static mesh attachments, magazine/laser spawns, initial grip) - this is what makes a client's local proxy of this replicated actor actually look right, since clients never call InitializeFromConfig() themselves. Also refreshes the owning character's body meshes (Owner replicates natively). */
	UFUNCTION()
	void OnRep_CurrentConfig();

	UFUNCTION()
	void OnRep_CurrentGrip();

	UFUNCTION()
	void OnRep_CurrentFireMode();

	UFUNCTION()
	void OnRep_CurrentMagazineAmmo();

	UFUNCTION()
	void OnRep_CurrentReserveAmmo();

	/** Cosmetic-only assembly from CurrentConfig - the full set (receiver/static cosmetics/grip plus magazines/laser). Factored out of InitializeFromConfig so OnRep_CurrentConfig can run the identical visuals-only path on clients. */
	void AssembleCosmeticsFromConfig();

	/** Receiver mesh, anim class, static-mesh attachments (handguard/silencer/scope/sights), and initial grip - the subset InitializeAsFirstPersonVisual uses (no magazines/laser, see that function's comment). */
	void AssembleReceiverCosmetics();

	/** Magazine spawns + reserve-hidden + laser attachment spawn - the other half of AssembleCosmeticsFromConfig, split out so the FirstPerson-only visual twin can skip it. */
	void AssembleMagazinesAndLaser();

	/** Visual-only tail of SetGripAttachment_Implementation, factored out so OnRep_CurrentGrip can re-run it on clients without re-triggering the full BlueprintNativeEvent. Re-applies SetOwnerOnlyVisible if bIsFirstPersonVisual, since GripMesh may be created lazily on first non-None grip - after the initial InitializeAsFirstPersonVisual pass. */
	void UpdateGripVisual();

	/** True only for the FirstPerson-only cosmetic twin AZSPlayerCharacter spawns locally per-machine - see InitializeAsFirstPersonVisual. Never true for the real, replicated weapon. */
	bool bIsFirstPersonVisual = false;

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
