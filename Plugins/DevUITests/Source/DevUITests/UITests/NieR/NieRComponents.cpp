#include "NieRComponents.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamRectBlock.h"
#include "Core/Components/DreamText.h"
#include "Core/Components/DreamVisual.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "DreamTweener.h"
#include "Engine/Texture2D.h"
#include "Interaction/UIButton.h"
#include "Misc/CommandLine.h"

namespace NieRComponentsLocal
{
	/** How long the pointer takes to glide over from the row before, and its idle afterwards. */
	constexpr float CursorGlide = 0.14f;
	constexpr float CursorIdleAmplitude = 4.0f;
	constexpr float CursorIdlePeriod = 1.1f;
	constexpr int32 SegmentCount = 10;

	/** The open tab's ink box: the tab's own 48 px, dropping to the rule 72 px down. */
	constexpr float TabHeight = 48.0f;
	constexpr float OpenTabHeight = 72.0f;

	void SetImageTexture(UDreamWidget* Widget, UTexture2D* Texture)
	{
		if (UDreamImage* Image = IsValid(Widget) && Texture != nullptr ? Cast<UDreamImage>(Widget->GetVisual()) : nullptr)
		{
			Image->SetBrush_Texture(Texture);
		}
	}
}

bool NieRUI::UseFocusRing()
{
	return FParse::Param(FCommandLine::Get(), TEXT("NieRFocusRing"));
}

// ---------------------------------------------------------------------------------------------------------------------

UDreamWidget* UNieRComponent::Node(FName Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(Name) : nullptr;
}

void UNieRComponent::SetText(UDreamWidget* Widget, const FText& Text)
{
	if (UDreamText* TextVisual = IsValid(Widget) ? Cast<UDreamText>(Widget->GetVisual()) : nullptr)
	{
		TextVisual->SetText(Text);
	}
}

FText UNieRComponent::GetText(const UDreamWidget* Widget)
{
	const UDreamText* TextVisual = IsValid(Widget) ? Cast<UDreamText>(Widget->GetVisual()) : nullptr;
	return TextVisual != nullptr ? TextVisual->GetText() : FText::GetEmpty();
}

void UNieRComponent::SetShown(UDreamWidget* Widget, bool bShown)
{
	if (IsValid(Widget))
	{
		Widget->SetVisibility(bShown ? EDreamWidgetVisibility::Visible : EDreamWidgetVisibility::Collapsed);
	}
}

void UNieRComponent::SetTint(UDreamWidget* Widget, const FColor& Color)
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

// ---------------------------------------------------------------------------------------------------------------------

void UNieRRow::NativeOnInitialized()
{
	using namespace NieRComponentsLocal;
	Super::NativeOnInitialized();
	Bar = Node(TEXT("Bar"));
	Mark = Node(TEXT("Mark"));
	Hex = Node(TEXT("Hex"));
	LineT = Node(TEXT("LineT"));
	LineB = Node(TEXT("LineB"));
	Cursor = Node(TEXT("Pointer"));
	ArrowL = Node(TEXT("ArrowL"));
	ArrowR = Node(TEXT("ArrowR"));
	CycleValue = Node(TEXT("ValueText"));
	GaugeValue = Node(TEXT("GaugeText"));
	UDreamWidget* LabelNode = Node(TEXT("LabelText"));
	UDreamWidget* CountNode = Node(TEXT("CountText"));
	Inked = { LabelNode, CountNode, ArrowL, CycleValue, ArrowR, GaugeValue };
	for (int32 Index = 0; Index < SegmentCount; ++Index)
	{
		Segments.Add(Node(*FString::Printf(TEXT("Seg%d"), Index)));
	}

	SetText(LabelNode, Label);
	SetText(CountNode, Count);
	SetShown(CountNode, Kind == ENieRRowKind::Count);
	SetShown(Node(TEXT("Cycle")), Kind == ENieRRowKind::Cycle);
	SetShown(Node(TEXT("Gauge")), Kind == ENieRRowKind::Bar);
	SetShown(Mark.Get(), Look == ENieRRowLook::Box);
	SetShown(Hex.Get(), Look == ENieRRowLook::List);
	if (UDreamWidget* Pointer = Cursor.Get())
	{
		Pointer->SetHorizontalAnchoredPosition(CursorX);
	}
	SetShown(LineT.Get(), false);
	SetShown(LineB.Get(), false);
	SetShown(Cursor.Get(), false);
	DefaultValueIndex = ValueIndex;
	DefaultBarValue = BarValue;
	ShowValue();

	if (UDreamWidget* Root = GetContentRoot())
	{
		Selectable = Root->GetComponent<UUISelectable>();
	}
	if (UUISelectable* Live = Selectable.Get())
	{
		Live->SetUseFocusRing(NieRUI::UseFocusRing());
		Live->GetOnSelectionStateChangedEvent().AddUObject(this, &UNieRRow::HandleState);
		if (UUIButton* Button = Cast<UUIButton>(Live))
		{
			Button->GetOnClickEvent().AddUObject(this, &UNieRRow::HandleClick);
		}
	}
	Paint();
}

