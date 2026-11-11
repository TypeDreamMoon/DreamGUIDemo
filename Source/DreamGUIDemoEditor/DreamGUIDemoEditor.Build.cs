// Copyright DreamMoon. All Rights Reserved.

using UnrealBuildTool;

/*
 * Editor-only tools of the demo project: the P3R study's setup commandlet (imports the generated art and fonts,
 * makes its map) and the commandlet that takes machine paths out of imported assets before they are committed.
 */
public class DreamGUIDemoEditor : ModuleRules
{
	public DreamGUIDemoEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "UnrealEd", "AssetTools", "AssetRegistry", "Json",
			"DreamGUI", "P3RUIDemo" });
	}
}
