// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class demo_ikTarget : TargetRules
{
	public demo_ikTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		// DefaultBuildSettings = BuildSettingsVersion.V5;
		// IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_5;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("demo_ik");
	}
}
