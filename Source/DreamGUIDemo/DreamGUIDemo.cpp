// Copyright DreamMoon. All Rights Reserved.

#include "Modules/ModuleManager.h"

#if PLATFORM_ANDROID
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Containers/Ticker.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamScreenUISubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

/**
 * The phone gallery. It starts on the engine's empty Entry map (Config/Android/AndroidEngine.ini), where it shows a page
 * to pick a demo from; inside a demo a Back button in the top right corner returns to that page. Both belong to the
 * viewport, not to a map or its controller, so every demo gets them without knowing of them.
 */
class FPhoneGalleryModule : public FDefaultGameModuleImpl
{
	/**
	 * Pages of the demos laid out on a fixed 1920x1080 design (NieR, Detroit): on a screen wider than 16:9 they stretch
	 * to the whole canvas while what is in them stays where the design put it, so it all sits left of centre. Each such
	 * page becomes a 1920x1080 box in the middle -- nothing inside it moves, so code and tweens that place its widgets
	 * still work -- and a backdrop in it (a widget with nothing inside that fills the page exactly) is widened back out
	 * to the screen's edges. Only that: a row, frame or container spanning the page lays its content out from its left
	 * edge, so widening one moved that content off centre (NieR's "Press Any Key", Detroit's analysis frame).
	 *
	 * The screen stack stretches a page back to full-bleed whenever it shows it again (NieR's menu, back from the
	 * settings), so each page is marked as placed by hand -- the stack's own way to leave a page where it was put --
	 * and any page found stretched again is boxed again. Widening a backdrop is idempotent: once widened it no longer
	 * fills the page exactly.
	 */

	static bool HasFixedDesign(const UWorld* World)
	{
		const FString Map = World->GetOutermost()->GetName();
		return Map.StartsWith(TEXT("/Game/UITests/NieR/")) || Map.StartsWith(TEXT("/Game/UITests/Detroit/"));
	}

	void CenterDesign(UWorld* World)
	{
		const FVector2D Design(1920.0, 1080.0);
		UDreamScreenUISubsystem* ScreenUI = UDreamScreenUISubsystem::Get(World);
		UDreamWidget* Root = ScreenUI != nullptr ? ScreenUI->GetScreenRoot() : nullptr;
		const float Extra = Root != nullptr ? Root->GetWidth() - Design.X : 0.0f;
		if (Extra <= 1.0f)
		{
			return;//16:9 or narrower: the design fills the screen as it is
		}
		const TArray<UDreamWidget*> Pages = Root->GetChildren();//a copy: the loop changes the pages' rects
		for (UDreamWidget* Page : Pages)
		{
			if (Page == nullptr || Page->GetAnchorMin() != FVector2D(0.0, 0.0) || Page->GetAnchorMax() != FVector2D(1.0, 1.0))
			{
				continue;
			}
			ScreenUI->SetPageHasCustomPlacement(Page, true);
			Page->SetAnchorMin(FVector2D(0.5, 0.5));
			Page->SetAnchorMax(FVector2D(0.5, 0.5));
			Page->SetPivot(FVector2D(0.5, 0.5));
			Page->SetAnchoredPositionAndSizeDelta(FVector2D::ZeroVector, Design);
			// The page's own root fills the box; a backdrop filling that is widened by the extra, half each side.
			for (UDreamWidget* PageRoot : Page->GetChildren())
			{
				for (UDreamWidget* Child : PageRoot != nullptr ? PageRoot->GetChildren() : TArray<UDreamWidget*>())
				{
					if (Child != nullptr && Child->GetAnchorMin() == FVector2D(0.0, 0.0) && Child->GetAnchorMax() == FVector2D(1.0, 1.0)
						&& Child->GetSizeDelta().IsNearlyZero() && Child->GetAnchoredPosition().IsNearlyZero() && Child->GetChildrenCount() == 0)
					{
						Child->SetAnchoredPositionAndSizeDelta(FVector2D(Extra * (Child->GetPivot().X - 0.5), 0.0), FVector2D(Extra, 0.0));
						UE_LOG(LogTemp, Display, TEXT("Gallery: %s widened backdrop '%s' (%s) by %.0f"), *Page->GetClass()->GetName(),
							*Child->GetDisplayName(), *Child->GetClass()->GetName(), Extra);
					}
				}
			}
		}
	}

	struct FDemo
	{
		const TCHAR* Title;
		const TCHAR* Line;
		const TCHAR* Map;
		FLinearColor Accent;
	};

