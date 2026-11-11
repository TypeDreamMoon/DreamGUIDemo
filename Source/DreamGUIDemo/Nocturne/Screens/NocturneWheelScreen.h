// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneWheelScreen.generated.h"

class UDreamRingMenu;

/**
 * NOCTURNE//OS's item wheel, behind the star 轮 (DUI/Nocturne/Nocturne_Wheel.dui). Eight items on a ring menu round
 * the star -- DreamGUI's UDreamRingMenu, so the wedges' exact hit sectors, the highlight and the click are the
 * control's -- their glyphs on the wedges and their names outside, placed at the control's own item angles. Pointing
 * describes the item on the left; a click puts it in the lit quick slot on the right and lights the next one.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneWheelScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

public:
	static constexpr int32 WheelCount = 8;
	static constexpr int32 QuickCount = 4;

	/** Which quick slot the next item goes in. */
	void SetQuickTarget(int32 InSlot);

public:
	virtual void PlayExit() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;
	virtual void PlayContentEntrance(float Delay) override;

private:
	UFUNCTION()
	void HandleHighlight(int32 Index);
	UFUNCTION()
	void HandleActivated(int32 Index, FName Tag);

	void PlaceMarks();
	void Describe(int32 InWheelIndex, bool bInAnimate);
	void PaintQuick();
	void PaintMarks();

	TWeakObjectPtr<UDreamRingMenu> Wheel;
	/** Which item of NocturneItems each quick slot holds, or INDEX_NONE. */
	int32 Quick[QuickCount] = { 3, INDEX_NONE, INDEX_NONE, INDEX_NONE };
	int32 QuickTarget = 1;
	int32 HoveredQuick = INDEX_NONE;
	int32 Highlighted = INDEX_NONE;
	int32 Described = INDEX_NONE;
	bool bMarksPlaced = false;
	/** When the wheel opens, on the screen's clock; negative once it has. */
	float OpenAt = -1.0f;
	float FlashAt = -100.0f;
	int32 FlashSlot = INDEX_NONE;
	/** When the wheel starts rising from lying back, and falling back on the way out; negative for not. */
	float RiseAt = -1.0f;
	float FallAt = -1.0f;
	/** How far the wheel leans toward the highlighted item now, degrees. */
	FRotator Tilt = FRotator::ZeroRotator;
};
