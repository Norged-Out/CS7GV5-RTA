// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class demo_ik : ModuleRules
{
	public demo_ik(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
