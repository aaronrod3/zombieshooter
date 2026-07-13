// Copyright Epic Games, Inc. All Rights Reserved.

#include "AN_ZS_DropMagazine.h"
#include "ZSWeapon.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_ZS_DropMagazine::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AZSWeapon* Weapon = MeshComp ? Cast<AZSWeapon>(MeshComp->GetOwner()) : nullptr)
	{
		Weapon->SpawnDroppedMagazine(ImpulseForce, RotationForce);
	}
}
