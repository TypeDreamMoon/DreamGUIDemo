// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RStatusScreen.generated.h"

class UDreamMaskTransition;
class UP3RPartyRow;
class UTexture2D;

/**
 * STATUS (WBP_P3R_Status): the party as black bands; on the right the protagonist with a hand over his face, and in the
 * gap between two fingers a shard of glass with the chosen member's eyes. Choosing another member flashes the shard
 * white and slides the new eyes in under the glass; L1 / R1 (Q / E) flip the header between the screen's modes.
 */
UCLASS()
class P3RUIDEMO_API UP3RStatusScreen : public UP3RScreen
{
	GENERATED_BODY()

public:
	static constexpr int32 Count = 7;

protected:
	virtual void NativeOnInitialized() override;
	virtual bool HandleKey(EP3RKey Key) override;
	virtual void PlayEntrance() override;
	virtual float PlayExit() override;
	virtual void TickScreen(float DeltaTime) override;

private:
	void Select(int32 Index, bool bInstant = false);
	void FlipMode(int32 Delta);
	/** The shard shows member Index's eyes: at once, or through a white flash as the new eyes slide in. */
	void ShowEyes(int32 Index, bool bInstant);

	TWeakObjectPtr<UP3RPartyRow> Rows[Count];
	TWeakObjectPtr<UDreamWidget> HeroGroup;
	TWeakObjectPtr<UDreamWidget> ShardEyes;
	TWeakObjectPtr<UDreamWidget> ShardFlash;
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> Eyes[Count];
	TWeakObjectPtr<UDreamMaskTransition> Blot;
	TWeakObjectPtr<UDreamWidget> Disc;
	TWeakObjectPtr<UDreamWidget> Header;
	TWeakObjectPtr<UDreamWidget> Mode;
	TWeakObjectPtr<UDreamWidget> TopBand;
	int32 Selected = 0;
	int32 ModeIndex = 0;
	float EnterAt = -100.0f;
	float FlipAt = -100.0f;
	int32 PendingMode = 0;
};
