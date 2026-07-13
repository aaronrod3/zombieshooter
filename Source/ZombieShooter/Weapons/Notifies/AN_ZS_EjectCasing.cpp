// Copyright Epic Games, Inc. All Rights Reserved.

#include "AN_ZS_EjectCasing.h"
#include "ZSWeapon.h"
#include "Components/SkeletalMeshComponent.h"

void UAN_ZS_EjectCasing::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (AZSWeapon* Weapon = MeshComp ? Cast<AZSWeapon>(MeshComp->GetOwner()) : nullptr)
	{
		const FRotator RandomOffset(
			FMath::RandRange(MinEjectRotationOffset.Pitch, MaxEjectRotationOffset.Pitch),
			FMath::RandRange(MinEjectRotationOffset.Yaw, MaxEjectRotationOffset.Yaw),
			FMath::RandRange(MinEjectRotationOffset.Roll, MaxEjectRotationOffset.Roll));

		Weapon->EjectCasing(RandomOffset, MinEjectForce, MaxEjectForce, RotationSpeed);
	}
}
