// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ZSWeaponTypes.h"
#include "ZSAnimInstanceBase.generated.h"

class AZSPlayerCharacter;

/**
 *  Native shared logic for both the FP and TP AnimBPs. Per Infima guide 06/07's own
 *  "Variables" tables, TP needs nothing beyond what's common to both perspectives, so this
 *  class is directly usable as the TP AnimGraph's parent class - only FP needs a dedicated
 *  subclass (UZSFirstPersonAnimInstance) for its extra locomotion/stance/ADS/crouch fields.
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

	/** Empty in the base class (TP needs nothing extra); overridden by UZSFirstPersonAnimInstance to pull FP-only state each frame. */
	virtual void PullPerspectiveState(float DeltaSeconds);

	void UpdateGripAlpha(float DeltaSeconds);

	UPROPERTY()
	TObjectPtr<AZSPlayerCharacter> CharacterOwner;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsAiming = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	EZSGripAttachment CurrentGrip = EZSGripAttachment::None;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	FTransform RecoilTransform;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsLeftHandOnWeapon = true;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float CurrentGripAlpha = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float TargetGripAlpha = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	float GripPoseBlendSpeed = 15.f;
};
