// Copyright Epic Games, Inc. All Rights Reserved.

#include "AN_ZS_UnlockActions.h"
#include "ZSPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_ZS_UnlockActions::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AZSPlayerCharacter* Character = MeshComp ? Cast<AZSPlayerCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->SetBusy(false);
	}
}
