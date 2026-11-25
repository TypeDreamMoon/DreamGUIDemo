#pragma once

#include "CoreMinimal.h"
#include "NieRScreen.h"
#include "NieRTitleScreen.generated.h"

/**
 * The start screen: the title over a field of stars, "Press Any Key" breathing under it, and once a key is pressed the
 * start menu (Continue, New Game, Load Game, Settings, Exit Game) rising in. Back from the menu returns to "Press Any
 * Key". The title glitches now and then -- blocks beside it blink and shift, and the whole title jumps sideways for a
 * frame or two. A row's Action says what it does: menu (the system menu), settings, exit.
 * Nodes: Logo, LogoRule, Glitch (its children are the blocks), PressKey (PressText), PressHit (the whole screen, a
 * button), Menu (the rows), MenuPrompts.
 */
UCLASS()
class DEVUITESTS_API UNieRTitleScreen : public UNieRScreen
{
	GENERATED_BODY()

public:
	virtual bool HandleBack() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnEnable() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;
	virtual bool NativeOnPointerClick(UDreamPointerEventData* EventData) override;
	virtual void OnRowClicked(UNieRRow* Row) override;
	virtual UNieRRow* GetEntryRow() const override;
	virtual void PlayEntrance() override;

private:
	UNieRRow* GetFirstMenuRow() const;
	void ShowPressKey();
	void ShowMenu();

	bool bInMenu = false;
	bool bEntrancePlayed = false;
	TArray<TWeakObjectPtr<UDreamWidget>> Glitches;
	float NextGlitch = 0.0f;
	float NextJump = 2.5f;
	float JumpEnds = -1.0f;
	FRandomStream Random{ 7 };
};
