// Copyright DreamMoon. All Rights Reserved.

using UnrealBuildTool;

/*
 * The demo's own code: the shot director and the NOCTURNE//OS interface. The P3R study is its own module
 * (P3RUIDemo); the NieR / Detroit style studies stay in the DevUITests plugin.
 */
public class DreamGUIDemo : ModuleRules
{
	public DreamGUIDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "DreamGUI", "DreamGUIInput", "DreamGUIControls", "DreamGUIExtensions", "DreamTween", "RHI", "Slate", "SlateCore", "DreamGUIRenderer" });
	}
}