void UNieRRow::SetLabel(const FText& InLabel)
{
	Label = InLabel;
	SetText(Node(TEXT("LabelText")), Label);
}

void UNieRRow::SetCount(const FText& InCount)
{
	Count = InCount;
	SetText(Node(TEXT("CountText")), Count);
}

void UNieRRow::SetDescription(const FText& InDescription)
{
	// Read by the screen when the row is highlighted; nothing of the row's own shows it.
	Description = InDescription;
}

void UNieRRow::NativeOnTick(float DeltaTime)
{
	using namespace NieRComponentsLocal;
	Super::NativeOnTick(DeltaTime);
	RowTime += DeltaTime;
	// Once it has glided in, the highlighted row's pointer idles a few pixels back and forth.
	if (bHighlighted && RowTime - HighlightTime > CursorGlide)
	{
		const float Phase = (RowTime - HighlightTime - CursorGlide) * UE_TWO_PI / CursorIdlePeriod;
		FNieRMotion::SetOffset(Cursor.Get(), FVector2D(-CursorIdleAmplitude * 0.5f * (1.0f - FMath::Cos(Phase)), 0.0));
	}
}

bool UNieRRow::HasRowFocus() const
{
	const UDreamWidget* Focus = GetFocusWidget();
	return Focus != nullptr && Focus->HasFocus();
}

void UNieRRow::SetKeyboardFocusOnRow()
{
	if (UDreamWidget* Focus = GetFocusWidget())
	{
		Focus->SetKeyboardFocus();
	}
}

void UNieRRow::SetHighlighted(bool bInHighlighted, const UNieRRow* InFrom)
{
	using namespace NieRComponentsLocal;
	const bool bTurningOn = bInHighlighted && !bHighlighted;
	bHighlighted = bInHighlighted;
	SetWantsTick(bHighlighted);
	SetShown(LineT.Get(), bHighlighted);
	SetShown(LineB.Get(), bHighlighted);
	SetShown(Cursor.Get(), bHighlighted);
	if (!bHighlighted)
	{
		Motion.Settle(LineT.Get());
		Motion.Settle(LineB.Get());
		Motion.Settle(Cursor.Get());
	}
	else if (bTurningOn)
	{
		HighlightTime = RowTime;
		// The lines open from the left; the pointer glides over from the row before when it sits in the same list.
		Motion.GrowX(LineT.Get(), 0.18f);
		Motion.GrowX(LineB.Get(), 0.18f);
		if (IsValid(InFrom) && InFrom->GetParent() == GetParent())
		{
			const FVector Local = GetWorldTransform().InverseTransformVector(InFrom->GetWorldLocation() - GetWorldLocation());
			Motion.Glide(Cursor.Get(), FVector2D(Local.Y, -Local.Z), CursorGlide);
		}
		else
		{
			Motion.SlideIn(Cursor.Get(), FVector2D(-20.0, 0.0), CursorGlide);
		}
	}
	Paint();
}

