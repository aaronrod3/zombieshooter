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
			"GameplayTasks",
			"UMG",
			"Slate",
			"PhysicsCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AnimGraphRuntime" });

		PublicIncludePaths.AddRange(new string[] {
			"ZombieShooter",
			"ZombieShooter/Framework",
			"ZombieShooter/Player",
			"ZombieShooter/Player/Animation",
			"ZombieShooter/Combat",
			"ZombieShooter/Weapons",
			"ZombieShooter/Weapons/Notifies",
			"ZombieShooter/Zombies",
			"ZombieShooter/Interaction",
			"ZombieShooter/Survival"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
