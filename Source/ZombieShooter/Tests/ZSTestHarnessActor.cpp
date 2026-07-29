// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSTestHarnessActor.h"
#include "ZSHealthComponent.h"
#include "ZSInventoryComponent.h"

AZSTestHarnessActor::AZSTestHarnessActor()
{
	HealthComponent = CreateDefaultSubobject<UZSHealthComponent>(TEXT("HealthComponent"));
	InventoryComponent = CreateDefaultSubobject<UZSInventoryComponent>(TEXT("InventoryComponent"));
}
