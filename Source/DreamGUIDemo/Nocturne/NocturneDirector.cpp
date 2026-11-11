// Copyright DreamMoon. All Rights Reserved.

#include "NocturneDirector.h"

#include "NocturneCamera.h"
#include "NocturneDeck.h"
#include "NocturneSpace.h"
#include "Menu/NocturneMainMenu.h"
#include "Menu/NocturneMenu.h"
#include "Menu/NocturneStarLabel.h"
#include "NocturneVoyage.h"
#include "Screens/NocturneStarScreen.h"
#include "Settings/NocturneOptionRow.h"
#include "Settings/NocturneSettingsScreen.h"

#include "Camera/CameraComponent.h"
#include "Controls/DreamSlider.h"
#include "Controls/DreamToggle.h"
#include "Core/Components/DreamCanvas.h"
#include "Core/Components/DreamText.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamScreenUISubsystem.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWorldWidgetComponent.h"
#include "DreamUIBPLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Event/DreamUIInputSubsystem.h"
#include "Event/DreamUIInputUser.h"
#include "Event/InputModule/DreamStandaloneInputModule.h"
#include "Extensions/Effects/DreamPixelSort.h"
#include "Screens/NocturneLens.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"
#include "Misc/CommandLine.h"
#include "RHI.h"
#include "SceneView.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogNocturne, Log, All);

namespace NocturneDirector
{
	// The test shot: fly in from far out to the Settings star and its panel.
	constexpr float FlySeconds = 3.4f;
	const FVector CameraStart(-1150.0, -560.0, 380.0);
	const FVector CameraEnd(-330.0, 30.0, 10.0);
	// Where the camera looks at the end: a little left of the panel's centre, so the star stays in frame.
	const FVector LookEndOffset(0.0, -20.0, 4.0);
	// The world shot: where the deck stands, and the camera's way round it, in the deck's own frame (it faces -X).
	const FVector DeckAt(-900.0, 1400.0, -400.0);
	struct FWorldKey { float Time; FVector Position; FVector Look; };
	const FWorldKey WorldKeys[] = {
		// The curved screen, from the left, then sliding right so its curve reads.
		{ 0.0f, FVector(-230.0, -260.0, 30.0), FVector(10.0, -150.0, 0.0) },
		{ 2.2f, FVector(-205.0, -95.0, 40.0), FVector(20.0, -140.0, 0.0) },
		// The wall, the marker standing on it; round the wall's end, and the marker is behind it.
		{ 3.2f, FVector(-300.0, 40.0, 50.0), FVector(40.0, 115.0, 10.0) },
		{ 4.3f, FVector(-150.0, 290.0, 70.0), FVector(60.0, 135.0, 10.0) },
		// The panel the engine draws, the pillar through it.
		{ 5.4f, FVector(-110.0, 430.0, 40.0), FVector(160.0, 290.0, 10.0) },
		{ 6.8f, FVector(-90.0, 405.0, 30.0), FVector(165.0, 290.0, 10.0) },
		// Back, all three together, drifting.
		{ 8.6f, FVector(-780.0, 120.0, 210.0), FVector(50.0, 85.0, 0.0) },
		{ 15.0f, FVector(-720.0, -40.0, 150.0), FVector(50.0, 85.0, 0.0) },
	};

	// How long the shot runs; -DemoMeasure quits after it.
	constexpr float ShotSeconds = 11.0f;

	// The panel cuts in as the camera settles. The pixel sort over it comes up as the rows start to come in -- on an
	// empty plate it would have nothing to sort -- and settles as they finish.
	constexpr float RevealAt = 3.0f;
	constexpr float SortRiseFrom = 0.08f;
	constexpr float SortPeakAt = 0.12f;
	constexpr float SortGoneAt = 0.55f;
	constexpr float SortPeak = 0.85f;
	// The screen-wide sort at full camera speed.
	constexpr float WarpPeak = 0.5f;
	constexpr double PanelScale = 0.2;
	// The volume readout counts up while its row comes in (seconds after RevealAt).
	constexpr float CountFrom = 0.22f;
	constexpr float CountSeconds = 0.45f;

	// Camera speed (units per second) at which the screen-wide pixel sort is strongest.
	constexpr double WarpFullSpeed = 650.0;

	const FColor Cyan(0x8F, 0xE3, 0xFF);
	const FColor InkFaint(0x56, 0x63, 0x6F);
	const TCHAR* ScrambleGlyphs = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ0123456789#/<>");

	double EaseInOutCubic(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return X < 0.5 ? 4.0 * X * X * X : 1.0 - FMath::Pow(-2.0 * X + 2.0, 3.0) / 2.0;
	}

	double EaseOutCubic(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return 1.0 - FMath::Pow(1.0 - X, 3.0);
	}

	double EaseOutQuad(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return 1.0 - (1.0 - X) * (1.0 - X);
	}

	FVector Bezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, double T)
	{
		const double U = 1.0 - T;
		return U * U * U * P0 + 3.0 * U * U * T * P1 + 3.0 * U * T * T * P2 + T * T * T * P3;
	}

	// The slider handle's centre at a value: the handle area is inset by half the handle (6 of the 330-wide slider).
	double SliderSpot(double Value)
	{
		return (3.0 + Value * (330.0 - 6.0)) / 330.0;
	}

	// Final, with everything past the resolved part replaced by glyphs that change every other frame. Seeded by the
	// frame, so a take is the same every time.
	FString Scramble(const FString& Final, double Progress, int32 Frame)
	{
		const int32 Resolved = FMath::FloorToInt(FMath::Clamp(Progress, 0.0, 1.0) * Final.Len());
		const int32 GlyphCount = FCString::Strlen(ScrambleGlyphs);
		FString Out = Final;
		for (int32 Index = Resolved; Index < Final.Len(); ++Index)
		{
			if (Final[Index] == TEXT(' '))
			{
				continue;
			}
			FRandomStream Stream(Frame * 7919 + Index * 104729);
			Out[Index] = ScrambleGlyphs[Stream.RandRange(0, GlyphCount - 1)];
		}
		return Out;
	}
}

ANocturneDirector::ANocturneDirector()
{
	PrimaryActorTick.bCanEverTick = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	SetRootComponent(Camera);
	NocturneCamera::Configure(Camera);

	// Soft references, loaded in BeginPlay: see ANocturneSpace's constructor for why a widget Blueprint is never loaded
	// from one.
	CursorClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_Cursor.WBP_Nocturne_Cursor_C")));
	ScreenFXClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_ScreenFX.WBP_Nocturne_ScreenFX_C")));
	LensClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_Lens.WBP_Nocturne_Lens_C")));
	SettingsClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_Settings.WBP_Nocturne_Settings_C")));
}

void ANocturneDirector::Setup(ANocturneSpace* InSpace)
{
	Space = InSpace;
}

