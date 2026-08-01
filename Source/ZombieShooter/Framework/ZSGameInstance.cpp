// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UZSGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UZSGameInstance::HandlePreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UZSGameInstance::HandlePostLoadMap);
}

void UZSGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	Super::Shutdown();
}

void UZSGameInstance::HostGame(FName MapName)
{
	UGameplayStatics::OpenLevel(this, MapName, true, TEXT("listen"));
}

void UZSGameInstance::JoinGame(const FString& IPAddress)
{
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
