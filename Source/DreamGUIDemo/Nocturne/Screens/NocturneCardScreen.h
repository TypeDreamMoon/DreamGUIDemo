// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneCardScreen.generated.h"

/**
 * A title card over the star map: the video's opening card (WBP_Nocturne_CardOpen) and its end card
 * (WBP_Nocturne_CardEnd). Parts by id, any of them optional:
 *
 *   Word   a row of letters that drop in one after another
 *   Lines  lines under the word that run in one after another
 *   Art    a picture that slides in from the right
 *   Rule   a hairline that draws out from its left end
 */
UCLASS(Abstract)
class DREAMGUIDEMO_API UNocturneCardScreen : public UNocturneStarScreen
{
	GENERATED_BODY()

protected:
	virtual void PlayContentEntrance(float Delay) override;
	virtual float GetEntrancePulse() const override { return 0.5f; }
};
