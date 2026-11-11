// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Screens/NocturneStarScreen.h"
#include "Interaction/DreamUITabSwitchTarget.h"
#include "Interaction/UISelectable.h"
#include "NocturneSettingsScreen.generated.h"

class UNocturneOptionRow;
class UDreamWidget;

/**
 * NOCTURNE//OS's settings screen: three panels in a shallow arc in front of the star map -- the tab rail on the left
 * turned in, the options in the middle, the detail on the right turned in -- and a flat key-hint bar along the bottom.
 * Its .dui is DUI/Nocturne/Nocturne_Settings.dui; parts are found by id.
 *
 * Six pages (Page0 ... Page5), one per tab (Tab0 ... Tab5). One row is highlighted at a time -- the focused one, or the
 * one under the pointer -- and the detail panel describes it. Left / Right adjust it, Q / E or the shoulder buttons
 * switch the page, R asks to put the page back to its defaults, a key row listens for its key after a click.
 *
 * Motion runs on tweens and on the screen's own clock (world time), so a take recorded on a fixed step repeats exactly.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneSettingsScreen : public UNocturneStarScreen, public IDreamUITabSwitchTarget
{
	GENERATED_BODY()

public:
	static constexpr int32 PageCount = 6;

	void ShowPage(int32 InPage, bool bInAnimate = true);
	int32 GetPage() const { return Page; }
	/** A row of a page, in the order it is laid out. */
	UNocturneOptionRow* GetRow(int32 InPage, int32 InIndex) const;
	/** Ask to put the page back to its defaults (the confirm panel). */
	void AskReset();

	// IDreamUITabSwitchTarget
	virtual bool CanSwitchTab(int32 InUserIndex) const override;
	virtual bool SwitchTab(int32 InUserIndex, int32 InDelta) override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;
	virtual void PlayContentEntrance(float Delay) override;
	virtual bool CanGoBack() const override;

private:
	void WireRows();
	void HandleRowState(UNocturneOptionRow* Row, EUISelectableSelectionState InState);
	void HandleRowClicked(UNocturneOptionRow* Row);
	void HandleRowValue(UNocturneOptionRow* Row);
	void Highlight(UNocturneOptionRow* Row);
	void UpdateDetail(UNocturneOptionRow* Row, bool bInAnimate);
	void UpdateDirty();
	void PaintTabs();
	void ShowConfirm(bool bInShow);
	void ConfirmReset();
	void FocusRow(UNocturneOptionRow* Row);
	void ApplyPendingFocus();
	/** The page that turned away hidden and put back where it rests, ready to turn in again. */
	void FinishLeaving();

	TArray<TArray<TWeakObjectPtr<UNocturneOptionRow>>> PageRows;
	TArray<TWeakObjectPtr<UDreamWidget>> Pages;
	TWeakObjectPtr<UNocturneOptionRow> Highlighted;
	TWeakObjectPtr<UDreamWidget> PendingFocus;
	TWeakObjectPtr<UNocturneOptionRow> Listening;

	int32 Page = 0;
	/** The page turning away, kept shown until it has: which, and when to hide it. */
	int32 LeavingPage = INDEX_NONE;
	float LeavingUntil = -1.0f;
	int32 HoveredTab = INDEX_NONE;
	bool bConfirmOpen = false;
	bool bWired = false;
};
