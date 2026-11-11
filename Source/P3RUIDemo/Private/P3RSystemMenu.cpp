// Copyright DreamMoon. All Rights Reserved.

#include "P3RSystemMenu.h"

#include "P3RUI.h"

#include "Core/Components/DreamWidget.h"
#include "Extensions/DreamMaskTransition.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace P3RSystemMenuLocal
{
	const TCHAR* Guides[] = {
		TEXT("查看教程"), TEXT("更改游戏设置"), TEXT("查看用语词典"), TEXT("读取存档"), TEXT("保存进度"),
		TEXT("回到稍早之前"), TEXT("返回标题画面"),
	};
}

void UP3RSystemMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	Menu.Layers = { 1, 0, 2, 1, 2, 0, 1 };
	Menu.Init(this, TEXT("Cmd"), Count, TEXT("Cursor"), TEXT("CursorPink"), 1.3f);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		WireButton(Menu.GetWord(Index), [this, Index]() { Select(Index); }, [this, Index]() { Choose(Index); });
	}
	if (UDreamWidget* Content = GetContentRoot())
	{
		Blot = UDreamMaskTransition::FindOrAdd(Content);
	}
	Disc = Node(TEXT("Disc"));
	Giant = Node(TEXT("Giant"));
	Nine = Node(TEXT("Nine"));
	Hero = Node(TEXT("Hero"));
	Guide = Node(TEXT("Guide"));
	// Half over white paper: his blazer stays solid here (the picture's own blazer is indigo, see toon.py).
	if (UMaterialInstanceDynamic* Figure = P3RUI::MakeFigure(Hero.Get(), this))
	{
		Figure->SetScalarParameterValue(TEXT("SeeThrough"), 0.0f);
	}
	Select(Tutorial);
}

void UP3RSystemMenu::Select(int32 Index)
{
	Menu.Select(Index, GetScreenTime());
	P3RUI::SetText(Guide.Get(), FText::FromString(P3RSystemMenuLocal::Guides[Menu.GetSelected()]));
}

void UP3RSystemMenu::Choose(int32 Index)
{
	Select(Index);
	OnChosen.Broadcast(Menu.GetSelected());
}

void UP3RSystemMenu::Deny()
{
	Menu.Deny(GetScreenTime());
	P3RUI::SetText(Guide.Get(), FText::FromString(TEXT("这次的复刻没有收录这一项")));
}

bool UP3RSystemMenu::HandleKey(EP3RKey Key)
{
	switch (Key)
	{
	case EP3RKey::Up: Select(Menu.GetSelected() - 1); return true;
	case EP3RKey::Down: Select(Menu.GetSelected() + 1); return true;
	case EP3RKey::Confirm: Choose(Menu.GetSelected()); return true;
	case EP3RKey::Back: OnBack.Broadcast(); return true;
	default: return false;
	}
}

void UP3RSystemMenu::PlayEntrance()
{
	EnterAt = GetScreenTime();
	PlayBlotIn(Blot.Get());
	if (UDreamWidget* D = Disc.Get())
	{
		P3RUI::Settle(D);
		P3RUI::SetScale2D(D, FVector2D(0.72));
		D->SetRenderRotation(FRotator(0.0, 0.0, 28.0));
		P3RUI::Scale2DTo(D, FVector2D(1.0), 0.45f, 0.0f, EDreamTweenEase::OutCubic);
		P3RUI::RotateTo(D, FRotator::ZeroRotator, 0.5f, 0.0f, EDreamTweenEase::OutCubic);
	}
	if (UDreamWidget* G = Giant.Get())
	{
		// Along its own slope, from the upper left.
		P3RUI::Settle(G);
		P3RUI::SetOffset(G, FVector2D(-420.0, -300.0));
		G->SetRenderOpacity(1.0f);
		P3RUI::OffsetTo(G, FVector2D::ZeroVector, 0.4f, 0.04f, EDreamTweenEase::OutCubic);
	}
	if (UDreamWidget* N = Nine.Get())
	{
		P3RUI::Settle(N);
		P3RUI::SetOffset(N, FVector2D(0.0, -420.0));
		N->SetRenderOpacity(1.0f);
		P3RUI::OffsetTo(N, FVector2D::ZeroVector, 0.42f, 0.08f, EDreamTweenEase::OutBack);
	}
	if (UDreamWidget* H = Hero.Get())
	{
		P3RUI::Settle(H);
		H->SetRenderOpacity(0.0f);
		P3RUI::FadeTo(H, 1.0f, 0.25f, 0.12f);
	}
	Menu.DropIn(GetScreenTime(), 0.18f);
	LockInput(0.3f);
}

float UP3RSystemMenu::PlayExit()
{
	if (!IsExitingToMenu())
	{
		Menu.Vanish();
	}
	return PlayBlotOut(Blot.Get());
}

void UP3RSystemMenu::TickScreen(float DeltaTime)
{
	const float Now = GetScreenTime();
	Menu.Tick(Now);

	// The protagonist rises into reach of the 9 and hangs there, swaying.
	if (UDreamWidget* H = Hero.Get())
	{
		const float E = Now - EnterAt;
		const float Rise = 1.0f - P3RUI::EaseOutCubic(E / 0.55f);
		P3RUI::SetOffset(H, FVector2D(FMath::Sin(E * UE_TWO_PI / 5.0f) * 5.0f, 320.0f * Rise + FMath::Sin(E * UE_TWO_PI / 3.6f) * 7.0f));
		H->SetRenderRotation(FRotator(0.0, 0.0, FMath::Sin(E * UE_TWO_PI / 5.0f) * 1.2f));
	}
	// The disc keeps turning, very slowly, so the sunk words drift.
	if (UDreamWidget* D = Disc.Get(); D != nullptr && Now - EnterAt > 0.5f)
	{
		D->SetRenderRotation(FRotator(0.0, 0.0, FMath::Sin((Now - EnterAt - 0.5f) * 0.25f) * 2.0f));
	}
}
