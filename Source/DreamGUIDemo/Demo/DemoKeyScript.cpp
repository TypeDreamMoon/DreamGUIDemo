// Copyright DreamMoon. All Rights Reserved.

#include "DemoKeyScript.h"

#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"

DEFINE_LOG_CATEGORY_STATIC(LogDemoKeys, Log, All);

namespace
{
	bool GetScript(FString& Out)
	{
		return FParse::Value(FCommandLine::Get(), TEXT("-DemoKeys="), Out, false) && !Out.IsEmpty();
	}
}

bool UDemoKeyScript::ShouldCreateSubsystem(UObject* Outer) const
{
	const UWorld* World = Cast<UWorld>(Outer);
	FString Script;
	return World != nullptr && World->IsGameWorld() && GetScript(Script);
}

void UDemoKeyScript::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	FString Script;
	GetScript(Script);
	TArray<FString> Entries;
	Script.ParseIntoArray(Entries, TEXT(","));
	for (const FString& Entry : Entries)
	{
		// time:Key or time:Key/hold
		FString TimeText, Rest, KeyName, HoldText;
		if (!Entry.Split(TEXT(":"), &TimeText, &Rest))
		{
			UE_LOG(LogDemoKeys, Warning, TEXT("Skipping '%s': no time"), *Entry);
			continue;
		}
		if (!Rest.Split(TEXT("/"), &KeyName, &HoldText))
		{
			KeyName = Rest;
		}
		const FKey Key(*KeyName.TrimStartAndEnd());
		if (!Key.IsValid())
		{
			UE_LOG(LogDemoKeys, Warning, TEXT("Skipping '%s': no key named '%s'"), *Entry, *KeyName);
			continue;
		}
		const double Time = FCString::Atod(*TimeText);
		// No hold: up on the frame after the press (a later step at the same time sorts after the press).
		const double Hold = HoldText.IsEmpty() ? 0.0 : FCString::Atod(*HoldText);
		Steps.Add({ Time, Key, true });
		Steps.Add({ Time + Hold, Key, false });
	}
	Steps.StableSort([](const FStep& A, const FStep& B) { return A.Time < B.Time; });
	bPlaying = Steps.Num() > 0;
	UE_LOG(LogDemoKeys, Display, TEXT("%d key presses scripted"), Steps.Num() / 2);
}

void UDemoKeyScript::Tick(float DeltaTime)
{
	if (!bPlaying || !FSlateApplication::IsInitialized())
	{
		return;
	}
	Clock += DeltaTime;
	// One step per key per frame: an up never lands on the frame of its own down.
	TSet<FKey> SentThisFrame;
	while (Next < Steps.Num() && Steps[Next].Time <= Clock)
	{
		if (SentThisFrame.Contains(Steps[Next].Key))
		{
			break;
		}
		SentThisFrame.Add(Steps[Next].Key);
		Send(Steps[Next]);
		++Next;
	}
	bPlaying = Next < Steps.Num();
}

void UDemoKeyScript::Send(const FStep& Step) const
{
	FSlateApplication& Slate = FSlateApplication::Get();
	const FKeyEvent Event(Step.Key, FModifierKeysState(), 0, false, 0, 0);
	const bool bHandled = Step.bDown ? Slate.ProcessKeyDownEvent(Event) : Slate.ProcessKeyUpEvent(Event);
	UE_LOG(LogDemoKeys, Display, TEXT("%.2f s %s %s%s"), Clock, *Step.Key.ToString(), Step.bDown ? TEXT("down") : TEXT("up"),
		bHandled ? TEXT(" (handled)") : TEXT(""));
}

TStatId UDemoKeyScript::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDemoKeyScript, STATGROUP_Tickables);
}
