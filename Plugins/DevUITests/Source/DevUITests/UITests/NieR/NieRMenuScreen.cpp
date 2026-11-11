#include "NieRMenuScreen.h"

#include "Core/Components/DreamWidget.h"
#include "Kismet/KismetSystemLibrary.h"

namespace NieRMenuScreenLocal
{
	const TCHAR* const TabSubtitles[UNieRMenuScreen::TabCount] = {
		TEXT("-Area Map"), TEXT("-Quest List"), TEXT("-All Items"), TEXT("-Weapon Sets"), TEXT("-Plug-in Chips"), TEXT("-Archives"), TEXT("-System Menu") };
}

void UNieRMenuScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// What the inventory would hand the page. Made here, before the bindings resolve and the `for` asks for it.
	struct FEntry { const TCHAR* Label; const TCHAR* Count; const TCHAR* Description; };
	static const FEntry Entries[] = {
		{ TEXT("Warped Wire"), TEXT("5"), TEXT("A length of wire bent out of shape.") },
		{ TEXT("Stretched Coil"), TEXT("3"), TEXT("A coil that has lost its spring.") },
		{ TEXT("Broken Circuit"), TEXT("35"), TEXT("A circuit board with a crack through it.") },
		{ TEXT("Pristine Screw"), TEXT("1"), TEXT("A screw without a scratch on it.") },
		{ TEXT("Small Gear"), TEXT("7"), TEXT("A small gear from a machine's joint.") },
		{ TEXT("Large Gear"), TEXT("4"), TEXT("A heavy gear from a machine's core.") },
		{ TEXT("Rusty Bolt"), TEXT("21"), TEXT("A rusted, beat-up bolt.") },
		{ TEXT("New Bolt"), TEXT("5"), TEXT("A bolt that has never been used.") },
		{ TEXT("Crushed Nut"), TEXT("9"), TEXT("A nut flattened by something heavy.") },
	};
	Items.Reset();
	for (const FEntry& Entry : Entries)
	{
		UNieRListItem* Item = NewObject<UNieRListItem>(this);
		Item->Label = FText::FromString(Entry.Label);
		Item->Count = FText::FromString(Entry.Count);
		Item->Description = FText::FromString(Entry.Description);
		Items.Add(Item);
	}

	for (int32 Index = 0; Index < TabCount; ++Index)
	{
		if (UNieRTab* Tab = GetTab(Index))
		{
			Tab->OnClicked.AddWeakLambda(this, [this, Index](UNieRTab*)
			{
				if (Index != ActiveTab)
				{
					SelectTab(Index, true, Index > ActiveTab ? 1 : -1);
				}
			});
		}
	}
	SelectTab(ActiveTab, false, 0);
}

TArray<UObject*> UNieRMenuScreen::GetItems() const
{
	return TArray<UObject*>(Items);
}

UNieRTab* UNieRMenuScreen::GetTab(int32 InIndex) const
{
	// The tabs are a `rows` table: their ids are made from their labels, so they are found by their place in the bar.
	const UDreamWidget* Tabs = Node(TEXT("Tabs"));
	return Tabs != nullptr && Tabs->GetChildren().IsValidIndex(InIndex) ? Cast<UNieRTab>(Tabs->GetChildren()[InIndex]) : nullptr;
}

UNieRListPage* UNieRMenuScreen::GetPage(int32 InIndex) const
{
	return Cast<UNieRListPage>(Node(*FString::Printf(TEXT("Page_%d"), InIndex)));
}

UNieRRow* UNieRMenuScreen::GetEntryRow() const
{
	const TArray<UNieRRow*> PageRows = CollectRows(GetPage(ActiveTab));
	return PageRows.Num() > 0 ? PageRows[0] : nullptr;
}

void UNieRMenuScreen::PlayEntrance()
{
	Super::PlayEntrance();
	for (int32 Index = 0; Index < TabCount; ++Index)
	{
		Motion.SlideIn(GetTab(Index), FVector2D(0.0, -18.0), 0.26f, 0.03f * Index);
	}
	Motion.SlideIn(GetPage(ActiveTab), FVector2D(0.0, 24.0), 0.3f, 0.12f);
	if (UNieRHeader* Header = Cast<UNieRHeader>(Node(TEXT("Header"))))
	{
		Header->PlayEntrance(0.08f);
	}
}

bool UNieRMenuScreen::CanSwitchTab(int32 InUserIndex) const
{
	return IsConstructed() && GetVisibility() != EDreamWidgetVisibility::Collapsed;
}

bool UNieRMenuScreen::SwitchTab(int32 InUserIndex, int32 InDelta)
{
	if (InDelta == 0)
	{
		return false;
	}
	SelectTab((ActiveTab + InDelta % TabCount + TabCount) % TabCount, true, InDelta > 0 ? 1 : -1);
	return true;
}

void UNieRMenuScreen::SelectTab(int32 InIndex, bool bInFocusFirstRow, int32 InDirection)
{
	using namespace NieRMenuScreenLocal;
	const bool bChanged = InIndex != ActiveTab;
	ActiveTab = InIndex;
	for (int32 Index = 0; Index < TabCount; ++Index)
	{
		const bool bActive = Index == ActiveTab;
		if (UNieRTab* Tab = GetTab(Index))
		{
			Tab->SetOpen(bActive, bActive && bChanged ? InDirection : 0);
		}
		SetShown(GetPage(Index), bActive);
	}
	if (UNieRHeader* Header = Cast<UNieRHeader>(Node(TEXT("Header"))))
	{
		const UNieRTab* Tab = GetTab(ActiveTab);
		Header->SetTitle(Tab != nullptr ? Tab->Label : FText::GetEmpty(), FText::FromString(TabSubtitles[ActiveTab]), bChanged && InDirection != 0);
	}
	if (bChanged && InDirection != 0)
	{
		// The page comes in from the side it was switched to.
		Motion.SlideIn(GetPage(ActiveTab), FVector2D(70.0 * InDirection, 0.0), 0.24f);
	}
	if (bInFocusFirstRow)
	{
		FocusRow(GetEntryRow());
	}
}

void UNieRMenuScreen::OnRowHighlighted(UNieRRow* Row)
{
	// The page the row is on shows it in its detail panel.
	for (UDreamWidget* Walker = Row; IsValid(Walker); Walker = Walker->GetParent())
	{
		if (UNieRListPage* Page = Cast<UNieRListPage>(Walker))
		{
			Page->ShowRow(Row);
			return;
		}
	}
}

void UNieRMenuScreen::OnRowClicked(UNieRRow* Row)
{
	if (Row->Action == TEXT("settings"))
	{
		PushScreen(TEXT("NieR_Settings"), NieRUI::SettingsClass);
	}
	else if (Row->Action == TEXT("title"))
	{
		PopThisScreen();
	}
	else if (Row->Action == TEXT("exit"))
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}
