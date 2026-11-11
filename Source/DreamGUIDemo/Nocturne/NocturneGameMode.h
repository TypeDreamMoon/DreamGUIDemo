// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NocturneGameMode.generated.h"

/**
 * NOCTURNE//OS: spawns the space and the director into whatever level it runs in, so the empty Entry map is enough:
 *   /Engine/Maps/Entry?game=/Script/DreamGUIDemo.NocturneGameMode
 * No pawn: the director's camera is the view.
 */
UCLASS()
class DREAMGUIDEMO_API ANocturneGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANocturneGameMode();

	virtual void BeginPlay() override;
};
