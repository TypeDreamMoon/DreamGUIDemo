#include "DetroitWidgets.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamRectBlock.h"
#include "Core/Components/DreamText.h"
#include "Core/Components/DreamVisual.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "DetroitUI.h"
#include "DreamTweenBPLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace DetroitWidgetsLocal
{
	FText Invariant(const FString& String)
	{
		return FText::AsCultureInvariant(String);
	}

	void CollectUnder(UDreamWidget* InWidget, TArray<UDetroitObjectiveRow*>& OutRows)
	{
		if (!IsValid(InWidget))
		{
			return;
		}
		if (UDetroitObjectiveRow* Row = Cast<UDetroitObjectiveRow>(InWidget))
		{
			OutRows.Add(Row);
			return;
		}
		for (UDreamWidget* Child : InWidget->GetChildren())
		{
			CollectUnder(Child, OutRows);
		}
	}

	/** Width of the HUD's clue bar, in design pixels. */
	constexpr float CounterTrackWidth = 520.0f;
}

// ---------------------------------------------------------------------------------------------------------------------

void FDetroitMotion::SetOffset(UDreamWidget* Widget, FVector2D Offset)
{
	if (IsValid(Widget))
	{
		Widget->SetRenderTranslation(FVector(Widget->GetRenderTranslation().X, Offset.X, -Offset.Y));
	}
}

void FDetroitMotion::SetPlaneScale(UDreamWidget* Widget, FVector2D Scale)
{
	if (IsValid(Widget))
	{
		Widget->SetRenderScale(FVector(1.0, Scale.X, Scale.Y));
	}
}

void FDetroitMotion::SlideIn(UDreamWidget* Widget, FVector2D Offset, float Duration, float Delay, EDreamTweenEase Ease)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	SetOffset(Widget, Offset);
	Widget->SetRenderOpacity(0.0f);
	Track(Widget, Widget->RenderOffsetTo(FVector2D::ZeroVector, Duration, Delay, Ease));
	Track(Widget, Widget->RenderOpacityTo(1.0f, Duration, Delay, EDreamTweenEase::OutCubic));
}

void FDetroitMotion::FadeTo(UDreamWidget* Widget, float Opacity, float Duration, float Delay)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	Track(Widget, Widget->RenderOpacityTo(Opacity, Duration, Delay, EDreamTweenEase::OutCubic));
}

void FDetroitMotion::Grow(UDreamWidget* Widget, bool bVertical, float Duration, float Delay, EDreamTweenEase Ease)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	SetPlaneScale(Widget, bVertical ? FVector2D(1.0, 0.0) : FVector2D(0.0, 1.0));
	Widget->SetRenderOpacity(1.0f);
	Track(Widget, Widget->RenderScaleTo(FVector::OneVector, Duration, Delay, Ease));
}

void FDetroitMotion::Pop(UDreamWidget* Widget, float FromScale, float Duration, float Delay)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	SetPlaneScale(Widget, FVector2D(FromScale, FromScale));
	Widget->SetRenderOpacity(0.0f);
	Track(Widget, Widget->RenderScaleTo(FVector::OneVector, Duration, Delay, EDreamTweenEase::OutBack));
	Track(Widget, Widget->RenderOpacityTo(1.0f, Duration * 0.6f, Delay, EDreamTweenEase::OutCubic));
}

void FDetroitMotion::ScaleTo(UDreamWidget* Widget, float Scale, float Duration, float Delay, EDreamTweenEase Ease)
{
	if (IsValid(Widget))
	{
		Track(Widget, Widget->RenderScaleTo(FVector(1.0, Scale, Scale), Duration, Delay, Ease));
	}
}

void FDetroitMotion::OffsetTo(UDreamWidget* Widget, FVector2D Offset, float Duration, float Delay, EDreamTweenEase Ease)
{
	if (IsValid(Widget))
	{
		Track(Widget, Widget->RenderOffsetTo(FVector2D(Offset.X, -Offset.Y), Duration, Delay, Ease));
	}
}

void FDetroitMotion::Stop(UDreamWidget* Widget)
{
	if (TArray<TWeakObjectPtr<UDreamTweener>>* Tweens = Running.Find(Widget))
	{
		for (const TWeakObjectPtr<UDreamTweener>& Tweener : *Tweens)
		{
			if (UDreamTweener* Live = Tweener.Get())
			{
				UDreamTweenBPLibrary::KillIfIsTweening(Widget, Live);
			}
		}
		Tweens->Reset();
	}
}

void FDetroitMotion::Settle(UDreamWidget* Widget, float Opacity)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	Widget->SetRenderTranslation(FVector::ZeroVector);
	Widget->SetRenderScale(FVector::OneVector);
	Widget->SetRenderOpacity(Opacity);
}

void FDetroitMotion::Track(UDreamWidget* Widget, UDreamTweener* Tweener)
{
	if (Tweener != nullptr)
	{
		Running.FindOrAdd(Widget).Add(Tweener);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

UDreamWidget* UDetroitWidget::Node(FName Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	UDreamWidget* Found = Tree != nullptr ? Tree->FindWidgetByVariableName(Name) : nullptr;
	if (Found == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Detroit UI: %s has no node %s"), *GetClass()->GetName(), *Name.ToString());
	}
	return Found;
}

void UDetroitWidget::SetText(UDreamWidget* Widget, const FText& Text)
{
	if (UDreamText* TextVisual = IsValid(Widget) ? Cast<UDreamText>(Widget->GetVisual()) : nullptr)
	{
		TextVisual->SetText(Text);
	}
}

void UDetroitWidget::SetShown(UDreamWidget* Widget, bool bShown)
{
	if (IsValid(Widget))
	{
		Widget->SetVisibility(bShown ? EDreamWidgetVisibility::Visible : EDreamWidgetVisibility::Collapsed);
	}
}

void UDetroitWidget::SetTint(UDreamWidget* Widget, const FColor& Color)
{
	if (!IsValid(Widget))
	{
		return;
	}
	if (UDreamRectBlock* Rect = Cast<UDreamRectBlock>(Widget->GetVisual()))
	{
		Rect->SetBodyColor(Color);
	}
	else if (UDreamVisual* Visual = Widget->GetVisual())
	{
		Visual->SetColor(Color);
	}
}

UMaterialInstanceDynamic* UDetroitWidget::MakeDynamicMaterial(UDreamWidget* ImageWidget)
{
	UDreamImage* Image = IsValid(ImageWidget) ? Cast<UDreamImage>(ImageWidget->GetVisual()) : nullptr;
	UMaterialInterface* Source = Image != nullptr ? Cast<UMaterialInterface>(Image->GetBrush().GetResourceObject()) : nullptr;
	if (Source == nullptr)
	{
		return nullptr;
	}
	if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Source))
	{
		return Existing;
	}
	UMaterialInstanceDynamic* Dynamic = UMaterialInstanceDynamic::Create(Source, this);
	Image->SetBrushFromMaterial(Dynamic);
	return Dynamic;
}

void UDetroitWidget::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	Clock += DeltaTime;
}

// ---------------------------------------------------------------------------------------------------------------------

