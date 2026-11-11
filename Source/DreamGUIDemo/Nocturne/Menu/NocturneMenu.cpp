// Copyright DreamMoon. All Rights Reserved.

#include "NocturneMenu.h"

#include "NocturneMainMenu.h"
#include "NocturneStarLabel.h"
#include "../NocturneCamera.h"
#include "../NocturneSpace.h"
#include "../NocturneStars.h"
#include "../Screens/NocturneDepartScreen.h"
#include "../Screens/NocturneStarScreen.h"
#include "../Screens/NocturneTitleScreen.h"
#include "../Settings/NocturneSettingsScreen.h"

#include "Camera/CameraComponent.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWorldWidgetComponent.h"
#include "DreamUIBPLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogNocturneMenu, Log, All);

namespace NocturneMenuLocal
{
	// A label's design size (Nocturne_StarLabel.dui), the point on it that sits on the star (the ring's centre), and how
	// big one design unit is in the world.
	const FVector2D LabelDesignSize(460.0, 190.0);
	const FVector2D LabelPivot(60.0 / 460.0, 0.5);
	constexpr double LabelScale = 0.8;

	// The title: far back and looking up at the moon (the sky material's MoonDirection), the constellation below the
	// frame. The moon lands above the wordmark, between it and the administrator's card.
	const FVector TitleBack(-1150.0, 0.0, 250.0);
	const FRotator TitleLook(15.4, -21.0, 0.0);
	constexpr float WakeSeconds = 2.6f;
	// The lines draw in during the wake, one after another, from this far into it.
	constexpr float LinesAt = 1.2f;
	constexpr float LineStagger = 0.14f;
	constexpr float LineSeconds = 0.3f;

	// The map from afar: where the camera looks, how far back it starts when the menu opens, how it drifts and leans.
	const FVector OverviewLookAhead(1800.0, 0.0, 0.0);
	const FVector IntroBack(-320.0, 0.0, 40.0);
	constexpr float IntroSeconds = 2.4f;
	const FVector ParallaxReach(0.0, 70.0, 40.0);
	constexpr float ParallaxRate = 3.0f;

	// A star close up, from the star: the camera, and where it looks. Most stars are seen as the settings shot sees
	// them, low on the left behind the panels; a screen built round its star sees it straight on in the middle.
	const FVector VisitOffset(-350.0, 180.0, -42.0);
	const FVector VisitLookOffset(-20.0, 130.0, -48.0);
	const FVector CenteredVisitOffset(-520.0, 0.0, 0.0);
	// How lit the visited star is: fully at the side, less in the middle, where the screen sits over it.
	constexpr float VisitLit = 1.0f;
	constexpr float CenteredVisitLit = 0.25f;

	// The flights. A star's screen comes in a little before the camera settles; the map's chrome and labels come back a
	// little before it is home.
	constexpr float FlySeconds = 1.5f;
	constexpr float ReturnSeconds = 1.4f;
	constexpr float ReturnDelay = 0.12f;
	constexpr float ScreenLead = 0.25f;
	constexpr float ChromeLead = 0.45f;
	constexpr float ScreenRemoveDelay = 0.36f;
	const FVector FlightLift(0.0, -110.0, 60.0);

	// The launch: a dive straight ahead, speeding up.
	constexpr float DiveDelay = 0.2f;
	constexpr float DiveSeconds = 1.3f;
	constexpr double DiveDistance = 3200.0;

	// When the focused label opens, and when the map starts taking input, after it comes up; how fast a star lights.
	constexpr float FocusRevealAt = 1.3f;
	constexpr float InputFrom = 1.0f;
	constexpr float FocusRate = 9.0f;

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

	FVector Bezier(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, double T)
	{
		const double U = 1.0 - T;
		return U * U * U * P0 + 3.0 * U * U * T * P1 + 3.0 * U * T * T * P2 + T * T * T * P3;
	}

	/** A slow drift, so the frame never quite stops. */
	FVector Drift(float T)
	{
		return FVector(0.0, FMath::Sin(T * 0.45) * 6.0, FMath::Sin(T * 0.33) * 4.0);
	}
}

