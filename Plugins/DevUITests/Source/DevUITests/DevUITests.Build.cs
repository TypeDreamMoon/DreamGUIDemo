using UnrealBuildTool;

public class DevUITests : ModuleRules
{
	public DevUITests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		// The UI style test screens (UITests/<Style>/): DreamGUI widgets, its input layer and its controls.
		PrivateDependencyModuleNames.AddRange(new string[] { "DreamGUI", "DreamGUIInput", "DreamGUIControls", "DreamTween", "Slate", "SlateCore" });
	}
}
