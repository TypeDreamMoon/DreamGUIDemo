// Copyright DreamMoon. All Rights Reserved.

#include "NocturneWheelScreen.h"

#include "NocturneItems.h"
#include "../Settings/NocturneUI.h"

#include "Controls/DreamRingMenu.h"
#include "Core/Components/DreamWidget.h"
#include "Event/DreamKeyEventData.h"
#include "Interaction/UIButton.h"

namespace NocturneWheelLocal
{
	/** The items on the wheel, as indices into NocturneItems::Items, clockwise from twelve o'clock. */
	const int32 WheelItems[] = { 3, 2, 9, 11, 6, 17, 12, 0 };
	// Where the glyphs and names sit from the wheel's centre, design units.
	constexpr double GlyphRadius = 236.0;
	constexpr double NameRadius = 358.0;
	constexpr float SweepSpeed = 18.0f;
	constexpr float FlashSeconds = 0.5f;
	// The wheel comes in lying back (top away) and far, and rises to face the camera.
	constexpr double RisePitch = 70.0;
	constexpr double RiseDepth = 260.0;
	constexpr float RiseSeconds = 0.8f;
	constexpr double FallPitch = 40.0;
	// It leans toward the highlighted item, as if pressed on that side: degrees at the wheel's top and side.
	constexpr double TiltPitch = 10.0;
	constexpr double TiltYaw = 12.0;
	// Its layers in depth, design units (+ further away): the track and sweep behind the wedges, the marks in front.
	constexpr double TrackDepth = 28.0;
	constexpr double SweepDepth = 18.0;
	constexpr double MarksDepth = -30.0;
}

void UNocturneWheelScreen::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	using namespace NocturneWheelLocal;

	// The wheel is a stack: track and sweep a little behind the wedges, glyphs and names floating in front.
	for (const TPair<const TCHAR*, double>& Layer : { TPair<const TCHAR*, double>(TEXT("WheelTrack"), TrackDepth),
		TPair<const TCHAR*, double>(TEXT("WheelSweep"), SweepDepth), TPair<const TCHAR*, double>(TEXT("WheelMarks"), MarksDepth) })
	{
		if (UDreamWidget* Part = Node(Layer.Key))
		{
			Part->SetRenderTranslation(FVector(Layer.Value, 0.0, 0.0));
			GetRest(Part);
		}
	}
	Wheel = Cast<UDreamRingMenu>(Node(TEXT("Wheel")));
	if (UDreamRingMenu* Ring = Wheel.Get())
	{
		TArray<FDreamRingMenuItem> Items;
		for (int32 Index = 0; Index < WheelCount; ++Index)
		{
			FDreamRingMenuItem Item;
			Item.Label = FText::FromString(NocturneItems::Items[WheelItems[Index]].Glyph);
			Item.Tag = FName(*FString::Printf(TEXT("Item%d"), Index));
			Items.Add(Item);
		}
		Ring->SetItems(Items);
		Ring->OnHighlightChanged.AddDynamic(this, &UNocturneWheelScreen::HandleHighlight);
		Ring->OnItemActivated.AddDynamic(this, &UNocturneWheelScreen::HandleActivated);
		// Shut until the screen comes in, so it opens with it.
		Ring->Close();
	}
	for (int32 Index = 0; Index < WheelCount; ++Index)
	{
		const FNocturneItemDef& Def = NocturneItems::Items[WheelItems[Index]];
		NocturneUI::SetText(Node(*FString::Printf(TEXT("WheelGlyph%d"), Index)), FText::FromString(Def.Glyph));
		NocturneUI::SetText(Node(*FString::Printf(TEXT("WheelName%d"), Index)), FText::FromString(Def.Name));
	}
	for (int32 Slot = 0; Slot < QuickCount; ++Slot)
	{
		UDreamWidget* Row = Node(*FString::Printf(TEXT("Quick%d"), Slot));
		UUIButton* Button = Row != nullptr ? Row->GetComponent<UUIButton>() : nullptr;
		if (Button == nullptr)
		{
			continue;
		}
		Button->SetUseFocusRing(false);
		Button->GetOnClickEvent().AddWeakLambda(this, [this, Slot]() { SetQuickTarget(Slot); });
		Button->GetOnSelectionStateChangedEvent().AddWeakLambda(this, [this, Slot](EUISelectableSelectionState InState, bool)
		{
			const bool bOver = InState == EUISelectableSelectionState::Hovered || InState == EUISelectableSelectionState::Pressed;
			HoveredQuick = bOver ? Slot : (HoveredQuick == Slot ? INDEX_NONE : HoveredQuick);
			PaintQuick();
		});
	}
	Describe(0, false);
	PaintQuick();
	PaintMarks();
}

