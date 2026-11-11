// The Detroit test's widgets: each a widget Blueprint built from its .dui in DUI/UITests/Detroit, whose parent is one of
// these classes. The class finds the parts its .dui named, fills them and plays their motion; the game (DetroitGame.h)
// tells it what is happening. A widget with no behaviour (the jacket print, the wall writing) has the base class.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "DreamTweener.h"
#include "DetroitWidgets.generated.h"

class UDreamWidget;
class UMaterialInstanceDynamic;

/**
 * Render-transform motion for the Detroit widgets: their layout never moves, so nothing a panel lays out is written
 * back. Offsets are design pixels with +y down; a move started on a widget first stops the one still running on it.
 */
struct FDetroitMotion
{
	/** From Offset (and transparent) to where layout put it, after Delay. */
	void SlideIn(UDreamWidget* Widget, FVector2D Offset, float Duration, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	/** To Opacity from wherever it is. */
	void FadeTo(UDreamWidget* Widget, float Opacity, float Duration, float Delay = 0.0f);
	/** From nothing to full width (Y = false) or height about its render pivot. */
	void Grow(UDreamWidget* Widget, bool bVertical, float Duration, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	/** From Scale (uniform, in the plane) to one, fading in. */
	void Pop(UDreamWidget* Widget, float FromScale, float Duration, float Delay = 0.0f);
	/** To a uniform in-plane scale. */
	void ScaleTo(UDreamWidget* Widget, float Scale, float Duration, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	/** To an offset in design pixels (+y down). */
	void OffsetTo(UDreamWidget* Widget, FVector2D Offset, float Duration, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	void Stop(UDreamWidget* Widget);
	/** Stopped, at rest, and at Opacity. */
	void Settle(UDreamWidget* Widget, float Opacity = 1.0f);

	static void SetOffset(UDreamWidget* Widget, FVector2D Offset);
	/** In-plane scale: X across, Y up. */
	static void SetPlaneScale(UDreamWidget* Widget, FVector2D Scale);

private:
	void Track(UDreamWidget* Widget, UDreamTweener* Tweener);

	TMap<TWeakObjectPtr<UDreamWidget>, TArray<TWeakObjectPtr<UDreamTweener>>> Running;
};

/** The base of the Detroit widgets: finding its own parts, painting them, and its motion. */
UCLASS()
class DEVUITESTS_API UDetroitWidget : public UDreamTextUserWidget
{
	GENERATED_BODY()

protected:
	/** A node of this widget's own .dui by its id, or null. */
	UDreamWidget* Node(FName Name) const;
	static void SetText(UDreamWidget* Widget, const FText& Text);
	static void SetShown(UDreamWidget* Widget, bool bShown);
	/** A rect block's body colour, else the visual's colour. */
	static void SetTint(UDreamWidget* Widget, const FColor& Color);
	/** Gives an Image a dynamic instance of the material its brush holds, and returns the instance. */
	UMaterialInstanceDynamic* MakeDynamicMaterial(UDreamWidget* ImageWidget);

	FDetroitMotion Motion;
	/** Seconds since the widget was made, on the UI clock. */
	float Clock = 0.0f;

	virtual void NativeOnTick(float DeltaTime) override;
};

// ---------------------------------------------------------------------------------------------------------------------

/**
 * The loading screen, as the game's runs (Detroit_Loading.dui): the ring (M_DBH_LoadingRing) over the sliding panes
 * (M_DBH_LoadingBackdrop), the percentage under its short line, and a timeline of small things coming and going -- a
 * process label typed out of the ring's left edge, LOADING glitching in at its lower right and settling, a second label
 * at its right edge, a strip of triangles flipping at the left and later at the right, blocks of tiny text -- while
 * the ring and what hangs on it drift slowly right and back. Every part's state is worked out from the time since Play
 * on every tick, so nothing is left half-way by a frame that came late. Play runs the load; Finished fires once it has
 * faded away.
 */
UCLASS()
class DEVUITESTS_API UDetroitLoadingScreen : public UDetroitWidget
{
	GENERATED_BODY()

public:
	void Play(float InDuration);
	TMulticastDelegate<void()> OnFinished;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	/** One slot's paint at one moment of a triangle strip's flip: a key per slot, '.' empty, 'n' navy, 'm' mid, 'p' pale. */
	struct FStripKey
	{
		float Since;
		const TCHAR* Slots;
	};

	float ProgressAt(float InTime) const;
	/** Where the ring has drifted to, in design pixels (+y down). */
	FVector2D DriftAt(float InTime) const;
	void PlayLabelA(float T);
	void PlayLabelB(float T);
	void PlayLoadingWord(float T);
	void PlayStrip(const TCHAR* Prefix, const FStripKey* Keys, int32 KeyCount, float Since, float Opacity);
	void PlayBlocks(float T);
	/** Fresh lines of tiny text for a block, Lines long. */
	FString BlockText(int32 Lines, int32 Columns);

	UDreamWidget* Part(FName Name);
	void SetOpacity(FName Name, float Opacity);
	/** The first Count characters of Full; a text is only set when it changes. */
	void SetTyped(FName Name, const FString& Full, int32 Count);
	void SetWidthScale(FName Name, float Scale);
	void SetPartTint(FName Name, const FColor& Color);

	TMap<FName, TWeakObjectPtr<UDreamWidget>> Parts;
	TMap<FName, FString> Texts;
	TMap<FName, FColor> Tints;
	TWeakObjectPtr<UMaterialInstanceDynamic> RingMaterial;
	float Duration = 8.5f;
	float Time = -1.0f;
	float NextCode = 0.0f;
	TMap<FName, bool> BlockShown;
	bool bFading = false;
	bool bFinished = false;
	FRandomStream Random = FRandomStream(800);
};

/**
 * The screen HUD of the investigation: "x/5 CLUES TO ANALYZE" with its bar, shown in analysis mode and for a moment after
 * each find; the key prompts at the bottom right; and the analysis mode's frame.
 */
UCLASS()
class DEVUITESTS_API UDetroitHUD : public UDetroitWidget
{
	GENERATED_BODY()

public:
	void SetClues(int32 Found, int32 Total);
	void SetAnalysisMode(bool bOn);
	/** A clue is in reach: the analyze prompt shows. */
	void SetCanAnalyze(bool bCan);
	/** Show the counter for a while even outside analysis mode. */
	void HoldCounter(float Seconds);
	/** Fade the whole HUD in (after the loading screen). */
	void PlayEntrance();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	void UpdateCounterShown();

	TWeakObjectPtr<UDreamWidget> Counter;
	TWeakObjectPtr<UDreamWidget> CounterNum;
	TWeakObjectPtr<UDreamWidget> CounterLabelA;
	TWeakObjectPtr<UDreamWidget> CounterLabelB;
	TWeakObjectPtr<UDreamWidget> CounterFill;
	TWeakObjectPtr<UDreamWidget> CounterTrack;
	TWeakObjectPtr<UDreamWidget> CounterTick;
	TWeakObjectPtr<UDreamWidget> PromptAnalyze;
	TWeakObjectPtr<UDreamWidget> PromptModeText;
	TWeakObjectPtr<UDreamWidget> ModeFrame;
	TArray<TWeakObjectPtr<UDreamWidget>> TintedByProgress;
	TArray<TWeakObjectPtr<UDreamWidget>> CounterLines;
	float ShownFill = 0.0f;
	float TargetFill = 0.0f;
	float CounterHold = 0.0f;
	bool bAnalysisMode = false;
	bool bCounterShown = false;
	bool bCanAnalyze = false;
	bool bComplete = false;
};

UENUM(BlueprintType)
enum class EDetroitClueKind : uint8
{
	/** A tag: an unlocked padlock and a two-line title on a plate (RELIGIOUS / OFFERING?). */
	Tag,
	/** A tag, and the HIT MARKS analysis panel beside it. */
	Panel,
};

/**
 * A clue's label in the world (Detroit_ClueTag.dui), centred on the clue: in analysis mode a yellow marker until the clue
 * is analyzed, then while it is analyzed the staged readout (SYNC IN PROGRESS, COLLECTING DATA, PROCESSING DATA), and
 * once it is, the unlocked padlock with the title on its plate and the measurement lines around it.
 */
UCLASS()
class DEVUITESTS_API UDetroitClueTag : public UDetroitWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	FText TitleA;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	FText TitleB;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	FText Suffix;

	void SetTitle(const FText& InA, const FText& InB, const FText& InSuffix);
	/** The marker, for a clue not analyzed yet; bInReach brightens it and shows its key. */
	void ShowMarker(bool bShow, bool bInReach);
	void BeginAnalysis();
	/** 0..1: the readout's number, stage and bar. */
	void SetAnalysisProgress(float Progress);
	/** The analysis is done: the readout goes and the tag comes. */
	void PlayUnlock();
	/** The tag of an analyzed clue, shown or not (analysis mode, or a while after the unlock). */
	void SetTagShown(bool bShown);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	TWeakObjectPtr<UDreamWidget> Marker;
	TWeakObjectPtr<UDreamWidget> MarkerPulse;
	TWeakObjectPtr<UDreamWidget> MarkerKey;
	TWeakObjectPtr<UDreamWidget> Readout;
	TWeakObjectPtr<UDreamWidget> ReadPct;
	TWeakObjectPtr<UDreamWidget> ReadStage;
	TWeakObjectPtr<UDreamWidget> ReadFill;
	TWeakObjectPtr<UDreamWidget> Tag;
	TWeakObjectPtr<UDreamWidget> Lock;
	TWeakObjectPtr<UDreamWidget> Shackle;
	TWeakObjectPtr<UDreamWidget> PlateBack;
	TWeakObjectPtr<UDreamWidget> Plate;
	TWeakObjectPtr<UDreamWidget> TitleAText;
	TWeakObjectPtr<UDreamWidget> TitleBText;
	TWeakObjectPtr<UDreamWidget> SuffixText;
	TArray<TWeakObjectPtr<UDreamWidget>> Measures;
	TArray<TWeakObjectPtr<UDreamWidget>> Numbers;
	TArray<float> NumberValues;
	float UnlockTime = -1.0f;
	bool bMarkerShown = false;
	bool bTagShown = false;
	bool bUnlocked = false;
	int32 Stage = -1;
};

/**
 * The HIT MARKS analysis panel (Detroit_HitMarks.dui): the icon box, the title and its findings, the reconstruction
 * picture (M_DBH_Hologram, wiped in) with its label, the yellow corner brackets, the dashed guides and their numbers.
 */
UCLASS()
class DEVUITESTS_API UDetroitAnalysisPanel : public UDetroitWidget
{
	GENERATED_BODY()

public:
	void PlayShow();
	void SetPanelShown(bool bShown);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	TWeakObjectPtr<UDreamWidget> Content;
	TWeakObjectPtr<UDreamWidget> IconBox;
	TWeakObjectPtr<UDreamWidget> Title;
	TWeakObjectPtr<UDreamWidget> Body;
	TWeakObjectPtr<UDreamWidget> HoloFrame;
	TWeakObjectPtr<UDreamWidget> HoloLabel;
	TArray<TWeakObjectPtr<UDreamWidget>> Brackets;
	TArray<TWeakObjectPtr<UDreamWidget>> Guides;
	TArray<TWeakObjectPtr<UDreamWidget>> Numbers;
	TArray<float> NumberValues;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Hologram;
	float ShowTime = -1.0f;
	bool bShown = false;
};

UENUM(BlueprintType)
enum class EDetroitObjectiveState : uint8
{
	/** Not known yet: a bracket frame, a padlock and "[...]". */
	Locked,
	/** The one to do: white on a dark plate. */
	Active,
	/** Done: dimmed, with a check. */
	Completed,
};

/** One objective (Detroit_ObjectiveRow.dui): "SEARCH FOR" / "BLUE BLOOD TRACES", in one of three states. */
UCLASS()
class DEVUITESTS_API UDetroitObjectiveRow : public UDetroitWidget
{
	GENERATED_BODY()

public:
	/** The first line's bold word, the rest of the first line, and the bold second line. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	FText Lead;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	FText LeadRest;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	FText Line2;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Detroit")
	EDetroitObjectiveState State = EDetroitObjectiveState::Locked;

	void SetState(EDetroitObjectiveState InState, bool bAnimate);
	EDetroitObjectiveState GetState() const { return State; }
	/** The row's part of the panel's entrance. */
	void PlayEnter(float Delay);

protected:
	virtual void NativeOnInitialized() override;

private:
	void ApplyState();

	TWeakObjectPtr<UDreamWidget> Box;
	TWeakObjectPtr<UDreamWidget> Words;
	TWeakObjectPtr<UDreamWidget> LeadText;
	TWeakObjectPtr<UDreamWidget> LeadRestText;
	TWeakObjectPtr<UDreamWidget> Line2Text;
	TWeakObjectPtr<UDreamWidget> Locked;
	TWeakObjectPtr<UDreamWidget> Check;
	TArray<TWeakObjectPtr<UDreamWidget>> Corners;
};

/**
 * The objectives in the room (Detroit_Objectives.dui): the white FIND DEVIANT header with its bars and brackets, the rows
 * under it. Show plays the entrance and fades it away after Hold seconds.
 */
UCLASS()
class DEVUITESTS_API UDetroitObjectives : public UDetroitWidget
{
	GENERATED_BODY()

public:
	void Show(float Hold);
	void Hide();
	void SetObjective(int32 Index, EDetroitObjectiveState State, bool bAnimate);
	bool IsShowing() const { return bShowing; }

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	TWeakObjectPtr<UDreamWidget> Content;
	TWeakObjectPtr<UDreamWidget> HeaderBox;
	TWeakObjectPtr<UDreamWidget> HeaderWords;
	TWeakObjectPtr<UDreamWidget> Spine;
	TArray<TWeakObjectPtr<UDreamWidget>> StripesL;
	TArray<TWeakObjectPtr<UDreamWidget>> StripesR;
	TArray<TWeakObjectPtr<UDreamWidget>> Tabs;
	TArray<TWeakObjectPtr<UDreamWidget>> Brackets;
	TArray<TWeakObjectPtr<UDetroitObjectiveRow>> Rows;
	float HoldLeft = 0.0f;
	bool bShowing = false;
};
