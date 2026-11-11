// Copyright DreamMoon. All Rights Reserved.

#include "P3RFieldHud.h"

#include "P3RUI.h"

#include "Core/Components/DreamWidget.h"

bool UP3RFieldHud::HandleKey(EP3RKey Key)
{
	if (Key == EP3RKey::Open)
	{
		OnChosen.Broadcast(0);
		return true;
	}
	return false;
}

void UP3RFieldHud::PlayEntrance()
{
	for (const TCHAR* Name : { TEXT("Clock"), TEXT("Place"), TEXT("PlaceName"), TEXT("Prompt") })
	{
		if (UDreamWidget* W = Node(Name))
		{
			P3RUI::Settle(W);
			W->SetRenderOpacity(0.0f);
			P3RUI::FadeTo(W, 1.0f, 0.3f, 0.15f);
		}
	}
	if (UDreamWidget* Place = Node(TEXT("Place")))
	{
		P3RUI::SetOffset(Place, FVector2D(-560.0, 0.0));
		P3RUI::OffsetTo(Place, FVector2D::ZeroVector, 0.35f, 0.1f);
	}
	LockInput(0.35f);
}

float UP3RFieldHud::PlayExit()
{
	for (const TCHAR* Name : { TEXT("Clock"), TEXT("Place"), TEXT("PlaceName"), TEXT("Prompt") })
	{
		if (UDreamWidget* W = Node(Name))
		{
			P3RUI::Settle(W);
			P3RUI::FadeTo(W, 0.0f, 0.12f);
		}
	}
	return 0.14f;
}

void UP3RFieldHud::TickScreen(float DeltaTime)
{
	// The prompt breathes so it reads as waiting for a key.
	if (UDreamWidget* Prompt = Node(TEXT("Prompt")); Prompt != nullptr && GetEnterTime() > 0.6f)
	{
		Prompt->SetRenderOpacity(0.65f + 0.35f * FMath::Cos(GetEnterTime() * 3.0f));
	}
}
