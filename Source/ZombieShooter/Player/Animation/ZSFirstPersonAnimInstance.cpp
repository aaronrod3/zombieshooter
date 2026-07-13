// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSFirstPersonAnimInstance.h"
#include "ZSPlayerCharacter.h"

void UZSFirstPersonAnimInstance::PullPerspectiveState(float DeltaSeconds)
{
	if (!CharacterOwner)
	{
		return;
	}

	// Real velocity-derived move input, not Infima's simulated-velocity demo hack (see
	// CoreLoopPlan.md Phase 2 "Key architecture decisions").
	const FVector LocalVelocity = CharacterOwner->GetActorRotation().UnrotateVector(CharacterOwner->GetVelocity());
	InputMoveVector = FVector2D(LocalVelocity.Y, LocalVelocity.X);

	bIsSprinting = CharacterOwner->IsSprinting();
	bIsWalking = !LocalVelocity.IsNearlyZero() && !bIsSprinting;

	Stance = CharacterOwner->GetStance();
	bAnimateCamera = CharacterOwner->GetAnimateCamera();

	AimDownSightsTransform = CharacterOwner->GetAimDownSightsTransform();
	CrouchTransform = CharacterOwner->GetCrouchTransform();
}
