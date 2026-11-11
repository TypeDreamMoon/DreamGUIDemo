// Copyright DreamMoon. All Rights Reserved.

#include "NocturneOptionRow.h"

#include "NocturneUI.h"

#include "Controls/DreamSlider.h"
#include "Core/Components/DreamRectBlock.h"
#include "Core/Components/DreamText.h"
#include "Core/Components/DreamVisual.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "Interaction/UIButton.h"

namespace NocturneOptionRowLocal
{
	/** The segment boxes the row's .dui carries (Seg0 ... Seg4), their width and the gap between them. */
	constexpr int32 SegmentCount = 5;
	constexpr float SegmentWidth = 72.0f;
	constexpr float SegmentGap = 4.0f;
}

void NocturneUI::SetText(UDreamWidget* Widget, const FText& Text)
{
	if (UDreamText* Visual = IsValid(Widget) ? Cast<UDreamText>(Widget->GetVisual()) : nullptr)
	{
		if (!Visual->GetText().EqualTo(Text))
		{
			Visual->SetText(Text);
		}
	}
}

void NocturneUI::SetTextColor(UDreamWidget* Widget, const FColor& Color)
{
	if (UDreamVisual* Visual = IsValid(Widget) ? Widget->GetVisual() : nullptr)
	{
		Visual->SetColor(Color);
	}
}

void NocturneUI::SetTint(UDreamWidget* Widget, const FColor& Color)
{
	if (!IsValid(Widget))
	{
		return;
	}
	if (UDreamRectBlock* Rect = Cast<UDreamRectBlock>(Widget->GetVisual()))
	{
		Rect->SetBodyColor(Color);
	}
	else if (UDreamVisual* Visual = Widget->GetVisual())
	{
		Visual->SetColor(Color);
	}
}