void UNocturneWheelScreen::PlaceMarks()
{
	const UDreamRingMenu* Ring = Wheel.Get();
	if (Ring == nullptr)
	{
		return;
	}
	using namespace NocturneWheelLocal;
	for (int32 Index = 0; Index < WheelCount; ++Index)
	{
		// Degrees clockwise from twelve o'clock; design space is +y up.
		const double Angle = FMath::DegreesToRadians(Ring->GetItemMidAngle(Index));
		const FVector2D Dir(FMath::Sin(Angle), FMath::Cos(Angle));
		if (UDreamWidget* Glyph = Node(*FString::Printf(TEXT("WheelGlyph%d"), Index)))
		{
			Glyph->SetAnchoredPosition(Dir * GlyphRadius);
		}
		if (UDreamWidget* Name = Node(*FString::Printf(TEXT("WheelName%d"), Index)))
		{
			Name->SetAnchoredPosition(Dir * NameRadius);
		}
	}
	bMarksPlaced = true;
}

void UNocturneWheelScreen::NativeOnTick(float DeltaTime)
{
	using namespace NocturneWheelLocal;
	Super::NativeOnTick(DeltaTime);
	if (!bMarksPlaced)
	{
		PlaceMarks();
	}
	if (OpenAt >= 0.0f && GetScreenTime() >= OpenAt)
	{
		OpenAt = -1.0f;
		if (UDreamRingMenu* Ring = Wheel.Get())
		{
			Ring->Open();
		}
	}
	if (UDreamWidget* Sweep = Node(TEXT("WheelSweep")))
	{
		Sweep->SetRenderTransformAngle(-GetScreenTime() * SweepSpeed);
	}
	// The wheel rises to face the camera, leans toward what is highlighted, and tips back on the way out.
	if (UDreamWidget* Holder = Node(TEXT("WheelHolder")))
	{
		const float Now = GetScreenTime();
		const double Rise = RiseAt < 0.0f ? 1.0 : 1.0 - FMath::Pow(1.0 - FMath::Clamp((Now - RiseAt) / RiseSeconds, 0.0f, 1.0f), 3.0);
		const double Fall = FallAt < 0.0f ? 0.0 : FMath::Pow(FMath::Clamp((Now - FallAt) / 0.26f, 0.0f, 1.0f), 3.0);
		FRotator TiltTarget = FRotator::ZeroRotator;
		if (const UDreamRingMenu* Ring = Wheel.Get(); Ring != nullptr && Highlighted >= 0 && Highlighted < WheelCount)
		{
			// Degrees clockwise from twelve o'clock: the side it points to goes back.
			const double Angle = FMath::DegreesToRadians(Ring->GetItemMidAngle(Highlighted));
			TiltTarget = FRotator(-FMath::Cos(Angle) * TiltPitch, -FMath::Sin(Angle) * TiltYaw, 0.0);
		}
		Tilt += (TiltTarget - Tilt) * (1.0f - FMath::Exp(-DeltaTime * 8.0f));
		Holder->SetRenderRotation(FRotator(-RisePitch * (1.0 - Rise) - FallPitch * Fall, 0.0, 0.0) + Tilt);
		Holder->SetRenderTranslation(FVector(RiseDepth * (1.0 - Rise) + 200.0 * Fall, 0.0, 0.0));
	}
	if (GetScreenTime() - FlashAt < FlashSeconds + 0.05f)
	{
		PaintQuick();
	}
}

