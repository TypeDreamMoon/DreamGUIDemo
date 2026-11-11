// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturnePerfScreen.generated.h"

class UDreamRectBlock;

/**
 * NOCTURNE//OS's performance screen, the video's performance shot. On its entrance it fills the wall behind its panels
 * with 5000 buttons, then on the screen's clock:
 *
 *   1.0 s  a band sweeps across the wall: only the buttons under it turn and light, and only they are touched -- the
 *          left panel's figures show a few dozen widgets updated a frame, in a handful of batches
 *   5.2 s  every button turns at once, a wave out from the middle; 5000 widgets a frame, the batches no more
 *   8.5 s  the right panel counts the automation test suite and runs down its names
 *
 * The figures are DreamGUI's own (DreamUIRenderStats), sampled every quarter second. The frame rate cannot come from an
 * off-screen take on a fixed step, so it is measured in a real-time run (-DemoShot=Perf -DemoMeasure) and handed in as
 * -PerfFrameMs=<average>.
 */
UCLASS(Abstract)
class DREAMGUIDEMO_API UNocturnePerfScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnTick(float DeltaTime) override;
	virtual void PlayContentEntrance(float Delay) override;

private:
	void BuildWall();
	void LightSpec(int32 InIndex);
	void SetSpec(int32 InIndex, const FString& InValue);

	UPROPERTY(Transient)
	TArray<TObjectPtr<UDreamWidget>> Tiles;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UDreamRectBlock>> Blocks;
	/** Each tile's angle and lit state as last set, so a frame touches only the tiles that change. */
	TArray<float> TileAngle;
	TArray<bool> TileLit;
	TArray<FVector2D> TileSpot;

	float StartTime = 0.0f;
	float NextSample = 0.0f;
	int32 Lit = -1;
	bool bStarted = false;
	bool bTestsShown = false;
	float FrameMs = 0.0f;
};
