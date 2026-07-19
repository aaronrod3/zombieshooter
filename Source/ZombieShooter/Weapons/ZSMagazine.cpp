// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMagazine.h"
#include "ZSWeaponConfig.h"
#include "Components/SkeletalMeshComponent.h"

AZSMagazine::AZSMagazine()
{
	PrimaryActorTick.bCanEverTick = false;

	MagazineMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("MagazineMesh"));
	SetRootComponent(MagazineMesh);
}

void AZSMagazine::InitializeFromConfig(UZSWeaponConfig* Config)
{
	if (!Config)
	{
		return;
	}

	WeaponConfig = Config;

	if (Config->MeshMagazineSK)
	{
		MagazineMesh->SetSkeletalMesh(Config->MeshMagazineSK);
	}
}
