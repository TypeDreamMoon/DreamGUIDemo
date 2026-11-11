// Copyright DreamMoon. All Rights Reserved.

#include "NocturneStarLabel.h"

#include "../NocturneStars.h"
#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "DreamTweenManager.h"
#include "Interaction/UIButton.h"

namespace NocturneStarLabelLocal
{
	/** How fast the focus arc turns, degrees a second, and how long the locked flash lasts. */
	constexpr float ArcSpeed = 50.0f;
	constexpr float FlashSeconds = 0.6f;

	// The plate behind the text (Nocturne_StarLabel.dui): its left edge, how far it runs past the text, its height
	// over the name row, and how much more it takes in over the description.
	constexpr float PlateLeft = 116.0f;
	constexpr float PlateMargin = 22.0f;
	constexpr float PlateHeight = 108.0f;
	constexpr float PlateDetailsHeight = 60.0f;
	const FColor PlateIdle(0x05, 0x08, 0x0C, 0x8C);
	const FColor PlateLit(0x07, 0x0A, 0x0E, 0xC0);

	/** Roughly how wide a line of text is drawn: a CJK glyph is a square of the font size, a Latin one about half of it. */
	float TextWidth(const FString& Text, float FontSize, float Spacing)
	{
		float Width = 0.0f;
		for (const TCHAR Char : Text)
		{
			Width += (Char >= 0x2E80 ? FontSize : FontSize * 0.5f) + Spacing;
		}
		return Width;
	}

	/** Stop what is running on a part before it is sent somewhere else: two fades on one part fight to the last frame. */
	void Settle(UDreamWidget* Widget)
	{
		if (IsValid(Widget))
		{
			UDreamTweenManager::KillAllTweensOnTarget(Widget, Widget);
		}
	}
}

UDreamWidget* UNocturneStarLabel::Node(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

void UNocturneStarLabel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (UDreamWidget* Root = GetContentRoot())
	{
		if (UUIButton* Button = Root->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(false);
			// The label is never a Tab stop: the menu's route list carries the keyboard focus.
			Root->SetIsFocusable(false);
			Button->GetOnSelectionStateChangedEvent().AddUObject(this, &UNocturneStarLabel::HandleState);
			Button->GetOnClickEvent().AddUObject(this, &UNocturneStarLabel::HandleClick);
		}
	}
	SetWantsTick(true);
	ApplyFocusAt(0.0f);
	Paint();
}

void UNocturneStarLabel::Setup(int32 InIndex)
{
	using namespace NocturneStarLabelLocal;
	Index = FMath::Clamp(InIndex, 0, NocturneStars::Count - 1);
	const FNocturneStarDef& Star = NocturneStars::Stars[Index];
	bOpen = Star.bOpen;
	NocturneUI::SetText(Node(TEXT("Number")), FText::FromString(FString::Printf(TEXT("%02d"), Index + 1)));
	NocturneUI::SetText(Node(TEXT("GlyphText")), FText::FromString(Star.Glyph));
	NocturneUI::SetText(Node(TEXT("NameText")), FText::FromString(Star.Name));
	NocturneUI::SetText(Node(TEXT("EnText")), FText::FromString(Star.En));
	NocturneUI::SetText(Node(TEXT("DescText")), FText::FromString(Star.Description));
	NocturneUI::SetShown(Node(TEXT("EnterCap")), bOpen);
	NocturneUI::SetShown(Node(TEXT("EnterKey")), bOpen);
	NocturneUI::SetShown(Node(TEXT("EnterText")), bOpen);
	NocturneUI::SetShown(Node(TEXT("LockedText")), !bOpen);

	// The plate hugs this star's text: the name row, and the description row when it shows.
	const float NameRight = FMath::Max3(128.0f + TextWidth(Star.Glyph, 42.0f, 0.0f), 130.0f + TextWidth(Star.Name, 16.0f, 0.0f),
		214.0f + TextWidth(Star.En, 12.0f, 3.0f));
	const float StateWidth = bOpen ? 68.0f + TextWidth(TEXT("进入"), 14.0f, 0.0f) : TextWidth(TEXT("未开放  ·  LOCKED"), 14.0f, 0.0f);
	const float DetailsRight = 130.0f + FMath::Max(TextWidth(Star.Description, 14.0f, 0.0f), StateWidth);
	PlateWidth = NameRight - PlateLeft + PlateMargin;
	PlateFocusedWidth = FMath::Max(NameRight, DetailsRight) - PlateLeft + PlateMargin;
	ApplyFocusAt(0.0f);
	Paint();
}

