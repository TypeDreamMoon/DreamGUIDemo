// Copyright DreamMoon. All Rights Reserved.

#include "P3RKeyScript.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogP3RKeys, Log, All);

namespace P3RKeyScriptLocal
{
	bool GetScript(FString& Out)
	{
		return FParse::Value(FCommandLine::Get(), TEXT("-P3RKeys="), Out, false) && !Out.IsEmpty();
	}
}

bool UP3RKeyScript::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	FString Script;
	return World != nullptr && World->IsGameWorld() && P3RKeyScriptLocal::GetScript(Script);
}

void UP3RKeyScript::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	FString Script;
	P3RKeyScriptLocal::GetScript(Script);
	TArray<FString> Entries;
	Script.ParseIntoArray(Entries, TEXT(","));
	for (const FString& Entry : Entries)
	{
		FString TimeText, Rest, KeyName, HoldText;
		if (!Entry.Split(TEXT(":"), &TimeText, &Rest))
		{
			UE_LOG(LogP3RKeys, Warning, TEXT("Skipping '%s': no time"), *Entry);
			continue;
		}
		// The pointer: Move@X;Y or Click@X;Y.
		FString Verb, Where, XText, YText;
		if (Rest.Split(TEXT("@"), &Verb, &Where) && Where.Split(TEXT(";"), &XText, &YText))
		{
			FStep Step;
			Step.Time = FCString::Atod(*TimeText);
			Step.Kind = Verb.TrimStartAndEnd().Equals(TEXT("Click"), ESearchCase::IgnoreCase) ? EKind::Click : EKind::Move;
			Step.Point = FVector2D(FCString::Atod(*XText), FCString::Atod(*YText));
			Steps.Add(Step);
			continue;
		}
		if (!Rest.Split(TEXT("/"), &KeyName, &HoldText))
		{
			KeyName = Rest;
		}
		const FKey Key(*KeyName.TrimStartAndEnd());
		if (!Key.IsValid())
		{
			UE_LOG(LogP3RKeys, Warning, TEXT("Skipping '%s': no key named '%s'"), *Entry, *KeyName);
			continue;
		}
		const double Time = FCString::Atod(*TimeText);
		const double Hold = HoldText.IsEmpty() ? 0.0 : FCString::Atod(*HoldText);
		FStep Down;
		Down.Time = Time;
		Down.Key = Key;
		FStep Up = Down;
		Up.Time = Time + Hold;
		Up.bDown = false;
		Steps.Add(Down);
		Steps.Add(Up);
	}
	Steps.StableSort([](const FStep& A, const FStep& B) { return A.Time < B.Time; });
	bPlaying = Steps.Num() > 0;
	UE_LOG(LogP3RKeys, Display, TEXT("%d steps scripted"), Steps.Num());
}

void UP3RKeyScript::Tick(float DeltaTime)
{
	if (!bPlaying || !FSlateApplication::IsInitialized())
	{
		return;
	}
	Clock += DeltaTime;
	// A glide: the pointer eases to its target over 0.15 s, a move event every frame, as a hand moves a mouse.
	if (GlideAt >= 0.0)
	{
		const double K = FMath::Clamp((Clock - GlideAt) / 0.15, 0.0, 1.0);
		MovePointer(FMath::Lerp(GlideFrom, GlideTo, 1.0 - FMath::Pow(1.0 - K, 3.0)));
		if (K >= 1.0)
		{
			GlideAt = -1.0;
		}
	}
	// One step per key per frame: an up never lands on the frame of its own down.
	TSet<FKey> SentThisFrame;
	while (Next < Steps.Num() && Steps[Next].Time <= Clock)
	{
		if (Steps[Next].Kind == EKind::Key)
		{
			if (SentThisFrame.Contains(Steps[Next].Key))
			{
				break;
			}
			SentThisFrame.Add(Steps[Next].Key);
		}
		Send(Steps[Next]);
		++Next;
	}
	bPlaying = Next < Steps.Num() || GlideAt >= 0.0;
}

FVector2D UP3RKeyScript::ToScreen(const FVector2D& Design) const
{
	// The game window's client area, in screen space; the design frame is stretched over it.
	if (GEngine != nullptr && GEngine->GameViewport != nullptr)
	{
		if (const TSharedPtr<SWindow> Window = GEngine->GameViewport->GetWindow())
		{
			const FGeometry Geometry = Window->GetWindowGeometryInScreen();
			const FVector2D Size = Geometry.GetAbsoluteSize();
			return Geometry.GetAbsolutePosition() + Design / FVector2D(1920.0, 1080.0) * Size;
		}
	}
	return Design;
}

void UP3RKeyScript::MovePointer(const FVector2D& Design)
{
	FSlateApplication& Slate = FSlateApplication::Get();
	const FVector2D From = ToScreen(Pointer);
	const FVector2D To = ToScreen(Design);
	Pointer = Design;
	Slate.SetCursorPos(To);
	const FPointerEvent Event(Slate.CursorPointerIndex, To, From, TSet<FKey>(), EKeys::Invalid, 0.0f, FModifierKeysState());
	Slate.ProcessMouseMoveEvent(Event);
}

void UP3RKeyScript::Send(const FStep& Step)
{
	FSlateApplication& Slate = FSlateApplication::Get();
	if (Step.Kind == EKind::Move)
	{
		GlideFrom = Pointer;
		GlideTo = Step.Point;
		GlideAt = Clock;
		UE_LOG(LogP3RKeys, Display, TEXT("%.2f s pointer to %.0f, %.0f"), Clock, Step.Point.X, Step.Point.Y);
		return;
	}
	if (Step.Kind == EKind::Click)
	{
		GlideAt = -1.0;
		MovePointer(Step.Point);
		const FVector2D At = ToScreen(Step.Point);
		TSet<FKey> Pressed;
		Pressed.Add(EKeys::LeftMouseButton);
		const FPointerEvent Down(Slate.CursorPointerIndex, At, At, Pressed, EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
		const FPointerEvent Up(Slate.CursorPointerIndex, At, At, TSet<FKey>(), EKeys::LeftMouseButton, 0.0f, FModifierKeysState());
		const bool bDown = Slate.ProcessMouseButtonDownEvent(nullptr, Down);
		const bool bUp = Slate.ProcessMouseButtonUpEvent(Up);
		UE_LOG(LogP3RKeys, Display, TEXT("%.2f s click at %.0f, %.0f%s"), Clock, Step.Point.X, Step.Point.Y, bDown || bUp ? TEXT(" (handled)") : TEXT(""));
		return;
	}
	const FKeyEvent Event(Step.Key, FModifierKeysState(), 0, false, 0, 0);
	const bool bHandled = Step.bDown ? Slate.ProcessKeyDownEvent(Event) : Slate.ProcessKeyUpEvent(Event);
	UE_LOG(LogP3RKeys, Display, TEXT("%.2f s %s %s%s"), Clock, *Step.Key.ToString(), Step.bDown ? TEXT("down") : TEXT("up"),
		bHandled ? TEXT(" (handled)") : TEXT(""));
}

TStatId UP3RKeyScript::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UP3RKeyScript, STATGROUP_Tickables);
}
