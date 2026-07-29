// Copyright Epic Games, Inc. All Rights Reserved.
//
// Test-only actor for ZSAutomationTests.cpp. UZSHealthComponent/UZSInventoryComponent need to be
// real constructor subobjects (CreateDefaultSubobject) for the engine's normal SpawnActor flow to
// call their BeginPlay() automatically - a component added to an already-spawned actor via
// NewObject+RegisterComponent does NOT get BeginPlay() called for free, and BeginPlay() itself is
// protected (not callable from outside the component), so this is the correct way to get real,
// fully-initialized components in a test, not a workaround. Not gated behind
// WITH_DEV_AUTOMATION_TESTS - UCLASS declarations don't play well with conditional compilation, and
// an unused, unreferenced actor class costs nothing meaningful in a Shipping build.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ZSTestHarnessActor.generated.h"

class UZSHealthComponent;
class UZSInventoryComponent;

UCLASS()
class AZSTestHarnessActor : public AActor
{
	GENERATED_BODY()

public:
	AZSTestHarnessActor();

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UZSHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UZSInventoryComponent> InventoryComponent;
};
