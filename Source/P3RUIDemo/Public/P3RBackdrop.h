// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "P3RScreen.h"
#include "P3RBackdrop.generated.h"

class UMaterialInstanceDynamic;
class UDreamMaskTransition;

/**
 * The ground under every screen: the paused field, and the inner sea over it. Sinking into the sea is an ink mask that
 * spreads from the protagonist's side of the screen in ten held frames of 50 ms -- a hand-drawn reveal rather than a
 * smooth wipe -- while the sea material's own state eases from the field's colours to the blue ramp. Sub-screens dim it.
 */
UCLASS()
class P3RUIDEMO_API UP3RBackdrop : public UP3RScreen
{
	GENERATED_BODY()

public:
	/** Sink into the sea (true) or surface back to the field (false). */
	void SetSea(bool bInSea);
	bool IsInSea() const { return bInSea; }
	/** 0 = the full sea, 1 = darkened under a sub-screen. */
	void SetDim(float InDim, float Seconds = 0.25f);

	/** How long the ink reveal takes, so a screen can time its own entrance against it. */
	static constexpr float InkSeconds = 0.5f;

protected:
	virtual void NativeOnInitialized() override;
	virtual void TickScreen(float DeltaTime) override;

private:
	void SetSeaParam(FName Name, float Value);

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> SeaMaterial;
	TWeakObjectPtr<UDreamMaskTransition> Ink;
	TWeakObjectPtr<UDreamWidget> Bubbles;

	bool bInSea = false;
	float InkStart = -100.0f;
	float InkFrom = 0.0f;
	float InkTo = 0.0f;
	float Dim = 0.0f;
	float DimFrom = 0.0f;
	float DimTo = 0.0f;
	float DimStart = -100.0f;
	float DimSeconds = 0.25f;
};
