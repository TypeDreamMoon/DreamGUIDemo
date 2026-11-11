// Copyright DreamMoon. All Rights Reserved.

#include "P3RMainMenu.h"

#include "P3RUI.h"

#include "Core/Components/DreamWidget.h"
#include "Extensions/DreamMaskTransition.h"

namespace P3RMainMenuLocal
{
	const TCHAR* Guides[] = {
		TEXT("使用技能"), TEXT("使用道具"), TEXT("确认 / 更换装备"), TEXT("确认 / 更换人格面具"), TEXT("查看状态 / 编组队伍"),
		TEXT("查看委托"), TEXT("查看羁绊"), TEXT("查看日程"), TEXT("系统设置"),
	};
}

void UP3RMainMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// The game's own stacking of the overlapping words, as measured by those who rebuilt it.
	Menu.Layers = { 1, 0, 1, 2, 0, 1, 2, 1, 0 };
	Menu.Init(this, TEXT("Word"), Count, TEXT("Cursor"), TEXT("CursorPink"), 1.35f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		WireButton(Menu.GetWord(Index), [this, Index]() { Select(Index); }, [this, Index]() { Choose(Index); });
	}

	Hero = Node(TEXT("Hero"));
	Slab = Node(TEXT("Slab"));
	Number = Node(TEXT("Number"));
	MainWord = Node(TEXT("MainWord"));
	Guide = Node(TEXT("Guide"));
	GuideRule = Node(TEXT("GuideRule"));
	Design = Node(TEXT("Design"));
	Sweep = Node(TEXT("Sweep"));
	Flash = Node(TEXT("Flash"));
	if (UDreamWidget* Content = GetContentRoot())
	{
		Reveal = UDreamMaskTransition::FindOrAdd(Content);
		if (UDreamMaskTransition* Mask = Reveal.Get())
		{
			Mask->SetPreset(EDreamMaskTransitionPreset::Iris);
			Mask->SetCentre(FVector2D(0.88, 0.1));
			Mask->SetSoftness(0.02f);
			Mask->SetProgress(1.0f);
		}
	}
	P3RUI::MakeFigure(Hero.Get(), this);
	P3RUI::MakeFigure(Sweep.Get(), this, 1.0f);
	if (UDreamWidget* H = Hero.Get())
	{
		HeroRoll = H->GetRenderRotation().Roll;
	}
	for (const TCHAR* Name : { TEXT("Money"), TEXT("Guide"), TEXT("GuideTag"), TEXT("GuideRule"), TEXT("Hints"), TEXT("Shards"), TEXT("Design") })
	{
		Chrome.Add(Node(Name));
	}
	Select(Skill);
}

FText UP3RMainMenu::GetGuide(int32 Item)
{
	return FText::FromString(P3RMainMenuLocal::Guides[FMath::Clamp(Item, 0, Count - 1)]);
}

void UP3RMainMenu::SetDesignLabel(const FText& InLabel)
{
	P3RUI::SetText(Design.Get(), InLabel);
}

void UP3RMainMenu::Select(int32 Index)
{
	Menu.Select(Index, GetScreenTime());
	const int32 Chosen = Menu.GetSelected();
	P3RUI::SetText(Number.Get(), FText::FromString(FString::Printf(TEXT("%02d"), Chosen + 1)));
	P3RUI::SetText(Guide.Get(), FText::FromString(P3RMainMenuLocal::Guides[Chosen]));
}

void UP3RMainMenu::Choose(int32 Index)
{
	Select(Index);
	OnChosen.Broadcast(Menu.GetSelected());
}

void UP3RMainMenu::Deny()
{
	Menu.Deny(GetScreenTime());
	P3RUI::SetText(Guide.Get(), FText::FromString(TEXT("这次的复刻没有收录这一项")));
}

bool UP3RMainMenu::HandleKey(EP3RKey Key)
{
	switch (Key)
	{
	case EP3RKey::Up: Select(Menu.GetSelected() - 1); return true;
	case EP3RKey::Down: Select(Menu.GetSelected() + 1); return true;
	case EP3RKey::Confirm: Choose(Menu.GetSelected()); return true;
	case EP3RKey::Back: OnBack.Broadcast(); return true;
	case EP3RKey::Design: OnDesign.Broadcast(); return true;
	default: return false;
	}
}

