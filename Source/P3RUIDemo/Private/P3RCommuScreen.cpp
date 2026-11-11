// Copyright DreamMoon. All Rights Reserved.

#include "P3RCommuScreen.h"

#include "P3RCommuRow.h"
#include "P3RUI.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamWidget.h"
#include "Engine/Texture2D.h"
#include "Extensions/DreamMaskTransition.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace P3RCommuScreenLocal
{
	struct FBond
	{
		const TCHAR* Numeral;
		const TCHAR* Arcana;
		int32 Rank;   // 10 = MAX
		const TCHAR* Partner;
	};
	const FBond Bonds[UP3RCommuScreen::Count] = {
		{ TEXT("0"), TEXT("愚者"), 5, TEXT("夜巡小队") },
		{ TEXT("I"), TEXT("魔术师"), 6, TEXT("邻座的同学") },
		{ TEXT("IV"), TEXT("皇帝"), 10, TEXT("学生会") },
		{ TEXT("V"), TEXT("教皇"), 10, TEXT("巷口的钟表匠") },
		{ TEXT("VII"), TEXT("战车"), 6, TEXT("游泳部") },
		{ TEXT("IX"), TEXT("隐者"), 2, TEXT("深夜电台") },
	};
	constexpr float CardFlipSeconds = 0.32f;
	/** The chosen card pushes out this far to the left, and grows a little. */
	constexpr double PushOut = -54.0;
	constexpr float ChosenScale = 1.06f;
}

void UP3RCommuScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace P3RCommuScreenLocal;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		UP3RCommuRow* Row = Cast<UP3RCommuRow>(Node(FString::Printf(TEXT("Row%d"), Index)));
		Rows[Index] = Row;
		if (Row != nullptr)
		{
			Row->SetData(Bonds[Index].Numeral, Bonds[Index].Arcana, Bonds[Index].Rank, Bonds[Index].Partner);
			Row->SetRenderTransformPivot(FVector2D(0.0, 0.5));
			WireButton(Row->GetContentRoot(), [this, Index]() { Select(Index); }, [this, Index]()
			{
				Select(Index);
				Glint();
			});
		}
	}
	if (UDreamWidget* Content = GetContentRoot())
	{
		Blot = UDreamMaskTransition::FindOrAdd(Content);
	}
	Giant = Node(TEXT("Giant"));
	Hero = Node(TEXT("Hero"));
	Glinter = Node(TEXT("CardGlint"));
	Card = Node(TEXT("Card"));
	CardFace = Node(TEXT("CardFace"));
	CardSheen = Node(TEXT("CardSheen"));
	HeroFront = Node(TEXT("HeroFront"));
	if (UDreamWidget* C = Card.Get())
	{
		CardRoll = C->GetRenderRotation().Roll;
	}
	for (int32 Index = 0; Index < Count; ++Index)
	{
		Faces[Index] = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/P3R/Textures/T_P3R_Tarot_%d.T_P3R_Tarot_%d"), Index, Index));
	}
	Back = LoadObject<UTexture2D>(nullptr, TEXT("/Game/P3R/Textures/T_P3R_TarotBack.T_P3R_TarotBack"));
	// Over the white wall and his own shadow: solid (the figure material's see-through would show them through his blazer).
	for (UDreamWidget* Figure : { Hero.Get(), HeroFront.Get() })
	{
		if (UMaterialInstanceDynamic* Material = P3RUI::MakeFigure(Figure, this))
		{
			Material->SetScalarParameterValue(TEXT("SeeThrough"), 0.0f);
		}
	}
	HeroShadow = Node(TEXT("HeroShadow"));
	if (UMaterialInstanceDynamic* Shadow = P3RUI::MakeFigure(HeroShadow.Get(), this, 1.0f))
	{
		// A cast shadow: one flat, solid grey (the material's silhouette colour is written in sRGB).
		Shadow->SetVectorParameterValue(TEXT("SilhouetteColour"), FLinearColor(0.63f, 0.645f, 0.68f, 1.0f));
		Shadow->SetScalarParameterValue(TEXT("SilhouetteShade"), 0.0f);
		Shadow->SetScalarParameterValue(TEXT("SeeThrough"), 0.0f);
	}
	Flag = Node(TEXT("Flag"));
	List = Node(TEXT("List"));
	Select(0, /*bInstant*/true);
}