namespace DetroitLoadingLocal
{
	/** 0 before Start, 1 from End, eased out between. */
	float Ramp(float T, float Start, float End)
	{
		const float X = FMath::Clamp((T - Start) / FMath::Max(End - Start, 1e-4f), 0.0f, 1.0f);
		return 1.0f - FMath::Square(1.0f - X);
	}

	bool Within(float T, float Start, float End)
	{
		return T >= Start && T < End;
	}

	/** How many characters of a text typed from Start, one every PerChar seconds. */
	int32 Typed(float T, float Start, float PerChar)
	{
		return T < Start ? 0 : FMath::FloorToInt((T - Start) / PerChar) + 1;
	}

	/** 1 between Start and End, but on and off by turns in its first Flicker seconds: how a part comes in. */
	float FlickerIn(float T, float Start, float End, float Flicker = 0.1f)
	{
		if (!Within(T, Start, End))
		{
			return 0.0f;
		}
		const float Since = T - Start;
		return Since >= Flicker || FMath::FloorToInt(Since / 0.033f) % 2 == 0 ? 1.0f : 0.0f;
	}

	FColor LerpColor(const FColor& A, const FColor& B, float Alpha)
	{
		auto Channel = [Alpha](uint8 From, uint8 To) { return uint8(FMath::RoundToInt(FMath::Lerp(float(From), float(To), Alpha))); };
		return FColor(Channel(A.R, B.R), Channel(A.G, B.G), Channel(A.B, B.B), Channel(A.A, B.A));
	}

	/** A smooth path through keyframes (time, value): a cubic Hermite with each key's slope from its neighbours. */
	float Smooth(const FVector2f* Keys, int32 Count, float T)
	{
		if (T <= Keys[0].X)
		{
			return Keys[0].Y;
		}
		for (int32 Index = 1; Index < Count; ++Index)
		{
			if (T > Keys[Index].X)
			{
				continue;
			}
			const FVector2f& P0 = Keys[FMath::Max(Index - 2, 0)];
			const FVector2f& P1 = Keys[Index - 1];
			const FVector2f& P2 = Keys[Index];
			const FVector2f& P3 = Keys[FMath::Min(Index + 1, Count - 1)];
			const float Span = FMath::Max(P2.X - P1.X, 1e-4f);
			const float U = (T - P1.X) / Span;
			const float M1 = (P2.Y - P0.Y) / FMath::Max(P2.X - P0.X, 1e-4f) * Span;
			const float M2 = (P3.Y - P1.Y) / FMath::Max(P3.X - P1.X, 1e-4f) * Span;
			const float U2 = U * U;
			const float U3 = U2 * U;
			return (2.0f * U3 - 3.0f * U2 + 1.0f) * P1.Y + (U3 - 2.0f * U2 + U) * M1 + (-2.0f * U3 + 3.0f * U2) * P2.Y + (U3 - U2) * M2;
		}
		return Keys[Count - 1].Y;
	}

	// The loading screen's paints, as the game's look at 1280 x 720.
	const FColor Navy(15, 58, 98);
	const FColor Mid(125, 147, 171);
	const FColor Pale(201, 209, 220);
	const FColor Steel(122, 140, 158);
	const FColor Faint(205, 209, 216);

	/** The width the percentage's line has, in design pixels. */
	constexpr float PercentTrackWidth = 500.0f;
	/** Where GRID INDEX moves to inside the ring, from where the .dui put it. */
	const FVector2D GridIndexInside(-396.0, -7.0);
	/** How far the second layer (Drift) goes when the ring drifts. */
	constexpr float DriftDepth = 0.2f;
}

void UDetroitLoadingScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	RingMaterial = MakeDynamicMaterial(Part(TEXT("Ring")));
	SetTyped(TEXT("ALogoText"), TEXT("LOADING SEQUENCE\nSYS 2.0.1  CHKSM\nDSTRM 04"), MAX_int32);
	for (const TCHAR* Name : { TEXT("BlockA"), TEXT("BlockR1"), TEXT("BlockL"), TEXT("BlockR2"), TEXT("BlockR3") })
	{
		SetOpacity(Name, 0.0f);
	}
	SetWantsTick(true);
}

void UDetroitLoadingScreen::Play(float InDuration)
{
	Duration = FMath::Max(2.0f, InDuration);
	Time = 0.0f;
	NextCode = 0.0f;
	BlockShown.Reset();
	bFading = false;
	bFinished = false;
	// Opaque from the first frame: the room behind it is not to be seen until the load is done.
	Motion.Settle(Part(TEXT("Content")), 1.0f);
}

UDreamWidget* UDetroitLoadingScreen::Part(FName Name)
{
	if (const TWeakObjectPtr<UDreamWidget>* Found = Parts.Find(Name))
	{
		return Found->Get();
	}
	// Looked up once, a missing one too, so a typo warns once rather than every frame.
	UDreamWidget* Widget = Node(Name);
	Parts.Add(Name, Widget);
	return Widget;
}

void UDetroitLoadingScreen::SetOpacity(FName Name, float Opacity)
{
	UDreamWidget* Widget = Part(Name);
	if (Widget != nullptr && FMath::Abs(Widget->GetRenderOpacity() - Opacity) > 1e-3f)
	{
		Widget->SetRenderOpacity(Opacity);
	}
}

void UDetroitLoadingScreen::SetTyped(FName Name, const FString& Full, int32 Count)
{
	const FString Head = Full.Left(FMath::Clamp(Count, 0, Full.Len()));
	const FString* Last = Texts.Find(Name);
	if (Last == nullptr || *Last != Head)
	{
		Texts.Add(Name, Head);
		SetText(Part(Name), FText::AsCultureInvariant(Head));
	}
}

void UDetroitLoadingScreen::SetWidthScale(FName Name, float Scale)
{
	FDetroitMotion::SetPlaneScale(Part(Name), FVector2D(FMath::Max(Scale, 0.001f), 1.0));
}

void UDetroitLoadingScreen::SetPartTint(FName Name, const FColor& Color)
{
	FColor* Last = Tints.Find(Name);
	if (Last == nullptr || *Last != Color)
	{
		Tints.Add(Name, Color);
		SetTint(Part(Name), Color);
	}
}

float UDetroitLoadingScreen::ProgressAt(float InTime) const
{
	// Uneven, as a real load is: stalls and spurts, the last of it quick. Keyed for 8.5 seconds.
	static const FVector2f Keys[] = {
		{ 0.0f, 0.00f }, { 0.6f, 0.05f }, { 1.5f, 0.12f }, { 2.4f, 0.16f }, { 3.0f, 0.31f }, { 3.9f, 0.38f },
		{ 4.6f, 0.55f }, { 5.6f, 0.63f }, { 6.4f, 0.80f }, { 7.4f, 0.93f }, { 8.1f, 1.00f }, { 8.5f, 1.00f } };
	const float T = InTime * 8.5f / Duration;
	for (int32 Index = 1; Index < int32(UE_ARRAY_COUNT(Keys)); ++Index)
	{
		if (T <= Keys[Index].X)
		{
			const float Alpha = (T - Keys[Index - 1].X) / FMath::Max(Keys[Index].X - Keys[Index - 1].X, 1e-4f);
			return FMath::Lerp(Keys[Index - 1].Y, Keys[Index].Y, FMath::Clamp(Alpha, 0.0f, 1.0f));
		}
	}
	return 1.0f;
}

