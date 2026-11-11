// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "DreamTweener.h"
#include "NocturneStarScreen.generated.h"

class UDreamWidget;
class UDreamPixelSort;
class UDreamTweener;

/**
 * What every star's screen in NOCTURNE//OS shares: panels over the dimmed star map -- usually three in a shallow arc,
 * the side ones turned in -- and a flat key-hint bar along the bottom. The parts are found by id in the screen's .dui,
 * and any it does not have are simply skipped:
 *
 *   Dim                         the scrim over the star map
 *   Rig                         what leans a degree or two toward the pointer
 *   LeftFrame / CenterFrame / RightFrame, LeftPlate / CenterPlate / RightPlate   draw out and fade up on entrance
 *   CenterSort / RightSort / LeftSort   pixel sorts over the panels, burst on entrance and on demand
 *   KeyBar                      slides up on entrance
 *   HintBack, HintBackHit, HintBackLabel   the "ESC back" hint, which is also a button
 *   Trim...                     polygon trim (corner diamonds): pops in after the frames
 *   Sigil...                    polygon sigils: turn slowly all the time
 *   ...Frame                    flickers on (two quick on-off cycles) as it draws; plates and text only fade
 *
 * The screen is a set of layers in depth, not a flat picture: frames float a little in front of their plates, trims and
 * sigils further still, so the whole rig leaning toward the pointer (and drifting a little on its own) shows them
 * slide against each other. Side panels swing open on their inner edges as the screen comes in, the middle one rises
 * from lying back, and on the way out they fold shut and the rig falls away into the distance.
 *
 * Esc, Backspace or the pad's B ask to leave (OnBackRequested) unless the screen has something of its own open.
 * Motion runs on tweens and on the screen's own clock, so a take on a fixed time step repeats exactly.
 */
