using UnrealBuildTool;

public class WebUIRuntimeEditor : ModuleRules
{
	public WebUIRuntimeEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"WebUIRuntime"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"AssetRegistry",
				"EditorStyle",
				"ContentBrowser",
				"Projects",
				"LevelEditor",
				"Slate",
				"SlateCore",
				"ToolMenus",
				"UnrealEd",
				"BlueprintGraph",
				"KismetCompiler"
			}
		);
	}
}