ANocturneMenu::ANocturneMenu()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	NocturneCamera::Configure(Camera);

	for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
	{
		UDreamWorldWidgetComponent* Label = CreateDefaultSubobject<UDreamWorldWidgetComponent>(*FString::Printf(TEXT("Label%d"), Index));
		Label->SetupAttachment(Root);
		Label->SetRelativeLocation(NocturneStars::Stars[Index].Location);
		Label->SetRelativeScale3D(FVector(NocturneMenuLocal::LabelScale));
		Labels.Add(Label);
	}

	LabelClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/Components/WBP_Nocturne_StarLabel.WBP_Nocturne_StarLabel_C")));
	MenuClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_MainMenu.WBP_Nocturne_MainMenu_C")));
	TitleClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_Title.WBP_Nocturne_Title_C")));
}

void ANocturneMenu::BeginPlay()
{
	using namespace NocturneMenuLocal;
	UClass* LoadedLabel = LabelClass.LoadSynchronous();
	for (UDreamWorldWidgetComponent* Label : Labels)
	{
		Label->SetBackend(EDreamWorldWidgetBackend::DreamUIRenderer);
		Label->SetDrawSize(LabelDesignSize);
		Label->SetPivot(LabelPivot);
		Label->SetWidgetClass(LoadedLabel);
	}

	Super::BeginPlay();

	FocusAmount.Init(0.0f, NocturneStars::Count);
	LabelBound.Init(false, NocturneStars::Count);

	if (UClass* LoadedMenu = MenuClass.LoadSynchronous())
	{
		// Over the 3D view and the screen effects (100), under the title (160), a star's screen (200) and the cursor.
		MenuWidget = Cast<UNocturneMainMenu>(UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, LoadedMenu, 150));
	}
	if (IsValid(MenuWidget))
	{
		MenuWidget->OnEntryFocused.AddWeakLambda(this, [this](int32 Index) { FocusStar(Index); });
		MenuWidget->OnEntryChosen.AddWeakLambda(this, [this](int32 Index) { ChooseStar(Index); });
	}

	if (bStartAtTitle)
	{
		State = EMenuState::Title;
		bPushIn = false;
		if (UClass* LoadedTitle = TitleClass.LoadSynchronous())
		{
			Title = Cast<UNocturneTitleScreen>(UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, LoadedTitle, 160));
		}
		if (IsValid(Title))
		{
			Title->OnWake.AddWeakLambda(this, [this]() { PressAnyKey(); });
			Title->PlayEntrance();
		}
		if (IsValid(MenuWidget))
		{
			MenuWidget->HideNow();
		}
		// No labels and no lines until the system wakes.
		LabelFadeFrom = LabelFadeTo = 0.0f;
		if (IsValid(Space))
		{
			for (int32 Line = 0; Line < NocturneStars::LineCount; ++Line)
			{
				Space->SetLineReveal(Line, 0.0f);
			}
		}
	}
	else if (IsValid(MenuWidget))
	{
		MenuWidget->PlayEntrance(0.35f);
	}
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: chrome %s, label class %s, %s"), IsValid(MenuWidget) ? TEXT("up") : TEXT("FAILED"),
		LoadedLabel != nullptr ? TEXT("loaded") : TEXT("FAILED"), bStartAtTitle ? (IsValid(Title) ? TEXT("title up") : TEXT("title FAILED")) : TEXT("no title"));

	BindLabels();
	ApplyFocus();
	UpdateCamera(0.0f);
	UpdateLabels();
}

UNocturneStarLabel* ANocturneMenu::GetStarLabel(int32 InIndex) const
{
	return Labels.IsValidIndex(InIndex) ? Cast<UNocturneStarLabel>(Labels[InIndex]->GetLoadedWidget()) : nullptr;
}

UNocturneSettingsScreen* ANocturneMenu::GetSettings() const
{
	return Cast<UNocturneSettingsScreen>(Screen);
}

bool ANocturneMenu::IsOnMap() const
{
	return State == EMenuState::Overview;
}

void ANocturneMenu::BindLabels()
{
	for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
	{
		UNocturneStarLabel* Label = LabelBound[Index] ? nullptr : GetStarLabel(Index);
		if (Label == nullptr)
		{
			continue;
		}
		Label->Setup(Index);
		Label->OnHovered.AddWeakLambda(this, [this](UNocturneStarLabel* InLabel) { FocusStar(InLabel->GetIndex()); });
		Label->OnClicked.AddWeakLambda(this, [this](UNocturneStarLabel* InLabel) { ChooseStar(InLabel->GetIndex()); });
		if (!bStartAtTitle)
		{
			// The rings come up one after another, from the first star on.
			Label->PlayIntro(0.5f + 0.09f * Index);
		}
		LabelBound[Index] = true;
	}
}

