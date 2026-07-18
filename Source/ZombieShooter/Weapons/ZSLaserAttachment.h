// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSLaserAttachment.generated.h"

class UStaticMeshComponent;
class UStaticMesh;
class UPointLightComponent;
class USkeletalMeshComponent;
class UZSWeaponConfig;

/**
 *  Weapon-mounted laser sight. Traces from the owning weapon's SocketLaserStart every tick,
 *  scaling a beam mesh and repositioning a point light to the hit point. Only spawned by
 *  AZSWeapon when SocketLaserAttachment exists on the receiver mesh.
 */
UCLASS()
class AZSLaserAttachment : public AActor
{
	GENERATED_BODY()

public:

	AZSLaserAttachment();

	/** Assigns visuals from the config and caches the receiver mesh to trace SocketLaserStart from each tick. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Laser Attachment")
	void InitializeFromConfig(UZSWeaponConfig* Config, USkeletalMeshComponent* InReceiverMesh);

	/** Marks LaserBodyMesh/LaserBeamMesh SetOwnerNoSee - see AZSWeapon::SetHiddenFromOwner. Does NOT
	 *  cover LaserLight: UPointLightComponent derives from ULightComponentBase, not
	 *  UPrimitiveComponent, so SetOwnerNoSee isn't available on it - a small, accepted residual gap
	 *  (the light itself may still be visible to the owner in FirstPerson view, without its body/beam
	 *  meshes). */
	UFUNCTION(BlueprintCallable, Category = "ZS|Laser Attachment")
	void SetHiddenFromOwner(bool bHideFromOwner);

protected:

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LaserBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> LaserBeamMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPointLightComponent> LaserLight;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Laser Attachment")
	TObjectPtr<UZSWeaponConfig> WeaponConfig;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ZS|Laser Attachment")
	float MaxTraceDistance = 15000.f;

	/**
	 *  Beam static mesh, assumed authored as a unit-length (1cm) cylinder along +X so
	 *  SetWorldScale3D(X=Distance) stretches it to the hit point. Not part of UZSWeaponConfig
	 *  since Infima's own equivalent (SM_TFA_LaserBeam) is a shared asset across all weapons,
	 *  not per-weapon. Left unset until content is assigned (M7).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ZS|Laser Attachment")
	TObjectPtr<UStaticMesh> LaserBeamMeshAsset;

private:

	TWeakObjectPtr<USkeletalMeshComponent> ReceiverMesh;
};
