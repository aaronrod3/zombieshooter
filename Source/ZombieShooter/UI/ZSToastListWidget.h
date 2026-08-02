// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSUserWidgetBase.h"
#include "ZSNotificationSubsystem.h"
#include "ZSToastListWidget.generated.h"

class UVerticalBox;
class UZSToastEntryWidget;

/** B1-T3.10, 2026-08-02 C++ conversion: queued toast list, top-right. Replaces WBP_ZS_ToastList's Graph tab - spawn/fade-in/wait/fade-out/dismiss is a timer chain, more natural in C++ than a Delay-node chain in Blueprint. */
UCLASS()
class UZSToastListWidget : public UZSUserWidgetBase
{
	GENERATED_BODY()

protected:

	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UVerticalBox> VBox_Toasts;

	/** Assign WBP_ZS_ToastEntry (or a Blueprint child of it) on this Blueprint's Class Defaults. */
	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI")
	TSubclassOf<UZSToastEntryWidget> ToastEntryClass;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI", meta = (ClampMin = "0"))
	float DisplaySeconds = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "ZS|UI", meta = (ClampMin = "0"))
	float FadeOutSeconds = 0.4f;

private:

	UFUNCTION()
	void OnToastReceived(FZSToastEntry NewToast);

	void BeginFadeOut(TWeakObjectPtr<UZSToastEntryWidget> EntryWidget, FGuid ToastId);
	void DismissAndRemove(TWeakObjectPtr<UZSToastEntryWidget> EntryWidget, FGuid ToastId);
};
