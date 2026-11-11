// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneTextScreen.generated.h"

/**
 * NOCTURNE//OS's text screen, the video's text shot. It plays itself on the screen's clock from PlayEntrance:
 *
 *   1.5 s  a line of 12 px text appears in the middle of the stage
 *   1.8 s  it grows to fourteen times its size, its edges sharp all the way (distance-field glyphs)
 *   3.5 s  it turns away in 3D and is seen at a slant
 *   4.9 s  it moves up out of the way
 *   5.6 s  Chinese, Japanese and English in one line; 6.4 s a line with colour emoji; 7.1 s a gradient title, a shimmer
 *          running over it from then on
 *
 * The left panel's rows light as each is shown, the scale and the angle read out live.
 */
UCLASS(Abstract)
class DREAMGUIDEMO_API UNocturneTextScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnTick(float DeltaTime) override;
	virtual void PlayContentEntrance(float Delay) override;

private:
	void LightSpec(int32 InIndex);
	/** 0..1 over a span of the clock, eased in and out. */
	float Smooth(float InFrom, float InSeconds) const;

	float StartTime = 0.0f;
	int32 Lit = -1;
	bool bStarted = false;
	/** Which of the timed lines have run in. */
	uint8 LinesShown = 0;
};
