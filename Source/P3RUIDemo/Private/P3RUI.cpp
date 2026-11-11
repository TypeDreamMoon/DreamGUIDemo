// Copyright DreamMoon. All Rights Reserved.

#include "P3RUI.h"

#include "Core/Components/DreamImage.h"
#include "Core/Components/DreamRectBlock.h"
#include "Core/Components/DreamText.h"
#include "Core/Components/DreamVisual.h"
#include "Core/Components/DreamWidget.h"
#include "DreamTweenManager.h"
#include "Engine/Texture.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace P3RUI
{
	void SetText(UDreamWidget* Widget, const FText& Text)
	{
		UDreamText* Visual = IsValid(Widget) ? Cast<UDreamText>(Widget->GetVisual()) : nullptr;
		if (Visual != nullptr && !Visual->GetText().EqualTo(Text))
		{
			Visual->SetText(Text);
		}
	}

	void SetTextColor(UDreamWidget* Widget, const FColor& Color)
	{
		if (UDreamVisual* Visual = IsValid(Widget) ? Widget->GetVisual() : nullptr)
		{
			Visual->SetColor(Color);
		}
	}

	void SetTint(UDreamWidget* Widget, const FColor& Color)
	{
		if (!IsValid(Widget))
		{
			return;
		}
		if (UDreamRectBlock* Block = Cast<UDreamRectBlock>(Widget->GetVisual()))
		{
			Block->SetBodyColor(Color);
		}
		else if (UDreamVisual* Visual = Widget->GetVisual())
		{
			Visual->SetColor(Color);
		}
	}

	void SetShown(UDreamWidget* Widget, bool bShown)
	{
		if (IsValid(Widget))
		{
			Widget->SetVisibility(bShown ? EDreamWidgetVisibility::Visible : EDreamWidgetVisibility::Collapsed);
		}
	}

	UMaterialInstanceDynamic* MakeFigure(UDreamWidget* Widget, UObject* Outer, float Silhouette)
	{
		UDreamImage* Image = IsValid(Widget) ? Cast<UDreamImage>(Widget->GetVisual()) : nullptr;
		UTexture* Picture = Image != nullptr ? Cast<UTexture>(Image->GetBrush().GetResourceObject()) : nullptr;
		UMaterialInterface* Parent = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/P3R/Materials/M_P3R_Figure.M_P3R_Figure"));
		if (Picture == nullptr || Parent == nullptr)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Figure = UMaterialInstanceDynamic::Create(Parent, Outer);
		Figure->SetTextureParameterValue(TEXT("FigureTex"), Picture);
		Figure->SetScalarParameterValue(TEXT("Silhouette"), Silhouette);
		Image->SetBrushFromMaterial(Figure);
		return Figure;
	}

	void Settle(UDreamWidget* Widget)
	{
		if (IsValid(Widget))
		{
			UDreamTweenManager::KillAllTweensOnTarget(Widget, Widget);
		}
	}

	void SetOffset(UDreamWidget* Widget, FVector2D Offset)
	{
		if (IsValid(Widget))
		{
			Widget->SetRenderTranslation(FVector(Widget->GetRenderTranslation().X, Offset.X, -Offset.Y));
		}
	}

	FVector2D GetOffset(const UDreamWidget* Widget)
	{
		if (!IsValid(Widget))
		{
			return FVector2D::ZeroVector;
		}
		const FVector T = Widget->GetRenderTranslation();
		return FVector2D(T.Y, -T.Z);
	}

	UDreamTweener* OffsetTo(UDreamWidget* Widget, FVector2D Offset, float Seconds, float Delay, EDreamTweenEase Ease)
	{
		if (!IsValid(Widget))
		{
			return nullptr;
		}
		const FVector Current = Widget->GetRenderTranslation();
		return Widget->RenderTranslationTo(FVector(Current.X, Offset.X, -Offset.Y), Seconds, Delay, Ease);
	}

	void SetScale2D(UDreamWidget* Widget, FVector2D Scale)
	{
		if (IsValid(Widget))
		{
			Widget->SetRenderScale(FVector(1.0, Scale.X, Scale.Y));
		}
	}

	UDreamTweener* Scale2DTo(UDreamWidget* Widget, FVector2D Scale, float Seconds, float Delay, EDreamTweenEase Ease)
	{
		return IsValid(Widget) ? Widget->RenderScaleTo(FVector(1.0, Scale.X, Scale.Y), Seconds, Delay, Ease) : nullptr;
	}

	UDreamTweener* RotateTo(UDreamWidget* Widget, const FRotator& To, float Seconds, float Delay, EDreamTweenEase Ease)
	{
		if (!IsValid(Widget))
		{
			return nullptr;
		}
		UDreamTweener* Tweener = UDreamTweenManager::To(Widget,
			FDreamTweenRotatorGetterFunction::CreateWeakLambda(Widget, [Widget]() { return Widget->GetRenderRotation(); }),
			FDreamTweenRotatorSetterFunction::CreateWeakLambda(Widget, [Widget](FRotator Value) { Widget->SetRenderRotation(Value); }),
			To, Seconds);
		if (Tweener != nullptr)
		{
			Tweener->SetEase(Ease)->SetDelay(Delay);
		}
		return Tweener;
	}

	UDreamTweener* FadeTo(UDreamWidget* Widget, float Opacity, float Seconds, float Delay, EDreamTweenEase Ease)
	{
		return IsValid(Widget) ? Widget->RenderOpacityTo(Opacity, Seconds, Delay, Ease) : nullptr;
	}

	float EaseOutCubic(float X)
	{
		X = FMath::Clamp(X, 0.0f, 1.0f);
		const float Y = 1.0f - X;
		return 1.0f - Y * Y * Y;
	}

	float EaseOutBack(float X, float Overshoot)
	{
		X = FMath::Clamp(X, 0.0f, 1.0f);
		const float C3 = Overshoot + 1.0f;
		const float Y = X - 1.0f;
		return 1.0f + C3 * Y * Y * Y + Overshoot * Y * Y;
	}

	float EaseInCubic(float X)
	{
		X = FMath::Clamp(X, 0.0f, 1.0f);
		return X * X * X;
	}
}
