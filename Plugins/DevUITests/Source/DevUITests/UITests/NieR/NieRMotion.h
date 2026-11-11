// Motion for the NieR test screens. Everything moves through the render transform and the render opacity, never through
// the layout: most widgets here are placed by a panel, which would write an anchored position or a size back on its
// next pass. Every move ends at rest -- no render offset, scale one, opacity one -- which is where the layout put it.

#pragma once

#include "CoreMinimal.h"

class UDreamTweener;
class UDreamWidget;

/** The motion one widget owner runs, so a new move on a widget first stops the one still running on it. */
struct FNieRMotion
{
	/** Slide in from Offset (design pixels, +y down) while fading in from transparent, after Delay. */
	void SlideIn(UDreamWidget* Widget, FVector2D Offset, float Duration = 0.22f, float Delay = 0.0f);
	/** Fade in from From, without moving. */
	void FadeIn(UDreamWidget* Widget, float Duration = 0.2f, float From = 0.0f, float Delay = 0.0f);
	/** Knocked Offset away (+y down) and sprung back. */
	void Nudge(UDreamWidget* Widget, FVector2D Offset, float Duration = 0.16f);
	/** From Offset (+y down) back to rest, fully opaque all the way: a pointer gliding over from the row before. */
	void Glide(UDreamWidget* Widget, FVector2D Offset, float Duration);
	/** Grown from nothing along X about the render pivot (a line opening from its left edge, given a left pivot). */
	void GrowX(UDreamWidget* Widget, float Duration);
	/** Stopped where it is. */
	void Stop(UDreamWidget* Widget);
	/** Stopped and put at rest. */
	void Settle(UDreamWidget* Widget);

	/** A render offset in design pixels, +y down. */
	static void SetOffset(UDreamWidget* Widget, FVector2D Offset);

private:
	void Track(UDreamWidget* Widget, UDreamTweener* Tweener);

	TMap<TWeakObjectPtr<UDreamWidget>, TArray<TWeakObjectPtr<UDreamTweener>>> Running;
};
