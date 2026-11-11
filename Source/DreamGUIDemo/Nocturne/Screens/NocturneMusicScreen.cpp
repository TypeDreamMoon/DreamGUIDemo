// Copyright DreamMoon. All Rights Reserved.

#include "NocturneMusicScreen.h"

#include "../Settings/NocturneUI.h"

#include "Core/Components/DreamWidget.h"
#include "Event/DreamKeyEventData.h"
#include "Extensions/DreamRing.h"
#include "Extensions/Lyrics/DreamLyricsView.h"
#include "Interaction/UIButton.h"

namespace NocturneMusicLocal
{
	struct FTrack
	{
		const TCHAR* Title;
		const TCHAR* En;
		float Seconds;
	};
	const FTrack Tracks[] = {
		{ TEXT("夜曲"), TEXT("NOCTURNE"), 222.0f },
		{ TEXT("旧港灯塔"), TEXT("HARBOR LIGHT"), 178.0f },
		{ TEXT("第六颗星"), TEXT("THE SIXTH STAR"), 245.0f },
		{ TEXT("弦上的风"), TEXT("WIND ON THE STRING"), 196.0f },
		{ TEXT("未寄出的信"), TEXT("UNSENT LETTER"), 157.0f },
		{ TEXT("启明之前"), TEXT("BEFORE THE DAWN"), 301.0f },
	};

	// The first track's words, written for this demo. The others are instrumental.
	const TCHAR* NocturneLrc =
		TEXT("[00:00.40]夜色把航线写进星图\n")
		TEXT("[00:03.40]我在第六颗星下等你\n")
		TEXT("[00:06.40]风停在没有名字的港口\n")
		TEXT("[00:09.40]灯一盏一盏替我回答\n")
		TEXT("[00:12.40]若你听见这支夜曲\n")
		TEXT("[00:15.40]就沿着弦的光回来\n")
		TEXT("[00:18.40]把没说完的话交给黎明\n")
		TEXT("[00:21.40]启明会替我们记得\n")
		TEXT("[00:24.40]夜色把航线写进星图\n")
		TEXT("[00:27.40]我还在第六颗星下\n");

	// The spectrum's ring and how tall its bars stand, design units.
	constexpr double BarRadius = 268.0;
	constexpr float BarMin = 4.0f;
	constexpr float BarMax = 38.0f;

	FString Clock(float Seconds)
	{
		const int32 Whole = FMath::Max(0, FMath::FloorToInt(Seconds));
		return FString::Printf(TEXT("%02d:%02d"), Whole / 60, Whole % 60);
	}
}

void UNocturneMusicScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace NocturneMusicLocal;

	if (UDreamWidget* Holder = Node(TEXT("Lyrics")))
	{
		LyricsView = Holder->GetComponent<UDreamLyricsView>();
	}
	for (int32 Index = 0; Index < TrackCount; ++Index)
	{
		UDreamWidget* Row = Node(*FString::Printf(TEXT("Track%d"), Index));
		UUIButton* Button = Row != nullptr ? Row->GetComponent<UUIButton>() : nullptr;
		if (Button == nullptr)
		{
			continue;
		}
		Button->SetUseFocusRing(false);
		Button->GetOnClickEvent().AddWeakLambda(this, [this, Index]() { PlayTrack(Index); });
		Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Index](EUISelectableSelectionState InState, bool)
		{
			const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed
				|| InState == EUISelectableSelectionState::Focused;
			HoveredTrack = bOver ? Index : (HoveredTrack == Index ? INDEX_NONE : HoveredTrack);
			PaintTracks();
		});
	}

	// The spectrum's bars stand round the ring, each turned to point away from the star.
	BarHeights.Init(BarMin, BarCount);
	for (int32 Index = 0; Index < BarCount; ++Index)
	{
		UDreamWidget* Bar = Node(*FString::Printf(TEXT("Spec%d"), Index));
		if (Bar == nullptr)
		{
			continue;
		}
		const double Angle = 2.0 * PI * Index / BarCount;
		Bar->SetAnchoredPosition(FVector2D(FMath::Sin(Angle), FMath::Cos(Angle)) * BarRadius);
		Bar->SetRenderTransformAngle(static_cast<float>(-FMath::RadiansToDegrees(Angle)));
	}
	if (UDreamWidget* Arc = Node(TEXT("ProgressArc")))
	{
		if (UDreamRing* Ring = Cast<UDreamRing>(Arc->GetVisual()))
		{
			Ring->SetStartAngle(90.0f);
		}
	}
	PlayTrack(0);
}

