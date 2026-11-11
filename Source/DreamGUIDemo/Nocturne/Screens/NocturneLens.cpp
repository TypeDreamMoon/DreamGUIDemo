// Copyright DreamMoon. All Rights Reserved.

#include "NocturneLens.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamUIStylize.h"
#include "Core/DreamWidgetTree.h"
#include "Extensions/Effects/DreamBackgroundPixelate.h"
#include "Extensions/Effects/DreamStylizeEffect.h"

TWeakObjectPtr<UNocturneLens> UNocturneLens::Current;

namespace NocturneLensLocal
{
	/** How long each pulse lasts, seconds: Transition, Hit, Wake, Dive. */
	const float PulseSeconds[] = { 0.42f, 0.5f, 0.9f, 1.55f };

	float Smooth(float X)
	{
		X = FMath::Clamp(X, 0.0f, 1.0f);
		return X * X * (3.0f - 2.0f * X);
	}
}

UNocturneLens* UNocturneLens::Get()
{
	return Current.Get();
}

void UNocturneLens::Pulse(ENocturneLensPulse InKind, float InStrength)
{
	if (UNocturneLens* Lens = Current.Get(); Lens != nullptr && InStrength > 0.0f)
	{
		Lens->Fire(InKind, InStrength);
	}
}

void UNocturneLens::Fire(ENocturneLensPulse InKind, float InStrength)
{
	const int32 Index = static_cast<int32>(InKind);
	PulseAt[Index] = LensTime;
	PulseStrength[Index] = InStrength;
}

void UNocturneLens::SetTube(float In01, float InSeconds)
{
	TubeFrom = Tube;
	TubeTo = FMath::Clamp(In01, 0.0f, 1.0f);
	TubeAt = LensTime;
	TubeSeconds = FMath::Max(InSeconds, 0.0f);
	if (TubeSeconds <= 0.0f)
	{
		Tube = TubeTo;
	}
}

float UNocturneLens::Since(ENocturneLensPulse InKind) const
{
	const int32 Index = static_cast<int32>(InKind);
	const float X = (LensTime - PulseAt[Index]) / NocturneLensLocal::PulseSeconds[Index];
	return X >= 0.0f && X < 1.0f ? X : -1.0f;
}

void UNocturneLens::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (const UDreamWidgetTree* Tree = GetWidgetTree())
	{
		if (const UDreamWidget* Holder = Tree->FindWidgetByVariableName(TEXT("Grade")))
		{
			Stylize = Cast<UDreamStylizeEffect>(Holder->GetVisual());
		}
		if (const UDreamWidget* Holder = Tree->FindWidgetByVariableName(TEXT("Blocks")))
		{
			Pixelate = Cast<UDreamBackgroundPixelate>(Holder->GetVisual());
		}
	}
	Current = this;
	SetWantsTick(true);
}

void UNocturneLens::NativeOnDestruct()
{
	if (Current.Get() == this)
	{
		Current.Reset();
	}
	Super::NativeOnDestruct();
}