FVector2D UDetroitLoadingScreen::DriftAt(float InTime) const
{
	using namespace DetroitLoadingLocal;
	// The game's ring, measured frame by frame: right 80 pixels (at 1280) over three seconds, a pause, and back,
	// rising and dipping a little on the way.
	static const FVector2f X[] = { { 0.0f, 0.0f }, { 0.9f, 9.0f }, { 2.0f, 68.0f }, { 3.3f, 120.0f }, { 3.9f, 122.0f },
		{ 5.0f, 57.0f }, { 6.6f, -5.0f }, { 8.5f, 20.0f } };
	static const FVector2f Y[] = { { 0.0f, 0.0f }, { 1.2f, 0.0f }, { 2.0f, -6.0f }, { 2.75f, -21.0f }, { 3.2f, -15.0f },
		{ 3.8f, 0.0f }, { 4.3f, 8.0f }, { 4.8f, 0.0f }, { 5.5f, -13.0f }, { 6.6f, -12.0f }, { 8.5f, -4.0f } };
	return FVector2D(Smooth(X, UE_ARRAY_COUNT(X), InTime), Smooth(Y, UE_ARRAY_COUNT(Y), InTime));
}

void UDetroitLoadingScreen::PlayLabelA(float T)
{
	using namespace DetroitLoadingLocal;
	// Out of the ring's left edge at once, typed, its bar reaching for the ring, falling back and reaching again; gone
	// all at once.
	const float End = 1.6f;
	const bool bOn = Within(T, 0.1f, End);
	SetOpacity(TEXT("ATop"), bOn ? 1.0f : 0.0f);
	SetOpacity(TEXT("ATopSmall"), bOn ? 1.0f : 0.0f);
	SetWidthScale(TEXT("ATop"), Ramp(T, 0.1f, 0.4f));
	SetWidthScale(TEXT("ATopSmall"), Ramp(T, 0.14f, 0.34f));
	SetOpacity(TEXT("ATopMark"), Within(T, 0.12f, End) ? 1.0f : 0.0f);
	SetOpacity(TEXT("ASmall"), bOn ? 1.0f : 0.0f);
	SetTyped(TEXT("ASmall"), TEXT("ALPHA PAYLOAD"), Typed(T, 0.15f, 0.022f));
	const int32 CodeCount = Typed(T, 0.2f, 0.03f);
	SetOpacity(TEXT("ACode"), bOn ? 1.0f : 0.0f);
	SetOpacity(TEXT("ACodeTail"), bOn ? 1.0f : 0.0f);
	SetTyped(TEXT("ACodeTail"), TEXT("DSTRM CHK"), CodeCount);
	SetTyped(TEXT("ACode"), TEXT("DSTRM"), CodeCount);
	SetOpacity(TEXT("AGhost"), FlickerIn(T, 0.35f, End) * 0.7f);
	SetOpacity(TEXT("ACount"), FlickerIn(T, 0.7f, End));
	SetOpacity(TEXT("AUnder"), bOn && T >= 0.3f ? 1.0f : 0.0f);
	SetWidthScale(TEXT("AUnder"), Ramp(T, 0.3f, 0.5f));
	float Bar = Ramp(T, 0.2f, 0.5f);
	if (T >= 0.65f)
	{
		Bar = FMath::Lerp(1.0f, 0.45f, Ramp(T, 0.65f, 0.8f));
	}
	if (T >= 0.95f)
	{
		Bar = FMath::Lerp(0.45f, 0.85f, Ramp(T, 0.95f, 1.35f));
	}
	SetOpacity(TEXT("ABar"), bOn && T >= 0.2f ? 1.0f : 0.0f);
	SetOpacity(TEXT("ABarHead"), bOn && T >= 0.2f ? 1.0f : 0.0f);
	SetWidthScale(TEXT("ABar"), Bar);
	// The mark flashes in, then sinks to a ghost of itself.
	float Logo = 0.0f;
	if (bOn)
	{
		Logo = T < 0.3f ? (FMath::FloorToInt((T - 0.1f) / 0.04f) % 2 == 0 ? 1.0f : 0.0f) : FMath::Lerp(1.0f, 0.35f, Ramp(T, 0.3f, 0.7f));
	}
	SetOpacity(TEXT("ALogo"), Logo);
	SetOpacity(TEXT("ALogoHi"), Logo);
	SetOpacity(TEXT("ALogoText"), FlickerIn(T, 0.12f, End) * 0.8f);
}

void UDetroitLoadingScreen::PlayLabelB(float T)
{
	using namespace DetroitLoadingLocal;
	// The same, at the ring's right edge, later and quicker.
	const float End = 4.93f;
	const bool bOn = Within(T, 4.1f, End);
	SetOpacity(TEXT("BTop"), bOn ? 1.0f : 0.0f);
	SetWidthScale(TEXT("BTop"), Ramp(T, 4.1f, 4.4f));
	SetOpacity(TEXT("BTopMark"), Within(T, 4.11f, End) ? 1.0f : 0.0f);
	SetOpacity(TEXT("BSmall"), bOn ? 1.0f : 0.0f);
	SetTyped(TEXT("BSmall"), TEXT("LOADING"), Typed(T, 4.1f, 0.025f));
	SetOpacity(TEXT("BCode"), bOn ? 1.0f : 0.0f);
	SetTyped(TEXT("BCode"), TEXT("DSTRM CHK"), Typed(T, 4.13f, 0.033f));
	SetOpacity(TEXT("BGhost"), FlickerIn(T, 4.3f, End) * 0.7f);
	SetOpacity(TEXT("BCount"), FlickerIn(T, 4.65f, End));
	SetOpacity(TEXT("BUnder"), bOn && T >= 4.2f ? 1.0f : 0.0f);
	SetWidthScale(TEXT("BUnder"), Ramp(T, 4.2f, 4.5f));
	float Bar = Ramp(T, 4.12f, 4.33f);
	if (T >= 4.36f)
	{
		Bar = FMath::Lerp(1.0f, 0.3f, Ramp(T, 4.36f, 4.42f));
	}
	if (T >= 4.45f)
	{
		Bar = FMath::Lerp(0.3f, 0.75f, Ramp(T, 4.45f, 4.9f));
	}
	SetOpacity(TEXT("BBar"), bOn && T >= 4.12f ? 1.0f : 0.0f);
	SetOpacity(TEXT("BBarHead"), bOn && T >= 4.12f ? 1.0f : 0.0f);
	SetWidthScale(TEXT("BBar"), Bar);
}

