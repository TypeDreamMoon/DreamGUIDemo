// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "Interaction/UISelectable.h"
#include "NocturneStarLabel.generated.h"

class UDreamWidget;

/**
 * A star's label in the NOCTURNE//OS star map: the ring around the star, its glyph, name and number, and while the star
 * is in focus a turning arc, its description and what Enter does (or that it is not open yet). Its .dui is
 * DUI/Nocturne/Components/Nocturne_StarLabel.dui. The menu (ANocturneMenu) owns the focus; the label shows it.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneStarLabel : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	/** Fill the label from NocturneStars::Stars[InIndex]. */
	void Setup(int32 InIndex);
	int32 GetIndex() const { return Index; }

	void SetFocused(bool bInFocused);
	/** The label builds in: ring, then text, after InDelay. */
	void PlayIntro(float InDelay);
	/** A refused Enter on a star that is not open: a knock and an amber flash. */
	void FlashLocked();
	/** A part by its id in the label's .dui. */
	UDreamWidget* Node(const TCHAR* Name) const;

	TMulticastDelegate<void(UNocturneStarLabel*)> OnHovered;
	TMulticastDelegate<void(UNocturneStarLabel*)> OnClicked;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	void HandleState(EUISelectableSelectionState InState, bool bInImmediate);
	void HandleClick();
	void Paint();
	/** Bring the focus-dependent parts (description, ring, arc, plate) to the focus state, over InSeconds or at once. */
	void ApplyFocusAt(float InSeconds);

	int32 Index = 0;
	/** The plate's width over the name row, and over the description too (design units). */
	float PlateWidth = 220.0f;
	float PlateFocusedWidth = 220.0f;
	bool bOpen = false;
	bool bFocused = false;
	float LabelTime = 0.0f;
	float FlashTime = -100.0f;
};
