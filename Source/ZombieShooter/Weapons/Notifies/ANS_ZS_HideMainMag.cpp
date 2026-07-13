// Copyright Epic Games, Inc. All Rights Reserved.

#include "ANS_ZS_HideMainMag.h"
#include "ZSWeapon.h"
#include "Components/SkeletalMeshComponent.h"

void UANS_ZS_HideMainMag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AZSWeapon* Weapon = MeshComp ? Cast<AZSWeapon>(MeshComp->GetOwner()) : nullptr)
	{
		Weapon->SetMagazineVisibility(false, false);
	}
}

void UANS_ZS_HideMainMag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AZSWeapon* Weapon = MeshComp ? Cast<AZSWeapon>(MeshComp->GetOwner()) : nullptr)
	{
		Weapon->SetMagazineVisibility(true, false);
	}
}
