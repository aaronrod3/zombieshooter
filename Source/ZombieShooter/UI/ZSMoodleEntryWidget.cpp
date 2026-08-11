// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMoodleEntryWidget.h"
#include "Components/ProgressBar.h"

void UZSMoodleEntryWidget::RefreshFromEntry(const FZSMoodleEntry& Entry)
{
	if (ProgressBar_Severity)
	{
		ProgressBar_Severity->SetPercent(FMath::Clamp(Entry.Value / 100.f, 0.f, 1.f));
		if (SeverityColors.IsValidIndex(Entry.SeverityTier))
		{
			ProgressBar_Severity->SetFillColorAndOpacity(SeverityColors[Entry.SeverityTier]);
		}
	}

	if (SeverityPulse && Entry.SeverityTier == 3)
	{
		PlayAnimation(SeverityPulse);
	}
}
