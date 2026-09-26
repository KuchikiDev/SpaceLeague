// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class MovementORAEditorTarget : TargetRules
{
	public MovementORAEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		bOverrideBuildEnvironment = true;
		WindowsPlatform.Compiler = WindowsCompiler.VisualStudio2026;

		ExtraModuleNames.AddRange( new string[] { "MovementORA" } );
	}
}


