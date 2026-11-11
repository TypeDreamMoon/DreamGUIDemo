// Copyright DreamMoon. All Rights Reserved.

#include "NocturneDeck.h"

#include "Components/StaticMeshComponent.h"
#include "Controls/DreamSlider.h"
#include "Core/Components/DreamCanvas.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWorldWidgetComponent.h"
#include "DreamUIBPLibrary.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Extensions/DreamUIRenderTargetGeometrySource.h"
#include "Extensions/DreamUIRenderTargetInteraction.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Settings/NocturneUI.h"
#include "UObject/ConstructorHelpers.h"

namespace NocturneDeckLocal
{
	// The test panel's design size, drawn at twice that into the target so the curved screen stays sharp up close.
	const FVector2D ScreenDesign(720.0, 440.0);
	constexpr int32 ScreenTargetScale = 2;
	constexpr double ScreenWorldScale = 0.2;
	const FVector ScreenAt(10.0, -150.0, 0.0);
	constexpr float ScreenArc = 70.0f;

	const FVector WallAt(-30.0, 120.0, -5.0);
	const FVector WallScale(0.12, 1.5, 1.3);
	const FVector MarkerAt(110.0, 120.0, 22.0);
	constexpr double MarkerScale = 0.3;
	// What the marker marks: a small crate behind the wall, under the foot of the marker's stem (the stem ends 154 design
	// units left of the panel's centre and 54 below it). Hidden from the front, seen once the camera rounds the wall.
	const FVector PodAt(115.0, 74.0, -8.0);
	constexpr double PodScale = 0.24;
	const FLinearColor PodLine(1.0f, 0.55f, 0.16f);

	const FVector EngineAt(170.0, 300.0, 10.0);
	constexpr double EngineScale = 0.11;
	// The pillar stands through the engine panel's left part, half in front of it and half behind: the panel is cut by
	// depth where it passes.
	const FVector PillarAt(146.0, 245.0, 0.0);
	const FVector PillarScale(0.25, 0.25, 1.6);

	const FVector Focus(60.0, 90.0, 10.0);

	// The marker's canvas against the scene's depth: 0 hidden behind the wall, 1 drawn over it; between, faint where
	// the wall is in front of it and full where nothing is.
	constexpr float MarkerThroughWall = 0.55f;

	// The curved screen's volume slider sweeps, so the screen is plainly live.
	constexpr float SweepSpeed = 0.6f;
}

ANocturneDeck::ANocturneDeck()
{
	PrimaryActorTick.bCanEverTick = true;
	using namespace NocturneDeckLocal;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> WallMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_Wall.M_Nocturne_Wall"));

	Wall = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Wall"));
	Wall->SetupAttachment(Root);
	Wall->SetStaticMesh(CubeMesh.Object);
	Wall->SetMaterial(0, WallMaterial.Object);
	Wall->SetRelativeLocation(WallAt);
	Wall->SetRelativeScale3D(WallScale);
	Wall->SetCastShadow(false);

	Pillar = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pillar"));
	Pillar->SetupAttachment(Root);
	Pillar->SetStaticMesh(CylinderMesh.Object);
	Pillar->SetMaterial(0, WallMaterial.Object);
	Pillar->SetRelativeLocation(PillarAt);
	Pillar->SetRelativeScale3D(PillarScale);
	Pillar->SetCastShadow(false);

	Pod = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Pod"));
	Pod->SetupAttachment(Root);
	Pod->SetStaticMesh(CubeMesh.Object);
	Pod->SetMaterial(0, WallMaterial.Object);
	Pod->SetRelativeLocation(PodAt);
	Pod->SetRelativeRotation(FRotator(0.0, 28.0, 0.0));
	Pod->SetRelativeScale3D(FVector(PodScale));
	Pod->SetCastShadow(false);

	Marker = CreateDefaultSubobject<UDreamWorldWidgetComponent>(TEXT("Marker"));
	Marker->SetupAttachment(Root);
	Marker->SetRelativeLocation(MarkerAt);
	Marker->SetRelativeScale3D(FVector(MarkerScale));

	EnginePanel = CreateDefaultSubobject<UDreamWorldWidgetComponent>(TEXT("EnginePanel"));
	EnginePanel->SetupAttachment(Root);
	EnginePanel->SetRelativeLocation(EngineAt);
	EnginePanel->SetRelativeRotation(FRotator(0.0, -24.0, 0.0));
	EnginePanel->SetRelativeScale3D(FVector(EngineScale));

	// Soft references, loaded in BeginPlay: see ANocturneSpace's constructor for why.
	ScreenClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_TestPanel.WBP_Nocturne_TestPanel_C")));
	MarkerClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_Marker.WBP_Nocturne_Marker_C")));
	EnginePanelClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_MusicBox.WBP_Nocturne_MusicBox_C")));
}

FVector ANocturneDeck::GetFocus() const
{
	return GetActorTransform().TransformPosition(NocturneDeckLocal::Focus);
}

