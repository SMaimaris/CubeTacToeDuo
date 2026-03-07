// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CubeTacToe : ModuleRules
{
	public CubeTacToe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"UMG", "Slate", "SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "RHI" });

		// Ensure subdirectories (Game/, Actors/, Types/) are reachable via path-prefixed includes
		PrivateIncludePaths.Add(ModuleDirectory);
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
