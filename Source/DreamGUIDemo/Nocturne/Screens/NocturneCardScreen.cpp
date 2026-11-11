// Copyright DreamMoon. All Rights Reserved.

#include "NocturneCardScreen.h"

#include "Core/Components/DreamWidget.h"

void UNocturneCardScreen::PlayContentEntrance(float Delay)
{
	// The letters drop in from above one by one, flipping up from lying back.
	if (UDreamWidget* Word = Node(TEXT("Word")))
	{
		int32 Index = 0;
		for (UDreamWidget* Letter : Word->GetChildren())
		{
			RunIn(Letter, Delay + 0.25f + 0.07f * Index++, 0.4f, FVector2D(0.0, -46.0));
		}
	}
	if (UDreamWidget* Rule = Node(TEXT("Rule")))
	{
		Settle(Rule);
		Rule->SetRenderScale(FVector(1.0, 0.0, 1.0));
		Rule->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.6f, Delay + 0.7f, EDreamTweenEase::OutCubic);
	}
	RunInChildren(Node(TEXT("Lines")), Delay + 0.95f, 0.12f, FVector2D(-40.0, 0.0));
	if (UDreamWidget* Art = Node(TEXT("Art")))
	{
		const FRestPose Rest = GetRest(Art);
		Settle(Art);
		Art->SetRenderOpacity(0.0f);
		Art->SetRenderTranslation(Rest.Translation + FVector(60.0, 320.0, 0.0));
		Art->RenderTranslationTo(Rest.Translation, 1.1f, Delay + 0.15f, EDreamTweenEase::OutCubic);
		Art->RenderOpacityTo(1.0f, 0.5f, Delay + 0.15f, EDreamTweenEase::OutQuad);
	}
	BurstSort(CenterSortIndex, 0.5f, 0.45f, Delay + 0.2f);
}