void ANocturneDeck::BeginPlay()
{
	Marker->SetBackend(EDreamWorldWidgetBackend::DreamUIRenderer);
	Marker->SetDrawSize(FVector2D(360.0, 140.0));
	Marker->SetWidgetClass(MarkerClass.LoadSynchronous());

	EnginePanel->SetBackend(EDreamWorldWidgetBackend::UERenderer);
	EnginePanel->SetDrawSize(FVector2D(1920.0, 1080.0));
	EnginePanel->SetWidgetClass(EnginePanelClass.LoadSynchronous());

	// The crate in the marker's amber.
	if (UMaterialInstanceDynamic* PodMaterial = Pod->CreateDynamicMaterialInstance(0))
	{
		PodMaterial->SetVectorParameterValue(TEXT("LineColor"), NocturneDeckLocal::PodLine);
		PodMaterial->SetScalarParameterValue(TEXT("GridCells"), 3.0f);
		PodMaterial->SetScalarParameterValue(TEXT("LineStrength"), 1.6f);
	}

	Super::BeginPlay();
	BuildCurvedScreen();
}

void ANocturneDeck::BuildCurvedScreen()
{
	using namespace NocturneDeckLocal;
	UWorld* World = GetWorld();
	UClass* Loaded = ScreenClass.LoadSynchronous();
	if (World == nullptr || Loaded == nullptr)
	{
		return;
	}
	const FIntPoint Size(static_cast<int32>(ScreenDesign.X) * ScreenTargetScale, static_cast<int32>(ScreenDesign.Y) * ScreenTargetScale);
	ScreenTarget = NewObject<UTextureRenderTarget2D>(this, NAME_None, RF_Transient);
	ScreenTarget->AddressX = TextureAddress::TA_Clamp;
	ScreenTarget->AddressY = TextureAddress::TA_Clamp;
	ScreenTarget->ClearColor = FLinearColor::Transparent;
	ScreenTarget->InitCustomFormat(Size.X, Size.Y, EPixelFormat::PF_B8G8R8A8, false);

	// A root drawn into the target, the size of the target; the test panel inside it at twice its design size.
	ScreenRoot = UDreamUIBPLibrary::ConstructWidget(World, TEXT("CurvedScreen"), nullptr);
	UDreamCanvas* Canvas = ScreenRoot != nullptr ? ScreenRoot->AddComponent<UDreamCanvas>() : nullptr;
	if (Canvas == nullptr)
	{
		return;
	}
	Canvas->SetRenderMode(EDreamRenderMode::RenderTarget);
	Canvas->SetRenderTargetSizeMode(EDreamCanvasRenderTargetSizeMode::CanvasFitToRenderTarget);
	Canvas->SetRenderTarget(ScreenTarget);
	ScreenContent = CreateDreamWidget<UDreamUserWidget>(World, Loaded, ScreenRoot);
	if (ScreenContent != nullptr)
	{
		ScreenContent->SetRenderScale(FVector(1.0, ScreenTargetScale, ScreenTargetScale));
	}

	FActorSpawnParameters Spawn;
	Spawn.ObjectFlags |= RF_Transient;
	ScreenActor = World->SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, Spawn);
	if (ScreenActor == nullptr)
	{
		return;
	}
	UDreamUIRenderTargetGeometrySource* Surface = NewObject<UDreamUIRenderTargetGeometrySource>(ScreenActor, TEXT("Surface"));
	// The canvas before registering: registering builds the mesh and the material from it.
	Surface->SetCanvas(Canvas);
	Surface->SetGeometryMode(EDreamUIRenderTargetGeometryMode::Cylinder);
	Surface->SetCylinderArcAngle(ScreenArc);
	Surface->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Surface->SetCollisionResponseToAllChannels(ECR_Block);
	ScreenActor->SetRootComponent(Surface);
	ScreenActor->AddInstanceComponent(Surface);
	Surface->RegisterComponent();
	FTransform At(FRotator::ZeroRotator, ScreenAt, FVector(ScreenWorldScale / ScreenTargetScale));
	Surface->SetWorldTransform(At * GetActorTransform());
	UDreamUIBPLibrary::AttachWidgetToSceneComponent(ScreenRoot, Surface);

	UDreamUIRenderTargetInteraction* Interaction = NewObject<UDreamUIRenderTargetInteraction>(ScreenActor, TEXT("Interaction"));
	ScreenActor->AddInstanceComponent(Interaction);
	Interaction->RegisterComponent();
}

void ANocturneDeck::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	Age += DeltaSeconds;
	if (ScreenContent != nullptr)
	{
		if (UDreamSlider* Volume = Cast<UDreamSlider>(ScreenContent->GetWidgetFromName(TEXT("Volume"))))
		{
			const float Value = 0.5f + 0.35f * FMath::Sin(Age * NocturneDeckLocal::SweepSpeed * UE_TWO_PI * 0.5f);
			Volume->SetValue(Value);
			NocturneUI::SetText(ScreenContent->GetWidgetFromName(TEXT("VolumeValue")),
				FText::AsCultureInvariant(FString::Printf(TEXT("%03d"), FMath::RoundToInt(Value * 100.0f))));
		}
	}
	// The marker's tree loads with the component; once it is there, its canvas shows through what stands in front of it.
	if (!bMarkerOverDepth)
	{
		if (UDreamWidget* Tree = Marker->GetLoadedWidget())
		{
			if (UDreamCanvas* Canvas = Tree->GetComponent<UDreamCanvas>())
			{
				Canvas->SetBlendDepth(NocturneDeckLocal::MarkerThroughWall);
				bMarkerOverDepth = true;
			}
		}
	}
}
