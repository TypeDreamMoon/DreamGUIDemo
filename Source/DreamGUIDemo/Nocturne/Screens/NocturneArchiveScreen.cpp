// Copyright DreamMoon. All Rights Reserved.

#include "NocturneArchiveScreen.h"

#include "NocturneItems.h"
#include "NocturneItemSlot.h"
#include "../Settings/NocturneUI.h"

#include "Controls/DreamProgressBar.h"
#include "Core/Components/DreamImage.h"
#include "Core/DreamUIStylize.h"
#include "Extensions/Effects/DreamStylizeEffect.h"
#include "Engine/Texture2D.h"
#include "Core/Components/DreamWidget.h"
#include "Interaction/UIButton.h"

namespace NocturneArchiveLocal
{
	/** A grid cell's step, design units: 92 and a 12 gap. */
	constexpr double CellStep = 104.0;
	/** One card's flip, and the wave's delay a cell along or down. */
	constexpr float FlipSeconds = 0.34f;
	constexpr float FlipWave = 0.022f;
	constexpr float DecodeSeconds = 0.55f;
}

UNocturneItemSlot* UNocturneArchiveScreen::GetSlot(int32 InIndex) const
{
	return Slots.IsValidIndex(InIndex) ? Slots[InIndex].Get() : nullptr;
}

void UNocturneArchiveScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		UNocturneItemSlot* Slot = Cast<UNocturneItemSlot>(Node(*FString::Printf(TEXT("Slot%d"), Index)));
		Slots.Add(Slot);
		if (Slot == nullptr)
		{
			continue;
		}
		Slot->SetAnchoredPosition(FVector2D((Index % Columns) * NocturneArchiveLocal::CellStep, -(Index / Columns) * NocturneArchiveLocal::CellStep));
		Slot->Setup(Index);
		// In the order they were found; the rest of the grid is room to spare.
		Slot->SetItem(Index < NocturneItems::Count ? Index : INDEX_NONE);
		Slot->OnClicked.AddWeakLambda(this, [this](UNocturneItemSlot* InSlot) { Select(InSlot->GetSlotIndex()); });
		Slot->OnItemDropped.AddUObject(this, &UNocturneArchiveScreen::HandleDrop);
	}

	for (int32 Index = 0; Index < CategoryCount; ++Index)
	{
		UDreamWidget* Cat = Node(*FString::Printf(TEXT("Cat%d"), Index));
		UUIButton* Button = Cat != nullptr ? Cat->GetComponent<UUIButton>() : nullptr;
		if (Button == nullptr)
		{
			continue;
		}
		Button->SetUseFocusRing(false);
		Button->GetOnClickEvent().AddWeakLambda(this, [this, Index]() { ShowCategory(Index); });
		Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Index](EUISelectableSelectionState InState, bool)
		{
			const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
			HoveredCategory = bOver ? Index : (HoveredCategory == Index ? INDEX_NONE : HoveredCategory);
			PaintCategories();
		});
	}
	for (const TCHAR* Action : { TEXT("ActionUse"), TEXT("ActionDrop") })
	{
		UDreamWidget* Button = Node(Action);
		UUIButton* Behaviour = Button != nullptr ? Button->GetComponent<UUIButton>() : nullptr;
		if (Behaviour == nullptr)
		{
			continue;
		}
		Behaviour->SetUseFocusRing(false);
		const FString Back = FString(Action) + TEXT("Back");
		Behaviour->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Back](EUISelectableSelectionState InState, bool)
		{
			const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
			NocturneUI::SetTint(Node(*Back), bOver ? FColor(0x8F, 0xE3, 0xFF, 0x22) : FColor(0xFF, 0xFF, 0xFF, 0x08));
		});
	}

	if (const UDreamWidget* Holder = Node(TEXT("IconDecode")))
	{
		IconDecode = Cast<UDreamStylizeEffect>(Holder->GetVisual());
	}
	FlipSwapped.Init(true, Slots.Num());
	ShowCategory(0);
	Select(0);
	UpdateCounts();
}

void UNocturneArchiveScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturneArchiveLocal;
	Super::NativeOnTick(DeltaTime);
	const float Now = GetScreenTime();

	// The grid flips over in a wave; each card takes its new state as it turns past edge-on.
	if (FlipAt >= 0.0f)
	{
		bool bRunning = false;
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			UNocturneItemSlot* Slot = Slots[Index].Get();
			if (Slot == nullptr)
			{
				continue;
			}
			const float X = (Now - FlipAt - FlipWave * ((Index % Columns) + (Index / Columns))) / FlipSeconds;
			double Yaw = 0.0;
			bRunning |= X < 1.0f;
			if (X >= 0.5f && !FlipSwapped[Index])
			{
				FlipSwapped[Index] = true;
				Slot->SetFiltered(PassesFilter(Slot->GetItem()));
			}
			if (X >= 0.0f && X < 0.5f)
			{
				Yaw = 90.0 * FMath::Square(X * 2.0);
			}
			else if (X >= 0.5f && X < 1.0f)
			{
				Yaw = -90.0 * FMath::Square(1.0 - (X - 0.5) * 2.0);
			}
			Slot->SetRenderRotation(FRotator(0.0, Yaw, 0.0));
		}
		if (!bRunning)
		{
			FlipAt = -1.0f;
		}
	}

	// The item's picture floats on its stand, turning a little, and turns in from edge-on when it changes.
	if (UDreamWidget* Art = Node(TEXT("IconArt")))
	{
		const float Spin = FMath::Clamp((Now - IconAt) / 0.5f, 0.0f, 1.0f);
		const double In = -90.0 * FMath::Pow(1.0 - Spin, 3.0);
		Art->SetRenderRotation(FRotator(5.0 * FMath::Sin(Now * 0.6f + 0.5f), 12.0 * FMath::Sin(Now * 0.9f) + In, 0.0));
	}
	// ...and decodes: dithered to two levels a channel, up to sixteen.
	if (UDreamStylizeEffect* Decode = IconDecode.Get())
	{
		const float X = (Now - IconAt) / DecodeSeconds;
		const bool bOn = X >= 0.0f && X < 1.0f;
		if (bOn)
		{
			FDreamStylizeLayer Layer;
			Layer.Type = EDreamStylizeType::Dither;
			Layer.MatrixSize = 4;
			Layer.Levels = FMath::Clamp(FMath::RoundToInt(2.0f + 14.0f * X * X), 2, 16);
			Layer.PixelSize = 2.0f;
			Decode->SetLayers({ Layer });
		}
		if (bOn != bDecodeOn)
		{
			bDecodeOn = bOn;
			Decode->SetEnabled(bOn);
		}
	}
}

bool UNocturneArchiveScreen::PassesFilter(int32 InItem) const
{
	if (Category == 0)
	{
		return true;
	}
	if (InItem == INDEX_NONE)
	{
		return false;
	}
	const ENocturneItemKind Kinds[] = { ENocturneItemKind::Gear, ENocturneItemKind::Gear, ENocturneItemKind::Material, ENocturneItemKind::Record };
	return NocturneItems::Items[InItem].Kind == Kinds[Category];
}

void UNocturneArchiveScreen::ShowCategory(int32 InCategory)
{
	const int32 Previous = Category;
	Category = FMath::Clamp(InCategory, 0, CategoryCount - 1);
	if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Cat%dBar"), Category)); Bar != nullptr && Previous != Category)
	{
		// The lit bar slides over from the tab it was on.
		Settle(Bar);
		SetOffset(Bar, FVector2D(0.0, 70.0 * (Previous - Category)));
		Bar->RenderOffsetTo(FVector2D::ZeroVector, 0.2f, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (Previous != Category)
	{
		// The grid flips over to the new category.
		FlipAt = GetScreenTime();
		FlipSwapped.Init(false, Slots.Num());
	}
	else
	{
		for (const TWeakObjectPtr<UNocturneItemSlot>& Slot : Slots)
		{
			if (UNocturneItemSlot* Live = Slot.Get())
			{
				Live->SetFiltered(PassesFilter(Live->GetItem()));
			}
		}
	}
	PaintCategories();
	BurstSort(CenterSortIndex, 0.45f, 0.3f);
}

bool UNocturneArchiveScreen::SwitchTab(int32 InUserIndex, int32 InDelta)
{
	if (InDelta == 0)
	{
		return false;
	}
	ShowCategory((Category + InDelta % CategoryCount + CategoryCount) % CategoryCount);
	return true;
}

void UNocturneArchiveScreen::PaintCategories()
{
	using namespace NocturneUI;
	for (int32 Index = 0; Index < CategoryCount; ++Index)
	{
		const bool bOn = Index == Category;
		const bool bOver = Index == HoveredCategory;
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Cat%dBar"), Index)), bOn);
		SetTint(Node(*FString::Printf(TEXT("Cat%dBack"), Index)), bOn ? RowLit : (bOver ? RowHover : Clear));
		SetTextColor(Node(*FString::Printf(TEXT("Cat%dText"), Index)), bOn ? InkBright : (bOver ? Ink : InkDim));
		SetTextColor(Node(*FString::Printf(TEXT("Cat%dEn"), Index)), bOn ? CyanTag : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Cat%dNum"), Index)), bOn ? Cyan : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Cat%dCount"), Index)), bOn ? Cyan : InkDim);
	}
}

