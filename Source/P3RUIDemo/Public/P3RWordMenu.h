// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDreamWidget;
class UP3RScreen;

/**
 * The game's word menu, shared by the pause menu and SYSTEM: heavy italic words each at its own angle, the chosen one
 * black with red over its upper half, a white slash with a pink edge behind it. Motion follows what has been measured
 * from the game by people rebuilding it:
 *   - words drop 30 px into place from the bottom up, 33 ms apart, over 0.4 s;
 *   - the cursor arrives 0.4 s later from the upper right, overshoots down-left, back up, and settles (0.2/0.1/0.1 s);
 *   - a move takes 60 ms, linear, turning the cursor to the next word's angle and stretching it to the word's size;
 *     the chosen word turns black and grows with a little overshoot, the one left behind snaps back;
 *   - the cursor pulses (1 -> 1.1 -> 1 in 0.2 s) on every move and once a second while resting; the pink edge breathes.
 *
 * The cursor is also a mask over the words, as the game's is: wherever its slash lies over a word's letters they change
 * colour -- red on the chosen word (whose letters are black elsewhere), white on the others. So the chosen word is cut
 * black and red along the slash's edge, and as the cursor moves, pulses or is pulled across by the pointer the words it
 * sweeps over flash white under it. Each word holds a Shape clipped to its letters (<Prefix><i>Red); every frame the
 * slash's outline is carried into that word's own frame and given to the shape.
 *
 * The .dui names the parts: <Prefix><i> (the word, its rest pose written there), <Prefix><i>Shadow (a dark copy under
 * the face), <Prefix><i>Text, <Prefix><i>Red (a Shape clipped to the text), and a Cursor widget of 100 x 100 holding
 * CursorWhite (whose outline is the mask) and CursorPink.
 */
struct P3RUIDEMO_API FP3RWordMenu
{
	struct FWord
	{
		TWeakObjectPtr<UDreamWidget> Root;
		TWeakObjectPtr<UDreamWidget> Text;
		TWeakObjectPtr<UDreamWidget> Shadow;
		TWeakObjectPtr<UDreamWidget> Red;
		/** What the red shape was last given, to skip giving it the same again. */
		TArray<FVector2D> InkPoints;
		bool bInkShown = false;
		FVector2D Centre = FVector2D::ZeroVector;   // design px, y down
		FVector2D Size = FVector2D(200.0, 84.0);
		float Angle = 0.0f;
	};

	void Init(UP3RScreen* InScreen, const TCHAR* InPrefix, int32 InCount, const TCHAR* InCursor, const TCHAR* InCursorPink, float InChosenScale);

	int32 Num() const { return Words.Num(); }
	int32 GetSelected() const { return Selected; }
	UDreamWidget* GetWord(int32 Index) const { return Words.IsValidIndex(Index) ? Words[Index].Root.Get() : nullptr; }
	/** A word's centre at rest, in design pixels from the top left. */
	FVector2D GetCentre(int32 Index) const { return Words.IsValidIndex(Index) ? Words[Index].Centre : FVector2D(960.0, 540.0); }

	/** Returns whether the choice changed. */
	bool Select(int32 Index, float Now, bool bInstant = false);
	void Deny(float Now) { DenyAt = Now; }

	/** The words drop in from the bottom up starting at Delay; the cursor follows when they have landed. */
	void DropIn(float Now, float Delay);
	/** Every word and the cursor in place at once (coming back to a menu that never really left). */
	void ShowNow(float Now);
	/** Off to the right, quickly (leaving for a deeper screen). */
	void FlyOut();
	/** Gone at once (closing). */
	void Vanish();

	void Tick(float Now);

	/** The unchosen words' colours, cycled. */
	TArray<FColor> Colours;
	/** Which word lies over which where they overlap: higher is drawn later. Missing entries are 0. Set before Init. */
	TArray<int32> Layers;

private:
	void Paint(int32 Index, bool bChosen);
	void PlaceCursor(const FVector2D& Centre, float Angle, const FVector2D& Size);
	/** Carries the slash's outline, as drawn now, into every word's frame. */
	void UpdateInk();

	TArray<FWord> Words;
	TWeakObjectPtr<UDreamWidget> Cursor;
	TWeakObjectPtr<UDreamWidget> CursorPink;
	/** The slash's outline in the cursor's 100 x 100 box (normalized, y up), read from CursorWhite. */
	TArray<FVector2D> SlashOutline;
	float CursorPulse = 1.0f;
	FVector2D CursorHome = FVector2D(960.0, 540.0);
	float ChosenScale = 1.35f;
	int32 Selected = 0;

	FVector2D CursorFromCentre = FVector2D::ZeroVector;
	FVector2D CursorFromSize = FVector2D(200.0, 84.0);
	float CursorFromAngle = 0.0f;
	float CursorMoveAt = -100.0f;
	float CursorShowAt = 1e9f;
	float TwitchAt = -100.0f;
	float DenyAt = -100.0f;
};
