#include "NieRUITestGameMode.h"

#include "Core/Components/DreamCanvas.h"
#include "Core/DreamScreenUISubsystem.h"
#include "Core/DreamUserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "NieRScreen.h"
#include "TimerManager.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

ANieRUITestGameMode::ANieRUITestGameMode()
{
	DefaultPawnClass = nullptr;
}

void ANieRUITestGameMode::BeginPlay()
{
	Super::BeginPlay();
	// A screen's first draw of new text can make one long frame. Tweens move at most a 30th of a second a frame, so the
	// entrances play on through such a frame instead of arriving at their end in it. Any ini or console setting wins.
	if (IConsoleVariable* MaxStep = IConsoleManager::Get().FindConsoleVariable(TEXT("DreamTween.MaxStepSeconds")))
	{
		MaxStep->Set(1.0f / 30.0f, ECVF_SetByGameSetting);
	}
	// A frame later: the local player's controller is in place by then however the map was opened.
	GetWorldTimerManager().SetTimerForNextTick(this, &ANieRUITestGameMode::ShowStartScreen);
	if (FPlatformMisc::SupportsTouchInput() || FParse::Param(FCommandLine::Get(), TEXT("DemoTouch")))
	{
		MountTouchBar();
	}
}

void ANieRUITestGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
	if (TouchViewport.IsValid() && TouchBar.IsValid())
	{
		TouchViewport->RemoveViewportWidgetContent(TouchBar.ToSharedRef());
	}
	TouchBar.Reset();
	TouchViewport.Reset();
	Super::EndPlay(Reason);
}

void ANieRUITestGameMode::MountTouchBar()
{
	UGameViewportClient* Viewport = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr;
	if (Viewport == nullptr)
	{
		return;
	}
	// The menu's own colours, given in sRGB as the art is (Slate's colours are linear): dark ink on a parchment fill.
	const FLinearColor Ink = FLinearColor::FromSRGBColor(FColor(72, 68, 58));
	FLinearColor Paper = FLinearColor::FromSRGBColor(FColor(218, 212, 192));
	Paper.A = 0.92f;
	static FButtonStyle Style = FButtonStyle()
		.SetNormal(FSlateRoundedBoxBrush(Paper, 4.0f, Ink, 2.0f))
		.SetHovered(FSlateRoundedBoxBrush(Paper, 4.0f, Ink, 2.0f))
		.SetPressed(FSlateRoundedBoxBrush(Ink, 4.0f, Ink, 2.0f))
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f));
	// A column on the right edge, which every NieR screen leaves empty: along the bottom it covered the hint bar.
	TSharedRef<SVerticalBox> Column = SNew(SVerticalBox);
	auto Add = [&Column, Ink](const FText& Label, const FKey& Key, int32 FontSize)
	{
		// Through FSlateApplication like a real keyboard, so DreamGUI's input routes it to the screen on top.
		auto Send = [Key](bool bDown)
		{
			FSlateApplication& Slate = FSlateApplication::Get();
			const FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
			bDown ? Slate.ProcessKeyDownEvent(Event) : Slate.ProcessKeyUpEvent(Event);
		};
		Column->AddSlot().AutoHeight().Padding(0.0f, 8.0f)
		[
			SNew(SBox).WidthOverride(132.0f).HeightOverride(88.0f)
			[
				SNew(SButton)
				.IsFocusable(false)
				.ButtonStyle(&Style)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnPressed_Lambda([Send]() { Send(true); })
				.OnReleased_Lambda([Send]() { Send(false); })
				[
					SNew(STextBlock).Text(Label).Font(FCoreStyle::GetDefaultFontStyle("Bold", FontSize)).ColorAndOpacity(Ink)
				]
			]
		];
	};
	Add(NSLOCTEXT("NieRTouch", "Ok", "确定"), EKeys::Enter, 22);
	Add(NSLOCTEXT("NieRTouch", "Back", "返回"), EKeys::BackSpace, 22);
	Add(NSLOCTEXT("NieRTouch", "Left", "◀"), EKeys::Left, 34);
	Add(NSLOCTEXT("NieRTouch", "Right", "▶"), EKeys::Right, 34);
	TouchBar = SNew(SBox)
		.Visibility(EVisibility::SelfHitTestInvisible)
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(FMargin(0.0f, 0.0f, 36.0f, 0.0f))
		[
			Column
		];
	// Under the gallery's own buttons (10000), over the screens.
	Viewport->AddViewportWidgetContent(TouchBar.ToSharedRef(), 8000);
	TouchViewport = Viewport;
}

void ANieRUITestGameMode::ShowStartScreen()
{
	UWorld* World = GetWorld();
	APlayerController* Player = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	UDreamScreenUISubsystem* ScreenUI = World != nullptr ? UDreamScreenUISubsystem::Get(World) : nullptr;
	if (Player == nullptr || ScreenUI == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("NieR UI test: no player or no DreamGUI screen subsystem"));
		return;
	}
	Player->SetShowMouseCursor(true);
	ScreenUI->GetOrCreateScreenRoot(Player);
	if (UDreamCanvas* Canvas = ScreenUI->GetScreenCanvas(Player))
	{
		Canvas->SetScaleMode(EDreamCanvasScaleMode::ScaleWithScreenSize);
		Canvas->SetReferenceResolution(FVector2D(1920.0, 1080.0));
		Canvas->SetScreenMatchMode(EDreamCanvasScreenMatchMode::MatchWidthOrHeight);
		Canvas->SetMatchFromWidthToHeight(0.5f);
#if PLATFORM_ANDROID
		// A phone is wider than 16:9: matching halfway cropped the top and bottom of the design.
		Canvas->SetScreenMatchMode(EDreamCanvasScreenMatchMode::Shrink);
#endif
	}
	for (const TCHAR* Path : { NieRUI::TitleClass, NieRUI::MenuClass, NieRUI::SettingsClass })
	{
		UClass* ScreenClass = LoadClass<UDreamUserWidget>(nullptr, Path);
		if (ScreenClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("NieR UI test: %s is missing; run DUI/UITests/NieR/_Tools/make_nier_assets.py"), Path);
			return;
		}
		ScreenClasses.Add(ScreenClass);
	}
	UClass* TitleClass = ScreenClasses[0];
	ScreenUI->PushWidgetOfClass(TEXT("NieR_Title"), TitleClass, EDreamUIScreenPageCachePolicy::KeepAlive, true, Player);
}