void ANocturneMenu::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	MenuTime += DeltaSeconds;

	BindLabels();
	const bool bMapUp = State == EMenuState::Overview || State == EMenuState::Waking;
	if (!bFocusRevealed && bMapUp && MenuTime >= MapFrom + NocturneMenuLocal::FocusRevealAt)
	{
		bFocusRevealed = true;
		ApplyFocus();
	}
	// DreamGUI gives the focus only to a widget that has been drawn.
	if (bKeyboardFocusPending && State == EMenuState::Overview && MenuTime > MapFrom + 0.1f && IsValid(MenuWidget))
	{
		bKeyboardFocusPending = false;
		MenuWidget->FocusEntry(Focused);
	}
	if (RemoveScreenAt >= 0.0f && MenuTime >= RemoveScreenAt)
	{
		RemoveScreenAt = -1.0f;
		if (IsValid(Screen))
		{
			UDreamUIBPLibrary::RemoveFromViewport(this, Screen);
			Screen = nullptr;
		}
	}
	if (RemoveTitleAt >= 0.0f && MenuTime >= RemoveTitleAt)
	{
		RemoveTitleAt = -1.0f;
		if (IsValid(Title))
		{
			UDreamUIBPLibrary::RemoveFromViewport(this, Title);
			Title = nullptr;
		}
	}

	UpdateCamera(DeltaSeconds);
	UpdateLabels();
	UpdateFocus(DeltaSeconds);
	UpdateLines();
}

ANocturneMenu::FPose ANocturneMenu::TitlePose() const
{
	using namespace NocturneMenuLocal;
	FPose Pose;
	Pose.Location = NocturneStars::OverviewCamera + TitleBack + Drift(MenuTime) * 0.6;
	Pose.Look = Pose.Location + TitleLook.Vector() * 1000.0;
	return Pose;
}

ANocturneMenu::FPose ANocturneMenu::OverviewPose() const
{
	using namespace NocturneMenuLocal;
	FPose Pose;
	Pose.Location = NocturneStars::OverviewCamera + Drift(MenuTime)
		+ FVector(0.0, Parallax.X * ParallaxReach.Y, -Parallax.Y * ParallaxReach.Z);
	Pose.Look = NocturneStars::OverviewCamera + OverviewLookAhead;
	if (bPushIn && MenuTime < IntroSeconds)
	{
		// The menu opens on a push in from further out.
		Pose.Location += IntroBack * (1.0 - EaseOutCubic(MenuTime / IntroSeconds));
	}
	return Pose;
}

ANocturneMenu::FPose ANocturneMenu::VisitPose(int32 InIndex) const
{
	using namespace NocturneMenuLocal;
	const int32 Star = FMath::Clamp(InIndex, 0, NocturneStars::Count - 1);
	const FVector Where = Space != nullptr ? Space->GetStarLocation(Star) : NocturneStars::Stars[Star].Location;
	FPose Pose;
	if (NocturneStars::Stars[Star].bVisitCentered)
	{
		Pose.Location = Where + CenteredVisitOffset;
		Pose.Look = Where;
	}
	else
	{
		Pose.Location = Where + VisitOffset;
		Pose.Look = Where + VisitLookOffset;
	}
	// Once there, the same slow drift as from afar, eased in from the arrival.
	const double Settle = FMath::Clamp((MenuTime - ArrivedAt) / 1.5, 0.0, 1.0);
	Pose.Location += Drift(MenuTime) * 0.5 * Settle;
	return Pose;
}

ANocturneMenu::FPose ANocturneMenu::FlightPose(const FPose& From, const FPose& To, double Alpha, bool bOutbound) const
{
	using namespace NocturneMenuLocal;
	const double A = EaseInOutCubic(Alpha);
	const FVector Along = To.Location - From.Location;
	// An arc, not a line: up and across on the way out, the same arc the other way home.
	const FVector P1 = From.Location + Along * (bOutbound ? 0.3 : 0.25) + FlightLift * (bOutbound ? 1.0 : 0.4);
	const FVector P2 = From.Location + Along * (bOutbound ? 0.75 : 0.7) + FlightLift * (bOutbound ? 0.4 : 1.0);
	FPose Pose;
	Pose.Location = Bezier(From.Location, P1, P2, To.Location, A);
	// The look turns a little ahead of the travel, so the star leads the way.
	Pose.Look = FMath::Lerp(From.Look, To.Look, EaseInOutCubic(Alpha / 0.85));
	return Pose;
}

