// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSWeaponTypes.h"
#include "ZSWeapon.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UZSWeaponConfig;
class AZSMagazine;

/** Broadcast by every Phase 3 OnRep_X on this class - lets Blueprint/UI/AnimGraph bind to state changes instead of polling, per CLAUDE.md's replication convention. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnConfigChanged, UZSWeaponConfig*, NewConfig);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnFireModeChanged, EZSFireMode, NewFireMode);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnAmmoChanged, int32, NewAmmo);
/** B0-T10.2: lets a future HUD indicator (B1) bind to jam state instead of polling IsJammed() - the "HUD indicator hook" half of T10.2's definition of done. The audio-cue half is a content task, no audio system exists yet. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FZSOnJamStateChanged, bool, bNewIsJammed);

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

	/** B0-T2.11: true only if the magazine isn't already full, CurrentConfig->AmmoItemConfig is set, and the owning character's UZSInventoryComponent actually carries at least one unit of it - checking real inventory state instead of a flat CurrentReserveAmmo > 0. */
	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool CanReload() const;

	/** P5: durability-lite ("melee breaks, no repair sim v1" - GameDevPlan.md). Decrements CurrentDurability by one landed hit; returns true if that hit broke it (reached 0). No-op / always returns false for an unbreakable weapon (CurrentConfig->MaxDurabilityHits == 0, e.g. every gun) or off a non-authoritative machine. Called by AZSPlayerCharacter::Server_WeaponMeleeAttack after a landed swing - this class only tracks the number, the caller decides what "broken" means for the loadout (auto-unequip). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	bool Server_ConsumeDurabilityHit();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	int32 GetCurrentDurability() const { return CurrentDurability; }

	/** B0-T2 Step B: overrides the fresh-spawn durability InitializeFromConfig just set, with the value carried over from the FZSItemInstance this weapon was equipped from - this is the actual "durability persists through equip/unequip" fix. InstanceDurability == -1 means "never touched yet" (a freshly-looted weapon's first equip) - falls back to Config->MaxDurabilityHits x ConditionQuality in that case, same seeding CurrentConfig would otherwise have used, just scaled by loot condition (B0-T2.10). Call right after InitializeFromConfig; no-op off a non-authoritative machine. Also stores ConditionQuality into CurrentConditionQuality regardless of MaxDurabilityHits (B0-T10.1's jam-chance roll needs it even for an otherwise-unbreakable gun). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void SeedDurabilityFromInstance(int32 InstanceDurability, float ConditionQuality);

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	bool IsJammed() const { return bIsJammed; }

	/** B0-T10.1: server-only, called from AZSPlayerCharacter::Server_Fire right after CanFire() passes. Rolls a jam chance that interpolates CurrentConfig->BaseJamChance (at CurrentConditionQuality == 1) to MaxJamChance (at 0) - always false for CurrentConfig->bJamImmune or a weapon already jammed. Returns true if this roll caused a fresh jam (the caller aborts the shot outright - no ammo consumed, no shot resolved). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	bool Server_RollForJam();

	/** B0-T10.1: clears bIsJammed - called after the Rack Firearm action completes (AZSPlayerCharacter::Server_StartRackFirearm). No-op if not currently jammed or off a non-authoritative machine. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Weapon")
	void Server_ClearJam();

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnJamStateChanged OnJamStateChanged;

	/** B0-T2.11: removes up to (MagazineCapacity - CurrentMagazineAmmo) units of CurrentConfig->AmmoItemConfig from the owning character's UZSInventoryComponent::CarrySlots and adds whatever was actually available to CurrentMagazineAmmo - reserve ammo is real inventory now, not a flat counter on this actor. Synchronous; the reload montage that follows is purely cosmetic (see CoreLoopPlan.md Phase 2 "Key architecture decisions"). Gameplay execution point - overridable per-weapon. */
	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void PerformReload();

	UFUNCTION(BlueprintNativeEvent, Category = "ZS|Weapon")
	void CycleFireMode();

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon", meta = (BlueprintThreadSafe))
	UZSWeaponConfig* GetConfig() const { return CurrentConfig; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	UStaticMeshComponent* GetBaseWeaponMesh() const { return BaseWeaponMesh; }

	UFUNCTION(BlueprintPure, Category = "ZS|Weapon")
	EZSFireMode GetCurrentFireMode() const { return CurrentFireMode; }

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnConfigChanged OnConfigChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnFireModeChanged OnFireModeChanged;

	UPROPERTY(BlueprintAssignable, Category = "ZS|Weapon")
	FZSOnAmmoChanged OnMagazineAmmoChanged;

protected:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BaseWeaponMesh;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentConfig, Category = "ZS|Weapon")
	TObjectPtr<UZSWeaponConfig> CurrentConfig;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentFireMode, Category = "ZS|Weapon")
	EZSFireMode CurrentFireMode = EZSFireMode::Semi;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CurrentMagazineAmmo, Category = "ZS|Weapon")
	int32 CurrentMagazineAmmo = 0;

	/** P5: seeded from CurrentConfig->MaxDurabilityHits in InitializeFromConfig. Stays at 0 (and Server_ConsumeDurabilityHit stays a no-op) for an unbreakable weapon. No OnRep needed - nothing client-side reacts to durability yet (no UI), same "plain replicated state" reasoning as AZSGameState::UtilitiesShutoffDay. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ZS|Weapon")
	int32 CurrentDurability = 0;

	/** B0-T10.1: stored by SeedDurabilityFromInstance regardless of MaxDurabilityHits - jam-chance rolls need it even for guns (which opt out of the melee-durability system via MaxDurabilityHits == 0). No OnRep needed, same reasoning as CurrentDurability. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "ZS|Weapon")
	float CurrentConditionQuality = 1.f;

	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_IsJammed, Category = "ZS|Weapon")
	bool bIsJammed = false;

	UFUNCTION()
	void OnRep_IsJammed();

	/** Runs the same cosmetic assembly InitializeFromConfig() runs server-side - this is what makes a client's local proxy of this replicated actor actually look right, since clients never call InitializeFromConfig() themselves. Also refreshes the owning character's body mesh (Owner replicates natively). */
	UFUNCTION()
	void OnRep_CurrentConfig();

	UFUNCTION()
	void OnRep_CurrentFireMode();

	UFUNCTION()
	void OnRep_CurrentMagazineAmmo();

	/** Cosmetic-only assembly from CurrentConfig: base weapon mesh, static-mesh attachments, magazine prop. Called from InitializeFromConfig on the server and OnRep_CurrentConfig on clients - each machine assembles its own local, unreplicated cosmetics. */
	void AssembleCosmeticsFromConfig();

private:

	UStaticMeshComponent* AssignNewStaticMesh(const FName& SocketName, UStaticMesh* Mesh, const FName& ComponentName);

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> TriggerMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> MuzzleMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HandguardMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> GripMesh;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> OpticMesh;

	UPROPERTY()
	TObjectPtr<AZSMagazine> MainMagazine;
};
