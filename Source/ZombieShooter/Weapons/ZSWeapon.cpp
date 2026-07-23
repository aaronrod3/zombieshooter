// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeapon.h"
#include "ZSWeaponConfig.h"
#include "ZSPlayerCharacter.h"
#include "ZSMagazine.h"
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
}

void AZSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSWeapon, CurrentConfig);
	DOREPLIFETIME(AZSWeapon, CurrentFireMode);
	DOREPLIFETIME(AZSWeapon, CurrentMagazineAmmo);
	DOREPLIFETIME(AZSWeapon, CurrentReserveAmmo);
	DOREPLIFETIME(AZSWeapon, CurrentDurability);
}

void AZSWeapon::InitializeFromConfig(UZSWeaponConfig* Config)
{
	if (!Config || !HasAuthority())
	{
		return;
	}

	CurrentConfig = Config;

	CurrentMagazineAmmo = Config->MagazineCapacity;
	CurrentReserveAmmo = Config->StartingReserveAmmo;
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

	// CurrentWeapon (on the character) and CurrentConfig (here) can replicate to a client in
	// either order - call both client-side setup steps here too (redundant with
	// AZSPlayerCharacter::OnRep_CurrentWeapon) so whichever replication event arrives second
	// completes the setup, regardless of which one that is.
	if (AZSPlayerCharacter* Character = GetOwner<AZSPlayerCharacter>())
	{
		Character->RefreshBodyMeshFromWeapon();
		Character->AttachWeaponToBodyMesh();
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

void AZSWeapon::OnRep_CurrentReserveAmmo()
{
	OnReserveAmmoChanged.Broadcast(CurrentReserveAmmo);
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
	return CurrentConfig && CurrentFireMode != EZSFireMode::Safety && CurrentMagazineAmmo > 0;
}

bool AZSWeapon::Server_ConsumeAmmoRound()
{
	if (!HasAuthority() || CurrentMagazineAmmo <= 0)
	{
		return false;
	}

	--CurrentMagazineAmmo;
	return true;
}

bool AZSWeapon::CanReload() const
{
	return CurrentConfig && CurrentReserveAmmo > 0 && CurrentMagazineAmmo < CurrentConfig->MagazineCapacity;
}

void AZSWeapon::PerformReload_Implementation()
{
	if (!HasAuthority() || !CanReload())
	{
		return;
	}

	const int32 AmmoNeeded = CurrentConfig->MagazineCapacity - CurrentMagazineAmmo;
	const int32 AmmoToTransfer = FMath::Min(AmmoNeeded, CurrentReserveAmmo);

	CurrentMagazineAmmo += AmmoToTransfer;
	CurrentReserveAmmo -= AmmoToTransfer;
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