void UNocturneLens::NativeOnTick(float DeltaTime)
{
	using namespace NocturneLensLocal;
	Super::NativeOnTick(DeltaTime);
	LensTime += DeltaTime;
	if (TubeSeconds > 0.0f)
	{
		const float X = FMath::Clamp((LensTime - TubeAt) / TubeSeconds, 0.0f, 1.0f);
		Tube = FMath::Lerp(TubeFrom, TubeTo, Smooth(X));
	}

	// What each moment asks of the picture, summed.
	float Glitch = Damage > 0.05f ? 0.16f * Damage : 0.0f;
	float BlockShift = 14.0f;
	float Split = 0.0f;
	float Radial = 6.0f * FMath::Pow(Speed, 1.3f);
	float Blocks = 0.0f;
	float Scan = 0.0f;
	float Vignette = 0.4f * Tube;
	float Grain = 0.07f * Tube;

	const auto Strength = [this](ENocturneLensPulse InKind) { return PulseStrength[static_cast<int32>(InKind)]; };
	if (const float X = Since(ENocturneLensPulse::Transition); X >= 0.0f)
	{
		// Breaks into blocks that resolve in the first three tenths, glitching, a band of scanlines rolling over.
		const float S = Strength(ENocturneLensPulse::Transition);
		const float F = 1.0f - X;
		Blocks = FMath::Max(Blocks, 44.0f * S * FMath::Max(0.0f, 1.0f - X * 1.4f));
		Glitch += 0.9f * S * F * F;
		Scan += 0.4f * S * F;
		Split += 5.0f * S * F * F;
	}
	if (const float X = Since(ENocturneLensPulse::Hit); X >= 0.0f)
	{
		const float S = Strength(ENocturneLensPulse::Hit);
		const float F = 1.0f - X;
		Glitch += 1.5f * S * FMath::Pow(F, 1.5f);
		BlockShift += 24.0f * S * F;
		Split += 9.0f * S * F * F;
		Blocks = FMath::Max(Blocks, 22.0f * S * FMath::Max(0.0f, 1.0f - X * 3.0f));
		Vignette += 0.35f * S * F;
	}
	if (const float X = Since(ENocturneLensPulse::Wake); X >= 0.0f)
	{
		// The tube gives way to the system: big blocks resolving, a glitch, the scanlines rolled off.
		const float S = Strength(ENocturneLensPulse::Wake);
		const float F = 1.0f - X;
		Blocks = FMath::Max(Blocks, 60.0f * S * FMath::Max(0.0f, 1.0f - X * 1.25f));
		Glitch += 1.1f * S * F * F;
		Scan += 0.5f * S * F;
		Split += 4.0f * S * F;
	}
	if (const float X = Since(ENocturneLensPulse::Dive); X >= 0.0f)
	{
		// Faster and faster: the colours part outward and the edges close in; at the very end it breaks up.
		const float S = Strength(ENocturneLensPulse::Dive);
		const float Ramp = X < 0.85f ? FMath::Square(X / 0.85f) : (1.0f - X) / 0.15f;
		Radial += 12.0f * S * Ramp;
		Vignette += 0.55f * S * Ramp;
		Grain += 0.05f * S * Ramp;
		if (X > 0.75f)
		{
			Glitch += 1.2f * S * FMath::Sin((X - 0.75f) / 0.25f * PI);
		}
		if (X > 0.85f)
		{
			Blocks = FMath::Max(Blocks, 50.0f * S * FMath::Sin((X - 0.85f) / 0.15f * PI));
		}
	}

	if (UDreamBackgroundPixelate* Pix = Pixelate.Get())
	{
		// Strength 0 is a pass that draws nothing; below a block of two pixels, leave it off.
		const bool bOn = Blocks >= 6.0f;
		if (bOn)
		{
			Pix->SetPixelateStrength(Blocks);
		}
		if (bOn != bPixelateShown)
		{
			bPixelateShown = bOn;
			Pix->SetEnabled(bOn);
		}
	}

	UDreamStylizeEffect* Grade = Stylize.Get();
	if (Grade == nullptr)
	{
		return;
	}
	TArray<FDreamStylizeLayer> Layers;
	if (Glitch > 0.02f)
	{
		FDreamStylizeLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Type = EDreamStylizeType::Glitch;
		Layer.GlitchIntensity = FMath::Min(Glitch, 2.0f);
		Layer.BlockSize = 18.0f;
		Layer.BlockShift = BlockShift;
		Layer.ColorSplit = 3.0f;
		Layer.LineJitter = 5.0f;
		Layer.LineDensity = 0.12f;
		Layer.Rate = 18.0f;
		Layer.Seed = 7;
	}
	if (Tube > 0.01f || Scan > 0.02f)
	{
		FDreamStylizeLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Type = EDreamStylizeType::CRT;
		Layer.ScanlineStrength = FMath::Min(0.26f * Tube + Scan, 1.0f);
		Layer.ScanlineSpacing = 4.0f;
		Layer.Curvature = 0.12f * Tube;
		Layer.PhosphorStrength = 0.16f * Tube;
		Layer.PhosphorSize = 1.0f;
		Layer.FlickerStrength = 0.025f * Tube;
		Layer.FlickerFrequency = 30.0f;
	}
	if (Radial > 0.15f)
	{
		FDreamStylizeLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Type = EDreamStylizeType::ChromaticAberration;
		Layer.AberrationMode = EDreamChromaticAberrationMode::Radial;
		Layer.AberrationAmount = FMath::Min(Radial, 16.0f);
	}
	if (Split > 0.15f)
	{
		FDreamStylizeLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Type = EDreamStylizeType::ChromaticAberration;
		Layer.AberrationMode = EDreamChromaticAberrationMode::Directional;
		Layer.AberrationAmount = FMath::Min(Split, 16.0f);
		Layer.AberrationAngle = 90.0f;
	}
	if (Grain > 0.01f)
	{
		FDreamStylizeLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Type = EDreamStylizeType::FilmGrain;
		Layer.GrainIntensity = FMath::Min(Grain, 1.0f);
		Layer.GrainSize = 1.5f;
		Layer.bAnimateGrain = true;
	}
	if (Vignette > 0.01f)
	{
		FDreamStylizeLayer& Layer = Layers.AddDefaulted_GetRef();
		Layer.Type = EDreamStylizeType::Vignette;
		Layer.VignetteIntensity = FMath::Min(Vignette, 1.0f);
		Layer.VignetteRadius = 0.45f;
		Layer.VignetteSoftness = 0.55f;
		Layer.VignetteRoundness = 1.0f;
		Layer.VignetteColor = FColor::Black;
	}
	const bool bOn = Layers.Num() > 0;
	if (bOn)
	{
		Grade->SetLayers(Layers);
	}
	if (bOn != bStylizeShown)
	{
		bStylizeShown = bOn;
		Grade->SetEnabled(bOn);
	}
}
