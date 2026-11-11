// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneMusicScreen.generated.h"

class UDreamLyricsView;

/**
 * NOCTURNE//OS's music box, behind the star 吟 (DUI/Nocturne/Nocturne_MusicBox.dui). Round the star: the track's
 * progress as a ring, a spectrum standing out from it, the line being sung running round the top as text on a circle
 * and the next line round the bottom. The track list on the left; the whole lyric on the right in DreamGUI's lyrics
 * view, driven by the screen's own play clock. Space plays and pauses, Q / E change the track. The music itself is
 * not in the demo: the clock stands in for it.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneMusicScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

public:
	static constexpr int32 TrackCount = 6;
	static constexpr int32 BarCount = 48;

	void PlayTrack(int32 InTrack);
	void SetPlaying(bool bInPlaying);

public:
	virtual void PlayExit() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;
	virtual void PlayContentEntrance(float Delay) override;
	virtual UDreamWidget* GetInitialFocus() const override;

private:
	struct FLine
	{
		float Time = 0.0f;
		FString Text;
	};

	void UpdateLyricLines(bool bInForce);
	void UpdateDisc(float DeltaTime);
	void PaintTracks();

	TWeakObjectPtr<UDreamLyricsView> LyricsView;
	TArray<FLine> Lines;
	int32 Track = 0;
	int32 HoveredTrack = INDEX_NONE;
	int32 ShownLine = -2;
	float PlayTime = 0.0f;
	bool bPlaying = true;
	TArray<float> BarHeights;
	/** When the disc starts tipping up to its angle, and back down on the way out; negative for not. */
	float TipAt = -1.0f;
	float FallAt = -1.0f;
	bool bDiscLayered = false;
};
