#include "NieRScreen.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamScreenUISubsystem.h"
#include "Engine/World.h"
#include "Event/DreamKeyEventData.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

namespace NieRScreenLocal
{
	bool IsHighlightState(EUISelectableSelectionState State)
	{
		return State == EUISelectableSelectionState::Focused || State == EUISelectableSelectionState::Pressed;
	}

	template<typename T>
	T* FindFirstUnder(UDreamWidget* InRoot)
	{
		if (!IsValid(InRoot))
		{
			return nullptr;
		}
		if (T* Found = Cast<T>(InRoot))
		{
			return Found;
		}
		for (UDreamWidget* Child : InRoot->GetChildren())
		{
			if (T* Found = FindFirstUnder<T>(Child))
			{
				return Found;
			}
		}
		return nullptr;
	}

	void CollectRowsUnder(UDreamWidget* InWidget, TArray<UNieRRow*>& OutRows)
	{
		if (!IsValid(InWidget))
		{
			return;
		}
		if (UNieRRow* Row = Cast<UNieRRow>(InWidget))
		{
			// A collapsed row is not one: it is the template a `for` repeats, kept in the tree, folded away.
			if (Row->GetVisibility() != EDreamWidgetVisibility::Collapsed)
			{
				OutRows.Add(Row);
			}
			return;
		}
		for (UDreamWidget* Child : InWidget->GetChildren())
		{
			CollectRowsUnder(Child, OutRows);
		}
	}
}

bool UNieRScreenScope::HandleBackAction_Implementation()
{
	for (UDreamWidget* Walker = GetWidget(); IsValid(Walker); Walker = Walker->GetParent())
	{
		if (UNieRScreen* Screen = Cast<UNieRScreen>(Walker))
		{
			return Screen->HandleBack();
		}
	}
	return Super::HandleBackAction_Implementation();
}

bool UNieRScreen::HandleBack()
{
	PopThisScreen();
	return true;
}

TArray<UNieRRow*> UNieRScreen::CollectRows(UDreamWidget* InRoot)
{
	TArray<UNieRRow*> Found;
	NieRScreenLocal::CollectRowsUnder(InRoot, Found);
	return Found;
}

void UNieRScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	HintBar = NieRScreenLocal::FindFirstUnder<UNieRHintBar>(GetContentRoot());
	SetWantsTick(true);
}

void UNieRScreen::WireRows()
{
	// Not in NativeOnInitialized: the rows a `for` makes are made after it, when the bindings resolve.
	for (UNieRRow* Row : CollectRows(GetContentRoot()))
	{
		if (!Rows.Contains(Row))
		{
			Rows.Add(Row);
			Row->OnStateChanged.AddUObject(this, &UNieRScreen::HandleRowState);
			Row->OnClicked.AddUObject(this, &UNieRScreen::HandleRowClicked);
		}
	}
}

void UNieRScreen::NativeOnEnable()
{
	Super::NativeOnEnable();
	WireRows();
	// Shown -- pushed, or uncovered by the screen above it closing: back to where the focus was, else the entry row.
	if (UNieRRow* Row = LastFocused.IsValid() ? LastFocused.Get() : GetEntryRow())
	{
		FocusRow(Row);
	}
	PlayEntrance();
}

void UNieRScreen::PlayEntrance()
{
	Motion.FadeIn(GetContentRoot(), 0.25f);
}

void UNieRScreen::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	ScreenTime += DeltaTime;
	if (const APlayerController* Player = GetOwningPlayer())
	{
		float X = 0.0f;
		float Y = 0.0f;
		if (Player->GetMousePosition(X, Y))
		{
			const FVector2D Mouse(X, Y);
			if (LastMousePosition.X >= 0.0 && !Mouse.Equals(LastMousePosition, 0.5))
			{
				MouseMovedTime = ScreenTime;
			}
			LastMousePosition = Mouse;
		}
	}
}

