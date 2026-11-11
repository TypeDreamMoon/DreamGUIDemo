// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NocturneMenu.generated.h"

class UCameraComponent;
class UDreamUserWidget;
class UDreamWorldWidgetComponent;
class ANocturneSpace;
class UNocturneMainMenu;
class UNocturneSettingsScreen;
class UNocturneStarLabel;
class UNocturneStarScreen;
class UNocturneTitleScreen;

/**
 * NOCTURNE//OS's front end: the title, and the main menu that is the constellation itself.
 *
 * The title (optional) looks up at the moon; any key wakes the system, the camera turns down to the stars and the
 * constellation's lines draw in one by one. On the map each of the six stars (NocturneStars.h) carries a label in the
 * world, turned to the camera; the screen keeps a top bar, a route list in the lower left and the key hints
 * (UNocturneMainMenu). Pointing at a star's label or its entry puts it in focus. Choosing a star flies the camera to it,
 * the stars streaking, and opens its screen there (a UNocturneStarScreen, named in the star table); Back flies home.
 * The Depart star's screen can launch: the camera dives past the star and the menu hands over (OnLaunched).
 *
 * The camera is the menu's own: it drifts and leans a little after the pointer. Everything runs on the actor's clock,
 * so a take on a fixed time step repeats exactly.
 */
UCLASS()
class DREAMGUIDEMO_API ANocturneMenu : public AActor
{
	GENERATED_BODY()

public:
	ANocturneMenu();

	void Setup(ANocturneSpace* InSpace) { Space = InSpace; }
	/** Open on the title rather than straight on the map (set before BeginPlay). */
	void SetStartAtTitle(bool bInStartAtTitle) { bStartAtTitle = bInStartAtTitle; }

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** The title's "any key": the system wakes into the map. */
	void PressAnyKey();
	/** Put star InIndex in focus (the map only). */
	void FocusStar(int32 InIndex);
	/** Choose star InIndex: fly to it if it is open, else refuse. */
	void ChooseStar(int32 InIndex);
	/** From a star's screen back to the map. */
	void Back();
	/** From the Depart screen: leave the menu with save slot InSlot. */
	void Launch(int32 InSlot);
	/** -1 to 1 on each axis: where the pointer is on the screen. */
	void SetParallaxTarget(const FVector2D& InTarget) { ParallaxTarget = InTarget; }

	/** The camera dived and the menu is gone; the slot launched with. */
	TMulticastDelegate<void(int32)> OnLaunched;

	/** The map is up and taking the pointer: not flying, not at a star. */
	bool IsOnMap() const;
	UCameraComponent* GetCamera() const { return Camera; }
	/** How fast the camera moved in the last frame, units a second. */
	double GetCameraSpeed() const { return CameraSpeed; }
	UNocturneStarLabel* GetStarLabel(int32 InIndex) const;
	UNocturneMainMenu* GetMenuWidget() const { return MenuWidget; }
	UNocturneTitleScreen* GetTitle() const { return Title; }
	/** The star's screen that is open, if any. */
	UNocturneStarScreen* GetScreen() const { return Screen; }
	UNocturneSettingsScreen* GetSettings() const;

private:
	enum class EMenuState : uint8 { Title, Waking, Overview, Flying, AtStar, Returning, Launching, Launched };

	struct FPose
	{
		FVector Location = FVector::ZeroVector;
		FVector Look = FVector::ForwardVector;
	};

	FPose TitlePose() const;
	FPose OverviewPose() const;
	FPose VisitPose(int32 InIndex) const;
	FPose FlightPose(const FPose& From, const FPose& To, double Alpha, bool bOutbound) const;
	void UpdateCamera(float DeltaSeconds);
	void UpdateLabels();
	void UpdateFocus(float DeltaSeconds);
	void UpdateLines();
	void BindLabels();
	void FadeLabels(bool bIn, float InDelay);
	void ApplyFocus();
	void OpenStarScreen();
	void EnterMap();
	/** Whether the map takes the pointer and the keys: not while it is still building in. */
	bool AcceptsInput() const;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UDreamWorldWidgetComponent>> Labels;

	UPROPERTY(Transient)
	TObjectPtr<ANocturneSpace> Space;

	// Soft references, loaded in BeginPlay: see ANocturneSpace's constructor for why a widget Blueprint is never loaded
	// from one. The star screens are named in the star table and loaded when first opened.
	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> LabelClass;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> MenuClass;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> TitleClass;

	UPROPERTY(Transient)
	TObjectPtr<UNocturneMainMenu> MenuWidget;

	UPROPERTY(Transient)
	TObjectPtr<UNocturneTitleScreen> Title;

	UPROPERTY(Transient)
	TObjectPtr<UNocturneStarScreen> Screen;

	bool bStartAtTitle = false;
	EMenuState State = EMenuState::Overview;
	/** The open star first: Enter at once goes somewhere. */
	int32 Focused = 1;
	int32 Visiting = INDEX_NONE;
	int32 LaunchSlot = 0;
	float MenuTime = 0.0f;
	/** When the map came up: the push-in at the start and the first input are timed from it. */
	float MapFrom = 0.0f;
	bool bPushIn = true;

	/** The flight under way: when it began (it may begin later than it was asked for), where from and where to. */
	float FlightStart = 0.0f;
	float ArrivedAt = 0.0f;
	FPose FlightFrom;
	FPose FlightTo;
	bool bFlightCaptured = false;
	bool bScreenOpened = false;
	bool bChromeBack = false;
	/** The focused label opens only once the labels have built in. */
	bool bFocusRevealed = false;
	float RemoveScreenAt = -1.0f;
	float RemoveTitleAt = -1.0f;
	/** When the constellation's lines start drawing in (the wake); negative while they are all drawn. */
	float LinesFrom = -1.0f;

	/** The labels' fade: from, to, when it starts, how long. */
	float LabelFadeFrom = 1.0f;
	float LabelFadeTo = 1.0f;
	float LabelFadeStart = 0.0f;
	float LabelFadeSeconds = 0.2f;

	FPose Current;
	FVector LastLocation = FVector::ZeroVector;
	bool bHaveLast = false;
	double CameraSpeed = 0.0;
	FVector2D ParallaxTarget = FVector2D::ZeroVector;
	FVector2D Parallax = FVector2D::ZeroVector;
	TArray<float> FocusAmount;
	TArray<bool> LabelBound;
	bool bKeyboardFocusPending = true;
};
