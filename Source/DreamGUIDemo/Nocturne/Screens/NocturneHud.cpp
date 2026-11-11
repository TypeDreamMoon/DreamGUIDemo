// Copyright DreamMoon. All Rights Reserved.

#include "NocturneHud.h"

#include "NocturneItems.h"
#include "NocturneLens.h"
#include "../Settings/NocturneUI.h"

#include "Controls/DreamProgressBar.h"
#include "Core/Components/DreamWidget.h"

namespace NocturneHudLocal
{
	// The heading strip: 640 wide, four design units a degree, a tick every ten degrees, a letter every forty-five.
	constexpr double PixelsPerDegree = 4.0;
	constexpr double HalfStrip = 320.0;
	constexpr int32 TickCount = 36;
	constexpr int32 LetterCount = 8;

	// How long a toast stays, how long it takes in and out; how long a damage number floats; how long the banner stays.
	constexpr float ToastSeconds = 3.2f;
	constexpr float ToastFade = 0.3f;
	constexpr float DamageSeconds = 1.1f;
	constexpr float FlashSeconds = 0.45f;
	constexpr float BannerSeconds = 2.4f;

	double Wrap180(double Degrees)
	{
		Degrees = FMath::Fmod(Degrees + 180.0, 360.0);
		return Degrees < 0.0 ? Degrees + 180.0 : Degrees - 180.0;
	}
}

void UNocturneHud::SetNodeText(const TCHAR* Name, const FString& Text) const
{
	NocturneUI::SetText(Node(Name), FText::FromString(Text));
}

void UNocturneHud::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	for (int32 Index = 0; Index < ToastCount; ++Index)
	{
		if (UDreamWidget* Toast = Node(*FString::Printf(TEXT("Toast%d"), Index)))
		{
			Toast->SetRenderOpacity(0.0f);
		}
	}
	for (int32 Index = 0; Index < DamageCount; ++Index)
	{
		if (UDreamWidget* Damage = Node(*FString::Printf(TEXT("Dmg%d"), Index)))
		{
			Damage->SetRenderOpacity(0.0f);
		}
	}
	for (int32 Index = 0; Index < MarkerCount; ++Index)
	{
		NocturneUI::SetShown(Node(*FString::Printf(TEXT("Marker%d"), Index)), false);
	}
	if (UDreamWidget* Banner = Node(TEXT("Banner")))
	{
		Banner->SetRenderOpacity(0.0f);
	}
	SetHull(1.0f);
	SetEnergy(0.64f);
	PlaceCompass(0.0f);
}

void UNocturneHud::PlayBoot(int32 InSlot)
{
	SetNodeText(TEXT("BannerEn"), FString::Printf(TEXT("VOYAGE START   ·   SLOT %02d"), InSlot + 1));
	BannerAt = GetScreenTime() + 0.5f;
	PlayEntrance();
}

void UNocturneHud::PlayContentEntrance(float Delay)
{
	// The visor deploys: the side clusters swing round from edge-on on their outer edges to their curve, the heading
	// strip tips down from the top, and their rows flip up inside them.
	SwingIn(Node(TEXT("Objective")), Delay, 0.6f, FRotator(0.0, -52.0, 0.0), 0.0);
	SwingIn(Node(TEXT("Status")), Delay + 0.08f, 0.6f, FRotator(0.0, -52.0, 0.0), 0.0);
	SwingIn(Node(TEXT("Flight")), Delay + 0.08f, 0.6f, FRotator(0.0, 52.0, 0.0), 0.0);
	SwingIn(Node(TEXT("Compass")), Delay + 0.04f, 0.55f, FRotator(58.0, 0.0, 0.0), 0.0);
	RunInChildren(Node(TEXT("Objective")), Delay + 0.1f, 0.05f, FVector2D(-20.0, 0.0));
	RunInChildren(Node(TEXT("Status")), Delay + 0.2f, 0.05f, FVector2D(-20.0, 0.0));
	RunInChildren(Node(TEXT("Flight")), Delay + 0.2f, 0.05f, FVector2D(20.0, 0.0));
	if (UDreamWidget* Compass = Node(TEXT("Compass")))
	{
		Compass->SetRenderOpacity(0.0f);
		Compass->RenderOpacityTo(1.0f, 0.25f, Delay + 0.04f, EDreamTweenEase::OutQuad);
	}
	if (UDreamWidget* Reticle = Node(TEXT("Reticle")))
	{
		Settle(Reticle);
		Reticle->SetRenderScale(FVector(1.0, 1.8, 1.8));
		Reticle->SetRenderOpacity(0.0f);
		Reticle->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.4f, Delay, EDreamTweenEase::OutCubic);
		Reticle->RenderOpacityTo(1.0f, 0.3f, Delay, EDreamTweenEase::OutCubic);
	}
	BurstSort(CenterSortIndex, 0.7f, 0.5f, Delay);
}