void ANocturneMenu::UpdateCamera(float DeltaSeconds)
{
	using namespace NocturneMenuLocal;
	Parallax += (ParallaxTarget - Parallax) * (1.0f - FMath::Exp(-DeltaSeconds * ParallaxRate));

	FPose Pose;
	switch (State)
	{
	case EMenuState::Title:
		Pose = TitlePose();
		break;

	case EMenuState::Waking:
	{
		const double Alpha = (MenuTime - FlightStart) / WakeSeconds;
		// Down from the moon to wherever the map's camera is by then.
		Pose = FlightPose(FlightFrom, OverviewPose(), Alpha, false);
		if (!bChromeBack && MenuTime >= FlightStart + WakeSeconds - ChromeLead)
		{
			bChromeBack = true;
			FadeLabels(true, 0.0f);
			if (IsValid(MenuWidget))
			{
				MenuWidget->PlayEntrance();
			}
		}
		if (Alpha >= 1.0)
		{
			EnterMap();
		}
		break;
	}

	case EMenuState::Overview:
		Pose = OverviewPose();
		break;

	case EMenuState::Flying:
	{
		const double Alpha = (MenuTime - FlightStart) / FlySeconds;
		Pose = FlightPose(FlightFrom, FlightTo, Alpha, true);
		if (!bScreenOpened && MenuTime >= FlightStart + FlySeconds - ScreenLead)
		{
			bScreenOpened = true;
			OpenStarScreen();
		}
		if (Alpha >= 1.0)
		{
			State = EMenuState::AtStar;
		}
		break;
	}

	case EMenuState::AtStar:
		Pose = VisitPose(Visiting);
		break;

	case EMenuState::Returning:
	{
		if (MenuTime < FlightStart)
		{
			Pose = VisitPose(Visiting);
			break;
		}
		if (!bFlightCaptured)
		{
			bFlightCaptured = true;
			FlightFrom = Current;
		}
		const double Alpha = (MenuTime - FlightStart) / ReturnSeconds;
		// Home is wherever the map's camera is by then: the drift and the lean go on while it flies.
		Pose = FlightPose(FlightFrom, OverviewPose(), Alpha, false);
		if (!bChromeBack && MenuTime >= FlightStart + ReturnSeconds - ChromeLead)
		{
			bChromeBack = true;
			FadeLabels(true, 0.0f);
			if (IsValid(MenuWidget))
			{
				MenuWidget->PlayEntrance();
			}
		}
		if (Alpha >= 1.0)
		{
			State = EMenuState::Overview;
			Visiting = INDEX_NONE;
			bKeyboardFocusPending = true;
			ApplyFocus();
		}
		break;
	}

	case EMenuState::Launching:
	case EMenuState::Launched:
	{
		if (MenuTime < FlightStart)
		{
			Pose = VisitPose(Visiting);
			break;
		}
		if (!bFlightCaptured)
		{
			bFlightCaptured = true;
			FlightFrom = Current;
		}
		// Straight ahead, faster and faster.
		const FVector Ahead = (FlightFrom.Look - FlightFrom.Location).GetSafeNormal();
		const double Alpha = FMath::Clamp((MenuTime - FlightStart) / DiveSeconds, 0.0, 1.0);
		Pose.Location = FlightFrom.Location + Ahead * (DiveDistance * Alpha * Alpha * Alpha);
		Pose.Look = Pose.Location + Ahead * 1000.0;
		if (State == EMenuState::Launching && Alpha >= 1.0)
		{
			State = EMenuState::Launched;
			UE_LOG(LogNocturneMenu, Log, TEXT("Menu: launched with slot %d"), LaunchSlot);
			OnLaunched.Broadcast(LaunchSlot);
		}
		break;
	}
	}

	Camera->SetWorldLocationAndRotation(Pose.Location, (Pose.Look - Pose.Location).Rotation());
	Current = Pose;
	if (bHaveLast && DeltaSeconds > 0.0f)
	{
		CameraSpeed = (Pose.Location - LastLocation).Size() / DeltaSeconds;
	}
	LastLocation = Pose.Location;
	bHaveLast = true;
}

void ANocturneMenu::EnterMap()
{
	State = EMenuState::Overview;
	bKeyboardFocusPending = true;
	ApplyFocus();
}

