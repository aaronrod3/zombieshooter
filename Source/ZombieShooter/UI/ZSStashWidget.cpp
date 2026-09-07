// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSStashWidget.h"
#include "ZSStashItemEntryWidget.h"
#include "../Framework/ZSPlayerState.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"

namespace
{
	// A hub-browsing player has no pawn at all (OQ-BH-01) - GetOwningZSPlayerCharacter() would
	// always return null here, so every hub screen resolves its player's identity through the
	// PlayerController -> PlayerState chain directly instead.
	AZSPlayerState* ResolveOwningZSPlayerState(const UUserWidget* Widget)
	{
		const APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr;
		return PC ? Cast<AZSPlayerState>(PC->PlayerState) : nullptr;
	}
}

void UZSStashWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AZSPlayerState* PS = ResolveOwningZSPlayerState(this))
	{
		PS->OnStashChanged.AddDynamic(this, &UZSStashWidget::RefreshStash);
		PS->OnCurrencyChanged.AddDynamic(this, &UZSStashWidget::RefreshStash);
	}

	BuildGrid();
	RefreshStash();
}

void UZSStashWidget::BuildGrid()
{
	if (!Grid_StashItems || !StashEntryClass || EntryWidgets.Num() > 0)
	{
		return;
	}

	for (int32 Row = 0; Row < GridRows; ++Row)
	{
		for (int32 Col = 0; Col < GridColumns; ++Col)
		{
			UZSStashItemEntryWidget* Entry = CreateWidget<UZSStashItemEntryWidget>(this, StashEntryClass);
			if (!Entry)
			{
				continue;
			}
			// View-only for now - see this class's own header comment on why withdraw isn't wired yet.
			Entry->SetActionLabel(FText::GetEmpty());
			Grid_StashItems->AddChildToUniformGrid(Entry, Row, Col);
			EntryWidgets.Add(Entry);
		}
	}
}

void UZSStashWidget::RefreshStash()
{
	AZSPlayerState* PS = ResolveOwningZSPlayerState(this);
	if (!PS)
	{
		return;
	}

	if (Text_Currency)
	{
		Text_Currency->SetText(FText::AsNumber(PS->GetCurrency()));
	}

	const TArray<FZSItemInstance> Stash = PS->GetStashContents();
	for (int32 i = 0; i < EntryWidgets.Num(); ++i)
	{
		UZSStashItemEntryWidget* Entry = EntryWidgets[i];
		if (!Entry)
		{
			continue;
		}
		Entry->Instance = Stash.IsValidIndex(i) ? Stash[i] : FZSItemInstance();
		Entry->RefreshFromInstance();
	}
}

void UZSStashWidget::OpenAsModal()
{
	AddToViewport();
	PushAsModal(FName("Stash"));
}

void UZSStashWidget::CloseAsModal()
{
	PopAsModal(FName("Stash"));
	RemoveFromParent();
}
