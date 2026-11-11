// Copyright DreamMoon. All Rights Reserved.

#include "NocturneStarScreen.h"

#include "NocturneLens.h"
#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWidgetTree.h"
#include "DreamTweenManager.h"
#include "Event/DreamKeyEventData.h"
#include "Extensions/Effects/DreamPixelSort.h"
#include "Interaction/UIButton.h"

namespace NocturneStarScreenLocal
{
	/** How far the panels lean toward the pointer, degrees, and how fast they follow (about 160 ms to settle). */
	constexpr float ParallaxYaw = 5.0f;
	constexpr float ParallaxPitch = 3.4f;
	constexpr float ParallaxRate = 6.5f;
	/** The rig's drift on its own, degrees and radians a second: slow enough to read as floating, not as motion. */
	constexpr float DriftYaw = 0.9f;
	constexpr float DriftPitch = 0.6f;

	/** How far in front of its plate each kind of decoration floats, design units: the layers the lean slides apart. */
	constexpr double FrameLift = 10.0;
	constexpr double SigilLift = 16.0;
	constexpr double TrimLift = 22.0;
	/** On the way in, frames and trims start this much nearer still and settle back onto their plates. */
	constexpr double FramePeel = 110.0;
	constexpr double TrimPeel = 70.0;

	/** A part running in flips up from this far back (degrees of pitch) and this far behind (design units). */
	constexpr double RunInPitch = 40.0;
	constexpr double RunInDepth = 60.0;

	const TCHAR* SortNames[] = { TEXT("CenterSort"), TEXT("RightSort"), TEXT("LeftSort") };

	// A frame coming on: on, off, on, off, on -- two frames of a 60 Hz picture each, about 130 ms in all. Only lines and
	// accents flicker; plates and text fade, so nothing large ever strobes.
	const float FlickerSteps[] = { 1.0f, 0.0f, 1.0f, 0.15f, 1.0f };
	constexpr float FlickerStep = 0.033f;
	// How fast the sigils turn, degrees a second.
	constexpr float SigilSpeed = 14.0f;

	double EaseOutQuad(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return 1.0 - (1.0 - X) * (1.0 - X);
	}
}

