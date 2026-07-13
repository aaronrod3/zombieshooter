// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AN_ZS_UnlockActions.generated.h"

/** Placed on a character-mesh montage (reload/inspect/mag-check/etc). Clears the owning AZSPlayerCharacter's bIsBusy flag - the only thing this notify does. */
UCLASS()
class UAN_ZS_UnlockActions : public UAnimNotify
{
	GENERATED_BODY()

public:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
};
