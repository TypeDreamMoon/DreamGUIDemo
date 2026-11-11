// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NocturneDirector.generated.h"

class UCameraComponent;
class ANocturneSpace;
class UDreamUserWidget;
class UDreamWidget;
class UDreamStandaloneInputModule;
class UDreamPixelSort;
class UNocturneSettingsScreen;
class ANocturneMenu;
class ANocturneVoyage;
class ANocturneDeck;
class UNocturneStarScreen;

/** Where a pointer key's spot is. */
enum class ENocturneTarget : uint8
{
	/** Spot is a viewport position in pixels. */
	Viewport,
	/** A widget of the world-space test panel, by its .dui id. */
	Panel,
	/** A part of the settings screen, by its .dui id. */
	Screen,
	/** A row of the settings screen, by page and index. */
	ScreenRow,
	/** A part of a star's label in the star map, by star (Page) and .dui id. */
	StarLabel,
	/** A part of the star map's screen chrome, by its .dui id. */
	Menu,
};

/** A point the director's pointer travels to, and the left button's state from then on. */
struct FNocturnePointerKey
{
	/** Seconds into the shot. */
	float Time = 0.0f;
	ENocturneTarget Target = ENocturneTarget::Viewport;
	FString Widget;
	int32 Page = 0;
	int32 Row = 0;
	/** Across the widget's rect, 0 to 1 from its left-bottom corner; or viewport pixels for a Viewport key. */
	FVector2D Spot = FVector2D(0.5, 0.5);
	bool bPressed = false;

	static FNocturnePointerKey At(float InTime, FVector2D InPixels)
	{
		FNocturnePointerKey Key;
		Key.Time = InTime;
		Key.Spot = InPixels;
		return Key;
	}
	static FNocturnePointerKey OnPanel(float InTime, const TCHAR* InWidget, FVector2D InSpot, bool bInPressed = false)
	{
		FNocturnePointerKey Key;
		Key.Time = InTime;
		Key.Target = ENocturneTarget::Panel;
		Key.Widget = InWidget;
		Key.Spot = InSpot;
		Key.bPressed = bInPressed;
		return Key;
	}
	static FNocturnePointerKey OnScreen(float InTime, const TCHAR* InWidget, FVector2D InSpot, bool bInPressed = false)
	{
		FNocturnePointerKey Key = OnPanel(InTime, InWidget, InSpot, bInPressed);
		Key.Target = ENocturneTarget::Screen;
		return Key;
	}
	static FNocturnePointerKey OnRow(float InTime, int32 InPage, int32 InRow, FVector2D InSpot, bool bInPressed = false)
	{
		FNocturnePointerKey Key;
		Key.Time = InTime;
		Key.Target = ENocturneTarget::ScreenRow;
		Key.Page = InPage;
		Key.Row = InRow;
		Key.Spot = InSpot;
		Key.bPressed = bInPressed;
		return Key;
	}
	/** Over a star's label: the spot is across its hit area. */
	static FNocturnePointerKey OnStar(float InTime, int32 InStar, FVector2D InSpot, bool bInPressed = false)
	{
		FNocturnePointerKey Key = OnPanel(InTime, TEXT("HitArea"), InSpot, bInPressed);
		Key.Target = ENocturneTarget::StarLabel;
		Key.Page = InStar;
		return Key;
	}
	static FNocturnePointerKey OnMenu(float InTime, const TCHAR* InWidget, FVector2D InSpot, bool bInPressed = false)
	{
		FNocturnePointerKey Key = OnPanel(InTime, InWidget, InSpot, bInPressed);
		Key.Target = ENocturneTarget::Menu;
		return Key;
	}
};

/** Something the director does once, at a time into the shot. */
struct FNocturneEvent
{
	float Time = 0.0f;
	TFunction<void()> Do;
	bool bDone = false;
};

