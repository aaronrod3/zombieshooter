// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSPhysicsObject.h"
#include "ZSPhysicsMagazine.generated.h"

/** A dropped, physics-simulated magazine. Mesh/sound are assigned by AZSWeapon::SpawnDroppedMagazine via InitializeVisuals. */
UCLASS()
class AZSPhysicsMagazine : public AZSPhysicsObject
{
	GENERATED_BODY()

public:

	AZSPhysicsMagazine();
};
