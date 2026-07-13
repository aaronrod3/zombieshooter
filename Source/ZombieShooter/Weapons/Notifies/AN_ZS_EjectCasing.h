// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ZS_EjectCasing.generated.h"

/** Placed on a weapon-mesh fire montage. Calls AZSWeapon::EjectCasing at the notify's owning weapon's SocketCasingEject. */
UCLASS()
class UAN_ZS_EjectCasing : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float MinEjectForce = 50.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float MaxEjectForce = 65.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float RotationSpeed = 0.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	FRotator MinEjectRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	FRotator MaxEjectRotationOffset = FRotator::ZeroRotator;
};