/** A label that comes in as scrambled glyphs and resolves left to right. */
struct FNocturneScramble
{
	FString Widget;
	FString Final;
	/** Seconds after the panel's entrance begins. */
	float Start = 0.0f;
	float Duration = 0.3f;
};

/**
 * Plays a shot of the demo video: flies the camera, brings the panel in, and drives a pointer of its own through
 * DreamGUI's input module -- the module's mouse-override seam, the one the virtual cursor uses -- so hover, press,
 * drag and click go down exactly the path a real mouse takes. It draws its own cursor on the screen.
 *
 * The panel's entrance: the plate cuts in under a pixel sort that settles in a quarter of a second, the panel's own
 * Reveal timeline staggers its rows in, English labels decode from scrambled glyphs, and the volume readout counts up.
 * While the camera flies, a screen-wide pixel sort streaks the stars in proportion to its speed.
 *
 * Everything is a function of the time since the shot began, so a take recorded on a fixed time step
 * (-benchmark -fps=60) is the same every time.
 *
 * -DemoShot=<Name> picks the shot: Test (the world-space panel), Settings, SettingsPages (a layout check of every page),
 * Menu (the star map: focus, a refused star, a flight to Settings and back). A key whose target is gone (a label faded
 * out while the camera flies) holds the pointer where it last was. -DemoMeasure logs frame and GPU times, then quits.
 */
UCLASS()
class DREAMGUIDEMO_API ANocturneDirector : public AActor
{
	GENERATED_BODY()

public:
	ANocturneDirector();

	void Setup(ANocturneSpace* InSpace);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	FVector CameraPositionAt(float T) const;
	void UpdateCamera(float T);
	void UpdateScreenFX(float T);
	void UpdatePanel(float T);
	void UpdatePointer(float T);
	/** Play: the real mouse, keys and pad drive everything; the director only draws the cursor where the mouse is,
	 *  leans the panels toward it, and wakes the title on any key or click. */
	void UpdatePlay();
	void UpdateReadouts(float T);
	void Measure(float DeltaSeconds);

	void SetWidgetText(const FString& InWidget, const FString& InText, const FColor* InColor = nullptr) const;
	UDreamPixelSort* FindPixelSort(UDreamWidget* InTree, const FString& InName) const;
	bool ResolveKey(const FNocturnePointerKey& Key, FVector2D& OutViewport) const;
	bool ProjectToViewport(const FVector& InWorld, FVector2D& OutViewport) const;
	FVector2D GetViewportSize() const;
	UDreamStandaloneInputModule* GetInputModule();
	void EnsureCursor();
	void PlaceCursor(const FVector2D& InViewport);

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(Transient)
	TObjectPtr<ANocturneSpace> Space;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> CursorClass;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> ScreenFXClass;
	TSoftClassPtr<UDreamUserWidget> LensClass;

	UPROPERTY(Transient)
	TSubclassOf<UDreamUserWidget> LoadedCursorClass;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> SettingsClass;

	UPROPERTY(Transient)
	TObjectPtr<UNocturneSettingsScreen> Settings;

	/** A feature shot's screen (WBP_Nocturne_<shot>), open over the map. */
	UPROPERTY(Transient)
	TObjectPtr<UDreamWidget> FeatureScreen;

	UPROPERTY(Transient)
	TObjectPtr<ANocturneMenu> Menu;

	UPROPERTY(Transient)
	TObjectPtr<ANocturneVoyage> Voyage;

	/** The world shot's deck: a curved screen, a marker behind a wall, a panel the engine draws. */
	UPROPERTY(Transient)
	TObjectPtr<ANocturneDeck> Deck;

