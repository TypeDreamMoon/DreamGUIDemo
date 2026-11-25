// Copyright DreamMoon. All Rights Reserved.

// The Detroit test's controls on a touch screen: a stick that appears where the left thumb lands, the right half of the
// screen dragged to look, and buttons for running, analysis mode, analyzing a clue, the objectives and the five clues.
// The player controller reads them every frame, beside the keys, the mouse and a pad.

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SCompoundWidget.h"

/** What a press of a touch button asks for, taken by the player controller once per frame. */
enum class EDetroitTouchAction : uint8
{
	None = 0,
	Analysis = 1 << 0,
	Analyze = 1 << 1,
	Objectives = 1 << 2,
};
ENUM_CLASS_FLAGS(EDetroitTouchAction);

class SDetroitTouchStick;
class SDetroitTouchLook;

class DEVUITESTS_API SDetroitTouchControls : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SDetroitTouchControls) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	/** The stick, X right and Y forward, each -1 to 1. */
	FVector2D GetMove() const;
	/** How far the look area was dragged since the last call, in Slate units, Y down. */
	FVector2D ConsumeLook();
	bool IsRunning() const { return bRunning; }
	/** The buttons pressed since the last call. */
	EDetroitTouchAction ConsumeActions();
	/** The clue button pressed since the last call, 0 to 4, or INDEX_NONE. */
	int32 ConsumeClue();
	/** Lets go of the stick and the look drag, and forgets presses not yet taken: for when the controls are hidden. */
	void Reset();

private:
	TSharedRef<SWidget> MakeButton(const FText& Label, TFunction<void()> OnPress, TFunction<bool()> IsOn = nullptr, float Size = 104.0f);

	TSharedPtr<SDetroitTouchStick> Stick;
	TSharedPtr<SDetroitTouchLook> Look;
	FButtonStyle ButtonStyle;
	EDetroitTouchAction Pending = EDetroitTouchAction::None;
	int32 PendingClue = INDEX_NONE;
	bool bRunning = false;
};