UDreamWidget* UNocturneStarScreen::Node(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

void UNocturneStarScreen::SetOffset(UDreamWidget* Widget, FVector2D Offset)
{
	if (IsValid(Widget))
	{
		Widget->SetRenderTranslation(FVector(Widget->GetRenderTranslation().X, Offset.X, -Offset.Y));
	}
}

void UNocturneStarScreen::Settle(UDreamWidget* Widget)
{
	if (IsValid(Widget))
	{
		UDreamTweenManager::KillAllTweensOnTarget(Widget, Widget);
	}
}

UDreamTweener* UNocturneStarScreen::RotateTo(UDreamWidget* InWidget, const FRotator& InTo, float InSeconds, float InDelay, EDreamTweenEase InEase)
{
	if (!IsValid(InWidget))
	{
		return nullptr;
	}
	UDreamTweener* Tweener = UDreamTweenManager::To(InWidget,
		FDreamTweenRotatorGetterFunction::CreateWeakLambda(InWidget, [InWidget]() { return InWidget->GetRenderRotation(); }),
		FDreamTweenRotatorSetterFunction::CreateWeakLambda(InWidget, [InWidget](FRotator Value) { InWidget->SetRenderRotation(Value); }),
		InTo, InSeconds);
	if (Tweener != nullptr)
	{
		Tweener->SetEase(InEase)->SetDelay(InDelay);
	}
	return Tweener;
}

UNocturneStarScreen::FRestPose UNocturneStarScreen::GetRest(UDreamWidget* InWidget)
{
	if (!IsValid(InWidget))
	{
		return FRestPose();
	}
	if (const FRestPose* Found = Rests.Find(InWidget))
	{
		return *Found;
	}
	FRestPose Pose;
	Pose.Translation = InWidget->GetRenderTranslation();
	Pose.Rotation = InWidget->GetRenderRotation();
	Rests.Add(InWidget, Pose);
	return Pose;
}

void UNocturneStarScreen::SwingIn(UDreamWidget* InWidget, float InDelay, float InSeconds, const FRotator& InFrom, double InDepth, EDreamTweenEase InEase)
{
	if (!IsValid(InWidget))
	{
		return;
	}
	const FRestPose Rest = GetRest(InWidget);
	Settle(InWidget);
	InWidget->SetRenderRotation(Rest.Rotation + InFrom);
	InWidget->SetRenderTranslation(Rest.Translation + FVector(InDepth, 0.0, 0.0));
	RotateTo(InWidget, Rest.Rotation, InSeconds, InDelay, InEase);
	InWidget->RenderTranslationTo(Rest.Translation, InSeconds, InDelay, InEase);
}

void UNocturneStarScreen::SwingOut(UDreamWidget* InWidget, float InDelay, float InSeconds, const FRotator& InTo, double InDepth, EDreamTweenEase InEase)
{
	if (!IsValid(InWidget))
	{
		return;
	}
	const FRestPose Rest = GetRest(InWidget);
	Settle(InWidget);
	RotateTo(InWidget, Rest.Rotation + InTo, InSeconds, InDelay, InEase);
	InWidget->RenderTranslationTo(Rest.Translation + FVector(InDepth, 0.0, 0.0), InSeconds, InDelay, InEase);
}

void UNocturneStarScreen::Collect(UDreamWidget* InWidget, TArray<TWeakObjectPtr<UDreamWidget>>& OutTrims,
	TArray<TWeakObjectPtr<UDreamWidget>>& OutSigils, TArray<TWeakObjectPtr<UDreamWidget>>& OutFrames) const
{
	if (!IsValid(InWidget))
	{
		return;
	}
	const FString& Name = InWidget->GetDisplayName();
	if (Name.StartsWith(TEXT("Trim")))
	{
		OutTrims.Add(InWidget);
	}
	else if (Name.StartsWith(TEXT("Sigil")))
	{
		OutSigils.Add(InWidget);
	}
	else if (Name.EndsWith(TEXT("Frame")))
	{
		OutFrames.Add(InWidget);
	}
	// A component screen (an option row, a slot) keeps its own parts.
	if (InWidget != GetContentRoot() && Cast<UDreamUserWidget>(InWidget) != nullptr)
	{
		return;
	}
	for (UDreamWidget* Child : InWidget->GetChildren())
	{
		Collect(Child, OutTrims, OutSigils, OutFrames);
	}
}

void UNocturneStarScreen::NativeOnInitialized()
{
	using namespace NocturneStarScreenLocal;
	Super::NativeOnInitialized();
	Collect(GetContentRoot(), Trims, Sigils, Frames);

	// The decorations float in front of their plates, each kind a little further: the layers the lean slides apart.
	auto Lift = [this](const TArray<TWeakObjectPtr<UDreamWidget>>& InParts, double InLift)
	{
		for (const TWeakObjectPtr<UDreamWidget>& Part : InParts)
		{
			if (UDreamWidget* Live = Part.Get())
			{
				Live->SetRenderTranslation(Live->GetRenderTranslation() - FVector(InLift, 0.0, 0.0));
				GetRest(Live);
			}
		}
	};
	Lift(Frames, FrameLift);
	Lift(Sigils, SigilLift);
	Lift(Trims, TrimLift);
	// Where the panels rest, before anything moves them.
	for (const TCHAR* Panel : { TEXT("LeftPanel"), TEXT("CenterPanel"), TEXT("RightPanel"), TEXT("Rig") })
	{
		GetRest(Node(Panel));
	}

	for (int32 Which = 0; Which < SortCount; ++Which)
	{
		const UDreamWidget* Holder = Node(SortNames[Which]);
		Sorts[Which] = Holder != nullptr ? Cast<UDreamPixelSort>(Holder->GetVisual()) : nullptr;
	}
	if (UDreamWidget* Back = Node(TEXT("HintBack")))
	{
		if (UUIButton* Button = Back->GetComponent<UUIButton>())
		{
			Button->SetUseFocusRing(false);
			// A pointer thing only: Tab never stops on it, Esc is its key.
			Back->SetIsFocusable(false);
			Button->GetOnClickEvent().AddWeakLambda(this, [this]() { RequestBack(); });
			Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this](EUISelectableSelectionState InState, bool)
			{
				PaintBackHint(InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed);
			});
		}
	}
	SetWantsTick(true);
}

