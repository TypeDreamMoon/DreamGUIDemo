#pragma once

#include "CoreMinimal.h"
#include "Interaction/DreamUITabSwitchTarget.h"
#include "NieRScreen.h"
#include "NieRSettingsScreen.generated.h"

/**
 * The settings screen: a list of categories, each opening its list of options -- a value to cycle, a ten-step bar, or an
 * action. Left/Right change the highlighted option (its row does, UNieRRow::Adjust), Q/E switch the category in both
 * lists, Back closes the options and then the screen. A row whose Action is `reset` puts every option back.
 * Nodes: Categories (one row per category, in order), OptPage_<c> (that category's option rows), Header (UNieRHeader).
 */
UCLASS()
class DEVUITESTS_API UNieRSettingsScreen : public UNieRScreen, public IDreamUITabSwitchTarget
{
	GENERATED_BODY()

public:
	virtual bool HandleBack() override;

	// IDreamUITabSwitchTarget
	virtual bool CanSwitchTab(int32 InUserIndex) const override;
	virtual bool SwitchTab(int32 InUserIndex, int32 InDelta) override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnRowClicked(UNieRRow* Row) override;
	virtual void OnRowHighlighted(UNieRRow* Row) override;
	virtual UNieRRow* GetEntryRow() const override;
	virtual void PlayEntrance() override;

private:
	void ShowCategories(bool bInFocus);
	void ShowOptions(int32 InCategory);
	UNieRRow* GetCategoryRow(int32 InCategory) const;
	TArray<UNieRRow*> GetOptionRows(int32 InCategory) const;
	/** The category a row of the category list stands for, or INDEX_NONE. */
	/** A category row's index, by its place in the Categories list (the rows are a `rows` table: their ids are made). */
	int32 CategoryOf(const UNieRRow* Row) const;

	int32 CategoryCount = 0;
	int32 Category = 0;
	bool bInOptions = false;
};
