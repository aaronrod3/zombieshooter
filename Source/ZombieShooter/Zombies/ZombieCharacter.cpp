// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieCharacter.h"
#include "ZSZombieConfig.h"
#include "ZombieAIController.h"
#include "../Combat/ZSDamageTypes.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

AZombieCharacter::AZombieCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

	AIControllerClass = AZombieAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AZombieCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZombieCharacter, CurrentHealth);
	DOREPLIFETIME(AZombieCharacter, bIsDead);
}

void AZombieCharacter::BeginPlay()
{
	Super::BeginPlay();

	AssembleCosmeticsFromConfig();

	if (HasAuthority())
	{
		CurrentHealth = ZombieConfig ? ZombieConfig->MaxHealth : CurrentHealth;
	}
}

void AZombieCharacter::AssembleCosmeticsFromConfig()
{
	if (!ZombieConfig)
	{
		return;
	}

	if (ZombieConfig->ZombieMesh)
	{
		GetMesh()->SetSkeletalMesh(ZombieConfig->ZombieMesh);
	}

	if (ZombieConfig->AnimClass)
	{
		GetMesh()->SetAnimInstanceClass(ZombieConfig->AnimClass);
	}

	GetCharacterMovement()->MaxWalkSpeed = ZombieConfig->WalkSpeed;
}

float AZombieCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (!HasAuthority() || bIsDead || ActualDamage <= 0.f)
	{
		return ActualDamage;
	}

	CurrentHealth = FMath::Clamp(CurrentHealth - ActualDamage, 0.f, ZombieConfig ? ZombieConfig->MaxHealth : 100.f);
	OnRep_CurrentHealth();

	if (CurrentHealth <= 0.f)
	{
		Die();
	}

	return ActualDamage;
}

void AZombieCharacter::Server_MeleeAttack(AActor* Target)
{
	if (!HasAuthority() || !Target || !ZombieConfig || bIsDead)
	{
		return;
	}

	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastAttackTime < ZombieConfig->AttackInterval)
	{
		return;
	}

	const float DistanceToTarget = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
	if (DistanceToTarget > ZombieConfig->MeleeRange)
	{
		return;
	}

	LastAttackTime = Now;

	const TSubclassOf<UDamageType> DamageTypeClass = ZombieConfig->AttackDamageTypeClass
		? ZombieConfig->AttackDamageTypeClass
		: TSubclassOf<UDamageType>(UZSDamageType_Bite::StaticClass());

	const FVector HitDirection = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
	const FHitResult HitResult;

	UGameplayStatics::ApplyPointDamage(Target, ZombieConfig->MeleeDamage, HitDirection, HitResult, GetController(), this, DamageTypeClass);
}

void AZombieCharacter::SetChasing(bool bChasing)
{
	if (!HasAuthority() || !ZombieConfig)
	{
		return;
	}

	GetCharacterMovement()->MaxWalkSpeed = bChasing ? ZombieConfig->ChaseSpeed : ZombieConfig->WalkSpeed;
}

void AZombieCharacter::Die()
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
		GetWorldTimerManager().SetTimer(CorpseCleanupTimerHandle, this, &AZombieCharacter::HandleCorpseCleanup, CorpseLingerSeconds, false);
	}
}

void AZombieCharacter::HandleCorpseCleanup()
{
	Destroy();
}

void AZombieCharacter::OnRep_CurrentHealth()
{
	OnHealthChanged.Broadcast(CurrentHealth);
}

void AZombieCharacter::OnRep_IsDead()
{
	if (bIsDead)
	{
		OnDeath.Broadcast();
	}
}
