// Copyright DreamMoon. All Rights Reserved.

#include "NocturneDepartScreen.h"

#include "NocturneLens.h"
#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Event/DreamKeyEventData.h"
#include "Interaction/UIButton.h"

namespace NocturneDepartLocal
{
	/** How far a hovered card lifts, design units. */
	constexpr double Lift = 14.0;
}

void UNocturneDepartScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		UDreamWidget* Card = Node(*FString::Printf(TEXT("Card%d"), Index));
		UUIButton* Button = Card != nullptr ? Card->GetComponent<UUIButton>() : nullptr;
		if (Button == nullptr)
		{
			continue;
		}
		Button->SetUseFocusRing(false);
		Button->GetOnClickEvent().AddWeakLambda(this, [this, Index]() { Pick(Index); });
		Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Index](EUISelectableSelectionState InState, bool)
		{
			const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed
				|| InState == EUISelectableSelectionState::Focused;
			if (bOver && Hovered != Index)
			{
				SetLifted(Hovered, false);
				Hovered = Index;
				SetLifted(Index, true);
			}
			else if (!bOver && Hovered == Index)
			{
				SetLifted(Index, false);
				Hovered = INDEX_NONE;
			}
			Paint();
		});
	}
	if (UDreamWidget* LaunchButton = Node(TEXT("Launch")))
	{
		if (UUIButton* Button = LaunchButton->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(false);
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { Launch(); });
			Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this](EUISelectableSelectionState InState, bool)
			{
				bLaunchHovered = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
				Paint();
			});
		}
	}
	Pick(0);
}

void UNocturneDepartScreen::SetLifted(int32 InCard, bool bInLifted)
{
	// The lift is on the card's inner layer, so it never fights the card's own swing in. It comes a little nearer too.
	if (UDreamWidget* Tilt = Node(*FString::Printf(TEXT("Card%dTilt"), InCard)))
	{
		Tilt->RenderTranslationTo(FVector(bInLifted ? -36.0 : 0.0, 0.0, bInLifted ? NocturneDepartLocal::Lift : 0.0), 0.2f, 0.0f, EDreamTweenEase::OutCubic);
	}
}

void UNocturneDepartScreen::Pick(int32 InCard)
{
	if (bLaunched)
	{
		return;
	}
	const bool bChanged = InCard != Picked;
	Picked = FMath::Clamp(InCard, 0, CardCount - 1);
	NocturneUI::SetText(Node(TEXT("LaunchSlot")), FText::FromString(FString::Printf(TEXT("SLOT %02d"), Picked + 1)));
	if (bChanged)
	{
		if (UDreamWidget* Tab = Node(*FString::Printf(TEXT("Card%dTab"), Picked)))
		{
			Settle(Tab);
			Tab->SetRenderScale(FVector(1.0, 0.0, 1.0));
			Tab->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.25f, 0.0f, EDreamTweenEase::OutCubic);
		}
		BurstSort(CenterSortIndex, 0.3f, 0.22f);
	}
	Paint();
}

void UNocturneDepartScreen::Launch()
{
	if (bLaunched)
	{
		return;
	}
	bLaunched = true;
	if (UDreamWidget* Text = Node(TEXT("LaunchText")))
	{
		Settle(Text);
		Text->SetRenderScale(FVector(1.0, 1.25, 1.25));
		Text->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.25f, 0.0f, EDreamTweenEase::OutCubic);
	}
	Paint();
	UNocturneLens::Pulse(ENocturneLensPulse::Dive, 1.0f);
	OnLaunch.Broadcast(Picked);
}

