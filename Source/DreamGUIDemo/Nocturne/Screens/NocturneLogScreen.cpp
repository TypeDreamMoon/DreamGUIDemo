// Copyright DreamMoon. All Rights Reserved.

#include "NocturneLogScreen.h"

#include "../Settings/NocturneUI.h"

#include "Controls/DreamExpandableArea.h"
#include "Controls/DreamProgressBar.h"
#include "Core/Components/DreamPanelLayouts.h"
#include "Core/Components/DreamWidget.h"
#include "Extensions/DreamMaskTransition.h"
#include "Interaction/UIButton.h"

namespace NocturneLogLocal
{
	enum class EKind : uint8 { Main, Side, Message };
	enum class EState : uint8 { Active, Done, Unread, Read };

	struct FEntry
	{
		EKind Kind;
		EState State;
		const TCHAR* Title;
		const TCHAR* En;
		const TCHAR* Text;
		const TCHAR* Place;
		const TCHAR* From;
		const TCHAR* Reward;
		float Progress;
	};

	// The objectives themselves are in Nocturne_Log.dui, under each entry.
	const FEntry EntryDefs[] = {
		{ EKind::Main, EState::Active, TEXT("旧港的灯"), TEXT("HARBOR LIGHTS"),
			TEXT("旧港的灯塔熄了三十年。看守说，只要有人把灯芯放回去，港口就会记起自己的名字。"),
			TEXT("旧港 · 第六象限"), TEXT("灯塔看守"), TEXT("航标灯芯"), 0.5f },
		{ EKind::Main, EState::Active, TEXT("第六颗星"), TEXT("THE SIXTH STAR"),
			TEXT("星图上第六颗星的位置是空的。有人在那里留下过一封信，信封上只有星位。"),
			TEXT("第六象限"), TEXT("未署名"), TEXT("星图残卷"), 0.33f },
		{ EKind::Side, EState::Active, TEXT("走调的八音盒"), TEXT("OUT OF TUNE"),
			TEXT("八音盒的最后一个音总是走调。也许不是坏了，而是在等谁来校。"),
			TEXT("吟 · 音乐盒"), TEXT("管理员"), TEXT("曲目「启明之前」"), 0.5f },
		{ EKind::Side, EState::Done, TEXT("银丝与帆"), TEXT("THREAD AND SAIL"),
			TEXT("太阳帆裂了一道口子。银丝够用了，帆也补好了。"),
			TEXT("启明 · 补给"), TEXT("补给官"), TEXT("太阳帆片 x2"), 1.0f },
		{ EKind::Side, EState::Done, TEXT("流星雨之夜"), TEXT("METEOR NIGHT"),
			TEXT("第二象限的流星雨每年只有一夜。拾到的星鳞背面有细小的纹路。"),
			TEXT("第二象限"), TEXT("—"), TEXT("星鳞 x4"), 1.0f },
		{ EKind::Message, EState::Unread, TEXT("来自 0721 的留言"), TEXT("NOTE FROM 0721"),
			TEXT("灯芯在补给舱第三格。别忘了关舱门。"),
			TEXT("—"), TEXT("0721"), TEXT("—"), 0.0f },
		{ EKind::Message, EState::Read, TEXT("补给到港"), TEXT("SUPPLIES ARRIVED"),
			TEXT("启明补给已送达：太阳帆片两片。"),
			TEXT("启明"), TEXT("补给系统"), TEXT("—"), 1.0f },
	};

	const TCHAR* KindName(EKind Kind)
	{
		return Kind == EKind::Main ? TEXT("主线  ·  MAIN") : (Kind == EKind::Side ? TEXT("支线  ·  SIDE") : TEXT("讯息  ·  MESSAGE"));
	}

	const TCHAR* StateName(EState State)
	{
		switch (State)
		{
		case EState::Active: return TEXT("进行中   IN PROGRESS");
		case EState::Done: return TEXT("已完成   COMPLETE");
		case EState::Unread: return TEXT("未读   UNREAD");
		default: return TEXT("已读   READ");
		}
	}

	FColor StateColor(EState State)
	{
		return State == EState::Active ? NocturneUI::Cyan : (State == EState::Unread ? NocturneUI::Amber : NocturneUI::InkDim);
	}

	bool InCategory(int32 Category, EKind Kind)
	{
		return Category == 0 || (Category == 1 && Kind == EKind::Main) || (Category == 2 && Kind == EKind::Side) || (Category == 3 && Kind == EKind::Message);
	}
}

void UNocturneLogScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	for (int32 Index = 0; Index < EntryCount; ++Index)
	{
		Entries.Add(Cast<UDreamExpandableArea>(Node(*FString::Printf(TEXT("Quest%d"), Index))));
		if (Index == 0)
		{
			if (UDreamWidget* List = Node(TEXT("QuestList")))
			{
				QuestScroll = Cast<UDreamLayoutContainerScrollBox>(List->GetLayoutContainer());
			}
		}
	}
	WasExpanded.Init(false, EntryCount);
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

	int32 Counts[CategoryCount] = { 0, 0, 0, 0 };
	int32 Active = 0;
	int32 Done = 0;
	for (const NocturneLogLocal::FEntry& Entry : NocturneLogLocal::EntryDefs)
	{
		for (int32 Cat = 0; Cat < CategoryCount; ++Cat)
		{
			Counts[Cat] += NocturneLogLocal::InCategory(Cat, Entry.Kind) ? 1 : 0;
		}
		Active += Entry.State == NocturneLogLocal::EState::Active ? 1 : 0;
		Done += Entry.State == NocturneLogLocal::EState::Done ? 1 : 0;
	}
	for (int32 Cat = 0; Cat < CategoryCount; ++Cat)
	{
		NocturneUI::SetText(Node(*FString::Printf(TEXT("Cat%dCount"), Cat)), FText::AsNumber(Counts[Cat]));
	}
	NocturneUI::SetText(Node(TEXT("LeftFoot")), FText::FromString(FString::Printf(TEXT("进行中 %d    已完成 %d    未读 1"), Active, Done)));
	NocturneUI::SetText(Node(TEXT("PageCount")), FText::FromString(FString::Printf(TEXT("%d 条记录"), EntryCount)));

	ShowCategory(0);
	OpenEntry(0);
	ShowDetail(0, false);
}

void UNocturneLogScreen::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	UpdateScroll();
	// An entry opened by its own header: it is the one shown, and the others close.
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const UDreamExpandableArea* Area = Entries[Index].Get();
		const bool bExpanded = Area != nullptr && Area->GetIsExpanded();
		if (bExpanded && !WasExpanded[Index])
		{
			OpenEntry(Index);
		}
		WasExpanded[Index] = Area != nullptr && Area->GetIsExpanded();
	}
}

void UNocturneLogScreen::OpenEntry(int32 InIndex)
{
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		if (UDreamExpandableArea* Area = Entries[Index].Get())
		{
			const bool bOpen = Index == InIndex;
			if (Area->GetIsExpanded() != bOpen)
			{
				Area->SetIsExpanded(bOpen);
			}
			WasExpanded[Index] = bOpen;
		}
	}
	ShowDetail(InIndex, true);
	// Its objectives flip up one after another.
	RunInChildren(Node(*FString::Printf(TEXT("Q%dObjectives"), InIndex)), 0.0f, 0.025f, FVector2D(-12.0, 0.0));
	// The opened entry is kept in view while it unfolds, so what it opened is never left below the panel's edge.
	RevealEntry = InIndex;
	RevealUntil = GetScreenTime() + 0.45f;
}

void UNocturneLogScreen::UpdateScroll()
{
	UDreamLayoutContainerScrollBox* Scroll = QuestScroll.Get();
	if (Scroll == nullptr)
	{
		return;
	}
	if (RevealUntil >= 0.0f)
	{
		if (UDreamExpandableArea* Area = Entries.IsValidIndex(RevealEntry) ? Entries[RevealEntry].Get() : nullptr)
		{
			Scroll->ScrollWidgetIntoView(Area, false, EDreamUIScrollDestination::IntoView);
		}
		if (GetScreenTime() >= RevealUntil)
		{
			RevealUntil = -1.0f;
		}
	}
	// The bar: as long as the list is to its window, as far down as the view is through it.
	const bool bNeeded = Scroll->IsScrollNeeded();
	NocturneUI::SetShown(Node(TEXT("QuestTrack")), bNeeded);
	UDreamWidget* Thumb = Node(TEXT("QuestThumb"));
	NocturneUI::SetShown(Thumb, bNeeded);
	if (bNeeded && Thumb != nullptr)
	{
		constexpr double Track = 570.0;
		const double Length = FMath::Max(Track * Scroll->GetViewFraction(), 24.0);
		Thumb->SetSizeDelta(FVector2D(3.0, Length));
		Thumb->SetAnchoredPosition(FVector2D(436.0, -116.0 - (Track - Length) * Scroll->GetViewOffsetFraction()));
	}
}

