// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeapon.h"
#include "ZSWeaponConfig.h"
#include "ZSMagazineConfig.h"
#include "ZSPlayerCharacter.h"
#include "ZSMagazine.h"
#include "../Inventory/ZSInventoryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AZSWeapon::AZSWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// This actor never moves independently - it's always attached to the character's body mesh -
	// so it has no absolute position/velocity that ever needs replicating.
	SetReplicateMovement(false);

	BaseWeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BaseWeaponMesh"));
	SetRootComponent(BaseWeaponMesh);

	// Purely cosmetic once attached to the character - hitscan uses socket-based tracing, not
	// physical collision. Left at the default BlockAll profile, this rigidly-attached mesh
	// overlaps the owning character's own capsule every tick, and CharacterMovementComponent's
	// penetration-resolution logic fights that overlap indefinitely - visible as the character
	// climbing upward or sliding sideways the instant a weapon is equipped. Every other cosmetic
	// mesh component here (see AssignNewStaticMesh) already gets this same NoCollision treatment;
	// this one was a gap since it's built directly in the constructor, not through that helper.
	BaseWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AZSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSWeapon, CurrentConfig);
	DOREPLIFETIME(AZSWeapon, CurrentFireMode);
	DOREPLIFETIME(AZSWeapon, CurrentMagazineAmmo);
	DOREPLIFETIME(AZSWeapon, CurrentMagazineInstanceId);
	DOREPLIFETIME(AZSWeapon, CurrentMagazineConfig);
	DOREPLIFETIME(AZSWeapon, CurrentDurability);
	DOREPLIFETIME(AZSWeapon, CurrentConditionQuality);
	DOREPLIFETIME(AZSWeapon, bIsJammed);
}

void AZSWeapon::InitializeFromConfig(UZSWeaponConfig* Config)
{
	if (!Config || !HasAuthority())
	{
		return;
	}

	CurrentConfig = Config;

	CurrentMagazineAmmo = Config->MagazineCapacity;
	CurrentDurability = Config->MaxDurabilityHits;

	if (Config->SupportedFireModes.Num() > 0)
	{
		CurrentFireMode = Config->SupportedFireModes[0];
	}

	// OnRep_X never fires on the machine that has authority (a listen server never gets its
	// own replication callback for state it just authored) - the cosmetic assembly below has
	// to be called directly here too, not just from OnRep_CurrentConfig, so the host/server's
	// own local view of this weapon looks right as well as every remote client's.
	AssembleCosmeticsFromConfig();
	OnRep_Durability();
}

void AZSWeapon::AssembleCosmeticsFromConfig()
{
	if (!CurrentConfig)
	{
		return;
	}

	UZSWeaponConfig* Config = CurrentConfig;

	if (Config->BaseWeaponMesh)
	{
		BaseWeaponMesh->SetStaticMesh(Config->BaseWeaponMesh);
	}

	TriggerMesh = AssignNewStaticMesh(Config->SocketTrigger, Config->TriggerMesh, TEXT("TriggerMesh"));
	MuzzleMesh = AssignNewStaticMesh(Config->SocketMuzzle, Config->MuzzleMesh, TEXT("MuzzleMesh"));
	HandguardMesh = AssignNewStaticMesh(Config->SocketHandguard, Config->HandguardMesh, TEXT("HandguardMesh"));
	GripMesh = AssignNewStaticMesh(Config->SocketGrip, Config->GripMesh, TEXT("GripMesh"));
	OpticMesh = AssignNewStaticMesh(Config->SocketOptic, Config->OpticMesh, TEXT("OpticMesh"));

	// Each machine (server and every client) spawns its own local, unreplicated magazine actor
	// here - AZSMagazine is deliberately never a replicated actor (see CoreLoopPlan.md's Phase 3
	// state classification), so this runs once per machine via whichever path got it here
	// (InitializeFromConfig on the server, OnRep_CurrentConfig on clients).
	MainMagazine = SpawnMagazine(Config->SocketMagazineAttachment);
}

void AZSWeapon::OnRep_CurrentConfig()
{
	AssembleCosmeticsFromConfig();

	// CurrentWeapon/SecondaryWeapon (on the character) and CurrentConfig (here) can replicate to a
	// client in either order - call the matching client-side attach step here too (redundant with
	// AZSPlayerCharacter::OnRep_CurrentWeapon/OnRep_SecondaryWeapon) so whichever replication event
	// arrives second completes the setup. 2026-07-28: this weapon actor could be either slot now
	// (offhand weapons added), so which one it is has to be checked explicitly - calling the wrong
	// character-side attach function would reassemble/reattach the *other* slot's weapon instead of
	// this one.
	if (AZSPlayerCharacter* Character = GetOwner<AZSPlayerCharacter>())
	{
		if (Character->GetCurrentWeapon() == this)
		{
			Character->RefreshBodyMeshFromWeapon();
			Character->AttachWeaponToBodyMesh();
		}
		else if (Character->GetSecondaryWeapon() == this)
		{
			Character->AttachSecondaryWeaponToBodyMesh();
		}
	}

	OnConfigChanged.Broadcast(CurrentConfig);
}