void UNocturneDepartScreen::PlayExit()
{
	if (!bLaunched)
	{
		Super::PlayExit();
		return;
	}
	// Launched: the chosen card rushes at the camera, turning to face it square, and the others fall away behind; the
	// screen is gone in under four tenths of a second, as the dive begins.
	FadeChrome();
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		UDreamWidget* Card = Node(*FString::Printf(TEXT("Card%d"), Index));
		if (Card == nullptr)
		{
			continue;
		}
		const FRestPose Rest = GetRest(Card);
		Settle(Card);
		if (Index == Picked)
		{
			RotateTo(Card, FRotator::ZeroRotator, 0.3f, 0.0f, EDreamTweenEase::OutCubic);
			Card->RenderTranslationTo(Rest.Translation + FVector(-720.0, 0.0, 0.0), 0.34f, 0.0f, EDreamTweenEase::InCubic);
			Card->RenderOpacityTo(0.0f, 0.14f, 0.2f, EDreamTweenEase::InQuad);
		}
		else
		{
			const double Side = Index < Picked ? 1.0 : -1.0;
			RotateTo(Card, Rest.Rotation + FRotator(0.0, 40.0 * Side, 0.0), 0.3f, 0.0f, EDreamTweenEase::InCubic);
			Card->RenderTranslationTo(Rest.Translation + FVector(360.0, -120.0 * Side, 0.0), 0.3f, 0.0f, EDreamTweenEase::InCubic);
			Card->RenderOpacityTo(0.0f, 0.18f, 0.06f, EDreamTweenEase::InQuad);
		}
	}
	for (const TCHAR* Part : { TEXT("Header"), TEXT("Launch") })
	{
		if (UDreamWidget* Widget = Node(Part))
		{
			Settle(Widget);
			Widget->RenderTranslationTo(GetRest(Widget).Translation + FVector(240.0, 0.0, 0.0), 0.26f, 0.0f, EDreamTweenEase::InCubic);
			Widget->RenderOpacityTo(0.0f, 0.16f, 0.04f, EDreamTweenEase::InQuad);
		}
	}
}

bool UNocturneDepartScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData != nullptr)
	{
		const FKey& Key = EventData->Key;
		if (Key == EKeys::A || Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left)
		{
			Pick((Picked + CardCount - 1) % CardCount);
			return true;
		}
		if (Key == EKeys::D || Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right)
		{
			Pick((Picked + 1) % CardCount);
			return true;
		}
		if (Key == EKeys::Enter || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			Launch();
			return true;
		}
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNocturneDepartScreen::Paint()
{
	using namespace NocturneUI;
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		const bool bOn = Index == Picked;
		const bool bOver = Index == Hovered;
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Card%dTab"), Index)), bOn);
		SetTint(Node(*FString::Printf(TEXT("Card%dBack"), Index)), bOn ? FColor(0x8F, 0xE3, 0xFF, 0x12) : (bOver ? FColor(0x8F, 0xE3, 0xFF, 0x0A) : Clear));
		SetTextColor(Node(*FString::Printf(TEXT("Card%dSlot"), Index)), bOn ? Cyan : InkDim);
		SetTextColor(Node(*FString::Printf(TEXT("Card%dTitle"), Index)), bOn ? InkBright : Ink);
		if (UDreamWidget* Frame = Node(*FString::Printf(TEXT("Card%dFrame"), Index)))
		{
			Frame->SetRenderOpacity(bOn ? 1.0f : 0.55f);
		}
	}
	SetTint(Node(TEXT("LaunchBack")), bLaunched ? FColor(0x8F, 0xE3, 0xFF, 0x50) : (bLaunchHovered ? FColor(0x8F, 0xE3, 0xFF, 0x2C) : FColor(0x8F, 0xE3, 0xFF, 0x14)));
	SetTextColor(Node(TEXT("LaunchText")), bLaunchHovered || bLaunched ? InkBright : Ink);
}

UDreamWidget* UNocturneDepartScreen::GetInitialFocus() const
{
	return Node(*FString::Printf(TEXT("Card%d"), Picked));
}

void UNocturneDepartScreen::PlayContentEntrance(float Delay)
{
	RunInChildren(Node(TEXT("Header")), Delay, 0.04f, FVector2D(-16.0, 0.0));
	// The cards swing in on their hinges, the side ones from behind, the middle one rising from lying back.
	const FRotator From[] = { FRotator(0.0, 60.0, 0.0), FRotator(-30.0, 0.0, 0.0), FRotator(0.0, -60.0, 0.0) };
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		if (UDreamWidget* Card = Node(*FString::Printf(TEXT("Card%d"), Index)))
		{
			const float At = Delay + 0.07f * Index;
			SwingIn(Card, At, 0.55f, From[FMath::Min(Index, 2)], Index == 1 ? 260.0 : 160.0);
			Card->SetRenderOpacity(0.0f);
			Card->RenderOpacityTo(1.0f, 0.2f, At, EDreamTweenEase::OutQuad);
		}
	}
	RunIn(Node(TEXT("Launch")), Delay + 0.3f, 0.3f, FVector2D(0.0, 16.0));
}