void UNocturneStarScreen::PaintBackHint(bool bInOver)
{
	NocturneUI::SetTint(Node(TEXT("HintBackHit")), bInOver ? NocturneUI::RowLit : NocturneUI::Clear);
	NocturneUI::SetTextColor(Node(TEXT("HintBackLabel")), bInOver ? NocturneUI::InkBright : NocturneUI::InkDim);
}

void UNocturneStarScreen::Shake(float InStrength)
{
	ShakeAt = ScreenTime;
	ShakeStrength = InStrength;
}

void UNocturneStarScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturneStarScreenLocal;
	Super::NativeOnTick(DeltaTime);
	ScreenTime += DeltaTime;

	// The sigils turn, every other one the other way.
	for (int32 Index = 0; Index < Sigils.Num(); ++Index)
	{
		if (UDreamWidget* Sigil = Sigils[Index].Get())
		{
			Sigil->SetRenderTransformAngle(ScreenTime * SigilSpeed * (Index % 2 == 0 ? 1.0f : -1.0f));
		}
	}
	// The frames flicker on, each up to the opacity it had when the entrance began.
	if (FlickerFrom >= 0.0f)
	{
		const int32 Step = FMath::FloorToInt((ScreenTime - FlickerFrom) / FlickerStep);
		const int32 StepCount = UE_ARRAY_COUNT(FlickerSteps);
		const float Level = Step < 0 ? 0.0f : (Step >= StepCount ? 1.0f : FlickerSteps[Step]);
		for (int32 Index = 0; Index < Frames.Num(); ++Index)
		{
			if (UDreamWidget* Live = Frames[Index].Get())
			{
				Live->SetRenderOpacity(Level * (FrameOpacity.IsValidIndex(Index) ? FrameOpacity[Index] : 1.0f));
			}
		}
		if (Step >= StepCount)
		{
			FlickerFrom = -1.0f;
		}
	}
	if (UDreamWidget* Focus = PendingFocus.Get(); Focus != nullptr && Focus->SetKeyboardFocus())
	{
		PendingFocus.Reset();
	}

	// The rig leans toward the pointer, drifts a little on its own, and jolts when something hits.
	Parallax += (ParallaxTarget - Parallax) * (1.0f - FMath::Exp(-DeltaTime * ParallaxRate));
	if (UDreamWidget* Rig = Node(TEXT("Rig")))
	{
		FRotator Lean(-Parallax.Y * ParallaxPitch, Parallax.X * ParallaxYaw, 0.0);
		Lean += FRotator(DriftPitch * FMath::Sin(ScreenTime * 0.31f + 1.3f), DriftYaw * FMath::Sin(ScreenTime * 0.43f), 0.0);
		const float SinceShake = ScreenTime - ShakeAt;
		if (SinceShake >= 0.0f && SinceShake < 0.6f)
		{
			const float Decay = FMath::Exp(-SinceShake * 9.0f) * ShakeStrength;
			Lean += FRotator(2.6f * Decay * FMath::Sin(SinceShake * 61.0f), 2.2f * Decay * FMath::Sin(SinceShake * 47.0f + 1.0f),
				1.4f * Decay * FMath::Sin(SinceShake * 53.0f + 2.0f));
		}
		Rig->SetRenderRotation(Lean);
	}

	for (int32 Which = 0; Which < SortCount; ++Which)
	{
		UDreamPixelSort* Sort = Sorts[Which].Get();
		if (Sort == nullptr)
		{
			continue;
		}
		const float Since = ScreenTime - SortStart[Which];
		float Strength = 0.0f;
		if (Since >= 0.0f && Since < SortSeconds[Which])
		{
			Strength = SortPeak[Which] * static_cast<float>(1.0 - EaseOutQuad(Since / SortSeconds[Which]));
		}
		// Strength 0 is the effect switched off, so a sliver is switched off rather than drawn.
		Sort->SetSortStrength(Strength < 0.02f ? 0.0f : Strength);
	}
}

bool UNocturneStarScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData != nullptr)
	{
		const FKey& Key = EventData->Key;
		if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right)
		{
			RequestBack();
			return true;
		}
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNocturneStarScreen::RequestBack()
{
	if (CanGoBack())
	{
		OnBackRequested.Broadcast();
	}
}

