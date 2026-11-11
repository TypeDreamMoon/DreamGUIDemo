// Copyright DreamMoon. All Rights Reserved.

#include "NocturneLayerFxScreen.h"

#include "../Settings/NocturneUI.h"
#include "Core/Components/DreamLayerGroup.h"
#include "Core/Components/DreamVisualBatchMesh.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamUIClipMask.h"
#include "Core/DreamUserWidget.h"
#include "Extensions/Effects/DreamBackgroundBlur.h"
#include "Extensions/Effects/DreamGlass.h"

namespace NocturneLayerFxLocal
{
	constexpr int32 RowCount = 7;
	constexpr int32 SheetCount = 7;
	const FColor Ink(0xD7, 0xE2, 0xEA);
	const FColor InkDim(0x8A, 0x98, 0xA6);

	// The beats, on the screen's clock from the entrance.
	constexpr float MaskAt = 2.0f;
	constexpr float ClipAt = 2.75f;
	constexpr float BlendAt = 3.45f;
	constexpr float GroupAt = 4.25f;
	constexpr float BlurAt = 5.6f;
	constexpr float GlassAt = 6.6f;
	constexpr float PsdAt = 7.6f;
	constexpr float ApartAt = 9.3f;
	constexpr float TogetherAt = 12.8f;

	// The stack apart: turned this way, each sheet this far from the next.
	const FRotator ApartTurn(10.0, -38.0, 0.0);
	constexpr double SheetGap = 110.0;
	constexpr double ApartScale = 0.8;

	FString Fixed(float InValue, int32 InDecimals = 2)
	{
		return FString::Printf(TEXT("%.*f"), InDecimals, InValue);
	}
}

