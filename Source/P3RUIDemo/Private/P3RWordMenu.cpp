// Copyright DreamMoon. All Rights Reserved.

#include "P3RWordMenu.h"

#include "P3RScreen.h"
#include "P3RUI.h"

#include "Core/Components/DreamText.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamUITextData.h"
#include "Extensions/DreamShape.h"

namespace P3RWordMenuLocal
{
	constexpr float CursorMoveSeconds = 0.06f;
	constexpr float TwitchSeconds = 0.2f;
	constexpr float TwitchEvery = 1.0f;
	constexpr float DenySeconds = 0.32f;
	/** The cursor shape is drawn for a 100 x 100 box and stretched to the word's. */
	constexpr double CursorBase = 100.0;
}

void FP3RWordMenu::Init(UP3RScreen* InScreen, const TCHAR* InPrefix, int32 InCount, const TCHAR* InCursor, const TCHAR* InCursorPink, float InChosenScale)
{
	ChosenScale = InChosenScale;
	if (Colours.IsEmpty())
	{
		Colours = { P3RUI::WordColours[0], P3RUI::WordColours[1], P3RUI::WordColours[2] };
	}
	Words.SetNum(InCount);
	for (int32 Index = 0; Index < InCount; ++Index)
	{
		FWord& Word = Words[Index];
		UDreamWidget* Root = InScreen->Node(FString::Printf(TEXT("%s%d"), InPrefix, Index));
		Word.Root = Root;
		Word.Text = InScreen->Node(FString::Printf(TEXT("%s%dText"), InPrefix, Index));
		Word.Shadow = InScreen->Node(FString::Printf(TEXT("%s%dShadow"), InPrefix, Index));
		Word.Red = InScreen->Node(FString::Printf(TEXT("%s%dRed"), InPrefix, Index));
		if (Root != nullptr)
		{
			const FVector2D Position = Root->GetAnchoredPosition();
			Word.Centre = FVector2D(Position.X, -Position.Y);
			Word.Size = Root->GetSizeDelta();
			Word.Angle = Root->GetRenderRotation().Roll;
		}
		Paint(Index, false);
		// Nothing is under the slash until the cursor arrives.
		P3RUI::SetShown(Word.Red.Get(), false);
	}
	Cursor = InScreen->Node(InCursor);
	CursorPink = InScreen->Node(InCursorPink);
	if (UDreamWidget* White = InScreen->Node(TEXT("CursorWhite")))
	{
		if (const UDreamShape* Shape = Cast<UDreamShape>(White->GetVisual()))
		{
			SlashOutline = Shape->GetPoints();
		}
	}
	if (UDreamWidget* C = Cursor.Get())
	{
		const FVector2D Position = C->GetAnchoredPosition();
		CursorHome = FVector2D(Position.X, -Position.Y);
	}
	Select(0, 0.0f, /*bInstant*/true);
}

void FP3RWordMenu::Paint(int32 Index, bool bChosen)
{
	FWord& Word = Words[Index];
	P3RUI::SetTextColor(Word.Text.Get(), bChosen ? P3RUI::Black : Colours[Index % Colours.Num()]);
	// The chosen word is black (red where the slash lies over it), with no shadow.
	P3RUI::SetShown(Word.Shadow.Get(), !bChosen);
	P3RUI::SetTextColor(Word.Red.Get(), bChosen ? P3RUI::Red : P3RUI::White);
	if (UDreamWidget* Root = Word.Root.Get())
	{
		P3RUI::Settle(Root);
		if (bChosen)
		{
			P3RUI::Scale2DTo(Root, FVector2D(ChosenScale), 0.1f, 0.0f, EDreamTweenEase::OutBack);
		}
		else
		{
			P3RUI::SetScale2D(Root, FVector2D(1.0));
		}
		P3RUI::SetOffset(Root, FVector2D::ZeroVector);
		Root->SetRenderOpacity(1.0f);
	}
}

