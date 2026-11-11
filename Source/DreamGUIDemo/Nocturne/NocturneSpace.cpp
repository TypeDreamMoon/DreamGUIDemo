// Copyright DreamMoon. All Rights Reserved.

#include "NocturneSpace.h"

#include "NocturneStars.h"

#include "Components/MaterialBillboardComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Components/DreamWidget.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWorldWidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace NocturneSpace
{
	// The test panel's design size (Nocturne_TestPanel.dui) and how big one design unit is in the world.
	const FVector2D PanelDesignSize(720.0, 440.0);
	constexpr double PanelScale = 0.2;

	// Far stars, well behind the constellation: depth, and nothing to click.
	const FVector FarStarSpots[] = {
		FVector(2600.0, -1300.0, 700.0),
		FVector(3400.0, 900.0, 1100.0),
		FVector(3000.0, 1600.0, -500.0),
		FVector(3800.0, -400.0, -1000.0),
	};
	const float FarBreathSpeeds[] = { 0.9f, 1.3f, 1.7f, 1.1f };

	// The engine's sphere is 100 units across, its cylinder 100 tall and 100 across, centred.
	constexpr double StarScale = 0.12;
	constexpr double FarStarScale = 0.9;
	constexpr double LineThickness = 0.006;
	constexpr float StarGlowSize = 110.0f;
	constexpr float FarGlowSize = 520.0f;

	// A lit star: how much brighter, how much larger its glow; its lines.
	constexpr float StarStrength = 9.0f;
	constexpr float StarStrengthLit = 24.0f;
	constexpr float GlowGrowth = 0.7f;
	constexpr float LineStrength = 0.5f;
	constexpr float LineStrengthLit = 2.2f;
}

