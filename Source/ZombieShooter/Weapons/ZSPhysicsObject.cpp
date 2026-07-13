// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSPhysicsObject.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"

AZSPhysicsObject::AZSPhysicsObject()
{
	PrimaryActorTick.bCanEverTick = false;

	PhysicsMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicsMesh"));
	SetRootComponent(PhysicsMesh);

	PhysicsMesh->SetSimulatePhysics(true);
	PhysicsMesh->SetCollisionProfileName(TEXT("PhysicsActor"));
	PhysicsMesh->SetNotifyRigidBodyCollision(true);
	PhysicsMesh->OnComponentHit.AddDynamic(this, &AZSPhysicsObject::HandlePhysicsMeshHit);
}

void AZSPhysicsObject::InitializeVisuals(UStaticMesh* Mesh, USoundBase* InImpactSound)
{
	if (Mesh)
	{
		PhysicsMesh->SetStaticMesh(Mesh);
	}

	ImpactSound = InImpactSound;
}

void AZSPhysicsObject::ApplyLaunchImpulse(const FVector& ImpulseDirection, float ImpulseForce, float RotationSpeed)
{
	PhysicsMesh->AddImpulse(ImpulseDirection.GetSafeNormal() * ImpulseForce, NAME_None, true);
	PhysicsMesh->AddAngularImpulseInDegrees(FVector(RotationSpeed, 0.f, 0.f), NAME_None, true);
}

void AZSPhysicsObject::BeginPlay()
{
	Super::BeginPlay();

	// 0 means "never expires" - AActor::SetLifeSpan only arms the destruction timer when
	// LifeSpan is greater than zero, so no special-case branch is needed here.
	SetLifeSpan(TimeUntilDestroyed);
}

void AZSPhysicsObject::HandlePhysicsMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!ImpactSound)
	{
		return;
	}

	if (bPlaySoundOnce)
	{
		if (bHasPlayedImpactSound)
		{
			return;
		}

		bHasPlayedImpactSound = true;
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.Location);
	}
	else if (NormalImpulse.Size() >= MinimumForceToPlaySound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Hit.Location);
	}
}
