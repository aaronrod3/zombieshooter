// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ZSGameState.generated.h"

/**
 *  Replicated data relevant to every connected client, not specific to any
 *  one player. Deliberately near-empty for the core loop — real content
 *  (wave state, zone state, shared economy) is a separate future planning
 *  pass, see docs/SessionHandoff.md.
 */
UCLASS()
class AZSGameState : public AGameStateBase
{
	GENERATED_BODY()

public:

	AZSGameState();
};
