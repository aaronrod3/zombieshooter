// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSAnimInstanceBase.h"
#include "ZSCharacterTypes.h"
#include "ZSFirstPersonAnimInstance.generated.h"

/**
 *  FP-only additions on top of UZSAnimInstanceBase's shared state: locomotion input,
 *  stance, camera-animate toggle, and the ADS/Crouch procedural transforms (TP doesn't need
 *  these per Infima guide 07's own variable table - only FP consumes them).
 */
UCLASS()
class UZSFirstPersonAnimInstance : public UZSAnimInstanceBase
{
	GENERATED_BODY()

protected:

	virtual void PullPerspectiveState(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	FVector2D InputMoveVector;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsWalking = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bIsSprinting = false;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	EZSStance Stance = EZSStance::Standing;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	bool bAnimateCamera = true;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	FTransform AimDownSightsTransform;

	UPROPERTY(BlueprintReadOnly, Category = "ZS|Animation")
	FTransform CrouchTransform;
};