void UNocturneArchiveScreen::UpdateCounts()
{
	int32 Counts[CategoryCount] = { 0, 0, 0, 0 };
	for (const TWeakObjectPtr<UNocturneItemSlot>& Slot : Slots)
	{
		const UNocturneItemSlot* Live = Slot.Get();
		if (Live == nullptr || Live->GetItem() == INDEX_NONE)
		{
			continue;
		}
		++Counts[0];
		const ENocturneItemKind Kind = NocturneItems::Items[Live->GetItem()].Kind;
		++Counts[Kind == ENocturneItemKind::Gear ? 1 : (Kind == ENocturneItemKind::Material ? 2 : 3)];
	}
	for (int32 Index = 0; Index < CategoryCount; ++Index)
	{
		NocturneUI::SetText(Node(*FString::Printf(TEXT("Cat%dCount"), Index)), FText::AsNumber(Counts[Index]));
	}
	NocturneUI::SetText(Node(TEXT("StorageValue")), FText::FromString(FString::Printf(TEXT("%d / %d"), Counts[0], SlotCount)));
	if (UDreamProgressBar* Bar = Cast<UDreamProgressBar>(Node(TEXT("StorageBar"))))
	{
		Bar->SetPercent(static_cast<float>(Counts[0]) / SlotCount);
	}
}

void UNocturneArchiveScreen::Select(int32 InSlot)
{
	if (!Slots.IsValidIndex(InSlot))
	{
		return;
	}
	const bool bChanged = InSlot != Selected;
	if (UNocturneItemSlot* Previous = GetSlot(Selected))
	{
		Previous->SetSelected(false);
	}
	Selected = InSlot;
	if (UNocturneItemSlot* Slot = GetSlot(Selected))
	{
		Slot->SetSelected(true);
	}
	UpdateDetail(bChanged);
}

