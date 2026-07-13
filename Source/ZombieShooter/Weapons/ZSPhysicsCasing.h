// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ZSPhysicsObject.h"
#include "ZSPhysicsCasing.generated.h"

/** A dropped, physics-simulated bullet casing. Mesh/sound are assigned by AZSWeapon::EjectCasing via InitializeVisuals. */
UCLASS()
class AZSPhysicsCasing : public AZSPhysicsObject
{
	GENERATED_BODY()

public:

	AZSPhysicsCasing();
};
