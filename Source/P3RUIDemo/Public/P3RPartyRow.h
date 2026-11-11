// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "P3RPartyRow.generated.h"

class UDreamWidget;
class UTexture2D;

/** What a party member shows in the STATUS list. */
struct FP3RMember
{
	const TCHAR* Role = TEXT("");      // LEADER / PARTY / NAVI / empty
	const TCHAR* Name = TEXT("");
	FColor Colour = FColor::White;
	int32 Level = 1;
	int32 Hp = 1, HpMax = 1;
	int32 Sp = 1, SpMax = 1;
};

/** One member in the STATUS list (WBP_P3R_PartyRow). */
UCLASS()
class P3RUIDEMO_API UP3RPartyRow : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	void SetMember(const FP3RMember& InMember, UTexture2D* InFace);
	/** The chosen look wipes on from the left (or off), unless bInstant. */
	void SetChosen(bool bInChosen, bool bInstant = false);

protected:
	virtual void NativeOnInitialized() override;

private:
	UDreamWidget* Find(const TCHAR* Name) const;

	/** The colours of the chosen (or unchosen) look, set when the wipe has crossed most of the row. */
	void ApplyColours();

	bool bChosen = false;
	FTimerHandle ColourTimer;
};
