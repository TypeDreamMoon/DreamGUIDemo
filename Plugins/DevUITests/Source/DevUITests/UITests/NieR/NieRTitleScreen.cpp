#include "NieRTitleScreen.h"

#include "Core/Components/DreamWidget.h"
#include "Event/DreamKeyEventData.h"
#include "Interaction/UIButton.h"
#include "Kismet/KismetSystemLibrary.h"

void UNieRTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (UDreamWidget* Glitch = Node(TEXT("Glitch")))
	{
		for (UDreamWidget* Block : Glitch->GetChildren())
		{
			Glitches.Add(Block);
		}
	}
	if (UDreamWidget* PressHit = Node(TEXT("PressHit")))
	{
		if (UUIButton* Button = PressHit->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(NieRUI::UseFocusRing());
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { ShowMenu(); });
		}
	}
	ShowPressKey();
}

void UNieRTitleScreen::NativeOnEnable()
{
	Super::NativeOnEnable();
	if (!bInMenu)
	{
		FocusWidget(Node(TEXT("PressHit")));
	}
}

UNieRRow* UNieRTitleScreen::GetFirstMenuRow() const
{
	// The menu is a `rows` table, so its rows are found by place, not by a name.
	const TArray<UNieRRow*> MenuRows = CollectRows(Node(TEXT("Menu")));
	return MenuRows.Num() > 0 ? MenuRows[0] : nullptr;
}

UNieRRow* UNieRTitleScreen::GetEntryRow() const
{
	return bInMenu ? GetFirstMenuRow() : nullptr;
}

void UNieRTitleScreen::PlayEntrance()
{
	// The first time only: coming back from a screen above, the title is already there.
	if (bEntrancePlayed)
	{
		Super::PlayEntrance();
		return;
	}
	bEntrancePlayed = true;
	Motion.FadeIn(Node(TEXT("Logo")), 1.6f);
	Motion.SlideIn(Node(TEXT("LogoRule")), FVector2D(-120.0, 0.0), 1.2f, 0.4f);
	Motion.FadeIn(Node(TEXT("PressKey")), 0.8f, 0.0f, 1.0f);
}

void UNieRTitleScreen::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	const float Now = GetScreenTime();
	// "Press Any Key" breathes once its fade-in is over.
	if (!bInMenu && Now > 1.8f)
	{
		if (UDreamWidget* Prompt = Node(TEXT("PressText")))
		{
			Prompt->SetRenderOpacity(0.35f + 0.65f * (0.5f + 0.5f * FMath::Cos((Now - 1.8f) * UE_TWO_PI / 2.2f)));
		}
	}
	// The block glitch: a few blocks blink and shift every few frames.
	if (Now >= NextGlitch)
	{
		NextGlitch = Now + Random.FRandRange(0.05f, 0.12f);
		for (const TWeakObjectPtr<UDreamWidget>& Weak : Glitches)
		{
			UDreamWidget* Block = Weak.Get();
			if (!IsValid(Block) || Random.FRand() > 0.35f)
			{
				continue;
			}
			FNieRMotion::SetOffset(Block, FVector2D(Random.FRandRange(-10.0f, 10.0f), 0.0));
			Block->SetRenderOpacity(Random.FRand() < 0.3f ? 0.0f : Random.FRandRange(0.5f, 1.0f));
		}
	}
	// Now and then the whole title jumps sideways for a moment.
	if (Now > 2.0f)
	{
		UDreamWidget* Logo = Node(TEXT("Logo"));
		if (Now >= NextJump)
		{
			NextJump = Now + Random.FRandRange(2.0f, 4.0f);
			JumpEnds = Now + 0.08f;
			FNieRMotion::SetOffset(Logo, FVector2D(Random.FRandRange(-6.0f, 6.0f), 0.0));
		}
		else if (JumpEnds > 0.0f && Now >= JumpEnds)
		{
			JumpEnds = -1.0f;
			FNieRMotion::SetOffset(Logo, FVector2D::ZeroVector);
		}
	}
}

bool UNieRTitleScreen::HandleBack()
{
	if (bInMenu)
	{
		ShowPressKey();
	}
	return true;
}

bool UNieRTitleScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (!bInMenu && EventData != nullptr && !EventData->Key.IsModifierKey() && !EventData->Key.IsMouseButton()
		&& EventData->Key != EKeys::Escape && EventData->Key != EKeys::BackSpace)
	{
		ShowMenu();
		return true;
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNieRTitleScreen::OnRowClicked(UNieRRow* Row)
{
	if (Row->Action == TEXT("menu"))
	{
		PushScreen(TEXT("NieR_Menu"), NieRUI::MenuClass);
	}
	else if (Row->Action == TEXT("settings"))
	{
		PushScreen(TEXT("NieR_Settings"), NieRUI::SettingsClass);
	}
	else if (Row->Action == TEXT("exit"))
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}

void UNieRTitleScreen::ShowPressKey()
{
	bInMenu = false;
	SetShown(Node(TEXT("Menu")), false);
	SetShown(Node(TEXT("MenuPrompts")), false);
	SetShown(Node(TEXT("PressKey")), true);
	SetShown(Node(TEXT("PressHit")), true);
	FocusWidget(Node(TEXT("PressHit")));
}

void UNieRTitleScreen::ShowMenu()
{
	bInMenu = true;
	SetShown(Node(TEXT("PressKey")), false);
	SetShown(Node(TEXT("Menu")), true);
	SetShown(Node(TEXT("MenuPrompts")), true);
	FocusRow(GetFirstMenuRow());
	SetShown(Node(TEXT("PressHit")), false);
	// The rows rise in one after another.
	SlideRowsIn(CollectRows(Node(TEXT("Menu"))), FVector2D(0.0, 26.0), 0.28f, 0.045f);
	Motion.FadeIn(Node(TEXT("MenuPrompts")), 0.4f, 0.0f, 0.25f);
}
