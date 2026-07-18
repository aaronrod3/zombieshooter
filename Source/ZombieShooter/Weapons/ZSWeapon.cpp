// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeapon.h"
#include "ZSWeaponConfig.h"
#include "ZSPlayerCharacter.h"
#include "ZSMagazine.h"
#include "ZSPhysicsMagazine.h"
#include "ZSPhysicsCasing.h"
#include "ZSLaserAttachment.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Net/UnrealNetwork.h"

AZSWeapon::AZSWeapon()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// This actor never moves independently - it's always attached to GetMesh() (see
	// AttachWeaponToActiveMesh's comment for why that attachment is now permanent, never
	// per-perspective) - so it has no absolute position/velocity that ever needs replicating.
	SetReplicateMovement(false);

	SK_Receiver = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SK_Receiver"));
	SetRootComponent(SK_Receiver);
}

void AZSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSWeapon, CurrentConfig);
	DOREPLIFETIME(AZSWeapon, CurrentGrip);
	DOREPLIFETIME(AZSWeapon, CurrentFireMode);
	DOREPLIFETIME(AZSWeapon, CurrentMagazineAmmo);
	DOREPLIFETIME(AZSWeapon, CurrentReserveAmmo);
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
	AssembleReceiverCosmetics();
	AssembleMagazinesAndLaser();
}

void AZSWeapon::AssembleReceiverCosmetics()
{
	if (!CurrentConfig)
	{
		return;
	}

	UZSWeaponConfig* Config = CurrentConfig;

	if (Config->MeshReceiver)
	{
		SK_Receiver->SetSkeletalMesh(Config->MeshReceiver);
	}

	if (Config->ABP_Weapon)
	{
		SK_Receiver->SetAnimInstanceClass(Config->ABP_Weapon);
	}

	HandguardMesh = AssignNewStaticMesh(Config->SocketHandguard, Config->MeshHandguard, TEXT("HandguardMesh"));
	SilencerMesh = AssignNewStaticMesh(Config->SocketMuzzle, Config->MeshSilencer, TEXT("SilencerMesh"));
	ScopeMesh = AssignNewStaticMesh(Config->SocketScope, Config->MeshScope, TEXT("ScopeMesh"));
	FrontSightMesh = AssignNewStaticMesh(Config->SocketSightFront, Config->MeshSightFront, TEXT("FrontSightMesh"));
	RearSightMesh = AssignNewStaticMesh(Config->SocketSightRear, Config->MeshSightRear, TEXT("RearSightMesh"));

	if (SK_Receiver->DoesSocketExist(Config->SocketGripAttachment))
	{
		UpdateGripVisual();
	}

	if (bIsFirstPersonVisual)
	{
		SetOwnerOnlyVisible(true);
	}
}

void AZSWeapon::AssembleMagazinesAndLaser()
{
	if (!CurrentConfig)
	{
		return;
	}

	UZSWeaponConfig* Config = CurrentConfig;

	if (SK_Receiver->DoesSocketExist(Config->SocketLaserAttachment))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		LaserAttachment = GetWorld()->SpawnActor<AZSLaserAttachment>(SpawnParams);
		if (LaserAttachment)
		{
			LaserAttachment->AttachToComponent(SK_Receiver, FAttachmentTransformRules::SnapToTargetNotIncludingScale, Config->SocketLaserAttachment);
			LaserAttachment->InitializeFromConfig(Config, SK_Receiver);
		}
	}

	// Each machine (server and every client) spawns its own local, unreplicated magazine
	// actors here - AZSMagazine is deliberately never a replicated actor (see CoreLoopPlan.md's
	// Phase 3 state classification), so this runs once per machine via whichever path got it
	// here (InitializeFromConfig on the server, OnRep_CurrentConfig on clients).
	MainMagazine = SpawnMagazine(Config->SocketMagazineAttachment);
	ReserveMagazine = SpawnMagazine(Config->SocketMagazineReserveAttachment);
	SetMagazineVisibility(false, true);
}

