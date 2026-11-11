// Copyright DreamMoon. All Rights Reserved.

#include "P3RDirector.h"

#include "P3RBackdrop.h"
#include "P3RFieldHud.h"
#include "P3RMainMenu.h"
#include "P3RProtoMenus.h"
#include "P3RSystemMenu.h"

#include "Core/DreamUserWidget.h"
#include "DreamUIBPLibrary.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogP3R, Log, All);

namespace P3RDirectorLocal
{
	const TCHAR* DesignLabels[] = {
		TEXT("最终版 · Tab 切换设计稿"),
		TEXT("早期设计稿 A · Tab 切换设计稿"),
		TEXT("早期设计稿 B · Tab 切换设计稿"),
	};
}

AP3RDirector::AP3RDirector()
{
	PrimaryActorTick.bCanEverTick = false;
}

UP3RScreen* AP3RDirector::Make(const TCHAR* Asset, int32 SortOrder)
{
	// Widget Blueprints are loaded here, never from a constructor (the editor module that owns their class may not be
	// loaded yet when a game module's CDO is built).
	const FString Path = FString::Printf(TEXT("/Game/P3R/UI/%s.%s_C"), Asset, Asset);
	UClass* Class = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(Path)).LoadSynchronous();
	if (Class == nullptr)
	{
		UE_LOG(LogP3R, Error, TEXT("Cannot load %s (run the setup and compile the widgets first)."), *Path);
		return nullptr;
	}
	UP3RScreen* Screen = Cast<UP3RScreen>(UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, Class, SortOrder));
	if (Screen != nullptr)
	{
		Screen->Hide();
	}
	return Screen;
}

void AP3RDirector::BeginPlay()
{
	Super::BeginPlay();

	if (APlayerController* Player = UGameplayStatics::GetPlayerController(this, 0))
	{
		FInputModeGameAndUI Mode;
		Mode.SetHideCursorDuringCapture(false);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		Player->SetInputMode(Mode);
		Player->SetShowMouseCursor(true);
	}

	Backdrop = Cast<UP3RBackdrop>(Make(TEXT("WBP_P3R_Backdrop"), 100));
	Field = Cast<UP3RFieldHud>(Make(TEXT("WBP_P3R_Field"), 150));
	MainMenu = Cast<UP3RMainMenu>(Make(TEXT("WBP_P3R_MainMenu"), 200));
	Glass = Make(TEXT("WBP_P3R_ProtoGlass"), 201);
	Mosaic = Make(TEXT("WBP_P3R_ProtoMosaic"), 202);
	System = Make(TEXT("WBP_P3R_System"), 300);
	Commu = Make(TEXT("WBP_P3R_Commu"), 301);
	Status = Make(TEXT("WBP_P3R_Status"), 302);

	if (Backdrop != nullptr)
	{
		Backdrop->Enter();
	}
	if (Field != nullptr)
	{
		Field->OnChosen.AddWeakLambda(this, [this](int32) { OpenMenu(); });
		Field->Enter();
	}
	for (UP3RScreen* Menu : { static_cast<UP3RScreen*>(MainMenu.Get()), Glass.Get(), Mosaic.Get() })
	{
		if (Menu != nullptr)
		{
			Menu->OnChosen.AddWeakLambda(this, [this](int32 Item) { ChooseItem(Item); });
			Menu->OnBack.AddWeakLambda(this, [this]() { CloseMenu(); });
			Menu->OnDesign.AddWeakLambda(this, [this]() { CycleDesign(); });
		}
	}
	for (UP3RScreen* Page : { System.Get(), Commu.Get(), Status.Get() })
	{
		if (Page != nullptr)
		{
			Page->OnBack.AddWeakLambda(this, [this]() { BackToMenu(); });
		}
	}
	if (System != nullptr)
	{
		System->OnChosen.AddWeakLambda(this, [this](int32 Command) { ChooseSystem(Command); });
	}
	if (MainMenu != nullptr)
	{
		MainMenu->SetDesignLabel(FText::FromString(P3RDirectorLocal::DesignLabels[DesignFinal]));
	}
}

bool AP3RDirector::IsBusy() const
{
	const UWorld* World = GetWorld();
	return World != nullptr && World->GetTimeSeconds() < BusyUntil;
}

void AP3RDirector::Busy(double Seconds)
{
	if (const UWorld* World = GetWorld())
	{
		BusyUntil = World->GetTimeSeconds() + Seconds;
	}
}

void AP3RDirector::After(float Seconds, TFunction<void()> Then)
{
	if (Seconds <= 0.0f)
	{
		Then();
		return;
	}
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Then]() { Then(); }), Seconds, false);
}

UP3RScreen* AP3RDirector::CurrentMenu() const
{
	switch (Design)
	{
	case DesignGlass: return Glass;
	case DesignMosaic: return Mosaic;
	default: return MainMenu;
	}
}

void AP3RDirector::OpenMenu()
{
	if (State != EState::Field || IsBusy())
	{
		return;
	}
	State = EState::Menu;
	Busy(0.45);
	if (Field != nullptr)
	{
		Field->Leave();
	}
	if (Backdrop != nullptr)
	{
		Backdrop->SetSea(true);
	}
	if (UP3RScreen* Menu = CurrentMenu())
	{
		Menu->Enter();
	}
}

