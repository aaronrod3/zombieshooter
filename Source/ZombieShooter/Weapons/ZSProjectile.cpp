// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSProjectile.h"
#include "ZSWeaponConfig.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "../Combat/ZSDamageTypes.h"
#include "../ZombieShooter.h"

AZSProjectile::AZSProjectile()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(5.f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AZSProjectile::HandleHit);
	SetRootComponent(CollisionComponent);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(CollisionComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Placeholder-mesh-friendly default - a config's ProjectileMesh (e.g. the engine's default
	// Sphere, ~100cm across) would otherwise render bullet-sized as a giant floating ball.
	ProjectileMesh->SetRelativeScale3D(FVector(0.15f));

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	InitialLifeSpan = 5.f;
}

void AZSProjectile::InitializeProjectile(const UZSWeaponConfig* InConfig, AActor* InInstigatorActor, AController* InInstigatorController, float InHeadshotChance)
{
	if (!HasAuthority() || !InConfig)
	{
		return;
	}

	InstigatorActorRef = InInstigatorActor;
	InstigatorControllerRef = InInstigatorController;
	Damage = InConfig->FireDamage;
	KnockbackStrength = InConfig->FireKnockbackStrength;
	HeadshotChance = InHeadshotChance;
	DamageTypeClass = InConfig->FireDamageTypeClass
		? InConfig->FireDamageTypeClass
		: TSubclassOf<UDamageType>(UZSDamageType_Laceration::StaticClass());

	if (InstigatorActorRef)
	{
		// Spawns just outside the muzzle, close enough to the firing character's own capsule that
		// an unfiltered blocking hit could register against its own shooter on the spawn frame -
		// same class of problem as the cosmetic-attachment-collision lesson elsewhere in Weapons/.
		CollisionComponent->IgnoreActorWhenMoving(InstigatorActorRef, true);
	}

	if (ProjectileMesh && InConfig->ProjectileMesh)
	{
		ProjectileMesh->SetStaticMesh(InConfig->ProjectileMesh);
	}

	ProjectileMovement->InitialSpeed = InConfig->ProjectileSpeed;
	ProjectileMovement->MaxSpeed = InConfig->ProjectileSpeed;
	ProjectileMovement->Velocity = GetActorForwardVector() * InConfig->ProjectileSpeed;
}

void AZSProjectile::HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority() || bHasHit || !OtherActor || OtherActor == InstigatorActorRef)
	{
		return;
	}
	bHasHit = true;

	// B0-T3.6: same headshot-weighting override the hitscan path uses (Server_Fire) - mutate the
	// local copy of Hit before it feeds AZSPlayerCharacter::TakeDamage's BodyZoneFromBoneName
	// inference. A capsule-collision hit (no real bone) would otherwise always resolve to Torso.
	FHitResult WeightedHit = Hit;
	if (FMath::FRand() < HeadshotChance)
	{
		WeightedHit.BoneName = TEXT("head");
	}

	const FVector HitFromDirection = ProjectileMovement->Velocity.GetSafeNormal();
	UGameplayStatics::ApplyPointDamage(OtherActor, Damage, HitFromDirection, WeightedHit, InstigatorControllerRef, InstigatorActorRef, DamageTypeClass);

	if (KnockbackStrength > 0.f)
	{
		if (ACharacter* TargetCharacter = Cast<ACharacter>(OtherActor))
		{
			TargetCharacter->LaunchCharacter(HitFromDirection * KnockbackStrength, true, false);
		}
	}

	// Temporary confirmation while no impact VFX/hit-reaction exists yet - remove once real
	// feedback is built (same note as Server_Fire/Server_MeleeAttack).
	UE_LOG(LogZombieShooter, Log, TEXT("%s: projectile hit %s for %.1f damage"),
		InstigatorActorRef ? *InstigatorActorRef->GetName() : TEXT("Unknown"), *OtherActor->GetName(), Damage);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 1.5f, FColor::Green, FString::Printf(TEXT("Projectile hit %s for %.0f"), *OtherActor->GetName(), Damage));
	}

	Destroy();
}
