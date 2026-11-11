// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "ScrubImportPathsCommandlet.generated.h"

/**
 * Takes the absolute paths of the machine that imported or generated an asset out of it, so the project can be
 * published without them:
 *
 *     UnrealEditor-Cmd.exe DreamGUIDemo.uproject -run=ScrubImportPaths [-Path=/Game] [-DryRun]
 *
 * What it rewrites, in every package under -Path:
 *   - import data (textures, fonts...): replaced by plain import data whose source files are relative to the
 *     package. Interchange's import data is replaced too: its node container spells the absolute source path.
 *   - string properties holding an absolute path (UFontFace::SourceFilename...).
 *   - package metadata values holding an absolute path (DreamFX's DreamFX.SourceFullPath hint).
 *
 * A path inside the project becomes project-relative; a path from another checkout becomes project-relative when
 * the same file is found here under the same tail (DevProject/DUI/... -> DUI/...); anything else keeps only its file
 * name. Only changed packages are saved. Run it after importing or rebuilding assets, before committing them.
 */
UCLASS()
class UScrubImportPathsCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UScrubImportPathsCommandlet();
	virtual int32 Main(const FString& Params) override;
};
