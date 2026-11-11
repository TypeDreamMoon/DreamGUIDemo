// Copyright DreamMoon. All Rights Reserved.

#include "P3RProtoMenus.h"

#include "P3RMainMenu.h"
#include "P3RUI.h"

#include "Core/Components/DreamVisual.h"
#include "Core/Components/DreamWidget.h"
#include "Extensions/DreamMaskTransition.h"

// ---------------------------------------------------------------------------------------------------- the shared part

void UP3RProtoMenu::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	for (int32 Index = 0; Index < GetEntryCount(); ++Index)
	{
		WireButton(Node(FString::Printf(TEXT("%s%d"), GetEntryPrefix(), Index)),
			[this, Index]() { Select(Index); },
			[this, Index]()
			{
				Select(Index);
				OnChosen.Broadcast(GetSelectedItem());
			});
	}
	if (UDreamWidget* Content = GetContentRoot())
	{
		Blot = UDreamMaskTransition::FindOrAdd(Content);
	}
}

void UP3RProtoMenu::Select(int32 Index, bool bInstant)
{
	const int32 Count = GetEntryCount();
	Index = (Index % Count + Count) % Count;
	if (Index == Selected && !bInstant)
	{
		return;
	}
	const int32 Previous = Selected;
	Selected = Index;
	ShowSelection(Previous, bInstant);
}

bool UP3RProtoMenu::HandleKey(EP3RKey Key)
{
	switch (Key)
	{
	case EP3RKey::Up: Select(Selected - 1); return true;
	case EP3RKey::Down: Select(Selected + 1); return true;
	case EP3RKey::Confirm: OnChosen.Broadcast(GetSelectedItem()); return true;
	case EP3RKey::Back: OnBack.Broadcast(); return true;
	case EP3RKey::Design: OnDesign.Broadcast(); return true;
	default: return false;
	}
}

void UP3RProtoMenu::PlayBlotIn()
{
	if (UDreamMaskTransition* Mask = Blot.Get())
	{
		Mask->PlayFromTo(0.0f, 1.0f, 0.4f, EDreamTweenEase::OutCubic);
	}
}

float UP3RProtoMenu::PlayExit()
{
	if (UDreamMaskTransition* Mask = Blot.Get())
	{
		Mask->PlayBackward(0.24f, EDreamTweenEase::InCubic);
	}
	return 0.26f;
}

// ---------------------------------------------------------------------------------------------------- glass

namespace P3RProtoGlassLocal
{
	constexpr int32 Items[] = { 0, 1, 2, 3, 4, 6, 7, 5, 8 };
	/** Each entry sits this far right and down from the last. */
	const FVector2D Step(36.0, 72.0);
}

int32 UP3RProtoGlass::GetItemOf(int32 Entry) const
{
	return P3RProtoGlassLocal::Items[FMath::Clamp(Entry, 0, 8)];
}

void UP3RProtoGlass::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	Board = Node(TEXT("Board"));
	Side = Node(TEXT("Side"));
	Bar = Node(TEXT("Bar"));
	Note = Node(TEXT("Note"));
	Select(0, /*bInstant*/true);
}

void UP3RProtoGlass::ShowSelection(int32 Previous, bool bInstant)
{
	using namespace P3RProtoGlassLocal;
	const FVector2D Offset = Step * Selected;
	if (bInstant)
	{
		P3RUI::SetOffset(Bar.Get(), Offset);
		P3RUI::SetOffset(Note.Get(), Offset);
	}
	else
	{
		P3RUI::Settle(Bar.Get());
		P3RUI::Settle(Note.Get());
		P3RUI::OffsetTo(Bar.Get(), Offset, 0.08f, 0.0f, EDreamTweenEase::OutCubic);
		P3RUI::OffsetTo(Note.Get(), Offset, 0.1f, 0.0f, EDreamTweenEase::OutCubic);
		if (UDreamWidget* B = Bar.Get())
		{
			B->SetRenderOpacity(1.0f);
			P3RUI::SetScale2D(B, FVector2D(0.6, 1.0));
			P3RUI::Scale2DTo(B, FVector2D(1.0, 1.0), 0.12f, 0.0f, EDreamTweenEase::OutBack);
		}
	}
	P3RUI::SetText(Note.Get(), UP3RMainMenu::GetGuide(GetSelectedItem()));
	for (int32 I = 0; I < GetEntryCount(); ++I)
	{
		P3RUI::SetTextColor(Node(FString::Printf(TEXT("Item%dText"), I)), I == Selected ? P3RUI::White : FColor(0xEC, 0xF2, 0xF8));
	}
}

