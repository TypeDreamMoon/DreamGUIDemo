// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DreamTweener.h"

class UDreamWidget;
class UDreamTweener;
class UMaterialInstanceDynamic;

/**
 * The style study's palette (sRGB, the same values as DUI/P3R/P3R_Common.dui's resources) and the small helpers every
 * screen paints and moves its parts with. Design space is 1920 x 1080 with DreamGUI's +y up.
 */
namespace P3RUI
{
	inline const FColor White(0xFF, 0xFF, 0xFF);
	inline const FColor Black(0x05, 0x06, 0x0C);
	inline const FColor Red(0xFF, 0x1E, 0x3C);
	inline const FColor Pink(0xFF, 0x6A, 0xB4);
	inline const FColor Cyan(0x16, 0xCF, 0xFB);
	inline const FColor CyanPale(0x7D, 0xE6, 0xFD);
	inline const FColor Aqua(0x77, 0xFE, 0xFC);
	inline const FColor DeepBlue(0x10, 0x2C, 0xB8);
	inline const FColor Navy(0x08, 0x12, 0x5A);
	inline const FColor Grey(0x8A, 0x90, 0xA8);
	inline const FColor HpCyan(0x2E, 0xE6, 0xFF);
	inline const FColor SpYellow(0xFF, 0xE0, 0x3A);
	inline const FColor Clear(0, 0, 0, 0);

	/** The unselected menu words cycle through three near cyans, as the game's do. */
	inline const FColor WordColours[3] = { Cyan, CyanPale, Aqua };

	void SetText(UDreamWidget* Widget, const FText& Text);
	void SetTextColor(UDreamWidget* Widget, const FColor& Color);
	/** A rect block's body colour, else the visual's colour. */
	void SetTint(UDreamWidget* Widget, const FColor& Color);
	void SetShown(UDreamWidget* Widget, bool bShown);

	/** Kill every tween running on the widget (two opacity tweens on one widget fight to their last frame). */
	void Settle(UDreamWidget* Widget);
	/** Screen-space offset in design pixels, +y DOWN (as a screen is read), keeping the depth. */
	void SetOffset(UDreamWidget* Widget, FVector2D Offset);
	FVector2D GetOffset(const UDreamWidget* Widget);
	UDreamTweener* OffsetTo(UDreamWidget* Widget, FVector2D Offset, float Seconds, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	/** Uniform in-plane scale (DreamGUI's RenderScale is depth, horizontal, vertical). */
	void SetScale2D(UDreamWidget* Widget, FVector2D Scale);
	UDreamTweener* Scale2DTo(UDreamWidget* Widget, FVector2D Scale, float Seconds, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	/** The full 3D rotation (DreamGUI has a tween for the in-plane angle only). */
	UDreamTweener* RotateTo(UDreamWidget* Widget, const FRotator& To, float Seconds, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);
	/** Fade, after settling whatever was fading it. */
	UDreamTweener* FadeTo(UDreamWidget* Widget, float Opacity, float Seconds, float Delay = 0.0f, EDreamTweenEase Ease = EDreamTweenEase::OutCubic);

	/**
	 * Draws an Image's picture the way the menus draw the protagonist (M_P3R_Figure: the dark masses let the sea through,
	 * a slow ripple), or, with Silhouette 1, as a flat dark-blue shape. The picture is whatever the .dui gave the brush.
	 */
	UMaterialInstanceDynamic* MakeFigure(UDreamWidget* Widget, UObject* Outer, float Silhouette = 0.0f);

	/** Ease helpers for hand-driven animation (fixed-step recordings replay them exactly). */
	float EaseOutCubic(float X);
	float EaseOutBack(float X, float Overshoot = 1.7f);
	float EaseInCubic(float X);
}
