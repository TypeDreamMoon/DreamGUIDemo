#pragma once

#include "CoreMinimal.h"
#include "Interaction/DreamUITabSwitchTarget.h"
#include "NieRScreen.h"
#include "NieRMenuScreen.generated.h"

class UNieRHeader;
class UNieRListPage;
class UNieRTab;

/**
 * The system menu: a nav bar of seven tabs (MAP QUESTS ITEMS WEAPONS SKILLS INTEL SYSTEM) over one list page each. Q
 * and E (and LB/RB) switch tabs through DreamGUI's tab-switch keys -- this widget is the player's
 * IDreamUITabSwitchTarget -- and a click on a tab opens it. On opening a tab its ink box drops to the rule, its page
 * slides in from the side it was switched to and the title changes; on entering, the tabs drop in one after another.
 * A row's Action says what it does: settings, title (back to the start screen), exit.
 * Nodes: Tabs (its children are the UNieRTab tabs, in order), Page_<i> (UNieRListPage), Header (UNieRHeader).
 */
UCLASS()
class DEVUITESTS_API UNieRMenuScreen : public UNieRScreen, public IDreamUITabSwitchTarget
{
	GENERATED_BODY()

public:
	// IDreamUITabSwitchTarget
	virtual bool CanSwitchTab(int32 InUserIndex) const override;
	virtual bool SwitchTab(int32 InUserIndex, int32 InDelta) override;

	static constexpr int32 TabCount = 7;

	/** The ITEMS page's entries: its .dui repeats one row per item (`for Item in GetItems() { Row { ... } }`). */
	UFUNCTION(BlueprintCallable, Category = "NieR")
	TArray<UObject*> GetItems() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual void OnRowClicked(UNieRRow* Row) override;
	virtual void OnRowHighlighted(UNieRRow* Row) override;
	virtual UNieRRow* GetEntryRow() const override;
	virtual void PlayEntrance() override;

private:
	/** InDirection: -1 came from the right of it (Q), 1 from the left (E), 0 no slide. */
	void SelectTab(int32 InIndex, bool bInFocusFirstRow, int32 InDirection);
	UNieRTab* GetTab(int32 InIndex) const;
	UNieRListPage* GetPage(int32 InIndex) const;

	int32 ActiveTab = 2;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Items;
};