void ANocturneDirector::BeginPlay()
{
	Super::BeginPlay();

	FParse::Value(FCommandLine::Get(), TEXT("DemoShot="), ShotName);
	bMeasure = FParse::Param(FCommandLine::Get(), TEXT("DemoMeasure"));
	UE_LOG(LogNocturne, Log, TEXT("Director: shot '%s'%s"), *ShotName, bMeasure ? TEXT(", measuring") : TEXT(""));

	// The lens first: over every screen, under the cursor, the post effects that break the whole picture up. Screens the
	// shot spawns below (the title among them) ask for it as they come in.
	if (UClass* LensLoaded = LensClass.LoadSynchronous())
	{
		Lens = UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, LensLoaded, 9000);
		UNocturneLens::SetCurrent(Cast<UNocturneLens>(Lens));
	}

	if (ShotName == TEXT("SettingsPages"))
	{
		// A layout check, not a shot of the video: every page in turn, then the confirm, with no pointer in the way.
		Events = { { 0.3f, [this]() { OpenSettings(); } } };
		for (int32 Index = 1; Index < UNocturneSettingsScreen::PageCount; ++Index)
		{
			Events.Add({ 0.3f + Index * 1.0f, [this, Index]() { if (IsValid(Settings)) { Settings->ShowPage(Index); } } });
		}
		Events.Add({ 6.5f, [this]() { if (IsValid(Settings)) { Settings->AskReset(); } } });
	}
	else if (IsSettingsShot())
	{
		SetupSettingsShot();
	}
	else if (IsMenuShot())
	{
		SetupMenuShot();
	}
	else if (IsHudShot())
	{
		// The game alone: a voyage under way past the constellation, no menu before it.
		SpawnVoyage(FVector(1600.0, -200.0, 120.0), FRotator(0.0, -6.0, 0.0), 0);
	}
	else if (IsFeatureShot())
	{
		// The feature screen plays itself; no pointer.
		Events = { { 0.2f, [this]() { OpenFeatureScreen(); } } };
	}
	else if (IsWorldShot())
	{
		// Out past the constellation, so none of its stars stand in the frame.
		FActorSpawnParameters Spawn;
		Spawn.ObjectFlags |= RF_Transient;
		Deck = GetWorld()->SpawnActor<ANocturneDeck>(NocturneDirector::DeckAt, FRotator::ZeroRotator, Spawn);
	}
	else
	{
		SetupTestShot();
	}

	Scrambles = {
		{ TEXT("MetaLeft"), TEXT("NOC//SYS    STAR 02 / 06"), 0.10f, 0.38f },
		{ TEXT("MetaRight"), TEXT("ONLINE"), 0.10f, 0.30f },
		{ TEXT("TitleTag"), TEXT("SETTINGS"), 0.14f, 0.32f },
		{ TEXT("VolumeSub"), TEXT("MASTER VOLUME"), 0.22f, 0.32f },
		{ TEXT("SubtitleSub"), TEXT("SUBTITLES"), 0.26f, 0.30f },
	};

	LoadedCursorClass = CursorClass.LoadSynchronous();
	if (UClass* ScreenFXLoaded = ScreenFXClass.LoadSynchronous())
	{
		// Over the 3D view, under the cursor (which sorts at 10000).
		ScreenFX = UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, ScreenFXLoaded, 100);
		if (UDreamPixelSort* Warp = FindPixelSort(ScreenFX, TEXT("Warp")))
		{
			// Only star cores and spikes streak; with the glows' outer rings in the sort, every glow got a hard edge
			// where its sorted half ended.
			Warp->SetThresholdMin(0.6f);
		}
	}

	if (!IsTestShot() && IsValid(Space))
	{
		// The test panel is the test shot's alone; anywhere else it must not even take the pointer.
		Space->GetPanel()->SetWidgetVisible(false);
	}
	// The pointer is the director's from the first frame: the OS mouse, wherever it happens to be, never hovers anything.
	GetInputModule();

	UpdateCamera(0.0f);
	UpdatePanel(0.0f);
}

bool ANocturneDirector::IsMenuShot() const
{
	for (const TCHAR* Name : { TEXT("Menu"), TEXT("Title"), TEXT("Archive"), TEXT("Wheel"), TEXT("Music"), TEXT("Log"), TEXT("Depart"), TEXT("Full"), TEXT("Tour"), TEXT("Pad"), TEXT("Play") })
	{
		if (ShotName == Name)
		{
			return true;
		}
	}
	return false;
}

void ANocturneDirector::SpawnMenu(bool bInTitle)
{
	// The star map is the menu's: its camera, its labels, its screens. The director only moves the pointer.
	Menu = GetWorld()->SpawnActorDeferred<ANocturneMenu>(ANocturneMenu::StaticClass(), FTransform::Identity);
	if (Menu == nullptr)
	{
		return;
	}
	Menu->Setup(Space);
	Menu->SetStartAtTitle(bInTitle);
	Menu->FinishSpawning(FTransform::Identity);
	// The menu moves its camera and labels first, so the pointer is placed on this frame's picture.
	AddTickPrerequisiteActor(Menu);
	// Depart launches into the game: the voyage carries on from where the menu's dive ends.
	Menu->OnLaunched.AddWeakLambda(this, [this](int32 InSlot)
	{
		const UCameraComponent* MenuCamera = Menu->GetCamera();
		SpawnVoyage(MenuCamera->GetComponentLocation(), MenuCamera->GetComponentRotation(), InSlot);
	});
}

void ANocturneDirector::SpawnVoyage(const FVector& InStart, const FRotator& InHeading, int32 InSlot)
{
	Voyage = GetWorld()->SpawnActorDeferred<ANocturneVoyage>(ANocturneVoyage::StaticClass(), FTransform::Identity);
	if (Voyage == nullptr)
	{
		return;
	}
	Voyage->Setup(InStart, InHeading, InSlot);
	Voyage->FinishSpawning(FTransform::Identity);
	AddTickPrerequisiteActor(Voyage);
	if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
	{
		Player->SetViewTarget(Voyage);
	}
	UE_LOG(LogNocturne, Log, TEXT("Director: voyage under way (slot %d)"), InSlot);
}

void ANocturneDirector::PadNavigate(int32 InDirection)
{
	if (UDreamStandaloneInputModule* Module = GetInputModule())
	{
		const EDreamUINavigationDirection Directions[] = { EDreamUINavigationDirection::Up, EDreamUINavigationDirection::Down,
			EDreamUINavigationDirection::Left, EDreamUINavigationDirection::Right };
		Module->InputNavigation(Directions[FMath::Clamp(InDirection, 0, 3)], true, 0);
		PadReleases.Add({ FMath::Clamp(InDirection, 0, 3), ShotTime + 0.001f });
	}
}

void ANocturneDirector::PadConfirm()
{
	if (UDreamStandaloneInputModule* Module = GetInputModule())
	{
		Module->InputTriggerForNavigation(true, 0);
		PadReleases.Add({ 4, ShotTime + 0.001f });
	}
}