void UNocturneArchiveScreen::UpdateDetail(bool bInAnimate)
{
	using namespace NocturneUI;
	const UNocturneItemSlot* Slot = GetSlot(Selected);
	const int32 Item = Slot != nullptr ? Slot->GetItem() : INDEX_NONE;
	if (Item == INDEX_NONE)
	{
		SetText(Node(TEXT("IconGlyph")), FText::GetEmpty());
		SetText(Node(TEXT("DetailName")), FText::FromString(TEXT("空")));
		SetText(Node(TEXT("DetailEn")), FText::FromString(TEXT("EMPTY")));
		SetText(Node(TEXT("DetailKind")), FText::GetEmpty());
		SetText(Node(TEXT("DetailText")), FText::FromString(TEXT("这一格还没有东西。")));
		SetText(Node(TEXT("StatCount")), FText::FromString(TEXT("—")));
		SetText(Node(TEXT("StatSource")), FText::FromString(TEXT("—")));
		SetTint(Node(TEXT("IconRarity")), FColor(0xFF, 0xFF, 0xFF, 0x18));
	}
	else
	{
		const FNocturneItemDef& Def = NocturneItems::Items[Item];
		const FColor Rarity = NocturneItems::RarityColor(Def.Rarity);
		SetText(Node(TEXT("IconGlyph")), FText::FromString(Def.Glyph));
		SetText(Node(TEXT("DetailName")), FText::FromString(Def.Name));
		SetText(Node(TEXT("DetailEn")), FText::FromString(Def.En));
		SetText(Node(TEXT("DetailKind")), FText::FromString(FString::Printf(TEXT("%s   ·   %s"), NocturneItems::KindName(Def.Kind), NocturneItems::RarityName(Def.Rarity))));
		SetTextColor(Node(TEXT("DetailKind")), Rarity);
		SetText(Node(TEXT("DetailText")), FText::FromString(Def.Description));
		SetText(Node(TEXT("StatCount")), FText::AsNumber(Def.Count));
		SetText(Node(TEXT("StatSource")), FText::FromString(Def.Source));
		SetTint(Node(TEXT("IconRarity")), Rarity);
	}
	SetText(Node(TEXT("StatSlot")), FText::FromString(FString::Printf(TEXT("A-%02d"), Selected + 1)));
	// The item's picture (Art/Nocturne/Items), the glyph a badge on its corner.
	UTexture2D* Art = nullptr;
	if (Item != INDEX_NONE)
	{
		const FString Path = FString::Printf(TEXT("/Game/Nocturne/Textures/Items/T_Item_%02d.T_Item_%02d"), Item + 1, Item + 1);
		Art = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(Path)).LoadSynchronous();
	}
	if (UDreamWidget* ArtNode = Node(TEXT("IconArt")))
	{
		if (UDreamImage* Image = Cast<UDreamImage>(ArtNode->GetVisual()); Image != nullptr && Art != nullptr)
		{
			Image->SetBrush_Texture(Art);
		}
		NocturneUI::SetShown(ArtNode, Art != nullptr);
	}
	NocturneUI::SetShown(Node(TEXT("IconBadge")), Item != INDEX_NONE);
	if (bInAnimate)
	{
		IconAt = GetScreenTime();
		RunInChildren(Node(TEXT("DetailBody")), 0.0f, 0.02f, FVector2D(12.0, 0.0));
		BurstSort(RightSortIndex, 0.5f, 0.28f);
	}
}

void UNocturneArchiveScreen::HandleDrop(UNocturneItemSlot* Target, UNocturneItemSlot* Source)
{
	if (Target == nullptr || Source == nullptr)
	{
		return;
	}
	const int32 Moving = Source->GetItem();
	Source->SetItem(Target->GetItem());
	Target->SetItem(Moving);
	Source->SetFiltered(PassesFilter(Source->GetItem()));
	Target->SetFiltered(PassesFilter(Target->GetItem()));
	// The item lands with a small knock, and is what the detail shows.
	SetOffset(Target, FVector2D(0.0, -8.0));
	Target->RenderOffsetTo(FVector2D::ZeroVector, 0.3f, 0.0f, EDreamTweenEase::OutBack);
	Select(Target->GetSlotIndex());
	UpdateCounts();
	BurstSort(CenterSortIndex, 0.35f, 0.22f);
}

UDreamWidget* UNocturneArchiveScreen::GetInitialFocus() const
{
	return GetSlot(Selected != INDEX_NONE ? Selected : 0);
}

void UNocturneArchiveScreen::PlayContentEntrance(float Delay)
{
	RunInChildren(Node(TEXT("LeftHeader")), Delay, 0.03f, FVector2D(-16.0, 0.0));
	RunInChildren(Node(TEXT("CatList")), Delay + 0.04f, 0.03f, FVector2D(-20.0, 0.0));
	RunInChildren(Node(TEXT("Storage")), Delay + 0.1f, 0.03f, FVector2D(-12.0, 0.0));
	RunInChildren(Node(TEXT("PageHeader")), Delay + 0.06f, 0.03f, FVector2D(-16.0, 0.0));
	// The grid fills row by row, left to right.
	for (int32 Index = 0; Index < Slots.Num(); ++Index)
	{
		RunIn(Slots[Index].Get(), Delay + 0.1f + 0.035f * (Index / Columns) + 0.012f * (Index % Columns), 0.24f, FVector2D(0.0, 10.0));
	}
	RunIn(Node(TEXT("GridHint")), Delay + 0.35f, 0.3f, FVector2D(0.0, 6.0));
	RunInChildren(Node(TEXT("DetailBody")), Delay + 0.14f, 0.03f, FVector2D(16.0, 0.0));
	RunIn(Node(TEXT("ActionUse")), Delay + 0.3f, 0.28f, FVector2D(0.0, 10.0));
	RunIn(Node(TEXT("ActionDrop")), Delay + 0.33f, 0.28f, FVector2D(0.0, 10.0));
}
