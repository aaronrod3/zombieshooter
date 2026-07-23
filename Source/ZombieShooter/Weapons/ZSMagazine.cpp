// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMagazine.h"
#include "ZSWeaponConfig.h"
#include "Components/StaticMeshComponent.h"

AZSMagazine::AZSMagazine()
{
	PrimaryActorTick.bCanEverTick = false;

	MagazineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MagazineMesh"));
	SetRootComponent(MagazineMesh);
}

void AZSMagazine::InitializeFromConfig(UZSWeaponConfig* Config)
{
	if (!Config)
	{
		return;
	}

	WeaponConfig = Config;

	if (Config->MagazineMesh)
	{
		MagazineMesh->SetStaticMesh(Config->MagazineMesh);
	}
}
