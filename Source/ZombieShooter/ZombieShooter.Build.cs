// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ZombieShooter : ModuleRules
{
	public ZombieShooter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"ZombieShooter",
			"ZombieShooter/Variant_Platforming",
			"ZombieShooter/Variant_Platforming/Animation",
			"ZombieShooter/Variant_Combat",
			"ZombieShooter/Variant_Combat/AI",
			"ZombieShooter/Variant_Combat/Animation",
			"ZombieShooter/Variant_Combat/Gameplay",
			"ZombieShooter/Variant_Combat/Interfaces",
			"ZombieShooter/Variant_Combat/UI",
			"ZombieShooter/Variant_SideScrolling",
			"ZombieShooter/Variant_SideScrolling/AI",
			"ZombieShooter/Variant_SideScrolling/Gameplay",
			"ZombieShooter/Variant_SideScrolling/Interfaces",
			"ZombieShooter/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
