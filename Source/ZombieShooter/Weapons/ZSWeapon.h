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

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnConfigChanged, UZSWeaponConfig*, NewConfig);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnFireModeChanged, EZSFireMode, NewFireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnAmmoChanged, int32, NewAmmo);

/**
 *  The equipped weapon actor. One replicated instance per equipped weapon, permanently
 *  attached to the character's body mesh at Config->SocketGunAttachment. Every field this
 *  class reads comes from CurrentConfig; no C++ branch is specific to any one weapon
 *  (multi-weapon extensibility rule, CLAUDE.md).
 */
UCLASS()
class AZSWeapon : public AActor
{
	GENERATED_BODY()

public:

	AZSWeapon();

	/** Phase 3: server-authoritative state replicated to all clients - see CoreLoopPlan.md's Phase 3 state classification. */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Seeds ammo/fire-mode state and assembles cosmetics from the config. Call right after SpawnActor - not BeginPlay, since SpawnActor invokes BeginPlay synchronously once the world has already begun play (the common case for EquipWeapon), which left this config-dependent setup unreachable when it lived in BeginPlay. Server-only (Phase 3) - gated by HasAuthority(); clients assemble the same cosmetics from OnRep_CurrentConfig instead. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void InitializeFromConfig(UZSWeaponConfig* Config);

	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	AZSMagazine* SpawnMagazine(FName SocketName);

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool CanFire() const;

	/** Decrements CurrentMagazineAmmo by one. Returns false (no-op) if the magazine is already empty or if called off a non-authoritative machine (Phase 3). Named Server_ per CLAUDE.md's convention - the one plain (non-BlueprintNativeEvent) mutator touching replicated ammo state. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	bool Server_ConsumeAmmoRound();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool CanReload() const;

	/** P5: durability-lite ("melee breaks, no repair sim v1" - GameDevPlan.md). Decrements CurrentDurability by one landed hit; returns true if that hit broke it (reached 0). No-op / always returns false for an unbreakable weapon (CurrentConfig->MaxDurabilityHits == 0, e.g. every gun) or off a non-authoritative machine. Called by AZSPlayerCharacter::Server_WeaponMeleeAttack after a landed swing - this class only tracks the number, the caller decides what "broken" means for the loadout (auto-unequip). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	bool Server_ConsumeDurabilityHit();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	int32 GetCurrentDurability() const { return CurrentDurability; }

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
	EZSFireMode GetCurrentFireMode() const { return CurrentFireMode; }

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnConfigChanged OnConfigChanged;

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

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentFireMode, Category = "ZS|Weapon")
	EZSFireMode CurrentFireMode = EZSFireMode::Semi;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentMagazineAmmo, Category = "ZS|Weapon")
	int32 CurrentMagazineAmmo = 0;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentReserveAmmo, Category = "ZS|Weapon")
	int32 CurrentReserveAmmo = 0;

	/** P5: seeded from CurrentConfig->MaxDurabilityHits in InitializeFromConfig. Stays at 0 (and Server_ConsumeDurabilityHit stays a no-op) for an unbreakable weapon. No OnRep needed - nothing client-side reacts to durability yet (no UI), same "plain replicated state" reasoning as AZSGameState::UtilitiesShutoffDay. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ZS|Weapon")
	int32 CurrentDurability = 0;

	/** Runs the same cosmetic assembly InitializeFromConfig() runs server-side - this is what makes a client's local proxy of this replicated actor actually look right, since clients never call InitializeFromConfig() themselves. Also refreshes the owning character's body mesh (Owner replicates natively). */
	UFUNCTION()
	void OnRep_CurrentConfig();

	UFUNCTION()
	void OnRep_CurrentFireMode();

	UFUNCTION()
	void OnRep_CurrentMagazineAmmo();

	UFUNCTION()
	void OnRep_CurrentReserveAmmo();

	/** Cosmetic-only assembly from CurrentConfig: receiver mesh, static-mesh attachments, magazine prop. Called from InitializeFromConfig on the server and OnRep_CurrentConfig on clients - each machine assembles its own local, unreplicated cosmetics. */
	void AssembleCosmeticsFromConfig();

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
	TObjectPtr<AZSMagazine> MainMagazine;
};
