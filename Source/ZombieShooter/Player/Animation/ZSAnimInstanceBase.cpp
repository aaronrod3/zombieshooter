// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSAnimInstanceBase.h"
#include "ZSPlayerCharacter.h"
#include "ZSWeapon.h"
#include "Kismet/KismetMathLibrary.h"

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
	RecoilTransform = CharacterOwner->GetRecoilTransform();

	if (AZSWeapon* Weapon = CharacterOwner->GetCurrentWeapon())
	{
		CurrentGrip = Weapon->GetCurrentGrip();
	}

	PullPerspectiveState(DeltaSeconds);
	UpdateGripAlpha(DeltaSeconds);
}

void UZSAnimInstanceBase::PullPerspectiveState(float DeltaSeconds)
{
	// Empty in the base class - TP needs nothing beyond the state pulled above.
	// See UZSFirstPersonAnimInstance for the FP-only additions.
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
