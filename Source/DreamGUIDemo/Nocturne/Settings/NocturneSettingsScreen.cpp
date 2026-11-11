// Copyright DreamMoon. All Rights Reserved.

#include "NocturneSettingsScreen.h"

#include "NocturneOptionRow.h"
#include "NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Engine/World.h"
#include "Event/DreamKeyEventData.h"
#include "Interaction/UIButton.h"
#include "TimerManager.h"

namespace NocturneSettingsLocal
{
	const TCHAR* PageTitles[] = { TEXT("画面"), TEXT("显示"), TEXT("音频"), TEXT("操作"), TEXT("辅助功能"), TEXT("语言") };
	const TCHAR* PageTags[] = { TEXT("GRAPHICS"), TEXT("DISPLAY"), TEXT("AUDIO"), TEXT("CONTROLS"), TEXT("ACCESSIBILITY"), TEXT("LANGUAGE") };

	constexpr int32 ImpactSegments = 5;

	void CollectRows(UDreamWidget* InWidget, TArray<TWeakObjectPtr<UNocturneOptionRow>>& OutRows)
	{
		if (!IsValid(InWidget))
		{
			return;
		}
		if (UNocturneOptionRow* Row = Cast<UNocturneOptionRow>(InWidget))
		{
			OutRows.Add(Row);
			return;
		}
		for (UDreamWidget* Child : InWidget->GetChildren())
		{
			CollectRows(Child, OutRows);
		}
	}
}

void UNocturneSettingsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	for (int32 Index = 0; Index < PageCount; ++Index)
	{
		Pages.Add(Node(*FString::Printf(TEXT("Page%d"), Index)));
		if (UDreamWidget* Tab = Node(*FString::Printf(TEXT("Tab%d"), Index)))
		{
			if (UUIButton* Button = Tab->GetComponent<UUIButton>())
			{
				Button->SetUseFocusRing(false);
				Button->GetOnClickEvent().AddWeakLambda(this, [this, Index]() { ShowPage(Index); });
				Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Index](EUISelectableSelectionState InState, bool)
				{
					const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
					if (bOver)
					{
						HoveredTab = Index;
					}
					else if (HoveredTab == Index)
					{
						HoveredTab = INDEX_NONE;
					}
					PaintTabs();
				});
			}
		}
	}
	if (UDreamWidget* Yes = Node(TEXT("ConfirmYes")))
	{
		if (UUIButton* Button = Yes->GetComponent<UUIButton>())
		{
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { ConfirmReset(); });
		}
	}
	if (UDreamWidget* No = Node(TEXT("ConfirmNo")))
	{
		if (UUIButton* Button = No->GetComponent<UUIButton>())
		{
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { ShowConfirm(false); });
		}
	}

	WireRows();
	NocturneUI::SetShown(Node(TEXT("ConfirmPanel")), false);
	NocturneUI::SetShown(Node(TEXT("ConfirmScrim")), false);
	ShowPage(0, false);
	UpdateDirty();
}

bool UNocturneSettingsScreen::CanGoBack() const
{
	return !bConfirmOpen && !Listening.IsValid();
}

void UNocturneSettingsScreen::WireRows()
{
	PageRows.SetNum(PageCount);
	for (int32 Index = 0; Index < PageCount; ++Index)
	{
		PageRows[Index].Reset();
		NocturneSettingsLocal::CollectRows(Pages.IsValidIndex(Index) ? Pages[Index].Get() : nullptr, PageRows[Index]);
		for (const TWeakObjectPtr<UNocturneOptionRow>& Row : PageRows[Index])
		{
			if (UNocturneOptionRow* Live = Row.Get())
			{
				Live->OnStateChanged.AddUObject(this, &UNocturneSettingsScreen::HandleRowState);
				Live->OnClicked.AddUObject(this, &UNocturneSettingsScreen::HandleRowClicked);
				Live->OnValueChanged.AddUObject(this, &UNocturneSettingsScreen::HandleRowValue);
			}
		}
	}
	bWired = true;
}

UNocturneOptionRow* UNocturneSettingsScreen::GetRow(int32 InPage, int32 InIndex) const
{
	return PageRows.IsValidIndex(InPage) && PageRows[InPage].IsValidIndex(InIndex) ? PageRows[InPage][InIndex].Get() : nullptr;
}

