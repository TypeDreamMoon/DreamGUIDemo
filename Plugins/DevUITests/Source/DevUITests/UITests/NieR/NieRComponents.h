// The NieR test screens' components. Each is a widget Blueprint built from its own .dui in DUI/UITests/NieR/Components
// whose parent is one of these classes; a screen's .dui places them by the short names DUI/UITests/NieR/NieR_Common.dui
// gives them (`use "…/NieR_Row.dui" as Row`, then `Row Row_Cat_0 { Label = "Language" }`) and sets their properties on
// that line. The classes apply the properties to the parts their .dui named, and own the components' behaviour and
// motion. A component with no behaviour of its own (NieR_Stat) has no class here: its .dui declares its `props`.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "Interaction/UISelectable.h"
#include "NieRMotion.h"
#include "NieRComponents.generated.h"

class UDreamWidget;
class UTexture2D;
class UUIButton;

/** The palette, measured from 1920x1080 reference screenshots (sRGB). */
namespace NieRUI
{
	/** The selected row's bar, header bars, the selected tab. */
	inline const FColor Ink(81, 77, 66);
	/** Text and markers on a row that is not selected. */
	inline const FColor InkText(78, 74, 61);
	/** Text on ink. */
	inline const FColor Paper(245, 241, 207);
	/** The square marker of the selected row. */
	inline const FColor PaperMarker(223, 214, 182);
	/** A row's marker while it is not selected. */
	inline const FColor Mark(74, 70, 61);
	/** A value bar's unlit segments. */
	inline const FColor InkDim(81, 77, 66, 64);
	inline const FColor PaperDim(245, 241, 207, 80);

	/** Where the screens' generated widget classes live. */
	inline const TCHAR* TitleClass = TEXT("/Game/UITests/NieR/UI/NieR_Title.NieR_Title_C");
	inline const TCHAR* MenuClass = TEXT("/Game/UITests/NieR/UI/NieR_Menu.NieR_Menu_C");
	inline const TCHAR* SettingsClass = TEXT("/Game/UITests/NieR/UI/NieR_Settings.NieR_Settings_C");

	/** NieR marks focus with a row's own bar and pointer; -NieRFocusRing brings DreamGUI's focus ring back to look at it. */
	bool UseFocusRing();
}

/** The base of the components and the screens: finding its own parts, painting them, and its motion. */
UCLASS()
class DEVUITESTS_API UNieRComponent : public UDreamTextUserWidget
{
	GENERATED_BODY()

protected:
	/** A node of this widget's own .dui by its id, or null. */
	UDreamWidget* Node(FName Name) const;
	static void SetText(UDreamWidget* Widget, const FText& Text);
	static FText GetText(const UDreamWidget* Widget);
	static void SetShown(UDreamWidget* Widget, bool bShown);
	/** A rect block's body colour, else the visual's colour. */
	static void SetTint(UDreamWidget* Widget, const FColor& Color);

	FNieRMotion Motion;
};

UENUM(BlueprintType)
enum class ENieRRowKind : uint8
{
	/** A label only. */
	Plain,
	/** A label and a count on the right (Count). */
	Count,
	/** A label and a value between arrows, cycled by Left and Right (Values, ValueIndex). */
	Cycle,
	/** A label and a ten-segment bar with its number, stepped by Left and Right (BarValue, BarMax). */
	Bar,
};

UENUM(BlueprintType)
enum class ENieRRowLook : uint8
{
	/** A filled box with a square marker, lines above and below while highlighted: menus and settings. */
	Box,
	/** No box until highlighted, a hexagon marker, no lines: the system menu's lists. */
	List,
};

/**
 * One menu row: the box, its marker, its label and its value, and while it is highlighted the ink bar, the lines above
 * and below opening from the left and the pointer gliding over from the row highlighted before, then idling. It is
 * focusable (its .dui root carries the UIButton); the screen decides which row is highlighted and tells it.
 */