void ANocturneDirector::SetupMenuShot()
{
	using Key = FNocturnePointerKey;
	SpawnMenu(ShotName == TEXT("Title") || ShotName == TEXT("Full") || ShotName == TEXT("Tour") || IsPlayShot());

	// Over a label's glyph: its hit area runs 330 x 116 from just left of the ring; the glyph sits 132 in, 67 up. Every
	// tour is ordered so that no move crosses a label it is not going to: a hand crossing one would light it on the way.
	const FVector2D Glyph(0.40, 0.58);
	const FVector2D Mid(0.5, 0.5);
	// A click: over the spot, pressed, released.
	auto Click = [this](float InTime, FNocturnePointerKey InKey)
	{
		InKey.Time = InTime;
		PointerKeys.Add(InKey);
		FNocturnePointerKey Down = InKey;
		Down.Time = InTime + 0.3f;
		Down.bPressed = true;
		PointerKeys.Add(Down);
		FNocturnePointerKey Up = InKey;
		Up.Time = InTime + 0.4f;
		PointerKeys.Add(Up);
	};
	// The item wheel's wedge InIndex on the screen: the wheel is round the middle of the frame.
	auto Wedge = [](int32 InIndex)
	{
		const double Angle = FMath::DegreesToRadians(45.0 * InIndex);
		return FVector2D(960.0 + 236.0 * FMath::Sin(Angle), 540.0 - 236.0 * FMath::Cos(Angle));
	};
	// Into a star's screen from the map: in from the top right, onto the star's label, click.
	auto EnterStar = [&](int32 InStar)
	{
		PointerKeys.Add(Key::At(1.9f, FVector2D(1760.0, 250.0)));
		Click(2.7f, Key::OnStar(0.0f, InStar, Glyph));
	};
	// Out by the key hint, and away below the labels.
	auto Leave = [&](float InTime)
	{
		Click(InTime, Key::OnScreen(0.0f, TEXT("HintBack"), Mid));
		PointerKeys.Add(Key::At(InTime + 1.2f, FVector2D(1400.0, 1000.0)));
		PointerKeys.Add(Key::At(InTime + 3.0f, FVector2D(1400.0, 1000.0)));
	};

	if (ShotName == TEXT("Menu"))
	{
		PointerKeys = {
			Key::At(1.9f, FVector2D(1760.0, 250.0)),
			// Round the map, from the top right: archive, item wheel, music box, log, settings, depart.
			Key::OnStar(2.7f, 2, Glyph),
			Key::OnStar(3.6f, 3, Glyph),
			Key::OnStar(4.5f, 4, Glyph),
			Key::OnStar(5.4f, 5, Glyph),
			Key::OnStar(6.2f, 1, Glyph),
			Key::OnStar(7.0f, 0, Glyph),
			// Back to Settings: the flight there.
			Key::OnStar(8.3f, 1, Glyph),
			Key::OnStar(8.8f, 1, Glyph, true),
			Key::OnStar(8.9f, 1, Glyph),
			Key::At(9.9f, FVector2D(1180.0, 820.0)),
			// A look over the options, then back by the key hint.
			Key::OnRow(11.0f, 0, 0, FVector2D(0.3, 0.5)),
			Key::OnRow(11.8f, 0, 2, FVector2D(0.3, 0.5)),
			Key::OnScreen(12.6f, TEXT("HintBack"), Mid),
			Key::OnScreen(13.05f, TEXT("HintBack"), Mid, true),
			Key::OnScreen(13.15f, TEXT("HintBack"), Mid),
			// Home: in below the labels to the route list, then up it a step at a time.
			Key::At(14.0f, FVector2D(1400.0, 1000.0)),
			Key::OnMenu(15.2f, TEXT("Nav4"), FVector2D(0.6, 0.5)),
			Key::OnMenu(15.8f, TEXT("Nav3"), FVector2D(0.6, 0.5)),
			Key::OnMenu(16.4f, TEXT("Nav2"), FVector2D(0.6, 0.5)),
			Key::OnMenu(17.0f, TEXT("Nav1"), FVector2D(0.6, 0.5)),
			Key::OnMenu(19.0f, TEXT("Nav1"), FVector2D(0.62, 0.45)),
		};
	}
	else if (ShotName == TEXT("Title"))
	{
		// The title breathes, any key, the wake into the map; then a look round two stars.
		Events = { { 4.6f, [this]() { if (IsValid(Menu)) { Menu->PressAnyKey(); } } } };
		PointerKeys = {
			Key::At(8.6f, FVector2D(1760.0, 250.0)),
			Key::OnStar(9.4f, 2, Glyph),
			Key::OnStar(10.4f, 1, Glyph),
			Key::OnStar(12.5f, 1, FVector2D(0.42, 0.55)),
		};
	}
	else if (ShotName == TEXT("Archive"))
	{
		EnterStar(2);
		PointerKeys.Add(Key::At(4.3f, FVector2D(1100.0, 860.0)));
		// Rest on an item for its tooltip, pick another, drag the first item to an empty cell, filter, back.
		PointerKeys.Add(Key::OnScreen(5.0f, TEXT("Slot4"), Mid));
		PointerKeys.Add(Key::OnScreen(6.3f, TEXT("Slot4"), FVector2D(0.56, 0.44)));
		Click(6.9f, Key::OnScreen(0.0f, TEXT("Slot6"), Mid));
		PointerKeys.Add(Key::OnScreen(7.9f, TEXT("Slot0"), Mid));
		PointerKeys.Add(Key::OnScreen(8.1f, TEXT("Slot0"), Mid, true));
		PointerKeys.Add(Key::OnScreen(8.4f, TEXT("Slot0"), FVector2D(0.75, 0.4), true));
		PointerKeys.Add(Key::OnScreen(9.4f, TEXT("Slot23"), Mid, true));
		PointerKeys.Add(Key::OnScreen(9.6f, TEXT("Slot23"), Mid, true));
		PointerKeys.Add(Key::OnScreen(9.7f, TEXT("Slot23"), Mid));
		Click(10.6f, Key::OnScreen(0.0f, TEXT("Cat1"), Mid));
		Click(11.8f, Key::OnScreen(0.0f, TEXT("Cat0"), Mid));
		Leave(12.9f);
	}
	else if (ShotName == TEXT("Wheel"))
	{
		EnterStar(3);
		PointerKeys.Add(Key::At(4.4f, FVector2D(1300.0, 860.0)));
		// Round the wheel; two items into the quick slots, a slot picked by hand, a third item.
		PointerKeys.Add(Key::At(5.2f, Wedge(0)));
		Click(6.0f, Key::At(0.0f, Wedge(2)));
		Click(7.2f, Key::At(0.0f, Wedge(5)));
		Click(8.6f, Key::OnScreen(0.0f, TEXT("Quick3"), Mid));
		Click(9.8f, Key::At(0.0f, Wedge(7)));
		Leave(11.2f);
	}
	else if (ShotName == TEXT("Music"))
	{
		EnterStar(4);
		PointerKeys.Add(Key::At(4.4f, FVector2D(1500.0, 900.0)));
		// The words run a while; another track (instrumental); back to the first.
		Click(8.0f, Key::OnScreen(0.0f, TEXT("Track2"), Mid));
		Click(9.6f, Key::OnScreen(0.0f, TEXT("Track0"), Mid));
		Leave(13.0f);
	}
	else if (ShotName == TEXT("Log"))
	{
		EnterStar(5);
		PointerKeys.Add(Key::At(4.4f, FVector2D(1000.0, 900.0)));
		// Open two quests in turn, then the messages and the second of them.
		Click(5.6f, Key::OnScreen(0.0f, TEXT("Quest1"), FVector2D(0.3, 0.5)));
		Click(7.2f, Key::OnScreen(0.0f, TEXT("Quest2"), FVector2D(0.3, 0.5)));
		Click(8.8f, Key::OnScreen(0.0f, TEXT("Cat3"), Mid));
		Click(10.2f, Key::OnScreen(0.0f, TEXT("Quest6"), FVector2D(0.3, 0.5)));
		Leave(11.8f);
	}
	else if (ShotName == TEXT("Depart"))
	{
		EnterStar(0);
		PointerKeys.Add(Key::At(4.4f, FVector2D(1100.0, 960.0)));
		// Look over the cards, pick the second, set off.
		PointerKeys.Add(Key::OnScreen(5.3f, TEXT("Card1"), FVector2D(0.5, 0.6)));
		PointerKeys.Add(Key::OnScreen(6.2f, TEXT("Card2"), FVector2D(0.5, 0.6)));
		Click(7.0f, Key::OnScreen(0.0f, TEXT("Card1"), FVector2D(0.5, 0.6)));
		Click(8.3f, Key::OnScreen(0.0f, TEXT("Launch"), Mid));
		PointerKeys.Add(Key::At(9.4f, FVector2D(1100.0, 900.0)));
	}
	else if (ShotName == TEXT("Pad"))
	{
		// No pointer: a pad down the route list, into the archive, across its grid, back out.
		enum { Up, Down, Left, Right };
		Events = {
			{ 1.8f, [this]() { PadNavigate(Down); } },
			{ 2.5f, [this]() { PadNavigate(Down); } },
			{ 3.2f, [this]() { PadNavigate(Up); } },
			{ 4.0f, [this]() { PadConfirm(); } },
			{ 6.2f, [this]() { PadNavigate(Right); } },
			{ 6.7f, [this]() { PadNavigate(Right); } },
			{ 7.2f, [this]() { PadNavigate(Down); } },
			{ 7.7f, [this]() { PadNavigate(Right); } },
			{ 8.3f, [this]() { PadConfirm(); } },
			{ 9.1f, [this]() { PadNavigate(Left); } },
			{ 9.6f, [this]() { PadNavigate(Left); } },
			{ 10.2f, [this]() { PadConfirm(); } },
			{ 11.2f, [this]() { if (IsValid(Menu) && Menu->GetScreen() != nullptr) { Menu->GetScreen()->RequestBack(); } } },
			{ 13.6f, [this]() { PadNavigate(Down); } },
			{ 14.2f, [this]() { PadNavigate(Down); } },
		};
	}
	else if (ShotName == TEXT("Full"))
	{
		// The whole front end in one take: the title, the map, the archive, settings, and off into the game.
		Events = { { 3.4f, [this]() { if (IsValid(Menu)) { Menu->PressAnyKey(); } } } };
		PointerKeys.Add(Key::At(6.6f, FVector2D(1760.0, 250.0)));
		Click(7.4f, Key::OnStar(0.0f, 2, Glyph));
		PointerKeys.Add(Key::At(9.0f, FVector2D(1100.0, 860.0)));
		PointerKeys.Add(Key::OnScreen(9.7f, TEXT("Slot4"), Mid));
		PointerKeys.Add(Key::OnScreen(10.6f, TEXT("Slot0"), Mid));
		PointerKeys.Add(Key::OnScreen(10.8f, TEXT("Slot0"), Mid, true));
		PointerKeys.Add(Key::OnScreen(11.1f, TEXT("Slot0"), FVector2D(0.75, 0.4), true));
		PointerKeys.Add(Key::OnScreen(12.0f, TEXT("Slot23"), Mid, true));
		PointerKeys.Add(Key::OnScreen(12.2f, TEXT("Slot23"), Mid, true));
		PointerKeys.Add(Key::OnScreen(12.3f, TEXT("Slot23"), Mid));
		Click(13.1f, Key::OnScreen(0.0f, TEXT("HintBack"), Mid));
		PointerKeys.Add(Key::At(14.3f, FVector2D(1250.0, 600.0)));
		// Home by 15.1; settings.
		Click(16.0f, Key::OnStar(0.0f, 1, Glyph));
		PointerKeys.Add(Key::At(17.4f, FVector2D(1180.0, 820.0)));
		Click(18.4f, Key::OnScreen(0.0f, TEXT("Tab2"), Mid));
		PointerKeys.Add(Key::OnRow(19.4f, 2, 0, FVector2D(0.781, 0.5)));
		PointerKeys.Add(Key::OnRow(19.6f, 2, 0, FVector2D(0.781, 0.5), true));
		PointerKeys.Add(Key::OnRow(20.3f, 2, 0, FVector2D(0.640, 0.5), true));
		PointerKeys.Add(Key::OnRow(20.4f, 2, 0, FVector2D(0.640, 0.5)));
		Click(21.2f, Key::OnScreen(0.0f, TEXT("HintBack"), Mid));
		PointerKeys.Add(Key::At(22.4f, FVector2D(1400.0, 1000.0)));
		// Home by 23.2; depart.
		Click(24.0f, Key::OnStar(0.0f, 0, Glyph));
		PointerKeys.Add(Key::At(25.4f, FVector2D(1100.0, 960.0)));
		Click(26.2f, Key::OnScreen(0.0f, TEXT("Card1"), FVector2D(0.5, 0.6)));
		Click(27.4f, Key::OnScreen(0.0f, TEXT("Launch"), Mid));
		PointerKeys.Add(Key::At(28.4f, FVector2D(1100.0, 900.0)));
	}
	else if (ShotName == TEXT("Tour"))
	{
		// Every star in one take, each screen doing what its own shot does: the title, the archive, the item wheel, the
		// music box, the log, settings, and off into the game. T is when the next star is clicked; home is about two
		// seconds after the back hint.
		Events = { { 3.4f, [this]() { if (IsValid(Menu)) { Menu->PressAnyKey(); } } } };
		PointerKeys.Add(Key::At(6.6f, FVector2D(1760.0, 250.0)));
		float T = 7.4f;

		// Archive: rest on an item, drag one to an empty cell, back.
		Click(T, Key::OnStar(0.0f, 2, Glyph));
		PointerKeys.Add(Key::At(T + 1.6f, FVector2D(1100.0, 860.0)));
		PointerKeys.Add(Key::OnScreen(T + 2.3f, TEXT("Slot4"), Mid));
		PointerKeys.Add(Key::OnScreen(T + 3.4f, TEXT("Slot4"), FVector2D(0.56, 0.44)));
		PointerKeys.Add(Key::OnScreen(T + 4.0f, TEXT("Slot0"), Mid));
		PointerKeys.Add(Key::OnScreen(T + 4.2f, TEXT("Slot0"), Mid, true));
		PointerKeys.Add(Key::OnScreen(T + 4.5f, TEXT("Slot0"), FVector2D(0.75, 0.4), true));
		PointerKeys.Add(Key::OnScreen(T + 5.4f, TEXT("Slot23"), Mid, true));
		PointerKeys.Add(Key::OnScreen(T + 5.6f, TEXT("Slot23"), Mid, true));
		PointerKeys.Add(Key::OnScreen(T + 5.7f, TEXT("Slot23"), Mid));
		Click(T + 6.6f, Key::OnScreen(0.0f, TEXT("Cat1"), Mid));
		Leave(T + 7.8f);
		T += 10.7f;

		// Item wheel: round it, two items into the quick slots, a slot by hand, a third item.
		Click(T, Key::OnStar(0.0f, 3, Glyph));
		PointerKeys.Add(Key::At(T + 1.7f, FVector2D(1300.0, 860.0)));
		PointerKeys.Add(Key::At(T + 2.5f, Wedge(0)));
		Click(T + 3.3f, Key::At(0.0f, Wedge(2)));
		Click(T + 4.5f, Key::At(0.0f, Wedge(5)));
		Click(T + 5.9f, Key::OnScreen(0.0f, TEXT("Quick3"), Mid));
		Click(T + 7.1f, Key::At(0.0f, Wedge(7)));
		Leave(T + 8.5f);
		T += 11.4f;

		// Music box: the words run, another track, back to the first.
		Click(T, Key::OnStar(0.0f, 4, Glyph));
		PointerKeys.Add(Key::At(T + 1.7f, FVector2D(1500.0, 900.0)));
		Click(T + 4.6f, Key::OnScreen(0.0f, TEXT("Track2"), Mid));
		Click(T + 6.2f, Key::OnScreen(0.0f, TEXT("Track0"), Mid));
		Leave(T + 8.6f);
		T += 11.5f;

		// Log: two quests open in turn, then the messages and one of them.
		Click(T, Key::OnStar(0.0f, 5, Glyph));
		PointerKeys.Add(Key::At(T + 1.7f, FVector2D(1000.0, 900.0)));
		Click(T + 2.9f, Key::OnScreen(0.0f, TEXT("Quest1"), FVector2D(0.3, 0.5)));
		Click(T + 4.5f, Key::OnScreen(0.0f, TEXT("Quest2"), FVector2D(0.3, 0.5)));
		Click(T + 6.1f, Key::OnScreen(0.0f, TEXT("Cat3"), Mid));
		Click(T + 7.5f, Key::OnScreen(0.0f, TEXT("Quest6"), FVector2D(0.3, 0.5)));
		Leave(T + 9.1f);
		T += 12.0f;

		// Settings: the audio page, master volume dragged down, back.
		Click(T, Key::OnStar(0.0f, 1, Glyph));
		PointerKeys.Add(Key::At(T + 1.4f, FVector2D(1180.0, 820.0)));
		Click(T + 2.4f, Key::OnScreen(0.0f, TEXT("Tab2"), Mid));
		PointerKeys.Add(Key::OnRow(T + 3.4f, 2, 0, FVector2D(0.781, 0.5)));
		PointerKeys.Add(Key::OnRow(T + 3.6f, 2, 0, FVector2D(0.781, 0.5), true));
		PointerKeys.Add(Key::OnRow(T + 4.3f, 2, 0, FVector2D(0.640, 0.5), true));
		PointerKeys.Add(Key::OnRow(T + 4.4f, 2, 0, FVector2D(0.640, 0.5)));
		Click(T + 5.2f, Key::OnScreen(0.0f, TEXT("HintBack"), Mid));
		PointerKeys.Add(Key::At(T + 6.4f, FVector2D(1400.0, 1000.0)));
		T += 8.0f;

		// Depart: the second save, launch, into the game.
		Click(T, Key::OnStar(0.0f, 0, Glyph));
		PointerKeys.Add(Key::At(T + 1.4f, FVector2D(1100.0, 960.0)));
		Click(T + 2.2f, Key::OnScreen(0.0f, TEXT("Card1"), FVector2D(0.5, 0.6)));
		Click(T + 3.4f, Key::OnScreen(0.0f, TEXT("Launch"), Mid));
		PointerKeys.Add(Key::At(T + 4.4f, FVector2D(1100.0, 900.0)));
	}
	PointerKeys.Sort([](const FNocturnePointerKey& A, const FNocturnePointerKey& B) { return A.Time < B.Time; });
}

