// Copyright Epic Games, Inc. All Rights Reserved.

#include "ZSMagazine.h"
#include "ZSWeaponConfig.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"

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

	if (Config->ABP_Magazine)
	{
		MagazineMesh->SetAnimInstanceClass(Config->ABP_Magazine);
	}

	BulletMeshComponents.Empty();

	if (!Config->MeshBullet || Config->PrefixBulletSocket.IsNone())
	{
		return;
	}

	const FString BulletSocketPrefix = Config->PrefixBulletSocket.ToString();

	for (const FName& SocketName : MagazineMesh->GetAllSocketNames())
	{
		if (!SocketName.ToString().StartsWith(BulletSocketPrefix))
		{
			continue;
		}

		UStaticMeshComponent* BulletMesh = NewObject<UStaticMeshComponent>(this);
		BulletMesh->SetupAttachment(MagazineMesh, SocketName);
		BulletMesh->SetStaticMesh(Config->MeshBullet);
		BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BulletMesh->RegisterComponent();

		BulletMeshComponents.Add(BulletMesh);
	}
}

void AZSMagazine::SetHiddenFromOwner(bool bHideFromOwner)
{
	MagazineMesh->SetOwnerNoSee(bHideFromOwner);

	for (UStaticMeshComponent* BulletMesh : BulletMeshComponents)
	{
		if (BulletMesh)
		{
			BulletMesh->SetOwnerNoSee(bHideFromOwner);
		}
	}
}
