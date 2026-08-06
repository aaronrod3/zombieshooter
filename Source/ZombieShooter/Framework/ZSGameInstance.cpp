// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Blueprint/UserWidget.h"

void UZSGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UZSGameInstance::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UZSGameInstance::HandlePostLoadMap);

	// B1, 2026-08-05: both created via the UGameInstance-owning CreateWidget overload rather than
	// GetFirstLocalPlayerController(), which isn't guaranteed to resolve yet this early in the boot
	// sequence. Content-gap-safe if either TSubclassOf is unset - the null check just skips creation.
	if (LoadingScreenClass)
	{
		LoadingScreenRef = CreateWidget<UUserWidget>(this, LoadingScreenClass);
	}

	if (MainMenuScreenClass)
	{
		MainMenuScreenRef = CreateWidget<UUserWidget>(this, MainMenuScreenClass);
		if (MainMenuScreenRef)
		{
			MainMenuScreenRef->AddToViewport();
		}
	}
}

void UZSGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

void UZSGameInstance::HostGame(FName MapName)
{
	if (MainMenuScreenRef)
	{
		MainMenuScreenRef->RemoveFromParent();
	}

	UGameplayStatics::OpenLevel(this, MapName, true, TEXT("listen"));
}

void UZSGameInstance::JoinGame(const FString& IPAddress)
{
	if (MainMenuScreenRef)
	{
		MainMenuScreenRef->RemoveFromParent();
	}

	if (APlayerController* PC = GetFirstLocalPlayerController())
	{
		PC->ClientTravel(IPAddress, ETravelType::TRAVEL_Absolute);
	}
}

void UZSGameInstance::HandlePreLoadMap(const FString& MapName)
{
	OnLoadingScreenShouldShow.Broadcast();
}

void UZSGameInstance::HandlePostLoadMap(UWorld* LoadedWorld)
{
	OnLoadingScreenShouldHide.Broadcast();
}
