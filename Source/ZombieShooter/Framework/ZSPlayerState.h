// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ZSPlayerState.generated.h"

/**
 *  Per-player replicated data: health, ammo, kills. Health may move into a
 *  dedicated UZSHealthComponent once Phase 4 (Damage and Health) is reached —
 *  not decided yet, see docs/SessionHandoff.md.
 */
UCLASS()
class AZSPlayerState : public APlayerState
{
	GENERATED_BODY()

public:

	AZSPlayerState();
};
