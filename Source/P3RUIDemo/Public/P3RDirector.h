// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/GameModeBase.h"
#include "P3RDirector.generated.h"

class UP3RBackdrop;
class UP3RFieldHud;
class UP3RMainMenu;
class UP3RScreen;
class UDreamUserWidget;

/**
 * Runs the study's flow. The field; Tab / Esc / Start opens the pause menu, which sinks the field into the inner sea;
 * STATUS, COMMU and SYSTEM open their pages, Esc comes back; Tab in the menu switches between the final design and the
 * two early ones; RETURN to TITLE (or Esc from the menu) surfaces back to the field. Every screen is made once at the
 * start and shown or hidden from then on.
 */
UCLASS()
class P3RUIDEMO_API AP3RDirector : public AActor
{
	GENERATED_BODY()

public:
	AP3RDirector();

protected:
	virtual void BeginPlay() override;

private:
	enum class EState : uint8 { Field, Menu, Page };
	enum EDesign : int32 { DesignFinal, DesignGlass, DesignMosaic, DesignCount };

	UP3RScreen* Make(const TCHAR* Asset, int32 SortOrder);
	UP3RScreen* CurrentMenu() const;
	void After(float Seconds, TFunction<void()> Then);

	void OpenMenu();
	void CloseMenu();
	void ChooseItem(int32 Item);
	void OpenPage(UP3RScreen* Page);
	void BackToMenu();
	void CycleDesign();
	void ChooseSystem(int32 Command);

	UPROPERTY(Transient)
	TObjectPtr<UP3RBackdrop> Backdrop;
	UPROPERTY(Transient)
	TObjectPtr<UP3RFieldHud> Field;
	UPROPERTY(Transient)
	TObjectPtr<UP3RMainMenu> MainMenu;
	UPROPERTY(Transient)
	TObjectPtr<UP3RScreen> Glass;
	UPROPERTY(Transient)
	TObjectPtr<UP3RScreen> Mosaic;
	UPROPERTY(Transient)
	TObjectPtr<UP3RScreen> System;
	UPROPERTY(Transient)
	TObjectPtr<UP3RScreen> Commu;
	UPROPERTY(Transient)
	TObjectPtr<UP3RScreen> Status;
	UPROPERTY(Transient)
	TObjectPtr<UP3RScreen> OpenPageScreen;

	EState State = EState::Field;
	int32 Design = DesignFinal;
	/** One transition at a time: keys during one are dropped. */
	double BusyUntil = 0.0;
	bool IsBusy() const;
	void Busy(double Seconds);
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	TSharedPtr<class SWidget> TouchBar;
	TWeakObjectPtr<class UGameViewportClient> TouchViewport;
	void MountTouchBar();
};

/** Spawns the director into whatever level it runs in; the empty Entry map is enough. No pawn. */
UCLASS()
class P3RUIDEMO_API AP3RGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AP3RGameMode();
	virtual void BeginPlay() override;
};
