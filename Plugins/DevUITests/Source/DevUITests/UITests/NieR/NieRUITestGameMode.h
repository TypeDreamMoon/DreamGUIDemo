#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "NieRUITestGameMode.generated.h"

/**
 * The game mode of the NieR UI test map (/Game/UITests/NieR/Maps/NieR_TestMap): no pawn -- nothing in the world may take
 * the arrows, Q or E -- a screen canvas that scales a 1920x1080 design to the window, and the start screen on top.
 */
UCLASS()
class DEVUITESTS_API ANieRUITestGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ANieRUITestGameMode();

protected:
	virtual void BeginPlay() override;

private:
	void ShowStartScreen();

	/**
	 * Every screen's class, loaded with the map and held for as long as it runs: a screen opened for the first time loaded
	 * its class, textures and fonts inside the frame that opened it, and a screen closed let them be collected, to be
	 * loaded again by the next open.
	 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UClass>> ScreenClasses;
};
