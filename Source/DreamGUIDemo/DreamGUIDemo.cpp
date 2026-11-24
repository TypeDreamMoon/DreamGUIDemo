// Copyright DreamMoon. All Rights Reserved.

#include "Modules/ModuleManager.h"

#if PLATFORM_ANDROID
#include "Containers/Ticker.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Kismet/GameplayStatics.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

// Overlay owned by the viewport, not by a specific map or its controller.
class FPhoneGalleryModule : public FDefaultGameModuleImpl
{
    TWeakObjectPtr<UGameViewportClient> Owner;
    TSharedPtr<SWidget> Overlay;
    FTSTicker::FDelegateHandle TickHandle;
    FName Travel;
    bool bMenu = false, bPad = false;
    TSharedRef<SWidget> Choice(const TCHAR* Name, TFunction<void()> Fn)
    {
        return SNew(SButton).IsFocusable(false).ContentPadding(FMargin(8,7)).HAlign(HAlign_Center)
            .OnClicked_Lambda([Fn=MoveTemp(Fn)](){Fn(); return FReply::Handled();})
            [ SNew(STextBlock).Text(FText::FromString(Name)) ];
    }
    void Attach(UGameViewportClient* V)
    {
        TSharedRef<SVerticalBox> Rows = SNew(SVerticalBox);
        TSharedRef<SHorizontalBox> Top = SNew(SHorizontalBox);
        Rows->AddSlot().AutoHeight()[Top];
        Top->AddSlot().AutoWidth()[Choice(TEXT("DEMOS"),[this](){bMenu=!bMenu; bPad=false;})];
        Top->AddSlot().AutoWidth()[Choice(TEXT("PAD"),[this](){bPad=!bPad; bMenu=false;})];
        auto Menu = SNew(SVerticalBox).Visibility_Lambda([this](){return bMenu ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;});
        Rows->AddSlot().AutoHeight()[Menu];
        auto Map = [&](const TCHAR* Name, const TCHAR* Path){Menu->AddSlot().AutoHeight()[Choice(Name,[this,Path](){Travel=FName(Path); bMenu=false;})];};
        Map(TEXT("NOCTURNE // OS"),TEXT("/Game/Nocturne/Maps/Nocturne_Demo"));
        Map(TEXT("P3R"),TEXT("/Game/P3R/Maps/P3R_Demo"));
        Map(TEXT("NieR"),TEXT("/Game/UITests/NieR/Maps/NieR_TestMap"));
        Map(TEXT("Detroit"),TEXT("/Game/UITests/Detroit/Maps/Detroit_TestMap"));
        auto Pad = SNew(SVerticalBox).Visibility_Lambda([this](){return bPad ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;});
        Rows->AddSlot().AutoHeight()[Pad];
        auto Row=[&](){auto R=SNew(SHorizontalBox); Pad->AddSlot().AutoHeight()[R]; return R;};
        auto Key=[&](TSharedRef<SHorizontalBox> R,const TCHAR* Name,FKey K){
            auto Send=[K](bool Down){auto& Slate=FSlateApplication::Get(); const FKeyEvent Event(K,FModifierKeysState(),0,false,0,0); if(Down) Slate.ProcessKeyDownEvent(Event); else Slate.ProcessKeyUpEvent(Event);};
            R->AddSlot().AutoWidth()[SNew(SButton).IsFocusable(false).ContentPadding(FMargin(7,7)).OnPressed_Lambda([Send](){Send(true);}).OnReleased_Lambda([Send](){Send(false);})[SNew(STextBlock).Text(FText::FromString(Name))]];
        };
        auto A=Row(); Key(A,TEXT("UP"),EKeys::Up); Key(A,TEXT("DOWN"),EKeys::Down); Key(A,TEXT("OK"),EKeys::Enter);
        auto B=Row(); Key(B,TEXT("LEFT"),EKeys::Left); Key(B,TEXT("RIGHT"),EKeys::Right); Key(B,TEXT("BACK"),EKeys::BackSpace);
        auto C=Row(); Key(C,TEXT("TAB"),EKeys::Tab); Key(C,TEXT("Q"),EKeys::Q); Key(C,TEXT("E"),EKeys::E); Key(C,TEXT("W"),EKeys::W);
        Overlay=SNew(SBox).Visibility(EVisibility::SelfHitTestInvisible).HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(FMargin(24,12))[Rows];
        V->AddViewportWidgetContent(Overlay.ToSharedRef(),10000); Owner=V;
    }
public:
    virtual void StartupModule() override
    {
        TickHandle=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float){
            auto* V=GEngine ? GEngine->GameViewport.Get() : nullptr;
            if(V && V->GetWorld() && V->GetWorld()->HasBegunPlay() && FSlateApplication::IsInitialized()){
                if(Owner.Get()!=V) Attach(V);
                if(!Travel.IsNone()){FName Next=Travel; Travel=NAME_None; UGameplayStatics::OpenLevel(V->GetWorld(),Next);}
            } return true; }));
    }
    virtual void ShutdownModule() override
    {
        FTSTicker::GetCoreTicker().RemoveTicker(TickHandle);
        if(Owner.IsValid() && Overlay.IsValid()) Owner->RemoveViewportWidgetContent(Overlay.ToSharedRef());
        Overlay.Reset();
    }
};
IMPLEMENT_PRIMARY_GAME_MODULE(FPhoneGalleryModule, DreamGUIDemo, "DreamGUIDemo");
#else
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, DreamGUIDemo, "DreamGUIDemo");
#endif
