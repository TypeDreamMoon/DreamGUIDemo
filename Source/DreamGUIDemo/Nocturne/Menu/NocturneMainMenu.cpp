// Copyright DreamMoon. All Rights Reserved.

#include "NocturneMainMenu.h"

#include "../NocturneStars.h"
#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "Event/DreamKeyEventData.h"
#include "DreamTweenManager.h"
#include "Interaction/UIButton.h"

namespace NocturneMainMenuLocal
{
	constexpr float FlashSeconds = 0.6f;
	constexpr float ExitSeconds = 0.22f;
	const TCHAR* Parts[] = { TEXT("TopBar"), TEXT("RouteList"), TEXT("KeyBar") };
	/** The route list curves round the eye, turned on its outer edge (as in the .dui), and swings in from edge-on. */
	constexpr double ListYaw = -14.0;
	constexpr double ListSwing = -62.0;

	/** A render offset in design units, +y down; the depth the part has is kept. */
	void SetOffset(UDreamWidget* Widget, FVector2D Offset)
	{
		if (IsValid(Widget))
		{
			Widget->SetRenderTranslation(FVector(Widget->GetRenderTranslation().X, Offset.X, -Offset.Y));
		}
	}

	void RotateTo(UDreamWidget* Widget, const FRotator& To, float Seconds, float Delay, EDreamTweenEase Ease)
	{
		if (!IsValid(Widget))
		{
			return;
		}
		if (UDreamTweener* Tweener = UDreamTweenManager::To(Widget,
			FDreamTweenRotatorGetterFunction::CreateWeakLambda(Widget, [Widget]() { return Widget->GetRenderRotation(); }),
			FDreamTweenRotatorSetterFunction::CreateWeakLambda(Widget, [Widget](FRotator Value) { Widget->SetRenderRotation(Value); }),
			To, Seconds))
		{
			Tweener->SetEase(Ease)->SetDelay(Delay);
		}
	}

	/** Stop what is running on a part before it is sent somewhere else: two fades on one part fight to the last frame. */
	void Settle(UDreamWidget* Widget)
	{
		if (IsValid(Widget))
		{
			UDreamTweenManager::KillAllTweensOnTarget(Widget, Widget);
		}
	}

	/** In from Offset, fading up, flipping up from lying back a little behind (Flip scales the flip; parts rest flat). */
	void RunIn(UDreamWidget* Widget, FVector2D Offset, float Delay, float Seconds, float Flip = 1.0f)
	{
		if (!IsValid(Widget))
		{
			return;
		}
		Settle(Widget);
		Widget->SetRenderTranslation(FVector(60.0 * Flip, Offset.X, -Offset.Y));
		Widget->SetRenderRotation(FRotator(-40.0 * Flip, 0.0, 0.0));
		Widget->SetRenderOpacity(0.0f);
		Widget->RenderTranslationTo(FVector::ZeroVector, Seconds, Delay, EDreamTweenEase::OutCubic);
		RotateTo(Widget, FRotator::ZeroRotator, Seconds + 0.06f, Delay, EDreamTweenEase::OutCubic);
		Widget->RenderOpacityTo(1.0f, FMath::Min(Seconds, 0.2f), Delay, EDreamTweenEase::OutCubic);
	}
}

UDreamWidget* UNocturneMainMenu::Node(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

void UNocturneMainMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace NocturneUI;

	for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
	{
		const FNocturneStarDef& Star = NocturneStars::Stars[Index];
		SetText(Node(*FString::Printf(TEXT("Nav%dNum"), Index)), FText::FromString(FString::Printf(TEXT("%02d"), Index + 1)));
		SetText(Node(*FString::Printf(TEXT("Nav%dGlyph"), Index)), FText::FromString(Star.Glyph));
		SetText(Node(*FString::Printf(TEXT("Nav%dName"), Index)), FText::FromString(Star.Name));
		SetText(Node(*FString::Printf(TEXT("Nav%dState"), Index)), FText::FromString(Star.bOpen ? TEXT("OPEN") : TEXT("LOCKED")));

		UDreamWidget* Entry = Node(*FString::Printf(TEXT("Nav%d"), Index));
		UUIButton* Button = Entry != nullptr ? Entry->GetComponent<UUIButton>() : nullptr;
		if (Button == nullptr)
		{
			continue;
		}
		// The entry's own lit look is the focus mark; DreamGUI's ring would draw over the plate's hairline.
		Button->SetUseFocusRing(false);
		Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Index](EUISelectableSelectionState InState, bool)
		{
			HandleState(Index, InState);
		});
		Button->GetOnClickEvent().AddWeakLambda(this, [this, Index]() { OnEntryChosen.Broadcast(Index); });
	}
	SetWantsTick(true);
	Paint();
}

