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

		if (Target.Platform == UnrealTargetPlatform.Android)
		{
			// These font assets open project-relative files at runtime, including the CJK fallbacks.
			RuntimeDependencies.Add("$(ProjectDir)/Art/P3R/Fonts/*.ttf", StagedFileType.UFS);
			RuntimeDependencies.Add("$(ProjectDir)/Art/P3R/Fonts/*.otf", StagedFileType.UFS);
			RuntimeDependencies.Add("$(ProjectDir)/Art/Fonts/NotoColorEmoji.ttf", StagedFileType.UFS);
		}

		PrivateDependencyModuleNames.AddRange(new string[] { "DreamGUI", "DreamGUIInput", "DreamGUIControls", "DreamGUIExtensions", "DreamTween", "RHI", "Slate", "SlateCore", "DreamGUIRenderer" });
	}
}