void NocturneUI::SetShown(UDreamWidget* Widget, bool bShown)
{
	if (IsValid(Widget))
	{
		Widget->SetVisibility(bShown ? EDreamWidgetVisibility::Visible : EDreamWidgetVisibility::Collapsed);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

UDreamWidget* UNocturneOptionRow::Node(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

void UNocturneOptionRow::NativeOnInitialized()
{
	using namespace NocturneOptionRowLocal;
	Super::NativeOnInitialized();

	Back = Node(TEXT("Back"));
	Accent = Node(TEXT("Accent"));
	LabelText = Node(TEXT("LabelText"));
	EnText = Node(TEXT("EnText"));
	ArrowL = Node(TEXT("ArrowL"));
	ArrowR = Node(TEXT("ArrowR"));
	SelValue = Node(TEXT("SelValue"));
	SliderValue = Node(TEXT("SliderValue"));
	SliderCtl = Cast<UDreamSlider>(Node(TEXT("SliderCtl")));
	Segments = Node(TEXT("Segments"));
	KeyCap = Node(TEXT("KeyCap"));
	KeyText = Node(TEXT("KeyText"));
	ActionText = Node(TEXT("ActionText"));

	if (Kind == ENocturneOptionKind::Toggle && Values.IsEmpty())
	{
		Values = TEXT("关|开");
	}

	NocturneUI::SetText(LabelText.Get(), Label);
	NocturneUI::SetText(EnText.Get(), FText::FromString(En));
	NocturneUI::SetShown(Node(TEXT("Selector")), Kind == ENocturneOptionKind::Selector);
	NocturneUI::SetShown(Node(TEXT("SliderBox")), Kind == ENocturneOptionKind::Slider);
	NocturneUI::SetShown(Segments.Get(), Kind == ENocturneOptionKind::Segment || Kind == ENocturneOptionKind::Toggle);
	NocturneUI::SetShown(Node(TEXT("KeyBox")), Kind == ENocturneOptionKind::Key);
	NocturneUI::SetShown(ActionText.Get(), Kind == ENocturneOptionKind::Action);

	// Segments: as many boxes as values, the rest folded away, the row of them sized to fit from the right edge.
	const TArray<FString> List = GetValueList();
	const int32 Used = FMath::Clamp(List.Num(), 0, SegmentCount);
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		UDreamWidget* Segment = Node(*FString::Printf(TEXT("Seg%d"), Index));
		SegBoxes.Add(Node(*FString::Printf(TEXT("Seg%dBox"), Index)));
		SegTexts.Add(Node(*FString::Printf(TEXT("Seg%dText"), Index)));
		NocturneUI::SetShown(Segment, Index < Used);
		if (Index < Used)
		{
			NocturneUI::SetText(SegTexts.Last().Get(), FText::FromString(List[Index]));
			if (UUIButton* Button = IsValid(Segment) ? Segment->GetComponent<UUIButton>() : nullptr)
			{
				Button->GetOnClickEvent().AddWeakLambda(this, [this, Index]() { HandleSegmentClick(Index); });
			}
		}
	}
	if (UDreamWidget* Box = Segments.Get(); Box != nullptr && Used > 0)
	{
		Box->SetWidth(Used * SegmentWidth + (Used - 1) * SegmentGap);
	}

	if (UDreamSlider* Slider = SliderCtl.Get())
	{
		bSettingSlider = true;
		Slider->SetMinValue(Min);
		Slider->SetMaxValue(Max);
		Slider->SetValue(Value);
		bSettingSlider = false;
		Slider->OnValueChanged.AddDynamic(this, &UNocturneOptionRow::HandleSliderValue);
	}

	DefaultIndex = ValueIndex;
	DefaultValue = Value;
	DefaultKey = Values;
	ShowValue();

	if (UDreamWidget* Root = GetContentRoot())
	{
		Selectable = Root->GetComponent<UUISelectable>();
	}
	if (UUISelectable* Live = Selectable.Get())
	{
		Live->SetUseFocusRing(false);
		Live->GetOnSelectionStateChangedEvent().AddUObject(this, &UNocturneOptionRow::HandleState);
		if (UUIButton* Button = Cast<UUIButton>(Live))
		{
			Button->GetOnClickEvent().AddUObject(this, &UNocturneOptionRow::HandleClick);
		}
	}
	Paint();
}

void UNocturneOptionRow::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	RowTime += DeltaTime;
	if (bListening)
	{
		// The key cap blinks while it waits.
		const bool bOn = FMath::Fmod(RowTime, 0.7f) < 0.42f;
		NocturneUI::SetTextColor(KeyText.Get(), bOn ? NocturneUI::Amber : NocturneUI::InkFaint);
	}
}

bool UNocturneOptionRow::HasRowFocus() const
{
	const UDreamWidget* Focus = GetFocusWidget();
	return Focus != nullptr && Focus->HasFocus();
}

void UNocturneOptionRow::SetHighlighted(bool bInHighlighted)
{
	bHighlighted = bInHighlighted;
	if (!bHighlighted && bListening)
	{
		SetListening(false);
	}
	Paint();
}

bool UNocturneOptionRow::Adjust(int32 Delta)
{
	if (Delta == 0)
	{
		return false;
	}
	switch (Kind)
	{
	case ENocturneOptionKind::Selector:
	{
		const int32 Num = GetValueList().Num();
		if (Num == 0)
		{
			return false;
		}
		SetValueInternal((ValueIndex + Delta % Num + Num) % Num, Value, false);
		// The arrow on the side it went knocks outward and springs back.
		if (UDreamWidget* Arrow = Delta < 0 ? ArrowL.Get() : ArrowR.Get())
		{
			Arrow->SetRenderTranslation(FVector(0.0, Delta < 0 ? -8.0 : 8.0, 0.0));
			Arrow->RenderOffsetTo(FVector2D::ZeroVector, 0.18f, 0.0f, EDreamTweenEase::OutCubic);
		}
		return true;
	}
	case ENocturneOptionKind::Segment:
	case ENocturneOptionKind::Toggle:
	{
		const int32 Num = GetValueList().Num();
		if (Num == 0)
		{
			return false;
		}
		SetValueInternal(FMath::Clamp(ValueIndex + Delta, 0, Num - 1), Value, false);
		return true;
	}
	case ENocturneOptionKind::Slider:
		SetValueInternal(ValueIndex, FMath::Clamp(Value + Delta * Step, Min, Max), false);
		return true;
	default:
		return false;
	}
}

void UNocturneOptionRow::ResetValue()
{
	if (Kind == ENocturneOptionKind::Key)
	{
		Values = DefaultKey;
		ShowValue();
		return;
	}
	SetValueInternal(DefaultIndex, DefaultValue, false);
}

bool UNocturneOptionRow::IsChanged() const
{
	switch (Kind)
	{
	case ENocturneOptionKind::Selector:
	case ENocturneOptionKind::Segment:
	case ENocturneOptionKind::Toggle:
		return ValueIndex != DefaultIndex;
	case ENocturneOptionKind::Slider:
		return !FMath::IsNearlyEqual(Value, DefaultValue, Step * 0.5f);
	case ENocturneOptionKind::Key:
		return Values != DefaultKey;
	default:
		return false;
	}
}

FText UNocturneOptionRow::GetValueText() const
{
	switch (Kind)
	{
	case ENocturneOptionKind::Selector:
	case ENocturneOptionKind::Segment:
	case ENocturneOptionKind::Toggle:
	{
		const TArray<FString> List = GetValueList();
		return List.IsValidIndex(ValueIndex) ? FText::FromString(List[ValueIndex]) : FText::GetEmpty();
	}
	case ENocturneOptionKind::Slider:
		return FormatSliderValue(Value);
	case ENocturneOptionKind::Key:
		return FText::FromString(Values);
	default:
		return FText::GetEmpty();
	}
}

FText UNocturneOptionRow::GetDefaultValueText() const
{
	switch (Kind)
	{
	case ENocturneOptionKind::Selector:
	case ENocturneOptionKind::Segment:
	case ENocturneOptionKind::Toggle:
	{
		const TArray<FString> List = GetValueList();
		return List.IsValidIndex(DefaultIndex) ? FText::FromString(List[DefaultIndex]) : FText::GetEmpty();
	}
	case ENocturneOptionKind::Slider:
		return FormatSliderValue(DefaultValue);
	case ENocturneOptionKind::Key:
		return FText::FromString(DefaultKey);
	default:
		return FText::GetEmpty();
	}
}

void UNocturneOptionRow::SetListening(bool bInListening)
{
	bListening = bInListening && Kind == ENocturneOptionKind::Key;
	SetWantsTick(bListening);
	ShowValue();
	Paint();
}

void UNocturneOptionRow::TakeKey(const FKey& InKey)
{
	Values = InKey.GetDisplayName().ToString();
	SetListening(false);
	OnValueChanged.Broadcast(this);
}

void UNocturneOptionRow::HandleState(EUISelectableSelectionState InState, bool bInImmediate)
{
	const bool bWasHovered = bHovered;
	bHovered = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
	if (bWasHovered != bHovered)
	{
		Paint();
	}
	OnStateChanged.Broadcast(this, InState);
}

void UNocturneOptionRow::HandleClick()
{
	// A click on the row itself: a selector steps on, a toggle flips, a key starts listening; segments and the slider
	// take their own clicks.
	if (Kind == ENocturneOptionKind::Selector)
	{
		Adjust(1);
	}
	else if (Kind == ENocturneOptionKind::Toggle)
	{
		SetValueInternal(ValueIndex == 0 ? 1 : 0, Value, false);
	}
	else if (Kind == ENocturneOptionKind::Key)
	{
		SetListening(!bListening);
	}
	OnClicked.Broadcast(this);
}

void UNocturneOptionRow::HandleSegmentClick(int32 Index)
{
	SetValueInternal(Index, Value, false);
	OnClicked.Broadcast(this);
}

void UNocturneOptionRow::HandleSliderValue(float InValue)
{
	if (bSettingSlider)
	{
		return;
	}
	const float Snapped = Step > 0.0f ? FMath::GridSnap(InValue, Step) : InValue;
	SetValueInternal(ValueIndex, FMath::Clamp(Snapped, Min, Max), true);
}

void UNocturneOptionRow::SetValueInternal(int32 InIndex, float InValue, bool bFromSlider)
{
	const bool bChanged = InIndex != ValueIndex || !FMath::IsNearlyEqual(InValue, Value);
	ValueIndex = InIndex;
	Value = InValue;
	if (!bFromSlider)
	{
		if (UDreamSlider* Slider = SliderCtl.Get())
		{
			bSettingSlider = true;
			Slider->SetValue(Value);
			bSettingSlider = false;
		}
	}
	ShowValue();
	Paint();
	if (bChanged)
	{
		OnValueChanged.Broadcast(this);
	}
}

void UNocturneOptionRow::ShowValue()
{
	switch (Kind)
	{
	case ENocturneOptionKind::Selector:
		NocturneUI::SetText(SelValue.Get(), GetValueText());
		break;
	case ENocturneOptionKind::Slider:
		NocturneUI::SetText(SliderValue.Get(), GetValueText());
		break;
	case ENocturneOptionKind::Key:
		NocturneUI::SetText(KeyText.Get(), bListening ? FText::FromString(TEXT("按下按键")) : GetValueText());
		break;
	case ENocturneOptionKind::Action:
		NocturneUI::SetText(ActionText.Get(), FText::FromString(Values + TEXT("  >")));
		break;
	default:
		break;
	}
}

void UNocturneOptionRow::Paint()
{
	using namespace NocturneUI;
	SetTint(Back.Get(), bHighlighted ? RowLit : (bHovered ? RowHover : Clear));
	NocturneUI::SetShown(Accent.Get(), bHighlighted);
	SetTextColor(LabelText.Get(), bHighlighted ? InkBright : Ink);
	SetTextColor(EnText.Get(), bHighlighted ? CyanTag : InkFaint);
	SetTextColor(ArrowL.Get(), bHighlighted ? Cyan : InkFaint);
	SetTextColor(ArrowR.Get(), bHighlighted ? Cyan : InkFaint);
	SetTextColor(SelValue.Get(), bHighlighted ? InkBright : Ink);
	SetTextColor(SliderValue.Get(), bHighlighted ? Cyan : Ink);
	SetTextColor(ActionText.Get(), bHighlighted ? Cyan : InkDim);
	SetTint(KeyCap.Get(), bListening ? FColor(0xFF, 0xB4, 0x54, 0x40) : (bHighlighted ? FColor(0x8F, 0xE3, 0xFF, 0x26) : FColor(0xFF, 0xFF, 0xFF, 0x12)));
	if (!bListening)
	{
		SetTextColor(KeyText.Get(), bHighlighted ? InkBright : Ink);
	}
	for (int32 Index = 0; Index < SegBoxes.Num(); ++Index)
	{
		const bool bOn = Index == ValueIndex;
		SetTint(SegBoxes[Index].Get(), bOn ? SegOn : (bHighlighted ? SegOffLit : SegOff));
		SetTextColor(SegTexts[Index].Get(), bOn ? (bHighlighted ? InkBright : Cyan) : InkDim);
	}
}

TArray<FString> UNocturneOptionRow::GetValueList() const
{
	TArray<FString> List;
	Values.ParseIntoArray(List, TEXT("|"));
	return List;
}

FText UNocturneOptionRow::FormatSliderValue(float InValue) const
{
	const FString Number = Step >= 1.0f ? FString::Printf(TEXT("%d"), FMath::RoundToInt(InValue)) : FString::Printf(TEXT("%.2f"), InValue);
	return FText::FromString(Unit.IsEmpty() ? Number : Number + Unit);
}