float UNocturneLayerFxScreen::Ease(float InFrom, float InSeconds) const
{
	const float T = FMath::Clamp((GetScreenTime() - StartTime - InFrom) / FMath::Max(InSeconds, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
	return 1.0f - FMath::Pow(1.0f - T, 3.0f);
}

void UNocturneLayerFxScreen::PlayContentEntrance(float Delay)
{
	using namespace NocturneLayerFxLocal;
	StartTime = GetScreenTime();
	NextBeat = 0;
	CurrentRow = -1;
	Live = 0;
	if (!bBeatsBuilt)
	{
		BuildBeats();
		bBeatsBuilt = true;
	}

	RunInChildren(Node(TEXT("LayerList")), Delay + 0.08f, 0.05f, FVector2D(-30.0, 0.0));
	RunIn(Node(TEXT("PropBody")), Delay + 0.12f, 0.3f, FVector2D(30.0, 0.0), 0.0f);
	if (UDreamWidget* Title = Node(TEXT("TitleText")))
	{
		Title->RenderOpacityTo(1.0f, 0.5f, Delay + 0.5f, EDreamTweenEase::OutQuad);
	}
	if (UDreamWidget* Sub = Node(TEXT("TitleSub")))
	{
		Sub->RenderOpacityTo(1.0f, 0.4f, Delay + 0.75f, EDreamTweenEase::OutQuad);
	}
	ShowProps(TEXT("合成"), TEXT("COMPOSITE"), { { TEXT("LAYERS"), TEXT("07") }, { TEXT("SIZE"), TEXT("950 × 540") },
		{ TEXT("RENDERER"), TEXT("DREAMUI") } });
}

void UNocturneLayerFxScreen::BuildBeats()
{
	using namespace NocturneLayerFxLocal;
	using FLine = TPair<FString, FString>;

	Beats.Add({ MaskAt, [this]()
	{
		LightRow(0);
		ShowProps(TEXT("遮罩"), TEXT("CLIP MASK"), { FLine(TEXT("SOURCE"), TEXT("PROCEDURAL")), FLine(TEXT("PATTERN"), TEXT("RADIAL")),
			FLine(TEXT("THRESHOLD"), TEXT("1.10")) });
		Live = 1;
	} });
	Beats.Add({ ClipAt, [this]()
	{
		LightRow(1);
		ShowProps(TEXT("剪贴蒙版"), TEXT("CLIP TO LAYER BELOW"), { FLine(TEXT("LAYER BELOW"), TEXT("TITLE TEXT")),
			FLine(TEXT("CLIP TO BELOW"), TEXT("ON")), FLine(TEXT("DRAWN AS"), TEXT("GROUP")) });
		Live = 0;
		if (UDreamWidget* Fill = Node(TEXT("TitleFill")))
		{
			// The light sweeps through the letters from left to right.
			Fill->SetRenderTranslation(FVector(0.0, -330.0, 0.0));
			Fill->RenderOpacityTo(1.0f, 0.3f, 0.0f, EDreamTweenEase::OutQuad);
			Fill->RenderTranslationTo(FVector(0.0, 330.0, 0.0), 2.6f, 0.0f, EDreamTweenEase::InOutSine);
		}
	} });
	Beats.Add({ BlendAt, [this]()
	{
		LightRow(2);
		ShowProps(TEXT("混合模式"), TEXT("BLEND MODE"), { FLine(TEXT("MODE"), TEXT("DIFFERENCE")), FLine(TEXT("MODES"), TEXT("27")),
			FLine(TEXT("READS BEHIND"), TEXT("YES")) });
		if (UDreamWidget* Beam = Node(TEXT("BeamA")))
		{
			Beam->RenderOpacityTo(0.75f, 0.25f, 0.0f, EDreamTweenEase::OutQuad);
		}
		if (UDreamWidget* Beam = Node(TEXT("BeamB")))
		{
			Beam->RenderOpacityTo(0.7f, 0.25f, 0.06f, EDreamTweenEase::OutQuad);
		}
	} });
	// The beams go through three modes: Difference (as they come in), Overlay, and Screen to stay -- one the blend
	// hardware draws, which holds up when the stack turns in 3D; the modes that copy what is behind do not there.
	const TPair<float, EDreamUIBlendMode> Modes[] = { { BlendAt + 0.36f, EDreamUIBlendMode::Overlay },
		{ BlendAt + 0.72f, EDreamUIBlendMode::Screen } };
	for (const TPair<float, EDreamUIBlendMode>& Mode : Modes)
	{
		const EDreamUIBlendMode Value = Mode.Value;
		Beats.Add({ Mode.Key, [this, Value]()
		{
			for (const TCHAR* Name : { TEXT("BeamA"), TEXT("BeamB") })
			{
				if (UDreamWidget* Beam = Node(Name))
				{
					if (UDreamVisualBatchMesh* Mesh = Cast<UDreamVisualBatchMesh>(Beam->GetVisual()))
					{
						Mesh->SetBlendMode(Value);
					}
				}
			}
			SetPropValue(0, Value == EDreamUIBlendMode::Overlay ? TEXT("OVERLAY") : TEXT("SCREEN"));
		} });
	}
	Beats.Add({ GroupAt, [this]()
	{
		LightRow(3);
		ShowProps(TEXT("图层组"), TEXT("LAYER GROUP"), { FLine(TEXT("ISOLATION"), TEXT("ISOLATED")),
			FLine(TEXT("GROUP OPACITY"), TEXT("0%")), FLine(TEXT("GROUP BLEND"), TEXT("NORMAL")) });
		Live = 3;
	} });
	Beats.Add({ GroupAt + 0.5f, [this]()
	{
		if (UDreamWidget* Cluster = Node(TEXT("Cluster")))
		{
			if (UDreamLayerGroup* Group = Cluster->GetComponent<UDreamLayerGroup>())
			{
				Group->SetGroupBlendMode(EDreamUIBlendMode::Screen);
			}
		}
		SetPropValue(2, TEXT("SCREEN"));
	} });
	Beats.Add({ BlurAt, [this]()
	{
		LightRow(4);
		ShowProps(TEXT("渐进模糊"), TEXT("PROGRESSIVE BLUR"), { FLine(TEXT("STRENGTH MAP"), TEXT("LINEAR")),
			FLine(TEXT("BOTTOM → TOP"), TEXT("1.00 → 0.00")), FLine(TEXT("BLUR"), TEXT("0.00")) });
		Live = 4;
		if (UDreamWidget* Caption = Node(TEXT("FrostCaption")))
		{
			Caption->RenderOpacityTo(1.0f, 0.4f, 0.35f, EDreamTweenEase::OutQuad);
		}
	} });
	Beats.Add({ GlassAt, [this]()
	{
		LightRow(5);
		ShowProps(TEXT("折射玻璃"), TEXT("GLASS"), { FLine(TEXT("REFRACTION"), TEXT("46")), FLine(TEXT("DISPERSION"), TEXT("0.35")),
			FLine(TEXT("EDGE"), TEXT("34")) });
		Live = 0;
		if (UDreamWidget* Glass = Node(TEXT("GlassSlab")))
		{
			// Across the title and onto the portrait.
			Glass->RenderTranslationTo(FVector(0.0, 1030.0, 0.0), 1.7f, 0.0f, EDreamTweenEase::InOutCubic);
		}
	} });
	Beats.Add({ PsdAt, [this]()
	{
		LightRow(6);
		ShowProps(TEXT("PSD 导入"), TEXT("LAYERED DOCUMENT"), { FLine(TEXT("FILE"), TEXT("LayerSample.psd")),
			FLine(TEXT("LAYERS"), TEXT("13")), FLine(TEXT("MASKS · CLIPS · MODES"), TEXT("KEPT")) });
		if (UDreamWidget* Tag = Node(TEXT("PsdTag")))
		{
			Tag->RenderOpacityTo(1.0f, 0.3f, 0.0f, EDreamTweenEase::OutQuad);
		}
		if (UDreamWidget* Psd = Node(TEXT("Psd")))
		{
			Psd->SetRenderOpacity(1.0f);
			// The document's layers come in one after another, bottom first.
			UDreamUserWidget* Sample = Cast<UDreamUserWidget>(Psd);
			UDreamWidget* SampleRoot = Sample != nullptr ? Sample->GetContentRoot() : nullptr;
			RunInChildren(SampleRoot != nullptr ? SampleRoot : Psd, 0.05f, 0.09f, FVector2D(0.0, -40.0));
		}
	} });
	Beats.Add({ ApartAt, [this]()
	{
		ShowProps(TEXT("分层视图"), TEXT("LAYER STACK"), { FLine(TEXT("SHEETS"), TEXT("07")), FLine(TEXT("SPACING"), TEXT("0")),
			FLine(TEXT("COMPOSITE"), TEXT("LIVE")) });
		Live = 7;
		Explode(true);
	} });
	Beats.Add({ TogetherAt, [this]()
	{
		Explode(false);
	} });
	Beats.Add({ TogetherAt + 1.1f, [this]()
	{
		Live = 0;
		ShowProps(TEXT("合成"), TEXT("COMPOSITE"), { FLine(TEXT("LAYERS"), TEXT("07")), FLine(TEXT("SIZE"), TEXT("950 × 540")),
			FLine(TEXT("RENDERER"), TEXT("DREAMUI")) });
	} });
}

void UNocturneLayerFxScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturneLayerFxLocal;
	Super::NativeOnTick(DeltaTime);
	if (!bBeatsBuilt)
	{
		return;
	}
	const float T = GetScreenTime() - StartTime;
	while (NextBeat < Beats.Num() && Beats[NextBeat].Time <= T)
	{
		Beats[NextBeat++].Run();
	}

	// What runs continuously: the mask opening, the group fading, the blur rising.
	if (T >= MaskAt && T <= MaskAt + 1.2f)
	{
		if (UDreamWidget* Portrait = Node(TEXT("Portrait")))
		{
			FDreamClipMask Mask = Portrait->GetClipMask();
			Mask.Threshold = FMath::Lerp(1.1f, -0.12f, Ease(MaskAt, 0.9f));
			Portrait->SetClipMask(Mask);
			if (Live == 1)
			{
				SetPropValue(2, Fixed(Mask.Threshold));
			}
		}
	}
	if (T >= GroupAt && T <= GroupAt + 1.4f)
	{
		if (UDreamWidget* Cluster = Node(TEXT("Cluster")))
		{
			if (UDreamLayerGroup* Group = Cluster->GetComponent<UDreamLayerGroup>())
			{
				const float Opacity = T < GroupAt + 0.6f ? Ease(GroupAt, 0.35f) : FMath::Lerp(1.0f, 0.6f, Ease(GroupAt + 0.6f, 0.5f));
				Group->SetGroupOpacity(Opacity);
				if (Live == 3)
				{
					SetPropValue(1, FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Opacity * 100.0f)));
				}
			}
		}
	}
	if (T >= BlurAt && T <= BlurAt + 1.0f)
	{
		if (UDreamWidget* Band = Node(TEXT("FrostBand")))
		{
			if (UDreamBackgroundBlur* Blur = Cast<UDreamBackgroundBlur>(Band->GetVisual()))
			{
				const float Strength = 0.8f * Ease(BlurAt, 0.8f);
				Blur->SetBlurStrength(Strength);
				if (Live == 4)
				{
					SetPropValue(2, Fixed(Strength));
				}
			}
		}
	}
	if (Live == 7)
	{
		SetPropValue(1, FString::Printf(TEXT("%d"), FMath::RoundToInt(SheetGap * Ease(ApartAt, 1.1f))));
	}
}