	TWeakObjectPtr<UGameViewportClient> Owner;
	TWeakObjectPtr<UWorld> OwnerWorld;
	TSharedPtr<SWidget> Picker;
	TSharedPtr<SWidget> Back;
	FTSTicker::FDelegateHandle TickHandle;
	FName Travel;
	FButtonStyle CardStyle;
	FButtonStyle BackStyle;
	/** The cards' colour strips, held here: a Slate widget keeps only a pointer to its brush. */
	TArray<TSharedRef<FSlateRoundedBoxBrush>> AccentBrushes;

	static const TCHAR* PickerMap() { return TEXT("/Engine/Maps/Entry"); }

	static bool IsPicker(const UWorld* World)
	{
		return World != nullptr && World->GetOutermost()->GetName() == PickerMap();
	}

	static TArray<FDemo> Demos()
	{
		auto Srgb = [](uint8 R, uint8 G, uint8 B) { return FLinearColor::FromSRGBColor(FColor(R, G, B)); };
		return {
			{ TEXT("NOCTURNE // OS"), TEXT("星图菜单、设置、档案、音乐盒"), TEXT("/Game/Nocturne/Maps/Nocturne_Demo"), Srgb(140, 120, 255) },
			{ TEXT("P3R"), TEXT("场景 HUD、主菜单、三版设计稿"), TEXT("/Game/P3R/Maps/P3R_Demo"), Srgb(70, 160, 255) },
			{ TEXT("NieR"), TEXT("标题、菜单、设置"), TEXT("/Game/UITests/NieR/Maps/NieR_TestMap"), Srgb(218, 212, 192) },
			{ TEXT("底特律"), TEXT("调查现场：HUD、线索、分析模式"), TEXT("/Game/UITests/Detroit/Maps/Detroit_TestMap"), Srgb(80, 190, 255) },
		};
	}