bool UNieRScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (UNieRRow* Row = Highlighted.Get(); Row != nullptr && EventData != nullptr)
	{
		const FKey& Key = EventData->Key;
		int32 Delta = 0;
		if (Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left)
		{
			Delta = -1;
		}
		else if (Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right)
		{
			Delta = 1;
		}
		// A row with a value takes Left and Right; any other lets them move the focus.
		if (Delta != 0 && Row->Adjust(Delta))
		{
			return true;
		}
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNieRScreen::HandleRowState(UNieRRow* Row, EUISelectableSelectionState InState)
{
	using namespace NieRScreenLocal;
	if (!IsValid(Row))
	{
		return;
	}
	if (InState == EUISelectableSelectionState::Focused)
	{
		LastFocused = Row;
	}
	if (InState == EUISelectableSelectionState::Hovered)
	{
		// The mouse selects, as in the game -- but only a mouse that moved: rows sliding in under a resting pointer do not.
		if (ScreenTime - MouseMovedTime < 0.25f && Highlighted.Get() != Row)
		{
			FocusRow(Row);
		}
		return;
	}
	if (IsHighlightState(InState))
	{
		if (Highlighted.Get() != Row)
		{
			Highlight(Row);
		}
		return;
	}
	if (Highlighted.Get() != Row)
	{
		return;
	}
	Row->SetHighlighted(false);
	Highlighted.Reset();
	// The pointer left the row it was over: the highlight goes back to the row that holds the focus.
	for (const TWeakObjectPtr<UNieRRow>& Other : Rows)
	{
		if (UNieRRow* Live = Other.Get(); Live != nullptr && Live->HasRowFocus())
		{
			Highlight(Live);
			break;
		}
	}
}

void UNieRScreen::HandleRowClicked(UNieRRow* Row)
{
	if (IsValid(Row))
	{
		OnRowClicked(Row);
	}
}

void UNieRScreen::Highlight(UNieRRow* Row)
{
	UNieRRow* Previous = Highlighted.Get();
	if (Previous != nullptr)
	{
		Previous->SetHighlighted(false);
	}
	Highlighted = Row;
	Row->SetHighlighted(true, Previous);
	if (UNieRHintBar* Bar = HintBar.Get())
	{
		Bar->SetHint(Row->Description);
	}
	OnRowHighlighted(Row);
}

UNieRRow* UNieRScreen::FindRow(FName Name) const
{
	const FString Wanted = Name.ToString();
	for (const TWeakObjectPtr<UNieRRow>& Row : Rows)
	{
		if (UNieRRow* Live = Row.Get(); Live != nullptr && Live->GetDisplayName() == Wanted)
		{
			return Live;
		}
	}
	return nullptr;
}

void UNieRScreen::FocusRow(UNieRRow* Row)
{
	if (IsValid(Row))
	{
		FocusWidget(Row->GetFocusWidget());
	}
}

void UNieRScreen::FocusWidget(UDreamWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Widget->SetKeyboardFocus();
	PendingFocus = Widget;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UNieRScreen::ApplyPendingFocus));
	}
}

void UNieRScreen::ApplyPendingFocus()
{
	UDreamWidget* Widget = PendingFocus.Get();
	PendingFocus.Reset();
	if (IsValid(Widget) && !Widget->HasFocus())
	{
		Widget->SetKeyboardFocus();
	}
}

void UNieRScreen::SlideRowsIn(const TArray<UNieRRow*>& InRows, FVector2D Offset, float Duration, float Stagger, float Delay)
{
	for (int32 Index = 0; Index < InRows.Num(); ++Index)
	{
		Motion.SlideIn(InRows[Index], Offset, Duration, Delay + Stagger * Index);
	}
}

void UNieRScreen::PushScreen(FName InName, const TCHAR* InClassPath)
{
	UDreamScreenUISubsystem* ScreenUI = UDreamScreenUISubsystem::Get(GetWorld());
	UClass* ScreenClass = LoadClass<UDreamUserWidget>(nullptr, InClassPath);
	if (ScreenUI == nullptr || ScreenClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("NieR UI test: cannot push %s (%s)"), *InName.ToString(), InClassPath);
		return;
	}
	ScreenUI->PushWidgetOfClass(InName, ScreenClass, EDreamUIScreenPageCachePolicy::DestroyOnPop, true, GetOwningPlayer());
}

void UNieRScreen::PopThisScreen()
{
	if (UDreamScreenUISubsystem* ScreenUI = UDreamScreenUISubsystem::Get(GetWorld()))
	{
		ScreenUI->PopUI(GetOwningPlayer());
	}
}
