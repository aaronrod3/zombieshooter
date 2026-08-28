// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSHostileCharacter.h"
#include "ZSHostileConfig.h"
#include "ZSHostileAIController.h"
#include "../Combat/ZSDamageTypes.h"
#include "../Zombies/ZSNoiseSystem.h"
#include "../Inventory/ZSLootTableConfig.h"
#include "../Inventory/ZSWorldItemActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AZSHostileCharacter::AZSHostileCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AZSHostileAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AZSHostileCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZSHostileCharacter, CurrentHealth);
	DOREPLIFETIME(AZSHostileCharacter, bIsDead);
}

void AZSHostileCharacter::BeginPlay()
{
	Super::BeginPlay();

	AssembleCosmeticsFromConfig();

	if (HasAuthority())
	{
		CurrentHealth = HostileConfig ? HostileConfig->MaxHealth : CurrentHealth;
	}
}

void AZSHostileCharacter::AssembleCosmeticsFromConfig()
{
	if (!HostileConfig)
	{
		return;
	}

	if (HostileConfig->HostileMesh)
	{
		GetMesh()->SetSkeletalMesh(HostileConfig->HostileMesh);
	}

	if (HostileConfig->AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(HostileConfig->AnimClass);
	}

	GetCharacterMovement()->MaxWalkSpeed = HostileConfig->WalkSpeed;
}

float AZSHostileCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || bIsDead || ActualDamage <= 0.f)
	{
		return ActualDamage;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.f, HostileConfig ? HostileConfig->MaxHealth : 100.f);
	OnRep_CurrentHealth();

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return ActualDamage;
}

void AZSHostileCharacter::Server_RangedAttack(AActor* Target)
{
	if (!HasAuthority() || !Target || !HostileConfig || bIsDead)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastFireTime < HostileConfig->FireInterval)
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (DistanceToTarget > HostileConfig->FireRange)
	{
		return;
	}

	LastFireTime = Now;

	UZSNoiseSystem::ReportNoise(this, GetActorLocation(), 1.f, this, HostileConfig->FireNoiseRadius);

	// Mirrors AZSPlayerCharacter::FireWeapon's hitscan shape (muzzle-height trace, VRandCone spread
	// around the true aim direction, ApplyPointDamage on a genuine hit) rather than
	// AZombieCharacter::Server_MeleeAttack's capsule-trace-plus-weighted-zone-roll workaround - a
	// ranged trace against the target's real skeletal mesh naturally yields a real bone name, so no
	// BoneName override is needed for AZSPlayerCharacter::TakeDamage's zone inference to work.
	const FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector AimDirection = (Target->GetActorLocation() - TraceStart).GetSafeNormal();
	const FVector FireDirection = FMath::VRandCone(AimDirection, FMath::DegreesToRadians(HostileConfig->FireSpreadDegrees));
	const FVector TraceEnd = TraceStart + FireDirection * HostileConfig->FireRange;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	FHitResult Hit;
	const bool bHitActor = GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams) && Hit.GetActor();

	if (!bHitActor)
	{
		return;
	}

	const TSubclassOf<UDamageType> DamageTypeClass = HostileConfig->AttackDamageTypeClass
		? HostileConfig->AttackDamageTypeClass
		: TSubclassOf<UDamageType>(UZSDamageType_Laceration::StaticClass());

	const FVector HitFromDirection = (Hit.ImpactPoint - TraceStart).GetSafeNormal();
	UGameplayStatics::ApplyPointDamage(Hit.GetActor(), HostileConfig->RangedDamage, HitFromDirection, Hit, GetController(), this, DamageTypeClass);
}

void AZSHostileCharacter::Die()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	OnRep_IsDead();

	GetCharacterMovement()->DisableMovement();
	SetActorEnableCollision(false);

	if (HasAuthority())
	{
		// BF-T3.2/OQ-BF-03: rolled once here, not carried as a live inventory - a dead hostile has
		// nothing left to manage, only loot to leave behind. Same spawn pattern
		// UZSInventoryComponent::Server_DropAllItems already uses for a dead player's own loot.
		if (HostileConfig && HostileConfig->DeathLootTable)
		{
			const TArray<FZSItemInstance> DroppedLoot = HostileConfig->DeathLootTable->RollLoot(GetWorld());

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			for (const FZSItemInstance& Instance : DroppedLoot)
			{
				if (AZSWorldItemActor* WorldItem = GetWorld()->SpawnActor<AZSWorldItemActor>(AZSWorldItemActor::StaticClass(), GetActorLocation(), GetActorRotation(), SpawnParams))
				{
					WorldItem->InitializeFromInstance(Instance);
				}
			}
		}

		GetWorldTimerManager().SetTimer(CorpseCleanupTimerHandle, this, &AZSHostileCharacter::HandleCorpseCleanup, CorpseLingerSeconds, false);
	}
}

void AZSHostileCharacter::HandleCorpseCleanup()
{
	Destroy();
}

void AZSHostileCharacter::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth);
}

void AZSHostileCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast();
	}
}
