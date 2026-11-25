// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "InputCoreTypes.h"
#include "P3RScreen.generated.h"

class UDreamMaskTransition;
class UDreamWidget;
class UUIButton;

/** What a key means to the menus, whatever device it came from. */
enum class EP3RKey : uint8
{
	None,
	Up,
	Down,
	Left,
	Right,
	Confirm,
	Back,
	PrevTab,   // L1: Q / left shoulder
	NextTab,   // R1: E / right shoulder
	Design,    // switch the menu's design: Tab / F1 / the gamepad's view button
	Open,      // open the menu from the field: Tab / Esc / Start / Y
};

/**
 * One screen of the study: a text-authored DreamGUI widget (.dui) with a C++ class behind it. The screen owns its
 * entrance and exit animation and reads its own keys; what a choice leads to is the director's business, told through
 * OnChosen / OnBack / OnDesign.
 *
 * Keys reach a DreamGUI widget through the focused widget and bubble up, so every screen's .dui has a full-screen
 * `Catch` rect that is focusable and is kept focused while the screen is active (the menu items themselves are not
 * focusable: they take the pointer, the screen takes the keys).
 */
UCLASS(Abstract)
class P3RUIDEMO_API UP3RScreen : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	UDreamWidget* Node(const TCHAR* Name) const;
	UDreamWidget* Node(const FString& Name) const { return Node(*Name); }

	/** Show and play the way in. */
	virtual void Enter();
	/** Play the way out, then collapse. */
	virtual void Leave();
	/** Collapse at once, without an exit (a screen just made, waiting for its turn). */
	void Hide();
	bool IsActive() const { return bActive; }
	/** A touch button or a tap, taken as Key would be. Logs what it found, so a press that does nothing says why. */
	bool TouchAction(EP3RKey Key);

	/** Ignore keys and the pointer for a moment (while a transition plays). */
	void LockInput(float Seconds) { InputLockedUntil = FMath::Max(InputLockedUntil, ScreenTime + Seconds); }

	static EP3RKey Translate(const FKey& Key, bool bFieldKeys = false);

	/**
	 * Where the next way in opens from, as a share of the screen (0,0 its top left): a page chosen from a menu word opens
	 * from that word. Used once; unset, the page's blot opens from its own centre.
	 */
	void SetEntryPoint(FVector2D InPoint) { EntryPoint = InPoint; bHasEntryPoint = true; }
	/** The next way out uncovers the menu underneath through a hole that opens from the top right, as the game's does. */
	void SetExitToMenu(bool bInExitToMenu) { bExitToMenu = bInExitToMenu; }

	TMulticastDelegate<void(int32)> OnChosen;
	TMulticastDelegate<void()> OnBack;
	TMulticastDelegate<void()> OnDesign;

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual bool NativeOnKeyDown(UDreamKeyEventData* EventData) override;

	/** A key meant for this screen; return true when it was used. */
	virtual bool HandleKey(EP3RKey Key) { return false; }
	virtual void PlayEntrance() {}
	/** The exit animation; return how long it takes, after which the screen collapses. */
	virtual float PlayExit() { return 0.0f; }
	virtual void TickScreen(float DeltaTime) {}
	/** Whether the field's keys (Open) apply instead of the menu's (Design). */
	virtual bool UsesFieldKeys() const { return false; }

	/** Whether the pointer has really moved lately: hover only takes the selection then, not when an item slides under a still pointer. */
	bool PointerMovedRecently() const;

	/** Wire a node's UIButton: hovering it calls OnHover (when the pointer is live), clicking it OnClick. */
	void WireButton(UDreamWidget* Widget, TFunction<void()> OnHover, TFunction<void()> OnClick);

	bool IsInputLocked() const { return !bActive || ScreenTime < InputLockedUntil; }
	float GetScreenTime() const { return ScreenTime; }
	/** Seconds since the last Enter. */
	float GetEnterTime() const { return ScreenTime - EnteredAt; }

	/**
	 * A page's way in: the page spreads out through an ink blot from the entry point, and starts as a close-up -- a
	 * little larger, under a blue veil (a `Closeup` node, when the .dui has one) -- that settles as the blot spreads.
	 */
	void PlayBlotIn(UDreamMaskTransition* Mask, float Seconds = 0.34f);
	/** A page's way out: back through the blot, or (SetExitToMenu) away through a hole opening from the top right. Returns its length. */
	float PlayBlotOut(UDreamMaskTransition* Mask);
	/** Whether the next way out uncovers the menu underneath (so the page should stay whole until it is gone). */
	bool IsExitingToMenu() const { return bExitToMenu; }

private:
	void KeepFocus();

	TWeakObjectPtr<UDreamWidget> Catch;
	float ScreenTime = 0.0f;
	float EnteredAt = 0.0f;
	float InputLockedUntil = 0.0f;
	float LeaveAt = -1.0f;
	bool bActive = false;
	FVector2D LastPointer = FVector2D::ZeroVector;
	float PointerMovedAt = -100.0f;
	FVector2D EntryPoint = FVector2D(0.5, 0.5);
	bool bHasEntryPoint = false;
	bool bExitToMenu = false;
};
