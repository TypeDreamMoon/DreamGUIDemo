// Copyright DreamMoon. All Rights Reserved.

using UnrealBuildTool;

public class DreamGUIDemoTarget : TargetRules
{
	public DreamGUIDemoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.AddRange(new string[] { "DreamGUIDemo", "P3RUIDemo" });
	}
}