void UDetroitLoadingScreen::PlayLoadingWord(float T)
{
	using namespace DetroitLoadingLocal;
	const FString Word = TEXT("LOADING");
	// Faint at first, gone a while; then a glitch -- red dashes, two navy bars, an orange line, a frame each --, typed
	// with its newest letters in cyan, a flash of the word in a navy box, gone, typed again with a ghost behind it; it
	// settles in steel with a cyan fringe, and sinks back to faint.
	// Timed to the game's frames (29.97 a second): dashes at 3.30, bars 3.34, the line 3.37, L 3.40, the box 3.57 to
	// 3.64, nothing 3.67, LO again 3.70, LOADIN 3.84, LOADING 3.97.
	for (const TCHAR* Dash : { TEXT("GlitchDashA"), TEXT("GlitchDashB"), TEXT("GlitchDashC"), TEXT("GlitchDashD") })
	{
		SetOpacity(Dash, Within(T, 3.29f, 3.33f) ? 1.0f : 0.0f);
	}
	SetOpacity(TEXT("GlitchBarA"), Within(T, 3.33f, 3.37f) ? 1.0f : 0.0f);
	SetOpacity(TEXT("GlitchBarB"), Within(T, 3.33f, 3.37f) ? 1.0f : 0.0f);
	SetOpacity(TEXT("GlitchBox"), Within(T, 3.33f, 3.37f) ? 1.0f : 0.0f);
	SetOpacity(TEXT("GlitchLine"), Within(T, 3.37f, 3.4f) ? 1.0f : 0.0f);
	SetOpacity(TEXT("GlitchPink"), Within(T, 3.37f, 3.4f) ? 0.8f : 0.0f);
	const bool bBox = Within(T, 3.565f, 3.665f);
	SetOpacity(TEXT("LoadingBox"), bBox ? 1.0f : 0.0f);
	SetOpacity(TEXT("LoadingBoxGhost"), bBox ? 0.8f : 0.0f);
	SetOpacity(TEXT("LoadingBoxText"), bBox ? 1.0f : 0.0f);
	SetTyped(TEXT("LoadingBoxText"), Word, Word.Len());

	int32 Settled = 0;
	int32 Newest = 0;
	FColor Color = Steel;
	float Main = 1.0f;
	if (T < 1.3f)
	{
		Settled = Word.Len();
		Color = Faint;
		Main = 1.0f - Ramp(T, 1.0f, 1.3f);
	}
	else if (Within(T, 3.4f, 3.565f))
	{
		Newest = FMath::Min(Typed(T, 3.4f, 0.033f), 5);
		Settled = FMath::Min(Newest, 3);
	}
	else if (Within(T, 3.7f, 4.25f))
	{
		Newest = T < 3.965f ? FMath::Min(Typed(T, 3.7f, 0.033f) + 1, 6) : Word.Len();
		Settled = T < 4.1f ? FMath::Min(Newest, 3) : 5;
	}
	else if (T >= 4.25f)
	{
		Settled = Word.Len();
		Newest = Word.Len();
		Color = LerpColor(Steel, Faint, Ramp(T, 5.3f, 5.9f));
	}
	else
	{
		Main = 0.0f;
	}
	SetOpacity(TEXT("LoadingText"), Main);
	SetTyped(TEXT("LoadingText"), Word, Settled);
	SetPartTint(TEXT("LoadingText"), Color);
	SetOpacity(TEXT("LoadingTail"), Newest > Settled ? 1.0f : 0.0f);
	SetTyped(TEXT("LoadingTail"), Word, Newest);
	SetOpacity(TEXT("LoadingGhost"), Within(T, 3.83f, 4.4f) ? 0.55f * (1.0f - Ramp(T, 4.1f, 4.4f)) : 0.0f);
	SetTyped(TEXT("LoadingGhost"), Word, 6);
	// The lens's fringe while the word is in steel.
	const float Fringe = Within(T, 4.0f, 5.9f) ? 1.0f - Ramp(T, 5.3f, 5.9f) : 0.0f;
	SetOpacity(TEXT("LoadingCyan"), Fringe * 0.45f);
	SetOpacity(TEXT("LoadingRed"), Fringe * 0.18f);
	SetTyped(TEXT("LoadingCyan"), Word, Settled);
	SetTyped(TEXT("LoadingRed"), Word, Settled);
}

void UDetroitLoadingScreen::PlayStrip(const TCHAR* Prefix, const FStripKey* Keys, int32 KeyCount, float Since, float Opacity)
{
	using namespace DetroitLoadingLocal;
	const FStripKey* Key = nullptr;
	for (int32 Index = 0; Index < KeyCount && Since >= Keys[Index].Since; ++Index)
	{
		Key = &Keys[Index];
	}
	for (int32 Slot = 0; Slot < 5; ++Slot)
	{
		const FName Name(*FString::Printf(TEXT("%s%d"), Prefix, Slot));
		const TCHAR Paint = Key != nullptr ? Key->Slots[Slot] : TEXT('.');
		SetOpacity(Name, Paint == TEXT('.') ? 0.0f : Opacity);
		if (Paint != TEXT('.'))
		{
			SetPartTint(Name, Paint == TEXT('n') ? Navy : (Paint == TEXT('m') ? Mid : Pale));
		}
	}
}

FString UDetroitLoadingScreen::BlockText(int32 Lines, int32 Columns)
{
	static const TCHAR* Words[] = { TEXT("DSTRM"), TEXT("CHK"), TEXT("MEM.ALLOC"), TEXT("NRL"), TEXT("SYNC"), TEXT("OPT"),
		TEXT("SFTW"), TEXT("INIT"), TEXT("THR.LVL"), TEXT("BIOCOMP"), TEXT("NODE"), TEXT("GRID"), TEXT("BUF"), TEXT("0x00") };
	FString Text;
	for (int32 Line = 0; Line < Lines; ++Line)
	{
		// Now and then a gap between paragraphs, as the game's blocks have.
		if (Line > 0 && Random.FRand() < 0.12f)
		{
			Text += TEXT("\n");
			continue;
		}
		FString Row;
		while (Row.Len() < Columns)
		{
			Row += Random.FRand() < 0.5f ? FString::Printf(TEXT("%04X "), Random.RandRange(0, 0xFFFF))
				: FString(Words[Random.RandRange(0, int32(UE_ARRAY_COUNT(Words)) - 1)]) + TEXT(" ");
		}
		Text += Row.Left(Columns - Random.RandRange(0, Columns / 3)) + TEXT("\n");
	}
	return Text;
}

void UDetroitLoadingScreen::PlayBlocks(float T)
{
	using namespace DetroitLoadingLocal;
	struct FBlock
	{
		const TCHAR* Name;
		float Start;
		float End;
		int32 Lines;
		int32 Columns;
	};
	static const FBlock Blocks[] = {
		{ TEXT("BlockA"), 0.1f, 1.6f, 9, 12 },
		{ TEXT("BlockR1"), 0.0f, 1.7f, 16, 26 },
		{ TEXT("BlockL"), 3.5f, 5.7f, 18, 34 },
		{ TEXT("BlockR2"), 3.9f, 6.8f, 25, 26 },
		{ TEXT("BlockR3"), 4.2f, 7.2f, 14, 20 } };
	for (const FBlock& Block : Blocks)
	{
		const float Visible = FlickerIn(T, Block.Start, Block.End) * (Within(T, Block.Start + 0.8f, Block.Start + 0.85f) ? 0.0f : 1.0f);
		bool& bWasShown = BlockShown.FindOrAdd(Block.Name);
		if (Visible > 0.0f && !bWasShown)
		{
			SetTyped(Block.Name, BlockText(Block.Lines, Block.Columns), MAX_int32);
		}
		bWasShown = Visible > 0.0f || Within(T, Block.Start, Block.End);
		SetOpacity(Block.Name, Visible);
	}
}

