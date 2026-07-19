// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZSAnimInstanceBase.generated.h"

class AZSPlayerCharacter;

/**
 *  Native shared logic for the character AnimBP. Third-person only after the P0 de-scope
 *  (the UZSFirstPersonAnimInstance subclass and the FP AnimBP are removed - see
 *  Docs/GameDevPlan.md section 2); ABP_ZS_ThirdPerson parents directly to this class.
 *
 *  No BPI_TFA_AnimationState-equivalent interface (see CoreLoopPlan.md Phase 2 "Key
 *  architecture decisions") - UpdateLeftHandGrip is a plain virtual function, called directly
 *  from UANS_ZS_LeftHandGrip via a Cast<UZSAnimInstanceBase>. Both are native C++, so the
 *  interface indirection Infima's Blueprint version needs has no payoff here.
 */
UCLASS()
class UZSAnimInstanceBase : public UAnimInstance
{
	GENERATED_BODY()

public:

	/** Called by UANS_ZS_LeftHandGrip on NotifyBegin/NotifyEnd to smoothly blend the left-hand grip pose in/out. */
	UFUNCTION(BlueprintCallable, Category = "ZS|Animation")
	virtual void UpdateLeftHandGrip(bool bNewIsLeftHandOnWeapon, float BlendSpeed);

	/** BlueprintThreadSafe getter, not a raw BlueprintReadOnly property - AnimGraph fast-path nodes (e.g. BlendspacePlayer's BlendSpace pin) warn on thread-unsafe object-pointer access otherwise. */
	UFUNCTION(BlueprintPure, Category = "ZS|Animation", meta = (BlueprintThreadSafe))
	AZSPlayerCharacter* GetCharacterOwner() const { return CharacterOwner; }

protected:

	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	void UpdateGripAlpha(float DeltaSeconds);
	void UpdateLocomotionState();

	UPROPERTY()
	TObjectPtr<AZSPlayerCharacter> CharacterOwner;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsAiming = false;

	/** Movement state for the locomotion blend space(s) built in the P1 AnimGraph work - see
	 *  Docs/GameDevPlan.md section 5.1. GroundSpeed is planar (XY) velocity magnitude; Direction
	 *  is UKismetAnimationLibrary::CalculateDirection's standard -180..180 signed angle between
	 *  velocity and actor forward, the convention Lyra's own imported blend spaces (e.g.
	 *  BS_UnequippedIdleWalkRun) are built around. bIsFalling feeds the jump state. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float GroundSpeed = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float Direction = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsFalling = false;

	/** Mirrors AZSPlayerCharacter::GetStance() == EZSStance::Crouching - pulled once per frame like
	 *  every other locomotion flag on this class, rather than reached for from inside the AnimGraph. */
	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsCrouched = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsLeftHandOnWeapon = true;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float CurrentGripAlpha = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float TargetGripAlpha = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float GripPoseBlendSpeed = 15.f;
};