void AP3RDirector::CloseMenu()
{
	if (State != EState::Menu || IsBusy())
	{
		return;
	}
	State = EState::Field;
	Busy(0.5);
	if (MainMenu != nullptr)
	{
		MainMenu->SetLeavingToSub(false);
	}
	if (UP3RScreen* Menu = CurrentMenu())
	{
		Menu->Leave();
	}
	if (Backdrop != nullptr)
	{
		Backdrop->SetSea(false);
	}
	After(0.4f, [this]()
	{
		if (Field != nullptr && State == EState::Field)
		{
			Field->Enter();
		}
	});
}

void AP3RDirector::ChooseItem(int32 Item)
{
	if (State != EState::Menu || IsBusy())
	{
		return;
	}
	switch (Item)
	{
	case UP3RMainMenu::Status: OpenPage(Status); break;
	case UP3RMainMenu::Commu: OpenPage(Commu); break;
	case UP3RMainMenu::System: OpenPage(System); break;
	default:
		if (Design == DesignFinal && MainMenu != nullptr)
		{
			MainMenu->Deny();
		}
		break;
	}
}

void AP3RDirector::OpenPage(UP3RScreen* Page)
{
	if (Page == nullptr)
	{
		return;
	}
	State = EState::Page;
	OpenPageScreen = Page;
	Busy(0.5);
	const bool bFinal = Design == DesignFinal && MainMenu != nullptr;
	if (bFinal)
	{
		// The page spreads out from the word that was chosen, over the menu and the sweep of the protagonist.
		Page->SetEntryPoint(MainMenu->GetChosenPoint());
		MainMenu->SetLeavingToPage();
	}
	else if (MainMenu != nullptr)
	{
		MainMenu->SetLeavingToSub(true);
	}
	if (UP3RScreen* Menu = CurrentMenu())
	{
		Menu->Leave();
	}
	if (Backdrop != nullptr)
	{
		Backdrop->SetDim(Page == System ? 0.0f : 0.18f);
	}
	After(bFinal ? 0.12f : 0.08f, [this, Page]()
	{
		if (State == EState::Page && OpenPageScreen == Page)
		{
			Page->Enter();
		}
	});
}

void AP3RDirector::BackToMenu()
{
	if (State != EState::Page || IsBusy())
	{
		return;
	}
	State = EState::Menu;
	Busy(0.45);
	// The final design comes back as the game's does: the menu is already under the page, uncovered through a hole that
	// opens from the top right.
	const bool bFinal = Design == DesignFinal && MainMenu != nullptr;
	if (OpenPageScreen != nullptr)
	{
		OpenPageScreen->SetExitToMenu(bFinal);
		OpenPageScreen->Leave();
	}
	OpenPageScreen = nullptr;
	if (Backdrop != nullptr)
	{
		Backdrop->SetDim(0.0f);
	}
	if (bFinal)
	{
		MainMenu->SetReturning();
	}
	After(bFinal ? 0.0f : 0.18f, [this]()
	{
		if (State == EState::Menu)
		{
			if (UP3RScreen* Menu = CurrentMenu())
			{
				Menu->Enter();
			}
		}
	});
}

void AP3RDirector::ChooseSystem(int32 Command)
{
	if (State != EState::Page || IsBusy())
	{
		return;
	}
	if (Command != UP3RSystemMenu::ReturnToTitle)
	{
		if (UP3RSystemMenu* SystemMenu = Cast<UP3RSystemMenu>(System))
		{
			SystemMenu->Deny();
		}
		return;
	}
	// Back to the field: the page goes, the sea drains away.
	State = EState::Field;
	Busy(0.6);
	if (OpenPageScreen != nullptr)
	{
		OpenPageScreen->Leave();
	}
	OpenPageScreen = nullptr;
	if (Backdrop != nullptr)
	{
		Backdrop->SetSea(false);
	}
	After(0.45f, [this]()
	{
		if (Field != nullptr && State == EState::Field)
		{
			Field->Enter();
		}
	});
}

void AP3RDirector::CycleDesign()
{
	if (State != EState::Menu || IsBusy())
	{
		return;
	}
	Busy(0.45);
	if (MainMenu != nullptr)
	{
		MainMenu->SetLeavingToSub(true);
	}
	if (UP3RScreen* Old = CurrentMenu())
	{
		Old->Leave();
	}
	Design = (Design + 1) % DesignCount;
	if (MainMenu != nullptr)
	{
		MainMenu->SetDesignLabel(FText::FromString(P3RDirectorLocal::DesignLabels[Design]));
	}
	After(0.2f, [this]()
	{
		if (State == EState::Menu)
		{
			if (UP3RScreen* Menu = CurrentMenu())
			{
				Menu->Enter();
			}
		}
	});
}

AP3RGameMode::AP3RGameMode()
{
	DefaultPawnClass = nullptr;
}

void AP3RGameMode::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->SpawnActor<AP3RDirector>();
}