void UNocturneHud::NativeOnTick(float DeltaTime)
{
	using namespace NocturneHudLocal;
	Super::NativeOnTick(DeltaTime);
	const float Now = GetScreenTime();

	// The banner fades in, holds, fades out.
	if (UDreamWidget* Banner = Node(TEXT("Banner")))
	{
		const float Since = Now - BannerAt;
		const float In = FMath::Clamp(Since / 0.35f, 0.0f, 1.0f);
		const float Out = FMath::Clamp((BannerSeconds - Since) / 0.5f, 0.0f, 1.0f);
		Banner->SetRenderOpacity(Since < 0.0f ? 0.0f : FMath::Min(In, Out));
	}

	// Toasts slide in from the right, wait, and fade.
	for (int32 Index = 0; Index < ToastCount; ++Index)
	{
		UDreamWidget* Toast = Node(*FString::Printf(TEXT("Toast%d"), Index));
		if (Toast == nullptr)
		{
			continue;
		}
		const float Since = Now - Toasts[Index].Shown;
		if (Since < 0.0f || Since > ToastSeconds)
		{
			Toast->SetRenderOpacity(0.0f);
			continue;
		}
		const float In = FMath::Clamp(Since / ToastFade, 0.0f, 1.0f);
		const float Out = FMath::Clamp((ToastSeconds - Since) / ToastFade, 0.0f, 1.0f);
		Toast->SetRenderOpacity(FMath::Min(In, Out));
		const double Ease = 1.0 - FMath::Pow(1.0 - In, 3.0);
		SetOffset(Toast, FVector2D(60.0 * (1.0 - Ease), 0.0));
	}

	// Damage numbers rise and fade off the reticle.
	for (int32 Index = 0; Index < DamageCount; ++Index)
	{
		UDreamWidget* Damage = Node(*FString::Printf(TEXT("Dmg%d"), Index));
		if (Damage == nullptr)
		{
			continue;
		}
		const float Since = Now - Damages[Index].Shown;
		if (Since < 0.0f || Since > DamageSeconds)
		{
			Damage->SetRenderOpacity(0.0f);
			continue;
		}
		const float Alpha = Since / DamageSeconds;
		Damage->SetAnchoredPosition(Damages[Index].From + FVector2D(10.0 * Alpha, 70.0 * (1.0 - FMath::Pow(1.0 - Alpha, 2.0))));
		Damage->SetRenderOpacity(FMath::Clamp((1.0f - Alpha) * 1.6f, 0.0f, 1.0f));
		// ...and come at the camera as they go.
		Damage->SetRenderTranslation(FVector(-240.0 * Alpha, 0.0, 0.0));
		const double Pop = 1.0 + 0.4 * FMath::Max(0.0, 1.0 - Since / 0.15);
		Damage->SetRenderScale(FVector(1.0, Pop, Pop));
	}

	// The sweep: the block grows across the line from the left (180 ms), the line changes under it, the block leaves
	// to the right (220 ms).
	if (SweepAt >= 0.0f)
	{
		constexpr double Width = 400.0;
		const float Since = Now - SweepAt;
		double Left = 0.0;
		double Size = 0.0;
		if (Since < 0.18f)
		{
			Size = Width * (1.0 - FMath::Pow(1.0 - Since / 0.18, 3.0));
		}
		else
		{
			if (!bSweepSwapped)
			{
				bSweepSwapped = true;
				ApplyObjectiveDone();
			}
			const double Out = FMath::Clamp((Since - 0.18) / 0.22, 0.0, 1.0);
			Left = Width * Out * Out;
			Size = Width - Left;
		}
		if (UDreamWidget* Sweep = Node(TEXT("ObjSweep")))
		{
			NocturneUI::SetShown(Sweep, Size > 0.5);
			Sweep->SetAnchoredPosition(FVector2D(16.0 + Left, -80.0));
			Sweep->SetSizeDelta(FVector2D(Size, 22.0));
		}
		if (Since > 0.4f)
		{
			SweepAt = -1.0f;
		}
	}

	if (UDreamWidget* Flash = Node(TEXT("HitFlash")))
	{
		const float Since = Now - FlashAt;
		Flash->SetRenderOpacity(Since >= 0.0f && Since < FlashSeconds ? 1.0f - Since / FlashSeconds : 0.0f);
	}
}