void UNocturneLayerFxScreen::LightRow(int32 InIndex)
{
	using namespace NocturneLayerFxLocal;
	if (CurrentRow >= 0)
	{
		if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Row%dBar"), CurrentRow)))
		{
			Bar->RenderOpacityTo(0.0f, 0.2f, 0.0f, EDreamTweenEase::OutQuad);
		}
		NocturneUI::SetTextColor(Node(*FString::Printf(TEXT("Row%dText"), CurrentRow)), InkDim);
	}
	CurrentRow = InIndex;
	if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Row%dBar"), InIndex)))
	{
		Bar->RenderOpacityTo(1.0f, 0.15f, 0.0f, EDreamTweenEase::OutQuad);
	}
	if (UDreamWidget* Eye = Node(*FString::Printf(TEXT("Row%dEye"), InIndex)))
	{
		Settle(Eye);
		Eye->SetRenderScale(FVector(1.0, 1.8, 1.8));
		Eye->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.3f, 0.0f, EDreamTweenEase::OutBack);
		Eye->RenderOpacityTo(1.0f, 0.15f, 0.0f, EDreamTweenEase::OutQuad);
	}
	NocturneUI::SetTextColor(Node(*FString::Printf(TEXT("Row%dText"), InIndex)), Ink);
	// A light sort over the panel the new layer shows on: the picture's, or the document's on the right.
	BurstSort(InIndex + 1 < SheetCount ? CenterSortIndex : RightSortIndex, InIndex + 1 < SheetCount ? 0.18f : 0.35f, 0.28f);
}

