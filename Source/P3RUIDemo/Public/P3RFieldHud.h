// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RFieldHud.generated.h"

/** The field's light HUD (WBP_P3R_Field): date and moon, the place, and the prompt; Tab / Esc / Start / Y open the menu. */
UCLASS()
class P3RUIDEMO_API UP3RFieldHud : public UP3RScreen
{
	GENERATED_BODY()

protected:
	virtual bool HandleKey(EP3RKey Key) override;
	virtual bool UsesFieldKeys() const override { return true; }
	virtual void PlayEntrance() override;
	virtual float PlayExit() override;
	virtual void TickScreen(float DeltaTime) override;
};