UNocturneSettingsScreen* ANocturneDirector::GetSettingsScreen() const
{
	return IsValid(Menu) ? Menu->GetSettings() : Settings.Get();
}

UNocturneStarScreen* ANocturneDirector::GetStarScreen() const
{
	if (IsValid(Menu))
	{
		return Menu->GetScreen();
	}
	return Settings.Get();
}

UCameraComponent* ANocturneDirector::GetActiveCamera() const
{
	if (IsValid(Voyage))
	{
		return Voyage->GetCamera();
	}
	return IsValid(Menu) ? Menu->GetCamera() : Camera.Get();
}

void ANocturneDirector::SetupTestShot()
{
	using NocturneDirector::SliderSpot;
	using Key = FNocturnePointerKey;
	PointerKeys = {
		Key::At(3.8f, FVector2D(1730.0, 1000.0)),
		Key::OnPanel(4.9f, TEXT("Volume"), FVector2D(SliderSpot(0.35), 0.5)),
		Key::OnPanel(5.15f, TEXT("Volume"), FVector2D(SliderSpot(0.35), 0.5), true),
		Key::OnPanel(6.25f, TEXT("Volume"), FVector2D(SliderSpot(0.80), 0.5), true),
		Key::OnPanel(6.45f, TEXT("Volume"), FVector2D(SliderSpot(0.80), 0.5)),
		Key::OnPanel(7.4f, TEXT("Subtitles"), FVector2D(0.5, 0.5)),
		Key::OnPanel(7.55f, TEXT("Subtitles"), FVector2D(0.5, 0.5), true),
		Key::OnPanel(7.68f, TEXT("Subtitles"), FVector2D(0.5, 0.5)),
		Key::OnPanel(8.7f, TEXT("Apply"), FVector2D(0.5, 0.5)),
		Key::OnPanel(8.85f, TEXT("Apply"), FVector2D(0.5, 0.5), true),
		Key::OnPanel(9.0f, TEXT("Apply"), FVector2D(0.5, 0.5)),
		Key::OnPanel(10.5f, TEXT("Apply"), FVector2D(0.75, 0.2)),
	};
}