void UNocturneStarScreen::BurstSort(int32 InWhich, float InPeak, float InSeconds, float InDelay)
{
	if (InWhich < 0 || InWhich >= SortCount)
	{
		return;
	}
	SortPeak[InWhich] = InPeak;
	SortStart[InWhich] = ScreenTime + InDelay;
	SortSeconds[InWhich] = InSeconds;
}

void UNocturneStarScreen::RunIn(UDreamWidget* InWidget, float InDelay, float InSeconds, FVector2D InOffset, float InFlip)
{
	using namespace NocturneStarScreenLocal;
	if (!IsValid(InWidget))
	{
		return;
	}
	// The fade is short and the move a little longer and a short way: settled, never bouncing. The part flips up from
	// lying back a little way behind where it rests.
	const FRestPose Rest = GetRest(InWidget);
	const float Move = FMath::Max(InSeconds, 0.3f);
	Settle(InWidget);
	InWidget->SetRenderTranslation(Rest.Translation + FVector(RunInDepth * InFlip, InOffset.X * 0.5, -InOffset.Y * 0.5));
	InWidget->SetRenderOpacity(0.0f);
	InWidget->RenderTranslationTo(Rest.Translation, Move, InDelay, EDreamTweenEase::OutCubic);
	if (InFlip > 0.0f)
	{
		InWidget->SetRenderRotation(Rest.Rotation + FRotator(-RunInPitch * InFlip, 0.0, 0.0));
		RotateTo(InWidget, Rest.Rotation, Move + 0.06f, InDelay, EDreamTweenEase::OutCubic);
	}
	InWidget->RenderOpacityTo(1.0f, FMath::Min(InSeconds, 0.2f), InDelay, EDreamTweenEase::OutQuad);
}

void UNocturneStarScreen::RunInChildren(UDreamWidget* InParent, float InDelay, float InStagger, FVector2D InOffset, float InFlip)
{
	if (!IsValid(InParent))
	{
		return;
	}
	int32 Index = 0;
	for (UDreamWidget* Child : InParent->GetChildren())
	{
		if (!IsValid(Child) || Child->GetVisibility() == EDreamWidgetVisibility::Collapsed)
		{
			continue;
		}
		RunIn(Child, InDelay + InStagger * Index++, 0.28f, InOffset, InFlip);
	}
}

