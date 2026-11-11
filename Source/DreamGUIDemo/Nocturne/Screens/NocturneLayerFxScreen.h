// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneLayerFxScreen.generated.h"

/**
 * NOCTURNE//OS's layer effects screen, the video's layer effects shot: a picture built up a layer at a time while the
 * layer list lights each one and the properties panel reads it out, then the whole stack swings round and comes apart
 * in depth. It plays itself, on the screen's own clock, from PlayEntrance:
 *
 *   2.00 s  clip mask      the portrait opens through a radial mask
 *   2.75 s  clip to below  the harbour picture shows only inside the title's letters, panning
 *   3.45 s  blend mode     two light beams by Overlay, then Difference, then Screen
 *   4.25 s  layer group    three discs fade in and blend as one
 *   5.60 s  progressive blur  a band frosted at the bottom and clear at its top
 *   6.60 s  glass          a slab slides across, bending what is behind
 *   7.60 s  layered document  a .psd imported as a widget runs in, layer by layer
 *   9.30 s  the stack swings round and its sheets part in depth; 12.8 s it closes again
 *
 * The .dui (Nocturne_LayerFx.dui) holds the parts by id; see it for the stack's sheets.
 */
UCLASS(Abstract)
class DREAMGUIDEMO_API UNocturneLayerFxScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

protected:
	virtual void NativeOnTick(float DeltaTime) override;
	virtual void PlayContentEntrance(float Delay) override;

private:
	struct FBeat
	{
		float Time = 0.0f;
		TFunction<void()> Run;
	};

	void BuildBeats();
	/** The layer list's row InIndex turns on and becomes the current one. */
	void LightRow(int32 InIndex);
	/** The properties panel's heading and up to three key / value lines. */
	void ShowProps(const TCHAR* InTitle, const TCHAR* InEn, TArrayView<const TPair<FString, FString>> InLines);
	void SetPropValue(int32 InLine, const FString& InValue);
	void Explode(bool bInApart);
	/** 0..1 over a span of the screen's clock, eased out. */
	float Ease(float InFrom, float InSeconds) const;

	TArray<FBeat> Beats;
	int32 NextBeat = 0;
	int32 CurrentRow = -1;
	bool bBeatsBuilt = false;
	/** When the beats' clock started: the entrance. */
	float StartTime = 0.0f;
	/** What the live read-out tracks: 0 nothing, 1 the mask, 3 the group, 4 the blur, 7 the stack's depth. */
	int32 Live = 0;
};