void UNocturneMainMenu::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	MenuTime += DeltaTime;
	// Live, not made up: the time of night from the menu's clock, and the frame step the game is running at. (Not the
	// GPU's time: in an off-screen recording that counts the frame dump too.)
	const int32 Second = FMath::FloorToInt(MenuTime);
	if (Second != ShownSecond)
	{
		ShownSecond = Second;
		const int32 Clock = 23 * 3600 + 41 * 60 + Second;
		NocturneUI::SetText(Node(TEXT("Status")), FText::FromString(FString::Printf(TEXT("ONLINE    FRAME %.1f MS    %02d:%02d:%02d"),
			DeltaTime * 1000.0f, Clock / 3600 % 24, Clock / 60 % 60, Clock % 60)));
	}
	if (HideAt >= 0.0f && MenuTime >= HideAt)
	{
		HideAt = -1.0f;
		SetPartsShown(false);
	}
	if (MenuTime - FlashTime < NocturneMainMenuLocal::FlashSeconds + 0.05f)
	{
		Paint();
	}
}

bool UNocturneMainMenu::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData == nullptr || Focused == INDEX_NONE)
	{
		return Super::NativeOnKeyDown(EventData);
	}
	const FKey& Key = EventData->Key;
	int32 Delta = 0;
	if (Key == EKeys::W || Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		Delta = -1;
	}
	else if (Key == EKeys::S || Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		Delta = 1;
	}
	if (Delta != 0)
	{
		FocusEntry((Focused + Delta + NocturneStars::Count) % NocturneStars::Count);
		return true;
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		OnEntryChosen.Broadcast(Focused);
		return true;
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNocturneMainMenu::HandleState(int32 InIndex, EUISelectableSelectionState InState)
{
	const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
	if (bOver)
	{
		Hovered = InIndex;
	}
	else if (Hovered == InIndex)
	{
		Hovered = INDEX_NONE;
	}
	Paint();
	if (bOver || InState == EUISelectableSelectionState::Focused)
	{
		OnEntryFocused.Broadcast(InIndex);
	}
}

void UNocturneMainMenu::SetFocused(int32 InIndex)
{
	if (Focused == InIndex)
	{
		return;
	}
	const int32 Previous = Focused;
	Focused = InIndex;
	// The newly lit entry's text steps in a little from the left.
	if (UDreamWidget* Glyph = Node(*FString::Printf(TEXT("Nav%dGlyph"), Focused)))
	{
		NocturneMainMenuLocal::Settle(Glyph);
		NocturneMainMenuLocal::SetOffset(Glyph, FVector2D(-8.0, 0.0));
		Glyph->RenderOffsetTo(FVector2D::ZeroVector, 0.18f, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (UDreamWidget* Name = Node(*FString::Printf(TEXT("Nav%dName"), Focused)))
	{
		NocturneMainMenuLocal::Settle(Name);
		NocturneMainMenuLocal::SetOffset(Name, FVector2D(-8.0, 0.0));
		Name->RenderOffsetTo(FVector2D::ZeroVector, 0.2f, 0.02f, EDreamTweenEase::OutCubic);
	}
	if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Nav%dBar"), Focused)))
	{
		// The bar slides over from the entry it was on (48 apart), or draws on the first time.
		NocturneMainMenuLocal::Settle(Bar);
		if (Previous != INDEX_NONE)
		{
			NocturneMainMenuLocal::SetOffset(Bar, FVector2D(0.0, 48.0 * (Previous - Focused)));
			Bar->RenderOffsetTo(FVector2D::ZeroVector, 0.2f, 0.0f, EDreamTweenEase::OutCubic);
		}
		else
		{
			Bar->SetRenderScale(FVector(1.0, 1.0, 0.0));
			Bar->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.16f, 0.0f, EDreamTweenEase::OutCubic);
		}
	}
	Paint();
}

void UNocturneMainMenu::FocusEntry(int32 InIndex)
{
	if (UDreamWidget* Entry = Node(*FString::Printf(TEXT("Nav%d"), InIndex)))
	{
		if (!Entry->HasFocus())
		{
			Entry->SetKeyboardFocus();
		}
	}
}

void UNocturneMainMenu::FlashLocked(int32 InIndex)
{
	FlashIndex = InIndex;
	FlashTime = MenuTime;
	if (UDreamWidget* Entry = Node(*FString::Printf(TEXT("Nav%dState"), InIndex)))
	{
		NocturneMainMenuLocal::Settle(Entry);
		NocturneMainMenuLocal::SetOffset(Entry, FVector2D(8.0, 0.0));
		Entry->RenderOffsetTo(FVector2D::ZeroVector, 0.35f, 0.0f, EDreamTweenEase::OutElastic);
	}
	Paint();
}

void UNocturneMainMenu::Paint()
{
	using namespace NocturneUI;
	const bool bFlashing = MenuTime - FlashTime < NocturneMainMenuLocal::FlashSeconds;
	for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
	{
		const bool bOpen = NocturneStars::Stars[Index].bOpen;
		const bool bOn = Index == Focused;
		const bool bOver = Index == Hovered && !bOn;
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Nav%dBar"), Index)), bOn);
		SetTint(Node(*FString::Printf(TEXT("Nav%dBack"), Index)), bOn ? RowLit : (bOver ? RowHover : Clear));
		SetTextColor(Node(*FString::Printf(TEXT("Nav%dNum"), Index)), bOn ? Cyan : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Nav%dGlyph"), Index)), bOn ? InkBright : (bOpen ? Ink : InkDim));
		SetTextColor(Node(*FString::Printf(TEXT("Nav%dName"), Index)), bOn ? Ink : InkDim);
		FColor State = bOpen ? CyanTag : InkFaint;
		if (!bOpen && (bOn || (bFlashing && Index == FlashIndex)))
		{
			State = bFlashing && Index == FlashIndex ? Amber : FColor(0xFF, 0xB4, 0x54, 0xA0);
		}
		SetTextColor(Node(*FString::Printf(TEXT("Nav%dState"), Index)), State);
	}
}

void UNocturneMainMenu::SetPartsShown(bool bInShown)
{
	for (const TCHAR* Part : NocturneMainMenuLocal::Parts)
	{
		NocturneUI::SetShown(Node(Part), bInShown);
	}
}

void UNocturneMainMenu::PlayEntrance(float InDelay)
{
	using namespace NocturneMainMenuLocal;
	HideAt = -1.0f;
	SetPartsShown(true);
	RunIn(Node(TEXT("TopBar")), FVector2D(0.0, -28.0), InDelay, 0.35f, 0.6f);
	RunIn(Node(TEXT("KeyBar")), FVector2D(0.0, 28.0), InDelay + 0.08f, 0.35f, 0.0f);
	if (UDreamWidget* List = Node(TEXT("RouteList")))
	{
		// The list swings round from edge-on to its curve and stays opaque; what is in it flips up inside it, plate
		// first, then entry by entry.
		Settle(List);
		List->SetRenderOpacity(1.0f);
		List->SetRenderTranslation(FVector::ZeroVector);
		List->SetRenderRotation(FRotator(0.0, ListYaw + ListSwing, 0.0));
		RotateTo(List, FRotator(0.0, ListYaw, 0.0), 0.6f, InDelay, EDreamTweenEase::OutCubic);
		int32 Step = 0;
		for (UDreamWidget* Child : List->GetChildren())
		{
			RunIn(Child, FVector2D(-24.0, 0.0), InDelay + 0.12f + 0.045f * Step++, 0.3f);
		}
	}
}

void UNocturneMainMenu::HideNow()
{
	HideAt = -1.0f;
	SetPartsShown(false);
}

void UNocturneMainMenu::PlayExit()
{
	for (const TCHAR* Part : NocturneMainMenuLocal::Parts)
	{
		if (UDreamWidget* Widget = Node(Part))
		{
			NocturneMainMenuLocal::Settle(Widget);
			Widget->RenderOpacityTo(0.0f, NocturneMainMenuLocal::ExitSeconds, 0.0f, EDreamTweenEase::InCubic);
		}
	}
	// The list swings shut toward edge-on as it fades.
	NocturneMainMenuLocal::RotateTo(Node(TEXT("RouteList")), FRotator(0.0, NocturneMainMenuLocal::ListYaw + NocturneMainMenuLocal::ListSwing * 0.8, 0.0),
		NocturneMainMenuLocal::ExitSeconds, 0.0f, EDreamTweenEase::InCubic);
	HideAt = MenuTime + NocturneMainMenuLocal::ExitSeconds + 0.02f;
}
