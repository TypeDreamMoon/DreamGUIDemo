// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RWordMenu.h"
#include "P3RSystemMenu.generated.h"

class UDreamMaskTransition;

/**
 * SYSTEM (WBP_P3R_System): the sea shrunk into a disc, SYSTEM falling across white, the 9, the protagonist reaching for
 * it, and the seven commands as thrown words. It opens with an ink blot spreading from the protagonist's side of the
 * screen (the way the game cuts from the pause menu into any of its pages), the disc turning in as it grows.
 */
UCLASS()
class P3RUIDEMO_API UP3RSystemMenu : public UP3RScreen
{
	GENERATED_BODY()

public:
	enum ECommand : int32 { Tutorial, Config, Dictionary, DataLoad, DataSave, Rollback, ReturnToTitle, Count };

	void Deny();

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
	TWeakObjectPtr<UDreamMaskTransition> Blot;
	TWeakObjectPtr<UDreamWidget> Disc;
	TWeakObjectPtr<UDreamWidget> Giant;
	TWeakObjectPtr<UDreamWidget> Nine;
	TWeakObjectPtr<UDreamWidget> Hero;
	TWeakObjectPtr<UDreamWidget> Guide;
	float EnterAt = -100.0f;
};
