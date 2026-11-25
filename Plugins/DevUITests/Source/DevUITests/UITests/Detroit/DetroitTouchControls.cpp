// Copyright DreamMoon. All Rights Reserved.

#include "DetroitTouchControls.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace DetroitTouchLocal
{
	// The HUD's accent, the blue of the investigation UI.
	const FLinearColor Accent(0.25f, 0.69f, 1.0f, 1.0f);

	/** A finger, or the mouse, followed by one widget from press to release. */
	struct FDrag
	{
		int32 Pointer = INDEX_NONE;
		FVector2D Start = FVector2D::ZeroVector;
		FVector2D Last = FVector2D::ZeroVector;

		bool IsActive() const { return Pointer != INDEX_NONE; }
		bool Is(const FPointerEvent& Event) const { return Pointer == int32(Event.GetPointerIndex()); }
	};
}

/** Where the left thumb lands, a stick appears; dragging from there moves, up to its rim. */
class SDetroitTouchStick : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDetroitTouchStick) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	FVector2D GetValue() const
	{
		if (!Drag.IsActive())
		{
			return FVector2D::ZeroVector;
		}
		FVector2D Offset = (Drag.Last - Drag.Start) / Radius;
		if (Offset.SizeSquared() > 1.0)
		{
			Offset.Normalize();
		}
		// Down the screen is back.
		return FVector2D(Offset.X, -Offset.Y);
	}

	void Reset() { Drag = DetroitTouchLocal::FDrag(); }

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect& Culling, FSlateWindowElementList& OutElements,
		int32 LayerId, const FWidgetStyle& Style, bool bParentEnabled) const override
	{
		const FVector2D Size = Geometry.GetLocalSize();
		// At rest the stick is drawn faintly where a left thumb usually is, so it can be found.
		const FVector2D Home(Radius + 72.0, Size.Y - Radius - 64.0);
		const FVector2D Centre = Drag.IsActive() ? Drag.Start : Home;
		FVector2D Knob = Centre;
		if (Drag.IsActive())
		{
			const FVector2D Value = GetValue();
			Knob = Centre + FVector2D(Value.X, -Value.Y) * Radius;
		}
		const float Alpha = Drag.IsActive() ? 1.0f : 0.55f;
		auto Circle = [&](const FSlateBrush& Brush, const FVector2D& At, double R, int32 Layer)
		{
			FSlateDrawElement::MakeBox(OutElements, Layer,
				Geometry.ToPaintGeometry(FVector2f(R * 2.0, R * 2.0), FSlateLayoutTransform(FVector2f(At - FVector2D(R, R)))),
				&Brush, ESlateDrawEffect::None, FLinearColor(1.0f, 1.0f, 1.0f, Alpha));
		};
		Circle(BaseBrush, Centre, Radius, LayerId);
		Circle(KnobBrush, Knob, KnobRadius, LayerId + 1);
		return LayerId + 1;
	}

	virtual FReply OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event) override { return Press(Geometry, Event); }
	virtual FReply OnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event) override { return Move(Geometry, Event); }
	virtual FReply OnTouchEnded(const FGeometry& Geometry, const FPointerEvent& Event) override { return Release(Event); }
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override { return Press(Geometry, Event); }
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override { return Move(Geometry, Event); }
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override { return Release(Event); }
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override
	{
		if (Drag.Pointer == int32(Event.PointerIndex))
		{
			Reset();
		}
	}

