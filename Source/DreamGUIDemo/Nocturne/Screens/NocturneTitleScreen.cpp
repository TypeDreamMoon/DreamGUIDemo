// Copyright DreamMoon. All Rights Reserved.

#include "NocturneTitleScreen.h"

#include "NocturneLens.h"

#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Event/DreamKeyEventData.h"
#include "Interaction/UIButton.h"
#include "Math/RandomStream.h"

namespace NocturneTitleLocal
{
	constexpr int32 BootLines = 6;
	// After the entrance begins: the wordmark decodes, then the boot lines turn OK one after another, then the prompt.
	constexpr float LogoFrom = 0.25f;
	constexpr float LogoSeconds = 0.7f;
	constexpr float BootFrom = 0.7f;
	constexpr float BootStep = 0.22f;
	constexpr float PromptFrom = 2.3f;
	// How long the scan line takes down the picture, design units it runs.
	constexpr float ScanSeconds = 3.2f;
	constexpr double ScanLength = 640.0;

	const TCHAR* Glyphs = TEXT("ABCDEFGHJKLMNPQRSTUVWXYZ0123456789#/<>");

	/** Final with what is past Progress replaced by glyphs that change every other frame; seeded by the frame. */
	FString Scramble(const FString& Final, double Progress, int32 Frame)
	{
		const int32 Resolved = FMath::FloorToInt(FMath::Clamp(Progress, 0.0, 1.0) * Final.Len());
		const int32 Count = FCString::Strlen(Glyphs);
		FString Out = Final;
		for (int32 Index = Resolved; Index < Final.Len(); ++Index)
		{
			if (Final[Index] == TEXT(' '))
			{
				continue;
			}
			FRandomStream Stream(Frame * 7919 + Index * 104729);
			Out[Index] = Glyphs[Stream.RandRange(0, Count - 1)];
		}
		return Out;
	}
}

void UNocturneTitleScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (UDreamWidget* Catch = Node(TEXT("Catch")))
	{
		Catch->SetIsFocusable(true);
	}
	if (UDreamWidget* Root = GetContentRoot())
	{
		if (UUIButton* Button = Root->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(false);
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { Wake(); });
		}
	}
	BootDone.Init(false, NocturneTitleLocal::BootLines);
	// Nothing shows before the entrance.
	for (const TCHAR* Part : { TEXT("Brand"), TEXT("CardBody"), TEXT("Prompt"), TEXT("RightPlate"), TEXT("RightFrame"), TEXT("CardTab"), TEXT("KeyBar") })
	{
		if (UDreamWidget* Widget = Node(Part))
		{
			Widget->SetRenderOpacity(0.0f);
		}
	}
}

void UNocturneTitleScreen::PlayContentEntrance(float Delay)
{
	EntranceAt = GetScreenTime();
	// The title is an old tube: curved, scanlined, grainy, until the system wakes.
	if (UNocturneLens* Lens = UNocturneLens::Get())
	{
		Lens->SetTube(1.0f, 0.3f);
	}
	// The administrator's card swings round from behind on its left edge.
	SwingIn(Node(TEXT("AdminCard")), Delay + 0.25f, 0.7f, FRotator(0.0, -62.0, 0.0), 180.0);
	if (UDreamWidget* Brand = Node(TEXT("Brand")))
	{
		Brand->SetRenderOpacity(1.0f);
		RunInChildren(Brand, Delay, 0.06f, FVector2D(-20.0, 0.0));
	}
	// The boot rows wait their turn: only their statuses run in now, dim.
	for (int32 Index = 0; Index < NocturneTitleLocal::BootLines; ++Index)
	{
		if (UDreamWidget* Row = Node(*FString::Printf(TEXT("Boot%d"), Index)))
		{
			RunIn(Row, Delay + NocturneTitleLocal::BootFrom + NocturneTitleLocal::BootStep * Index - 0.1f, 0.2f, FVector2D(-10.0, 0.0));
		}
	}
	if (UDreamWidget* Card = Node(TEXT("CardBody")))
	{
		RunIn(Card, Delay + 0.35f, 0.4f, FVector2D(18.0, 0.0), 0.0f);
	}
	for (const TCHAR* Part : { TEXT("RightPlate"), TEXT("RightFrame"), TEXT("CardTab") })
	{
		if (UDreamWidget* Widget = Node(Part))
		{
			Widget->SetRenderOpacity(1.0f);
		}
	}
	if (UDreamWidget* Prompt = Node(TEXT("Prompt")))
	{
		RunIn(Prompt, Delay + NocturneTitleLocal::PromptFrom, 0.4f, FVector2D(0.0, 12.0));
	}
	BurstSort(RightSortIndex, 0.9f, 0.6f, Delay + 0.35f);
}

void UNocturneTitleScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturneTitleLocal;
	Super::NativeOnTick(DeltaTime);
	if (EntranceAt < 0.0f)
	{
		return;
	}
	const float Since = GetScreenTime() - EntranceAt;
	const int32 Frame = FMath::FloorToInt(GetScreenTime() * 30.0f);

	if (!bLogoDone)
	{
		const float Progress = (Since - LogoFrom) / LogoSeconds;
		SetNodeText(TEXT("Logo"), Progress >= 1.0f ? FString(TEXT("NOCTURNE//OS")) : Scramble(TEXT("NOCTURNE//OS"), Progress, Frame));
		bLogoDone = Progress >= 1.0f;
	}
	for (int32 Index = 0; Index < BootLines; ++Index)
	{
		if (!BootDone[Index] && Since >= BootFrom + BootStep * (Index + 1))
		{
			BootDone[Index] = true;
			const FString Name = FString::Printf(TEXT("Boot%dStatus"), Index);
			SetNodeText(*Name, TEXT("[ OK ]"));
			NocturneUI::SetTextColor(Node(*Name), NocturneUI::Cyan);
		}
	}

	// "Press any key" breathes; once woken it holds bright.
	if (UDreamWidget* Prompt = Node(TEXT("PromptCn")); Prompt != nullptr && !bAwake && Since > PromptFrom + 0.4f)
	{
		const float Breath = 0.55f + 0.45f * (0.5f + 0.5f * FMath::Cos((Since - PromptFrom - 0.4f) * 3.2f));
		Prompt->SetRenderOpacity(Breath);
	}

	// The scan line runs down the picture and starts again.
	if (UDreamWidget* Scan = Node(TEXT("ScanLine")))
	{
		const double Phase = FMath::Frac(Since / ScanSeconds);
		SetOffset(Scan, FVector2D(0.0, Phase * ScanLength));
		Scan->SetRenderOpacity(static_cast<float>(FMath::Sin(Phase * PI)));
	}

	// The uptime counts from the wake of the screen.
	const int32 Seconds = FMath::FloorToInt(FMath::Max(Since, 0.0f)) + 41;
	if (Seconds != ShownUptime)
	{
		ShownUptime = Seconds;
		SetNodeText(TEXT("CardUptime"), FString::Printf(TEXT("UPTIME  %02d:%02d:%02d"), 23, Seconds / 60 % 60, Seconds % 60));
	}
}

bool UNocturneTitleScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	// Any key at all.
	if (EventData != nullptr && !bAwake)
	{
		Wake();
		return true;
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNocturneTitleScreen::Wake()
{
	if (bAwake)
	{
		return;
	}
	bAwake = true;
	if (UDreamWidget* Prompt = Node(TEXT("PromptCn")))
	{
		Settle(Prompt);
		Prompt->SetRenderOpacity(1.0f);
	}
	NocturneUI::SetTextColor(Node(TEXT("PromptCn")), NocturneUI::Cyan);
	NocturneUI::SetTextColor(Node(TEXT("PromptEn")), NocturneUI::InkBright);
	// The tube switches off into the system.
	if (UNocturneLens* Lens = UNocturneLens::Get())
	{
		Lens->SetTube(0.0f, 0.45f);
		Lens->Fire(ENocturneLensPulse::Wake, 1.0f);
	}
	OnWake.Broadcast();
}

void UNocturneTitleScreen::PlayExit()
{
	// Not the rig falling away: the camera goes through the title. The card swings shut behind on its hinge, the logo
	// and the prompt come at the camera and are passed.
	FadeChrome();
	SwingOut(Node(TEXT("AdminCard")), 0.0f, 0.42f, FRotator(0.0, -64.0, 0.0), 320.0);
	if (UDreamWidget* Card = Node(TEXT("AdminCard")))
	{
		Card->RenderOpacityTo(0.0f, 0.2f, 0.2f, EDreamTweenEase::InQuad);
	}
	const TCHAR* Near[] = { TEXT("Brand"), TEXT("Prompt") };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(Near); ++Index)
	{
		if (UDreamWidget* Widget = Node(Near[Index]))
		{
			Settle(Widget);
			Widget->RenderTranslationTo(GetRest(Widget).Translation + FVector(-560.0, 0.0, 0.0), 0.5f, 0.04f * Index, EDreamTweenEase::InCubic);
			Widget->RenderOpacityTo(0.0f, 0.22f, 0.22f + 0.04f * Index, EDreamTweenEase::InQuad);
		}
	}
}

void UNocturneTitleScreen::SetNodeText(const TCHAR* Name, const FString& Text) const
{
	NocturneUI::SetText(Node(Name), FText::FromString(Text));
}

UDreamWidget* UNocturneTitleScreen::GetInitialFocus() const
{
	return Node(TEXT("Catch"));
}
