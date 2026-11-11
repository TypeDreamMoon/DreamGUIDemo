#include "NieRMotion.h"

#include "Core/Components/DreamWidget.h"
#include "DreamTweenBPLibrary.h"
#include "DreamTweener.h"

namespace NieRMotionLocal
{
	/** Design pixels (+y down) to the canvas plane (+y up). */
	FVector2D ToPlane(const FVector2D& Offset)
	{
		return FVector2D(Offset.X, -Offset.Y);
	}
}

void FNieRMotion::SetOffset(UDreamWidget* Widget, FVector2D Offset)
{
	if (IsValid(Widget))
	{
		const FVector2D Plane = NieRMotionLocal::ToPlane(Offset);
		Widget->SetRenderTranslation(FVector(Widget->GetRenderTranslation().X, Plane.X, Plane.Y));
	}
}

void FNieRMotion::SlideIn(UDreamWidget* Widget, FVector2D Offset, float Duration, float Delay)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	SetOffset(Widget, Offset);
	Widget->SetRenderOpacity(0.0f);
	Track(Widget, Widget->RenderOffsetTo(FVector2D::ZeroVector, Duration, Delay, EDreamTweenEase::OutCubic));
	Track(Widget, Widget->RenderOpacityTo(1.0f, Duration, Delay, EDreamTweenEase::OutCubic));
}

void FNieRMotion::FadeIn(UDreamWidget* Widget, float Duration, float From, float Delay)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	Widget->SetRenderOpacity(From);
	Track(Widget, Widget->RenderOpacityTo(1.0f, Duration, Delay, EDreamTweenEase::OutCubic));
}

void FNieRMotion::Nudge(UDreamWidget* Widget, FVector2D Offset, float Duration)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	SetOffset(Widget, Offset);
	Track(Widget, Widget->RenderOffsetTo(FVector2D::ZeroVector, Duration, 0.0f, EDreamTweenEase::OutCubic));
}

void FNieRMotion::Glide(UDreamWidget* Widget, FVector2D Offset, float Duration)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	SetOffset(Widget, Offset);
	Widget->SetRenderOpacity(1.0f);
	Track(Widget, Widget->RenderOffsetTo(FVector2D::ZeroVector, Duration, 0.0f, EDreamTweenEase::OutCubic));
}

void FNieRMotion::GrowX(UDreamWidget* Widget, float Duration)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	Widget->SetRenderScale(FVector(1.0, 0.0, 1.0));
	Track(Widget, Widget->RenderScaleTo(FVector::OneVector, Duration, 0.0f, EDreamTweenEase::OutCubic));
}

void FNieRMotion::Stop(UDreamWidget* Widget)
{
	if (TArray<TWeakObjectPtr<UDreamTweener>>* Tweens = Running.Find(Widget))
	{
		for (const TWeakObjectPtr<UDreamTweener>& Tweener : *Tweens)
		{
			if (UDreamTweener* Live = Tweener.Get())
			{
				UDreamTweenBPLibrary::KillIfIsTweening(Widget, Live);
			}
		}
		Tweens->Reset();
	}
}

void FNieRMotion::Settle(UDreamWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return;
	}
	Stop(Widget);
	Widget->SetRenderTranslation(FVector::ZeroVector);
	Widget->SetRenderScale(FVector::OneVector);
	Widget->SetRenderOpacity(1.0f);
}

void FNieRMotion::Track(UDreamWidget* Widget, UDreamTweener* Tweener)
{
	if (Tweener != nullptr)
	{
		Running.FindOrAdd(Widget).Add(Tweener);
	}
}
