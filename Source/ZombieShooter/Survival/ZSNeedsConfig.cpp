// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSNeedsConfig.h"
#include "Curves/CurveFloat.h"

FPrimaryAssetId UZSNeedsConfig::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ZSNeedsConfig"), GetFName());
}

float UZSNeedsConfig::EvaluatePerformanceCurve(const UCurveFloat* Curve, float NeedValue)
{
	if (!Curve)
	{
		return 1.f;
	}

	return FMath::Clamp(Curve->GetFloatValue(NeedValue), 0.f, 1.f);
}

int32 UZSNeedsConfig::GetSeverityTier(float NeedValue) const
{
	if (NeedValue > SeverityTier2Max)
	{
		return 0;
	}
	if (NeedValue > SeverityTier3Max)
	{
		return 1;
	}
	if (NeedValue > SeverityTier4Max)
	{
		return 2;
	}
	return 3;
}