void ANocturneMenu::PressAnyKey()
{
	if (State != EMenuState::Title)
	{
		return;
	}
	// Out of the title first: Wake below calls back here through OnWake.
	State = EMenuState::Waking;
	if (IsValid(Title))
	{
		Title->Wake();
		Title->PlayExit();
		RemoveTitleAt = MenuTime + 0.6f;
	}
	FlightStart = MenuTime;
	FlightFrom = Current;
	bChromeBack = false;
	LinesFrom = MenuTime + NocturneMenuLocal::LinesAt;
	// The map's clock starts as its chrome comes in: the focused label opens and input opens up from there.
	MapFrom = MenuTime + NocturneMenuLocal::WakeSeconds - NocturneMenuLocal::ChromeLead;
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: waking"));
}

void ANocturneMenu::UpdateLines()
{
	if (LinesFrom < 0.0f || !IsValid(Space))
	{
		return;
	}
	using namespace NocturneMenuLocal;
	bool bAllDrawn = true;
	for (int32 Line = 0; Line < NocturneStars::LineCount; ++Line)
	{
		const float Amount = static_cast<float>(EaseOutCubic((MenuTime - LinesFrom - LineStagger * Line) / LineSeconds));
		Space->SetLineReveal(Line, Amount);
		bAllDrawn &= Amount >= 1.0f;
	}
	if (bAllDrawn)
	{
		LinesFrom = -1.0f;
	}
}

void ANocturneMenu::UpdateLabels()
{
	const double Alpha = LabelFadeSeconds > 0.0f ? FMath::Clamp((MenuTime - LabelFadeStart) / LabelFadeSeconds, 0.0, 1.0) : 1.0;
	const float Opacity = FMath::Lerp(LabelFadeFrom, LabelFadeTo, static_cast<float>(Alpha));
	const FRotator Facing = Camera->GetComponentRotation();
	for (UDreamWorldWidgetComponent* Label : Labels)
	{
		// Square to the camera, so every label reads flat whichever way the camera turns.
		Label->SetWorldRotation(Facing);
		if (!FMath::IsNearlyEqual(Label->GetWidgetOpacity(), Opacity, 1e-3f))
		{
			Label->SetWidgetOpacity(Opacity);
		}
		// Faded out is gone: nothing left to point at while the camera is away.
		const bool bVisible = Opacity > 0.0f || LabelFadeTo > 0.0f;
		if (Label->GetWidgetVisible() != bVisible)
		{
			Label->SetWidgetVisible(bVisible);
		}
	}
}

void ANocturneMenu::FadeLabels(bool bIn, float InDelay)
{
	const double Alpha = LabelFadeSeconds > 0.0f ? FMath::Clamp((MenuTime - LabelFadeStart) / LabelFadeSeconds, 0.0, 1.0) : 1.0;
	LabelFadeFrom = FMath::Lerp(LabelFadeFrom, LabelFadeTo, static_cast<float>(Alpha));
	LabelFadeTo = bIn ? 1.0f : 0.0f;
	LabelFadeStart = MenuTime + InDelay;
	LabelFadeSeconds = bIn ? 0.3f : 0.18f;
	if (bIn)
	{
		for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
		{
			if (UNocturneStarLabel* Label = GetStarLabel(Index))
			{
				Label->PlayIntro(InDelay + 0.06f * Index);
			}
		}
	}
}

void ANocturneMenu::UpdateFocus(float DeltaSeconds)
{
	if (!IsValid(Space))
	{
		return;
	}
	using namespace NocturneMenuLocal;
	const bool bOnMap = State == EMenuState::Overview || ((State == EMenuState::Returning || State == EMenuState::Waking) && bChromeBack);
	const bool bAway = State == EMenuState::Flying || State == EMenuState::AtStar || State == EMenuState::Returning
		|| State == EMenuState::Launching || State == EMenuState::Launched;
	for (int32 Index = 0; Index < FocusAmount.Num(); ++Index)
	{
		float Target = 0.0f;
		if (bOnMap)
		{
			Target = bFocusRevealed && Index == Focused ? 1.0f : 0.0f;
		}
		else if (bAway && Index == Visiting)
		{
			Target = NocturneStars::Stars[Index].bVisitCentered ? CenteredVisitLit : VisitLit;
		}
		FocusAmount[Index] += (Target - FocusAmount[Index]) * (1.0f - FMath::Exp(-DeltaSeconds * FocusRate));
		Space->SetStarFocus(Index, FocusAmount[Index]);
	}
}

