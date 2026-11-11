// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneTitleScreen.generated.h"

/**
 * NOCTURNE//OS's title: the system waking at night. The wordmark decodes from scrambled glyphs, the boot log ticks its
 * lines over to OK one by one, the administrator's card draws in with her picture, and "press any key" breathes at the
 * bottom. Any key, any pad button or a click anywhere wakes the system (OnWake); the screen then folds away under a
 * pixel sort while the menu flies the camera into the star map. Its .dui is DUI/Nocturne/Nocturne_Title.dui.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneTitleScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

public:
	/** The system wakes: any key or click, once. */
	void Wake();
	bool IsAwake() const { return bAwake; }

	TMulticastDelegate<void()> OnWake;

	virtual void PlayExit() override;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;
	virtual void PlayContentEntrance(float Delay) override;
	/** The full-screen catch, so "any key" reaches the title as soon as it is up. */
	virtual UDreamWidget* GetInitialFocus() const override;

private:
	void SetNodeText(const TCHAR* Name, const FString& Text) const;

	/** When the entrance began, on the screen's clock; negative until it has. */
	float EntranceAt = -1.0f;
	bool bAwake = false;
	TArray<bool> BootDone;
	bool bLogoDone = false;
	int32 ShownUptime = -1;
};
