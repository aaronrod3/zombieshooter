// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSLaserAttachment.h"
#include "ZSWeaponConfig.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"

AZSLaserAttachment::AZSLaserAttachment()
{
	PrimaryActorTick.bCanEverTick = true;

	LaserBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBodyMesh"));
	SetRootComponent(LaserBodyMesh);
	LaserBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LaserBeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LaserBeamMesh"));
	LaserBeamMesh->SetupAttachment(LaserBodyMesh);
	LaserBeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	LaserLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("LaserLight"));
	LaserLight->SetupAttachment(LaserBodyMesh);
	LaserLight->SetIntensity(500.f);
	LaserLight->SetLightColor(FLinearColor::Red);
}

void AZSLaserAttachment::InitializeFromConfig(UZSWeaponConfig* Config, USkeletalMeshComponent* InReceiverMesh)
{
	if (!Config || !InReceiverMesh)
	{
		return;
	}

	WeaponConfig = Config;
	ReceiverMesh = InReceiverMesh;

	if (Config->MeshLaserAttachment)
	{
		LaserBodyMesh->SetStaticMesh(Config->MeshLaserAttachment);
	}

	if (LaserBeamMeshAsset)
	{
		LaserBeamMesh->SetStaticMesh(LaserBeamMeshAsset);
	}
}

void AZSLaserAttachment::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!WeaponConfig || !ReceiverMesh.IsValid())
	{
		return;
	}

	if (!ReceiverMesh->DoesSocketExist(WeaponConfig->SocketLaserStart))
	{
		return;
	}

	const FTransform StartTransform = ReceiverMesh->GetSocketTransform(WeaponConfig->SocketLaserStart);
	const FVector TraceStart = StartTransform.GetLocation();
	const FVector TraceDirection = StartTransform.GetRotation().GetForwardVector();
	const FVector TraceEnd = TraceStart + TraceDirection * MaxTraceDistance;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(GetOwner());

	FHitResult Hit;
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams);
	const FVector HitLocation = bHit ? Hit.Location : TraceEnd;
	const float Distance = FVector::Dist(TraceStart, HitLocation);

	LaserBeamMesh->SetWorldLocation(TraceStart);
	LaserBeamMesh->SetWorldRotation(StartTransform.GetRotation());
	LaserBeamMesh->SetWorldScale3D(FVector(Distance, 1.f, 1.f));

	LaserLight->SetWorldLocation(HitLocation);
}
