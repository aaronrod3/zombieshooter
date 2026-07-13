// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "ANS_ZS_HideMainMag.generated.h"

/** Placed on a weapon-mesh reload montage section where the main magazine should be hidden (e.g. mid-swap). */
UCLASS()
class UANS_ZS_HideMainMag : public UAnimNotifyState
{
	GENERATED_BODY()

public:

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