void AZSWeapon::OnRep_CurrentFireMode()
{
	OnFireModeChanged.Broadcast(CurrentFireMode);
}

void AZSWeapon::OnRep_CurrentMagazineAmmo()
{
	OnMagazineAmmoChanged.Broadcast(CurrentMagazineAmmo);
}

void AZSWeapon::OnRep_Durability()
{
	OnDurabilityChanged.Broadcast(CurrentDurability, CurrentConditionQuality);
}

UStaticMeshComponent* AZSWeapon::AssignNewStaticMesh(const FName& SocketName, UStaticMesh* Mesh, const FName& ComponentName)
{
	if (!Mesh || SocketName.IsNone() || !BaseWeaponMesh->DoesSocketExist(SocketName))
	{
		return nullptr;
	}

	UStaticMeshComponent* NewComponent = NewObject<UStaticMeshComponent>(this, ComponentName);
	NewComponent->SetupAttachment(BaseWeaponMesh, SocketName);
	NewComponent->SetStaticMesh(Mesh);
	NewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComponent->RegisterComponent();

	return NewComponent;
}

void AZSWeapon::Destroyed()
{
	if (MainMagazine)
	{
		MainMagazine->Destroy();
		MainMagazine = nullptr;
	}

	Super::Destroyed();
}

AZSMagazine* AZSWeapon::SpawnMagazine(FName SocketName)
{
	if (!CurrentConfig || SocketName.IsNone() || !BaseWeaponMesh->DoesSocketExist(SocketName))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AZSMagazine* NewMagazine = GetWorld()->SpawnActor<AZSMagazine>(SpawnParams);
	if (NewMagazine)
	{
		NewMagazine->AttachToComponent(BaseWeaponMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		NewMagazine->InitializeFromConfig(CurrentConfig);
	}

	return NewMagazine;
}

bool AZSWeapon::CanFire() const
{
	return CurrentConfig && CurrentFireMode != EZSFireMode::Safety && CurrentMagazineAmmo > 0 && !bIsJammed;
}

bool AZSWeapon::Server_ConsumeAmmoRound()
{
	if (!HasAuthority() || CurrentMagazineAmmo <= 0)
	{
		return false;
	}

	--CurrentMagazineAmmo;

	// OnRep_X never fires on the machine that has authority - call it manually so the host's own
	// game reacts to its own authoritative write the same way every client does on replication.
	// (Pre-existing gap found during the B1-T3.5 audit, 2026-07-30 - fixed here since it's directly
	// relevant to T3.5's ammo counter; CycleFireMode_Implementation has the same gap for
	// CurrentFireMode but no B1 HUD task reads fire mode yet, so that one's left alone for now.)
	OnRep_CurrentMagazineAmmo();

	return true;
}

bool AZSWeapon::FindBestCompatibleMagazine(FGuid& OutInstanceId, int32& OutAmmoCount) const
{
	if (!CurrentConfig || !CurrentConfig->AmmoItemConfig)
	{
		return false;
	}

	const AZSPlayerCharacter* OwningCharacter = GetOwner<AZSPlayerCharacter>();
	const UZSInventoryComponent* Inventory = OwningCharacter ? OwningCharacter->GetInventoryComponent() : nullptr;
	if (!Inventory)
	{
		return false;
	}

	// 2026-08-11: picks the fullest compatible carried magazine, not just the first one found - a
	// minor quality-of-life over "first match" that costs nothing extra (already iterating every
	// CarrySlots entry regardless). Excludes whatever's already loaded, which per Server_LoadMagazine's
	// own comment is never itself sitting in CarrySlots while loaded - so that exclusion is
	// belt-and-suspenders, not load-bearing, but cheap to keep in case that invariant ever changes.
	bool bFound = false;
	int32 BestAmmoCount = -1;
	FGuid BestInstanceId;
	for (const FZSItemInstance& Instance : Inventory->GetCarrySlots())
	{
		if (Instance.InstanceId == CurrentMagazineInstanceId)
		{
			continue;
		}

		const UZSMagazineConfig* MagazineConfig = Cast<UZSMagazineConfig>(Instance.Config);
		if (!MagazineConfig || MagazineConfig->CompatibleAmmoConfig != CurrentConfig->AmmoItemConfig)
		{
			continue;
		}

		// -1 = uninitialised (a freshly-looted/granted magazine never explicitly filled) - resolves
		// to full capacity, same lazy-resolve pattern as CurrentDurability elsewhere in this project.
		const int32 ResolvedAmmo = Instance.InstanceState.CurrentAmmoCount >= 0
			? Instance.InstanceState.CurrentAmmoCount
			: MagazineConfig->MagazineCapacity;

		if (ResolvedAmmo > 0 && ResolvedAmmo > BestAmmoCount)
		{
			bFound = true;
			BestAmmoCount = ResolvedAmmo;
			BestInstanceId = Instance.InstanceId;
		}
	}

	if (bFound)
	{
		OutInstanceId = BestInstanceId;
		OutAmmoCount = BestAmmoCount;
	}
	return bFound;
}

bool AZSWeapon::CanReload() const
{
	if (!CurrentConfig || !CurrentConfig->AmmoItemConfig || CurrentMagazineAmmo >= CurrentConfig->MagazineCapacity)
	{
		return false;
	}

	FGuid UnusedInstanceId;
	int32 UnusedAmmoCount;
	return FindBestCompatibleMagazine(UnusedInstanceId, UnusedAmmoCount);
}

void AZSWeapon::Server_LoadMagazine(FGuid NewMagazineInstanceId, UZSItemConfig* NewMagazineConfig, int32 NewAmmoCount)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentMagazineInstanceId = NewMagazineInstanceId;
	CurrentMagazineConfig = NewMagazineConfig;
	CurrentMagazineAmmo = FMath::Max(NewAmmoCount, 0);

	// OnRep_X never fires on the machine that has authority - same pattern as every other
	// server-authored mutation in this class.
	OnRep_CurrentMagazineAmmo();
}

