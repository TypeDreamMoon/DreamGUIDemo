// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/DreamTextUserWidget.h"
#include "NocturneLens.generated.h"

class UDreamStylizeEffect;
class UDreamBackgroundPixelate;

/** The moments the lens answers to. */
enum class ENocturneLensPulse : uint8
{
	/** A screen arriving or leaving: the picture breaks into blocks and resolves, a glitch, scanlines rolling over. */
	Transition,
	/** The hull hit: a hard glitch and the colours split sideways. */
	Hit,
	/** Out of the title: the tube switches off into the system, blocks resolving. */
	Wake,
	/** The dive into a voyage: the colours part outward faster and faster, the edges close in. */
	Dive,
	Count,
};

/**
 * NOCTURNE//OS's lens (DUI/Nocturne/Nocturne_Lens.dui): one layer over everything on the screen, under the cursor, that
 * mixes DreamGUI's post effects -- a background pixelate under a stylize stack of glitch, CRT, chromatic aberration, film
 * grain and vignette -- and drives them all from a few numbers: how fast the camera goes, how damaged the hull is,
 * whether the title's old tube is on, and pulses for the moments that break the picture up. Nothing is drawn while
 * nothing is asked for.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneLens : public UDreamTextUserWidget
{
	GENERATED_BODY()

public:
	/** The lens on the screen now, if there is one. */
	static UNocturneLens* Get();
	/** Make InLens the one on the screen, as soon as it is added: a screen added in the same frame (the title) asks for
	 *  it before the lens has been initialised. */
	static void SetCurrent(UNocturneLens* InLens) { Current = InLens; }
	/** A pulse on the lens on the screen, if there is one; Strength 1 is the full look. */
	static void Pulse(ENocturneLensPulse InKind, float InStrength = 1.0f);

	void Fire(ENocturneLensPulse InKind, float InStrength);
	/** 0 to 1: how fast the camera goes. Red and blue part outward at the edges. */
	void SetSpeed(float In01) { Speed = FMath::Clamp(In01, 0.0f, 1.0f); }
	/** 0 to 1: how damaged the hull is. A little glitch that stays. */
	void SetDamage(float In01) { Damage = FMath::Clamp(In01, 0.0f, 1.0f); }
	/** The title's old tube -- curve, scanlines, phosphor, grain -- going to In01 over InSeconds. */
	void SetTube(float In01, float InSeconds);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnDestruct() override;
	virtual void NativeOnTick(float DeltaTime) override;

private:
	/** 1 at a pulse's start falling to 0 at its end; negative before it starts or after it ends. */
	float Since(ENocturneLensPulse InKind) const;

	TWeakObjectPtr<UDreamStylizeEffect> Stylize;
	TWeakObjectPtr<UDreamBackgroundPixelate> Pixelate;
	float LensTime = 0.0f;
	float Speed = 0.0f;
	float Damage = 0.0f;
	float Tube = 0.0f;
	float TubeFrom = 0.0f;
	float TubeTo = 0.0f;
	float TubeAt = 0.0f;
	float TubeSeconds = 0.0f;
	float PulseAt[static_cast<int32>(ENocturneLensPulse::Count)] = { -100.0f, -100.0f, -100.0f, -100.0f };
	float PulseStrength[static_cast<int32>(ENocturneLensPulse::Count)] = { 0.0f, 0.0f, 0.0f, 0.0f };
	bool bStylizeShown = true;
	bool bPixelateShown = true;

	static TWeakObjectPtr<UNocturneLens> Current;
};
