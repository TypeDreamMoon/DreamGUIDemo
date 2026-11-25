// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "Interaction/UISelectable.h"
#include "NocturneOptionRow.generated.h"

class UDreamWidget;
class UDreamSlider;

UENUM(BlueprintType)
enum class ENocturneOptionKind : uint8
{
	/** A value between arrows, stepped by Left / Right or a click (Values, ValueIndex). */
	Selector,
	/** A slider with its number (Value between Min and Max, Step, Unit). */
	Slider,
	/** Two to five boxes, one lit (Values, ValueIndex); a box takes a click. */
	Segment,
	/** Off / On: a two-box segment. */
	Toggle,
	/** A key binding: click, then press the key (Values holds the key's name). */
	Key,
	/** An action run on click (Action), its label on the right (Values). */
	Action,
};

/**
 * One row of the NOCTURNE//OS settings screen: its Chinese label and English tag on the left, its control on the right,
 * and while it is highlighted the cyan accent bar, the lit background and brighter text. Its .dui is
 * DUI/Nocturne/Components/Nocturne_OptionRow.dui; the parts are found there by id. The row is focusable (its root carries
 * the UIButton); the screen decides which row is highlighted and tells it.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneOptionRow : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	FText Label;
	/** The English tag after the label, upper case. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	FString En;
	/** Shown in the detail panel while the row is highlighted. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	FText Description;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	ENocturneOptionKind Kind = ENocturneOptionKind::Selector;
	/** Selector / Segment: the values, '|' between them. Key: the key's name. Action: the label on the right. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	FString Values;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	int32 ValueIndex = 0;
	/** Slider: the value, in the units shown. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	float Value = 50.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	float Min = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	float Max = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	float Step = 1.0f;
	/** Slider: what follows the number ("%", "FPS", "°"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	FString Unit;
	/** How much the option costs in performance, 0 to 5: the detail panel's meter. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	int32 Impact = 0;
	/** A change needs a restart: the detail panel says so. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	bool bNeedsRestart = false;
	/** Action: what the screen does on click (reset, calibrate...). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nocturne")
	FName Action;

	/** The row's selection state changed; the screen highlights by it. */
	TMulticastDelegate<void(UNocturneOptionRow*, EUISelectableSelectionState)> OnStateChanged;
	/** Enter, Space, the pad's accept or the mouse clicked the row. */
	TMulticastDelegate<void(UNocturneOptionRow*)> OnClicked;
	/** The value changed, by key, click or drag. */
	TMulticastDelegate<void(UNocturneOptionRow*)> OnValueChanged;

	UDreamWidget* GetFocusWidget() const { return GetContentRoot(); }
	bool HasRowFocus() const;
	void SetHighlighted(bool bInHighlighted);
	bool IsHighlighted() const { return bHighlighted; }
	/** Left (-1) or Right (+1). False for a row that takes no adjustment. */
	bool Adjust(int32 Delta);
	/** Back to the value the .dui gave it. */
	void ResetValue();
	bool IsChanged() const;
	FText GetValueText() const;
	FText GetDefaultValueText() const;

	/** Key: waiting for the key to bind. The screen hands the next key to TakeKey. */
	bool IsListening() const { return bListening; }
	void SetListening(bool bInListening);
	void TakeKey(const FKey& InKey);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	void HandleState(EUISelectableSelectionState InState, bool bInImmediate);
	void HandleClick();
	void HandleSegmentClick(int32 Index);
	UFUNCTION()
	void HandleSliderValue(float InValue);
	void ShowValue();
	void Paint();
	TArray<FString> GetValueList() const;
	FText FormatSliderValue(float InValue) const;
	void SetValueInternal(int32 InIndex, float InValue, bool bFromSlider);

	TWeakObjectPtr<UDreamWidget> Back;
	TWeakObjectPtr<UDreamWidget> Accent;
	TWeakObjectPtr<UDreamWidget> LabelText;
	TWeakObjectPtr<UDreamWidget> EnText;
	TWeakObjectPtr<UDreamWidget> ArrowL;
	TWeakObjectPtr<UDreamWidget> ArrowR;
	TWeakObjectPtr<UDreamWidget> SelValue;
	TWeakObjectPtr<UDreamWidget> SliderValue;
	TWeakObjectPtr<UDreamSlider> SliderCtl;
	TWeakObjectPtr<UDreamWidget> Segments;
	TWeakObjectPtr<UDreamWidget> KeyCap;
	TWeakObjectPtr<UDreamWidget> KeyText;
	TWeakObjectPtr<UDreamWidget> ActionText;
	TArray<TWeakObjectPtr<UDreamWidget>> SegBoxes;
	TArray<TWeakObjectPtr<UDreamWidget>> SegTexts;
	TWeakObjectPtr<UUISelectable> Selectable;

	bool bHighlighted = false;
	bool bHovered = false;
	bool bListening = false;
	bool bSettingSlider = false;
	int32 DefaultIndex = 0;
	float DefaultValue = 0.0f;
	FString DefaultKey;
	float RowTime = 0.0f;
};
