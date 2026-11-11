// Copyright DreamMoon. All Rights Reserved.

#include "NocturneVoyage.h"

#include "NocturneCamera.h"
#include "Screens/NocturneHud.h"

#include "Camera/CameraComponent.h"
#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Components/DreamCanvas.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamScreenUISubsystem.h"
#include "DreamUIBPLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "SceneView.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogNocturneVoyage, Log, All);

namespace NocturneVoyageLocal
{
	struct FBeacon
	{
		/** From the start, in the voyage's own frame: ahead, right, up. */
		FVector Offset;
		const TCHAR* Name;
	};
	const FBeacon BeaconDefs[] = {
		{ FVector(5200.0, -900.0, 260.0), TEXT("旧港信标") },
		{ FVector(7600.0, 1300.0, -380.0), TEXT("补给站 · 启明") },
		{ FVector(10400.0, -300.0, 820.0), TEXT("第六象限") },
		{ FVector(4200.0, 1900.0, 600.0), TEXT("流星带") },
	};
	constexpr int32 BeaconCount = UE_ARRAY_COUNT(BeaconDefs);

	// From the dive's speed down to a cruise; how the ship sways while it cruises.
	constexpr double DiveSpeed = 1800.0;
	constexpr double CruiseSpeed = 300.0;
	constexpr float SlowSeconds = 1.8f;
	// Units a second to the m/s the HUD reads; units to its kilometres.
	constexpr double MetresPerUnit = 1.2;
	constexpr double KilometresPerUnit = 0.001;

	struct FVoyageEvent
	{
		float Time;
		int32 Kind; // 0 pickup, 1 hit, 2 objective
		int32 A;
		int32 B;
	};
	const FVoyageEvent Events[] = {
		{ 2.8f, 0, 5, 3 },   // night crystal x3
		{ 4.3f, 0, 15, 1 },  // star scale
		{ 5.6f, 1, 12, 0 },
		{ 6.2f, 1, 7, 0 },
		{ 7.4f, 0, 3, 1 },   // beacon core
		{ 7.6f, 2, 0, 0 },   // ...which was the objective
	};
	constexpr int32 EventCount = UE_ARRAY_COUNT(Events);

	double EaseOutCubic(double X)
	{
		X = FMath::Clamp(X, 0.0, 1.0);
		return 1.0 - FMath::Pow(1.0 - X, 3.0);
	}
}

ANocturneVoyage::ANocturneVoyage()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);
	NocturneCamera::Configure(Camera);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> StarMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_Star.M_Nocturne_Star"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_StarGlow.M_Nocturne_StarGlow"));
	for (int32 Index = 0; Index < NocturneVoyageLocal::BeaconCount; ++Index)
	{
		UStaticMeshComponent* Beacon = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Beacon%d"), Index));
		Beacon->SetupAttachment(Root);
		Beacon->SetStaticMesh(SphereMesh.Object);
		Beacon->SetMaterial(0, StarMaterial.Object);
		Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Beacon->SetCastShadow(false);
		Beacon->SetRelativeScale3D(FVector(0.5));
		Beacons.Add(Beacon);

		UMaterialBillboardComponent* Glow = CreateDefaultSubobject<UMaterialBillboardComponent>(*FString::Printf(TEXT("BeaconGlow%d"), Index));
		Glow->SetupAttachment(Beacon);
		Glow->AddElement(GlowMaterial.Object, nullptr, false, 260.0f, 260.0f, nullptr);
		BeaconGlows.Add(Glow);
	}

	// Loaded in BeginPlay: see ANocturneSpace's constructor for why a widget Blueprint is never loaded from one.
	HudClass = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_Hud.WBP_Nocturne_Hud_C")));
}

void ANocturneVoyage::Setup(const FVector& InStart, const FRotator& InHeading, int32 InSlot)
{
	Start = InStart;
	Heading = FRotator(0.0, InHeading.Yaw, 0.0);
	Slot = InSlot;
}

void ANocturneVoyage::BeginPlay()
{
	Super::BeginPlay();
	using namespace NocturneVoyageLocal;
	Position = Start;
	// The beacons hang ahead, in the voyage's own frame.
	const FRotationMatrix Frame(Heading);
	for (int32 Index = 0; Index < BeaconCount; ++Index)
	{
		const FVector Offset = BeaconDefs[Index].Offset;
		Beacons[Index]->SetWorldLocation(Start + Frame.GetUnitAxis(EAxis::X) * Offset.X + Frame.GetUnitAxis(EAxis::Y) * Offset.Y + FVector(0.0, 0.0, Offset.Z));
	}
	if (UClass* Loaded = HudClass.LoadSynchronous())
	{
		Hud = Cast<UNocturneHud>(UDreamUIBPLibrary::AddWidgetOfClassToViewport(this, Loaded, 120));
	}
	if (IsValid(Hud))
	{
		Hud->PlayBoot(Slot);
	}
	UE_LOG(LogNocturneVoyage, Log, TEXT("Voyage: slot %d, HUD %s"), Slot, IsValid(Hud) ? TEXT("up") : TEXT("FAILED"));
}

