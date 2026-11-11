// Copyright DreamMoon. All Rights Reserved.

#include "P3RBackdrop.h"

#include "P3RUI.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamWidget.h"
#include "Extensions/DreamMaskTransition.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace P3RBackdropLocal
{
	/** The reveal is drawn on ten frames, each held for 50 ms (three frames at 60 Hz). */
	constexpr int32 InkFrames = 10;
}

void UP3RBackdrop::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (UDreamWidget* SeaWidget = Node(TEXT("Sea")))
	{
		if (UDreamImage* Image = Cast<UDreamImage>(SeaWidget->GetVisual()))
		{
			if (UMaterialInterface* SeaParent = Cast<UMaterialInterface>(Image->GetBrush().GetResourceObject()))
			{
				SeaMaterial = UMaterialInstanceDynamic::Create(SeaParent, this);
				Image->SetBrushFromMaterial(SeaMaterial);
			}
		}
		Ink = UDreamMaskTransition::FindOrAdd(SeaWidget);
		if (UDreamMaskTransition* Mask = Ink.Get())
		{
			Mask->SetProgress(0.0f);
		}
	}
	Bubbles = Node(TEXT("Bubbles"));
	SetSeaParam(TEXT("Dim"), 0.0f);
}

void UP3RBackdrop::SetSeaParam(FName Name, float Value)
{
	if (SeaMaterial != nullptr)
	{
		SeaMaterial->SetScalarParameterValue(Name, Value);
	}
}

void UP3RBackdrop::SetSea(bool bInInSea)
{
	if (bInSea == bInInSea)
	{
		return;
	}
	bInSea = bInInSea;
	InkFrom = Ink.IsValid() ? Ink->GetProgress() : (bInSea ? 0.0f : 1.0f);
	InkTo = bInSea ? 1.0f : 0.0f;
	InkStart = GetScreenTime();
	P3RUI::FadeTo(Bubbles.Get(), bInSea ? 1.0f : 0.0f, bInSea ? 0.8f : 0.25f, bInSea ? 0.3f : 0.0f);
	if (!bInSea)
	{
		SetDim(0.0f, 0.2f);
	}
}

void UP3RBackdrop::SetDim(float InDim, float Seconds)
{
	DimFrom = Dim;
	DimTo = InDim;
	DimStart = GetScreenTime();
	DimSeconds = FMath::Max(Seconds, 0.01f);
}

void UP3RBackdrop::TickScreen(float DeltaTime)
{
	using namespace P3RBackdropLocal;

	// The ink mask: held frames, not a smooth sweep.
	if (UDreamMaskTransition* Mask = Ink.Get())
	{
		const float T = (GetScreenTime() - InkStart) / InkSeconds;
		if (T >= 0.0f && T <= 1.0f + 1e-3f)
		{
			const float Held = FMath::FloorToFloat(FMath::Clamp(T, 0.0f, 1.0f) * InkFrames) / InkFrames;
			Mask->SetProgress(FMath::Lerp(InkFrom, InkTo, Held));
		}
		else if (T > 1.0f && !FMath::IsNearlyEqual(Mask->GetProgress(), InkTo))
		{
			Mask->SetProgress(InkTo);
		}
	}

	const float D = (GetScreenTime() - DimStart) / DimSeconds;
	if (D >= 0.0f && D <= 1.0f + DeltaTime / DimSeconds)
	{
		Dim = FMath::Lerp(DimFrom, DimTo, P3RUI::EaseOutCubic(FMath::Min(D, 1.0f)));
		SetSeaParam(TEXT("Dim"), Dim);
	}
}
