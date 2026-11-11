// Copyright DreamMoon. All Rights Reserved.

#include "NocturneItemSlot.h"

#include "NocturneItems.h"
#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWidgetTree.h"
#include "Interaction/DreamDragDropOperation.h"
#include "Interaction/DreamUIDragDrop.h"
#include "Interaction/DreamUITooltip.h"
#include "Interaction/UIButton.h"

namespace NocturneItemSlotLocal
{
	const TCHAR* GhostPath = TEXT("/Game/Nocturne/UI/Components/WBP_Nocturne_ItemGhost.WBP_Nocturne_ItemGhost_C");
	const TCHAR* TipPath = TEXT("/Game/Nocturne/UI/Components/WBP_Nocturne_ItemTip.WBP_Nocturne_ItemTip_C");
	const FName ItemTag(TEXT("Item"));

	UDreamWidget* FindNode(const UDreamUserWidget* Widget, const TCHAR* Name)
	{
		const UDreamWidgetTree* Tree = Widget != nullptr ? Widget->GetWidgetTree() : nullptr;
		return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
	}

	/** The slot a widget of the archive belongs to: the widget itself, or the slot it sits in. */
	UNocturneItemSlot* SlotOf(UDreamWidget* Widget)
	{
		for (UDreamWidget* Walk = Widget; Walk != nullptr; Walk = Walk->GetParent())
		{
			if (UNocturneItemSlot* Slot = Cast<UNocturneItemSlot>(Walk))
			{
				return Slot;
			}
		}
		return nullptr;
	}
}

UDreamWidget* UNocturneItemSlot::Node(const TCHAR* Name) const
{
	return NocturneItemSlotLocal::FindNode(this, Name);
}

void UNocturneItemSlot::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (UDreamWidget* Root = GetContentRoot())
	{
		if (UUIButton* Button = Root->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(false);
			Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this](EUISelectableSelectionState InState, bool) { HandleState(InState); });
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { OnClicked.Broadcast(this); });
		}
		if (UDreamUIDragSource* Source = Root->GetComponent<UDreamUIDragSource>())
		{
			// What is dragged is this slot: the drop reads the item from it.
			Source->Payload = this;
		}
		if (UDreamUIDropTarget* Target = Root->GetComponent<UDreamUIDropTarget>())
		{
			Target->OnDropAccepted.AddDynamic(this, &UNocturneItemSlot::HandleDropAccepted);
			Target->OnDragEnter.AddDynamic(this, &UNocturneItemSlot::HandleDragEnter);
			Target->OnDragLeave.AddDynamic(this, &UNocturneItemSlot::HandleDragLeave);
		}
	}
	Paint();
}

void UNocturneItemSlot::Setup(int32 InSlotIndex)
{
	SlotIndex = InSlotIndex;
	NocturneUI::SetText(Node(TEXT("SlotCode")), FText::FromString(FString::Printf(TEXT("A-%02d"), SlotIndex + 1)));
	Paint();
}

void UNocturneItemSlot::SetItem(int32 InItemIndex)
{
	using namespace NocturneItemSlotLocal;
	Item = InItemIndex >= 0 && InItemIndex < NocturneItems::Count ? InItemIndex : INDEX_NONE;
	const bool bHas = Item != INDEX_NONE;
	UDreamWidget* Root = GetContentRoot();
	if (Root != nullptr)
	{
		// An empty slot is not picked up and shows no tooltip; it still takes a drop.
		if (UDreamUIDragSource* Source = Root->GetComponent<UDreamUIDragSource>())
		{
			static TSoftClassPtr<UDreamUserWidget> GhostClass{ FSoftObjectPath(GhostPath) };
			Source->Tag = bHas ? ItemTag : NAME_None;
			Source->DragVisualClass = bHas ? GhostClass.LoadSynchronous() : nullptr;
		}
		static TSoftClassPtr<UDreamUserWidget> TipClass{ FSoftObjectPath(TipPath) };
		Root->SetToolTipWidgetClass(bHas ? TipClass.LoadSynchronous() : nullptr);
	}
	if (bHas)
	{
		const FNocturneItemDef& Def = NocturneItems::Items[Item];
		NocturneUI::SetText(Node(TEXT("SlotGlyph")), FText::FromString(Def.Glyph));
		NocturneUI::SetText(Node(TEXT("SlotCount")), FText::FromString(Def.Count > 1 ? FString::Printf(TEXT("x%d"), Def.Count) : FString()));
	}
	NocturneUI::SetShown(Node(TEXT("SlotGlyph")), bHas);
	NocturneUI::SetShown(Node(TEXT("SlotCount")), bHas);
	NocturneUI::SetShown(Node(TEXT("SlotRarity")), bHas);
	Paint();
}

void UNocturneItemSlot::SetSelected(bool bInSelected)
{
	const bool bNew = bInSelected && !bSelected;
	bSelected = bInSelected;
	Paint();
	if (bNew)
	{
		if (UDreamWidget* Mark = Node(TEXT("SlotMark")))
		{
			Mark->SetRenderScale(FVector(1.0, 0.0, 0.0));
			Mark->SetRenderTransformAngle(-135.0f);
			Mark->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.28f, 0.0f, EDreamTweenEase::OutCubic);
			Mark->RenderAngleTo(0.0f, 0.32f, 0.0f, EDreamTweenEase::OutCubic);
		}
	}
}

