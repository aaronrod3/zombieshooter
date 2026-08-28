// Copyright Epic Games, Inc. All Rights Reserved.

// B1, 2026-08-05: ZS.UI.Toggle* debug console commands, one per WBP_ZS_* screen that has no
// standalone production hotkey (Inventory/PauseMenu got real Tab/Escape binds instead - see
// AZSPlayerCharacter::ToggleInventoryScreen/TogglePauseMenuScreen). These exist purely so every
// screen built this session can actually be looked at in PIE without needing a human to force the
// real trigger condition (die, get amputated, host a game, find a container) - same "no PIE-input
// automation path exists, give the dev a console command instead" reasoning as ZS.SpawnZombies.
//
// Where a real production delegate already exists (UZSHealthComponent::OnDeath,
// UZSHealthComponent::OnDownedChanged, UZSGameInstance::OnLoadingScreenShouldShow/Hide - all
// wired and correct, confirmed 2026-08-05), these commands broadcast that SAME delegate rather than
// reaching into widget internals, so a debug toggle exercises the exact code path a real trigger
// would.
//
// 2026-08-11: ZS.UI.ToggleBlackoutOverlay removed along with WBP_ZS_BlackoutOverlay/
// UZSBlackoutOverlayWidget - the downed-state overlay was removed entirely per dev instruction,
// not renamed. Downed state currently has no dedicated UI feedback.
//
// ContainerLoot's own real trigger (2026-08-05): interacting with a real AZSContainerActor now
// opens this same screen (AZSPlayerCharacter::Client_OpenContainerLoot) instead of auto-looting -
// the container-interact UX question is resolved. ZS.UI.ToggleContainerLoot below stays useful for
// a layout/style check without needing a real container nearby, it's just no longer the only path.