void ANocturneDirector::SetupSettingsShot()
{
	using Key = FNocturnePointerKey;
	// Spots on an 840-wide option row: the control area is the right 380, 18 in from the edge.
	auto Segment = [](int32 InIndex, int32 InCount) { return (822.0 - (InCount * 72.0 + (InCount - 1) * 4.0) + InIndex * 76.0 + 36.0) / 840.0; };
	auto Slider = [](double InValue, double InMin, double InMax) { return (442.0 + 3.0 + (InValue - InMin) / (InMax - InMin) * 264.0) / 840.0; };
	const double KeyCap = (822.0 - 75.0) / 840.0;

	PointerKeys = {
		Key::At(1.6f, FVector2D(1700.0, 1000.0)),
		// Graphics: the quality preset to "very high", render scale up to 100 %.
		Key::OnRow(2.4f, 0, 0, FVector2D(Segment(3, 5), 0.5)),
		Key::OnRow(2.55f, 0, 0, FVector2D(Segment(3, 5), 0.5), true),
		Key::OnRow(2.65f, 0, 0, FVector2D(Segment(3, 5), 0.5)),
		Key::OnRow(3.4f, 0, 1, FVector2D(Slider(80, 50, 100), 0.5)),
		Key::OnRow(3.6f, 0, 1, FVector2D(Slider(80, 50, 100), 0.5), true),
		Key::OnRow(4.3f, 0, 1, FVector2D(Slider(100, 50, 100), 0.5), true),
		Key::OnRow(4.45f, 0, 1, FVector2D(Slider(100, 50, 100), 0.5)),
		// Over global illumination, which needs a restart; then motion blur on.
		Key::OnRow(5.0f, 0, 3, FVector2D(0.35, 0.5)),
		Key::OnRow(5.45f, 0, 3, FVector2D(0.37, 0.45)),
		Key::OnRow(5.8f, 0, 9, FVector2D(Segment(1, 2), 0.5)),
		Key::OnRow(5.9f, 0, 9, FVector2D(Segment(1, 2), 0.5), true),
		Key::OnRow(6.0f, 0, 9, FVector2D(Segment(1, 2), 0.5)),
		// Audio: master volume down to 35.
		Key::OnScreen(6.8f, TEXT("Tab2"), FVector2D(0.5, 0.5)),
		Key::OnScreen(7.0f, TEXT("Tab2"), FVector2D(0.5, 0.5), true),
		Key::OnScreen(7.1f, TEXT("Tab2"), FVector2D(0.5, 0.5)),
		Key::OnRow(7.9f, 2, 0, FVector2D(Slider(80, 0, 100), 0.5)),
		Key::OnRow(8.1f, 2, 0, FVector2D(Slider(80, 0, 100), 0.5), true),
		Key::OnRow(8.8f, 2, 0, FVector2D(Slider(35, 0, 100), 0.5), true),
		Key::OnRow(8.95f, 2, 0, FVector2D(Slider(35, 0, 100), 0.5)),
		// Controls: rebind jump.
		Key::OnScreen(9.6f, TEXT("Tab3"), FVector2D(0.5, 0.5)),
		Key::OnScreen(9.8f, TEXT("Tab3"), FVector2D(0.5, 0.5), true),
		Key::OnScreen(9.9f, TEXT("Tab3"), FVector2D(0.5, 0.5)),
		Key::OnRow(10.6f, 3, 4, FVector2D(KeyCap, 0.5)),
		Key::OnRow(10.8f, 3, 4, FVector2D(KeyCap, 0.5), true),
		Key::OnRow(10.9f, 3, 4, FVector2D(KeyCap, 0.5)),
		Key::OnRow(12.0f, 3, 4, FVector2D(0.62, 0.3)),
		// R: put the page back -- confirm.
		Key::OnScreen(12.8f, TEXT("ConfirmYes"), FVector2D(0.5, 0.5)),
		Key::OnScreen(13.1f, TEXT("ConfirmYes"), FVector2D(0.5, 0.5), true),
		Key::OnScreen(13.2f, TEXT("ConfirmYes"), FVector2D(0.5, 0.5)),
		Key::At(14.6f, FVector2D(1480.0, 900.0)),
		Key::At(16.0f, FVector2D(1480.0, 900.0)),
	};

	Events = {
		{ 0.5f, [this]() { OpenSettings(); } },
		{ 11.6f, [this]() {
			if (UNocturneOptionRow* Row = IsValid(Settings) ? Settings->GetRow(3, 4) : nullptr; Row != nullptr && Row->IsListening())
			{
				Row->TakeKey(EKeys::V);
			}
		} },
		{ 12.2f, [this]() { if (IsValid(Settings)) { Settings->AskReset(); } } },
		{ 14.0f, [this]() { CloseSettings(); } },
		{ 14.5f, [this]() {
			if (IsValid(Settings))
			{
				UDreamUIBPLibrary::RemoveFromViewport(this, Settings);
				Settings = nullptr;
			}
		} },
	};
}

void ANocturneDirector::OpenSettings()
{
	if (UClass* Loaded = SettingsClass.LoadSynchronous())
	{
		// Over the 3D view and the screen effects, under the cursor.
		Settings = Cast<UNocturneSettingsScreen>(UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, Loaded, 200));
		if (IsValid(Settings))
		{
			Settings->PlayEntrance();
		}
	}
	UE_LOG(LogNocturne, Log, TEXT("Director: settings %s"), IsValid(Settings) ? TEXT("open") : TEXT("FAILED to open"));
}

void ANocturneDirector::OpenFeatureScreen()
{
	const FString Path = FString::Printf(TEXT("/Game/Nocturne/UI/WBP_Nocturne_%s.WBP_Nocturne_%s_C"), *ShotName, *ShotName);
	if (UClass* Loaded = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(Path)).LoadSynchronous())
	{
		FeatureScreen = UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, Loaded, 200);
		if (UNocturneStarScreen* Screen = Cast<UNocturneStarScreen>(FeatureScreen))
		{
			Screen->PlayEntrance();
		}
	}
	UE_LOG(LogNocturne, Log, TEXT("Director: %s screen %s"), *ShotName, IsValid(FeatureScreen) ? TEXT("open") : TEXT("FAILED to open"));
}

void ANocturneDirector::CloseSettings()
{
	if (IsValid(Settings))
	{
		Settings->PlayExit();
	}
}

void ANocturneDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Pad buttons let go of, a few frames after they were pressed.
	for (int32 Index = PadReleases.Num() - 1; Index >= 0; --Index)
	{
		if (ShotTime < PadReleases[Index].Time)
		{
			continue;
		}
		if (UDreamStandaloneInputModule* Module = GetInputModule())
		{
			const EDreamUINavigationDirection Directions[] = { EDreamUINavigationDirection::Up, EDreamUINavigationDirection::Down,
				EDreamUINavigationDirection::Left, EDreamUINavigationDirection::Right };
			if (PadReleases[Index].Button == 4)
			{
				Module->InputTriggerForNavigation(false, 0);
			}
			else
			{
				Module->InputNavigation(Directions[PadReleases[Index].Button], false, 0);
			}
		}
		PadReleases.RemoveAt(Index);
	}

	for (FNocturneEvent& Event : Events)
	{
		if (!Event.bDone && ShotTime >= Event.Time)
		{
			Event.bDone = true;
			Event.Do();
		}
	}

	if (!bViewTargetSet)
	{
		if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
		{
			AActor* Target = IsValid(Voyage) ? static_cast<AActor*>(Voyage) : (IsValid(Menu) ? static_cast<AActor*>(Menu) : this);
			Player->SetViewTarget(Target);
			bViewTargetSet = true;
		}
	}

	ShotTime += DeltaSeconds;
	UpdateCamera(ShotTime);
	UpdateScreenFX(ShotTime);
	UpdatePanel(ShotTime);
	UpdatePointer(ShotTime);
	UpdateReadouts(ShotTime);

	if (bMeasure)
	{
		Measure(DeltaSeconds);
	}
}

FVector ANocturneDirector::CameraPositionAt(float T) const
{
	using namespace NocturneDirector;
	const double A = EaseInOutCubic(T / FlySeconds);
	const FVector P1 = CameraStart + FVector(380.0, 260.0, -40.0);
	const FVector P2 = CameraEnd + FVector(-300.0, -70.0, 60.0);
	FVector Position = Bezier(CameraStart, P1, P2, CameraEnd, A);

	// Once there, a slow drift, so the frame never quite stops.
	const double Settle = FMath::Clamp((T - FlySeconds) / 1.5, 0.0, 1.0);
	Position += FVector(0.0, FMath::Sin(T * 0.45) * 2.5, FMath::Sin(T * 0.33) * 1.6) * Settle;
	return Position;
}

void ANocturneDirector::WorldCameraAt(float T, FVector& OutPosition, FVector& OutLook) const
{
	using namespace NocturneDirector;
	constexpr int32 Count = UE_ARRAY_COUNT(WorldKeys);
	int32 Index = 0;
	while (Index < Count - 2 && T >= WorldKeys[Index + 1].Time)
	{
		++Index;
	}
	const FWorldKey& K1 = WorldKeys[Index];
	const FWorldKey& K2 = WorldKeys[Index + 1];
	const FWorldKey& K0 = WorldKeys[FMath::Max(Index - 1, 0)];
	const FWorldKey& K3 = WorldKeys[FMath::Min(Index + 2, Count - 1)];
	const float U = FMath::Clamp((T - K1.Time) / (K2.Time - K1.Time), 0.0f, 1.0f);
	// Catmull-Rom through the keys: the camera never stops at one.
	auto Spline = [U](const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3)
	{
		const float U2 = U * U;
		const float U3 = U2 * U;
		return 0.5 * ((2.0 * P1) + (P2 - P0) * U + (2.0 * P0 - 5.0 * P1 + 4.0 * P2 - P3) * U2 + (3.0 * P1 - P0 - 3.0 * P2 + P3) * U3);
	};
	const FTransform DeckFrame = IsValid(Deck) ? Deck->GetActorTransform() : FTransform(DeckAt);
	OutPosition = DeckFrame.TransformPosition(Spline(K0.Position, K1.Position, K2.Position, K3.Position));
	OutLook = DeckFrame.TransformPosition(Spline(K0.Look, K1.Look, K2.Look, K3.Look));
}

void ANocturneDirector::UpdateCamera(float T)
{
	if (IsWorldShot())
	{
		FVector Position;
		FVector Look;
		WorldCameraAt(T, Position, Look);
		SetActorLocationAndRotation(Position, (Look - Position).Rotation());
		return;
	}
	if (!IsValid(Space) || IsValid(Menu) || IsValid(Voyage))
	{
		return;
	}
	using namespace NocturneDirector;
	// The settings and layer effects shots start where the test shot ends: arrived, drifting.
	const float PathTime = IsArrivedShot() ? T + FlySeconds + 2.0f : T;
	const FVector Position = CameraPositionAt(PathTime);
	// The look moves from the star to the panel a little behind the camera, so the star leads the way in.
	const double B = EaseInOutCubic((PathTime - 0.3) / (FlySeconds - 0.3));
	const FVector Look = FMath::Lerp(Space->GetFocusStarLocation(), Space->GetPanelLocation() + LookEndOffset, B);
	SetActorLocationAndRotation(Position, (Look - Position).Rotation());
}

void ANocturneDirector::UpdateScreenFX(float T)
{
	UDreamPixelSort* Warp = FindPixelSort(ScreenFX, TEXT("Warp"));
	if (Warp == nullptr)
	{
		return;
	}
	// Speed from the path itself, a frame apart, so it is the same on any time step. The settings shot never flies; the
	// menu's camera says how fast it went.
	constexpr float Step = 1.0f / 60.0f;
	const float PathTime = IsArrivedShot() ? T + NocturneDirector::FlySeconds + 2.0f : T;
	// The world shot's camera only strolls: no streaks there.
	const double Speed = IsWorldShot() ? 0.0 : IsValid(Voyage) ? Voyage->GetCameraSpeed() : IsValid(Menu) ? Menu->GetCameraSpeed()
		: (CameraPositionAt(PathTime) - CameraPositionAt(FMath::Max(PathTime - Step, 0.0f))).Size() / Step;
	const double Amount = FMath::Pow(FMath::Clamp(Speed / NocturneDirector::WarpFullSpeed, 0.0, 1.0), 1.6);
	// Strength 0 is the effect switched off; below a sliver, switch it off rather than sort a pixel or two.
	Warp->SetSortStrength(Amount < 0.02 ? 0.0f : static_cast<float>(Amount * NocturneDirector::WarpPeak));
	// The lens parts the colours outward with the same speed.
	if (UNocturneLens* LensWidget = Cast<UNocturneLens>(Lens))
	{
		LensWidget->SetSpeed(static_cast<float>(Amount));
	}
}

void ANocturneDirector::UpdatePanel(float T)
{
	if (!IsValid(Space))
	{
		return;
	}
	using namespace NocturneDirector;
	UDreamWorldWidgetComponent* Panel = Space->GetPanel();
	if (!IsTestShot())
	{
		Panel->SetWidgetOpacity(0.0f);
		return;
	}
	const float Since = T - RevealAt;

	// The plate cuts in: no fade, one frame hidden and the next there, with a short vertical settle.
	Panel->SetWidgetOpacity(Since >= 0.0f ? 1.0f : 0.0f);
	const double Settle = EaseOutCubic(Since / 0.14);
	Panel->SetRelativeScale3D(FVector(PanelScale, PanelScale, PanelScale * (0.94 + 0.06 * Settle)));

	UDreamWidget* Tree = Panel->GetLoadedWidget();
	if (Since >= 0.0f && !bRevealStarted)
	{
		if (UDreamUserWidget* User = Cast<UDreamUserWidget>(Tree))
		{
			User->PlayAnimationByName(TEXT("Reveal"));
			bRevealStarted = true;
		}
	}

	// The rows' fade, on the schedule the panel's Reveal timeline slides them by (a track cannot drive RenderOpacity).
	struct FRowFade { const TCHAR* Widget; float From; float To; };
	static const FRowFade RowFades[] = {
		{ TEXT("HeaderMeta"), 0.10f, 0.38f },
		{ TEXT("Header"), 0.14f, 0.42f },
		{ TEXT("VolumeRow"), 0.22f, 0.50f },
		{ TEXT("SubtitleRow"), 0.26f, 0.54f },
		{ TEXT("Footer"), 0.30f, 0.58f },
	};
	for (const FRowFade& Row : RowFades)
	{
		if (UDreamWidget* Widget = Space->FindPanelWidget(Row.Widget))
		{
			Widget->SetRenderOpacity(static_cast<float>(EaseOutCubic((Since - Row.From) / (Row.To - Row.From))));
		}
	}

	if (UDreamPixelSort* Sort = FindPixelSort(Tree, TEXT("Sort")))
	{
		float Strength = 0.0f;
		if (Since >= SortRiseFrom && Since < SortPeakAt)
		{
			Strength = SortPeak * (Since - SortRiseFrom) / (SortPeakAt - SortRiseFrom);
		}
		else if (Since >= SortPeakAt && Since < SortGoneAt)
		{
			Strength = SortPeak * static_cast<float>(1.0 - EaseOutQuad((Since - SortPeakAt) / (SortGoneAt - SortPeakAt)));
		}
		// Strength 0 is the effect switched off, so a sliver is switched off rather than drawn.
		Sort->SetSortStrength(Strength < 0.02f ? 0.0f : Strength);
	}
}