void UNocturneMusicScreen::PlayTrack(int32 InTrack)
{
	// The disc's layers in depth (+ further away): the spectrum behind the rings, the lyric lines in front.
	if (!bDiscLayered)
	{
		bDiscLayered = true;
		for (const TPair<const TCHAR*, double>& Layer : { TPair<const TCHAR*, double>(TEXT("Spectrum"), 40.0),
			TPair<const TCHAR*, double>(TEXT("OrbitNow"), -26.0), TPair<const TCHAR*, double>(TEXT("OrbitNext"), -20.0) })
		{
			if (UDreamWidget* Part = Node(Layer.Key))
			{
				Part->SetRenderTranslation(FVector(Layer.Value, 0.0, 0.0));
				GetRest(Part);
			}
		}
	}
	using namespace NocturneMusicLocal;
	Track = FMath::Clamp(InTrack, 0, TrackCount - 1);
	PlayTime = 0.0f;
	bPlaying = true;
	Lines.Reset();
	const FString Lrc = Track == 0 ? FString(NocturneLrc) : FString(TEXT("[00:00.00]纯音乐   ·   INSTRUMENTAL\n"));
	// The orbit's lines come from the same text the lyrics view reads.
	TArray<FString> Raw;
	Lrc.ParseIntoArrayLines(Raw);
	for (const FString& Line : Raw)
	{
		int32 Close = INDEX_NONE;
		if (Line.Len() < 10 || Line[0] != TEXT('[') || !Line.FindChar(TEXT(']'), Close))
		{
			continue;
		}
		const float Minutes = FCString::Atof(*Line.Mid(1, 2));
		const float Seconds = FCString::Atof(*Line.Mid(4, Close - 4));
		Lines.Add({ Minutes * 60.0f + Seconds, Line.Mid(Close + 1) });
	}
	if (UDreamLyricsView* View = LyricsView.Get())
	{
		View->LoadLRC(Lrc);
		View->Seek(0.0f);
	}
	NocturneUI::SetText(Node(TEXT("NowTitle")), FText::FromString(FString::Printf(TEXT("%s   ·   %s"), Tracks[Track].Title, Tracks[Track].En)));
	// The track's cover turns over to face the camera.
	if (UDreamWidget* Cover = Node(*FString::Printf(TEXT("Track%dCover"), Track)))
	{
		Settle(Cover);
		Cover->SetRenderRotation(FRotator(0.0, -90.0, 0.0));
		RotateTo(Cover, FRotator::ZeroRotator, 0.45f, 0.0f, EDreamTweenEase::OutBack);
	}
	ShownLine = -2;
	UpdateLyricLines(true);
	PaintTracks();
	BurstSort(LeftSortIndex, 0.4f, 0.25f);
}

void UNocturneMusicScreen::SetPlaying(bool bInPlaying)
{
	bPlaying = bInPlaying;
	PaintTracks();
}

bool UNocturneMusicScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData != nullptr)
	{
		const FKey& Key = EventData->Key;
		if (Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom)
		{
			SetPlaying(!bPlaying);
			return true;
		}
		if (Key == EKeys::Q || Key == EKeys::Gamepad_LeftShoulder)
		{
			PlayTrack((Track + TrackCount - 1) % TrackCount);
			return true;
		}
		if (Key == EKeys::E || Key == EKeys::Gamepad_RightShoulder)
		{
			PlayTrack((Track + 1) % TrackCount);
			return true;
		}
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNocturneMusicScreen::NativeOnTick(float DeltaTime)
{
	Super::NativeOnTick(DeltaTime);
	if (bPlaying)
	{
		PlayTime = FMath::Min(PlayTime + DeltaTime, NocturneMusicLocal::Tracks[Track].Seconds);
	}
	if (UDreamLyricsView* View = LyricsView.Get())
	{
		View->SetTime(PlayTime);
	}
	UpdateLyricLines(false);
	UpdateDisc(DeltaTime);

	// The disc lies back round the star like a planet's ring, swaying a little: it tips up from flat as the screen comes
	// in and falls flat again on the way out.
	if (UDreamWidget* Disc = Node(TEXT("Disc")))
	{
		const float Now = GetScreenTime();
		const double Tip = TipAt < 0.0f ? 1.0 : 1.0 - FMath::Pow(1.0 - FMath::Clamp((Now - TipAt) / 0.7f, 0.0f, 1.0f), 3.0);
		const double Fall = FallAt < 0.0f ? 0.0 : FMath::Pow(FMath::Clamp((Now - FallAt) / 0.28f, 0.0f, 1.0f), 2.0);
		const double Pitch = FMath::Lerp(-82.0, -34.0, Tip) - 46.0 * Fall + 3.0 * FMath::Sin(Now * 0.4f);
		Disc->SetRenderRotation(FRotator(Pitch, 9.0 * FMath::Sin(Now * 0.27f), 0.0));
	}
}

void UNocturneMusicScreen::PlayExit()
{
	Super::PlayExit();
	FallAt = GetScreenTime();
}

void UNocturneMusicScreen::UpdateLyricLines(bool bInForce)
{
	int32 Current = INDEX_NONE;
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		if (Lines[Index].Time <= PlayTime)
		{
			Current = Index;
		}
	}
	if (Current == ShownLine && !bInForce)
	{
		return;
	}
	ShownLine = Current;
	const FString Now = Lines.IsValidIndex(Current) ? Lines[Current].Text : FString();
	const FString Next = Lines.IsValidIndex(Current + 1) ? Lines[Current + 1].Text : FString();
	NocturneUI::SetText(Node(TEXT("OrbitNow")), FText::FromString(Now));
	NocturneUI::SetText(Node(TEXT("OrbitNext")), FText::FromString(Next));
	// The new line fades up where it is; the next one slides in under it. (They lie on the disc; the disc tilts.)
	RunIn(Node(TEXT("OrbitNow")), 0.0f, 0.35f, FVector2D::ZeroVector, 0.0f);
	RunIn(Node(TEXT("OrbitNext")), 0.1f, 0.35f, FVector2D::ZeroVector, 0.0f);
}

