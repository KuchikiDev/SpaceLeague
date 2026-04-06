using UnrealBuildTool;
using System.IO;

public class MovementParadoxe : ModuleRules
{
	public MovementParadoxe(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Keep legacy folder layout (no Public/Private split yet) usable for includes like "SpaceLeague/...".
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
			"CableComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AssetRegistry"
		});
	}
}
