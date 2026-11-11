#include "NieRUITestGameMode.h"

#include "Core/Components/DreamCanvas.h"
#include "Core/DreamScreenUISubsystem.h"
#include "Core/DreamUserWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "NieRScreen.h"
#include "TimerManager.h"

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

