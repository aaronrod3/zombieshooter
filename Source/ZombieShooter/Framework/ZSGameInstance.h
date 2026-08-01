// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZSGameInstance.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnLoadingScreenShouldShow);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FZSOnLoadingScreenShouldHide);

/**
 *  B1-T8: greenfield main-menu/travel machinery - no UGameInstance subclass existed anywhere in
 *  this project before this. Lives here rather than on AZSGameMode/AZSPlayerController because a
 *  GameInstance is the one object that survives a level travel, which host/join fundamentally are.
 *
 *  T8.1's Steam friends-list invite is NOT built here - OnlineSubsystemSteam isn't wired into
 *  ZombieShooter.Build.cs at all (still commented out - "no dedicated-server packaging or online
 *  subsystem yet" per CLAUDE.md's Off-Limits) - enabling it is an infrastructure decision (a Steam
 *  AppID, plugin enablement) for the dev to make, not guessed past here. Host/Join by direct IP
 *  need no online subsystem at all - they're plain engine travel.
 */
UCLASS()
class UZSGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	virtual void Init() override;
	virtual void Shutdown() override;

	/** T8.1 "host": opens MapName as a listen server. Call from the main menu, not mid-session (a level change while already hosting is ServerTravel, not needed by any B1 task). */
	UFUNCTION(BlueprintCallable, Category = "ZS|MainMenu")
	void HostGame(FName MapName);

	/** T8.1 "join by IP": travels the local player's PlayerController to IPAddress (e.g. "127.0.0.1:7777"). No validation beyond what ClientTravel itself does - a malformed address just fails to connect, same as typing it into the console manually. */
	UFUNCTION(BlueprintCallable, Category = "ZS|MainMenu")
	void JoinGame(const FString& IPAddress);

	/** T8.4: bind to show a loading-screen widget the instant a level load begins. Fires from FCoreUObjectDelegates::PreLoadMap. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|MainMenu")
	FZSOnLoadingScreenShouldShow OnLoadingScreenShouldShow;

	/** T8.4: bind to hide the loading-screen widget the instant a level finishes loading. Fires from FCoreUObjectDelegates::PostLoadMapWithWorld. */
	UPROPERTY(BlueprintAssignable, Category = "ZS|MainMenu")
	FZSOnLoadingScreenShouldHide OnLoadingScreenShouldHide;

private:

	void HandlePreLoadMap(const FString& MapName);
	void HandlePostLoadMap(UWorld* LoadedWorld);
};
