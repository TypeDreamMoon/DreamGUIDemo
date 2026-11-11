// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "Interaction/UISelectable.h"
#include "NocturneItemSlot.generated.h"

class UDreamDragDropOperation;
class UDreamWidget;

/**
 * A slot of the NOCTURNE//OS archive's grid (DUI/Nocturne/Components/Nocturne_ItemSlot.dui). It holds one item of
 * NocturneItems.h, or none. A filled slot can be dragged onto another slot -- DreamGUI's drag source and drop target
 * carry it, the two swap -- and shows the item's tooltip when the pointer rests on it.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneItemSlot : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	/** Which slot of the grid this is (its code), and what it holds (INDEX_NONE for nothing). */
	void Setup(int32 InSlotIndex);
	void SetItem(int32 InItemIndex);
	int32 GetItem() const { return Item; }
	int32 GetSlotIndex() const { return SlotIndex; }
	void SetSelected(bool bInSelected);
	/** Shown or not by the archive's category filter: a filtered-out slot is dimmed and does not take a drop. */
	void SetFiltered(bool bInPassesFilter);

	UDreamWidget* Node(const TCHAR* Name) const;

	TMulticastDelegate<void(UNocturneItemSlot*)> OnHovered;
	TMulticastDelegate<void(UNocturneItemSlot*)> OnClicked;
	/** Another slot's item was dropped here: (this slot, the slot it came from). */
	TMulticastDelegate<void(UNocturneItemSlot*, UNocturneItemSlot*)> OnItemDropped;

protected:
	virtual void NativeOnInitialized() override;

private:
	UFUNCTION()
	void HandleDropAccepted(UDreamDragDropOperation* Operation);
	UFUNCTION()
	void HandleDragEnter(UDreamDragDropOperation* Operation);
	UFUNCTION()
	void HandleDragLeave(UDreamDragDropOperation* Operation);
	void HandleState(EUISelectableSelectionState InState);
	void Paint();

	int32 SlotIndex = 0;
	int32 Item = INDEX_NONE;
	bool bSelected = false;
	bool bHovered = false;
	bool bDropHover = false;
	bool bPassesFilter = true;
};

/** What follows the pointer while an archive item is dragged (Nocturne_ItemGhost.dui): it reads the drag in progress. */
UCLASS()
class DREAMGUIDEMO_API UNocturneItemGhost : public UDreamTextUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	bool Fill();
	bool bFilled = false;
};

/** An archive item's tooltip (Nocturne_ItemTip.dui): it reads the slot the tooltip service is showing it for. */
UCLASS()
class DREAMGUIDEMO_API UNocturneItemTip : public UDreamTextUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	bool Fill();
	bool bFilled = false;
};
