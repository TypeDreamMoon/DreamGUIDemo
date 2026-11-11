// Copyright DreamMoon. All Rights Reserved.

#include "P3RScreen.h"

#include "P3RUI.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "Engine/World.h"
#include "Event/DreamKeyEventData.h"
#include "Extensions/DreamMaskTransition.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/UIButton.h"

UDreamWidget* UP3RScreen::Node(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

EP3RKey UP3RScreen::Translate(const FKey& Key, bool bFieldKeys)
{
	if (Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_LeftStick_Up)
	{
		return EP3RKey::Up;
	}
	if (Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down || Key == EKeys::Gamepad_LeftStick_Down)
	{
		return EP3RKey::Down;
	}
	if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_LeftStick_Left)
	{
		return EP3RKey::Left;
	}
	if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_LeftStick_Right)
	{
		return EP3RKey::Right;
	}
	if (bFieldKeys && (Key == EKeys::Tab || Key == EKeys::Escape || Key == EKeys::Enter || Key == EKeys::SpaceBar
		|| Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Gamepad_FaceButton_Bottom))
	{
		return EP3RKey::Open;
	}
	if (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
	{
		return EP3RKey::Confirm;
	}
	if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right)
	{
		return EP3RKey::Back;
	}
	if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
	{
		return EP3RKey::PrevTab;
	}
	if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
	{
		return EP3RKey::NextTab;
	}
	if (Key == EKeys::Tab || Key == EKeys::F1 || Key == EKeys::Gamepad_Special_Left)
	{
		return EP3RKey::Design;
	}
	return EP3RKey::None;
}

void UP3RScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	SetWantsTick(true);
	if (UDreamWidget* Found = Node(TEXT("Catch")))
	{
		Found->SetIsFocusable(true);
		Catch = Found;
	}
	// Hidden until the director brings it on (in a game only: the designer's preview shows the screen as written).
	if (const UWorld* World = GetWorld(); World != nullptr && World->IsGameWorld())
	{
		SetVisibility(EDreamWidgetVisibility::Collapsed);
	}
}

void UP3RScreen::Enter()
{
	LeaveAt = -1.0f;
	bActive = true;
	EnteredAt = ScreenTime;
	SetVisibility(EDreamWidgetVisibility::Visible);
	SetRenderOpacity(1.0f);
	PlayEntrance();
	KeepFocus();
}

void UP3RScreen::Leave()
{
	if (!bActive && LeaveAt >= 0.0f)
	{
		return;
	}
	bActive = false;
	LeaveAt = ScreenTime + PlayExit();
}

void UP3RScreen::PlayBlotIn(UDreamMaskTransition* Mask, float Seconds)
{
	if (Mask != nullptr)
	{
		// From hidden: the blot is re-aimed (preset, centre) before it starts to spread.
		Mask->Stop();
		Mask->SetProgress(0.0f);
		Mask->SetPreset(EDreamMaskTransitionPreset::Ink);
		Mask->SetReverse(false);
		if (bHasEntryPoint)
		{
			Mask->SetCentre(EntryPoint);
		}
		Mask->PlayFromTo(0.0f, 1.0f, Seconds, EDreamTweenEase::OutCubic);
	}
	// The close-up: larger about the point it opens from, under a blue veil, settling as the blot spreads.
	if (UDreamWidget* Root = GetContentRoot())
	{
		const FVector2D From = bHasEntryPoint ? EntryPoint : FVector2D(0.5, 0.5);
		Root->SetRenderTransformPivot(FVector2D(From.X, 1.0 - From.Y));
		P3RUI::SetScale2D(Root, FVector2D(1.14));
		P3RUI::Scale2DTo(Root, FVector2D(1.0), Seconds + 0.08f, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (UDreamWidget* Veil = Node(TEXT("Closeup")))
	{
		P3RUI::Settle(Veil);
		P3RUI::SetShown(Veil, true);
		Veil->SetRenderOpacity(0.85f);
		P3RUI::FadeTo(Veil, 0.0f, Seconds, 0.05f, EDreamTweenEase::InOutSine);
	}
	bHasEntryPoint = false;
}

float UP3RScreen::PlayBlotOut(UDreamMaskTransition* Mask)
{
	if (Mask == nullptr)
	{
		return 0.0f;
	}
	if (bExitToMenu)
	{
		// An iris run backwards closes in from the edge; played back from shown it opens a hole from its centre, which
		// grows until nothing is left.
		bExitToMenu = false;
		Mask->Stop();
		Mask->SetPreset(EDreamMaskTransitionPreset::Iris);
		Mask->SetReverse(true);
		Mask->SetCentre(FVector2D(0.88, 0.1));
		Mask->PlayBackward(0.24f, EDreamTweenEase::InQuad);
		return 0.26f;
	}
	Mask->PlayBackward(0.24f, EDreamTweenEase::InCubic);
	return 0.26f;
}

void UP3RScreen::Hide()
{
	bActive = false;
	LeaveAt = -1.0f;
	SetVisibility(EDreamWidgetVisibility::Collapsed);
}

void UP3RScreen::KeepFocus()
{
	if (UDreamWidget* Target = Catch.Get(); Target != nullptr && !Target->HasFocus())
	{
		// Only a widget that has been drawn takes the focus, so this is asked again every frame until it holds.
		Target->SetKeyboardFocus();
	}
}

void UP3RScreen::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	ScreenTime += DeltaTime;

	if (const APlayerController* Player = GetOwningPlayer())
	{
		float X = 0.0f, Y = 0.0f;
		if (Player->GetMousePosition(X, Y))
		{
			const FVector2D Pointer(X, Y);
			if (!Pointer.Equals(LastPointer, 0.5))
			{
				PointerMovedAt = ScreenTime;
				LastPointer = Pointer;
			}
		}
	}

	if (bActive)
	{
		KeepFocus();
	}
	else if (LeaveAt >= 0.0f && ScreenTime >= LeaveAt)
	{
		LeaveAt = -1.0f;
		SetVisibility(EDreamWidgetVisibility::Collapsed);
	}
	if (GetVisibility() != EDreamWidgetVisibility::Collapsed)
	{
		TickScreen(DeltaTime);
	}
}

bool UP3RScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData != nullptr && bActive)
	{
		const EP3RKey Key = Translate(EventData->Key, UsesFieldKeys());
		if (Key != EP3RKey::None)
		{
			// Swallowed while locked too, so the default navigation does not wander off with it.
			if (!IsInputLocked())
			{
				HandleKey(Key);
			}
			return true;
		}
	}
	return Super::NativeOnKeyDown(EventData);
}

bool UP3RScreen::PointerMovedRecently() const
{
	return ScreenTime - PointerMovedAt < 0.25f;
}

void UP3RScreen::WireButton(UDreamWidget* Widget, TFunction<void()> OnHover, TFunction<void()> OnClick)
{
	UUIButton* Button = IsValid(Widget) ? Widget->GetComponent<UUIButton>() : nullptr;
	if (Button == nullptr)
	{
		return;
	}
	Button->SetUseFocusRing(false);
	if (OnClick)
	{
		Button->GetOnClickEvent().AddWeakLambda(this, [this, OnClick]()
		{
			if (!IsInputLocked())
			{
				OnClick();
			}
		});
	}
	if (OnHover)
	{
		Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, OnHover](EUISelectableSelectionState InState, bool)
		{
			if (InState == EUISelectableSelectionState::Hovered && !IsInputLocked() && PointerMovedRecently())
			{
				OnHover();
			}
		});
	}
}