void UNocturneMusicScreen::UpdateDisc(float DeltaTime)
{
	using namespace NocturneMusicLocal;
	const float Duration = Tracks[Track].Seconds;
	if (UDreamWidget* Arc = Node(TEXT("ProgressArc")))
	{
		if (UDreamRing* Ring = Cast<UDreamRing>(Arc->GetVisual()))
		{
			// Clockwise from twelve o'clock.
			Ring->SetEndAngle(90.0f - 360.0f * FMath::Clamp(PlayTime / Duration, 0.0f, 1.0f));
		}
	}
	NocturneUI::SetText(Node(TEXT("NowTime")), FText::FromString(FString::Printf(TEXT("%s   /   %s"), *Clock(PlayTime), *Clock(Duration))));

	// The spectrum: each bar a few waves of its own, louder in the low end, still when paused.
	const float Time = PlayTime;
	for (int32 Index = 0; Index < BarCount; ++Index)
	{
		float Target = BarMin;
		if (bPlaying)
		{
			const float Band = static_cast<float>(Index) / BarCount;
			const float Wave = 0.5f + 0.25f * FMath::Sin(Time * (5.1f + 2.3f * Band) + Index * 1.7f)
				+ 0.25f * FMath::Sin(Time * (8.3f - 3.1f * Band) + Index * 0.6f);
			const float Beat = 0.6f + 0.4f * FMath::Pow(0.5f + 0.5f * FMath::Cos(Time * PI * 2.0f * 1.25f), 6.0f);
			Target = BarMin + (BarMax - BarMin) * Wave * Beat * (1.0f - 0.45f * FMath::Abs(Band - 0.5f) * 2.0f);
		}
		BarHeights[Index] += (Target - BarHeights[Index]) * (1.0f - FMath::Exp(-DeltaTime * 14.0f));
		if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Spec%d"), Index)))
		{
			Bar->SetSizeDelta(FVector2D(4.0, BarHeights[Index]));
		}
	}
}

void UNocturneMusicScreen::PaintTracks()
{
	using namespace NocturneUI;
	for (int32 Index = 0; Index < TrackCount; ++Index)
	{
		const bool bOn = Index == Track;
		const bool bOver = Index == HoveredTrack;
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Track%dBar"), Index)), bOn);
		SetTint(Node(*FString::Printf(TEXT("Track%dBack"), Index)), bOn ? RowLit : (bOver ? RowHover : Clear));
		SetTextColor(Node(*FString::Printf(TEXT("Track%dTitle"), Index)), bOn ? InkBright : (bOver ? Ink : InkDim));
		SetTextColor(Node(*FString::Printf(TEXT("Track%dEn"), Index)), bOn ? CyanTag : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Track%dNum"), Index)), bOn ? Cyan : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Track%dTime"), Index)), bOn ? (bPlaying ? Cyan : Amber) : InkFaint);
	}
}

UDreamWidget* UNocturneMusicScreen::GetInitialFocus() const
{
	return Node(*FString::Printf(TEXT("Track%d"), Track));
}

void UNocturneMusicScreen::PlayContentEntrance(float Delay)
{
	TipAt = GetScreenTime() + Delay - 0.1f;
	FallAt = -1.0f;
	RunIn(Node(TEXT("Disc")), Delay, 0.4f, FVector2D::ZeroVector, 0.0f);
	RunIn(Node(TEXT("NowPlaying")), Delay + 0.15f, 0.3f, FVector2D(0.0, 10.0));
	RunInChildren(Node(TEXT("TrackList")), Delay + 0.05f, 0.03f, FVector2D(-16.0, 0.0));
	RunIn(Node(TEXT("Lyrics")), Delay + 0.15f, 0.35f, FVector2D(16.0, 0.0));
	RunIn(Node(TEXT("LyricTag")), Delay + 0.1f, 0.3f, FVector2D(16.0, 0.0));
	if (UDreamWidget* Disc = Node(TEXT("Disc")))
	{
		Disc->SetRenderScale(FVector(1.0, 0.92, 0.92));
		Disc->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.5f, Delay, EDreamTweenEase::OutCubic);
	}
}