bool FP3RWordMenu::Select(int32 Index, float Now, bool bInstant)
{
	using namespace P3RWordMenuLocal;
	if (Words.IsEmpty())
	{
		return false;
	}
	Index = (Index % Words.Num() + Words.Num()) % Words.Num();
	if (Index == Selected && !bInstant)
	{
		return false;
	}

	// The cursor leaves from wherever it is drawn now, which may be part way through the last move.
	if (UDreamWidget* C = Cursor.Get())
	{
		CursorFromCentre = CursorHome + P3RUI::GetOffset(C);
		CursorFromAngle = C->GetRenderRotation().Roll;
		const FVector S = C->GetRenderScale();
		CursorFromSize = FVector2D(S.Y, S.Z) * CursorBase;
	}
	const int32 Previous = Selected;
	Selected = Index;
	if (Previous != Index && Words.IsValidIndex(Previous))
	{
		Paint(Previous, false);
	}
	Paint(Index, true);

	// Draw order: the words overlap, and which lies over which is set word by word (Layers, low to high, then in list
	// order); the chosen word last of all, on top of everything it overlaps.
	TArray<int32> Order;
	for (int32 I = 0; I < Words.Num(); ++I)
	{
		if (I != Index)
		{
			Order.Add(I);
		}
	}
	Order.StableSort([this](int32 A, int32 B)
	{
		return (Layers.IsValidIndex(A) ? Layers[A] : 0) < (Layers.IsValidIndex(B) ? Layers[B] : 0);
	});
	for (const int32 I : Order)
	{
		if (UDreamWidget* Root = Words[I].Root.Get())
		{
			Root->SetAsLastSibling();
		}
	}
	if (UDreamWidget* Root = Words[Index].Root.Get())
	{
		Root->SetAsLastSibling();
	}
	CursorMoveAt = bInstant ? -100.0f : Now;
	TwitchAt = Now;
	return true;
}

void FP3RWordMenu::DropIn(float Now, float Delay)
{
	const int32 Count = Words.Num();
	for (int32 Index = 0; Index < Count; ++Index)
	{
		UDreamWidget* Root = Words[Index].Root.Get();
		if (Root == nullptr)
		{
			continue;
		}
		P3RUI::Settle(Root);
		const float D = Delay + (Count - 1 - Index) / 30.0f;
		P3RUI::SetOffset(Root, FVector2D(0.0, -30.0));
		Root->SetRenderOpacity(0.0f);
		P3RUI::OffsetTo(Root, FVector2D::ZeroVector, 0.4f, D, EDreamTweenEase::OutCubic);
		P3RUI::FadeTo(Root, 1.0f, 0.4f, D, EDreamTweenEase::Linear);
		if (Index == Selected)
		{
			P3RUI::SetScale2D(Root, FVector2D(1.0));
			P3RUI::Scale2DTo(Root, FVector2D(ChosenScale), 0.12f, D + 0.4f, EDreamTweenEase::OutBack);
		}
	}
	CursorShowAt = Now + Delay + 0.4f;
	CursorMoveAt = -100.0f;
	if (UDreamWidget* C = Cursor.Get())
	{
		C->SetRenderOpacity(0.0f);
	}
}

void FP3RWordMenu::ShowNow(float Now)
{
	for (int32 Index = 0; Index < Words.Num(); ++Index)
	{
		if (UDreamWidget* Root = Words[Index].Root.Get())
		{
			P3RUI::Settle(Root);
			P3RUI::SetOffset(Root, FVector2D::ZeroVector);
			Root->SetRenderOpacity(1.0f);
			P3RUI::SetScale2D(Root, FVector2D(Index == Selected ? ChosenScale : 1.0f));
		}
	}
	// As if the cursor had arrived long ago.
	CursorShowAt = Now - 1.0f;
	CursorMoveAt = -100.0f;
	if (UDreamWidget* C = Cursor.Get())
	{
		C->SetRenderOpacity(1.0f);
	}
}

