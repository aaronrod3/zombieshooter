// Copyright Epic Games, Inc. All Rights Reserved.

#include "ANS_ZS_LeftHandGrip.h"
#include "ZSAnimInstanceBase.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_ZS_LeftHandGrip::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (UZSAnimInstanceBase* AnimInstance = MeshComp ? Cast<UZSAnimInstanceBase>(MeshComp->GetAnimInstance()) : nullptr)
	{
		AnimInstance->UpdateLeftHandGrip(false, BlendOutSpeed);
	}
}

void UANS_ZS_LeftHandGrip::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (UZSAnimInstanceBase* AnimInstance = MeshComp ? Cast<UZSAnimInstanceBase>(MeshComp->GetAnimInstance()) : nullptr)
	{
		AnimInstance->UpdateLeftHandGrip(true, BlendReturnSpeed);
	}
}