ANocturneSpace::ANocturneSpace()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SkyMesh(TEXT("/Engine/EngineSky/SM_SkySphere.SM_SkySphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SkyMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_Sky.M_Nocturne_Sky"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> StarMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_Star.M_Nocturne_Star"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> GlowMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_StarGlow.M_Nocturne_StarGlow"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> LineMaterial(TEXT("/Game/Nocturne/Materials/M_Nocturne_Line.M_Nocturne_Line"));

	auto MakeMesh = [this](const TCHAR* Name, UStaticMesh* Mesh, UMaterialInterface* Material)
	{
		UStaticMeshComponent* Component = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Component->SetupAttachment(Root);
		Component->SetStaticMesh(Mesh);
		Component->SetMaterial(0, Material);
		// Nothing here may stop a pointer ray: the panels and labels are clicked through the space.
		Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component->SetCastShadow(false);
		return Component;
	};
	auto MakeGlow = [this](const TCHAR* Name, const FVector& Where, float Size)
	{
		// A camera-facing sprite, sized in the world so it shrinks with distance like the star.
		UMaterialBillboardComponent* Glow = CreateDefaultSubobject<UMaterialBillboardComponent>(Name);
		Glow->SetupAttachment(Root);
		Glow->SetRelativeLocation(Where);
		Glow->AddElement(GlowMaterial.Object, nullptr, false, Size, Size, nullptr);
		return Glow;
	};

	// The sky is a function of the view direction alone, so its size only has to enclose everything else.
	Sky = MakeMesh(TEXT("Sky"), SkyMesh.Object, SkyMaterial.Object);
	Sky->SetRelativeScale3D(FVector(400.0));

	for (int32 Index = 0; Index < NocturneStars::Count; ++Index)
	{
		const FVector Where = NocturneStars::Stars[Index].Location;
		UStaticMeshComponent* Star = MakeMesh(*FString::Printf(TEXT("Star%d"), Index), SphereMesh.Object, StarMaterial.Object);
		Star->SetRelativeLocation(Where);
		Star->SetRelativeScale3D(FVector(NocturneSpace::StarScale));
		Stars.Add(Star);
		Glows.Add(MakeGlow(*FString::Printf(TEXT("Glow%d"), Index), Where, NocturneSpace::StarGlowSize));
	}

	int32 Far = 0;
	for (const FVector& Spot : NocturneSpace::FarStarSpots)
	{
		UStaticMeshComponent* Star = MakeMesh(*FString::Printf(TEXT("FarStar%d"), Far), SphereMesh.Object, StarMaterial.Object);
		Star->SetRelativeLocation(Spot);
		Star->SetRelativeScale3D(FVector(NocturneSpace::FarStarScale));
		FarStars.Add(Star);
		MakeGlow(*FString::Printf(TEXT("FarGlow%d"), Far), Spot, NocturneSpace::FarGlowSize);
		++Far;
	}

	// The constellation's lines: thin cylinders from star to star.
	int32 LineIndex = 0;
	for (const auto& Pair : NocturneStars::Lines)
	{
		const FVector From = NocturneStars::Stars[Pair[0]].Location;
		const FVector To = NocturneStars::Stars[Pair[1]].Location;
		const FVector Along = To - From;
		UStaticMeshComponent* Line = MakeMesh(*FString::Printf(TEXT("Line%d"), LineIndex++), CylinderMesh.Object, LineMaterial.Object);
		Line->SetRelativeLocation((From + To) * 0.5);
		Line->SetRelativeRotation(FRotationMatrix::MakeFromZ(Along.GetSafeNormal()).Rotator());
		Line->SetRelativeScale3D(FVector(NocturneSpace::LineThickness, NocturneSpace::LineThickness, Along.Size() / 100.0));
		Lines.Add(Line);
	}

	// The test panel stands at the origin, beside the Settings star.
	Panel = CreateDefaultSubobject<UDreamWorldWidgetComponent>(TEXT("Panel"));
	Panel->SetupAttachment(Root);
	// Turned a little away, so the director's clicks land on a panel seen at an angle.
	Panel->SetRelativeRotation(FRotator(0.0, -18.0, 0.0));
	Panel->SetRelativeScale3D(FVector(NocturneSpace::PanelScale));
	// A soft reference, loaded in BeginPlay. A widget Blueprint must never be loaded from a constructor: the class
	// default object is built while the editor starts, before the DreamGUI editor module that defines the Blueprint's
	// own class is loaded, and the Blueprint then fails to load for the whole session.
	PanelClassToLoad = TSoftClassPtr<UDreamUserWidget>(FSoftObjectPath(TEXT("/Game/Nocturne/UI/WBP_Nocturne_TestPanel.WBP_Nocturne_TestPanel_C")));
}

void ANocturneSpace::BeginPlay()
{
	Panel->SetBackend(EDreamWorldWidgetBackend::DreamUIRenderer);
	Panel->SetDrawSize(NocturneSpace::PanelDesignSize);
	Panel->SetWidgetClass(PanelClassToLoad.LoadSynchronous());
	// Hidden unless a shot shows it (the test shot brings it in).
	Panel->SetWidgetOpacity(0.0f);

	Super::BeginPlay();

	StarFocus.Init(0.0f, NocturneStars::Count);
	LineReveal.Init(1.0f, Lines.Num());
	for (int32 Index = 0; Index < Stars.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Material = Stars[Index]->CreateDynamicMaterialInstance(0);
		if (Material != nullptr)
		{
			Material->SetScalarParameterValue(TEXT("Strength"), NocturneSpace::StarStrength);
			// Each star breathes at its own pace, fixed so every take is the same.
			Material->SetScalarParameterValue(TEXT("BreathSpeed"), 1.0f + 0.23f * Index);
		}
		StarMaterials.Add(Material);
	}
	for (int32 Index = 0; Index < FarStars.Num(); ++Index)
	{
		if (UMaterialInstanceDynamic* Material = FarStars[Index]->CreateDynamicMaterialInstance(0))
		{
			Material->SetScalarParameterValue(TEXT("Strength"), 6.0f);
			Material->SetScalarParameterValue(TEXT("BreathSpeed"), NocturneSpace::FarBreathSpeeds[Index]);
		}
	}
	for (int32 Index = 0; Index < Lines.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Pulse = Lines[Index]->CreateDynamicMaterialInstance(0);
		if (Pulse != nullptr)
		{
			// Each line's pulse at its own point of the cycle, so they do not all run at once.
			Pulse->SetScalarParameterValue(TEXT("PulseOffset"), FMath::Frac(Index * 0.37f));
		}
		LineMaterials.Add(Pulse);
	}
	ApplyLines();
}

FVector ANocturneSpace::GetStarLocation(int32 InIndex) const
{
	return Stars.IsValidIndex(InIndex) ? Stars[InIndex]->GetComponentLocation() : FVector::ZeroVector;
}

FVector ANocturneSpace::GetFocusStarLocation() const
{
	return GetStarLocation(NocturneStars::SettingsStar);
}

FVector ANocturneSpace::GetPanelLocation() const
{
	return Panel->GetComponentLocation();
}

void ANocturneSpace::SetStarFocus(int32 InIndex, float InAmount)
{
	if (!StarFocus.IsValidIndex(InIndex) || FMath::IsNearlyEqual(StarFocus[InIndex], InAmount, 1e-3f))
	{
		return;
	}
	using namespace NocturneSpace;
	StarFocus[InIndex] = InAmount;
	if (UMaterialInstanceDynamic* Material = StarMaterials.IsValidIndex(InIndex) ? StarMaterials[InIndex].Get() : nullptr)
	{
		Material->SetScalarParameterValue(TEXT("Strength"), FMath::Lerp(StarStrength, StarStrengthLit, InAmount));
	}
	if (Glows.IsValidIndex(InIndex))
	{
		Glows[InIndex]->SetRelativeScale3D(FVector(1.0 + GlowGrowth * InAmount));
	}
	ApplyLines();
}

void ANocturneSpace::SetLineReveal(int32 InLine, float InAmount)
{
	InAmount = FMath::Clamp(InAmount, 0.0f, 1.0f);
	if (!Lines.IsValidIndex(InLine) || !LineReveal.IsValidIndex(InLine) || FMath::IsNearlyEqual(LineReveal[InLine], InAmount, 1e-4f))
	{
		return;
	}
	LineReveal[InLine] = InAmount;
	const FVector From = NocturneStars::Stars[NocturneStars::Lines[InLine][0]].Location;
	const FVector Along = NocturneStars::Stars[NocturneStars::Lines[InLine][1]].Location - From;
	UStaticMeshComponent* Line = Lines[InLine];
	// The cylinder is centred: half of what is drawn ahead of the first star, scaled to that length.
	Line->SetRelativeLocation(From + Along * (0.5 * InAmount));
	Line->SetRelativeScale3D(FVector(NocturneSpace::LineThickness, NocturneSpace::LineThickness, FMath::Max(Along.Size() * InAmount, 0.01) / 100.0));
	Line->SetVisibility(InAmount > 0.0f);
}

void ANocturneSpace::ApplyLines()
{
	using namespace NocturneSpace;
	for (int32 Index = 0; Index < LineMaterials.Num(); ++Index)
	{
		UMaterialInstanceDynamic* Material = LineMaterials[Index].Get();
		if (Material == nullptr || !StarFocus.IsValidIndex(NocturneStars::Lines[Index][1]))
		{
			continue;
		}
		const float Lit = FMath::Max(StarFocus[NocturneStars::Lines[Index][0]], StarFocus[NocturneStars::Lines[Index][1]]);
		Material->SetScalarParameterValue(TEXT("Strength"), FMath::Lerp(LineStrength, LineStrengthLit, Lit));
	}
}

UDreamWidget* ANocturneSpace::FindPanelWidget(const FString& InName) const
{
	UDreamWidget* Tree = Panel->GetLoadedWidget();
	if (!IsValid(Tree))
	{
		return nullptr;
	}
	if (const UDreamUserWidget* User = Cast<UDreamUserWidget>(Tree))
	{
		if (UDreamWidget* Found = User->GetWidgetFromName(FName(*InName)))
		{
			return Found;
		}
	}
	return Tree->FindChildByDisplayName(InName, true);
}