void UNocturneHud::SetFlight(float InSpeed, float InHeading, const FVector& InPosition, float InThrust)
{
	const int32 Speed = FMath::RoundToInt(InSpeed);
	if (Speed != ShownSpeed)
	{
		ShownSpeed = Speed;
		SetNodeText(TEXT("SpeedValue"), FString::Printf(TEXT("%04d"), Speed));
	}
	PlaceCompass(InHeading);
	const FString Coords = FString::Printf(TEXT("X %05d   Y %05d   Z %04d"), FMath::RoundToInt(InPosition.X), FMath::RoundToInt(InPosition.Y), FMath::RoundToInt(InPosition.Z));
	if (Coords != ShownCoords)
	{
		ShownCoords = Coords;
		SetNodeText(TEXT("Coords"), Coords);
	}
	if (UDreamProgressBar* Thrust = Cast<UDreamProgressBar>(Node(TEXT("ThrustBar"))))
	{
		Thrust->SetPercent(InThrust);
	}
}

void UNocturneHud::PlaceCompass(float InHeading)
{
	using namespace NocturneHudLocal;
	const int32 Rounded = (FMath::RoundToInt(InHeading) % 360 + 360) % 360;
	if (Rounded != ShownHeading)
	{
		ShownHeading = Rounded;
		SetNodeText(TEXT("HeadingValue"), FString::Printf(TEXT("HDG  %03d"), Rounded));
	}
	for (int32 Index = 0; Index < TickCount; ++Index)
	{
		if (UDreamWidget* Tick = Node(*FString::Printf(TEXT("Tick%d"), Index)))
		{
			const double Offset = Wrap180(Index * 10.0 - InHeading) * PixelsPerDegree;
			Tick->SetAnchoredPosition(FVector2D(Offset, 0.0));
			Tick->SetSizeDelta(FVector2D(1.0, Index % 3 == 0 ? 12.0 : 7.0));
			NocturneUI::SetShown(Tick, FMath::Abs(Offset) < HalfStrip);
		}
	}
	for (int32 Index = 0; Index < LetterCount; ++Index)
	{
		if (UDreamWidget* Letter = Node(*FString::Printf(TEXT("Head%d"), Index)))
		{
			const double Offset = Wrap180(Index * 45.0 - InHeading) * PixelsPerDegree;
			Letter->SetAnchoredPosition(FVector2D(Offset, -14.0));
			NocturneUI::SetShown(Letter, FMath::Abs(Offset) < HalfStrip - 20.0);
		}
	}
}

void UNocturneHud::SetHull(float InFraction)
{
	// The lens keeps a little glitch the more the hull is damaged.
	if (UNocturneLens* Lens = UNocturneLens::Get())
	{
		Lens->SetDamage(1.0f - FMath::Clamp(InFraction, 0.0f, 1.0f));
	}
	Hull = FMath::Clamp(InFraction, 0.0f, 1.0f);
	SetNodeText(TEXT("HullValue"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Hull * 100.0f)));
	if (UDreamProgressBar* Bar = Cast<UDreamProgressBar>(Node(TEXT("HullBar"))))
	{
		Bar->SetPercent(Hull);
		Bar->SetFillColorAndOpacity(Hull < 0.75f ? NocturneUI::Amber : NocturneUI::Cyan);
	}
	NocturneUI::SetTextColor(Node(TEXT("HullValue")), Hull < 0.75f ? NocturneUI::Amber : NocturneUI::Cyan);
}

void UNocturneHud::SetEnergy(float InFraction)
{
	const float Energy = FMath::Clamp(InFraction, 0.0f, 1.0f);
	SetNodeText(TEXT("EnergyValue"), FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Energy * 100.0f)));
	if (UDreamProgressBar* Bar = Cast<UDreamProgressBar>(Node(TEXT("EnergyBar"))))
	{
		Bar->SetPercent(Energy);
	}
}

void UNocturneHud::SetMarker(int32 InIndex, bool bInShown, const FVector2D& InPosition, const FString& InName, float InKilometres)
{
	UDreamWidget* Marker = Node(*FString::Printf(TEXT("Marker%d"), InIndex));
	if (Marker == nullptr)
	{
		return;
	}
	// Nothing is bracketed while the banner is up: the brackets lock on once it has gone.
	const bool bShown = bInShown && GetScreenTime() >= BannerAt + NocturneHudLocal::BannerSeconds - 0.2f + 0.25f * InIndex;
	NocturneUI::SetShown(Marker, bShown);
	if (!bShown)
	{
		return;
	}
	if (!MarkerLocked[InIndex])
	{
		MarkerLocked[InIndex] = true;
		Settle(Marker);
		Marker->SetRenderScale(FVector(1.0, 1.7, 1.7));
		Marker->SetRenderOpacity(0.0f);
		Marker->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.3f, 0.0f, EDreamTweenEase::OutCubic);
		Marker->RenderOpacityTo(1.0f, 0.2f, 0.0f, EDreamTweenEase::OutCubic);
	}
	Marker->SetAnchoredPosition(InPosition);
	SetNodeText(*FString::Printf(TEXT("Marker%dName"), InIndex), InName);
	SetNodeText(*FString::Printf(TEXT("Marker%dDist"), InIndex), FString::Printf(TEXT("%.2f KM"), InKilometres));
}

