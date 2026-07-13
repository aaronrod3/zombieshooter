// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSPhysicsObject.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class USoundBase;

/**
 *  Shared base for dropped/ejected physics props (magazines, bullet casings). Simulates
 *  physics on spawn, plays an impact sound on collision, and self-destroys after
 *  TimeUntilDestroyed seconds (0 = never).
 *
 *  Fixes a documented bug in the Infima pack's own BP_TFA_BasePhysicsObject: there,
 *  bPlaySoundOnce=false silently does nothing (the force-threshold branch was never wired
 *  up). Here both branches are functional.
 */
UCLASS()
class AZSPhysicsObject : public AActor
{
	GENERATED_BODY()

public:

	AZSPhysicsObject();

	/** Assigns the visual mesh and impact sound. Called by AZSWeapon right after SpawnActor. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Physics Object")
	void InitializeVisuals(UStaticMesh* Mesh, USoundBase* InImpactSound);

	/** Applies a launch impulse + angular impulse to PhysicsMesh. ImpulseDirection is normalized internally. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Physics Object")
	void ApplyLaunchImpulse(const FVector& ImpulseDirection, float ImpulseForce, float RotationSpeed);

protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void HandlePhysicsMeshHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PhysicsMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZS|Physics Object")
	TObjectPtr<USoundBase> ImpactSound;

	/** Seconds until this actor self-destroys. 0 = never destroyed. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZS|Physics Object")
	float TimeUntilDestroyed = 30.f;

	/** If true, ImpactSound plays once on the first qualifying hit. If false, it can play again on every hit that clears MinimumForceToPlaySound. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZS|Physics Object")
	bool bPlaySoundOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZS|Physics Object")
	float MinimumForceToPlaySound = 100.f;

private:

	bool bHasPlayedImpactSound = false;
};
