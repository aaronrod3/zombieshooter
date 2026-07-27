// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSCameraDirector.h"

UZSCameraDirector::UZSCameraDirector()
{
	PrimaryComponentTick.bCanEverTick = false;
	CurrentArmLength = OutdoorDistance;
	ManualTargetArmLength = OutdoorDistance;
}

void UZSCameraDirector::TickZoom(float DeltaTime)
{
	const float Target = bManualOverrideActive ? ManualTargetArmLength : GetPresetDistanceForContext(GetActiveContext());
	CurrentArmLength = FMath::FInterpTo(CurrentArmLength, Target, DeltaTime, ZoomInterpSpeed);
}

void UZSCameraDirector::PushContext(EZSCameraContext Context)
{
	ContextStack.Add(Context);
	CheckOverrideResume();
}

void UZSCameraDirector::PopContext(EZSCameraContext Context)
{
	// LIFO removal of the most recent matching entry, not a blanket RemoveSingle-from-front - a
	// context can legitimately be pushed more than once (e.g. two overlapping interior volumes).
	for (int32 Index = ContextStack.Num() - 1; Index >= 0; --Index)
	{
		if (ContextStack[Index] == Context)
		{
			ContextStack.RemoveAt(Index);
			break;
		}
	}

	CheckOverrideResume();
}

void UZSCameraDirector::ApplyManualZoom(float Delta)
{
	if (FMath::IsNearlyZero(Delta))
	{
		return;
	}

	// First override this session/context - seed from wherever the camera actually is right now
	// (not the previous auto-zoom target), so the first scroll tick doesn't jump.
	if (!bManualOverrideActive)
	{
		ManualTargetArmLength = CurrentArmLength;
		bManualOverrideActive = true;
		ContextAtOverrideTime = GetActiveContext();
	}

	ManualTargetArmLength = FMath::Clamp(ManualTargetArmLength - Delta * CameraZoomStep, MinCameraDistance, MaxCameraDistance);
}

float UZSCameraDirector::GetPresetDistanceForContext(EZSCameraContext Context) const
{
	switch (Context)
	{
	case EZSCameraContext::Interior:
		return InteriorDistance;
	case EZSCameraContext::Underground:
		return UndergroundDistance;
	case EZSCameraContext::Driving:
		return DrivingDistance;
	case EZSCameraContext::Outdoor:
	default:
		return OutdoorDistance;
	}
}

void UZSCameraDirector::CheckOverrideResume()
{
	if (bManualOverrideActive && GetActiveContext() != ContextAtOverrideTime)
	{
		bManualOverrideActive = false;
	}
}