void FP3RWordMenu::FlyOut()
{
	CursorShowAt = 1e9f;
	if (UDreamWidget* C = Cursor.Get())
	{
		C->SetRenderOpacity(0.0f);
	}
	for (int32 Index = 0; Index < Words.Num(); ++Index)
	{
		if (UDreamWidget* Root = Words[Index].Root.Get())
		{
			P3RUI::Settle(Root);
			const float Delay = Index * 0.012f;
			P3RUI::OffsetTo(Root, FVector2D(140.0, 0.0), 0.16f, Delay, EDreamTweenEase::InCubic);
			P3RUI::FadeTo(Root, 0.0f, 0.16f, Delay, EDreamTweenEase::InCubic);
		}
	}
}

void FP3RWordMenu::Vanish()
{
	CursorShowAt = 1e9f;
	if (UDreamWidget* C = Cursor.Get())
	{
		C->SetRenderOpacity(0.0f);
	}
	for (FWord& Word : Words)
	{
		if (UDreamWidget* Root = Word.Root.Get())
		{
			P3RUI::Settle(Root);
			Root->SetRenderOpacity(0.0f);
		}
	}
}

void FP3RWordMenu::PlaceCursor(const FVector2D& Centre, float Angle, const FVector2D& Size)
{
	using namespace P3RWordMenuLocal;
	if (UDreamWidget* C = Cursor.Get())
	{
		P3RUI::SetOffset(C, Centre - CursorHome);
		C->SetRenderRotation(FRotator(0.0, 0.0, Angle));
		C->SetRenderScale(FVector(1.0, Size.X / CursorBase * CursorPulse, Size.Y / CursorBase * CursorPulse));
	}
}

void FP3RWordMenu::UpdateInk()
{
	using namespace P3RWordMenuLocal;
	const UDreamWidget* C = Cursor.Get();
	const float CursorOpacity = C != nullptr ? C->GetRenderOpacity() : 0.0f;
	const bool bLive = C != nullptr && CursorOpacity > 0.01f && SlashOutline.Num() >= 3;

	// The slash as drawn now, in design pixels (y down): its box scaled, then turned clockwise by Roll, about its centre.
	TArray<FVector2D> Slash;
	if (bLive)
	{
		const FVector2D Centre = CursorHome + P3RUI::GetOffset(C);
		const FVector S = C->GetRenderScale();
		const double Radians = FMath::DegreesToRadians(C->GetRenderRotation().Roll);
		const double Cos = FMath::Cos(Radians), Sin = FMath::Sin(Radians);
		for (const FVector2D& P : SlashOutline)
		{
			const double X = (P.X - 0.5) * CursorBase * S.Y;
			const double Y = -(P.Y - 0.5) * CursorBase * S.Z;
			Slash.Add(Centre + FVector2D(X * Cos - Y * Sin, X * Sin + Y * Cos));
		}
	}

	for (FWord& Word : Words)
	{
		UDreamWidget* Root = Word.Root.Get();
		UDreamWidget* Red = Word.Red.Get();
		if (Root == nullptr || Red == nullptr)
		{
			continue;
		}
		// Into the word's frame: back by its offset, turned back by its angle, divided by its scale, then normalized
		// over its box with y up.
		bool bOverlaps = false;
		TArray<FVector2D> Local;
		if (bLive && Root->GetRenderOpacity() > 0.01f)
		{
			const FVector2D Centre = Word.Centre + P3RUI::GetOffset(Root);
			const FVector S = Root->GetRenderScale();
			const double Radians = FMath::DegreesToRadians(-Root->GetRenderRotation().Roll);
			const double Cos = FMath::Cos(Radians), Sin = FMath::Sin(Radians);
			FBox2D Bounds(ForceInit);
			for (const FVector2D& P : Slash)
			{
				const FVector2D D = P - Centre;
				const double X = (D.X * Cos - D.Y * Sin) / FMath::Max(S.Y, 1e-3);
				const double Y = (D.X * Sin + D.Y * Cos) / FMath::Max(S.Z, 1e-3);
				const FVector2D N(X / Word.Size.X + 0.5, -Y / Word.Size.Y + 0.5);
				Local.Add(N);
				Bounds += N;
			}
			bOverlaps = Bounds.Max.X > 0.0 && Bounds.Min.X < 1.0 && Bounds.Max.Y > 0.0 && Bounds.Min.Y < 1.0;
		}
		if (bOverlaps != Word.bInkShown)
		{
			Word.bInkShown = bOverlaps;
			P3RUI::SetShown(Red, bOverlaps);
		}
		if (!bOverlaps)
		{
			continue;
		}
		Red->SetRenderOpacity(CursorOpacity);
		bool bSame = Local.Num() == Word.InkPoints.Num();
		for (int32 I = 0; bSame && I < Local.Num(); ++I)
		{
			bSame = Local[I].Equals(Word.InkPoints[I], 1e-4);
		}
		if (!bSame)
		{
			Word.InkPoints = Local;
			if (UDreamShape* Shape = Cast<UDreamShape>(Red->GetVisual()))
			{
				Shape->SetPoints(Local);
			}
		}
	}
}