void ANocturneDirector::UpdatePlay()
{
	APlayerController* Player = GetWorld()->GetFirstPlayerController();
	if (Player == nullptr)
	{
		return;
	}
	if (!bPlayInputSet)
	{
		// The mouse stays free over the UI; the OS cursor is there but drawn as nothing, so ours is the one seen.
		bPlayInputSet = true;
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Player->SetInputMode(Mode);
		Player->SetShowMouseCursor(true);
		Player->CurrentMouseCursor = EMouseCursor::None;
	}

	// The title: any key, a pad button or a click wakes it (its own key handling needs the keyboard's focus first).
	if (IsValid(Menu) && (Player->WasInputKeyJustPressed(EKeys::AnyKey) || Player->WasInputKeyJustPressed(EKeys::LeftMouseButton)
		|| Player->WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)))
	{
		Menu->PressAnyKey();
	}

	float X = 0.0f;
	float Y = 0.0f;
	// In the game there is no pointer: the HUD is looked at, not clicked.
	if (IsValid(Voyage) || !Player->GetMousePosition(X, Y))
	{
		if (IsValid(Cursor))
		{
			PlaceCursor(FVector2D(-200.0, -200.0));
		}
		return;
	}
	const FVector2D Position(X, Y);
	EnsureCursor();
	PlaceCursor(Position);
	const FVector2D Size = GetViewportSize();
	if (Size.X > 0.0 && Size.Y > 0.0)
	{
		const FVector2D Lean(Position.X / Size.X * 2.0 - 1.0, Position.Y / Size.Y * 2.0 - 1.0);
		if (UNocturneStarScreen* Screen = GetStarScreen())
		{
			Screen->SetParallaxTarget(Lean);
		}
		if (IsValid(Menu))
		{
			Menu->SetParallaxTarget(Lean);
		}
	}
}

void ANocturneDirector::UpdatePointer(float T)
{
	if (IsPlayShot())
	{
		UpdatePlay();
		return;
	}
	UDreamStandaloneInputModule* Module = GetInputModule();
	if (Module == nullptr || PointerKeys.Num() == 0)
	{
		return;
	}

	FVector2D Position(-100.0, -100.0);
	bool bPressed = false;
	// In the game there is no pointer: the HUD is looked at, not clicked.
	const bool bShown = T >= PointerKeys[0].Time && !IsValid(Voyage);
	if (bShown)
	{
		int32 Index = 0;
		while (Index + 1 < PointerKeys.Num() && PointerKeys[Index + 1].Time <= T)
		{
			++Index;
		}
		const FNocturnePointerKey& From = PointerKeys[Index];
		bPressed = From.bPressed;

		FVector2D A, B;
		bool bHaveA = ResolveKey(From, A);
		if (!bHaveA && bHaveLastPointer)
		{
			// The key's target is gone (a label faded out, a screen closed): the hand stays where it was.
			A = LastPointer;
			bHaveA = true;
		}
		if (Index + 1 < PointerKeys.Num() && bHaveA && ResolveKey(PointerKeys[Index + 1], B))
		{
			const FNocturnePointerKey& To = PointerKeys[Index + 1];
			const double Span = FMath::Max(static_cast<double>(To.Time - From.Time), 1e-4);
			const double Alpha = FMath::SmoothStep(0.0, 1.0, static_cast<double>(T - From.Time) / Span);
			Position = FMath::Lerp(A, B, Alpha);
			// A slight arc, as a hand moves; none while dragging, where the pointer follows the slider's track.
			if (!From.bPressed)
			{
				const FVector2D Along = B - A;
				const FVector2D Across(-Along.Y, Along.X);
				Position += Across * (0.08 * FMath::Sin(PI * Alpha));
			}
		}
		else if (bHaveA)
		{
			Position = A;
		}
	}

	if (bShown)
	{
		LastPointer = Position;
		bHaveLastPointer = true;
	}
	Module->SetOverridePointerPosition(Position);
	if (bPressed != bButtonDown)
	{
		bButtonDown = bPressed;
		Module->InputTrigger(FVector(Position.X, Position.Y, 0.0), bPressed);
	}

	if (bShown)
	{
		EnsureCursor();
		PlaceCursor(Position);
	}
	else if (IsValid(Cursor))
	{
		PlaceCursor(FVector2D(-200.0, -200.0));
	}

	// The settings panels lean a little toward the pointer, and the star map's camera with them.
	const FVector2D Size = GetViewportSize();
	if (bShown && Size.X > 0.0 && Size.Y > 0.0)
	{
		const FVector2D Lean(Position.X / Size.X * 2.0 - 1.0, Position.Y / Size.Y * 2.0 - 1.0);
		if (UNocturneStarScreen* Screen = GetStarScreen())
		{
			Screen->SetParallaxTarget(Lean);
		}
		if (IsValid(Menu))
		{
			Menu->SetParallaxTarget(Lean);
		}
	}
}

void ANocturneDirector::UpdateReadouts(float T)
{
	if (!IsValid(Space) || !IsTestShot())
	{
		return;
	}
	using namespace NocturneDirector;
	const float Since = T - RevealAt;
	const int32 Frame = FMath::FloorToInt(T * 30.0f);

	// English labels decode while their rows come in.
	if (Since >= 0.0f)
	{
		ScrambleDone.SetNum(Scrambles.Num());
		for (int32 Index = 0; Index < Scrambles.Num(); ++Index)
		{
			const FNocturneScramble& Item = Scrambles[Index];
			if (ScrambleDone[Index])
			{
				continue;
			}
			const float Progress = (Since - Item.Start) / Item.Duration;
			// Past the end, the final text is written once whatever step the frames take.
			SetWidgetText(Item.Widget, Progress >= 1.0f ? Item.Final : Scramble(Item.Final, Progress, Frame));
			ScrambleDone[Index] = Progress >= 1.0f;
		}
	}

	// The volume readout counts up during the entrance, then follows the slider.
	if (const UDreamSlider* Volume = Cast<UDreamSlider>(Space->FindPanelWidget(TEXT("Volume"))))
	{
		double Shown = Volume->GetValue() * 100.0;
		if (Since < CountFrom + CountSeconds)
		{
			Shown *= EaseOutCubic((Since - CountFrom) / CountSeconds);
		}
		SetWidgetText(TEXT("VolumeValue"), FString::Printf(TEXT("%03d"), FMath::RoundToInt(Shown)));
	}

	if (const UDreamToggle* Subtitles = Cast<UDreamToggle>(Space->FindPanelWidget(TEXT("Subtitles"))))
	{
		const bool bOn = Subtitles->IsChecked();
		SetWidgetText(TEXT("SubtitleState"), bOn ? TEXT("ON") : TEXT("OFF"), bOn ? &Cyan : &InkFaint);
	}
}

void ANocturneDirector::SetWidgetText(const FString& InWidget, const FString& InText, const FColor* InColor) const
{
	const UDreamWidget* Widget = IsValid(Space) ? Space->FindPanelWidget(InWidget) : nullptr;
	UDreamText* Text = Widget != nullptr ? Cast<UDreamText>(Widget->GetVisual()) : nullptr;
	if (Text == nullptr)
	{
		return;
	}
	if (!Text->GetText().ToString().Equals(InText, ESearchCase::CaseSensitive))
	{
		Text->SetText(FText::FromString(InText));
	}
	if (InColor != nullptr)
	{
		Text->SetColor(*InColor);
	}
}

UDreamPixelSort* ANocturneDirector::FindPixelSort(UDreamWidget* InTree, const FString& InName) const
{
	if (!IsValid(InTree))
	{
		return nullptr;
	}
	const UDreamWidget* Holder = nullptr;
	if (const UDreamUserWidget* User = Cast<UDreamUserWidget>(InTree))
	{
		Holder = User->GetWidgetFromName(FName(*InName));
	}
	if (Holder == nullptr)
	{
		Holder = InTree->FindChildByDisplayName(InName, true);
	}
	return Holder != nullptr ? Cast<UDreamPixelSort>(Holder->GetVisual()) : nullptr;
}