void UP3RProtoGlass::PlayEntrance()
{
	using namespace P3RProtoGlassLocal;
	EnterAt = GetScreenTime();
	PlayBlotIn();
	// The two blurred polygons slide in from the right, the second a beat later (no fade: a see-through group would draw
	// off screen, and the blur in it would have no street to read).
	float PieceDelay = 0.0f;
	for (UDreamWidget* Piece : { Board.Get(), Side.Get() })
	{
		if (Piece != nullptr)
		{
			P3RUI::Settle(Piece);
			P3RUI::SetOffset(Piece, FVector2D(900.0, -130.0));
			Piece->SetRenderOpacity(1.0f);
			P3RUI::OffsetTo(Piece, FVector2D::ZeroVector, 0.45f, PieceDelay, EDreamTweenEase::OutCubic);
		}
		PieceDelay += 0.08f;
	}
	for (int32 I = 0; I < GetEntryCount(); ++I)
	{
		if (UDreamWidget* Word = Node(FString::Printf(TEXT("Item%d"), I)))
		{
			P3RUI::Settle(Word);
			const float Delay = 0.12f + I * 0.03f;
			P3RUI::SetOffset(Word, FVector2D(-260.0, 0.0));
			Word->SetRenderOpacity(0.0f);
			P3RUI::OffsetTo(Word, FVector2D::ZeroVector, 0.3f, Delay, EDreamTweenEase::OutCubic);
			P3RUI::FadeTo(Word, 1.0f, 0.2f, Delay);
		}
	}
	if (UDreamWidget* B = Bar.Get())
	{
		P3RUI::Settle(B);
		P3RUI::SetOffset(B, Step * Selected);
		P3RUI::SetScale2D(B, FVector2D(0.0, 1.0));
		P3RUI::Scale2DTo(B, FVector2D(1.0, 1.0), 0.25f, 0.4f, EDreamTweenEase::OutCubic);
	}
	LockInput(0.3f);
}

void UP3RProtoGlass::TickScreen(float DeltaTime)
{
	// Still: the design's polygons do not move once they are in.
}

// ---------------------------------------------------------------------------------------------------- mosaic

namespace P3RProtoMosaicLocal
{
	/** The still each entry lights. */
	constexpr int32 TileOf[] = { 1, 2, 3, 4, 7 };
	// Only a little darker: a tint is a multiply in linear light, so a grey that looks mid is much darker than it reads.
	const FColor TileDim(0xC8, 0xC8, 0xD0);
}

void UP3RProtoMosaic::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	for (int32 I = 0; I < TileCount; ++I)
	{
		Tiles[I] = Node(FString::Printf(TEXT("Tile%d"), I));
		if (UDreamWidget* T = Tiles[I].Get())
		{
			T->SetRenderTransformPivot(FVector2D(0.5, 0.5));
		}
	}
	Underline = Node(TEXT("Underline"));
	Note = Node(TEXT("Note"));
	Triangle = Node(TEXT("Triangle"));
	Select(0, /*bInstant*/true);
}

