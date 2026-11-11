// Copyright DreamMoon. All Rights Reserved.

using UnrealBuildTool;

/*
 * Shared build environment on the installed engine: only this project's modules and its plugins
 * (DreamGUI, DreamShader, DreamFX as git worktrees, DevUITests copied in) are compiled.
 */
public class DreamGUIDemoEditorTarget : TargetRules
{
	public DreamGUIDemoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		BuildEnvironment = TargetBuildEnvironment.Shared;
		ExtraModuleNames.AddRange(new string[] { "DreamGUIDemo", "P3RUIDemo", "DreamGUIDemoEditor" });
	}
}
