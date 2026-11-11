// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "Interaction/UISelectable.h"
#include "NocturneMainMenu.generated.h"

class UDreamWidget;

/**
 * What NOCTURNE//OS's star map keeps on the screen: the top bar, the route list in the lower left -- one entry a star,
 * the keyboard's and the pad's way through the map -- and the key hints. Its .dui is DUI/Nocturne/Nocturne_MainMenu.dui.
 *
 * The menu (ANocturneMenu) owns which star is in focus; this shows it, and says when an entry is pointed at, focused
 * or chosen. W / S (or Up / Down, the pad's d-pad) move through the list, Enter (or the pad's A) chooses.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneMainMenu : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	/** Light the entry of star InIndex (INDEX_NONE for none). */
	void SetFocused(int32 InIndex);
	/** Give the keyboard focus to the entry of star InIndex. */
	void FocusEntry(int32 InIndex);
	/** A refused choice: the entry's tag flashes amber. */
	void FlashLocked(int32 InIndex);

	/** The chrome builds in after InDelay: the bars slide in from their edges, the entries run in one by one. */
	void PlayEntrance(float InDelay = 0.0f);
	/** It gives the screen up: everything fades, then stops taking the pointer. */
	void PlayExit();
	/** Gone at once, as before the title wakes the map. */
	void HideNow();

	/** A part by its id in the .dui. */
	UDreamWidget* Node(const TCHAR* Name) const;

	/** An entry was pointed at or took the keyboard focus. */
	TMulticastDelegate<void(int32)> OnEntryFocused;
	/** An entry was clicked, or Enter was pressed on it. */
	TMulticastDelegate<void(int32)> OnEntryChosen;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;

private:
	void HandleState(int32 InIndex, EUISelectableSelectionState InState);
	void Paint();
	void SetPartsShown(bool bInShown);

	int32 Focused = INDEX_NONE;
	int32 Hovered = INDEX_NONE;
	int32 FlashIndex = INDEX_NONE;
	float MenuTime = 0.0f;
	float FlashTime = -100.0f;
	/** When PlayExit's fade is over and the parts stop taking the pointer; negative while shown. */
	float HideAt = -1.0f;
	int32 ShownSecond = -1;
};
