// Copyright DreamMoon. All Rights Reserved.

using UnrealBuildTool;

/*
 * The style study's own code: the flow director, the screens behind each .dui, and the backdrop that every menu
 * screen sits on.
 */
public class P3RUIDemo : ModuleRules
{
	public P3RUIDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] {
			"DreamGUI", "DreamGUIInput", "DreamGUIControls", "DreamGUIExtensions", "DreamGUIRenderer", "DreamGUINiagara",
			"DreamTween", "Niagara", "RHI", "Slate", "SlateCore" });
	}
}
