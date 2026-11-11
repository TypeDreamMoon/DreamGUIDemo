// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneDepartScreen.generated.h"

/**
 * NOCTURNE//OS's departure, behind the star 启明 (DUI/Nocturne/Nocturne_Depart.dui): three save cards in a shallow arc
 * and the button that sets off. Hovering a card lifts it, clicking picks it; Depart (or Enter) launches the chosen
 * voyage (OnLaunch), and the menu dives past the star into the game. A / D or the d-pad pick a card.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneDepartScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

public:
	static constexpr int32 CardCount = 3;

	void Pick(int32 InCard);
	void Launch();

	/** The voyage to go on: the save slot picked. */
	TMulticastDelegate<void(int32)> OnLaunch;

public:
	virtual void PlayExit() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;
	virtual void PlayContentEntrance(float Delay) override;
	virtual UDreamWidget* GetInitialFocus() const override;

private:
	void Paint();
	void SetLifted(int32 InCard, bool bInLifted);

	int32 Picked = 0;
	int32 Hovered = INDEX_NONE;
	bool bLaunched = false;
	bool bLaunchHovered = false;
};