void UNocturneStarScreen::PlayEntrance()
{
	using namespace NocturneStarScreenLocal;
	if (UDreamWidget* Rig = Node(TEXT("Rig")))
	{
		Settle(Rig);
		Rig->SetRenderTranslation(GetRest(Rig).Translation);
		Rig->SetRenderScale(FVector::OneVector);
		Rig->SetRenderOpacity(1.0f);
	}
	// The panels open: the side ones swing out from behind on their inner edges, the middle one rises from lying back.
	SwingIn(Node(TEXT("CenterPanel")), 0.0f, 0.5f, FRotator(-26.0, 0.0, 0.0), 320.0);
	SwingIn(Node(TEXT("LeftPanel")), 0.03f, 0.55f, FRotator(0.0, 58.0, 0.0), 140.0);
	SwingIn(Node(TEXT("RightPanel")), 0.06f, 0.55f, FRotator(0.0, -58.0, 0.0), 140.0);

	// Frames draw out from their middles, a little in front of the plates, and settle back onto them.
	for (const TCHAR* Frame : { TEXT("LeftFrame"), TEXT("CenterFrame"), TEXT("RightFrame") })
	{
		if (UDreamWidget* Widget = Node(Frame))
		{
			Settle(Widget);
			Widget->SetRenderScale(FVector(1.0, 0.0, 1.0));
			Widget->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.25f, 0.0f, EDreamTweenEase::OutCubic);
		}
	}
	// The frames flicker on as they draw; plates and the scrim over the map fade up behind them.
	FrameOpacity.SetNum(Frames.Num());
	for (int32 Index = 0; Index < Frames.Num(); ++Index)
	{
		UDreamWidget* Widget = Frames[Index].Get();
		if (Widget == nullptr)
		{
			continue;
		}
		FrameOpacity[Index] = FlickerFrom >= 0.0f ? FrameOpacity[Index] : Widget->GetRenderOpacity();
		Widget->SetRenderOpacity(0.0f);
		const FRestPose Rest = GetRest(Widget);
		Widget->SetRenderTranslation(Rest.Translation - FVector(FramePeel, 0.0, 0.0));
		Widget->RenderTranslationTo(Rest.Translation, 0.45f, 0.05f, EDreamTweenEase::OutCubic);
	}
	FlickerFrom = ScreenTime + 0.04f;
	for (const TCHAR* Plate : { TEXT("LeftPlate"), TEXT("CenterPlate"), TEXT("RightPlate"), TEXT("Dim") })
	{
		if (UDreamWidget* Widget = Node(Plate))
		{
			Settle(Widget);
			Widget->SetRenderOpacity(0.0f);
			Widget->RenderOpacityTo(1.0f, 0.2f, 0.08f, EDreamTweenEase::OutCubic);
		}
	}
	// The corner diamonds drop onto the corners from nearer still once the frames are drawn, one corner after another.
	for (int32 Index = 0; Index < Trims.Num(); ++Index)
	{
		if (UDreamWidget* Trim = Trims[Index].Get())
		{
			const float Delay = 0.18f + 0.025f * Index;
			const FRestPose Rest = GetRest(Trim);
			Settle(Trim);
			Trim->SetRenderScale(FVector(1.0, 0.0, 0.0));
			Trim->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.32f, Delay, EDreamTweenEase::OutBack);
			Trim->SetRenderTranslation(Rest.Translation - FVector(TrimPeel, 0.0, 0.0));
			Trim->RenderTranslationTo(Rest.Translation, 0.36f, Delay, EDreamTweenEase::OutCubic);
		}
	}
	if (UDreamWidget* Bar = Node(TEXT("KeyBar")))
	{
		RunIn(Bar, 0.3f, 0.3f, FVector2D(0.0, 30.0), 0.0f);
	}
	PlayContentEntrance(0.22f);
	PendingFocus = GetInitialFocus();
	// The sorts come up once there is text to sort; the lens breaks up and resolves as the screen arrives.
	BurstSort(CenterSortIndex, 0.8f, 0.45f, 0.32f);
	BurstSort(RightSortIndex, 0.6f, 0.4f, 0.36f);
	BurstSort(LeftSortIndex, 0.6f, 0.4f, 0.3f);
	UNocturneLens::Pulse(ENocturneLensPulse::Transition, GetEntrancePulse());
}

void UNocturneStarScreen::PlayExit()
{
	// Folded shut and fallen away: the side panels swing back on their hinges, the middle one tips back, and the whole
	// rig drops into the distance as it fades.
	FoldPanels();
	RecedeRig();
	FadeChrome();
	UNocturneLens::Pulse(ENocturneLensPulse::Transition, 0.6f);
}

void UNocturneStarScreen::FoldPanels()
{
	SwingOut(Node(TEXT("LeftPanel")), 0.0f, 0.26f, FRotator(0.0, 52.0, 0.0), 120.0);
	SwingOut(Node(TEXT("RightPanel")), 0.0f, 0.26f, FRotator(0.0, -52.0, 0.0), 120.0);
	SwingOut(Node(TEXT("CenterPanel")), 0.0f, 0.26f, FRotator(-22.0, 0.0, 0.0), 160.0);
}

void UNocturneStarScreen::RecedeRig()
{
	if (UDreamWidget* Rig = Node(TEXT("Rig")))
	{
		Settle(Rig);
		Rig->RenderTranslationTo(GetRest(Rig).Translation + FVector(420.0, 0.0, 0.0), 0.3f, 0.0f, EDreamTweenEase::InCubic);
		Rig->RenderOpacityTo(0.0f, 0.14f, 0.16f, EDreamTweenEase::InCubic);
	}
}

void UNocturneStarScreen::FadeChrome()
{
	if (UDreamWidget* Bar = Node(TEXT("KeyBar")))
	{
		Settle(Bar);
		Bar->RenderOpacityTo(0.0f, 0.2f, 0.0f, EDreamTweenEase::InCubic);
	}
	if (UDreamWidget* Dim = Node(TEXT("Dim")))
	{
		Settle(Dim);
		Dim->RenderOpacityTo(0.0f, 0.35f, 0.05f, EDreamTweenEase::InCubic);
	}
	for (int32 Which = 0; Which < SortCount; ++Which)
	{
		BurstSort(Which, 1.0f, 0.35f);
	}
}