private:
	FReply Press(const FGeometry& Geometry, const FPointerEvent& Event)
	{
		if (Drag.IsActive())
		{
			return FReply::Unhandled();
		}
		Drag.Pointer = int32(Event.GetPointerIndex());
		Drag.Start = Drag.Last = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	FReply Move(const FGeometry& Geometry, const FPointerEvent& Event)
	{
		if (!Drag.Is(Event))
		{
			return FReply::Unhandled();
		}
		Drag.Last = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		return FReply::Handled();
	}
	FReply Release(const FPointerEvent& Event)
	{
		if (!Drag.Is(Event))
		{
			return FReply::Unhandled();
		}
		Reset();
		return FReply::Handled().ReleaseMouseCapture();
	}

	static constexpr double Radius = 104.0;
	static constexpr double KnobRadius = 42.0;
	DetroitTouchLocal::FDrag Drag;
	FSlateRoundedBoxBrush BaseBrush{ FLinearColor(0.0f, 0.0f, 0.0f, 0.28f), float(Radius), FLinearColor(1.0f, 1.0f, 1.0f, 0.5f), 2.0f };
	FSlateRoundedBoxBrush KnobBrush{ FLinearColor(DetroitTouchLocal::Accent.R, DetroitTouchLocal::Accent.G, DetroitTouchLocal::Accent.B, 0.75f),
		float(KnobRadius), FLinearColor(1.0f, 1.0f, 1.0f, 0.8f), 2.0f };
};

/** The right of the screen: a drag turns the camera. Draws nothing. */
class SDetroitTouchLook : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SDetroitTouchLook) {}
	SLATE_END_ARGS()

	void Construct(const FArguments&) {}

	FVector2D Consume()
	{
		const FVector2D Result = Pending;
		Pending = FVector2D::ZeroVector;
		return Result;
	}

	void Reset()
	{
		Drag = DetroitTouchLocal::FDrag();
		Pending = FVector2D::ZeroVector;
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual int32 OnPaint(const FPaintArgs&, const FGeometry&, const FSlateRect&, FSlateWindowElementList&, int32 LayerId, const FWidgetStyle&, bool) const override
	{
		return LayerId;
	}

	virtual FReply OnTouchStarted(const FGeometry& Geometry, const FPointerEvent& Event) override { return Press(Geometry, Event); }
	virtual FReply OnTouchMoved(const FGeometry& Geometry, const FPointerEvent& Event) override { return Move(Geometry, Event); }
	virtual FReply OnTouchEnded(const FGeometry& Geometry, const FPointerEvent& Event) override { return Release(Event); }
	virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override { return Press(Geometry, Event); }
	virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override { return Move(Geometry, Event); }
	virtual FReply OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event) override { return Release(Event); }
	virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override
	{
		if (Drag.Pointer == int32(Event.PointerIndex))
		{
			Drag = DetroitTouchLocal::FDrag();
		}
	}

private:
	FReply Press(const FGeometry& Geometry, const FPointerEvent& Event)
	{
		if (Drag.IsActive())
		{
			return FReply::Unhandled();
		}
		Drag.Pointer = int32(Event.GetPointerIndex());
		Drag.Start = Drag.Last = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		return FReply::Handled().CaptureMouse(SharedThis(this));
	}
	FReply Move(const FGeometry& Geometry, const FPointerEvent& Event)
	{
		if (!Drag.Is(Event))
		{
			return FReply::Unhandled();
		}
		const FVector2D At = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
		Pending += At - Drag.Last;
		Drag.Last = At;
		return FReply::Handled();
	}
	FReply Release(const FPointerEvent& Event)
	{
		if (!Drag.Is(Event))
		{
			return FReply::Unhandled();
		}
		Drag = DetroitTouchLocal::FDrag();
		return FReply::Handled().ReleaseMouseCapture();
	}

	DetroitTouchLocal::FDrag Drag;
	FVector2D Pending = FVector2D::ZeroVector;
};

