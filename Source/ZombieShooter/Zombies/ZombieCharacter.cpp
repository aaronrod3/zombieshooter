// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZombieCharacter.h"
#include "ZSZombieConfig.h"
#include "ZombieAIController.h"
#include "../Combat/ZSDamageTypes.h"
#include "../Framework/ZSGameMode.h"
#include "../ZombieShooter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "HAL/IConsoleManager.h"

// B0-T12.1, 2026-07-26: the code-buildable half of "Lvl_ZS_StressTest ... scriptable via console
// (ZS.SpawnZombies <n>)" - the graybox level itself is a .umap content asset this session has no
// editor/MCP access to create, so it's an honest content gap (see Docs/Beta/B0_Stabilization.md
// T12.1). This command works in any level once AZSGameMode::StressTestZombieClass is assigned to a
// real BP_Zombie_* Blueprint. Host-only (GetAuthGameMode() is only ever non-null on the server) -
// scatters Count zombies in a flat ring around the local player's pawn so they can actually
// path/spread for a meaningful stress test, not stack on one point.
static FAutoConsoleCommandWithWorldAndArgs CVarZSSpawnZombies(
	TEXT("ZS.SpawnZombies"),
	TEXT("B0-T12.1: spawns <n> zombies (AZSGameMode::StressTestZombieClass) scattered around the local player's pawn, for profiling. Usage: ZS.SpawnZombies <n>. Host-only."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		const AZSGameMode* GameMode = World ? World->GetAuthGameMode<AZSGameMode>() : nullptr;
		if (!GameMode)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.SpawnZombies: host-only, no authoritative AZSGameMode found"));
			return;
		}

		if (!GameMode->StressTestZombieClass)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.SpawnZombies: AZSGameMode::StressTestZombieClass is unset - assign a real BP_Zombie_* class (with a UZSZombieConfig on its CDO) first"));
			return;
		}

		int32 Count = (Args.Num() > 0) ? FCString::Atoi(*Args[0]) : 10;
		Count = FMath::Clamp(Count, 1, 500);

		const APlayerController* PC = World->GetFirstPlayerController();
		const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		const FVector Origin = Pawn ? Pawn->GetActorLocation() : FVector::ZeroVector;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		int32 SpawnedCount = 0;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			const FVector2D RandomOffset = FMath::RandPointInCircle(2000.f);
			const FVector SpawnLocation = Origin + FVector(RandomOffset.X, RandomOffset.Y, 0.f);

			if (World->SpawnActor<AZombieCharacter>(GameMode->StressTestZombieClass, SpawnLocation, FRotator::ZeroRotator, SpawnParams))
			{
				++SpawnedCount;
			}
		}

		UE_LOG(LogZombieShooter, Log, TEXT("ZS.SpawnZombies: spawned %d/%d zombies around %s"), SpawnedCount, Count, *Origin.ToString());
	}));

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
	DOREPLIFETIME(AZombieCharacter, bIsDowned);
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

	// B0-T5.1 fix: a blank FHitResult() has no BoneName, so AZSPlayerCharacter::TakeDamage's zone
	// inference (BodyZoneFromBoneName) always fell back to Torso - every zombie bite landed there
	// regardless of where the zombie actually was relative to the player. A target's capsule (not
	// its skeletal mesh) is usually what actually blocks a channel trace, so a capsule hit (no bone)
	// falls through to a direct geometric query against the target's own skeletal mesh component -
	// LineTraceComponent tests just that component's real collision, ignoring channel-response
	// settings entirely, so it finds a bone regardless of how the capsule/mesh are each configured.
	FHitResult HitResult;
	const FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, 40.f);
	const FVector TraceEnd = Target->GetActorLocation() + FVector(0.f, 0.f, 40.f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_Visibility, QueryParams);

	if (HitResult.BoneName.IsNone())
	{
		if (USkeletalMeshComponent* TargetMesh = Target->FindComponentByClass<USkeletalMeshComponent>())
		{
			FHitResult MeshHitResult;
			if (TargetMesh->LineTraceComponent(MeshHitResult, TraceStart, TraceEnd, FCollisionQueryParams(TEXT("ZombieBiteZoneTrace"), true)))
			{
				HitResult = MeshHitResult;
			}
		}
	}

	UGameplayStatics::ApplyPointDamage(Target, ZombieConfig->MeleeDamage, HitDirection, HitResult, GetController(), this, DamageTypeClass);
}

void AZombieCharacter::Server_EnterDownedState()
{
	if (!HasAuthority() || bIsDead || bIsDowned)
	{
		return;
	}

	bIsDowned = true;
	OnRep_IsDowned();

	GetCharacterMovement()->DisableMovement();

	if (AZombieAIController* ZombieController = Cast<AZombieAIController>(GetController()))
	{
		ZombieController->SetDowned(true);
	}

	const float RecoverySeconds = ZombieConfig ? ZombieConfig->DownedRecoverySeconds : 6.f;
	GetWorldTimerManager().SetTimer(DownedRecoveryTimerHandle, this, &AZombieCharacter::Server_ExitDownedState, RecoverySeconds, false);
}

void AZombieCharacter::Server_ExitDownedState()
{
	if (!HasAuthority() || !bIsDowned || bIsDead)
	{
		return;
	}

	bIsDowned = false;
	OnRep_IsDowned();

	GetCharacterMovement()->SetMovementMode(MOVE_Walking);

	if (AZombieAIController* ZombieController = Cast<AZombieAIController>(GetController()))
	{
		ZombieController->SetDowned(false);
	}
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

	// A zombie killed while downed (e.g. ranged fire, which has no downed-exclusion the way
	// PerformMeleeSwing does) shouldn't leave bIsDowned stuck true on the corpse forever - same
	// state-clear Server_ExitDownedState does, just as part of dying instead of recovering.
	if (bIsDowned)
	{
		bIsDowned = false;
		OnRep_IsDowned();
	}

	// A downed zombie finished off by the T10.6 finisher shouldn't still recover mid-corpse-linger.
	GetWorldTimerManager().ClearTimer(DownedRecoveryTimerHandle);

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

void AZombieCharacter::OnRep_IsDowned()
{
	OnDownedChanged.Broadcast(bIsDowned);
}
