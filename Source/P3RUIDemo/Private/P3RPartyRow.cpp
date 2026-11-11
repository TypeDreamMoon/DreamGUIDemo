// Copyright DreamMoon. All Rights Reserved.

#include "P3RPartyRow.h"

#include "P3RUI.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamWidgetTree.h"
#include "Engine/Texture2D.h"
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

namespace P3RPartyRowLocal
{
	const FColor Band(0x07, 0x08, 0x0F);
	const FColor BandChosen(0xF6, 0xF7, 0xFB);
	const FColor InkChosen(0x0A, 0x14, 0x40);
	const FColor HpChosen(0x00, 0xA6, 0xD6);
	const FColor SpChosen(0xD8, 0xA8, 0x00);
}

UDreamWidget* UP3RPartyRow::Find(const TCHAR* Name) const
{
	const UDreamWidgetTree* Tree = GetWidgetTree();
	return Tree != nullptr ? Tree->FindWidgetByVariableName(FName(Name)) : nullptr;
}

void UP3RPartyRow::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	bChosen = true;
	SetChosen(false, /*bInstant*/true);
}

void UP3RPartyRow::SetMember(const FP3RMember& InMember, UTexture2D* InFace)
{
	const bool bRole = FCString::Strlen(InMember.Role) > 0;
	P3RUI::SetShown(Find(TEXT("Role")), bRole);
	P3RUI::SetText(Find(TEXT("Role")), FText::FromString(InMember.Role));
	P3RUI::SetText(Find(TEXT("Name")), FText::FromString(InMember.Name));
	P3RUI::SetText(Find(TEXT("Lv")), FText::AsNumber(InMember.Level));
	P3RUI::SetText(Find(TEXT("Hp")), FText::FromString(FString::FromInt(InMember.Hp)));
	P3RUI::SetText(Find(TEXT("Sp")), FText::FromString(FString::FromInt(InMember.Sp)));
	P3RUI::SetTint(Find(TEXT("Window")), InMember.Colour);
	P3RUI::SetTint(Find(TEXT("Slash")), InMember.Colour);
	P3RUI::SetScale2D(Find(TEXT("HpBar")), FVector2D(FMath::Clamp((double)InMember.Hp / FMath::Max(InMember.HpMax, 1), 0.05, 1.0), 1.0));
	P3RUI::SetScale2D(Find(TEXT("SpBar")), FVector2D(FMath::Clamp((double)InMember.Sp / FMath::Max(InMember.SpMax, 1), 0.05, 1.0), 1.0));
	UDreamWidget* FaceWidget = Find(TEXT("Face"));
	if (UDreamImage* Image = InFace != nullptr && FaceWidget != nullptr ? Cast<UDreamImage>(FaceWidget->GetVisual()) : nullptr)
	{
		FDreamUIImageBrush Brush = Image->GetBrush();
		Brush.SetResourceObject(InFace);
		Image->SetBrush(Brush);
	}
}

void UP3RPartyRow::SetChosen(bool bInChosen, bool bInstant)
{
	using namespace P3RPartyRowLocal;
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

void UP3RPartyRow::ApplyColours()
{
	using namespace P3RPartyRowLocal;
	P3RUI::SetTint(Find(TEXT("WindowShade")), bChosen ? BandChosen : Band);
	P3RUI::SetTextColor(Find(TEXT("Name")), bChosen ? InkChosen : P3RUI::White);
	P3RUI::SetTextColor(Find(TEXT("Lv")), bChosen ? InkChosen : P3RUI::White);
	P3RUI::SetTextColor(Find(TEXT("LvTag")), bChosen ? InkChosen : P3RUI::White);
	P3RUI::SetTextColor(Find(TEXT("Hp")), bChosen ? HpChosen : P3RUI::HpCyan);
	P3RUI::SetTextColor(Find(TEXT("Sp")), bChosen ? SpChosen : P3RUI::SpYellow);
}