UCLASS()
class DEVUITESTS_API UNieRRow : public UNieRComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Label;
	/** Shown in the hint bar while the row is highlighted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	ENieRRowKind Kind = ENieRRowKind::Plain;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	ENieRRowLook Look = ENieRRowLook::Box;
	/** Count: the text on the right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Count;
	/** Cycle: the values, separated by '|'. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FString Values;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	int32 ValueIndex = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	int32 BarValue = 0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	int32 BarMax = 10;
	/** What clicking the row does, for the screen to read (settings, title, exit, reset...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FName Action;

	// The setters a `.dui` binding writes through -- `for Item in GetItems() { Row { Label <- Item.Label } }` -- since a
	// copy is initialized before its item is written.
	UFUNCTION(BlueprintCallable, Category = "NieR")
	void SetLabel(const FText& InLabel);
	UFUNCTION(BlueprintCallable, Category = "NieR")
	void SetCount(const FText& InCount);
	UFUNCTION(BlueprintCallable, Category = "NieR")
	void SetDescription(const FText& InDescription);
	/** The box while the row is not highlighted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FColor BoxColor = FColor(205, 197, 166);
	/** The pointer's x, from the row's left edge. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	float CursorX = -62.0f;

	/** The row's selection state changed (UUISelectable's states); the screen highlights by it. */
	TMulticastDelegate<void(UNieRRow*, EUISelectableSelectionState)> OnStateChanged;
	/** Enter, Space, the pad's accept or the mouse clicked the row. */
	TMulticastDelegate<void(UNieRRow*)> OnClicked;

	UDreamWidget* GetFocusWidget() const { return GetContentRoot(); }
	bool HasRowFocus() const;
	void SetKeyboardFocusOnRow();
	/** Highlighted or not; turning on, the pointer glides over from InFrom's when it is a row beside this one. */
	void SetHighlighted(bool bInHighlighted, const UNieRRow* InFrom = nullptr);
	bool IsHighlighted() const { return bHighlighted; }
	/** Left (-1) or Right (+1): steps a cycle or a bar. False for a row that takes no adjustment. */
	bool Adjust(int32 Delta);
	/** Back to the value the .dui gave it. */
	void ResetValue();
	FText GetValueText() const;
	UDreamWidget* GetCursorWidget() const { return Cursor.Get(); }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	void HandleState(EUISelectableSelectionState InState, bool bInImmediate);
	void HandleClick();
	void ShowValue();
	void Paint();
	TArray<FString> GetValueList() const;

	TWeakObjectPtr<UDreamWidget> Bar;
	TWeakObjectPtr<UDreamWidget> Mark;
	TWeakObjectPtr<UDreamWidget> Hex;
	TWeakObjectPtr<UDreamWidget> LineT;
	TWeakObjectPtr<UDreamWidget> LineB;
	TWeakObjectPtr<UDreamWidget> Cursor;
	TWeakObjectPtr<UDreamWidget> ArrowL;
	TWeakObjectPtr<UDreamWidget> ArrowR;
	TWeakObjectPtr<UDreamWidget> CycleValue;
	TWeakObjectPtr<UDreamWidget> GaugeValue;
	/** The parts that are text-coloured: paper while highlighted. */
	TArray<TWeakObjectPtr<UDreamWidget>> Inked;
	TArray<TWeakObjectPtr<UDreamWidget>> Segments;
	TWeakObjectPtr<UUISelectable> Selectable;
	bool bHighlighted = false;
	int32 DefaultValueIndex = 0;
	int32 DefaultBarValue = 0;
	float HighlightTime = 0.0f;
	float RowTime = 0.0f;
};

/** One tab of the system menu's nav bar: its icon and label, and while it is open the ink box dropping to the rule. */
UCLASS()
class DEVUITESTS_API UNieRTab : public UNieRComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Label;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	TObjectPtr<UTexture2D> Icon;

	TMulticastDelegate<void(UNieRTab*)> OnClicked;

	/** Open or not; InDirection (-1 from the right, 1 from the left, 0 none) animates opening. */
	void SetOpen(bool bInOpen, int32 InDirection = 0);

protected:
	virtual void NativeOnInitialized() override;
};

/** A page title, wide-set, with its pale shadow down and right, and a smaller subtitle after it. */
UCLASS()
class DEVUITESTS_API UNieRHeader : public UNieRComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Subtitle;

	void SetTitle(const FText& InTitle, const FText& InSubtitle, bool bInAnimate);
	void SetSubtitle(const FText& InSubtitle, bool bInAnimate);
	/** Slides in from the left. */
	void PlayEntrance(float Delay);

protected:
	virtual void NativeOnInitialized() override;
};

/** The description bar at the bottom: the highlighted row's description, and the key prompts its host puts in it. */
UCLASS()
class DEVUITESTS_API UNieRHintBar : public UNieRComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Hint;

	/** The hint, faded up when it changes. */
	void SetHint(const FText& InHint);

protected:
	virtual void NativeOnInitialized() override;
};

/** A key prompt: up to three key caps and what they do. */
UCLASS()
class DEVUITESTS_API UNieRPrompt : public UNieRComponent
{
	GENERATED_BODY()

public:
	/** The keys, separated by spaces ("W S", "Enter"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FString Keys;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	FText Label;

protected:
	virtual void NativeOnInitialized() override;
};

/** A page of the system menu: the list its host fills with rows, and the detail panel that follows the highlighted one. */
UCLASS()
class DEVUITESTS_API UNieRListPage : public UNieRComponent
{
	GENERATED_BODY()

public:
	/** The picture in the detail panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	TObjectPtr<UTexture2D> Icon;
	/** Show "Number Held" from the highlighted row's count (its .dui: `if bShowCount { Text DetailCount ... }`). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NieR")
	bool bShowCount = false;

	/** The detail panel shows InRow. */
	void ShowRow(const UNieRRow* InRow);

protected:
	virtual void NativeOnInitialized() override;
};

/** One entry of a list a screen's .dui repeats with `for` -- what a row shows of it. */
UCLASS(BlueprintType)
class DEVUITESTS_API UNieRListItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "NieR")
	FText Label;
	UPROPERTY(BlueprintReadOnly, Category = "NieR")
	FText Count;
	UPROPERTY(BlueprintReadOnly, Category = "NieR")
	FText Description;
};
