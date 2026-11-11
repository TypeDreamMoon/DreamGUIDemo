// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "Interaction/DreamUITabSwitchTarget.h"
#include "NocturneLogScreen.generated.h"

class UDreamExpandableArea;

/**
 * NOCTURNE//OS's log, behind the star 笺 (DUI/Nocturne/Nocturne_Log.dui). Quests and messages, each a DreamGUI
 * expandable area opening on its objectives; opening one closes the others and shows it on the right, where the detail
 * wipes in under a mask transition. Categories on the left filter the list; Q / E switch them.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneLogScreen : public UNocturneStarScreen, public IDreamUITabSwitchTarget
{
	GENERATED_BODY()

public:
	static constexpr int32 EntryCount = 7;
	static constexpr int32 CategoryCount = 4;

	/** Open entry InIndex (closing the others) and show it on the right. */
	void OpenEntry(int32 InIndex);
	void ShowCategory(int32 InCategory);

	// IDreamUITabSwitchTarget
	virtual bool CanSwitchTab(int32 InUserIndex) const override { return true; }
	virtual bool SwitchTab(int32 InUserIndex, int32 InDelta) override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual void PlayContentEntrance(float Delay) override;
	virtual UDreamWidget* GetInitialFocus() const override;

private:
	void ShowDetail(int32 InIndex, bool bInAnimate);
	/** Keeps an opening entry in view and places the scroll bar. */
	void UpdateScroll();
	void PaintCategories();

	TArray<TWeakObjectPtr<UDreamExpandableArea>> Entries;
	/** The entries' scroll box, and the entry to keep in view while it opens (until when). */
	TWeakObjectPtr<class UDreamLayoutContainerScrollBox> QuestScroll;
	int32 RevealEntry = INDEX_NONE;
	float RevealUntil = -1.0f;
	TArray<bool> WasExpanded;
	int32 Category = 0;
	int32 HoveredCategory = INDEX_NONE;
	int32 Shown = INDEX_NONE;
};
