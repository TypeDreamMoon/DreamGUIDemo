// Copyright DreamMoon. All Rights Reserved.

#include "P3RCommuRow.h"

#include "P3RUI.h"

#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Extensions/DreamMaskTransition.h"

namespace
{
	/** The chosen look is wiped on (or off) through the Chosen node's mask transition. */
	void WipeChosen(UDreamWidget* Chosen, bool bOn, bool bInstant)
	{
		UDreamMaskTransition* Mask = Chosen != nullptr ? UDreamMaskTransition::FindOrAdd(Chosen) : nullptr;
		if (Mask == nullptr)
		{
			return;
		}
		Mask->Stop();
		if (bInstant)
		{
			Mask->SetProgress(bOn ? 1.0f : 0.0f);
		}
		else if (bOn)
		{
			Mask->PlayFromTo(0.0f, 1.0f, 0.11f, EDreamTweenEase::OutCubic);
		}
		else
		{
			Mask->PlayFromTo(Mask->GetProgress(), 0.0f, 0.08f, EDreamTweenEase::OutCubic);
		}
	}
}

namespace P3RCommuRowLocal
{
	const FColor Ink(0x0A, 0x14, 0x40);
	const FColor Face(0x8E, 0xEA, 0xF4);
	const FColor Strip(0xC8, 0xF7, 0xFB);
	const FColor StripChosen(0xE6, 0xE9, 0xF2);
}

UDreamWidget* UP3RCommuRow::Find(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

void UP3RCommuRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	bChosen = true;
	SetChosen(false, /*bInstant*/true);
}

void UP3RCommuRow::SetData(const FString& InNumeral, const FString& InArcana, int32 InRank, const FString& InPartner)
{
	P3RUI::SetText(Find(TEXT("Numeral")), FText::FromString(InNumeral));
	P3RUI::SetText(Find(TEXT("Arcana")), FText::FromString(InArcana));
	const bool bMax = InRank >= 10;
	P3RUI::SetShown(Find(TEXT("RankTag")), !bMax);
	P3RUI::SetText(Find(TEXT("RankValue")), FText::FromString(bMax ? FString(TEXT("MAX")) : FString::FromInt(InRank)));
	P3RUI::SetText(Find(TEXT("Partner")), FText::FromString(InPartner));
}

void UP3RCommuRow::SetChosen(bool bInChosen, bool bInstant)
{
	using namespace P3RCommuRowLocal;
	if (bChosen == bInChosen)
	{
		return;
	}
	bChosen = bInChosen;
	WipeChosen(Find(TEXT("Chosen")), bChosen, bInstant);
	UWorld* World = GetWorld();
	if (bInstant || World == nullptr)
	{
		ApplyColours();
		return;
	}
	World->GetTimerManager().SetTimer(ColourTimer, FTimerDelegate::CreateWeakLambda(this, [this]() { ApplyColours(); }), bChosen ? 0.05f : 0.03f, false);
}

void UP3RCommuRow::ApplyColours()
{
	using namespace P3RCommuRowLocal;
	P3RUI::SetTint(Find(TEXT("BadgeFill")), bChosen ? P3RUI::Black : P3RUI::White);
	P3RUI::SetTextColor(Find(TEXT("Numeral")), bChosen ? P3RUI::White : P3RUI::Black);
	for (const TCHAR* Name : { TEXT("Arcana"), TEXT("RankTag"), TEXT("RankValue"), TEXT("Partner") })
	{
		P3RUI::SetTextColor(Find(Name), bChosen ? P3RUI::Black : Ink);
	}
}
