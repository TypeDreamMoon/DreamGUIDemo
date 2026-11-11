// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "P3RCommuRow.generated.h"

class UDreamWidget;

/** One bond in the COMMU list (WBP_P3R_CommuRow). */
UCLASS()
class P3RUIDEMO_API UP3RCommuRow : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	void SetData(const FString& InNumeral, const FString& InArcana, int32 InRank, const FString& InPartner);
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