void UNocturneLogScreen::ShowDetail(int32 InIndex, bool bInAnimate)
{
	using namespace NocturneLogLocal;
	if (InIndex < 0 || InIndex >= EntryCount || (InIndex == Shown && bInAnimate))
	{
		return;
	}
	Shown = InIndex;
	const FEntry& Entry = EntryDefs[InIndex];
	NocturneUI::SetText(Node(TEXT("DetailTag")), FText::FromString(KindName(Entry.Kind)));
	NocturneUI::SetText(Node(TEXT("DetailTitle")), FText::FromString(Entry.Title));
	NocturneUI::SetText(Node(TEXT("DetailEn")), FText::FromString(Entry.En));
	NocturneUI::SetText(Node(TEXT("DetailStatus")), FText::FromString(StateName(Entry.State)));
	NocturneUI::SetTextColor(Node(TEXT("DetailStatus")), StateColor(Entry.State));
	NocturneUI::SetText(Node(TEXT("DetailText")), FText::FromString(Entry.Text));
	NocturneUI::SetText(Node(TEXT("InfoPlace")), FText::FromString(Entry.Place));
	NocturneUI::SetText(Node(TEXT("InfoGiver")), FText::FromString(Entry.From));
	NocturneUI::SetText(Node(TEXT("InfoReward")), FText::FromString(Entry.Reward));
	NocturneUI::SetText(Node(TEXT("ProgressValue")), FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Entry.Progress * 100.0f))));
	if (UDreamProgressBar* Bar = Cast<UDreamProgressBar>(Node(TEXT("ProgressBar"))))
	{
		Bar->SetPercent(Entry.Progress);
	}
	if (bInAnimate)
	{
		// The detail turns in like a page hinged on its left edge, and wipes in from the left under a mask.
		if (UDreamWidget* Body = Node(TEXT("DetailBody")))
		{
			Body->SetRenderTransformPivot(FVector2D(0.0, 0.5));
			SwingIn(Body, 0.0f, 0.45f, FRotator(0.0, -62.0, 0.0), 90.0);
			if (UDreamMaskTransition* Mask = UDreamMaskTransition::FindOrAdd(Body))
			{
				Mask->SetProgress(0.0f);
				Mask->PlayForward(0.4f);
			}
		}
		BurstSort(RightSortIndex, 0.45f, 0.3f);
	}
}

void UNocturneLogScreen::ShowCategory(int32 InCategory)
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
	int32 FirstShown = INDEX_NONE;
	for (int32 Index = 0; Index < Entries.Num(); ++Index)
	{
		const bool bIn = NocturneLogLocal::InCategory(Category, NocturneLogLocal::EntryDefs[Index].Kind);
		NocturneUI::SetShown(Entries[Index].Get(), bIn);
		if (bIn && FirstShown == INDEX_NONE)
		{
			FirstShown = Index;
		}
	}
	PaintCategories();
	BurstSort(CenterSortIndex, 0.45f, 0.3f);
	if (Shown != INDEX_NONE && !NocturneLogLocal::InCategory(Category, NocturneLogLocal::EntryDefs[Shown].Kind) && FirstShown != INDEX_NONE)
	{
		OpenEntry(FirstShown);
	}
}

bool UNocturneLogScreen::SwitchTab(int32 InUserIndex, int32 InDelta)
{
	if (InDelta == 0)
	{
		return false;
	}
	ShowCategory((Category + InDelta % CategoryCount + CategoryCount) % CategoryCount);
	return true;
}

void UNocturneLogScreen::PaintCategories()
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

UDreamWidget* UNocturneLogScreen::GetInitialFocus() const
{
	return Entries.IsValidIndex(0) ? Entries[0].Get() : nullptr;
}

void UNocturneLogScreen::PlayContentEntrance(float Delay)
{
	RunInChildren(Node(TEXT("LeftHeader")), Delay, 0.03f, FVector2D(-16.0, 0.0));
	RunInChildren(Node(TEXT("CatList")), Delay + 0.04f, 0.03f, FVector2D(-20.0, 0.0));
	RunIn(Node(TEXT("LeftFoot")), Delay + 0.2f, 0.28f, FVector2D(-12.0, 0.0));
	RunInChildren(Node(TEXT("PageHeader")), Delay + 0.06f, 0.03f, FVector2D(-16.0, 0.0));
	RunInChildren(Node(TEXT("QuestList")), Delay + 0.1f, 0.04f, FVector2D(-24.0, 0.0));
	RunInChildren(Node(TEXT("DetailBody")), Delay + 0.14f, 0.03f, FVector2D(16.0, 0.0));
}