#include "CoreMinimal.h"
#include "HAL/IConsoleManager.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "ZSContainerLootWidget.h"
#include "../Player/ZSPlayerCharacter.h"
#include "../Combat/ZSHealthComponent.h"
#include "../Framework/ZSGameInstance.h"
#include "../ZombieShooter.h"

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIToggleMainMenu(
	TEXT("ZS.UI.ToggleMainMenu"),
	TEXT("B1 debug: shows/hides WBP_ZS_MainMenu for visual verification in PIE (creates one instance the first time). Usage: ZS.UI.ToggleMainMenu"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		static TWeakObjectPtr<UUserWidget> DebugWidget;

		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleMainMenu: no local PlayerController found"));
			return;
		}

		if (DebugWidget.IsValid() && DebugWidget->IsInViewport())
		{
			DebugWidget->RemoveFromParent();
			return;
		}

		if (!DebugWidget.IsValid())
		{
			UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ZS/UI/MainMenu/WBP_ZS_MainMenu.WBP_ZS_MainMenu_C"));
			if (!WidgetClass)
			{
				UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleMainMenu: WBP_ZS_MainMenu not found at /Game/ZS/UI/MainMenu/ - has it been built/moved?"));
				return;
			}
			DebugWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
		}

		if (DebugWidget.IsValid())
		{
			DebugWidget->AddToViewport();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIToggleLoadingScreen(
	TEXT("ZS.UI.ToggleLoadingScreen"),
	TEXT("B1 debug: shows/hides WBP_ZS_LoadingScreen by broadcasting UZSGameInstance's real OnLoadingScreenShouldShow/Hide delegates (creates one instance the first time, exercising the same show/hide path a real level load uses). Usage: ZS.UI.ToggleLoadingScreen"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		static TWeakObjectPtr<UUserWidget> DebugWidget;
		static bool bShowing = false;

		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		UZSGameInstance* GI = World ? World->GetGameInstance<UZSGameInstance>() : nullptr;
		if (!PC || !GI)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleLoadingScreen: no local PlayerController/UZSGameInstance found"));
			return;
		}

		if (!DebugWidget.IsValid())
		{
			UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ZS/UI/MainMenu/WBP_ZS_LoadingScreen.WBP_ZS_LoadingScreen_C"));
			if (!WidgetClass)
			{
				UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleLoadingScreen: WBP_ZS_LoadingScreen not found at /Game/ZS/UI/MainMenu/ - has it been built/moved?"));
				return;
			}
			// Binds to GI's OnLoadingScreenShouldShow/Hide in its own NativeConstruct - nothing else to do here.
			DebugWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
		}

		bShowing = !bShowing;
		if (bShowing)
		{
			GI->OnLoadingScreenShouldShow.Broadcast();
		}
		else
		{
			GI->OnLoadingScreenShouldHide.Broadcast();
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIToggleDeathScreen(
	TEXT("ZS.UI.ToggleDeathScreen"),
	TEXT("B1 debug: shows WBP_ZS_DeathScreen by broadcasting the local pawn's real UZSHealthComponent::OnDeath delegate (creates one instance the first time). Cause-of-death text reflects whatever's currently in GetLastDeathInfo(), which is blank/default if the player hasn't actually died - that's expected, this is a visual/layout check, not a real death. The screen removes itself automatically after its respawn countdown, same as a real death. Usage: ZS.UI.ToggleDeathScreen"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		static TWeakObjectPtr<UUserWidget> DebugWidget;

		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		AZSPlayerCharacter* Character = PC ? Cast<AZSPlayerCharacter>(PC->GetPawn()) : nullptr;
		UZSHealthComponent* Health = Character ? Character->GetHealthComponent() : nullptr;
		if (!PC || !Health)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleDeathScreen: no local pawn/UZSHealthComponent found"));
			return;
		}

		if (!DebugWidget.IsValid())
		{
			UClass* WidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/ZS/UI/DeathRespawnSleep/WBP_ZS_DeathScreen.WBP_ZS_DeathScreen_C"));
			if (!WidgetClass)
			{
				UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleDeathScreen: WBP_ZS_DeathScreen not found at /Game/ZS/UI/DeathRespawnSleep/ - has it been built/moved?"));
				return;
			}
			// Binds to Health->OnDeath in its own NativeConstruct - nothing else to do here.
			DebugWidget = CreateWidget<UUserWidget>(PC, WidgetClass);
		}

		Health->OnDeath.Broadcast();
	}));

static FAutoConsoleCommandWithWorldAndArgs CVarZSUIToggleContainerLoot(
	TEXT("ZS.UI.ToggleContainerLoot"),
	TEXT("B1 debug: opens/closes WBP_ZS_ContainerLoot for visual verification in PIE without needing a real container nearby (creates one instance the first time, no container attached - Grid_ContainerItems stays empty, that's expected). 2026-08-05: interacting with a real AZSContainerActor now opens this same screen for real (AZSPlayerCharacter::Client_OpenContainerLoot) - this command is for layout/style checks only, not the only way to see it anymore. Usage: ZS.UI.ToggleContainerLoot"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		static TWeakObjectPtr<UZSContainerLootWidget> DebugWidget;

		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (!PC)
		{
			UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleContainerLoot: no local PlayerController found"));
			return;
		}

		if (DebugWidget.IsValid() && DebugWidget->IsInViewport())
		{
			DebugWidget->CloseAsModal();
			return;
		}

		if (!DebugWidget.IsValid())
		{
			UClass* WidgetClass = LoadClass<UZSContainerLootWidget>(nullptr, TEXT("/Game/ZS/UI/Container/WBP_ZS_ContainerLoot.WBP_ZS_ContainerLoot_C"));
			if (!WidgetClass)
			{
				UE_LOG(LogZombieShooter, Warning, TEXT("ZS.UI.ToggleContainerLoot: WBP_ZS_ContainerLoot not found at /Game/ZS/UI/Container/ - has it been built/moved?"));
				return;
			}
			DebugWidget = CreateWidget<UZSContainerLootWidget>(PC, WidgetClass);
		}

		if (DebugWidget.IsValid())
		{
			DebugWidget->OpenAsModal();
		}
	}));