void UNocturneItemSlot::SetFiltered(bool bInPassesFilter)
{
	bPassesFilter = bInPassesFilter;
	if (UDreamWidget* Root = GetContentRoot())
	{
		Root->SetRenderOpacity(bPassesFilter ? 1.0f : 0.25f);
	}
}

void UNocturneItemSlot::HandleState(EUISelectableSelectionState InState)
{
	// The pad's focus lights a slot as the pointer does.
	const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed
		|| InState == EUISelectableSelectionState::Focused;
	if (bOver != bHovered)
	{
		bHovered = bOver;
		Paint();
		if (bHovered)
		{
			OnHovered.Broadcast(this);
		}
	}
}

void UNocturneItemSlot::HandleDropAccepted(UDreamDragDropOperation* Operation)
{
	bDropHover = false;
	Paint();
	UNocturneItemSlot* From = Operation != nullptr ? Cast<UNocturneItemSlot>(Operation->Payload) : nullptr;
	if (From != nullptr && From != this)
	{
		OnItemDropped.Broadcast(this, From);
	}
}

void UNocturneItemSlot::HandleDragEnter(UDreamDragDropOperation* Operation)
{
	bDropHover = Operation != nullptr && Operation->Payload != this;
	Paint();
}

void UNocturneItemSlot::HandleDragLeave(UDreamDragDropOperation* Operation)
{
	bDropHover = false;
	Paint();
}

void UNocturneItemSlot::Paint()
{
	using namespace NocturneUI;
	const bool bHas = Item != INDEX_NONE;
	SetTint(Node(TEXT("SlotBack")), bHovered ? FColor(0x8F, 0xE3, 0xFF, 0x1C) : (bHas ? FColor(0xFF, 0xFF, 0xFF, 0x0E) : FColor(0xFF, 0xFF, 0xFF, 0x06)));
	NocturneUI::SetShown(Node(TEXT("SlotSelect")), bSelected);
	NocturneUI::SetShown(Node(TEXT("SlotMark")), bSelected);
	NocturneUI::SetShown(Node(TEXT("SlotDrop")), bDropHover);
	if (bHas)
	{
		const FNocturneItemDef& Def = NocturneItems::Items[Item];
		SetTint(Node(TEXT("SlotRarity")), NocturneItems::RarityColor(Def.Rarity));
		SetTextColor(Node(TEXT("SlotGlyph")), bSelected || bHovered ? InkBright : Ink);
	}
	SetTextColor(Node(TEXT("SlotCode")), bSelected ? Cyan : InkFaint);
}

// ---------------------------------------------------------------------------------------------------------------------

void UNocturneItemGhost::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	bFilled = Fill();
	SetWantsTick(!bFilled);
}

void UNocturneItemGhost::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	if (!bFilled)
	{
		bFilled = Fill();
		SetWantsTick(!bFilled);
	}
}

bool UNocturneItemGhost::Fill()
{
	const UDreamUIDragDropSubsystem* Drags = UDreamUIDragDropSubsystem::Get(this);
	const UDreamDragDropOperation* Operation = Drags != nullptr ? Drags->GetDragOperationForPointer(0) : nullptr;
	const UNocturneItemSlot* Slot = Operation != nullptr ? Cast<UNocturneItemSlot>(Operation->Payload) : nullptr;
	if (Slot == nullptr || Slot->GetItem() == INDEX_NONE)
	{
		return false;
	}
	NocturneUI::SetText(NocturneItemSlotLocal::FindNode(this, TEXT("GhostGlyph")), FText::FromString(NocturneItems::Items[Slot->GetItem()].Glyph));
	return true;
}

// ---------------------------------------------------------------------------------------------------------------------

void UNocturneItemTip::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	bFilled = Fill();
	SetWantsTick(!bFilled);
}

void UNocturneItemTip::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	if (!bFilled)
	{
		bFilled = Fill();
		SetWantsTick(!bFilled);
	}
}

bool UNocturneItemTip::Fill()
{
	const UDreamUITooltipSubsystem* Tips = UDreamUITooltipSubsystem::Get(this);
	const UNocturneItemSlot* Slot = Tips != nullptr ? NocturneItemSlotLocal::SlotOf(Tips->GetShownForUser(0)) : nullptr;
	if (Slot == nullptr || Slot->GetItem() == INDEX_NONE)
	{
		return false;
	}
	using namespace NocturneUI;
	const FNocturneItemDef& Def = NocturneItems::Items[Slot->GetItem()];
	SetText(NocturneItemSlotLocal::FindNode(this, TEXT("TipName")), FText::FromString(Def.Name));
	SetText(NocturneItemSlotLocal::FindNode(this, TEXT("TipEn")), FText::FromString(Def.En));
	SetText(NocturneItemSlotLocal::FindNode(this, TEXT("TipKind")),
		FText::FromString(FString::Printf(TEXT("%s  ·  %s"), NocturneItems::KindName(Def.Kind), NocturneItems::RarityName(Def.Rarity))));
	SetTextColor(NocturneItemSlotLocal::FindNode(this, TEXT("TipKind")), NocturneItems::RarityColor(Def.Rarity));
	SetText(NocturneItemSlotLocal::FindNode(this, TEXT("TipText")), FText::FromString(FString::Printf(TEXT("来源   %s"), Def.Source)));
	SetTint(NocturneItemSlotLocal::FindNode(this, TEXT("TipTab")), NocturneItems::RarityColor(Def.Rarity));
	return true;
}
