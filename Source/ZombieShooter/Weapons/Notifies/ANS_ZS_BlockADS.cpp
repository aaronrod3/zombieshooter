// Copyright Epic Games, Inc. All Rights Reserved.

#include "ANS_ZS_BlockADS.h"
#include "ZSPlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_ZS_BlockADS::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AZSPlayerCharacter* Character = MeshComp ? Cast<AZSPlayerCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->SetAimingBlocked(true);
		Character->ForceStopAiming();
	}
}

void UANS_ZS_BlockADS::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AZSPlayerCharacter* Character = MeshComp ? Cast<AZSPlayerCharacter>(MeshComp->GetOwner()) : nullptr)
	{
		Character->SetAimingBlocked(false);
	}
}