void UP3RCommuScreen::Select(int32 Index, bool bInstant)
{
	using namespace P3RCommuScreenLocal;
	Index = (Index + Count) % Count;
	if (Index == Selected && !bInstant)
	{
		return;
	}
	Selected = Index;
	for (int32 I = 0; I < Count; ++I)
	{
		UP3RCommuRow* Row = Rows[I].Get();
		if (Row == nullptr)
		{
			continue;
		}
		const bool bChosen = I == Selected;
		const FVector2D Offset(bChosen ? PushOut : 0.0, 0.0);
		const FVector2D Scale(bChosen ? ChosenScale : 1.0f);
		if (bInstant)
		{
			P3RUI::SetOffset(Row, Offset);
			P3RUI::SetScale2D(Row, Scale);
		}
		else
		{
			P3RUI::Settle(Row);
			P3RUI::OffsetTo(Row, Offset, 0.12f, 0.0f, EDreamTweenEase::OutCubic);
			P3RUI::Scale2DTo(Row, Scale, 0.12f, 0.0f, bChosen ? EDreamTweenEase::OutBack : EDreamTweenEase::OutCubic);
			Row->SetRenderOpacity(1.0f);
		}
		// After the settling above: it would kill the wipe this starts.
		Row->SetChosen(bChosen, bInstant);
	}
	TurnCard(Selected, bInstant);
}

void UP3RCommuScreen::TurnCard(int32 Index, bool bInstant)
{
	using namespace P3RCommuScreenLocal;
	CardPending = Index;
	if (bInstant)
	{
		CardShows = Index;
		ShowFace(Index);
		P3RUI::SetText(Node(TEXT("CardName")), FText::FromString(Bonds[Index].Arcana));
		CardFlipAt = -100.0f;
		bCardLanded = true;
		return;
	}
	CardFlipAt = GetScreenTime();
	bCardLanded = false;
}

void UP3RCommuScreen::ShowFace(int32 Index)
{
	UTexture2D* Texture = Index == INDEX_NONE ? Back.Get() : Faces[FMath::Clamp(Index, 0, Count - 1)].Get();
	UDreamWidget* FaceWidget = CardFace.Get();
	if (UDreamImage* Image = FaceWidget != nullptr ? Cast<UDreamImage>(FaceWidget->GetVisual()) : nullptr; Image != nullptr && Texture != nullptr)
	{
		FDreamUIImageBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(Texture);
		Image->SetBrush(Brush);
	}
	P3RUI::SetShown(Node(TEXT("CardName")), Index != INDEX_NONE);
}

void UP3RCommuScreen::Sheen()
{
	if (UDreamWidget* S = CardSheen.Get())
	{
		P3RUI::Settle(S);
		S->SetRenderOpacity(0.85f);
		P3RUI::SetOffset(S, FVector2D(-60.0, 0.0));
		P3RUI::OffsetTo(S, FVector2D(170.0, 0.0), 0.36f, 0.0f, EDreamTweenEase::OutCubic);
		P3RUI::FadeTo(S, 0.0f, 0.18f, 0.2f);
	}
}

void UP3RCommuScreen::Glint()
{
	if (UDreamWidget* G = Glinter.Get())
	{
		P3RUI::Settle(G);
		G->SetRenderOpacity(0.35f);
		P3RUI::SetScale2D(G, FVector2D(0.2, 1.0));
		P3RUI::Scale2DTo(G, FVector2D(1.0, 1.0), 0.12f);
		P3RUI::FadeTo(G, 0.0f, 0.3f, 0.06f);
	}
}

bool UP3RCommuScreen::HandleKey(EP3RKey Key)
{
	switch (Key)
	{
	case EP3RKey::Up: Select(Selected - 1); return true;
	case EP3RKey::Down: Select(Selected + 1); return true;
	case EP3RKey::Confirm: Glint(); return true;
	case EP3RKey::Back: OnBack.Broadcast(); return true;
	default: return false;
	}
}