bool UNieRRow::Adjust(int32 Delta)
{
	if (Delta == 0)
	{
		return false;
	}
	if (Kind == ENieRRowKind::Cycle)
	{
		const int32 Num = GetValueList().Num();
		if (Num == 0)
		{
			return false;
		}
		ValueIndex = (ValueIndex + Delta % Num + Num) % Num;
		ShowValue();
		// The arrow on the side it went knocks outward and springs back; the value fades up.
		Motion.Nudge(Delta < 0 ? ArrowL.Get() : ArrowR.Get(), FVector2D(Delta < 0 ? -7.0 : 7.0, 0.0));
		Motion.FadeIn(CycleValue.Get(), 0.16f, 0.25f);
		return true;
	}
	if (Kind == ENieRRowKind::Bar)
	{
		BarValue = FMath::Clamp(BarValue + Delta, 0, BarMax);
		ShowValue();
		Motion.FadeIn(GaugeValue.Get(), 0.16f, 0.25f);
		return true;
	}
	return false;
}

void UNieRRow::ResetValue()
{
	ValueIndex = DefaultValueIndex;
	BarValue = DefaultBarValue;
	ShowValue();
}

FText UNieRRow::GetValueText() const
{
	if (Kind == ENieRRowKind::Cycle)
	{
		const TArray<FString> List = GetValueList();
		return List.IsValidIndex(ValueIndex) ? FText::FromString(List[ValueIndex]) : FText::GetEmpty();
	}
	if (Kind == ENieRRowKind::Bar)
	{
		return FText::AsNumber(BarValue);
	}
	return Count;
}

void UNieRRow::HandleState(EUISelectableSelectionState InState, bool bInImmediate)
{
	OnStateChanged.Broadcast(this, InState);
}

void UNieRRow::HandleClick()
{
	OnClicked.Broadcast(this);
}

void UNieRRow::ShowValue()
{
	if (Kind == ENieRRowKind::Cycle)
	{
		SetText(CycleValue.Get(), GetValueText());
	}
	else if (Kind == ENieRRowKind::Bar)
	{
		SetText(GaugeValue.Get(), GetValueText());
		Paint();
	}
}

void UNieRRow::Paint()
{
	using namespace NieRComponentsLocal;
	const FColor Clear(0, 0, 0, 0);
	SetTint(Bar.Get(), bHighlighted ? NieRUI::Ink : (Look == ENieRRowLook::List ? Clear : BoxColor));
	SetTint(Mark.Get(), bHighlighted ? NieRUI::PaperMarker : NieRUI::Mark);
	SetTint(Hex.Get(), bHighlighted ? NieRUI::PaperMarker : NieRUI::Mark);
	for (const TWeakObjectPtr<UDreamWidget>& Part : Inked)
	{
		SetTint(Part.Get(), bHighlighted ? NieRUI::Paper : NieRUI::InkText);
	}
	const int32 Lit = FMath::RoundToInt(static_cast<float>(SegmentCount) * BarValue / FMath::Max(1, BarMax));
	for (int32 Index = 0; Index < Segments.Num(); ++Index)
	{
		const bool bLit = Index < Lit;
		SetTint(Segments[Index].Get(), bHighlighted ? (bLit ? NieRUI::Paper : NieRUI::PaperDim) : (bLit ? NieRUI::Ink : NieRUI::InkDim));
	}
}

TArray<FString> UNieRRow::GetValueList() const
{
	TArray<FString> List;
	Values.ParseIntoArray(List, TEXT("|"));
	return List;
}

// ---------------------------------------------------------------------------------------------------------------------

void UNieRTab::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetText(Node(TEXT("LabelText")), Label);
	NieRComponentsLocal::SetImageTexture(Node(TEXT("IconImage")), Icon);
	if (UDreamWidget* Root = GetContentRoot())
	{
		if (UUIButton* Button = Root->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(false);
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { OnClicked.Broadcast(this); });
		}
	}
	SetOpen(false);
}

