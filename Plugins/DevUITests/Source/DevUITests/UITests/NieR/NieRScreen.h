// NieR:Automata-style UI test screens for DreamGUI. A screen's layout is its .dui in DUI/UITests/NieR, built from the
// components in NieRComponents.h; this is the behaviour behind it.

#pragma once

#include "CoreMinimal.h"
#include "Interaction/DreamUINavigationScope.h"
#include "NieRComponents.h"
#include "NieRScreen.generated.h"

class UDreamKeyEventData;
class UNieRHintBar;

/**
 * A NieR screen's navigation scope: Back (Escape, Backspace) is the screen's to answer -- close a sub-list, go back to
 * "press any key", or pop the screen. Put on each screen's root in its .dui as `+ NieRScreenScope { ... }`.
 */
UCLASS(ClassGroup = (NieRUITest), meta = (BlueprintSpawnableComponent))
class DEVUITESTS_API UNieRScreenScope : public UDreamUINavigationScope
{
	GENERATED_BODY()

protected:
	virtual bool HandleBackAction_Implementation() override;
};

/**
 * The base of the NieR screens. Every row (UNieRRow) anywhere in the screen is wired to it: one row is highlighted at a
 * time -- the one with the focus, or the one being pressed -- and a row the mouse moves onto takes the focus. The
 * highlighted row's description goes to the hint bar (UNieRHintBar), and Left/Right go to the row first.
 */
UCLASS(Abstract)
class DEVUITESTS_API UNieRScreen : public UNieRComponent
{
	GENERATED_BODY()

public:
	/** Back reached this screen. True when the screen dealt with it (it always does: a NieR screen never lets Back fall through). */
	virtual bool HandleBack();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnEnable() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;

	/** The row the focus goes to when the screen shows and it holds no row yet. */
	virtual UNieRRow* GetEntryRow() const { return nullptr; }
	virtual void OnRowClicked(UNieRRow* Row) {}
	virtual void OnRowHighlighted(UNieRRow* Row) {}
	/** The screen's entrance, played each time it shows. */
	virtual void PlayEntrance();

	/** A row by its id in the screen's .dui. */
	UNieRRow* FindRow(FName Name) const;
	UNieRRow* GetHighlightedRow() const { return Highlighted.Get(); }
	/** The rows under InRoot, in the order they are laid out. */
	static TArray<UNieRRow*> CollectRows(UDreamWidget* InRoot);
	/**
	 * Move the keyboard focus onto a row or a widget, now and again next frame if it did not take: DreamGUI gives the
	 * focus only to a widget that is drawn, so one shown this frame (a sub-list opened) may refuse it until then.
	 */
	void FocusRow(UNieRRow* Row);
	void FocusWidget(UDreamWidget* Widget);
	/** Each row slides in from Offset (+y down), one after another. */
	void SlideRowsIn(const TArray<UNieRRow*>& InRows, FVector2D Offset, float Duration, float Stagger, float Delay = 0.0f);

	void PushScreen(FName InName, const TCHAR* InClassPath);
	void PopThisScreen();
	float GetScreenTime() const { return ScreenTime; }

private:
	void WireRows();
	void HandleRowState(UNieRRow* Row, EUISelectableSelectionState InState);
	void HandleRowClicked(UNieRRow* Row);
	void Highlight(UNieRRow* Row);
	void ApplyPendingFocus();

	TArray<TWeakObjectPtr<UNieRRow>> Rows;
	TWeakObjectPtr<UNieRRow> Highlighted;
	/** The row that last held the focus, given back when the screen shows again. */
	TWeakObjectPtr<UNieRRow> LastFocused;
	TWeakObjectPtr<UDreamWidget> PendingFocus;
	TWeakObjectPtr<UNieRHintBar> HintBar;
	float ScreenTime = 0.0f;
	/** When the mouse last moved: a row the pointer rests over takes the focus only after real motion. */
	float MouseMovedTime = -100.0f;
	FVector2D LastMousePosition = FVector2D(-1.0, -1.0);
};