void UNocturneHud::SetObjectiveDistance(float InKilometres)
{
	SetNodeText(TEXT("ObjectiveDist"), FString::Printf(TEXT("%.2f KM"), InKilometres));
}

void UNocturneHud::ShowToast(int32 InItem, int32 InCount)
{
	if (InItem < 0 || InItem >= NocturneItems::Count)
	{
		return;
	}
	// The newest on top: the others step down a place.
	const int32 Slot = NextToast;
	NextToast = (NextToast + 1) % ToastCount;
	const FNocturneItemDef& Def = NocturneItems::Items[InItem];
	SetNodeText(*FString::Printf(TEXT("Toast%dGlyph"), Slot), Def.Glyph);
	SetNodeText(*FString::Printf(TEXT("Toast%dName"), Slot), Def.Name);
	SetNodeText(*FString::Printf(TEXT("Toast%dCount"), Slot), FString::Printf(TEXT("x%d"), InCount));
	NocturneUI::SetTint(Node(*FString::Printf(TEXT("Toast%dBar"), Slot)), NocturneItems::RarityColor(Def.Rarity));
	for (int32 Index = 0; Index < ToastCount; ++Index)
	{
		if (UDreamWidget* Toast = Node(*FString::Printf(TEXT("Toast%d"), Index)))
		{
			// Order by age: the newest at the top.
			const int32 Age = (Slot - Index + ToastCount) % ToastCount;
			Toast->AnchoredPositionTo(FVector2D(0.0, -70.0 * Age), 0.25f, 0.0f, EDreamTweenEase::OutCubic);
		}
	}
	Toasts[Slot].Shown = GetScreenTime();
	if (UDreamWidget* Toast = Node(*FString::Printf(TEXT("Toast%d"), Slot)))
	{
		Toast->SetAnchoredPosition(FVector2D(0.0, 0.0));
	}
}

void UNocturneHud::Hit(int32 InDamage)
{
	FlashAt = GetScreenTime();
	const int32 Slot = NextDamage;
	NextDamage = (NextDamage + 1) % DamageCount;
	SetNodeText(*FString::Printf(TEXT("Dmg%d"), Slot), FString::Printf(TEXT("-%d"), InDamage));
	// Off the reticle's upper right, a little different each time.
	Damages[Slot].From = FVector2D(70.0 + 26.0 * Slot, 30.0 - 18.0 * Slot);
	Damages[Slot].Shown = GetScreenTime();
	SetHull(Hull - InDamage / 100.0f);
	// The whole visor jolts in 3D and the lens glitches and splits.
	Shake(0.6f + InDamage / 25.0f);
	UNocturneLens::Pulse(ENocturneLensPulse::Hit, FMath::Clamp(0.5f + InDamage / 25.0f, 0.0f, 1.2f));
	if (UDreamWidget* Reticle = Node(TEXT("Reticle")))
	{
		Settle(Reticle);
		SetOffset(Reticle, FVector2D(8.0, -5.0));
		Reticle->RenderOffsetTo(FVector2D::ZeroVector, 0.3f, 0.0f, EDreamTweenEase::OutElastic);
	}
	BurstSort(CenterSortIndex, 0.35f, 0.2f);
}

void UNocturneHud::CompleteObjective()
{
	SweepAt = GetScreenTime();
	bSweepSwapped = false;
}

void UNocturneHud::ApplyObjectiveDone()
{
	NocturneUI::SetTint(Node(TEXT("Obj0Box")), NocturneUI::Cyan);
	NocturneUI::SetTextColor(Node(TEXT("Obj0Text")), NocturneUI::InkFaint);
	SetNodeText(TEXT("Obj0Count"), TEXT("1 / 1"));
	NocturneUI::SetTextColor(Node(TEXT("Obj1Text")), NocturneUI::Ink);
	if (UDreamWidget* Box = Node(TEXT("Obj0Box")))
	{
		Box->SetRenderScale(FVector(1.0, 1.8, 1.8));
		Box->RenderScaleTo(FVector(1.0, 1.0, 1.0), 0.3f, 0.0f, EDreamTweenEase::OutBack);
	}
}
