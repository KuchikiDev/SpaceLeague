// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MovementORAServerTarget : TargetRules
{
	public MovementORAServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			WindowsPlatform.Compiler = WindowsCompiler.VisualStudio2026;
		}

		ExtraModuleNames.AddRange(new string[] { "MovementORA" });
	}
}