	void MakeStyles()
	{
		const FLinearColor Fill = FLinearColor::FromSRGBColor(FColor(24, 26, 36, 235));
		const FLinearColor Lit = FLinearColor::FromSRGBColor(FColor(44, 48, 66, 245));
		const FLinearColor Rim = FLinearColor(1.0f, 1.0f, 1.0f, 0.18f);
		CardStyle = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(Fill, 14.0f, Rim, 1.5f))
			.SetHovered(FSlateRoundedBoxBrush(Fill, 14.0f, Rim, 1.5f))
			.SetPressed(FSlateRoundedBoxBrush(Lit, 14.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.6f), 2.0f))
			.SetNormalPadding(FMargin(0.0f)).SetPressedPadding(FMargin(0.0f));
		const FLinearColor BackFill(0.0f, 0.0f, 0.0f, 0.55f);
		BackStyle = FButtonStyle()
			.SetNormal(FSlateRoundedBoxBrush(BackFill, 10.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.5f), 1.5f))
			.SetHovered(FSlateRoundedBoxBrush(BackFill, 10.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.5f), 1.5f))
			.SetPressed(FSlateRoundedBoxBrush(FLinearColor(1.0f, 1.0f, 1.0f, 0.3f), 10.0f, FLinearColor::White, 1.5f))
			.SetNormalPadding(FMargin(0.0f)).SetPressedPadding(FMargin(0.0f));
	}

	TSharedRef<SWidget> MakeCard(const FDemo& Demo)
	{
		const FName Map(Demo.Map);
		const FSlateBrush* Strip = &AccentBrushes.Add_GetRef(MakeShared<FSlateRoundedBoxBrush>(Demo.Accent, 5.0f)).Get();
		return SNew(SBox).HeightOverride(220.0f)
		[
			SNew(SButton)
			.IsFocusable(false)
			.ButtonStyle(&CardStyle)
			.OnClicked_Lambda([this, Map]() { Travel = Map; return FReply::Handled(); })
			[
				SNew(SHorizontalBox)
				// The demo's colour down the left edge.
				+ SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(10.0f)
					[
						SNew(SBorder).BorderImage(Strip)
					]
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(FMargin(36.0f, 0.0f, 24.0f, 0.0f))
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot().AutoHeight()
					[
						SNew(STextBlock).Text(FText::FromString(Demo.Title)).Font(FCoreStyle::GetDefaultFontStyle("Bold", 40)).ColorAndOpacity(FLinearColor::White)
					]
					+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
					[
						SNew(STextBlock).Text(FText::FromString(Demo.Line)).Font(FCoreStyle::GetDefaultFontStyle("Regular", 22))
						.ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.62f))
					]
				]
			]
		];
	}

	TSharedRef<SWidget> MakePicker()
	{
		TSharedRef<SUniformGridPanel> Grid = SNew(SUniformGridPanel).SlotPadding(FMargin(18.0f));
		const TArray<FDemo> All = Demos();
		for (int32 Index = 0; Index < All.Num(); ++Index)
		{
			Grid->AddSlot(Index % 2, Index / 2)[MakeCard(All[Index])];
		}
		static FSlateColorBrush Backdrop(FLinearColor::FromSRGBColor(FColor(10, 11, 16)));
		return SNew(SBorder)
			.BorderImage(&Backdrop)
			.Padding(FMargin(96.0f, 64.0f))
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("DreamGUI Gallery"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 48)).ColorAndOpacity(FLinearColor::White)
				]
				+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 10.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock).Text(FText::FromString(TEXT("选择一个 Demo。进入后点右上角「返回」回到这里。")))
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", 22)).ColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 0.55f))
				]
				+ SVerticalBox::Slot().FillHeight(1.0f).VAlign(VAlign_Center).Padding(FMargin(0.0f, 24.0f, 0.0f, 0.0f))
				[
					Grid
				]
			];
	}

	TSharedRef<SWidget> MakeBack()
	{
		return SNew(SBox)
			.Visibility(EVisibility::SelfHitTestInvisible)
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Top)
			.Padding(FMargin(0.0f, 28.0f, 28.0f, 0.0f))
			[
				SNew(SBox).WidthOverride(150.0f).HeightOverride(72.0f)
				[
					SNew(SButton)
					.IsFocusable(false)
					.ButtonStyle(&BackStyle)
					.HAlign(HAlign_Center)
					.VAlign(VAlign_Center)
					.OnClicked_Lambda([this]() { Travel = FName(PickerMap()); return FReply::Handled(); })
					[
						SNew(STextBlock).Text(FText::FromString(TEXT("返回"))).Font(FCoreStyle::GetDefaultFontStyle("Bold", 24)).ColorAndOpacity(FLinearColor::White)
					]
				]
			];
	}

	/** The picker on the Entry map, the Back button anywhere else; once per world. */
	void Attach(UGameViewportClient* Viewport, UWorld* World)
	{
		Detach();
		if (IsPicker(World))
		{
			Picker = MakePicker();
			Viewport->AddViewportWidgetContent(Picker.ToSharedRef(), 10000);
		}
		else
		{
			Back = MakeBack();
			Viewport->AddViewportWidgetContent(Back.ToSharedRef(), 10000);
		}
		Owner = Viewport;
		OwnerWorld = World;
	}

	void Detach()
	{
		if (UGameViewportClient* Viewport = Owner.Get())
		{
			if (Picker.IsValid())
			{
				Viewport->RemoveViewportWidgetContent(Picker.ToSharedRef());
			}
			if (Back.IsValid())
			{
				Viewport->RemoveViewportWidgetContent(Back.ToSharedRef());
			}
		}
		Picker.Reset();
		Back.Reset();
		AccentBrushes.Reset();
	}

public:
	virtual void StartupModule() override
	{
		MakeStyles();
		TickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float)
		{
			UGameViewportClient* Viewport = GEngine != nullptr ? GEngine->GameViewport.Get() : nullptr;
			UWorld* World = Viewport != nullptr ? Viewport->GetWorld() : nullptr;
			if (World != nullptr && World->HasBegunPlay() && FSlateApplication::IsInitialized())
			{
				// A map change clears the viewport's widgets, so each new world gets its own.
				if (Owner.Get() != Viewport || OwnerWorld.Get() != World)
				{
					Attach(Viewport, World);
				}
				if (HasFixedDesign(World))
				{
					CenterDesign(World);
				}
				if (!Travel.IsNone())
				{
					const FName Next = Travel;
					Travel = NAME_None;
					UGameplayStatics::OpenLevel(World, Next);
				}
			}
			return true;
		}));
	}

	virtual void ShutdownModule() override
	{
		FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
		Detach();
	}
};
IMPLEMENT_PRIMARY_GAME_MODULE(FPhoneGalleryModule, DreamGUIDemo, "DreamGUIDemo");
#else
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, DreamGUIDemo, "DreamGUIDemo");
#endif
