// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RWordMenu.h"
#include "P3RMainMenu.generated.h"

class UDreamMaskTransition;

/**
 * The pause menu, final design (WBP_P3R_MainMenu): the protagonist hanging upside down in the inner sea over a white
 * slab with MAIN and the item's number down the edge, and the nine words (FP3RWordMenu has their motion).
 */
UCLASS()
class P3RUIDEMO_API UP3RMainMenu : public UP3RScreen
{
	GENERATED_BODY()

public:
	enum EItem : int32 { Skill, Item, Equip, Persona, Status, Quest, Commu, Calendar, System, Count };

	int32 GetSelected() const { return Menu.GetSelected(); }
	/** The chosen item is not in the study: shake it and say so. */
	void Deny();
	/** The next Leave goes to another design of the menu (the words fly off) rather than back to the field. */
	void SetLeavingToSub(bool bInToSub) { bLeavingToSub = bInToSub; }
	/**
	 * The next Leave opens a page: the protagonist turns to the camera as a dark-blue shape sweeping across the menu,
	 * and the page spreads out over it from the chosen word.
	 */
	void SetLeavingToPage() { bLeavingToPage = true; }
	/** The next Enter comes back from a page: everything is already in place, lit for a moment as the page's hole opens. */
	void SetReturning() { bReturning = true; }
	/** The chosen word's centre, as a share of the screen from the top left (where a page opens from). */
	FVector2D GetChosenPoint() const { return Menu.GetCentre(Menu.GetSelected()) / FVector2D(1920.0, 1080.0); }
	void SetDesignLabel(const FText& InLabel);
	/** What an item does, as the guide says it. */
	static FText GetGuide(int32 Item);

protected:
	virtual void NativeOnInitialized() override;
	virtual bool HandleKey(EP3RKey Key) override;
	virtual void PlayEntrance() override;
	virtual float PlayExit() override;
	virtual void TickScreen(float DeltaTime) override;

private:
	void Select(int32 Index);
	void Choose(int32 Index);

	FP3RWordMenu Menu;
	TWeakObjectPtr<UDreamWidget> Hero;
	TWeakObjectPtr<UDreamWidget> Slab;
	TWeakObjectPtr<UDreamWidget> Number;
	TWeakObjectPtr<UDreamWidget> MainWord;
	TWeakObjectPtr<UDreamWidget> Guide;
	TWeakObjectPtr<UDreamWidget> GuideRule;
	TArray<TWeakObjectPtr<UDreamWidget>> Chrome;
	TWeakObjectPtr<UDreamWidget> Design;
	TWeakObjectPtr<UDreamWidget> Sweep;
	TWeakObjectPtr<UDreamWidget> Flash;
	/** The menu's own mask: open between visits to a page (hidden), so it can come back through the page's hole. */
	TWeakObjectPtr<UDreamMaskTransition> Reveal;

	bool bLeavingToSub = false;
	bool bLeavingToPage = false;
	bool bReturning = false;
	float SweepAt = -100.0f;
	float HeroInAt = -100.0f;
	/** The tilt the .dui gives him; the sway turns him about it. */
	float HeroRoll = 0.0f;
};