void AZSWeapon::InitializeAsFirstPersonVisual(UZSWeaponConfig* Config)
{
	if (!Config)
	{
		return;
	}

	bIsFirstPersonVisual = true;
	SetReplicateMovement(false);
	SetReplicates(false);

	CurrentConfig = Config;
	AssembleReceiverCosmetics();
}

void AZSWeapon::SetOwnerOnlyVisible(bool bOwnerOnly)
{
	SK_Receiver->SetOnlyOwnerSee(bOwnerOnly);

	for (UStaticMeshComponent* Cosmetic : { HandguardMesh.Get(), SilencerMesh.Get(), ScopeMesh.Get(), FrontSightMesh.Get(), RearSightMesh.Get(), GripMesh.Get() })
	{
		if (Cosmetic)
		{
			Cosmetic->SetOnlyOwnerSee(bOwnerOnly);
		}
	}
}

void AZSWeapon::SetHiddenFromOwner(bool bHideFromOwner)
{
	SK_Receiver->SetOwnerNoSee(bHideFromOwner);

	for (UStaticMeshComponent* Cosmetic : { HandguardMesh.Get(), SilencerMesh.Get(), ScopeMesh.Get(), FrontSightMesh.Get(), RearSightMesh.Get(), GripMesh.Get() })
	{
		if (Cosmetic)
		{
			Cosmetic->SetOwnerNoSee(bHideFromOwner);
		}
	}
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
		Character->RefreshBodyMeshesFromWeapon();
		Character->AttachWeaponToActiveMesh();
	}

	OnConfigChanged.Broadcast(CurrentConfig);
}

void AZSWeapon::OnRep_CurrentGrip()
{
	UpdateGripVisual();
	OnGripChanged.Broadcast(CurrentGrip);
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
	if (!Mesh || SocketName.IsNone() || !SK_Receiver->DoesSocketExist(SocketName))
	{
		return nullptr;
	}

	UStaticMeshComponent* NewComponent = NewObject<UStaticMeshComponent>(this, ComponentName);
	NewComponent->SetupAttachment(SK_Receiver, SocketName);
	NewComponent->SetStaticMesh(Mesh);
	NewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	NewComponent->RegisterComponent();

	return NewComponent;
}

AZSMagazine* AZSWeapon::SpawnMagazine(FName SocketName)
{
	if (!CurrentConfig || SocketName.IsNone() || !SK_Receiver->DoesSocketExist(SocketName))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	AZSMagazine* NewMagazine = GetWorld()->SpawnActor<AZSMagazine>(SpawnParams);
	if (NewMagazine)
	{
		NewMagazine->AttachToComponent(SK_Receiver, FAttachmentTransformRules::SnapToTargetNotIncludingScale, SocketName);
		NewMagazine->InitializeFromConfig(CurrentConfig);
	}

	return NewMagazine;
}

void AZSWeapon::SetMagazineVisibility(bool bVisible, bool bIsReserve)
{
	AZSMagazine* Target = bIsReserve ? ReserveMagazine : MainMagazine;
	if (Target)
	{
		Target->SetActorHiddenInGame(!bVisible);
	}
}

AZSPhysicsMagazine* AZSWeapon::SpawnDroppedMagazine_Implementation(float ImpulseForce, float RotationForce)
{
	// Not HasAuthority()-gated: currently only ever called by AN_ZS_DropMagazine, a weapon-owned
	// notify class deferred since Phase 2 M9 (SK_Receiver has no AnimInstance yet, so this is
	// unreachable dead code for now). When that gap is closed, this is meant to run per-machine
	// as a local, unreplicated cosmetic spawn (same as AZSMagazine/AZSPhysicsCasing) - see
	// CoreLoopPlan.md's Phase 3 state classification.
	if (!CurrentConfig || !MainMagazine)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	const FTransform SpawnTransform = MainMagazine->GetActorTransform();
	AZSPhysicsMagazine* DroppedMagazine = GetWorld()->SpawnActor<AZSPhysicsMagazine>(SpawnTransform.GetLocation(), SpawnTransform.Rotator(), SpawnParams);
	if (DroppedMagazine)
	{
		DroppedMagazine->InitializeVisuals(CurrentConfig->MeshMagazine, CurrentConfig->SoundCue_WEP_MagDrop);
		DroppedMagazine->ApplyLaunchImpulse(-SpawnTransform.GetRotation().GetUpVector(), ImpulseForce, RotationForce);
	}

	return DroppedMagazine;
}