	TArray<FNocturneEvent> Events;
	bool IsSettingsShot() const { return ShotName.StartsWith(TEXT("Settings")); }
	/** A shot of the menu and what is behind its stars (Menu, Title, Archive, Wheel, Music, Log, Depart, Full, Tour), or the HUD alone. */
	bool IsMenuShot() const;
	bool IsHudShot() const { return ShotName == TEXT("Hud"); }
	/** No shot at all (the demo map played in the editor or as the game): NOCTURNE//OS to be used, from its title. */
	bool IsPlayShot() const { return ShotName == TEXT("Play"); }
	/** A feature screen over the star map, playing itself: LayerFx (layer effects), Text, Perf, and the CardOpen / CardEnd cards. */
	bool IsFeatureShot() const
	{
		return ShotName == TEXT("LayerFx") || ShotName == TEXT("Text") || ShotName == TEXT("Perf") || ShotName == TEXT("CardOpen")
			|| ShotName == TEXT("CardEnd");
	}
	/** A screen over the map with the camera already there, drifting: the settings shots and the feature shots. */
	bool IsArrivedShot() const { return IsSettingsShot() || IsFeatureShot(); }
	/** World: the camera goes round the deck (ANocturneDeck), three ways a tree lives in a scene. */
	bool IsWorldShot() const { return ShotName == TEXT("World"); }
	bool IsTestShot() const { return !IsSettingsShot() && !IsMenuShot() && !IsHudShot() && !IsFeatureShot() && !IsWorldShot(); }
	/** The world shot's camera at time T: where it is, and where it looks. */
	void WorldCameraAt(float T, FVector& OutPosition, FVector& OutLook) const;
	/** The settings screen on the screen: the director's own, or the one the menu opened. */
	UNocturneSettingsScreen* GetSettingsScreen() const;
	/** The star's screen the menu has open (settings included), or the director's own settings screen. */
	UNocturneStarScreen* GetStarScreen() const;
	void SpawnMenu(bool bInTitle);
	/** A pad press: a direction (Up, Down, Left, Right) or the confirm button, through the input module. */
	void PadNavigate(int32 InDirection);
	void PadConfirm();
	/** Pad presses waiting for their release: the input user steps on its own tick, so a press let go of in the same
	 *  frame never moves anything; it is let go of on the next frame. Held any longer it repeats, and the repeat runs
	 *  on real time -- a take dumping frames runs far slower than real time, so even 80 ms held stepped four times.
	 *  (Direction 0-3, or 4 for the confirm button; when to let go.) */
	struct FPadRelease
	{
		int32 Button = 0;
		float Time = 0.0f;
	};
	TArray<FPadRelease> PadReleases;
	void SpawnVoyage(const FVector& InStart, const FRotator& InHeading, int32 InSlot);
	/** The camera the frame is seen through. */
	UCameraComponent* GetActiveCamera() const;
	void SetupTestShot();
	void SetupSettingsShot();
	void SetupMenuShot();
	void OpenSettings();
	void CloseSettings();
	void OpenFeatureScreen();

	UPROPERTY(Transient)
	TObjectPtr<UDreamUserWidget> Cursor;

	UPROPERTY(Transient)
	TObjectPtr<UDreamWidget> ScreenFX;
	UPROPERTY(Transient)
	TObjectPtr<UDreamWidget> Lens;

	TWeakObjectPtr<UDreamStandaloneInputModule> InputModule;
	TArray<FNocturnePointerKey> PointerKeys;
	TArray<FNocturneScramble> Scrambles;
	TArray<bool> ScrambleDone;
	FVector2D LastPointer = FVector2D(-100.0, -100.0);
	bool bHaveLastPointer = false;
	/** -DemoShot=Name picks a take; without one, the demo map is played. */
	FString ShotName = TEXT("Play");
	float ShotTime = 0.0f;
	bool bButtonDown = false;
	bool bViewTargetSet = false;
	/** Play: the player controller set up for the mouse (shown to the UI, its own cursor hidden under ours). */
	bool bPlayInputSet = false;
	bool bRevealStarted = false;
	bool bMeasure = false;
	TArray<float> FrameMs;
	TArray<float> GpuMs;
};