void SDetroitTouchControls::Construct(const FArguments& InArgs)
{
	using namespace DetroitTouchLocal;
	const FLinearColor Fill(0.0f, 0.0f, 0.0f, 0.35f);
	const FLinearColor Rim(1.0f, 1.0f, 1.0f, 0.55f);
	const FLinearColor Lit(Accent.R, Accent.G, Accent.B, 0.55f);
	const FSlateRoundedBoxBrush Normal(Fill, 20.0f, Rim, 2.0f);
	const FSlateRoundedBoxBrush Pressed(Lit, 20.0f, FLinearColor::White, 2.0f);
	ButtonStyle = FButtonStyle().SetNormal(Normal).SetHovered(Normal).SetPressed(Pressed).SetDisabled(Normal)
		.SetNormalPadding(FMargin(0.0f)).SetPressedPadding(FMargin(0.0f));

	TSharedRef<SHorizontalBox> Clues = SNew(SHorizontalBox);
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Clues->AddSlot().AutoWidth().Padding(0.0f, 0.0f, 10.0f, 0.0f)
		[
			MakeButton(FText::AsNumber(Index + 1), [this, Index]() { PendingClue = Index; }, nullptr, 64.0f)
		];
	}

	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(0.45f)[SAssignNew(Stick, SDetroitTouchStick)]
			+ SHorizontalBox::Slot().FillWidth(0.55f)[SAssignNew(Look, SDetroitTouchLook)]
		]
		+ SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(FMargin(0.0f, 0.0f, 48.0f, 40.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right).Padding(0.0f, 0.0f, 0.0f, 14.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(0.0f, 0.0f, 14.0f, 0.0f)
				[
					MakeButton(NSLOCTEXT("DetroitTouch", "Objectives", "目标"), [this]() { Pending |= EDetroitTouchAction::Objectives; })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeButton(NSLOCTEXT("DetroitTouch", "Analysis", "分析"), [this]() { Pending |= EDetroitTouchAction::Analysis; })
				]
			]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0.0f, 0.0f, 14.0f, 0.0f)
				[
					MakeButton(NSLOCTEXT("DetroitTouch", "Run", "跑"), [this]() { bRunning = !bRunning; }, [this]() { return bRunning; })
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeButton(NSLOCTEXT("DetroitTouch", "Analyze", "调查"), [this]() { Pending |= EDetroitTouchAction::Analyze; }, nullptr, 136.0f)
				]
			]
		]
		+ SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(FMargin(0.0f, 24.0f, 0.0f, 0.0f))
		[
			Clues
		]
	];
}

TSharedRef<SWidget> SDetroitTouchControls::MakeButton(const FText& Label, TFunction<void()> OnPress, TFunction<bool()> IsOn, float Size)
{
	return SNew(SBox).WidthOverride(Size).HeightOverride(Size)
	[
		SNew(SButton)
		.IsFocusable(false)
		.ClickMethod(EButtonClickMethod::MouseDown)
		.TouchMethod(EButtonTouchMethod::Down)
		.ButtonStyle(&ButtonStyle)
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.OnClicked_Lambda([OnPress = MoveTemp(OnPress)]() { OnPress(); return FReply::Handled(); })
		[
			SNew(STextBlock)
			.Text(Label)
			.Font(FCoreStyle::GetDefaultFontStyle("Bold", Size >= 100.0f ? 22 : 18))
			// A switch that is on shows it in the HUD's blue.
			.ColorAndOpacity_Lambda([IsOn]() { return FSlateColor(IsOn && IsOn() ? DetroitTouchLocal::Accent : FLinearColor::White); })
			.Justification(ETextJustify::Center)
		]
	];
}

FVector2D SDetroitTouchControls::GetMove() const
{
	return Stick.IsValid() ? Stick->GetValue() : FVector2D::ZeroVector;
}

FVector2D SDetroitTouchControls::ConsumeLook()
{
	return Look.IsValid() ? Look->Consume() : FVector2D::ZeroVector;
}

EDetroitTouchAction SDetroitTouchControls::ConsumeActions()
{
	const EDetroitTouchAction Result = Pending;
	Pending = EDetroitTouchAction::None;
	return Result;
}

int32 SDetroitTouchControls::ConsumeClue()
{
	const int32 Result = PendingClue;
	PendingClue = INDEX_NONE;
	return Result;
}

void SDetroitTouchControls::Reset()
{
	if (Stick.IsValid())
	{
		Stick->Reset();
	}
	if (Look.IsValid())
	{
		Look->Reset();
	}
	Pending = EDetroitTouchAction::None;
	PendingClue = INDEX_NONE;
}