bool UNocturneWheelScreen::NativeOnKeyDown(UDreamKeyEventData* EventData)
{
	if (EventData != nullptr)
	{
		const FKey& Key = EventData->Key;
		const FKey Numbers[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four };
		for (int32 Slot = 0; Slot < QuickCount; ++Slot)
		{
			if (Key == Numbers[Slot])
			{
				SetQuickTarget(Slot);
				return true;
			}
		}
		if (Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Gamepad_RightShoulder)
		{
			SetQuickTarget((QuickTarget + (Key == EKeys::Gamepad_RightShoulder ? 1 : QuickCount - 1)) % QuickCount);
			return true;
		}
	}
	return Super::NativeOnKeyDown(EventData);
}

void UNocturneWheelScreen::HandleHighlight(int32 Index)
{
	Highlighted = Index;
	if (Index >= 0 && Index < WheelCount && Index != Described)
	{
		Describe(Index, true);
	}
	PaintMarks();
}

void UNocturneWheelScreen::HandleActivated(int32 Index, FName Tag)
{
	if (Index < 0 || Index >= WheelCount)
	{
		return;
	}
	Quick[QuickTarget] = NocturneWheelLocal::WheelItems[Index];
	FlashSlot = QuickTarget;
	FlashAt = GetScreenTime();
	if (UDreamWidget* Glyph = Node(*FString::Printf(TEXT("Quick%dGlyph"), QuickTarget)))
	{
		Settle(Glyph);
		Glyph->SetRenderScale(FVector(1.0, 1.6, 1.6));
		Glyph->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.3f, 0.0f, EDreamTweenEase::OutBack);
	}
	BurstSort(RightSortIndex, 0.45f, 0.25f);
	// The next empty slot lights, or the next one round.
	int32 Next = (QuickTarget + 1) % QuickCount;
	for (int32 Step = 1; Step <= QuickCount; ++Step)
	{
		const int32 Candidate = (QuickTarget + Step) % QuickCount;
		if (Quick[Candidate] == INDEX_NONE)
		{
			Next = Candidate;
			break;
		}
	}
	QuickTarget = Next;
	PaintQuick();
}

void UNocturneWheelScreen::SetQuickTarget(int32 InSlot)
{
	QuickTarget = FMath::Clamp(InSlot, 0, QuickCount - 1);
	PaintQuick();
}

void UNocturneWheelScreen::Describe(int32 InWheelIndex, bool bInAnimate)
{
	using namespace NocturneUI;
	Described = InWheelIndex;
	const FNocturneItemDef& Def = NocturneItems::Items[NocturneWheelLocal::WheelItems[InWheelIndex]];
	SetText(Node(TEXT("ItemGlyph")), FText::FromString(Def.Glyph));
	SetText(Node(TEXT("ItemCount")), FText::FromString(FString::Printf(TEXT("x%02d"), Def.Count)));
	SetText(Node(TEXT("ItemName")), FText::FromString(Def.Name));
	SetText(Node(TEXT("ItemEn")), FText::FromString(Def.En));
	SetText(Node(TEXT("ItemKind")), FText::FromString(FString::Printf(TEXT("%s   ·   %s"), NocturneItems::KindName(Def.Kind), NocturneItems::RarityName(Def.Rarity))));
	SetTextColor(Node(TEXT("ItemKind")), NocturneItems::RarityColor(Def.Rarity));
	SetText(Node(TEXT("ItemText")), FText::FromString(Def.Description));
	if (bInAnimate)
	{
		for (const TCHAR* Part : { TEXT("ItemGlyph"), TEXT("ItemCount"), TEXT("ItemName"), TEXT("ItemEn"), TEXT("ItemKind"), TEXT("ItemText") })
		{
			RunIn(Node(Part), 0.0f, 0.22f, FVector2D(-12.0, 0.0));
		}
		BurstSort(LeftSortIndex, 0.4f, 0.22f);
	}
}

