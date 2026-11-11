// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UDreamWidget;

/** NOCTURNE//OS's palette (sRGB), the same as DUI/Nocturne/Nocturne_Common.dui's resources, and small painting helpers. */
namespace NocturneUI
{
	inline const FColor Ink(0xD7, 0xE2, 0xEA);
	inline const FColor InkBright(0xFF, 0xFF, 0xFF);
	inline const FColor InkDim(0x8A, 0x98, 0xA6);
	inline const FColor InkFaint(0x56, 0x63, 0x6F);
	inline const FColor Cyan(0x8F, 0xE3, 0xFF);
	inline const FColor CyanTag(0x8F, 0xE3, 0xFF, 0xB0);
	inline const FColor Amber(0xFF, 0xB4, 0x54);
	inline const FColor Clear(0, 0, 0, 0);

	/** A row's background: idle, under the pointer, highlighted. */
	inline const FColor RowHover(0x8F, 0xE3, 0xFF, 0x0B);
	inline const FColor RowLit(0x8F, 0xE3, 0xFF, 0x1A);
	/** A segment box: off, off on a highlighted row, on. */
	inline const FColor SegOff(0xFF, 0xFF, 0xFF, 0x0E);
	inline const FColor SegOffLit(0xFF, 0xFF, 0xFF, 0x18);
	inline const FColor SegOn(0x8F, 0xE3, 0xFF, 0x38);

	void SetText(UDreamWidget* Widget, const FText& Text);
	void SetTextColor(UDreamWidget* Widget, const FColor& Color);
	/** A rect block's body colour, else the visual's colour. */
	void SetTint(UDreamWidget* Widget, const FColor& Color);
	void SetShown(UDreamWidget* Widget, bool bShown);
}
