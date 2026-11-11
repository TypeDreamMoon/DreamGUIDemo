// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "Interaction/DreamUITabSwitchTarget.h"
#include "NocturneArchiveScreen.generated.h"

class UNocturneItemSlot;

/**
 * NOCTURNE//OS's archive, behind the star 匣 (DUI/Nocturne/Nocturne_Archive.dui). Categories on the left -- all, gear,
 * materials, records; the others dim -- the item grid in the middle, the selected item on the right. Items move by drag
 * and drop between slots (DreamGUI's drag source and drop target, with a ghost that follows the pointer); resting on a
 * slot shows the item's tooltip (DreamGUI's tooltip service). Q / E or the shoulders switch the category.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneArchiveScreen : public UNocturneStarScreen, public IDreamUITabSwitchTarget
{
	GENERATED_BODY()

public:
	static constexpr int32 SlotCount = 40;
	static constexpr int32 Columns = 8;
	static constexpr int32 CategoryCount = 4;

	UNocturneItemSlot* GetSlot(int32 InIndex) const;
	void ShowCategory(int32 InCategory);
	void Select(int32 InSlot);

	// IDreamUITabSwitchTarget
	virtual bool CanSwitchTab(int32 InUserIndex) const override { return true; }
	virtual bool SwitchTab(int32 InUserIndex, int32 InDelta) override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual void PlayContentEntrance(float Delay) override;
	virtual UDreamWidget* GetInitialFocus() const override;

private:
	void HandleDrop(UNocturneItemSlot* Target, UNocturneItemSlot* Source);
	void UpdateDetail(bool bInAnimate);
	void UpdateCounts();
	void PaintCategories();
	bool PassesFilter(int32 InItem) const;

	TArray<TWeakObjectPtr<UNocturneItemSlot>> Slots;
	/** A category change flips the grid over, a wave from the top left: when it began (negative for none) and which
	 *  slots have turned past edge-on, where they take their new state. */
	float FlipAt = -1.0f;
	TArray<bool> FlipSwapped;
	/** When the item in the detail was last changed: its picture turns in and decodes from a few levels. */
	float IconAt = -100.0f;
	TWeakObjectPtr<class UDreamStylizeEffect> IconDecode;
	bool bDecodeOn = true;
	int32 Category = 0;
	int32 HoveredCategory = INDEX_NONE;
	int32 Selected = INDEX_NONE;
};
