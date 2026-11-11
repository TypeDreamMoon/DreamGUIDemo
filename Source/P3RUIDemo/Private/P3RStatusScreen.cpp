// Copyright DreamMoon. All Rights Reserved.

#include "P3RStatusScreen.h"

#include "P3RPartyRow.h"
#include "P3RUI.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamWidget.h"
#include "Engine/Texture2D.h"
#include "Extensions/DreamMaskTransition.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace P3RStatusScreenLocal
{
	const FP3RMember Party[UP3RStatusScreen::Count] = {
		{ TEXT("LEADER"), TEXT("朝雾 凛"), FColor(0x3A, 0x7B, 0xFF), 28, 280, 312, 183, 201 },
		{ TEXT("PARTY"), TEXT("七海 唯"), FColor(0xFF, 0x4F, 0xA8), 28, 245, 268, 207, 230 },
		{ TEXT("PARTY"), TEXT("久我 大地"), FColor(0x2E, 0x56, 0xE8), 26, 269, 301, 161, 170 },
		{ TEXT(""), TEXT("鹰司 冬真"), FColor(0x9A, 0xA3, 0xB5), 24, 294, 294, 167, 180 },
		{ TEXT("PARTY"), TEXT("绯村 玲华"), FColor(0xE8, 0x32, 0x3C), 27, 255, 290, 205, 214 },
		{ TEXT(""), TEXT("艾琳"), FColor(0xF2, 0xC2, 0x30), 27, 277, 320, 149, 168 },
		{ TEXT("NAVI"), TEXT("水原 栞"), FColor(0x3E, 0xD6, 0x8A), 28, 223, 240, 205, 205 },
	};
	const TCHAR* Modes[] = { TEXT("状态确认"), TEXT("装备确认"), TEXT("技能确认") };
	constexpr int32 ModeCount = 3;
	constexpr float FlipSeconds = 0.18f;
}

void UP3RStatusScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace P3RStatusScreenLocal;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		UP3RPartyRow* Row = Cast<UP3RPartyRow>(Node(FString::Printf(TEXT("Row%d"), Index)));
		Rows[Index] = Row;
		Eyes[Index] = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/P3R/Textures/T_P3R_Eyes_%d.T_P3R_Eyes_%d"), Index, Index));
		if (Row != nullptr)
		{
			UTexture2D* Face = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/P3R/Textures/T_P3R_Face_%d.T_P3R_Face_%d"), Index, Index));
			Row->SetMember(Party[Index], Face);
			WireButton(Row->GetContentRoot(), [this, Index]() { Select(Index); }, [this, Index]() { Select(Index); });
		}
	}
	if (UDreamWidget* Content = GetContentRoot())
	{
		Blot = UDreamMaskTransition::FindOrAdd(Content);
	}
	Disc = Node(TEXT("Disc"));
	Header = Node(TEXT("Header"));
	Mode = Node(TEXT("Mode"));
	TopBand = Node(TEXT("TopBand"));
	HeroGroup = Node(TEXT("HeroGroup"));
	ShardEyes = Node(TEXT("ShardEyes"));
	ShardFlash = Node(TEXT("ShardFlash"));
	// In front of the white disc: solid (the sea-through of the figure material would show the disc as a pale wash).
	for (const TCHAR* Name : { TEXT("Hero"), TEXT("HeroFront") })
	{
		if (UMaterialInstanceDynamic* Figure = P3RUI::MakeFigure(Node(Name), this))
		{
			Figure->SetScalarParameterValue(TEXT("SeeThrough"), 0.0f);
		}
	}
	Select(0, /*bInstant*/true);
}

void UP3RStatusScreen::Select(int32 Index, bool bInstant)
{
	Index = (Index + Count) % Count;
	if (Index == Selected && !bInstant)
	{
		return;
	}
	Selected = Index;
	for (int32 I = 0; I < Count; ++I)
	{
		UP3RPartyRow* Row = Rows[I].Get();
		if (Row == nullptr)
		{
			continue;
		}
		const bool bChosen = I == Selected;
		const FVector2D Offset(bChosen ? 22.0 : 0.0, 0.0);
		if (bInstant)
		{
			P3RUI::SetOffset(Row, Offset);
		}
		else
		{
			P3RUI::Settle(Row);
			Row->SetRenderOpacity(1.0f);
			P3RUI::OffsetTo(Row, Offset, 0.1f, 0.0f, EDreamTweenEase::OutCubic);
		}
		// After the settling above: it would kill the wipe this starts.
		Row->SetChosen(bChosen, bInstant);
	}

	ShowEyes(Selected, bInstant);
}