bool UNocturneSettingsScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData == nullptr)
	{
		return Super::NativeOnKeyDown(EventData);
	}
	const FKey& Key = EventData->Key;

	if (UNocturneOptionRow* Row = Listening.Get(); Row != nullptr && Row->IsListening())
	{
		if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
		{
			Row->SetListening(false);
		}
		else
		{
			Row->TakeKey(Key);
		}
		Listening.Reset();
		return true;
	}
	if (bConfirmOpen)
	{
		if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right)
		{
			ShowConfirm(false);
			return true;
		}
		return Super::NativeOnKeyDown(EventData);
	}
	if (Key == EKeys::R || Key == EKeys::Gamepad_FaceButton_Top)
	{
		AskReset();
		return true;
	}
	if (UNocturneOptionRow* Row = Highlighted.Get())
	{
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

bool UNocturneSettingsScreen::CanSwitchTab(int32 InUserIndex) const
{
	return !bConfirmOpen && !Listening.IsValid();
}

bool UNocturneSettingsScreen::SwitchTab(int32 InUserIndex, int32 InDelta)
{
	if (!CanSwitchTab(InUserIndex) || InDelta == 0)
	{
		return false;
	}
	ShowPage((Page + InDelta % PageCount + PageCount) % PageCount);
	return true;
}

void UNocturneSettingsScreen::ShowPage(int32 InPage, bool bInAnimate)
{
	using namespace NocturneSettingsLocal;
	const int32 Previous = Page;
	Page = FMath::Clamp(InPage, 0, PageCount - 1);
	if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Tab%dBar"), Page)); Bar != nullptr && bInAnimate && Previous != Page)
	{
		Settle(Bar);
		SetOffset(Bar, FVector2D(0.0, 70.0 * (Previous - Page)));
		Bar->RenderOffsetTo(FVector2D::ZeroVector, 0.2f, 0.0f, EDreamTweenEase::OutCubic);
	}
	// A page still turning away from the last switch is put back at once.
	if (Pages.IsValidIndex(LeavingPage))
	{
		FinishLeaving();
	}
	for (int32 Index = 0; Index < Pages.Num(); ++Index)
	{
		NocturneUI::SetShown(Pages[Index].Get(), Index == Page);
	}
	if (bInAnimate && Previous != Page && Pages.IsValidIndex(Previous))
	{
		// The pages turn like the faces of a prism: the old one away to one side, the new one in from the other.
		const double Dir = Page > Previous ? 1.0 : -1.0;
		if (UDreamWidget* Old = Pages[Previous].Get())
		{
			const FRestPose Rest = GetRest(Old);
			NocturneUI::SetShown(Old, true);
			Settle(Old);
			RotateTo(Old, Rest.Rotation + FRotator(0.0, 70.0 * Dir, 0.0), 0.3f, 0.0f, EDreamTweenEase::InQuad);
			Old->RenderTranslationTo(Rest.Translation + FVector(150.0, -170.0 * Dir, 0.0), 0.3f, 0.0f, EDreamTweenEase::InQuad);
			Old->RenderOpacityTo(0.0f, 0.22f, 0.06f, EDreamTweenEase::InQuad);
			LeavingPage = Previous;
			LeavingUntil = GetScreenTime() + 0.31f;
		}
		if (UDreamWidget* New = Pages[Page].Get())
		{
			const FRestPose Rest = GetRest(New);
			Settle(New);
			New->SetRenderRotation(Rest.Rotation + FRotator(0.0, -70.0 * Dir, 0.0));
			New->SetRenderTranslation(Rest.Translation + FVector(150.0, 170.0 * Dir, 0.0));
			New->SetRenderOpacity(0.0f);
			RotateTo(New, Rest.Rotation, 0.4f, 0.04f, EDreamTweenEase::OutCubic);
			New->RenderTranslationTo(Rest.Translation, 0.4f, 0.04f, EDreamTweenEase::OutCubic);
			New->RenderOpacityTo(1.0f, 0.2f, 0.06f, EDreamTweenEase::OutQuad);
		}
	}
	PaintTabs();
	NocturneUI::SetText(Node(TEXT("PageTitle")), FText::FromString(PageTitles[Page]));
	NocturneUI::SetText(Node(TEXT("PageEn")), FText::FromString(PageTags[Page]));
	NocturneUI::SetText(Node(TEXT("PageIndex")), FText::FromString(FString::Printf(TEXT("%02d / %02d"), Page + 1, PageCount)));

	if (bInAnimate)
	{
		// Only the options and the detail change; the tab rail stays where it is. The page itself turns (above).
		RunInChildren(Node(TEXT("PageHeader")), 0.0f, 0.03f, FVector2D(-16.0, 0.0));
		BurstSort(CenterSortIndex, 0.7f, 0.35f);
		BurstSort(RightSortIndex, 0.55f, 0.3f);
	}
	if (UNocturneOptionRow* First = GetRow(Page, 0))
	{
		FocusRow(First);
		Highlight(First);
	}
}

void UNocturneSettingsScreen::PaintTabs()
{
	using namespace NocturneUI;
	for (int32 Index = 0; Index < PageCount; ++Index)
	{
		const bool bOn = Index == Page;
		const bool bOver = Index == HoveredTab;
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Tab%dBar"), Index)), bOn);
		SetTint(Node(*FString::Printf(TEXT("Tab%dBack"), Index)), bOn ? RowLit : (bOver ? RowHover : Clear));
		SetTextColor(Node(*FString::Printf(TEXT("Tab%dText"), Index)), bOn ? InkBright : (bOver ? Ink : InkDim));
		SetTextColor(Node(*FString::Printf(TEXT("Tab%dEn"), Index)), bOn ? CyanTag : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Tab%dNum"), Index)), bOn ? Cyan : InkFaint);
	}
}

void UNocturneSettingsScreen::HandleRowState(UNocturneOptionRow* Row, EUISelectableSelectionState InState)
{
	if (!IsValid(Row) || bConfirmOpen)
	{
		return;
	}
	if (InState == EUISelectableSelectionState::Hovered)
	{
		// The pointer selects: the row under it takes the focus.
		if (Highlighted.Get() != Row)
		{
			FocusRow(Row);
			Highlight(Row);
		}
		return;
	}
	if (InState == EUISelectableSelectionState::Focused || InState == EUISelectableSelectionState::Pressed)
	{
		if (Highlighted.Get() != Row)
		{
			Highlight(Row);
		}
	}
}

void UNocturneSettingsScreen::HandleRowClicked(UNocturneOptionRow* Row)
{
	if (!IsValid(Row))
	{
		return;
	}
	if (Row->IsListening())
	{
		Listening = Row;
	}
	if (Row->Kind == ENocturneOptionKind::Action && Row->Action == TEXT("reset"))
	{
		AskReset();
	}
}

void UNocturneSettingsScreen::HandleRowValue(UNocturneOptionRow* Row)
{
	UpdateDirty();
	if (Row == Highlighted.Get())
	{
		UpdateDetail(Row, false);
	}
}

void UNocturneSettingsScreen::Highlight(UNocturneOptionRow* Row)
{
	if (UNocturneOptionRow* Previous = Highlighted.Get(); Previous != nullptr && Previous != Row)
	{
		Previous->SetHighlighted(false);
	}
	Highlighted = Row;
	if (IsValid(Row))
	{
		Row->SetHighlighted(true);
		UpdateDetail(Row, true);
	}
}

void UNocturneSettingsScreen::UpdateDetail(UNocturneOptionRow* Row, bool bInAnimate)
{
	using namespace NocturneUI;
	if (!IsValid(Row))
	{
		return;
	}
	SetText(Node(TEXT("DetailTitle")), Row->Label);
	SetText(Node(TEXT("DetailEn")), FText::FromString(Row->En));
	SetText(Node(TEXT("DetailText")), Row->Description);
	const FText Current = Row->GetValueText();
	SetText(Node(TEXT("DetailBig")), Current.IsEmpty() ? FText::FromString(TEXT("—")) : Current);
	SetText(Node(TEXT("DetailCurrent")), FText::FromString(FString::Printf(TEXT("当前   %s"), *Current.ToString())));
	SetText(Node(TEXT("DetailDefault")), FText::FromString(FString::Printf(TEXT("默认   %s"), *Row->GetDefaultValueText().ToString())));
	SetTextColor(Node(TEXT("DetailCurrent")), Row->IsChanged() ? Amber : Ink);
	for (int32 Index = 0; Index < NocturneSettingsLocal::ImpactSegments; ++Index)
	{
		SetTint(Node(*FString::Printf(TEXT("Impact%d"), Index)), Index < Row->Impact ? Cyan : FColor(0xFF, 0xFF, 0xFF, 0x18));
	}
	NocturneUI::SetShown(Node(TEXT("RestartNote")), Row->bNeedsRestart);
	if (bInAnimate)
	{
		RunInChildren(Node(TEXT("DetailBody")), 0.0f, 0.02f, FVector2D(12.0, 0.0));
	}
}

void UNocturneSettingsScreen::UpdateDirty()
{
	bool bDirty = false;
	for (const TArray<TWeakObjectPtr<UNocturneOptionRow>>& Rows : PageRows)
	{
		for (const TWeakObjectPtr<UNocturneOptionRow>& Row : Rows)
		{
			bDirty |= Row.IsValid() && Row->IsChanged();
		}
	}
	NocturneUI::SetShown(Node(TEXT("DirtyNote")), bDirty);
}

void UNocturneSettingsScreen::AskReset()
{
	if (!bConfirmOpen)
	{
		ShowConfirm(true);
	}
}

void UNocturneSettingsScreen::ShowConfirm(bool bInShow)
{
	bConfirmOpen = bInShow;
	UDreamWidget* Panel = Node(TEXT("ConfirmPanel"));
	UDreamWidget* Scrim = Node(TEXT("ConfirmScrim"));
	NocturneUI::SetShown(Panel, bInShow);
	NocturneUI::SetShown(Scrim, bInShow);
	if (bInShow && IsValid(Scrim))
	{
		Scrim->SetRenderOpacity(0.0f);
		Scrim->RenderOpacityTo(1.0f, 0.18f, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (bInShow && IsValid(Panel))
	{
		Panel->SetRenderOpacity(0.0f);
		Panel->RenderOpacityTo(1.0f, 0.18f, 0.0f, EDreamTweenEase::OutCubic);
		SetOffset(Panel, FVector2D(0.0, 16.0));
		Panel->RenderOffsetTo(FVector2D::ZeroVector, 0.22f, 0.0f, EDreamTweenEase::OutCubic);
		BurstSort(CenterSortIndex, 0.5f, 0.25f);
		if (UDreamWidget* No = Node(TEXT("ConfirmNo")))
		{
			No->SetKeyboardFocus();
		}
	}
	else if (UNocturneOptionRow* Row = Highlighted.Get())
	{
		FocusRow(Row);
	}
}

void UNocturneSettingsScreen::ConfirmReset()
{
	if (PageRows.IsValidIndex(Page))
	{
		for (const TWeakObjectPtr<UNocturneOptionRow>& Row : PageRows[Page])
		{
			if (UNocturneOptionRow* Live = Row.Get())
			{
				Live->ResetValue();
			}
		}
	}
	ShowConfirm(false);
	RunInChildren(Pages.IsValidIndex(Page) ? Pages[Page].Get() : nullptr, 0.0f, 0.02f, FVector2D(-12.0, 0.0));
	BurstSort(CenterSortIndex, 0.6f, 0.3f);
	UpdateDirty();
	UpdateDetail(Highlighted.Get(), false);
}

void UNocturneSettingsScreen::FocusRow(UNocturneOptionRow* Row)
{
	UDreamWidget* Widget = IsValid(Row) ? Row->GetFocusWidget() : nullptr;
	if (!IsValid(Widget))
	{
		return;
	}
	Widget->SetKeyboardFocus();
	// DreamGUI gives the focus only to a widget that is drawn: one shown this frame may take it only next frame.
	PendingFocus = Widget;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &UNocturneSettingsScreen::ApplyPendingFocus));
	}
}

void UNocturneSettingsScreen::ApplyPendingFocus()
{
	UDreamWidget* Widget = PendingFocus.Get();
	PendingFocus.Reset();
	if (IsValid(Widget) && !Widget->HasFocus())
	{
		Widget->SetKeyboardFocus();
	}
}

void UNocturneSettingsScreen::PlayContentEntrance(float Delay)
{
	// Left to right: where you are, the tabs, the page, the detail.
	RunInChildren(Node(TEXT("LeftHeader")), Delay, 0.03f, FVector2D(-16.0, 0.0));
	RunInChildren(Node(TEXT("TabList")), Delay + 0.04f, 0.03f, FVector2D(-20.0, 0.0));
	RunInChildren(Node(TEXT("PageHeader")), Delay + 0.06f, 0.03f, FVector2D(-16.0, 0.0));
	RunInChildren(Pages.IsValidIndex(Page) ? Pages[Page].Get() : nullptr, Delay + 0.1f, 0.03f, FVector2D(-24.0, 0.0));
	RunInChildren(Node(TEXT("DetailBody")), Delay + 0.14f, 0.03f, FVector2D(16.0, 0.0));
}

void UNocturneSettingsScreen::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	if (LeavingPage != INDEX_NONE && GetScreenTime() >= LeavingUntil)
	{
		FinishLeaving();
	}
}

void UNocturneSettingsScreen::FinishLeaving()
{
	if (UDreamWidget* Old = Pages.IsValidIndex(LeavingPage) ? Pages[LeavingPage].Get() : nullptr)
	{
		const FRestPose Rest = GetRest(Old);
		Settle(Old);
		NocturneUI::SetShown(Old, LeavingPage == Page);
		Old->SetRenderRotation(Rest.Rotation);
		Old->SetRenderTranslation(Rest.Translation);
		Old->SetRenderOpacity(1.0f);
	}
	LeavingPage = INDEX_NONE;
	LeavingUntil = -1.0f;
}