void UP3RProtoMosaic::ShowSelection(int32 Previous, bool bInstant)
{
	using namespace P3RProtoMosaicLocal;
	for (int32 Entry = 0; Entry < GetEntryCount(); ++Entry)
	{
		UDreamWidget* T = Tiles[TileOf[Entry]].Get();
		if (T == nullptr)
		{
			continue;
		}
		const bool bChosen = Entry == Selected;
		UDreamVisual* TileVisual = T->GetVisual();
		if (bInstant)
		{
			if (TileVisual != nullptr)
			{
				TileVisual->SetColor(bChosen ? P3RUI::White : TileDim);
			}
			continue;
		}
		if (TileVisual != nullptr)
		{
			TileVisual->ColorTo(bChosen ? P3RUI::White : TileDim, 0.15f);
		}
		P3RUI::Settle(T);
		T->SetRenderOpacity(1.0f);
		P3RUI::Scale2DTo(T, FVector2D(bChosen ? 1.05 : 1.0), 0.15f, 0.0f, EDreamTweenEase::OutBack);
	}
	if (UDreamWidget* Label = Node(FString::Printf(TEXT("Label%d"), Selected)))
	{
		if (UDreamWidget* U = Underline.Get())
		{
			const FVector2D At = Label->GetAnchoredPosition();
			const FVector2D Size = Label->GetSizeDelta();
			U->SetAnchoredPosition(FVector2D(At.X + 8.0, At.Y - Size.Y + 6.0));
			P3RUI::Settle(U);
			P3RUI::SetScale2D(U, FVector2D(0.0, 1.0));
			P3RUI::Scale2DTo(U, FVector2D(Size.X / 220.0 * 0.8, 1.0), bInstant ? 0.0f : 0.2f, 0.0f, EDreamTweenEase::OutCubic);
		}
	}
	P3RUI::SetText(Note.Get(), UP3RMainMenu::GetGuide(GetSelectedItem()));
}

void UP3RProtoMosaic::PlayEntrance()
{
	EnterAt = GetScreenTime();
	PlayBlotIn();
	ShowSelection(Selected, /*bInstant*/true);
	const int32 ChosenTile = P3RProtoMosaicLocal::TileOf[Selected];
	for (int32 I = 0; I < TileCount; ++I)
	{
		if (UDreamWidget* T = Tiles[I].Get())
		{
			P3RUI::Settle(T);
			const float Delay = 0.05f + I * 0.04f;
			T->SetRenderOpacity(0.0f);
			P3RUI::SetScale2D(T, FVector2D(0.8));
			P3RUI::FadeTo(T, 1.0f, 0.25f, Delay);
			P3RUI::Scale2DTo(T, FVector2D(I == ChosenTile ? 1.05 : 1.0), 0.3f, Delay, EDreamTweenEase::OutBack);
		}
	}
	for (int32 I = 0; I < GetEntryCount(); ++I)
	{
		if (UDreamWidget* L = Node(FString::Printf(TEXT("Label%d"), I)))
		{
			P3RUI::Settle(L);
			const float Delay = 0.25f + I * 0.05f;
			L->SetRenderOpacity(0.0f);
			P3RUI::SetOffset(L, FVector2D(0.0, 40.0));
			P3RUI::OffsetTo(L, FVector2D::ZeroVector, 0.35f, Delay, EDreamTweenEase::OutCubic);
			P3RUI::FadeTo(L, 1.0f, 0.35f, Delay);
		}
	}
	if (UDreamWidget* Tri = Triangle.Get())
	{
		P3RUI::Settle(Tri);
		P3RUI::SetOffset(Tri, FVector2D(900.0, 0.0));
		P3RUI::OffsetTo(Tri, FVector2D::ZeroVector, 0.45f, 0.0f, EDreamTweenEase::OutCubic);
	}
	LockInput(0.3f);
}

void UP3RProtoMosaic::TickScreen(float DeltaTime)
{
	// The stills drift a little, each its own way.
	const float E = GetScreenTime() - EnterAt;
	if (E < 0.6f)
	{
		return;
	}
	for (int32 I = 0; I < TileCount; ++I)
	{
		if (UDreamWidget* T = Tiles[I].Get())
		{
			P3RUI::SetOffset(T, FVector2D(FMath::Sin(E * 0.4f + I * 1.7f) * 4.0, FMath::Cos(E * 0.33f + I * 2.3f) * 4.0));
		}
	}
}
