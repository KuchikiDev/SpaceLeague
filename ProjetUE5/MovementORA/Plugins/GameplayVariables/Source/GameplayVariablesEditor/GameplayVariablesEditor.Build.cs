using UnrealBuildTool;

public class GameplayVariablesEditor : ModuleRules
{
	public GameplayVariablesEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoPCHs;

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"PropertyEditor",
			"GameplayVariables",
			"UnrealEd"
		});
	}
}
