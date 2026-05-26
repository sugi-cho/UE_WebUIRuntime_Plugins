using UnrealBuildTool;

public class WebUI_NDI : ModuleRules
{
	public WebUI_NDI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"WebUIRuntime",
				"NDIIO",
				"RenderCore",
				"RHI"
			}
		);
	}
}