void UNocturneStarLabel::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	LabelTime += DeltaTime;
	if (UDreamWidget* Arc = Node(TEXT("Arc")))
	{
		Arc->SetRenderTransformAngle(LabelTime * NocturneStarLabelLocal::ArcSpeed);
	}
	if (UDreamWidget* Hex = Node(TEXT("Hex")))
	{
		Hex->SetRenderTransformAngle(-LabelTime * NocturneStarLabelLocal::ArcSpeed * 0.4f);
	}
	if (LabelTime - FlashTime < NocturneStarLabelLocal::FlashSeconds + 0.05f)
	{
		Paint();
	}
}

void UNocturneStarLabel::SetFocused(bool bInFocused)
{
	if (bFocused == bInFocused)
	{
		return;
	}
	bFocused = bInFocused;
	ApplyFocusAt(bFocused ? 0.22f : 0.14f);
	Paint();
}

void UNocturneStarLabel::ApplyFocusAt(float InSeconds)
{
	using namespace NocturneStarLabelLocal;
	UDreamWidget* Details = Node(TEXT("Details"));
	UDreamWidget* Halo = Node(TEXT("Halo"));
	UDreamWidget* Arc = Node(TEXT("Arc"));
	UDreamWidget* Plate = Node(TEXT("Plate"));
	UDreamWidget* Hex = Node(TEXT("Hex"));
	for (UDreamWidget* Part : { Details, Halo, Arc, Plate, Hex })
	{
		Settle(Part);
	}
	const FVector2D PlateSize(bFocused ? PlateFocusedWidth : PlateWidth, PlateHeight + (bFocused ? PlateDetailsHeight : 0.0f));
	const FVector HaloScale(1.0, bFocused ? 1.22 : 1.0, bFocused ? 1.22 : 1.0);
	if (InSeconds <= 0.0f)
	{
		// At once: the label's state as it is, with nothing in flight.
		if (Details != nullptr)
		{
			Details->SetRenderOpacity(bFocused ? 1.0f : 0.0f);
			Details->SetRenderTranslation(FVector::ZeroVector);
		}
		if (Halo != nullptr)
		{
			Halo->SetRenderScale(HaloScale);
		}
		if (Arc != nullptr)
		{
			Arc->SetRenderOpacity(bFocused ? 1.0f : 0.0f);
		}
		if (Hex != nullptr)
		{
			Hex->SetRenderOpacity(bFocused ? 1.0f : 0.0f);
			Hex->SetRenderScale(FVector(1.0, 1.0, 1.0));
		}
		if (Plate != nullptr)
		{
			Plate->SetSizeDelta(PlateSize);
		}
		return;
	}
	if (Details != nullptr)
	{
		Details->RenderOpacityTo(bFocused ? 1.0f : 0.0f, InSeconds, 0.0f, EDreamTweenEase::OutCubic);
		// The description comes in from a little to the left, and goes without moving.
		Details->SetRenderTranslation(FVector(0.0, bFocused ? -14.0 : 0.0, 0.0));
		Details->RenderOffsetTo(FVector2D::ZeroVector, InSeconds, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (Halo != nullptr)
	{
		Halo->RenderScaleTo(HaloScale, InSeconds, 0.0f, EDreamTweenEase::OutBack);
	}
	if (Arc != nullptr)
	{
		Arc->RenderOpacityTo(bFocused ? 1.0f : 0.0f, InSeconds, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (Plate != nullptr)
	{
		Plate->SizeDeltaTo(PlateSize, InSeconds, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (Hex != nullptr)
	{
		// In from a little larger, as if it locked on.
		Hex->SetRenderScale(FVector(1.0, bFocused ? 1.35 : 1.0, bFocused ? 1.35 : 1.0));
		Hex->RenderScaleTo(FVector(1.0, 1.0, 1.0), InSeconds + 0.08f, 0.0f, EDreamTweenEase::OutCubic);
		Hex->RenderOpacityTo(bFocused ? 1.0f : 0.0f, InSeconds, 0.0f, EDreamTweenEase::OutCubic);
	}
}

void UNocturneStarLabel::PlayIntro(float InDelay)
{
	using namespace NocturneStarLabelLocal;
	// From the label's settled state, whatever was in flight.
	ApplyFocusAt(0.0f);
	if (UDreamWidget* Halo = Node(TEXT("Halo")))
	{
		Settle(Halo);
		Halo->SetRenderScale(FVector(1.0, 0.0, 0.0));
		Halo->RenderScaleTo(FVector(1.0, bFocused ? 1.22 : 1.0, bFocused ? 1.22 : 1.0), 0.32f, InDelay, EDreamTweenEase::OutBack);
	}
	int32 Step = 0;
	for (const TCHAR* Name : { TEXT("Plate"), TEXT("Leader"), TEXT("Number"), TEXT("GlyphText"), TEXT("NameText"), TEXT("EnText"), TEXT("Details") })
	{
		UDreamWidget* Part = Node(Name);
		if (Part == nullptr)
		{
			continue;
		}
		const float Delay = InDelay + 0.12f + 0.04f * Step++;
		// The description only if the star is in focus; the plate first, the text over it.
		const float Target = Part == Node(TEXT("Details")) && !bFocused ? 0.0f : 1.0f;
		Settle(Part);
		Part->SetRenderOpacity(0.0f);
		Part->SetRenderTranslation(FVector(0.0, -12.0, 0.0));
		Part->RenderOpacityTo(Target, 0.24f, Delay, EDreamTweenEase::OutCubic);
		Part->RenderOffsetTo(FVector2D::ZeroVector, 0.24f, Delay, EDreamTweenEase::OutCubic);
	}
	if (UDreamWidget* Arc = Node(TEXT("Arc")))
	{
		Settle(Arc);
		Arc->SetRenderOpacity(0.0f);
		if (bFocused)
		{
			Arc->RenderOpacityTo(1.0f, 0.24f, InDelay + 0.3f, EDreamTweenEase::OutCubic);
		}
	}
}

void UNocturneStarLabel::FlashLocked()
{
	FlashTime = LabelTime;
	if (UDreamWidget* Root = GetContentRoot())
	{
		// A knock sideways that springs back.
		NocturneStarLabelLocal::Settle(Root);
		Root->SetRenderTranslation(FVector(0.0, 10.0, 0.0));
		Root->RenderOffsetTo(FVector2D::ZeroVector, 0.35f, 0.0f, EDreamTweenEase::OutElastic);
	}
	Paint();
}

void UNocturneStarLabel::HandleState(EUISelectableSelectionState InState, bool bInImmediate)
{
	if (InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed)
	{
		OnHovered.Broadcast(this);
	}
}

void UNocturneStarLabel::HandleClick()
{
	OnClicked.Broadcast(this);
}

void UNocturneStarLabel::Paint()
{
	using namespace NocturneUI;
	const bool bFlash = LabelTime - FlashTime < NocturneStarLabelLocal::FlashSeconds;
	const FColor Accent = bFlash ? Amber : Cyan;
	const FColor Faint(0x8F, 0xE3, 0xFF, 0x88);
	SetTint(Node(TEXT("Halo")), bFocused || bFlash ? Accent : Faint);
	SetTint(Node(TEXT("Arc")), Accent);
	SetTint(Node(TEXT("Leader")), bFocused ? Accent : Faint);
	SetTint(Node(TEXT("Plate")), bFocused ? NocturneStarLabelLocal::PlateLit : NocturneStarLabelLocal::PlateIdle);
	SetTint(Node(TEXT("PlateTab")), bFocused || bFlash ? Accent : FColor(0x8F, 0xE3, 0xFF, 0x50));
	SetTextColor(Node(TEXT("GlyphText")), bFocused ? InkBright : Ink);
	SetTextColor(Node(TEXT("NameText")), bFocused ? Ink : InkDim);
	SetTextColor(Node(TEXT("EnText")), bFocused ? CyanTag : InkFaint);
	SetTextColor(Node(TEXT("Number")), bFocused || bFlash ? Accent : Faint);
}