void UNocturneLayerFxScreen::ShowProps(const TCHAR* InTitle, const TCHAR* InEn, TArrayView<const TPair<FString, FString>> InLines)
{
	NocturneUI::SetText(Node(TEXT("PropTitle")), FText::FromString(InTitle));
	NocturneUI::SetText(Node(TEXT("PropEn")), FText::FromString(InEn));
	for (int32 Line = 0; Line < 3; ++Line)
	{
		const bool bHas = InLines.IsValidIndex(Line);
		UDreamWidget* Key = Node(*FString::Printf(TEXT("PropKey%d"), Line));
		UDreamWidget* Value = Node(*FString::Printf(TEXT("PropValue%d"), Line));
		NocturneUI::SetText(Key, FText::FromString(bHas ? InLines[Line].Key : FString()));
		NocturneUI::SetText(Value, FText::FromString(bHas ? InLines[Line].Value : FString()));
		// Each line runs in from the right, one after another.
		RunIn(Key, 0.03f * Line, 0.22f, FVector2D(18.0, 0.0), 0.0f);
		RunIn(Value, 0.03f * Line + 0.02f, 0.22f, FVector2D(18.0, 0.0), 0.0f);
	}
	RunIn(Node(TEXT("PropTitle")), 0.0f, 0.25f, FVector2D(18.0, 0.0), 0.0f);
	RunIn(Node(TEXT("PropEn")), 0.03f, 0.25f, FVector2D(18.0, 0.0), 0.0f);
}

void UNocturneLayerFxScreen::SetPropValue(int32 InLine, const FString& InValue)
{
	NocturneUI::SetText(Node(*FString::Printf(TEXT("PropValue%d"), InLine)), FText::FromString(InValue));
}

void UNocturneLayerFxScreen::Explode(bool bInApart)
{
	using namespace NocturneLayerFxLocal;
	const float Seconds = bInApart ? 1.1f : 1.0f;
	const EDreamTweenEase Ease = bInApart ? EDreamTweenEase::OutCubic : EDreamTweenEase::InOutCubic;
	if (UDreamWidget* Stack = Node(TEXT("Stack")))
	{
		Settle(Stack);
		RotateTo(Stack, bInApart ? ApartTurn : FRotator::ZeroRotator, Seconds, 0.0f, Ease);
		const double Scale = bInApart ? ApartScale : 1.0;
		Stack->RenderScaleTo(FVector(1.0, Scale, Scale), Seconds, 0.0f, Ease);
		if (bInApart)
		{
			// While apart, the stack keeps turning a little: the depth reads as depth.
			RotateTo(Stack, ApartTurn + FRotator(0.0, 10.0, 0.0), 2.4f, Seconds, EDreamTweenEase::InOutSine);
		}
	}
	for (int32 Index = 0; Index < SheetCount; ++Index)
	{
		UDreamWidget* Sheet = Node(*FString::Printf(TEXT("Sheet%d"), Index));
		if (Sheet == nullptr)
		{
			continue;
		}
		// The bottom sheet furthest away, the top one nearest; the middle stays where the picture was.
		const double Depth = bInApart ? (3.0 - Index) * SheetGap : 0.0;
		Settle(Sheet);
		Sheet->RenderTranslationTo(FVector(Depth, 0.0, 0.0), Seconds, 0.02f * Index, Ease);
		for (const TCHAR* Suffix : { TEXT("Edge"), TEXT("Tag") })
		{
			if (UDreamWidget* Part = Node(*FString::Printf(TEXT("Sheet%d%s"), Index, Suffix)))
			{
				Part->RenderOpacityTo(bInApart ? 1.0f : 0.0f, bInApart ? 0.4f : 0.25f, bInApart ? 0.3f + 0.04f * Index : 0.0f,
					EDreamTweenEase::OutQuad);
			}
		}
	}
}
