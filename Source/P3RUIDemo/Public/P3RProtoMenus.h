// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RProtoMenus.generated.h"

class UDreamMaskTransition;

/**
 * The two early designs of the pause menu, kept as menus that work: Tab cycles the final design and these two, so the
 * same nine choices can be walked through each. Both report the same item indices as UP3RMainMenu.
 */
UCLASS(Abstract)
class P3RUIDEMO_API UP3RProtoMenu : public UP3RScreen
{
	GENERATED_BODY()

public:
	/** Which pause-menu item (UP3RMainMenu::EItem) each of this design's entries is. */
	virtual int32 GetItemOf(int32 Entry) const { return Entry; }
	int32 GetSelectedItem() const { return GetItemOf(Selected); }

protected:
	virtual void NativeOnInitialized() override;
	virtual bool HandleKey(EP3RKey Key) override;
	virtual float PlayExit() override;

	virtual int32 GetEntryCount() const PURE_VIRTUAL(UP3RProtoMenu::GetEntryCount, return 0;);
	virtual const TCHAR* GetEntryPrefix() const PURE_VIRTUAL(UP3RProtoMenu::GetEntryPrefix, return TEXT(""););
	virtual void ShowSelection(int32 Previous, bool bInstant) {}

	void Select(int32 Index, bool bInstant = false);
	void PlayBlotIn();

	int32 Selected = 0;
	TWeakObjectPtr<UDreamMaskTransition> Blot;
};

/** The first design (WBP_P3R_ProtoGlass): white capitals down a slope over a tilted sheet of ink-drawn glass. */
UCLASS()
class P3RUIDEMO_API UP3RProtoGlass : public UP3RProtoMenu
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void PlayEntrance() override;
	virtual void TickScreen(float DeltaTime) override;
	virtual int32 GetEntryCount() const override { return 9; }
	virtual const TCHAR* GetEntryPrefix() const override { return TEXT("Item"); }
	/** This design lists COMMU before CALENDAR and QUEST last but one. */
	virtual int32 GetItemOf(int32 Entry) const override;
	virtual void ShowSelection(int32 Previous, bool bInstant) override;

private:
	/** The two blurred polygons. */
	TWeakObjectPtr<UDreamWidget> Board;
	TWeakObjectPtr<UDreamWidget> Side;
	TWeakObjectPtr<UDreamWidget> Bar;
	TWeakObjectPtr<UDreamWidget> Note;
	float EnterAt = -100.0f;
};

/** The second design (WBP_P3R_ProtoMosaic): thin words among scattered stills; the chosen item's still lights up. */
UCLASS()
class P3RUIDEMO_API UP3RProtoMosaic : public UP3RProtoMenu
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void PlayEntrance() override;
	virtual void TickScreen(float DeltaTime) override;
	virtual int32 GetEntryCount() const override { return 5; }
	virtual const TCHAR* GetEntryPrefix() const override { return TEXT("Label"); }
	virtual void ShowSelection(int32 Previous, bool bInstant) override;

private:
	static constexpr int32 TileCount = 8;
	TWeakObjectPtr<UDreamWidget> Tiles[TileCount];
	TWeakObjectPtr<UDreamWidget> Underline;
	TWeakObjectPtr<UDreamWidget> Note;
	TWeakObjectPtr<UDreamWidget> Triangle;
	float EnterAt = -100.0f;
};
