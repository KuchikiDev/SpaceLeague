using UnrealBuildTool;
using System.IO;

public class MovementORA : ModuleRules
{
	public MovementORA(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;

		// Keep legacy folder layout (no Public/Private split yet) usable for includes like "ORA/...".
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory));

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"GameplayTags",
			"UMG",
			"SlateCore",
			"CableComponent",
			"ProceduralMeshComponent",
			"Niagara",
			"NiagaraCore",
			"HTTP",
			"Json",
			"JsonUtilities"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry",
			"GameplayVariables"
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicSystemLibraries.Add("Crypt32.lib");
		}
	}
}