void ANocturneVoyage::Tick(float DeltaSeconds)
{
	using namespace NocturneVoyageLocal;
	Super::Tick(DeltaSeconds);
	VoyageTime += DeltaSeconds;

	// Out of the dive into a cruise, swaying a little.
	Speed = FMath::Lerp(DiveSpeed, CruiseSpeed, EaseOutCubic(VoyageTime / SlowSeconds));
	const FRotator Sway(FMath::Sin(VoyageTime * 0.55) * 1.4, FMath::Sin(VoyageTime * 0.37) * 2.2, FMath::Sin(VoyageTime * 0.45) * 3.0);
	const FRotator Facing = Heading + Sway;
	Position += Facing.Vector() * Speed * DeltaSeconds;
	Camera->SetWorldLocationAndRotation(Position, Facing);

	if (IsValid(Hud))
	{
		const float Thrust = static_cast<float>(0.35 + 0.5 * (1.0 - FMath::Clamp(VoyageTime / SlowSeconds, 0.0f, 1.0f)) + 0.05 * FMath::Sin(VoyageTime * 2.3));
		Hud->SetFlight(static_cast<float>(Speed * MetresPerUnit), static_cast<float>(FRotator::ClampAxis(Facing.Yaw)), Position, Thrust);
		Hud->SetEnergy(0.64f - 0.01f * VoyageTime);
		Hud->SetObjectiveDistance(static_cast<float>((Beacons[0]->GetComponentLocation() - Position).Size() * KilometresPerUnit));
		while (NextEvent < EventCount && VoyageTime >= Events[NextEvent].Time)
		{
			const FVoyageEvent& Event = Events[NextEvent++];
			if (Event.Kind == 0)
			{
				Hud->ShowToast(Event.A, Event.B);
			}
			else if (Event.Kind == 1)
			{
				Hud->Hit(Event.A);
			}
			else
			{
				Hud->CompleteObjective();
			}
		}
		UpdateMarkers();
	}
}

void ANocturneVoyage::UpdateMarkers()
{
	using namespace NocturneVoyageLocal;
	FVector2D Viewport(0.0, 0.0);
	if (const UGameViewportClient* Client = GetWorld()->GetGameViewport())
	{
		Client->GetViewportSize(Viewport);
	}
	UDreamScreenUISubsystem* ScreenUI = UDreamScreenUISubsystem::Get(GetWorld());
	UDreamWidget* ScreenRoot = IsValid(ScreenUI) ? ScreenUI->GetOrCreateScreenRootForUserIndex(0) : nullptr;
	UDreamCanvas* RootCanvas = IsValid(ScreenRoot) ? ScreenRoot->GetComponent<UDreamCanvas>() : nullptr;
	if (Viewport.X <= 0.0 || RootCanvas == nullptr)
	{
		return;
	}
	FMinimalViewInfo View;
	Camera->GetCameraView(0.0f, View);
	View.AspectRatio = Viewport.X / Viewport.Y;
	FMatrix ViewMatrix, ProjectionMatrix, ViewProjection;
	UGameplayStatics::GetViewProjectionMatrix(View, ViewMatrix, ProjectionMatrix, ViewProjection);
	const FVector Ahead = View.Rotation.Vector();

	for (int32 Index = 0; Index < BeaconCount; ++Index)
	{
		const FVector Where = Beacons[Index]->GetComponentLocation();
		FVector2D OnViewport;
		FVector2D OnCanvas;
		const bool bAhead = FVector::DotProduct(Where - View.Location, Ahead) > 0.0;
		const bool bShown = bAhead
			&& FSceneView::ProjectWorldToScreen(Where, FIntRect(0, 0, FMath::RoundToInt(Viewport.X), FMath::RoundToInt(Viewport.Y)), ViewProjection, OnViewport)
			&& RootCanvas->ConvertPositionFromViewportToCanvas(OnViewport, OnCanvas);
		// Bottom-left origin out of the conversion, centre origin into the markers' layer.
		const FVector2D Centred = OnCanvas - FVector2D(ScreenRoot->GetWidth() * 0.5, ScreenRoot->GetHeight() * 0.5);
		// Clear of the edges by enough for the name and the distance under and over the bracket.
		const bool bOnScreen = bShown && FMath::Abs(Centred.X) < ScreenRoot->GetWidth() * 0.5 - 130.0 && FMath::Abs(Centred.Y) < ScreenRoot->GetHeight() * 0.5 - 90.0;
		Hud->SetMarker(Index, bOnScreen, Centred, BeaconDefs[Index].Name, static_cast<float>((Where - Position).Size() * KilometresPerUnit));
	}
}