void FP3RWordMenu::Tick(float Now)
{
	using namespace P3RWordMenuLocal;
	if (!Words.IsValidIndex(Selected))
	{
		return;
	}
	const FWord& Target = Words[Selected];

	// The cursor pulses on a move and then once a second; the words under it change colour with it.
	{
		float Since = Now - TwitchAt;
		if (Since > TwitchEvery)
		{
			TwitchAt += FMath::FloorToFloat(Since / TwitchEvery) * TwitchEvery;
			Since = Now - TwitchAt;
		}
		const float K = Since < TwitchSeconds ? FMath::Sin(Since / TwitchSeconds * UE_PI) : 0.0f;
		CursorPulse = 1.0f + 0.1f * K;
	}

	if (UDreamWidget* C = Cursor.Get(); C != nullptr && Now >= CursorShowAt)
	{
		// The way in: from the upper right, past the word to the lower left, back a little, home.
		const float E = Now - CursorShowAt;
		FVector2D Lead = FVector2D::ZeroVector;
		if (E < 0.2f)
		{
			Lead = FMath::Lerp(FVector2D(40.0, -40.0), FVector2D(-20.0, 20.0), (double)FMath::Sin(E / 0.2f * UE_HALF_PI));
		}
		else if (E < 0.3f)
		{
			Lead = FMath::Lerp(FVector2D(-20.0, 20.0), FVector2D(10.0, -10.0), (double)FMath::Sin((E - 0.2f) / 0.1f * UE_HALF_PI));
		}
		else if (E < 0.4f)
		{
			Lead = FMath::Lerp(FVector2D(10.0, -10.0), FVector2D::ZeroVector, (double)FMath::Sin((E - 0.3f) / 0.1f * UE_HALF_PI));
		}
		C->SetRenderOpacity(FMath::Clamp(E / 0.08f, 0.0f, 1.0f));

		// A move: 60 ms, linear -- position, angle and size together.
		const float M = FMath::Clamp((Now - CursorMoveAt) / CursorMoveSeconds, 0.0f, 1.0f);
		const FVector2D TargetSize = Target.Size * ChosenScale;
		PlaceCursor(FMath::Lerp(CursorFromCentre, Target.Centre, (double)M) + Lead,
			FMath::Lerp(CursorFromAngle, Target.Angle, M),
			FMath::Lerp(CursorFromSize, TargetSize, (double)M));
	}

	if (UDreamWidget* Pink = CursorPink.Get())
	{
		const float Phase = FMath::Fmod(Now, 0.75f);
		const float K = Phase > 0.6f ? FMath::Sin((Phase - 0.6f) / 0.15f * UE_PI) : 0.0f;
		P3RUI::SetScale2D(Pink, FVector2D(1.0 + 0.05 * K));
	}

	// A refused choice shakes.
	if (UDreamWidget* Root = Target.Root.Get())
	{
		const float D = Now - DenyAt;
		if (D >= 0.0f && D < DenySeconds)
		{
			const float Amp = 12.0f * (1.0f - D / DenySeconds);
			P3RUI::SetOffset(Root, FVector2D(FMath::Sin(D * 70.0f) * Amp, 0.0));
		}
		else if (D >= DenySeconds && D < DenySeconds + 0.1f)
		{
			P3RUI::SetOffset(Root, FVector2D::ZeroVector);
		}
	}

	UpdateInk();
}