void UP3RCommuScreen::PlayEntrance()
{
	using namespace P3RCommuScreenLocal;
	EnterAt = GetScreenTime();
	PlayBlotIn(Blot.Get());
	// The cards slide in from the left, top first.
	for (int32 I = 0; I < Count; ++I)
	{
		if (UP3RCommuRow* Row = Rows[I].Get())
		{
			P3RUI::Settle(Row);
			const double Out = I == Selected ? PushOut : 0.0;
			P3RUI::SetOffset(Row, FVector2D(-700.0, 0.0));
			Row->SetRenderOpacity(0.0f);
			const float Delay = 0.1f + I * 0.035f;
			P3RUI::OffsetTo(Row, FVector2D(Out, 0.0), 0.32f, Delay, EDreamTweenEase::OutCubic);
			P3RUI::FadeTo(Row, 1.0f, 0.2f, Delay);
		}
	}
	for (UDreamWidget* Top : { Flag.Get(), List.Get() })
	{
		if (Top != nullptr)
		{
			P3RUI::Settle(Top);
			P3RUI::SetOffset(Top, FVector2D(0.0, -220.0));
			P3RUI::OffsetTo(Top, FVector2D::ZeroVector, 0.35f, 0.04f, EDreamTweenEase::OutBack);
		}
	}
	if (UDreamWidget* G = Giant.Get())
	{
		P3RUI::Settle(G);
		P3RUI::SetOffset(G, FVector2D(260.0, 380.0));
		P3RUI::OffsetTo(G, FVector2D::ZeroVector, 0.45f, 0.0f, EDreamTweenEase::OutCubic);
	}
	// The card comes up showing its back and turns over to the chosen bond.
	CardShows = INDEX_NONE;
	ShowFace(INDEX_NONE);
	CardPending = Selected;
	CardFlipAt = EnterAt + 0.35f;
	bCardLanded = false;
	for (UDreamWidget* H : { Hero.Get(), HeroShadow.Get(), HeroFront.Get() })
	{
		if (H != nullptr)
		{
			P3RUI::Settle(H);
			H->SetRenderOpacity(0.0f);
			P3RUI::FadeTo(H, 1.0f, 0.3f, 0.08f);
		}
	}
	LockInput(0.3f);
}

float UP3RCommuScreen::PlayExit()
{
	return PlayBlotOut(Blot.Get());
}

void UP3RCommuScreen::TickScreen(float DeltaTime)
{
	using namespace P3RCommuScreenLocal;
	// The held card turns over in depth about its upright: edge-on half way, where its face changes, then round from the
	// other edge; a glint and a sheen as it lands. At rest it tilts a little with his breathing.
	if (UDreamWidget* C = Card.Get())
	{
		const float Now = GetScreenTime();
		const float F = (Now - CardFlipAt) / CardFlipSeconds;
		float Yaw = 0.0f;
		if (F >= 0.0f && F < 1.0f)
		{
			if (F >= 0.5f && CardShows != CardPending)
			{
				CardShows = CardPending;
				ShowFace(CardShows);
				P3RUI::SetText(Node(TEXT("CardName")), FText::FromString(Bonds[CardShows].Arcana));
			}
			// Fastest edge-on, slowing as it faces out again (ease in and out).
			const float K = F < 0.5f ? 2.0f * F * F : 1.0f - FMath::Square(2.0f - 2.0f * F) * 0.5f;
			Yaw = K < 0.5f ? K * 180.0f : (K - 1.0f) * 180.0f;
		}
		else if (F >= 1.0f && !bCardLanded)
		{
			bCardLanded = true;
			Glint();
			Sheen();
		}
		Yaw += FMath::Sin((Now - EnterAt) * UE_TWO_PI / 5.0f) * 4.0f;
		C->SetRenderRotation(FRotator(0.0, Yaw, CardRoll));
	}

	// The protagonist breathes in the water.
	if (UDreamWidget* H = Hero.Get())
	{
		const float E = GetScreenTime() - EnterAt;
		const float Rise = 1.0f - P3RUI::EaseOutCubic(E / 0.5f);
		const FVector2D Drift(0.0, 120.0f * Rise + FMath::Sin(E * UE_TWO_PI / 4.5f) * 6.0f);
		P3RUI::SetOffset(H, Drift);
		P3RUI::SetOffset(Card.Get(), Drift);
		P3RUI::SetOffset(Glinter.Get(), Drift);
		P3RUI::SetOffset(HeroFront.Get(), Drift);
		// The shadow on the wall moves less: the wall is further back.
		P3RUI::SetOffset(HeroShadow.Get(), Drift * 0.6);
	}
}