void UDetroitLoadingScreen::NativeOnTick(float DeltaTime)
{
	using namespace DetroitLoadingLocal;
	Super::NativeOnTick(DeltaTime);
	if (Time < 0.0f)
	{
		return;
	}
	Time += DeltaTime;
	const float T = Time;

	const float Progress = ProgressAt(T);
	const int32 Whole = FMath::FloorToInt(Progress * 100.0f);
	SetTyped(TEXT("Percent"), FString::Printf(TEXT("%d%%"), Whole), MAX_int32);
	SetWidthScale(TEXT("PercentFill"), Whole / 100.0f);
	if (T >= NextCode)
	{
		NextCode = T + 0.25f;
		SetTyped(TEXT("PercentCode"), FString::Printf(TEXT("%d.%04d %04d"), Random.RandRange(1, 9), Random.RandRange(0, 9999), Random.RandRange(0, 9999)), MAX_int32);
	}

	// The ring and what hangs on it drift; the second layer a fifth as far; the back sheet of the ring sits off the
	// front by the parallax the drift gives it (measured off the game's frames).
	const FVector2D Drift = DriftAt(T);
	FDetroitMotion::SetOffset(Part(TEXT("RingGroup")), Drift);
	FDetroitMotion::SetOffset(Part(TEXT("Drift")), Drift * DriftDepth);
	if (UMaterialInstanceDynamic* Material = RingMaterial.Get())
	{
		const float Px = (-18.0f + 0.09f * float(Drift.X)) / 320.0f;
		const float Py = (-18.0f + 0.14f * float(Drift.X)) / 320.0f;
		Material->SetVectorParameterValue(TEXT("Parallax"), FLinearColor(Px, Py, 0.0f, 0.0f));
	}
	SetOpacity(TEXT("Ring"), Ramp(T, 0.0f, 0.15f));

	PlayLabelA(T);
	PlayLabelB(T);
	PlayLoadingWord(T);
	PlayBlocks(T);

	// The triangle strips: at the left as the second label goes, at the right near the end; each settles to a ghost.
	static const FStripKey Left[] = {
		{ 0.0f, TEXT("..np.") }, { 0.033f, TEXT(".....") }, { 0.067f, TEXT("..nn.") }, { 0.1f, TEXT("..np.") },
		{ 0.133f, TEXT("pnp..") }, { 0.167f, TEXT("pnn..") }, { 0.2f, TEXT("ppp..") }, { 0.233f, TEXT("pmmn.") },
		{ 0.3f, TEXT("ppmn.") }, { 0.5f, TEXT(".ppm.") }, { 0.7f, TEXT("...m.") }, { 0.8f, TEXT("...p.") } };
	static const FStripKey Right[] = {
		{ 0.0f, TEXT("p.n..") }, { 0.033f, TEXT(".p.n.") }, { 0.067f, TEXT("..p..") }, { 0.1f, TEXT("ppmn.") },
		{ 0.3f, TEXT(".pmn.") }, { 0.6f, TEXT(".pmm.") } };
	PlayStrip(TEXT("LogoL"), Left, UE_ARRAY_COUNT(Left), T - 4.77f, FMath::Lerp(1.0f, 0.55f, Ramp(T, 5.57f, 5.97f)));
	PlayStrip(TEXT("LogoR"), Right, UE_ARRAY_COUNT(Right), T - 6.27f, FMath::Lerp(1.0f, 0.8f, Ramp(T, 6.87f, 7.4f)));

	// The red marks, with a blink; GRID INDEX moves inside the ring part-way through, and back out later.
	const float Red = Within(T, 0.0f, 3.2f) && !Within(T, 2.0f, 2.07f) ? 1.0f : 0.0f;
	SetOpacity(TEXT("RedMarkA"), Red);
	SetOpacity(TEXT("RedMarkB"), Red);
	const bool bInside = Within(T, 2.9f, 5.2f);
	SetOpacity(TEXT("GridIndex"), Within(T, 2.9f, 2.97f) || Within(T, 5.2f, 5.27f) ? 0.0f : 1.0f);
	FDetroitMotion::SetOffset(Part(TEXT("GridIndex")), bInside ? GridIndexInside : FVector2D::ZeroVector);
	SetTyped(TEXT("GridCode"), bInside ? TEXT("02.05") : (T >= 5.2f ? TEXT("A 9.1") : TEXT("A 01")), MAX_int32);

	if (!bFading && T >= Duration)
	{
		bFading = true;
		Motion.FadeTo(Part(TEXT("Content")), 0.0f, 0.6f, 0.0f);
	}
	if (!bFinished && T >= Duration + 0.9f)
	{
		bFinished = true;
		OnFinished.Broadcast();
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void UDetroitHUD::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Counter = Node(TEXT("Counter"));
	CounterNum = Node(TEXT("CounterNum"));
	CounterLabelA = Node(TEXT("CounterLabelA"));
	CounterLabelB = Node(TEXT("CounterLabelB"));
	CounterFill = Node(TEXT("CounterFill"));
	CounterTrack = Node(TEXT("CounterTrack"));
	CounterTick = Node(TEXT("CounterTick"));
	PromptAnalyze = Node(TEXT("PromptAnalyze"));
	PromptModeText = Node(TEXT("PromptModeText"));
	ModeFrame = Node(TEXT("ModeFrame"));
	for (const TCHAR* Name : { TEXT("CounterNum"), TEXT("CounterFill"), TEXT("CounterTick"), TEXT("CounterTicks") })
	{
		TintedByProgress.Add(Node(Name));
	}
	for (const TCHAR* Name : { TEXT("CounterLineT"), TEXT("CounterLineB") })
	{
		CounterLines.Add(Node(Name));
	}
	for (UDreamWidget* Widget : { Counter.Get(), PromptAnalyze.Get(), ModeFrame.Get() })
	{
		if (IsValid(Widget))
		{
			Widget->SetRenderOpacity(0.0f);
		}
	}
	FDetroitMotion::SetPlaneScale(CounterFill.Get(), FVector2D(0.001, 1.0));
	SetWantsTick(true);
}

void UDetroitHUD::PlayEntrance()
{
	Motion.Settle(this, 0.0f);
	Motion.FadeTo(this, 1.0f, 0.6f);
}

void UDetroitHUD::SetClues(int32 Found, int32 Total)
{
	using namespace DetroitWidgetsLocal;
	TargetFill = Total > 0 ? FMath::Clamp(float(Found) / float(Total), 0.0f, 1.0f) : 0.0f;
	SetText(CounterNum.Get(), Invariant(FString::Printf(TEXT("%d/%d"), Found, Total)));
	const bool bNowComplete = Total > 0 && Found >= Total;
	if (bNowComplete != bComplete)
	{
		bComplete = bNowComplete;
		const FColor Color = bComplete ? DetroitUI::Cyan : DetroitUI::Orange;
		for (const TWeakObjectPtr<UDreamWidget>& Widget : TintedByProgress)
		{
			SetTint(Widget.Get(), Color);
		}
		SetTint(CounterTrack.Get(), FColor(Color.R, Color.G, Color.B, 80));
		for (const TWeakObjectPtr<UDreamWidget>& Line : CounterLines)
		{
			SetTint(Line.Get(), FColor(Color.R, Color.G, Color.B, 100));
		}
		SetText(CounterLabelA.Get(), bComplete ? NSLOCTEXT("DetroitUI", "AnalysisDoneA", "ANALYSIS") : NSLOCTEXT("DetroitUI", "CluesA", "CLUES TO"));
		SetText(CounterLabelB.Get(), bComplete ? NSLOCTEXT("DetroitUI", "AnalysisDoneB", "COMPLETE") : NSLOCTEXT("DetroitUI", "CluesB", "ANALYZE"));
	}
	if (UDreamWidget* Number = CounterNum.Get())
	{
		FDetroitMotion::SetPlaneScale(Number, FVector2D(1.3, 1.3));
		Motion.ScaleTo(Number, 1.0f, 0.4f, 0.0f, EDreamTweenEase::OutBack);
	}
}

void UDetroitHUD::SetAnalysisMode(bool bOn)
{
	bAnalysisMode = bOn;
	Motion.FadeTo(ModeFrame.Get(), bOn ? 1.0f : 0.0f, 0.35f);
	SetText(PromptModeText.Get(), bOn ? NSLOCTEXT("DetroitUI", "ExitAnalysis", "EXIT ANALYSIS") : NSLOCTEXT("DetroitUI", "Analysis", "ANALYSIS MODE"));
	UpdateCounterShown();
}

void UDetroitHUD::SetCanAnalyze(bool bCan)
{
	if (bCan != bCanAnalyze)
	{
		bCanAnalyze = bCan;
		Motion.FadeTo(PromptAnalyze.Get(), bCan ? 1.0f : 0.0f, 0.2f);
	}
}

void UDetroitHUD::HoldCounter(float Seconds)
{
	CounterHold = FMath::Max(CounterHold, Seconds);
	UpdateCounterShown();
}

void UDetroitHUD::UpdateCounterShown()
{
	const bool bWant = bAnalysisMode || CounterHold > 0.0f;
	if (bWant == bCounterShown)
	{
		return;
	}
	bCounterShown = bWant;
	if (bWant)
	{
		Motion.SlideIn(Counter.Get(), FVector2D(0.0, 18.0), 0.35f);
	}
	else
	{
		Motion.FadeTo(Counter.Get(), 0.0f, 0.4f);
	}
}

void UDetroitHUD::NativeOnTick(float DeltaTime)
{
	using namespace DetroitWidgetsLocal;
	Super::NativeOnTick(DeltaTime);
	ShownFill = FMath::FInterpTo(ShownFill, TargetFill, DeltaTime, 5.0f);
	FDetroitMotion::SetPlaneScale(CounterFill.Get(), FVector2D(FMath::Max(ShownFill, 0.001f), 1.0));
	FDetroitMotion::SetOffset(CounterTick.Get(), FVector2D(ShownFill * CounterTrackWidth, 0.0));
	if (CounterHold > 0.0f)
	{
		CounterHold -= DeltaTime;
		if (CounterHold <= 0.0f)
		{
			UpdateCounterShown();
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void UDetroitClueTag::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Marker = Node(TEXT("Marker"));
	MarkerPulse = Node(TEXT("MarkerPulse"));
	MarkerKey = Node(TEXT("MarkerKey"));
	Readout = Node(TEXT("Readout"));
	ReadPct = Node(TEXT("ReadPct"));
	ReadStage = Node(TEXT("ReadStage"));
	ReadFill = Node(TEXT("ReadFill"));
	Tag = Node(TEXT("Tag"));
	Lock = Node(TEXT("Lock"));
	Shackle = Node(TEXT("Shackle"));
	PlateBack = Node(TEXT("PlateBack"));
	Plate = Node(TEXT("Plate"));
	TitleAText = Node(TEXT("TitleAText"));
	TitleBText = Node(TEXT("TitleBText"));
	SuffixText = Node(TEXT("SuffixText"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Measures.Add(Node(*FString::Printf(TEXT("Measure%d"), Index)));
		Numbers.Add(Node(*FString::Printf(TEXT("Num%d"), Index)));
	}
	NumberValues = { 88.8f, 24.3f, 30.5f };
	for (UDreamWidget* Widget : { Marker.Get(), MarkerKey.Get(), Readout.Get(), Tag.Get() })
	{
		if (IsValid(Widget))
		{
			Widget->SetRenderOpacity(0.0f);
		}
	}
	SetTitle(TitleA, TitleB, Suffix);
	SetWantsTick(true);
}

void UDetroitClueTag::SetTitle(const FText& InA, const FText& InB, const FText& InSuffix)
{
	TitleA = InA;
	TitleB = InB;
	Suffix = InSuffix;
	SetText(TitleAText.Get(), TitleA);
	SetText(TitleBText.Get(), TitleB);
	SetText(SuffixText.Get(), Suffix);
	// Each clue its own measurements, from its title.
	const uint32 Hash = GetTypeHash(TitleA.ToString() + TitleB.ToString());
	FRandomStream Stream(int32(Hash & 0x7fffffff));
	NumberValues = { float(Stream.FRandRange(10.0f, 99.9f)), float(Stream.FRandRange(10.0f, 99.9f)), float(Stream.FRandRange(10.0f, 99.9f)) };
}

void UDetroitClueTag::ShowMarker(bool bShow, bool bInReach)
{
	if (bUnlocked)
	{
		bShow = false;
	}
	if (bShow != bMarkerShown)
	{
		bMarkerShown = bShow;
		if (bShow)
		{
			Motion.Pop(Marker.Get(), 0.3f, 0.35f);
		}
		else
		{
			Motion.FadeTo(Marker.Get(), 0.0f, 0.2f);
		}
	}
	Motion.FadeTo(MarkerKey.Get(), bShow && bInReach ? 1.0f : 0.0f, 0.15f);
}

void UDetroitClueTag::BeginAnalysis()
{
	Stage = -1;
	SetAnalysisProgress(0.0f);
	Motion.SlideIn(Readout.Get(), FVector2D(-24.0, 0.0), 0.25f);
	Motion.FadeTo(MarkerKey.Get(), 0.0f, 0.1f);
}

void UDetroitClueTag::SetAnalysisProgress(float Progress)
{
	using namespace DetroitWidgetsLocal;
	Progress = FMath::Clamp(Progress, 0.0f, 1.0f);
	SetText(ReadPct.Get(), Invariant(FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Progress * 100.0f))));
	const int32 NewStage = Progress < 0.34f ? 0 : (Progress < 0.67f ? 1 : 2);
	if (NewStage != Stage)
	{
		Stage = NewStage;
		static const FText StageNames[] = {
			NSLOCTEXT("DetroitUI", "Sync", "SYNC IN PROGRESS"),
			NSLOCTEXT("DetroitUI", "Collect", "COLLECTING DATA"),
			NSLOCTEXT("DetroitUI", "Process", "PROCESSING DATA") };
		const FColor StageColors[] = { DetroitUI::Orange, DetroitUI::White, DetroitUI::Cyan };
		SetText(ReadStage.Get(), StageNames[Stage]);
		SetTint(ReadPct.Get(), StageColors[Stage]);
		SetTint(ReadStage.Get(), StageColors[Stage]);
		SetTint(ReadFill.Get(), StageColors[Stage]);
	}
	FDetroitMotion::SetPlaneScale(ReadFill.Get(), FVector2D(FMath::Max(Progress, 0.001f), 1.0));
}

void UDetroitClueTag::PlayUnlock()
{
	bUnlocked = true;
	bMarkerShown = false;
	bTagShown = true;
	UnlockTime = Clock;
	Motion.FadeTo(Readout.Get(), 0.0f, 0.2f);
	Motion.FadeTo(Marker.Get(), 0.0f, 0.15f);
	Motion.FadeTo(MarkerKey.Get(), 0.0f, 0.1f);
	Motion.Settle(Tag.Get(), 1.0f);
	Motion.Pop(Lock.Get(), 0.4f, 0.3f, 0.05f);
	Motion.Settle(Shackle.Get());
	Motion.OffsetTo(Shackle.Get(), FVector2D(0.0, -10.0), 0.2f, 0.42f, EDreamTweenEase::OutBack);
	Motion.Grow(PlateBack.Get(), false, 0.35f, 0.2f, EDreamTweenEase::OutExpo);
	Motion.Grow(Plate.Get(), false, 0.32f, 0.28f, EDreamTweenEase::OutExpo);
	Motion.SlideIn(TitleAText.Get(), FVector2D(-40.0, 0.0), 0.4f, 0.4f);
	Motion.SlideIn(TitleBText.Get(), FVector2D(-40.0, 0.0), 0.4f, 0.5f);
	Motion.Pop(SuffixText.Get(), 1.8f, 0.3f, 0.78f);
	for (int32 Index = 0; Index < Measures.Num(); ++Index)
	{
		Motion.Grow(Measures[Index].Get(), false, 0.45f, 0.5f + Index * 0.09f);
		if (UDreamWidget* Number = Numbers[Index].Get())
		{
			Number->SetRenderOpacity(0.0f);
			Motion.FadeTo(Number, 0.85f, 0.3f, 0.65f + Index * 0.09f);
		}
	}
}

void UDetroitClueTag::SetTagShown(bool bShown)
{
	if (!bUnlocked || bShown == bTagShown)
	{
		return;
	}
	bTagShown = bShown;
	Motion.FadeTo(Tag.Get(), bShown ? 1.0f : 0.0f, 0.3f);
}

void UDetroitClueTag::NativeOnTick(float DeltaTime)
{
	using namespace DetroitWidgetsLocal;
	Super::NativeOnTick(DeltaTime);
	if (bMarkerShown)
	{
		if (UDreamWidget* Pulse = MarkerPulse.Get())
		{
			const float Phase = FMath::Frac(Clock * 0.8f);
			FDetroitMotion::SetPlaneScale(Pulse, FVector2D(1.0f + 0.7f * Phase, 1.0f + 0.7f * Phase));
			Pulse->SetRenderOpacity(1.0f - Phase);
		}
	}
	if (bUnlocked)
	{
		// The measurements count up as they come in, then keep their last digit restless.
		const float Count = FMath::Clamp((Clock - UnlockTime - 0.65f) / 0.6f, 0.0f, 1.0f);
		const bool bRestless = FMath::FloorToInt(Clock * 6.0f) % 7 == 0;
		for (int32 Index = 0; Index < Numbers.Num() && Index < NumberValues.Num(); ++Index)
		{
			const float Value = NumberValues[Index] * Count + (bRestless && Count >= 1.0f ? 0.1f * Index : 0.0f);
			SetText(Numbers[Index].Get(), Invariant(FString::Printf(TEXT("%05.1f"), Value)));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void UDetroitAnalysisPanel::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Content = Node(TEXT("Content"));
	IconBox = Node(TEXT("IconBox"));
	Title = Node(TEXT("Title"));
	Body = Node(TEXT("Body"));
	HoloFrame = Node(TEXT("HoloFrame"));
	HoloLabel = Node(TEXT("HoloLabel"));
	for (const TCHAR* Name : { TEXT("BrTL"), TEXT("BrTR"), TEXT("BrBL"), TEXT("BrBR") })
	{
		Brackets.Add(Node(Name));
	}
	for (const TCHAR* Name : { TEXT("GuideTop"), TEXT("GuideLeft"), TEXT("GuideLow") })
	{
		Guides.Add(Node(Name));
	}
	for (int32 Index = 0; Index < 3; ++Index)
	{
		Numbers.Add(Node(*FString::Printf(TEXT("Num%d"), Index)));
	}
	NumberValues = { 72.3f, 81.7f, 41.4f };
	Hologram = MakeDynamicMaterial(Node(TEXT("Holo")));
	if (Hologram != nullptr)
	{
		Hologram->SetScalarParameterValue(TEXT("Reveal"), 0.0f);
	}
	if (UDreamWidget* Root = Content.Get())
	{
		Root->SetRenderOpacity(0.0f);
	}
	SetWantsTick(true);
}

void UDetroitAnalysisPanel::PlayShow()
{
	bShown = true;
	ShowTime = Clock;
	Motion.Settle(Content.Get(), 1.0f);
	for (int32 Index = 0; Index < Brackets.Num(); ++Index)
	{
		Motion.Pop(Brackets[Index].Get(), 0.6f, 0.35f, Index * 0.04f);
	}
	Motion.Pop(IconBox.Get(), 0.3f, 0.35f, 0.12f);
	Motion.SlideIn(Title.Get(), FVector2D(-48.0, 0.0), 0.4f, 0.22f);
	Motion.SlideIn(Body.Get(), FVector2D(-32.0, 0.0), 0.4f, 0.36f);
	for (int32 Index = 0; Index < Guides.Num(); ++Index)
	{
		Motion.Grow(Guides[Index].Get(), Index == 1, 0.5f, 0.25f + Index * 0.1f);
	}
	Motion.SlideIn(HoloFrame.Get(), FVector2D(24.0, 0.0), 0.35f, 0.45f);
	Motion.SlideIn(HoloLabel.Get(), FVector2D(0.0, 12.0), 0.3f, 1.45f);
	for (int32 Index = 0; Index < Numbers.Num(); ++Index)
	{
		if (UDreamWidget* Number = Numbers[Index].Get())
		{
			Number->SetRenderOpacity(0.0f);
			Motion.FadeTo(Number, 0.85f, 0.3f, 0.5f + Index * 0.1f);
		}
	}
	if (Hologram != nullptr)
	{
		Hologram->SetScalarParameterValue(TEXT("Reveal"), 0.0f);
	}
}

void UDetroitAnalysisPanel::SetPanelShown(bool bInShown)
{
	if (bInShown == bShown)
	{
		return;
	}
	bShown = bInShown;
	Motion.FadeTo(Content.Get(), bShown ? 1.0f : 0.0f, 0.3f);
}

void UDetroitAnalysisPanel::NativeOnTick(float DeltaTime)
{
	using namespace DetroitWidgetsLocal;
	Super::NativeOnTick(DeltaTime);
	if (ShowTime < 0.0f)
	{
		return;
	}
	const float Since = Clock - ShowTime;
	if (Hologram != nullptr && Since < 2.0f)
	{
		Hologram->SetScalarParameterValue(TEXT("Reveal"), FMath::Clamp((Since - 0.6f) / 1.0f, 0.0f, 1.0f));
	}
	if (Since < 2.0f)
	{
		const float Count = FMath::Clamp((Since - 0.5f) / 0.7f, 0.0f, 1.0f);
		for (int32 Index = 0; Index < Numbers.Num() && Index < NumberValues.Num(); ++Index)
		{
			SetText(Numbers[Index].Get(), Invariant(FString::Printf(TEXT("%05.1f"), NumberValues[Index] * Count)));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void UDetroitObjectiveRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Box = Node(TEXT("Box"));
	Words = Node(TEXT("Words"));
	LeadText = Node(TEXT("LeadText"));
	LeadRestText = Node(TEXT("LeadRestText"));
	Line2Text = Node(TEXT("Line2Text"));
	Locked = Node(TEXT("Locked"));
	Check = Node(TEXT("Check"));
	for (const TCHAR* Name : { TEXT("CTL"), TEXT("CTR"), TEXT("CBL"), TEXT("CBR") })
	{
		Corners.Add(Node(Name));
	}
	SetText(LeadText.Get(), Lead);
	SetText(LeadRestText.Get(), LeadRest);
	SetText(Line2Text.Get(), Line2);
	ApplyState();
}

void UDetroitObjectiveRow::ApplyState()
{
	const bool bLocked = State == EDetroitObjectiveState::Locked;
	const bool bDone = State == EDetroitObjectiveState::Completed;
	SetShown(Box.Get(), !bLocked);
	SetShown(Words.Get(), !bLocked);
	SetShown(Locked.Get(), bLocked);
	SetShown(Check.Get(), bDone);
	SetTint(Box.Get(), bDone ? FColor(44, 50, 58, 110) : FColor(14, 19, 26, 200));
	const FColor Ink = bDone ? DetroitUI::Dim : DetroitUI::White;
	for (const TWeakObjectPtr<UDreamWidget>& Text : { LeadText, LeadRestText, Line2Text })
	{
		SetTint(Text.Get(), Ink);
	}
	const float CornerOpacity = bLocked ? 0.5f : (bDone ? 0.25f : 0.7f);
	for (const TWeakObjectPtr<UDreamWidget>& Corner : Corners)
	{
		if (UDreamWidget* CornerWidget = Corner.Get())
		{
			CornerWidget->SetRenderOpacity(CornerOpacity);
		}
	}
}

void UDetroitObjectiveRow::SetState(EDetroitObjectiveState InState, bool bAnimate)
{
	if (InState == State)
	{
		return;
	}
	State = InState;
	ApplyState();
	if (bAnimate)
	{
		Motion.Grow(Box.Get(), false, 0.35f, 0.15f, EDreamTweenEase::OutExpo);
		Motion.SlideIn(Words.Get(), FVector2D(-28.0, 0.0), 0.35f, 0.3f);
		Motion.Pop(Check.Get(), 1.8f, 0.3f, 0.45f);
	}
}

void UDetroitObjectiveRow::PlayEnter(float Delay)
{
	Motion.SlideIn(GetContentRoot(), FVector2D(-20.0, -10.0), 0.35f, Delay);
}

// ---------------------------------------------------------------------------------------------------------------------

void UDetroitObjectives::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Content = Node(TEXT("Content"));
	HeaderBox = Node(TEXT("HeaderBox"));
	HeaderWords = Node(TEXT("HeaderWords"));
	Spine = Node(TEXT("Spine"));
	for (int32 Index = 0; Index < 3; ++Index)
	{
		StripesL.Add(Node(*FString::Printf(TEXT("StripeL%d"), Index)));
		StripesR.Add(Node(*FString::Printf(TEXT("StripeR%d"), Index)));
	}
	for (const TCHAR* Name : { TEXT("TabTop"), TEXT("TabBottom") })
	{
		Tabs.Add(Node(Name));
	}
	for (const TCHAR* Name : { TEXT("HBrTL"), TEXT("HBrTR"), TEXT("HBrBL"), TEXT("HBrBR") })
	{
		Brackets.Add(Node(Name));
	}
	TArray<UDetroitObjectiveRow*> Found;
	DetroitWidgetsLocal::CollectUnder(GetContentRoot(), Found);
	for (UDetroitObjectiveRow* Row : Found)
	{
		Rows.Add(Row);
	}
	if (UDreamWidget* Root = Content.Get())
	{
		Root->SetRenderOpacity(0.0f);
	}
	SetWantsTick(true);
}

void UDetroitObjectives::Show(float Hold)
{
	bShowing = true;
	HoldLeft = Hold;
	Motion.Settle(Content.Get(), 0.0f);
	Motion.FadeTo(Content.Get(), 1.0f, 0.15f);
	Motion.Grow(HeaderBox.Get(), false, 0.4f, 0.0f, EDreamTweenEase::OutExpo);
	Motion.SlideIn(HeaderWords.Get(), FVector2D(0.0, 14.0), 0.35f, 0.2f);
	for (int32 Index = 0; Index < StripesL.Num(); ++Index)
	{
		Motion.SlideIn(StripesL[Index].Get(), FVector2D(-36.0, 0.0), 0.3f, 0.12f + Index * 0.04f);
		Motion.SlideIn(StripesR[Index].Get(), FVector2D(36.0, 0.0), 0.3f, 0.12f + Index * 0.04f);
	}
	for (const TWeakObjectPtr<UDreamWidget>& TabWidget : Tabs)
	{
		Motion.Grow(TabWidget.Get(), false, 0.3f, 0.22f);
	}
	for (int32 Index = 0; Index < Brackets.Num(); ++Index)
	{
		Motion.Pop(Brackets[Index].Get(), 1.35f, 0.4f, 0.05f + Index * 0.03f);
	}
	Motion.Grow(Spine.Get(), true, 0.45f, 0.3f);
	for (int32 Index = 0; Index < Rows.Num(); ++Index)
	{
		if (UDetroitObjectiveRow* Row = Rows[Index].Get())
		{
			Row->PlayEnter(0.42f + Index * 0.12f);
		}
	}
}

void UDetroitObjectives::Hide()
{
	bShowing = false;
	Motion.FadeTo(Content.Get(), 0.0f, 0.45f);
}

void UDetroitObjectives::SetObjective(int32 Index, EDetroitObjectiveState State, bool bAnimate)
{
	if (UDetroitObjectiveRow* Row = Rows.IsValidIndex(Index) ? Rows[Index].Get() : nullptr)
	{
		Row->SetState(State, bAnimate);
	}
}

void UDetroitObjectives::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	if (bShowing && HoldLeft > 0.0f)
	{
		HoldLeft -= DeltaTime;
		if (HoldLeft <= 0.0f)
		{
			Hide();
		}
	}
}
