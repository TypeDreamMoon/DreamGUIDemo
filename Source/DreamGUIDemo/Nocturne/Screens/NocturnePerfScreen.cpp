// Copyright DreamMoon. All Rights Reserved.

#include "NocturnePerfScreen.h"

#include "../Settings/NocturneUI.h"
#include "Core/Components/DreamRectBlock.h"
#include "Core/Components/DreamWidget.h"
#include "DreamUIRender/DreamUIRenderStats.h"
#include "Interaction/UIButton.h"
#include "Misc/CommandLine.h"

namespace NocturnePerfLocal
{
	constexpr int32 Columns = 100;
	constexpr int32 Rows = 50;
	constexpr double PitchX = 18.8;
	constexpr double PitchY = 16.8;
	// Bars, not squares: an 11-unit square turning reads as a dot standing still.
	constexpr double TileWidth = 14.0;
	constexpr double TileHeight = 4.0;

	constexpr float BandFrom = 1.0f;
	constexpr float BandTo = 5.2f;
	constexpr double BandHalfWidth = 90.0;
	constexpr float AllAt = 5.2f;
	constexpr float WaveSeconds = 0.7f;
	constexpr float SpinDegreesPerSecond = 220.0f;
	constexpr float TestsAt = 8.5f;
	constexpr int32 TestCount = 3166;

	const FColor Body(0x8F, 0xE3, 0xFF, 0x1A);
	const FColor Border(0x8F, 0xE3, 0xFF, 0x55);
	const FColor AmberBody(0xFF, 0xB4, 0x54, 0x22);
	const FColor AmberBorder(0xFF, 0xB4, 0x54, 0x66);
	const FColor LitBody(0x8F, 0xE3, 0xFF, 0xA0);
	const FColor SpinBody(0x8F, 0xE3, 0xFF, 0x48);
	const FColor Ink(0xD7, 0xE2, 0xEA);
	const FColor Cyan(0x8F, 0xE3, 0xFF);

	bool IsAmber(int32 InIndex) { return InIndex % 13 == 5; }
}

void UNocturnePerfScreen::PlayContentEntrance(float Delay)
{
	StartTime = GetScreenTime();
	NextSample = 0.0f;
	bStarted = true;
	bTestsShown = false;
	Lit = -1;
	FParse::Value(FCommandLine::Get(), TEXT("-PerfFrameMs="), FrameMs);
	if (Tiles.Num() == 0)
	{
		BuildWall();
	}
	RunInChildren(Node(TEXT("SpecList")), Delay + 0.08f, 0.05f, FVector2D(-30.0, 0.0));
	// What happened before this screen does not count.
	DreamUIRenderStats::TakeSnapshot(true);
}

void UNocturnePerfScreen::BuildWall()
{
	using namespace NocturnePerfLocal;
	UDreamWidget* Wall = Node(TEXT("Wall"));
	if (Wall == nullptr)
	{
		return;
	}
	const int32 Count = Columns * Rows;
	Tiles.Reserve(Count);
	Blocks.Reserve(Count);
	TileAngle.Init(0.0f, Count);
	TileLit.Init(false, Count);
	TileSpot.Reserve(Count);
	for (int32 Row = 0; Row < Rows; ++Row)
	{
		for (int32 Column = 0; Column < Columns; ++Column)
		{
			const int32 Index = Row * Columns + Column;
			const FVector2D Spot((Column - (Columns - 1) * 0.5) * PitchX, ((Rows - 1) * 0.5 - Row) * PitchY);
			// The runtime's own order: a widget, its size, registered, parented, placed, then its visual once it has a
			// canvas to be drawn on.
			UDreamWidget* Tile = NewObject<UDreamWidget>(Wall, NAME_None, RF_Transient);
			Tile->SetWidth(TileWidth);
			Tile->SetHeight(TileHeight);
			Tile->OnRegister();
			Tile->TrySetParent(Wall, false);
			Tile->SetAnchorMin(FVector2D(0.5, 0.5));
			Tile->SetAnchorMax(FVector2D(0.5, 0.5));
			Tile->SetAnchoredPosition(Spot);
			UDreamRectBlock* Block = Cast<UDreamRectBlock>(Tile->CreateNewVisual(UDreamRectBlock::StaticClass()));
			if (Block != nullptr)
			{
				Block->SetCornerRadius(FVector4(1.0, 1.0, 1.0, 1.0));
				Block->SetBodyColor(IsAmber(Index) ? AmberBody : Body);
				Block->SetEnableBorder(true);
				Block->SetBorderWidth(1.0f);
				Block->SetBorderColor(IsAmber(Index) ? AmberBorder : Border);
			}
			// A button, as the wall says: it takes the pointer like any other.
			Tile->AddComponent<UUIButton>();
			if (Wall->HasBegunPlay() && !Tile->HasBegunPlay())
			{
				Tile->BeginPlay();
			}
			Tiles.Add(Tile);
			Blocks.Add(Block);
			TileSpot.Add(Spot);
		}
	}
}

void UNocturnePerfScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturnePerfLocal;
	Super::NativeOnTick(DeltaTime);
	if (!bStarted || Tiles.Num() == 0)
	{
		return;
	}
	const float T = GetScreenTime() - StartTime;

	if (T >= BandFrom && T < AllAt)
	{
		// The band: only the tiles it covers, or has just left, are touched.
		const double Progress = (T - BandFrom) / (BandTo - BandFrom);
		const double BandX = FMath::Lerp(-1000.0, 1000.0, Progress);
		for (int32 Index = 0; Index < Tiles.Num(); ++Index)
		{
			const double Distance = FMath::Abs(TileSpot[Index].X - BandX);
			const bool bIn = Distance < BandHalfWidth;
			if (!bIn && !TileLit[Index])
			{
				continue;
			}
			const float Angle = bIn ? static_cast<float>((1.0 - Distance / BandHalfWidth) * 90.0) : 0.0f;
			if (!FMath::IsNearlyEqual(Angle, TileAngle[Index], 0.05f))
			{
				Tiles[Index]->SetRenderTransformAngle(Angle);
				TileAngle[Index] = Angle;
			}
			if (bIn != TileLit[Index])
			{
				if (UDreamRectBlock* Block = Blocks[Index])
				{
					Block->SetBodyColor(bIn ? LitBody : (IsAmber(Index) ? AmberBody : Body));
				}
				TileLit[Index] = bIn;
			}
		}
	}
	else if (T >= AllAt)
	{
		// Every tile turns, starting in a wave out from the middle.
		const double MaxDistance = FVector2D((Columns - 1) * 0.5 * PitchX, (Rows - 1) * 0.5 * PitchY).Size();
		for (int32 Index = 0; Index < Tiles.Num(); ++Index)
		{
			const float Delay = static_cast<float>(TileSpot[Index].Size() / MaxDistance) * WaveSeconds;
			const float Since = T - AllAt - Delay;
			if (Since <= 0.0f)
			{
				continue;
			}
			if (!TileLit[Index])
			{
				if (UDreamRectBlock* Block = Blocks[Index])
				{
					Block->SetBodyColor(IsAmber(Index) ? AmberBorder : SpinBody);
				}
				TileLit[Index] = true;
			}
			const float Angle = FMath::Fmod(Since * SpinDegreesPerSecond, 360.0f);
			Tiles[Index]->SetRenderTransformAngle(Angle);
			TileAngle[Index] = Angle;
		}
	}

	// The panel's figures, DreamGUI's own, a few times a second.
	if (T >= NextSample)
	{
		NextSample = T + 0.25f;
		const DreamUIRenderStats::FSnapshot Snapshot = DreamUIRenderStats::TakeSnapshot(true);
		if (T >= BandFrom)
		{
			using DreamUIRenderStats::ECounter;
			using DreamUIRenderStats::EStage;
			SetSpec(1, FString::Printf(TEXT("%d / FRAME"), FMath::RoundToInt(Snapshot.GetPerFrame(ECounter::WidgetsUpdated))));
			SetSpec(2, FString::Printf(TEXT("%d / FRAME"), FMath::RoundToInt(Snapshot.GetPerFrame(ECounter::BatchesRecorded))));
			SetSpec(3, FString::Printf(TEXT("%.2f MS"), Snapshot.GetMillisecondsPerFrame(EStage::CanvasUpdate)));
		}
	}

	// The rows light as their figure starts to matter.
	const float RowAt[] = { 0.6f, BandFrom + 0.3f, BandFrom + 1.2f, BandFrom + 2.1f, AllAt + 0.6f };
	for (int32 Index = Lit + 1; Index < UE_ARRAY_COUNT(RowAt); ++Index)
	{
		if (T >= RowAt[Index])
		{
			LightSpec(Index);
			if (Index == 4 && FrameMs > 0.0f)
			{
				// The time on the value's side, the rate in place of the row's English name: "FRAME" and both figures ran together.
				SetSpec(4, FString::Printf(TEXT("%.2f MS"), FrameMs));
				NocturneUI::SetText(Node(TEXT("Spec4En")),
					FText::FromString(FString::Printf(TEXT("%d FPS"), FMath::RoundToInt(1000.0f / FrameMs))));
			}
		}
	}

	// The test suite.
	if (T >= TestsAt)
	{
		UDreamWidget* Right = Node(TEXT("RightPanel"));
		if (!bTestsShown && Right != nullptr)
		{
			bTestsShown = true;
			// The swing first: it settles the panel, which kills every tween on it, the fade among them.
			SwingIn(Right, 0.0f, 0.5f, FRotator(0.0, -58.0, 0.0), 140.0);
			Right->RenderOpacityTo(1.0f, 0.25f, 0.0f, EDreamTweenEase::OutQuad);
			BurstSort(RightSortIndex, 0.6f, 0.4f, 0.1f);
		}
		const float Count = 1.0f - FMath::Pow(1.0f - FMath::Clamp((T - TestsAt - 0.15f) / 1.8f, 0.0f, 1.0f), 3.0f);
		NocturneUI::SetText(Node(TEXT("TestCount")), FText::FromString(FString::Printf(TEXT("%04d"), FMath::RoundToInt(Count * TestCount))));
		if (UDreamWidget* List = Node(TEXT("TestRows")))
		{
			List->SetRenderTranslation(FVector(0.0, 0.0, FMath::Max(0.0f, T - TestsAt - 0.3f) * 70.0f));
		}
	}
}

void UNocturnePerfScreen::LightSpec(int32 InIndex)
{
	using namespace NocturnePerfLocal;
	if (Lit >= 0)
	{
		if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Spec%dBar"), Lit)))
		{
			Bar->RenderOpacityTo(0.0f, 0.2f, 0.0f, EDreamTweenEase::OutQuad);
		}
	}
	Lit = InIndex;
	if (UDreamWidget* Bar = Node(*FString::Printf(TEXT("Spec%dBar"), InIndex)))
	{
		Bar->RenderOpacityTo(1.0f, 0.15f, 0.0f, EDreamTweenEase::OutQuad);
	}
	NocturneUI::SetTextColor(Node(*FString::Printf(TEXT("Spec%dValue"), InIndex)), Cyan);
	NocturneUI::SetTextColor(Node(*FString::Printf(TEXT("Spec%dKey"), InIndex)), Ink);
}

void UNocturnePerfScreen::SetSpec(int32 InIndex, const FString& InValue)
{
	NocturneUI::SetText(Node(*FString::Printf(TEXT("Spec%dValue"), InIndex)), FText::FromString(InValue));
}
