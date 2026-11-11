// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "P3RSetupCommandlet.generated.h"

/**
 * Makes the study's assets that are not built from text, headlessly:
 *
 *     UnrealEditor-Cmd.exe DreamGUIDemo.uproject -run=P3RSetup [-Textures] [-Fonts] [-Map]   (none = all three)
 *
 *   -Textures  every PNG in Art/P3R/Final becomes /Game/P3R/Textures/<name>, set up for UI (UI group, uncompressed, mips).
 *   -Fonts     the DreamGUI distance-field fonts /Game/P3R/Fonts/F_P3R_*: the narrowed Roboto cuts in Art/P3R/Fonts for Latin,
 *              each with a Source Han Sans weight as its CJK fallback. The font files are read from beside the project.
 *   -Map       /Game/P3R/Maps/P3R_Demo, an empty level whose game mode is AP3RGameMode.
 *
 * Re-runnable: what exists is updated in place. Exits non-zero when anything failed.
 */
UCLASS()
class UP3RSetupCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UP3RSetupCommandlet();
	virtual int32 Main(const FString& Params) override;

private:
	bool ImportTextures();
	bool MakeFonts();
	bool MakeMap();
};
