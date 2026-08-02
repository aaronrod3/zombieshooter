// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSMoodleStackWidget.generated.h"

class UZSMoodleEntryWidget;

/** B1-T3.1/T5.1, 2026-08-02 C++ conversion: Hunger/Thirst/Fatigue/Stamina/Temperature at a glance. Lives in T5's Inventory panel, not the HUD (2026-08-01 redesign). Replaces WBP_ZS_MoodleStack's Graph tab - the 5 children are placed by hand in Designer, not spawned, since the set of needs is fixed. */
UCLASS()
class UZSMoodleStackWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	/** Fixed order, matches UZSNeedsComponent::GetMoodleEntries()'s own guaranteed order exactly. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UZSMoodleEntryWidget> MoodleEntry_Hunger;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UZSMoodleEntryWidget> MoodleEntry_Thirst;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UZSMoodleEntryWidget> MoodleEntry_Fatigue;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UZSMoodleEntryWidget> MoodleEntry_Stamina;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UZSMoodleEntryWidget> MoodleEntry_Temperature;

private:

	UFUNCTION()
	void RefreshMoodleStack();
};
