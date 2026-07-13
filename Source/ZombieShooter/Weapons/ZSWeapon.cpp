// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSWeapon.h"
#include "ZSWeaponConfig.h"
#include "ZSMagazine.h"
#include "ZSPhysicsMagazine.h"
#include "ZSPhysicsCasing.h"
#include "ZSLaserAttachment.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

AZSWeapon::AZSWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	SK_Receiver = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SK_Receiver"));
	SetRootComponent(SK_Receiver);
}

void AZSWeapon::InitializeFromConfig(UZSWeaponConfig* Config)
{
	if (!Config)
	{
		return;
	}

	CurrentConfig = Config;

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
}

void AZSWeapon::BeginPlay()
{
	Super::BeginPlay();

	if (!CurrentConfig)
	{
		return;
	}

	CurrentMagazineAmmo = CurrentConfig->MagazineCapacity;
	CurrentReserveAmmo = CurrentConfig->StartingReserveAmmo;

	if (CurrentConfig->SupportedFireModes.Num() > 0)
	{
		CurrentFireMode = CurrentConfig->SupportedFireModes[0];
	}

	if (SK_Receiver->DoesSocketExist(CurrentConfig->SocketLaserAttachment))
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;

		LaserAttachment = GetWorld()->SpawnActor<AZSLaserAttachment>(SpawnParams);
		if (LaserAttachment)
		{
			LaserAttachment->AttachToComponent(SK_Receiver, FAttachmentTransformRules::SnapToTargetNotIncludingScale, CurrentConfig->SocketLaserAttachment);
			LaserAttachment->InitializeFromConfig(CurrentConfig, SK_Receiver);
		}
	}

	if (SK_Receiver->DoesSocketExist(CurrentConfig->SocketGripAttachment))
	{
		SetGripAttachment(EZSGripAttachment::None);
	}

	MainMagazine = SpawnMagazine(CurrentConfig->SocketMagazineAttachment);
	ReserveMagazine = SpawnMagazine(CurrentConfig->SocketMagazineReserveAttachment);
	SetMagazineVisibility(false, true);
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
	CurrentGrip = NewGrip;

	if (!CurrentConfig)
	{
		return;
	}

	UStaticMesh* GripVisual = nullptr;
	switch (NewGrip)
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

bool AZSWeapon::ConsumeAmmoRound()
{
	if (CurrentMagazineAmmo <= 0)
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
	if (!CanReload())
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
	if (!CurrentConfig || CurrentConfig->SupportedFireModes.Num() == 0)
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