void ANocturneDirector::Measure(float DeltaSeconds)
{
	// The first second is loading and shader warm-up, not the shot; the performance shot is measured once every button turns.
	if (ShotTime > (ShotName == TEXT("Perf") ? 6.5f : 1.0f) && ShotTime <= NocturneDirector::ShotSeconds)
	{
		FrameMs.Add(DeltaSeconds * 1000.0f);
		GpuMs.Add(FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()));
		return;
	}
	if (ShotTime <= NocturneDirector::ShotSeconds || FrameMs.Num() == 0)
	{
		return;
	}
	auto Report = [](const TCHAR* Label, TArray<float> Values)
	{
		Values.Sort();
		double Sum = 0.0;
		for (float Value : Values)
		{
			Sum += Value;
		}
		const float Average = static_cast<float>(Sum / Values.Num());
		const float P95 = Values[FMath::Min(Values.Num() - 1, FMath::FloorToInt(Values.Num() * 0.95f))];
		UE_LOG(LogNocturne, Display, TEXT("Measure %s: frames=%d avg=%.2f ms (%.1f fps) p95=%.2f ms worst=%.2f ms"),
			Label, Values.Num(), Average, 1000.0f / Average, P95, Values.Last());
	};
	Report(TEXT("frame"), FrameMs);
	Report(TEXT("gpu"), GpuMs);
	FrameMs.Reset();
	bMeasure = false;
	if (APlayerController* Player = GetWorld()->GetFirstPlayerController())
	{
		Player->ConsoleCommand(TEXT("quit"));
	}
}

bool ANocturneDirector::ResolveKey(const FNocturnePointerKey& Key, FVector2D& OutViewport) const
{
	const UDreamWidget* Widget = nullptr;
	switch (Key.Target)
	{
	case ENocturneTarget::Viewport:
		OutViewport = Key.Spot;
		return true;
	case ENocturneTarget::Panel:
		Widget = IsValid(Space) ? Space->FindPanelWidget(Key.Widget) : nullptr;
		break;
	case ENocturneTarget::Screen:
		Widget = GetStarScreen() != nullptr ? GetStarScreen()->Node(*Key.Widget) : nullptr;
		break;
	case ENocturneTarget::ScreenRow:
		Widget = GetSettingsScreen() != nullptr ? GetSettingsScreen()->GetRow(Key.Page, Key.Row) : nullptr;
		break;
	case ENocturneTarget::StarLabel:
		// Only while the map is up: a label fading away is not something to point at.
		if (IsValid(Menu) && Menu->IsOnMap())
		{
			const UNocturneStarLabel* Label = Menu->GetStarLabel(Key.Page);
			Widget = Label != nullptr ? Label->Node(*Key.Widget) : nullptr;
		}
		break;
	case ENocturneTarget::Menu:
		Widget = IsValid(Menu) && Menu->GetMenuWidget() != nullptr ? Menu->GetMenuWidget()->Node(*Key.Widget) : nullptr;
		break;
	}
	if (Widget == nullptr)
	{
		return false;
	}
	const FVector2D LeftBottom = Widget->GetLocalSpaceLeftBottomPoint();
	const FVector2D RightTop = Widget->GetLocalSpaceRightTopPoint();
	const FVector2D Local(FMath::Lerp(LeftBottom.X, RightTop.X, Key.Spot.X), FMath::Lerp(LeftBottom.Y, RightTop.Y, Key.Spot.Y));
	const FVector World = Widget->GetWorldTransform().TransformPosition(FVector(0.0, Local.X, Local.Y));
	if (Key.Target == ENocturneTarget::Panel || Key.Target == ENocturneTarget::StarLabel)
	{
		return ProjectToViewport(World, OutViewport);
	}
	// A screen-space widget: through its canvas's own perspective camera, then onto the viewport.
	const UDreamCanvas* Canvas = Widget->GetRenderCanvas();
	const UDreamCanvas* Root = Canvas != nullptr ? Canvas->GetRootCanvas() : nullptr;
	FVector2D OnCanvas;
	return Root != nullptr && Root->Project3DToScreen(World, OnCanvas) && Root->ConvertPositionFromCanvasToViewport(OnCanvas, OutViewport);
}

bool ANocturneDirector::ProjectToViewport(const FVector& InWorld, FVector2D& OutViewport) const
{
	const FVector2D Size = GetViewportSize();
	if (Size.X <= 0.0 || Size.Y <= 0.0)
	{
		return false;
	}
	// The camera this frame, not the player camera manager's copy from the last one.
	FMinimalViewInfo View;
	GetActiveCamera()->GetCameraView(0.0f, View);
	View.AspectRatio = Size.X / Size.Y;
	FMatrix ViewMatrix, ProjectionMatrix, ViewProjection;
	UGameplayStatics::GetViewProjectionMatrix(View, ViewMatrix, ProjectionMatrix, ViewProjection);
	return FSceneView::ProjectWorldToScreen(InWorld, FIntRect(0, 0, FMath::RoundToInt(Size.X), FMath::RoundToInt(Size.Y)), ViewProjection, OutViewport);
}

FVector2D ANocturneDirector::GetViewportSize() const
{
	FVector2D Size(0.0, 0.0);
	if (const UGameViewportClient* Viewport = GetWorld()->GetGameViewport())
	{
		Viewport->GetViewportSize(Size);
	}
	return Size;
}

UDreamStandaloneInputModule* ANocturneDirector::GetInputModule()
{
	if (UDreamStandaloneInputModule* Known = InputModule.Get())
	{
		return Known;
	}
	const UDreamUIInputSubsystem* Input = UDreamUIInputSubsystem::Get(this);
	const UDreamUIInputUser* User = Input != nullptr ? Input->GetUser(0) : nullptr;
	UDreamStandaloneInputModule* Module = User != nullptr ? Cast<UDreamStandaloneInputModule>(User->GetInputModule()) : nullptr;
	if (Module != nullptr)
	{
		// From here on the pointer is the director's, whatever the OS mouse does -- except in play, where it is the player's.
		if (!IsPlayShot())
		{
			Module->SetOverrideMousePosition(true);
		}
		InputModule = Module;
		UE_LOG(LogNocturne, Log, TEXT("Director: driving %s"), *Module->GetName());
	}
	return Module;
}

void ANocturneDirector::EnsureCursor()
{
	if (IsValid(Cursor) || LoadedCursorClass == nullptr)
	{
		return;
	}
	UDreamScreenUISubsystem* ScreenUI = UDreamScreenUISubsystem::Get(GetWorld());
	UDreamWidget* ScreenRoot = IsValid(ScreenUI) ? ScreenUI->GetOrCreateScreenRootForUserIndex(0) : nullptr;
	if (!IsValid(ScreenRoot))
	{
		return;
	}
	Cursor = CreateDreamWidget(GetWorld(), LoadedCursorClass, ScreenRoot);
	if (!IsValid(Cursor))
	{
		return;
	}
	// Above every other layer, whatever sort order a screen picks.
	UDreamCanvas* Canvas = Cursor->GetComponent<UDreamCanvas>();
	if (Canvas == nullptr)
	{
		Canvas = Cursor->AddComponent<UDreamCanvas>();
	}
	if (Canvas != nullptr)
	{
		Canvas->SetOverrideSorting(true);
		Canvas->SetSortOrder(10000, true);
	}
}

void ANocturneDirector::PlaceCursor(const FVector2D& InViewport)
{
	if (!IsValid(Cursor))
	{
		return;
	}
	UDreamWidget* ScreenRoot = Cursor->GetParent();
	UDreamCanvas* RootCanvas = IsValid(ScreenRoot) ? ScreenRoot->GetComponent<UDreamCanvas>() : nullptr;
	FVector2D InCanvas;
	if (RootCanvas != nullptr && RootCanvas->ConvertPositionFromViewportToCanvas(InViewport, InCanvas))
	{
		// Bottom-left origin out of the conversion, centre origin into the anchored position.
		InCanvas -= FVector2D(ScreenRoot->GetWidth() * 0.5, ScreenRoot->GetHeight() * 0.5);
		Cursor->SetAnchoredPosition(InCanvas);
	}
}
