#include "NieRSettingsScreen.h"

#include "Core/Components/DreamWidget.h"

void UNieRSettingsScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	CategoryCount = CollectRows(Node(TEXT("Categories"))).Num();
	ShowCategories(false);
}

int32 UNieRSettingsScreen::CategoryOf(const UNieRRow* Row) const
{
	return IsValid(Row) ? CollectRows(Node(TEXT("Categories"))).IndexOfByKey(Row) : INDEX_NONE;
}

UNieRRow* UNieRSettingsScreen::GetCategoryRow(int32 InCategory) const
{
	const TArray<UNieRRow*> CategoryRows = CollectRows(Node(TEXT("Categories")));
	return CategoryRows.IsValidIndex(InCategory) ? CategoryRows[InCategory] : nullptr;
}

TArray<UNieRRow*> UNieRSettingsScreen::GetOptionRows(int32 InCategory) const
{
	return CollectRows(Node(*FString::Printf(TEXT("OptPage_%d"), InCategory)));
}

UNieRRow* UNieRSettingsScreen::GetEntryRow() const
{
	if (bInOptions)
	{
		const TArray<UNieRRow*> Options = GetOptionRows(Category);
		return Options.Num() > 0 ? Options[0] : nullptr;
	}
	return GetCategoryRow(Category);
}

void UNieRSettingsScreen::PlayEntrance()
{
	Super::PlayEntrance();
	if (UNieRHeader* Header = Cast<UNieRHeader>(Node(TEXT("Header"))))
	{
		Header->PlayEntrance(0.05f);
	}
	// The list showing slides in from the left, one row after another.
	SlideRowsIn(bInOptions ? GetOptionRows(Category) : CollectRows(Node(TEXT("Categories"))), FVector2D(-40.0, 0.0), 0.24f, 0.03f, 0.1f);
}

bool UNieRSettingsScreen::HandleBack()
{
	if (bInOptions)
	{
		ShowCategories(true);
		return true;
	}
	PopThisScreen();
	return true;
}

bool UNieRSettingsScreen::CanSwitchTab(int32 InUserIndex) const
{
	return IsConstructed() && GetVisibility() != EDreamWidgetVisibility::Collapsed;
}

bool UNieRSettingsScreen::SwitchTab(int32 InUserIndex, int32 InDelta)
{
	if (InDelta == 0 || CategoryCount == 0)
	{
		return false;
	}
	const int32 Next = (Category + InDelta % CategoryCount + CategoryCount) % CategoryCount;
	if (bInOptions)
	{
		ShowOptions(Next);
	}
	else
	{
		FocusRow(GetCategoryRow(Next));
	}
	return true;
}

void UNieRSettingsScreen::ShowCategories(bool bInFocus)
{
	bInOptions = false;
	SetShown(Node(TEXT("Categories")), true);
	for (int32 Index = 0; Index < CategoryCount; ++Index)
	{
		SetShown(Node(*FString::Printf(TEXT("OptPage_%d"), Index)), false);
	}
	if (UNieRHeader* Header = Cast<UNieRHeader>(Node(TEXT("Header"))))
	{
		Header->SetSubtitle(FText::GetEmpty(), false);
	}
	if (bInFocus)
	{
		FocusRow(GetCategoryRow(Category));
		// Back from the options: the categories slide back in from the left.
		SlideRowsIn(CollectRows(Node(TEXT("Categories"))), FVector2D(-40.0, 0.0), 0.22f, 0.025f);
	}
}

void UNieRSettingsScreen::ShowOptions(int32 InCategory)
{
	// InCategory throughout: hiding the page that held the focus moves the highlight, and a highlighted category row
	// would set Category (OnRowHighlighted reads it only while the categories show, but the order here does not rely on it).
	Category = InCategory;
	bInOptions = true;
	for (int32 Index = 0; Index < CategoryCount; ++Index)
	{
		SetShown(Node(*FString::Printf(TEXT("OptPage_%d"), Index)), Index == InCategory);
	}
	const TArray<UNieRRow*> Options = GetOptionRows(InCategory);
	FocusRow(Options.Num() > 0 ? Options[0] : nullptr);
	SetShown(Node(TEXT("Categories")), false);
	if (UNieRHeader* Header = Cast<UNieRHeader>(Node(TEXT("Header"))))
	{
		const UNieRRow* CategoryRow = GetCategoryRow(InCategory);
		Header->SetSubtitle(FText::FromString(TEXT("-") + (CategoryRow != nullptr ? CategoryRow->Label.ToString() : FString())), true);
	}
	// The options come in from the right, one after another.
	SlideRowsIn(Options, FVector2D(48.0, 0.0), 0.22f, 0.03f);
}

void UNieRSettingsScreen::OnRowHighlighted(UNieRRow* Row)
{
	// Only the category list chooses the category; in the options it is the page shown.
	if (const int32 Found = CategoryOf(Row); !bInOptions && Found != INDEX_NONE)
	{
		Category = Found;
	}
}

void UNieRSettingsScreen::OnRowClicked(UNieRRow* Row)
{
	if (const int32 Found = CategoryOf(Row); Found != INDEX_NONE)
	{
		ShowOptions(Found);
		return;
	}
	if (Row->Action == TEXT("reset"))
	{
		for (int32 Index = 0; Index < CategoryCount; ++Index)
		{
			for (UNieRRow* Option : GetOptionRows(Index))
			{
				Option->ResetValue();
			}
		}
		return;
	}
	// Confirm on a value to cycle steps it, as Right does.
	if (Row->Kind == ENieRRowKind::Cycle)
	{
		Row->Adjust(1);
	}
}