void UP3RStatusScreen::ShowEyes(int32 Index, bool bInstant)
{
	UDreamWidget* EyesWidget = ShardEyes.Get();
	if (UDreamImage* Image = EyesWidget != nullptr ? Cast<UDreamImage>(EyesWidget->GetVisual()) : nullptr; Image != nullptr && Eyes[Index] != nullptr)
	{
		FDreamUIImageBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Eyes[Index]);
		Image->SetBrush(Brush);
	}
	UDreamWidget* Flash = ShardFlash.Get();
	if (bInstant || EyesWidget == nullptr || Flash == nullptr)
	{
		P3RUI::SetOffset(EyesWidget, FVector2D::ZeroVector);
		if (Flash != nullptr)
		{
			Flash->SetRenderOpacity(0.0f);
		}
		return;
	}
	// The glass goes white for a couple of frames, then the new eyes slide in under it from the right as it clears.
	P3RUI::Settle(Flash);
	Flash->SetRenderOpacity(1.0f);
	P3RUI::FadeTo(Flash, 0.0f, 0.14f, 0.07f, EDreamTweenEase::InQuad);
	P3RUI::Settle(EyesWidget);
	P3RUI::SetOffset(EyesWidget, FVector2D(260.0, -30.0));
	P3RUI::OffsetTo(EyesWidget, FVector2D::ZeroVector, 0.16f, 0.06f, EDreamTweenEase::OutCubic);
}

void UP3RStatusScreen::FlipMode(int32 Delta)
{
	using namespace P3RStatusScreenLocal;
	PendingMode = (ModeIndex + Delta + ModeCount) % ModeCount;
	FlipAt = GetScreenTime();
}

bool UP3RStatusScreen::HandleKey(EP3RKey Key)
{
	switch (Key)
	{
	case EP3RKey::Up: Select(Selected - 1); return true;
	case EP3RKey::Down: Select(Selected + 1); return true;
	case EP3RKey::PrevTab: FlipMode(-1); return true;
	case EP3RKey::NextTab: FlipMode(1); return true;
	case EP3RKey::Back: OnBack.Broadcast(); return true;
	case EP3RKey::Confirm: return true;
	default: return false;
	}
}

void UP3RStatusScreen::PlayEntrance()
{
	EnterAt = GetScreenTime();
	PlayBlotIn(Blot.Get());
	// The bands slide out from the left edge, top first.
	for (int32 I = 0; I < Count; ++I)
	{
		if (UP3RPartyRow* Row = Rows[I].Get())
		{
			P3RUI::Settle(Row);
			const double Out = I == Selected ? 22.0 : 0.0;
			const float Delay = 0.08f + I * 0.03f;
			P3RUI::SetOffset(Row, FVector2D(-980.0, 0.0));
			Row->SetRenderOpacity(1.0f);
			P3RUI::OffsetTo(Row, FVector2D(Out, 0.0), 0.3f, Delay, EDreamTweenEase::OutCubic);
		}
	}
	if (UDreamWidget* G = HeroGroup.Get())
	{
		P3RUI::Settle(G);
		G->SetRenderOpacity(0.0f);
		P3RUI::SetOffset(G, FVector2D(160.0, 30.0));
		P3RUI::OffsetTo(G, FVector2D::ZeroVector, 0.45f, 0.06f, EDreamTweenEase::OutCubic);
		P3RUI::FadeTo(G, 1.0f, 0.2f, 0.06f);
	}
	ShowEyes(Selected, /*bInstant*/true);
	if (UDreamWidget* D = Disc.Get())
	{
		P3RUI::Settle(D);
		P3RUI::SetScale2D(D, FVector2D(0.6));
		P3RUI::Scale2DTo(D, FVector2D(1.0), 0.4f, 0.0f, EDreamTweenEase::OutBack);
	}
	for (UDreamWidget* Top : { Header.Get(), TopBand.Get() })
	{
		if (Top != nullptr)
		{
			P3RUI::Settle(Top);
			P3RUI::SetOffset(Top, FVector2D(300.0, -200.0));
			P3RUI::OffsetTo(Top, FVector2D::ZeroVector, 0.35f, 0.06f, EDreamTweenEase::OutCubic);
		}
	}
	LockInput(0.3f);
}

float UP3RStatusScreen::PlayExit()
{
	return PlayBlotOut(Blot.Get());
}

void UP3RStatusScreen::TickScreen(float DeltaTime)
{
	using namespace P3RStatusScreenLocal;
	const float Now = GetScreenTime();

	// The header flips over its horizontal axis; the mode changes as it is edge-on.
	if (UDreamWidget* H = Header.Get())
	{
		const float F = (Now - FlipAt) / FlipSeconds;
		if (F >= 0.0f && F <= 1.0f)
		{
			if (F >= 0.5f && ModeIndex != PendingMode)
			{
				ModeIndex = PendingMode;
				P3RUI::SetText(Mode.Get(), FText::FromString(Modes[ModeIndex]));
			}
			P3RUI::SetScale2D(H, FVector2D(1.0, FMath::Abs(FMath::Cos(F * UE_PI))));
		}
		else if (F > 1.0f && F < 1.5f)
		{
			P3RUI::SetScale2D(H, FVector2D(1.0));
		}
	}
	// The disc turns slowly, carrying STATUS round its edge.
	if (UDreamWidget* D = Disc.Get(); D != nullptr && Now - EnterAt > 0.4f)
	{
		D->SetRenderRotation(FRotator(0.0, 0.0, FMath::Sin((Now - EnterAt) * 0.35f) * 6.0f));
	}
	// He breathes, the shard with him.
	if (UDreamWidget* G = HeroGroup.Get())
	{
		G->SetRenderRotation(FRotator(0.0, 0.0, FMath::Sin(Now * UE_TWO_PI / 6.0f) * 0.5f));
	}
}