void UNieRTab::SetOpen(bool bInOpen, int32 InDirection)
{
	using namespace NieRComponentsLocal;
	UDreamWidget* SelBox = Node(TEXT("SelBox"));
	UDreamWidget* Pointer = Node(TEXT("Pointer"));
	SetShown(SelBox, bInOpen);
	SetShown(Node(TEXT("Box")), !bInOpen);
	SetShown(Pointer, bInOpen);
	SetTint(Node(TEXT("IconImage")), bInOpen ? NieRUI::Paper : NieRUI::InkText);
	SetTint(Node(TEXT("LabelText")), bInOpen ? NieRUI::Paper : NieRUI::InkText);
	if (bInOpen && InDirection != 0 && IsValid(SelBox))
	{
		// The ink box drops from the tab down to the rule; the pointer comes in from the side the tab was switched from.
		Motion.Stop(SelBox);
		SelBox->SetHeight(TabHeight);
		SelBox->HeightTo(OpenTabHeight, 0.16f, 0.0f, EDreamTweenEase::OutCubic);
		Motion.SlideIn(Pointer, FVector2D(-16.0 * InDirection, 0.0), 0.16f);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void UNieRHeader::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetTitle(Title, Subtitle, false);
}

void UNieRHeader::SetTitle(const FText& InTitle, const FText& InSubtitle, bool bInAnimate)
{
	Title = InTitle;
	SetText(Node(TEXT("TitleText")), InTitle);
	SetText(Node(TEXT("TitleShadow")), InTitle);
	SetSubtitle(InSubtitle, false);
	if (bInAnimate)
	{
		Motion.FadeIn(Node(TEXT("TitleLine")), 0.22f);
		Motion.FadeIn(Node(TEXT("TitleShadow")), 0.22f);
	}
}

void UNieRHeader::SetSubtitle(const FText& InSubtitle, bool bInAnimate)
{
	Subtitle = InSubtitle;
	SetText(Node(TEXT("SubText")), InSubtitle);
	if (bInAnimate)
	{
		Motion.FadeIn(Node(TEXT("SubText")), 0.2f);
	}
}

void UNieRHeader::PlayEntrance(float Delay)
{
	Motion.SlideIn(Node(TEXT("TitleLine")), FVector2D(-30.0, 0.0), 0.3f, Delay);
	Motion.SlideIn(Node(TEXT("TitleShadow")), FVector2D(-30.0, 0.0), 0.3f, Delay);
}

// ---------------------------------------------------------------------------------------------------------------------

void UNieRHintBar::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetText(Node(TEXT("HintText")), Hint);
}

void UNieRHintBar::SetHint(const FText& InHint)
{
	if (Hint.EqualTo(InHint))
	{
		return;
	}
	Hint = InHint;
	SetText(Node(TEXT("HintText")), InHint);
	Motion.FadeIn(Node(TEXT("HintText")), 0.18f, 0.2f);
}

// ---------------------------------------------------------------------------------------------------------------------

void UNieRPrompt::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	TArray<FString> KeyNames;
	Keys.ParseIntoArrayWS(KeyNames);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		const bool bUsed = KeyNames.IsValidIndex(Index);
		SetShown(Node(*FString::Printf(TEXT("Key%d"), Index)), bUsed);
		SetText(Node(*FString::Printf(TEXT("KeyText%d"), Index)), bUsed ? FText::FromString(KeyNames[Index]) : FText::GetEmpty());
	}
	SetText(Node(TEXT("LabelText")), Label);
}

// ---------------------------------------------------------------------------------------------------------------------

void UNieRListPage::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	NieRComponentsLocal::SetImageTexture(Node(TEXT("IconImage")), Icon);
}

void UNieRListPage::ShowRow(const UNieRRow* InRow)
{
	if (!IsValid(InRow))
	{
		return;
	}
	SetText(Node(TEXT("DetailTitle")), InRow->Label);
	SetText(Node(TEXT("DetailBody")), InRow->Description);
	Motion.FadeIn(Node(TEXT("DetailTitle")), 0.16f, 0.2f);
	Motion.FadeIn(Node(TEXT("DetailBody")), 0.2f, 0.0f);
	Motion.FadeIn(Node(TEXT("IconImage")), 0.2f, 0.4f);
	if (bShowCount)
	{
		SetText(Node(TEXT("DetailCount")), FText::FromString(FString::Printf(TEXT("Number Held: %s/99"), *InRow->Count.ToString())));
	}
}