void AZSWeapon::Server_EjectCurrentMagazine(FGuid& OutMagazineInstanceId, UZSItemConfig*& OutMagazineConfig, int32& OutAmmoRemaining)
{
	OutMagazineInstanceId = CurrentMagazineInstanceId;
	OutMagazineConfig = CurrentMagazineConfig;
	OutAmmoRemaining = CurrentMagazineAmmo;

	if (!HasAuthority())
	{
		return;
	}

	CurrentMagazineInstanceId = FGuid();
	CurrentMagazineConfig = nullptr;
	CurrentMagazineAmmo = 0;

	OnRep_CurrentMagazineAmmo();
}

void AZSWeapon::SeedDurabilityFromInstance(int32 InstanceDurability, float ConditionQuality)
{
	if (!HasAuthority())
	{
		return;
	}

	// B0-T10.1: stored regardless of MaxDurabilityHits - a gun (MaxDurabilityHits == 0, "unbreakable"
	// for melee purposes only) still needs this for its jam-chance roll.
	CurrentConditionQuality = FMath::Clamp(ConditionQuality, 0.f, 1.f);

	if (!CurrentConfig || CurrentConfig->MaxDurabilityHits <= 0)
	{
		// Unbreakable weapon - CurrentDurability stays whatever InitializeFromConfig set (0), same
		// "never reaches/goes below 0" contract Server_ConsumeDurabilityHit already documents.
		// CurrentConditionQuality still changed above though, so still worth a broadcast.
		OnRep_Durability();
		return;
	}

	CurrentDurability = (InstanceDurability >= 0)
		? InstanceDurability
		: FMath::RoundToInt(CurrentConfig->MaxDurabilityHits * CurrentConditionQuality);
	OnRep_Durability();
}

bool AZSWeapon::Server_RollForJam()
{
	if (!HasAuthority() || !CurrentConfig || CurrentConfig->bJamImmune || bIsJammed)
	{
		return false;
	}

	// Interpolates BaseJamChance (at CurrentConditionQuality == 1, pristine) to MaxJamChance (at 0,
	// worst condition) - a worse-condition weapon jams more often, never less.
	const float JamChance = FMath::Lerp(CurrentConfig->MaxJamChance, CurrentConfig->BaseJamChance, CurrentConditionQuality);
	if (FMath::FRand() < JamChance)
	{
		bIsJammed = true;
		OnRep_IsJammed();
		return true;
	}

	return false;
}

void AZSWeapon::Server_ClearJam()
{
	if (!HasAuthority() || !bIsJammed)
	{
		return;
	}

	bIsJammed = false;
	OnRep_IsJammed();
}

void AZSWeapon::Server_ForceJam()
{
	if (!HasAuthority() || bIsJammed || (CurrentConfig && CurrentConfig->bJamImmune))
	{
		return;
	}

	bIsJammed = true;
	OnRep_IsJammed();
}

void AZSWeapon::OnRep_IsJammed()
{
	OnJamStateChanged.Broadcast(bIsJammed);
}

bool AZSWeapon::Server_ConsumeDurabilityHit()
{
	if (!HasAuthority() || !CurrentConfig || CurrentConfig->MaxDurabilityHits <= 0)
	{
		// Unbreakable (durability system opted out via MaxDurabilityHits == 0, e.g. every gun) -
		// never reaches (or goes below) 0, so this never reports "broken".
		return false;
	}

	CurrentDurability = FMath::Max(CurrentDurability - 1, 0);
	OnRep_Durability();
	return CurrentDurability <= 0;
}

void AZSWeapon::CycleFireMode_Implementation()
{
	if (!HasAuthority() || !CurrentConfig || CurrentConfig->SupportedFireModes.Num() == 0)
	{
		return;
	}

	int32 CurrentIndex = CurrentConfig->SupportedFireModes.IndexOfByKey(CurrentFireMode);
	if (CurrentIndex == INDEX_NONE)
	{
		CurrentIndex = -1;
	}

	const int32 NextIndex = (CurrentIndex + 1) % CurrentConfig->SupportedFireModes.Num();
	CurrentFireMode = CurrentConfig->SupportedFireModes[NextIndex];
}