UCLASS(Abstract)
class DREAMGUIDEMO_API UNocturneStarScreen : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	/** The panels build in: frames draw, plates fade up, then the screen's own contents run in under a pixel sort. */
	void PlayEntrance();
	/** Everything folds away at once under a strong sort, faster than it came. */
	virtual void PlayExit();

	/** A part by its id in the screen's .dui. */
	UDreamWidget* Node(const TCHAR* Name) const;
	/** -1 to 1 on each axis: where the pointer is on the screen. The panels lean a degree or two toward it. */
	void SetParallaxTarget(const FVector2D& InTarget) { ParallaxTarget = InTarget; }
	/** Ask to leave, as Esc does: refused while the screen has something of its own open. */
	void RequestBack();
	/** The whole rig jolts in 3D -- pitch, yaw and roll, settling in a few tenths of a second. 1 is a hard knock. */
	void Shake(float InStrength);

	TMulticastDelegate<void()> OnBackRequested;

	enum ESort : int32 { CenterSortIndex = 0, RightSortIndex = 1, LeftSortIndex = 2, SortCount = 3 };

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;

	/** The screen's own contents coming in, after the frames and plates; Delay is when the contents may start. */
	virtual void PlayContentEntrance(float Delay) {}
	/** Whether leaving is allowed now (not while a dialog, a listening key or a drag is open). */
	virtual bool CanGoBack() const { return true; }
	/** What the keyboard and the pad start on when the screen comes in; none by default. */
	virtual UDreamWidget* GetInitialFocus() const { return nullptr; }
	/** How hard the lens breaks up as the screen comes in (NocturneLens's transition), 0 for not at all. */
	virtual float GetEntrancePulse() const { return 0.8f; }

	/** The exit's pieces, for a screen that leaves its own way: the panels fold, the rig falls away, the chrome fades. */
	void FoldPanels();
	void RecedeRig();
	void FadeChrome();

	/** A pixel sort over a panel: strong at once, gone after InSeconds; or rising after InDelay. */
	void BurstSort(int32 InWhich, float InPeak, float InSeconds, float InDelay = 0.0f);
	/** The children of InParent come in one after another from InOffset (design units, +y down), fading up. */
	void RunInChildren(UDreamWidget* InParent, float InDelay, float InStagger, FVector2D InOffset, float InFlip = 1.0f);
	/**
	 * One part comes in from InOffset, fading up, and flips up from lying back a little way behind where it rests (InFlip
	 * scales the flip and the depth: 0 for a part that turns on its own, such as a ring or a sigil).
	 */
	void RunIn(UDreamWidget* InWidget, float InDelay, float InSeconds, FVector2D InOffset, float InFlip = 1.0f);

	/** Where a part rests: its render translation and rotation in the .dui, remembered the first time it is moved. */
	struct FRestPose
	{
		FVector Translation = FVector::ZeroVector;
		FRotator Rotation = FRotator::ZeroRotator;
	};
	FRestPose GetRest(UDreamWidget* InWidget);
	/** A part swings in to its rest pose: from turned by InFrom and pushed InDepth further away. */
	void SwingIn(UDreamWidget* InWidget, float InDelay, float InSeconds, const FRotator& InFrom, double InDepth,
		EDreamTweenEase InEase = EDreamTweenEase::OutCubic);
	/** A part swings away from its rest pose: to turned by InTo and pushed InDepth further away. */
	void SwingOut(UDreamWidget* InWidget, float InDelay, float InSeconds, const FRotator& InTo, double InDepth,
		EDreamTweenEase InEase = EDreamTweenEase::InCubic);
	/** A render rotation tween (DreamGUI has translation, scale and the in-plane angle, not the full rotation). */
	static UDreamTweener* RotateTo(UDreamWidget* InWidget, const FRotator& InTo, float InSeconds, float InDelay, EDreamTweenEase InEase);

	/** A render offset in design units, +y down; the depth the part has is kept. */
	static void SetOffset(UDreamWidget* Widget, FVector2D Offset);
	/** Stop what is running on a part before it is sent somewhere else: two fades on one part fight to the last frame. */
	static void Settle(UDreamWidget* Widget);

	float GetScreenTime() const { return ScreenTime; }

private:
	void PaintBackHint(bool bInOver);
	/** Every part of the tree whose id starts with InPrefix (or ends with InSuffix), in tree order. */
	void Collect(UDreamWidget* InWidget, TArray<TWeakObjectPtr<UDreamWidget>>& OutTrims, TArray<TWeakObjectPtr<UDreamWidget>>& OutSigils,
		TArray<TWeakObjectPtr<UDreamWidget>>& OutFrames) const;

	TArray<TWeakObjectPtr<UDreamWidget>> Trims;
	TArray<TWeakObjectPtr<UDreamWidget>> Sigils;
	TArray<TWeakObjectPtr<UDreamWidget>> Frames;
	/** The opacity each frame had when the entrance began: the flicker ends there, not at full. */
	TArray<float> FrameOpacity;
	/** When the frames start flickering on; negative when they are not. */
	float FlickerFrom = -1.0f;

	TWeakObjectPtr<UDreamPixelSort> Sorts[SortCount];
	/** The initial focus, given a frame after the entrance: DreamGUI focuses only what has been drawn. */
	TWeakObjectPtr<UDreamWidget> PendingFocus;
	float SortPeak[SortCount] = { 0.0f, 0.0f, 0.0f };
	float SortStart[SortCount] = { -100.0f, -100.0f, -100.0f };
	float SortSeconds[SortCount] = { 0.3f, 0.3f, 0.3f };

	FVector2D ParallaxTarget = FVector2D::ZeroVector;
	FVector2D Parallax = FVector2D::ZeroVector;
	float ScreenTime = 0.0f;
	float ShakeAt = -100.0f;
	float ShakeStrength = 0.0f;
	TMap<TWeakObjectPtr<UDreamWidget>, FRestPose> Rests;
};