void ANocturneMenu::ApplyFocus()
{
	for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
	{
		if (UNocturneStarLabel* Label = GetStarLabel(Index))
		{
			Label->SetFocused(bFocusRevealed && Index == Focused);
		}
	}
	if (IsValid(MenuWidget))
	{
		MenuWidget->SetFocused(Focused);
	}
}

bool ANocturneMenu::AcceptsInput() const
{
	return State == EMenuState::Overview && MenuTime >= MapFrom + NocturneMenuLocal::InputFrom;
}

void ANocturneMenu::FocusStar(int32 InIndex)
{
	if (!AcceptsInput() || InIndex == Focused || InIndex < 0 || InIndex >= NocturneStars::Count)
	{
		return;
	}
	Focused = InIndex;
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: focus star %d"), InIndex);
	// The pointer's choice is the keyboard's too: Down from here goes to the next star.
	bFocusRevealed = true;
	ApplyFocus();
	if (IsValid(MenuWidget))
	{
		MenuWidget->FocusEntry(Focused);
	}
}

void ANocturneMenu::ChooseStar(int32 InIndex)
{
	if (!AcceptsInput() || InIndex < 0 || InIndex >= NocturneStars::Count)
	{
		return;
	}
	FocusStar(InIndex);
	if (!NocturneStars::Stars[InIndex].bOpen)
	{
		// Refused where it was asked: the label knocks, its tag and the entry's flash amber.
		if (UNocturneStarLabel* Label = GetStarLabel(InIndex))
		{
			Label->FlashLocked();
		}
		if (IsValid(MenuWidget))
		{
			MenuWidget->FlashLocked(InIndex);
		}
		UE_LOG(LogNocturneMenu, Log, TEXT("Menu: star %d is not open"), InIndex);
		return;
	}
	Visiting = InIndex;
	State = EMenuState::Flying;
	FlightStart = MenuTime;
	ArrivedAt = MenuTime + NocturneMenuLocal::FlySeconds;
	FlightFrom = Current;
	FlightTo = VisitPose(InIndex);
	bScreenOpened = false;
	FadeLabels(false, 0.0f);
	if (IsValid(MenuWidget))
	{
		MenuWidget->PlayExit();
	}
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: flying to star %d"), InIndex);
}

void ANocturneMenu::OpenStarScreen()
{
	if (Visiting < 0 || Visiting >= NocturneStars::Count)
	{
		return;
	}
	const FString Name = NocturneStars::Stars[Visiting].Screen;
	const TSoftClassPtr<UDreamUserWidget> ScreenClass(FSoftObjectPath(FString::Printf(TEXT("/Game/Nocturne/UI/%s.%s_C"), *Name, *Name)));
	if (UClass* Loaded = ScreenClass.LoadSynchronous())
	{
		Screen = Cast<UNocturneStarScreen>(UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, Loaded, 200));
	}
	if (IsValid(Screen))
	{
		Screen->OnBackRequested.AddUObject(this, &ANocturneMenu::Back);
		if (UNocturneDepartScreen* Depart = Cast<UNocturneDepartScreen>(Screen))
		{
			Depart->OnLaunch.AddUObject(this, &ANocturneMenu::Launch);
		}
		Screen->PlayEntrance();
	}
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: %s %s"), *Name, IsValid(Screen) ? TEXT("open") : TEXT("FAILED to open"));
}

void ANocturneMenu::Back()
{
	if (State != EMenuState::AtStar)
	{
		return;
	}
	if (IsValid(Screen))
	{
		Screen->PlayExit();
		RemoveScreenAt = MenuTime + NocturneMenuLocal::ScreenRemoveDelay;
	}
	State = EMenuState::Returning;
	FlightStart = MenuTime + NocturneMenuLocal::ReturnDelay;
	bFlightCaptured = false;
	bChromeBack = false;
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: back to the map"));
}

void ANocturneMenu::Launch(int32 InSlot)
{
	if (State != EMenuState::AtStar)
	{
		return;
	}
	LaunchSlot = InSlot;
	if (IsValid(Screen))
	{
		Screen->PlayExit();
		RemoveScreenAt = MenuTime + NocturneMenuLocal::ScreenRemoveDelay;
	}
	State = EMenuState::Launching;
	FlightStart = MenuTime + NocturneMenuLocal::DiveDelay;
	bFlightCaptured = false;
	UE_LOG(LogNocturneMenu, Log, TEXT("Menu: launching slot %d"), InSlot);
}