void UP3RMainMenu::PlayEntrance()
{
	bLeavingToSub = false;
	bLeavingToPage = false;
	SweepAt = -100.0f;
	P3RUI::SetShown(Sweep.Get(), false);
	if (bReturning)
	{
		// Back from a page: everything is in place, shown through the hole the page leaves by (the same iris, the same
		// timing), and lit for a moment.
		bReturning = false;
		if (UDreamMaskTransition* Mask = Reveal.Get())
		{
			Mask->PlayFromTo(0.0f, 1.0f, 0.24f, EDreamTweenEase::InQuad);
		}
		const float Now = GetScreenTime();
		Menu.ShowNow(Now);
		HeroInAt = Now - 1.0f;
		if (UDreamWidget* H = Hero.Get())
		{
			P3RUI::Settle(H);
			H->SetRenderOpacity(1.0f);
		}
		for (UDreamWidget* Part : { Slab.Get(), Number.Get(), MainWord.Get() })
		{
			if (Part != nullptr)
			{
				P3RUI::Settle(Part);
				P3RUI::SetOffset(Part, FVector2D::ZeroVector);
				Part->SetRenderOpacity(1.0f);
			}
		}
		for (const TWeakObjectPtr<UDreamWidget>& Weak : Chrome)
		{
			if (UDreamWidget* Part = Weak.Get())
			{
				P3RUI::Settle(Part);
				Part->SetRenderOpacity(1.0f);
			}
		}
		P3RUI::SetScale2D(GuideRule.Get(), FVector2D(1.0));
		if (UDreamWidget* F = Flash.Get())
		{
			P3RUI::Settle(F);
			P3RUI::SetShown(F, true);
			F->SetRenderOpacity(0.7f);
			P3RUI::FadeTo(F, 0.0f, 0.32f, 0.08f, EDreamTweenEase::OutCubic);
		}
		LockInput(0.25f);
		return;
	}
	if (UDreamMaskTransition* Mask = Reveal.Get())
	{
		Mask->Stop();
		Mask->SetProgress(1.0f);
	}
	Menu.DropIn(GetScreenTime(), 0.08f);

	// The protagonist sinks into view; the slab slides in from the left, MAIN and the number from above.
	HeroInAt = GetScreenTime();
	if (UDreamWidget* H = Hero.Get())
	{
		P3RUI::Settle(H);
		H->SetRenderOpacity(0.0f);
		P3RUI::FadeTo(H, 1.0f, 0.18f, 0.0f, EDreamTweenEase::Linear);
	}
	if (UDreamWidget* S = Slab.Get())
	{
		P3RUI::Settle(S);
		S->SetRenderOpacity(1.0f);
		P3RUI::SetOffset(S, FVector2D(-560.0, 0.0));
		P3RUI::OffsetTo(S, FVector2D::ZeroVector, 0.32f, 0.0f, EDreamTweenEase::OutCubic);
	}
	for (UDreamWidget* Big : { Number.Get(), MainWord.Get() })
	{
		if (Big != nullptr)
		{
			P3RUI::Settle(Big);
			Big->SetRenderOpacity(1.0f);
			P3RUI::SetOffset(Big, FVector2D(0.0, -420.0));
			P3RUI::OffsetTo(Big, FVector2D::ZeroVector, 0.42f, 0.06f, EDreamTweenEase::OutCubic);
		}
	}
	for (const TWeakObjectPtr<UDreamWidget>& Weak : Chrome)
	{
		if (UDreamWidget* Fade = Weak.Get())
		{
			P3RUI::Settle(Fade);
			Fade->SetRenderOpacity(0.0f);
			P3RUI::FadeTo(Fade, 1.0f, 0.3f, 0.25f);
		}
	}
	if (UDreamWidget* Rule = GuideRule.Get())
	{
		P3RUI::SetScale2D(Rule, FVector2D(0.0, 1.0));
		P3RUI::Scale2DTo(Rule, FVector2D(1.0, 1.0), 0.35f, 0.3f);
	}
	LockInput(0.25f);
}

