// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMoodleStackWidget.h"
#include "ZSMoodleEntryWidget.h"
#include "../Survival/ZSNeedsComponent.h"

void UZSMoodleStackWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UZSNeedsComponent* Needs = GetOwningNeedsComponent())
	{
		Needs->OnMoodleStackChanged.AddUniqueDynamic(this, &UZSMoodleStackWidget::RefreshMoodleStack);
	}

	RefreshMoodleStack();
}

void UZSMoodleStackWidget::RefreshMoodleStack()
{
	UZSNeedsComponent* Needs = GetOwningNeedsComponent();
	if (!Needs)
	{
		return;
	}

	const TArray<FZSMoodleEntry> Entries = Needs->GetMoodleEntries();
	const TObjectPtr<UZSMoodleEntryWidget> OrderedChildren[] = { MoodleEntry_Hunger, MoodleEntry_Thirst, MoodleEntry_Fatigue, MoodleEntry_Stamina, MoodleEntry_Temperature };

	for (int32 Index = 0; Index < Entries.Num() && Index < UE_ARRAY_COUNT(OrderedChildren); ++Index)
	{
		if (OrderedChildren[Index])
		{
			OrderedChildren[Index]->RefreshFromEntry(Entries[Index]);
		}
	}
}
