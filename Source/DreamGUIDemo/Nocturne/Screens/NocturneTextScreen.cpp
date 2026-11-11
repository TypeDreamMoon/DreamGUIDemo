// Copyright DreamMoon. All Rights Reserved.

#include "NocturneTextScreen.h"

#include "../Settings/NocturneUI.h"
#include "Core/Components/DreamText.h"
#include "Core/Components/DreamWidget.h"

namespace NocturneTextLocal
{
	constexpr float HeroAt = 1.5f;
	constexpr float GrowAt = 1.8f;
	constexpr float GrowSeconds = 1.6f;
	constexpr float TurnAt = 3.5f;
	constexpr float TurnSeconds = 1.1f;
	constexpr float AsideAt = 4.9f;
	constexpr float AsideSeconds = 0.6f;
	constexpr float ScriptsAt = 5.6f;
	constexpr float EmojiAt = 6.4f;
	constexpr float GradientAt = 7.1f;

	constexpr double MaxScale = 14.0;
	// Seen at a slant: turned away about the vertical and tipped back.
	constexpr double TurnYaw = -52.0;
	constexpr double TurnPitch = 16.0;
	// Out of the way: up near the top of the stage, at a third of its full size, still turned a little.
	constexpr double AsideScale = 4.2;
	constexpr double AsideUp = 150.0;
	constexpr double AsideYaw = -18.0;
	const FColor Ink(0xD7, 0xE2, 0xEA);
	const FColor Cyan(0x8F, 0xE3, 0xFF);
}

float UNocturneTextScreen::Smooth(float InFrom, float InSeconds) const
{
	const float T = FMath::Clamp((GetScreenTime() - StartTime - InFrom) / FMath::Max(InSeconds, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	return T * T * (3.0f - 2.0f * T);
}

void UNocturneTextScreen::PlayContentEntrance(float Delay)
{
	StartTime = GetScreenTime();
	bStarted = true;
	Lit = -1;
	LinesShown = 0;
	RunInChildren(Node(TEXT("SpecList")), Delay + 0.08f, 0.05f, FVector2D(-30.0, 0.0));
}

void UNocturneTextScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturneTextLocal;
	Super::NativeOnTick(DeltaTime);
	if (!bStarted)
	{
		return;
	}
	const float T = GetScreenTime() - StartTime;

	// The hero line: grows, turns, then steps aside.
	if (UDreamWidget* Hero = Node(TEXT("Hero")))
	{
		const float Grow = Smooth(GrowAt, GrowSeconds);
		const float Turn = Smooth(TurnAt, TurnSeconds);
		const float Aside = Smooth(AsideAt, AsideSeconds);
		// Growth runs on a log scale, so each moment it doubles at the same pace instead of crawling then leaping.
		const double Scale = FMath::Lerp(FMath::Pow(MaxScale, static_cast<double>(Grow)), AsideScale, static_cast<double>(Aside));
		Hero->SetRenderScale(FVector(1.0, Scale, Scale));
		Hero->SetRenderRotation(FRotator(TurnPitch * Turn * (1.0 - Aside), FMath::Lerp(TurnYaw * Turn, AsideYaw, Aside), 0.0));
		Hero->SetRenderTranslation(FVector(0.0, 0.0, AsideUp * Aside));
		Hero->SetRenderOpacity(FMath::Clamp((T - HeroAt) / 0.25f, 0.0f, 1.0f));

		if (T >= GrowAt && T < AsideAt + AsideSeconds)
		{
			NocturneUI::SetText(Node(TEXT("Spec1Value")), FText::FromString(FString::Printf(TEXT("12 PX  ×%.1f"), Scale)));
		}
		if (T >= TurnAt && T < AsideAt + AsideSeconds)
		{
			const double Yaw = FMath::Lerp(TurnYaw * Turn, AsideYaw, Aside);
			NocturneUI::SetText(Node(TEXT("Spec2Value")), FText::FromString(FString::Printf(TEXT("%d°"), FMath::RoundToInt(Yaw))));
		}
	}

	// The rows: rendering as the line appears, scale as it grows, angle as it turns, then one per line below.
	const float RowAt[] = { HeroAt, GrowAt, TurnAt, ScriptsAt, EmojiAt, GradientAt };
	for (int32 Index = Lit + 1; Index < UE_ARRAY_COUNT(RowAt); ++Index)
	{
		if (T >= RowAt[Index])
		{
			LightSpec(Index);
		}
	}

	// The three lines run in, one after another.
	const TPair<float, const TCHAR*> Lines[] = { { ScriptsAt, TEXT("LineScripts") }, { EmojiAt, TEXT("LineEmoji") },
		{ GradientAt, TEXT("LineGradient") } };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Lines); ++Index)
	{
		if ((LinesShown & (1 << Index)) == 0 && T >= Lines[Index].Key)
		{
			LinesShown |= 1 << Index;
			RunIn(Node(Lines[Index].Value), 0.0f, 0.35f, FVector2D(0.0, 26.0));
			BurstSort(CenterSortIndex, 0.25f, 0.3f);
		}
	}

	// The shimmer runs over the gradient title, once a second and a half, from when it arrives.
	if (UDreamWidget* Title = Node(TEXT("LineGradient")))
	{
		if (UDreamText* Text = Cast<UDreamText>(Title->GetVisual()))
		{
			const float Since = T - GradientAt - 0.2f;
			Text->SetOverlayPaintPhase(Since < 0.0f ? -1.0f : FMath::Fmod(Since / 1.5f, 1.0f) * 2.0f - 1.0f);
		}
	}
}

void UNocturneTextScreen::LightSpec(int32 InIndex)
{
	using namespace NocturneTextLocal;
	if (Lit >= 0)
	{
		if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Spec%dBar"), Lit)))
		{
			Bar->RenderOpacityTo(0.0f, 0.2f, 0.0f, EDreamTweenEase::OutQuad);
		}
	}
	Lit = InIndex;
	if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Spec%dBar"), InIndex)))
	{
		Bar->RenderOpacityTo(1.0f, 0.15f, 0.0f, EDreamTweenEase::OutQuad);
	}
	NocturneUI::SetTextColor(Node(*FString::Printf(TEXT("Spec%dValue"), InIndex)), Cyan);
	NocturneUI::SetTextColor(Node(*FString::Printf(TEXT("Spec%dKey"), InIndex)), Ink);
}