AZSPhysicsCasing* AZSWeapon::EjectCasing_Implementation(FRotator RotationOffset, float MinEjectForce, float MaxEjectForce, float RotationSpeed)
{
	// Same deferred-dead-code note as SpawnDroppedMagazine_Implementation above.
	if (!CurrentConfig || !SK_Receiver->DoesSocketExist(CurrentConfig->SocketCasingEject))
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;

	const FTransform EjectTransform = SK_Receiver->GetSocketTransform(CurrentConfig->SocketCasingEject);
	AZSPhysicsCasing* Casing = GetWorld()->SpawnActor<AZSPhysicsCasing>(EjectTransform.GetLocation(), EjectTransform.Rotator(), SpawnParams);
	if (Casing)
	{
		Casing->InitializeVisuals(CurrentConfig->MeshBulletCasing, CurrentConfig->SoundCue_WEP_CasingEject);

		const FVector EjectDirection = (EjectTransform.GetRotation() * RotationOffset.Quaternion()).GetUpVector();
		const float EjectForce = FMath::RandRange(MinEjectForce, MaxEjectForce);
		Casing->ApplyLaunchImpulse(EjectDirection, EjectForce, RotationSpeed);
	}

	return Casing;
}

void AZSWeapon::SetGripAttachment_Implementation(EZSGripAttachment NewGrip)
{
	if (!HasAuthority())
	{
		return;
	}

	CurrentGrip = NewGrip;

	// OnRep_CurrentGrip never fires on the authority machine itself - apply the visual directly
	// here too, same pattern as InitializeFromConfig/AssembleCosmeticsFromConfig above.
	UpdateGripVisual();
}

void AZSWeapon::UpdateGripVisual()
{
	if (!CurrentConfig)
	{
		return;
	}

	UStaticMesh* GripVisual = nullptr;
	switch (CurrentGrip)
	{
	case EZSGripAttachment::Vertical:
		GripVisual = CurrentConfig->MeshGripVertical;
		break;
	case EZSGripAttachment::Angled:
		GripVisual = CurrentConfig->MeshGripAngled;
		break;
	case EZSGripAttachment::None:
	default:
		break;
	}

	if (!GripMesh)
	{
		GripMesh = AssignNewStaticMesh(CurrentConfig->SocketGripAttachment, GripVisual, TEXT("GripMesh"));

		// GripMesh is created lazily here, on whatever the first non-nothing-relevant call happens
		// to be - if that's after InitializeAsFirstPersonVisual's own one-time SetOwnerOnlyVisible
		// pass already ran (e.g. the real weapon's grip changes later and the FirstPerson twin
		// mirrors it via OnGripChanged - see AZSPlayerCharacter::RefreshFirstPersonWeaponVisual),
		// this brand new component still needs marking too.
		if (bIsFirstPersonVisual && GripMesh)
		{
			GripMesh->SetOnlyOwnerSee(true);
		}

		return;
	}

	GripMesh->SetStaticMesh(GripVisual);
	GripMesh->SetVisibility(GripVisual != nullptr);
}

void AZSWeapon::RandomizeGripAttachment_Implementation()
{
	const uint8 NextGrip = (static_cast<uint8>(CurrentGrip) + 1) % 3;
	SetGripAttachment(static_cast<EZSGripAttachment>(NextGrip));
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
