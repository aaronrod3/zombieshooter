// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ZS_LeftHandGrip.generated.h"

/**
 *  Placed on a character-mesh montage section where the left hand should come off the weapon
 *  grip (e.g. during a reload). Drives CurrentGripAlpha on the mesh's UZSAnimInstanceBase via
 *  UpdateLeftHandGrip.
 */
UCLASS()
class UANS_ZS_LeftHandGrip : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float BlendOutSpeed = 15.f;

	UPROPERTY(EditAnywhere, Category = "ZS|Notify")
	float BlendReturnSpeed = 15.f;
};
