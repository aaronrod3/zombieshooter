// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;
class UZSWeaponConfig;
class UDamageType;

/**
 *  Replaces Server_Fire's instant hitscan trace for weapons with a ProjectileClass configured
 *  (UZSWeaponConfig). Server-spawned at the muzzle, travels via UProjectileMovementComponent,
 *  and applies the same damage/knockback contract the hitscan path used (FireDamage/
 *  FireDamageTypeClass/FireKnockbackStrength) - just resolved on first blocking hit instead of
 *  instantly. The one actor in Weapons/ that legitimately needs movement replication
 *  (SetReplicateMovement(true)) - everything else there is either static once attached
 *  (AZSWeapon/AZSMagazine) or a one-shot spawn-and-forget pickup (AZSWorldItemActor).
 */
UCLASS()
class AZSProjectile : public AActor
{
	GENERATED_BODY()

public:
	AZSProjectile();

	/** Server-only. Call immediately after SpawnActor, before the projectile starts moving - seeds
	 * the damage/knockback contract from the same config fields Server_Fire's hitscan path used,
	 * plus this projectile's own cosmetic mesh and travel speed. Sets outgoing velocity from the
	 * projectile's current (spawn) forward vector, so the caller should spawn it already rotated
	 * toward the intended fire direction. */
	void InitializeProjectile(const UZSWeaponConfig* InConfig, AActor* InInstigatorActor, AController* InInstigatorController);

protected:
	UFUNCTION()
	void HandleHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, Category = "ZS|Projectile")
	TObjectPtr<USphereComponent> CollisionComponent;

	/** Purely cosmetic - CollisionComponent (the root sphere) is what actually drives hit detection, per CLAUDE.md's cosmetic-attachment-needs-NoCollision convention. */
	UPROPERTY(VisibleAnywhere, Category = "ZS|Projectile")
	TObjectPtr<UStaticMeshComponent> ProjectileMesh;

	UPROPERTY(VisibleAnywhere, Category = "ZS|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

private:
	UPROPERTY()
	TObjectPtr<AActor> InstigatorActorRef;

	UPROPERTY()
	TObjectPtr<AController> InstigatorControllerRef;

	float Damage = 0.f;
	float KnockbackStrength = 0.f;

	UPROPERTY()
	TSubclassOf<UDamageType> DamageTypeClass;

	/** Guards HandleHit against firing more than once (e.g. re-entrant hit events) before Destroy() actually removes the actor. */
	bool bHasHit = false;
};
