// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSAnimInstanceBase.h"
#include "ZSPlayerCharacter.h"

void UZSAnimInstanceBase::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CharacterOwner = Cast<AZSPlayerCharacter>(TryGetPawnOwner());
}

void UZSAnimInstanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CharacterOwner)
	{
		CharacterOwner = Cast<AZSPlayerCharacter>(TryGetPawnOwner());
	}

	if (!CharacterOwner)
	{
		return;
	}

	bIsAiming = CharacterOwner->IsAiming();

	UpdateGripAlpha(DeltaSeconds);
}

void UZSAnimInstanceBase::UpdateGripAlpha(float DeltaSeconds)
{
	CurrentGripAlpha = FMath::FInterpTo(CurrentGripAlpha, TargetGripAlpha, DeltaSeconds, GripPoseBlendSpeed);
}

void UZSAnimInstanceBase::UpdateLeftHandGrip(bool bNewIsLeftHandOnWeapon, float BlendSpeed)
{
	bIsLeftHandOnWeapon = bNewIsLeftHandOnWeapon;
	TargetGripAlpha = bNewIsLeftHandOnWeapon ? 1.f : 0.f;
	GripPoseBlendSpeed = BlendSpeed;
}