float UP3RMainMenu::PlayExit()
{
	if (bLeavingToPage)
	{
		// The protagonist swings round to the camera, a dark-blue shape sweeping across the menu; the page then spreads
		// out over everything from the chosen word. The words stay where they are until it has.
		bLeavingToPage = false;
		SweepAt = GetScreenTime();
		if (UDreamWidget* S = Sweep.Get())
		{
			P3RUI::Settle(S);
			P3RUI::SetShown(S, true);
			S->SetRenderOpacity(0.0f);
			P3RUI::FadeTo(S, 1.0f, 0.06f, 0.0f, EDreamTweenEase::Linear);
		}
		return 0.5f;
	}
	if (bLeavingToSub)
	{
		// Into a sub-screen: the words fly off, the protagonist sinks out of sight.
		Menu.FlyOut();
		if (UDreamWidget* H = Hero.Get())
		{
			P3RUI::Settle(H);
			P3RUI::OffsetTo(H, P3RUI::GetOffset(H) + FVector2D(0.0, 260.0), 0.3f, 0.0f, EDreamTweenEase::InCubic);
			P3RUI::FadeTo(H, 0.0f, 0.3f, 0.0f, EDreamTweenEase::InCubic);
		}
	}
	else
	{
		// Closing: the words are gone at once, the protagonist rises out of the water.
		Menu.Vanish();
		if (UDreamWidget* H = Hero.Get())
		{
			P3RUI::Settle(H);
			P3RUI::OffsetTo(H, P3RUI::GetOffset(H) + FVector2D(0.0, -260.0), 0.28f, 0.0f, EDreamTweenEase::InCubic);
			P3RUI::FadeTo(H, 0.0f, 0.28f, 0.0f, EDreamTweenEase::InCubic);
		}
	}
	HeroInAt = -100.0f;
	if (UDreamWidget* S = Slab.Get())
	{
		P3RUI::Settle(S);
		P3RUI::OffsetTo(S, FVector2D(-560.0, 0.0), 0.26f, 0.0f, EDreamTweenEase::InCubic);
	}
	for (UDreamWidget* Big : { Number.Get(), MainWord.Get() })
	{
		P3RUI::Settle(Big);
		P3RUI::FadeTo(Big, 0.0f, 0.18f);
	}
	for (const TWeakObjectPtr<UDreamWidget>& Weak : Chrome)
	{
		P3RUI::Settle(Weak.Get());
		P3RUI::FadeTo(Weak.Get(), 0.0f, 0.18f);
	}
	return 0.32f;
}

void UP3RMainMenu::TickScreen(float DeltaTime)
{
	const float Now = GetScreenTime();
	Menu.Tick(Now);

	// The sweep: in from the lower left, turning, fast at first and then hardly moving.
	if (UDreamWidget* S = Sweep.Get(); S != nullptr && SweepAt > -50.0f)
	{
		const float K = P3RUI::EaseOutCubic(FMath::Clamp((Now - SweepAt) / 0.16f, 0.0f, 1.0f));
		const float Drift = (Now - SweepAt) * 30.0f;
		P3RUI::SetOffset(S, FVector2D(FMath::Lerp(-900.0, 0.0, (double)K) + Drift, FMath::Lerp(380.0, 0.0, (double)K)));
		S->SetRenderRotation(FRotator(0.0, 0.0, FMath::Lerp(-34.0f, -10.0f, K)));
		P3RUI::SetScale2D(S, FVector2D(FMath::Lerp(0.8f, 1.0f, K)));
	}

	// The protagonist: sinks in, then drifts and sways in the water.
	if (UDreamWidget* H = Hero.Get(); H != nullptr && HeroInAt > -50.0f)
	{
		const float E = Now - HeroInAt;
		const float Fall = 1.0f - P3RUI::EaseOutCubic(E / 0.45f);
		const float Bob = FMath::Sin(E * UE_TWO_PI / 4.2f) * 9.0f;
		P3RUI::SetOffset(H, FVector2D(FMath::Sin(E * UE_TWO_PI / 7.0f) * 6.0f, -260.0f * Fall + Bob));
		H->SetRenderRotation(FRotator(0.0, 0.0, HeroRoll + FMath::Sin(E * UE_TWO_PI / 6.0f) * 1.4f));
	}
}
