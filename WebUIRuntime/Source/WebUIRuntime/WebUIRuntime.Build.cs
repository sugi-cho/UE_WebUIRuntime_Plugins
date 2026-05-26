using UnrealBuildTool;

public class WebUIRuntime : ModuleRules
{
	public WebUIRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"EngineSettings",
				"ImageWrapper",
				"Json",
				"DeveloperSettings",
				"UMG",
				"WebBrowserWidget"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"HTTPServer",
				"JsonUtilities",
				"RenderCore",
				"RHI",
				"WebSocketNetworking",
				"Slate",
				"SlateCore"
			}
		);

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"UnrealEd",
					"BlueprintGraph"
				}
			);
		}
	}
}