void UNocturneWheelScreen::PaintQuick()
{
	using namespace NocturneUI;
	const bool bFlashing = GetScreenTime() - FlashAt < NocturneWheelLocal::FlashSeconds;
	for (int32 Slot = 0; Slot < QuickCount; ++Slot)
	{
		const bool bTarget = Slot == QuickTarget;
		const bool bOver = Slot == HoveredQuick;
		const bool bFlash = bFlashing && Slot == FlashSlot;
		const int32 Item = Quick[Slot];
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Quick%dBar"), Slot)), bTarget);
		SetTint(Node(*FString::Printf(TEXT("Quick%dBack"), Slot)), bFlash ? FColor(0x8F, 0xE3, 0xFF, 0x30) : (bTarget ? RowLit : (bOver ? RowHover : FColor(0xFF, 0xFF, 0xFF, 0x06))));
		SetTextColor(Node(*FString::Printf(TEXT("Quick%dKey"), Slot)), bTarget ? Cyan : InkFaint);
		SetText(Node(*FString::Printf(TEXT("Quick%dGlyph"), Slot)), FText::FromString(Item != INDEX_NONE ? NocturneItems::Items[Item].Glyph : TEXT("")));
		SetText(Node(*FString::Printf(TEXT("Quick%dName"), Slot)), FText::FromString(Item != INDEX_NONE ? NocturneItems::Items[Item].Name : TEXT("空  ·  EMPTY")));
		SetTextColor(Node(*FString::Printf(TEXT("Quick%dName"), Slot)), Item != INDEX_NONE ? (bTarget ? InkBright : Ink) : InkFaint);
		SetTextColor(Node(*FString::Printf(TEXT("Quick%dGlyph"), Slot)), bFlash ? Cyan : Ink);
	}
}

void UNocturneWheelScreen::PaintMarks()
{
	using namespace NocturneUI;
	for (int32 Index = 0; Index < WheelCount; ++Index)
	{
		const bool bOn = Index == Highlighted;
		SetTextColor(Node(*FString::Printf(TEXT("WheelGlyph%d"), Index)), bOn ? InkBright : Ink);
		SetTextColor(Node(*FString::Printf(TEXT("WheelName%d"), Index)), bOn ? Cyan : InkDim);
	}
}

void UNocturneWheelScreen::PlayContentEntrance(float Delay)
{
	OpenAt = GetScreenTime() + Delay;
	// The rise starts as the wedges open, so it is seen: started earlier, it is nearly over before there is a wheel.
	RiseAt = GetScreenTime() + Delay + 0.02f;
	FallAt = -1.0f;
	// The wheel's own rise is the 3D move here: its rings and marks only fade up.
	RunIn(Node(TEXT("WheelMarks")), Delay + 0.15f, 0.35f, FVector2D::ZeroVector, 0.0f);
	RunIn(Node(TEXT("WheelTrack")), Delay, 0.4f, FVector2D::ZeroVector, 0.0f);
	RunIn(Node(TEXT("WheelSweep")), Delay + 0.1f, 0.4f, FVector2D::ZeroVector, 0.0f);
	RunInChildren(Node(TEXT("ItemBody")), Delay + 0.05f, 0.03f, FVector2D(-16.0, 0.0));
	RunInChildren(Node(TEXT("QuickBody")), Delay + 0.1f, 0.04f, FVector2D(16.0, 0.0));
}

void UNocturneWheelScreen::PlayExit()
{
	Super::PlayExit();
	FallAt = GetScreenTime();
}
