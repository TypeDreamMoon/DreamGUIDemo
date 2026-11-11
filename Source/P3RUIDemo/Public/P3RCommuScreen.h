// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RCommuScreen.generated.h"

class UDreamMaskTransition;
class UP3RCommuRow;
class UTexture2D;

/** COMMU (WBP_P3R_Commu): the list of bonds as slanted cards; the chosen card turns white and pushes out to the left. */
UCLASS()
class P3RUIDEMO_API UP3RCommuScreen : public UP3RScreen
{
	GENERATED_BODY()

public:
	static constexpr int32 Count = 6;

protected:
	virtual void NativeOnInitialized() override;
	virtual bool HandleKey(EP3RKey Key) override;
	virtual void PlayEntrance() override;
	virtual float PlayExit() override;
	virtual void TickScreen(float DeltaTime) override;

private:
	void Select(int32 Index, bool bInstant = false);
	void Glint();
	/** The held card turns over to show bond Index (at once, or in depth through a quick flip). */
	void TurnCard(int32 Index, bool bInstant);
	/** Puts bond Index's face on the card (INDEX_NONE: its back). */
	void ShowFace(int32 Index);
	/** A sheen across the card's face. */
	void Sheen();

	TWeakObjectPtr<UP3RCommuRow> Rows[Count];
	TWeakObjectPtr<UDreamMaskTransition> Blot;
	TWeakObjectPtr<UDreamWidget> Giant;
	TWeakObjectPtr<UDreamWidget> Hero;
	/** His grey shadow on the white wall. */
	TWeakObjectPtr<UDreamWidget> HeroShadow;
	TWeakObjectPtr<UDreamWidget> Glinter;
	TWeakObjectPtr<UDreamWidget> Card;
	TWeakObjectPtr<UDreamWidget> CardFace;
	TWeakObjectPtr<UDreamWidget> CardSheen;
	/** His fingers, in front of the card. */
	TWeakObjectPtr<UDreamWidget> HeroFront;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Faces[Count];
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Back;
	int32 CardShows = -1;
	int32 CardPending = 0;
	float CardFlipAt = -100.0f;
	bool bCardLanded = true;
	/** The card's tilt in the .dui; the flip and the breathing turn it about its upright. */
	float CardRoll = 0.0f;
	TWeakObjectPtr<UDreamWidget> Flag;
	TWeakObjectPtr<UDreamWidget> List;
	int32 Selected = 0;
	float EnterAt = -100.0f;
};
