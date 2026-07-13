// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ZS_DropMagazine.generated.h"

/** Placed on a weapon-mesh reload montage. Calls AZSWeapon::SpawnDroppedMagazine on the notify's owning weapon. */
UCLASS()
class UAN_ZS_DropMagazine : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float ImpulseForce = -50.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float RotationForce = 150.f;
};
