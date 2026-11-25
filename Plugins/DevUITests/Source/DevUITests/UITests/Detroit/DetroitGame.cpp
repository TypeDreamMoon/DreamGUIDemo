#include "DetroitGame.h"

#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/Components/DreamCanvas.h"
#include "Core/DreamScreenUISubsystem.h"
#include "Core/DreamUserWidget.h"
#include "Core/DreamWorldWidgetComponent.h"
#include "DetroitTouchControls.h"
#include "DetroitUI.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Event/DreamUIInputModeLibrary.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace DetroitGameLocal
{
	/** Connor's looks, by index: colour (sRGB), roughness, metalness, emissive (linear, a = strength). */
	struct FLook
	{
		FColor Color;
		float Roughness;
		float Metallic;
		FLinearColor Emissive;
	};
	enum ELook : int32 { LookJacket, LookTrousers, LookSkin, LookHair, LookShirt, LookTie, LookShoes, LookBand, LookStrip };
	const FLook Looks[] = {
		{ FColor(43, 48, 56), 0.55f, 0.0f, FLinearColor::Transparent },
		{ FColor(28, 31, 37), 0.60f, 0.0f, FLinearColor::Transparent },
		{ FColor(214, 174, 149), 0.50f, 0.0f, FLinearColor::Transparent },
		{ FColor(42, 29, 22), 0.70f, 0.0f, FLinearColor::Transparent },
		{ FColor(232, 234, 236), 0.60f, 0.0f, FLinearColor::Transparent },
		{ FColor(21, 23, 27), 0.40f, 0.0f, FLinearColor::Transparent },
		{ FColor(16, 17, 19), 0.30f, 0.2f, FLinearColor::Transparent },
		{ FColor(238, 242, 245), 0.40f, 0.0f, FLinearColor(1.0f, 1.0f, 1.0f, 0.15f) },
		{ FColor(20, 40, 70), 0.30f, 0.0f, FLinearColor(0.05f, 0.45f, 1.0f, 6.0f) },
	};

	UMaterialInstanceDynamic* MakeLitInstance(UMaterialInterface* Lit, UObject* Outer, const FColor& Color, float Roughness, float Metallic,
		const FLinearColor& Emissive)
	{
		if (Lit == nullptr)
		{
			return nullptr;
		}
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Lit, Outer);
		Instance->SetVectorParameterValue(TEXT("Color"), DetroitUI::Linear(Color));
		Instance->SetScalarParameterValue(TEXT("Roughness"), Roughness);
		Instance->SetScalarParameterValue(TEXT("Metallic"), Metallic);
		Instance->SetVectorParameterValue(TEXT("Emissive"), Emissive);
		return Instance;
	}

	FVector GetCameraLocation(const UObject* Context)
	{
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(Context, 0);
		return Camera != nullptr ? Camera->GetCameraLocation() : FVector::ZeroVector;
	}

	FRotator GetCameraRotation(const UObject* Context)
	{
		const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(Context, 0);
		return Camera != nullptr ? Camera->GetCameraRotation() : FRotator::ZeroRotator;
	}

	/** A world-space panel's settings: its size in design pixels, what that is in the room, and how much of it shows through walls. */
	void SetUpPanel(UDreamWorldWidgetComponent* Component, const TCHAR* ClassPath, const FVector2D& DesignSize, float Scale,
		EDreamWorldWidgetBackend Backend, float BlendDepth)
	{
		if (Component == nullptr)
		{
			return;
		}
		Component->SetBackend(Backend);
		Component->SetDrawSize(DesignSize);
		Component->SetRelativeScale3D(FVector(Scale));
		UClass* WidgetClass = LoadClass<UDreamUserWidget>(nullptr, ClassPath);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Detroit UI test: %s is missing; run DUI/UITests/Detroit/_Tools/make_detroit_assets.py"), ClassPath);
			return;
		}
		Component->SetWidgetClass(WidgetClass);
		if (UDreamCanvas* Canvas = Component->GetLoadedCanvas())
		{
			Canvas->SetBlendDepth(BlendDepth);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

ADetroitCharacter::ADetroitCharacter()
{
	using namespace DetroitGameLocal;
	PrimaryActorTick.bCanEverTick = true;
	GetCapsuleComponent()->InitCapsuleSize(32.0f, 88.0f);
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	UCharacterMovementComponent* Movement = GetCharacterMovement();
	Movement->bOrientRotationToMovement = true;
	Movement->RotationRate = FRotator(0.0, 400.0, 0.0);
	Movement->MaxWalkSpeed = 170.0f;
	Movement->BrakingDecelerationWalking = 900.0f;
	Movement->MaxAcceleration = 900.0f;

	Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	Arm->SetupAttachment(GetRootComponent());
	Arm->SetRelativeLocation(FVector(0.0, 0.0, 58.0));
	Arm->TargetArmLength = 270.0f;
	Arm->SocketOffset = FVector(0.0, 55.0, 18.0);
	Arm->bUsePawnControlRotation = true;
	Arm->bEnableCameraLag = true;
	Arm->CameraLagSpeed = 10.0f;
	Arm->ProbeSize = 10.0f;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Arm, USpringArmComponent::SocketName);
	Camera->SetFieldOfView(70.0f);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeFinder(DetroitUI::CubeMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(DetroitUI::CylinderMesh);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereFinder(DetroitUI::SphereMesh);
	UStaticMesh* Cube = CubeFinder.Object;
	UStaticMesh* Cylinder = CylinderFinder.Object;
	UStaticMesh* Sphere = SphereFinder.Object;

	// The basic shapes are 100 units across, centred: a scale of 0.1 is 10 cm. The feet are at the capsule's bottom, -88.
	Body = CreateDefaultSubobject<USceneComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	AddPart(TEXT("Pelvis"), Cube, Body, FVector(0.0, 0.0, -6.0), FVector(0.22, 0.36, 0.16), FRotator::ZeroRotator, LookTrousers);
	HipL = AddJoint(TEXT("HipL"), Body, FVector(0.0, -10.0, -12.0));
	HipR = AddJoint(TEXT("HipR"), Body, FVector(0.0, 10.0, -12.0));
	for (USceneComponent* Hip : { HipL.Get(), HipR.Get() })
	{
		const FString Side = Hip == HipL ? TEXT("L") : TEXT("R");
		AddPart(*(TEXT("Leg") + Side), Cylinder, Hip, FVector(0.0, 0.0, -36.0), FVector(0.14, 0.14, 0.72), FRotator::ZeroRotator, LookTrousers);
		AddPart(*(TEXT("Shoe") + Side), Cube, Hip, FVector(5.0, 0.0, -73.0), FVector(0.27, 0.11, 0.07), FRotator::ZeroRotator, LookShoes);
	}
	AddPart(TEXT("Torso"), Cube, Body, FVector(0.0, 0.0, 26.0), FVector(0.24, 0.42, 0.58), FRotator::ZeroRotator, LookJacket);
	AddPart(TEXT("JacketHem"), Cube, Body, FVector(0.0, 0.0, -1.0), FVector(0.25, 0.41, 0.10), FRotator::ZeroRotator, LookJacket);
	AddPart(TEXT("Shirt"), Cube, Body, FVector(11.8, 0.0, 47.0), FVector(0.02, 0.13, 0.15), FRotator::ZeroRotator, LookShirt);
	AddPart(TEXT("Tie"), Cube, Body, FVector(12.4, 0.0, 44.0), FVector(0.02, 0.035, 0.16), FRotator::ZeroRotator, LookTie);
	AddPart(TEXT("Collar"), Cube, Body, FVector(-1.0, 0.0, 55.5), FVector(0.22, 0.30, 0.05), FRotator::ZeroRotator, LookJacket);
	ShoulderL = AddJoint(TEXT("ShoulderL"), Body, FVector(0.0, -26.0, 51.0));
	ShoulderR = AddJoint(TEXT("ShoulderR"), Body, FVector(0.0, 26.0, 51.0));
	for (USceneComponent* Shoulder : { ShoulderL.Get(), ShoulderR.Get() })
	{
		const FString Side = Shoulder == ShoulderL ? TEXT("L") : TEXT("R");
		AddPart(*(TEXT("Arm") + Side), Cylinder, Shoulder, FVector(0.0, 0.0, -29.0), FVector(0.11, 0.11, 0.60), FRotator::ZeroRotator, LookJacket);
		AddPart(*(TEXT("Hand") + Side), Sphere, Shoulder, FVector(0.0, 0.0, -62.0), FVector(0.085, 0.07, 0.10), FRotator::ZeroRotator, LookSkin);
	}
	// Connor's armband: a white band with a light strip, on the right arm.
	AddPart(TEXT("Band"), Cylinder, ShoulderR, FVector(0.0, 0.0, -14.0), FVector(0.118, 0.118, 0.055), FRotator::ZeroRotator, LookBand);
	AddPart(TEXT("Strip"), Cylinder, ShoulderR, FVector(0.0, 0.0, -10.5), FVector(0.12, 0.12, 0.012), FRotator::ZeroRotator, LookStrip);
	AddPart(TEXT("Neck"), Cylinder, Body, FVector(0.0, 0.0, 61.0), FVector(0.085, 0.085, 0.10), FRotator::ZeroRotator, LookSkin);
	AddPart(TEXT("Head"), Sphere, Body, FVector(1.0, 0.0, 74.0), FVector(0.20, 0.18, 0.24), FRotator::ZeroRotator, LookSkin);
	AddPart(TEXT("Hair"), Sphere, Body, FVector(-1.5, 0.0, 79.0), FVector(0.21, 0.19, 0.16), FRotator::ZeroRotator, LookHair);
	// The LED, on his right temple; its own instance, made at BeginPlay.
	Led = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Led"));
	Led->SetStaticMesh(Cylinder);
	Led->SetupAttachment(Body);
	Led->SetRelativeLocation(FVector(4.0, 9.2, 77.0));
	Led->SetRelativeRotation(FRotator(0.0, 0.0, 90.0));
	Led->SetRelativeScale3D(FVector(0.022, 0.022, 0.006));
	Led->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Led->SetCastShadow(false);

	// The jacket's print on his back: a hierarchy in the world, drawn by UE's renderer so it sits in the scene as cloth.
	Jacket = CreateDefaultSubobject<UDreamWorldWidgetComponent>(TEXT("JacketPrint"));
	Jacket->SetupAttachment(Body);
	Jacket->SetRelativeLocation(FVector(-12.4, 0.0, 34.0));
}

USceneComponent* ADetroitCharacter::AddJoint(FName Name, USceneComponent* Parent, const FVector& Location)
{
	USceneComponent* Joint = CreateDefaultSubobject<USceneComponent>(Name);
	Joint->SetupAttachment(Parent);
	Joint->SetRelativeLocation(Location);
	return Joint;
}

UStaticMeshComponent* ADetroitCharacter::AddPart(FName Name, UStaticMesh* PartMesh, USceneComponent* Parent, const FVector& Location,
	const FVector& Scale, const FRotator& Rotation, int32 Look)
{
	UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
	Part->SetStaticMesh(PartMesh);
	Part->SetupAttachment(Parent);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Parts.Add(Part);
	PartLooks.Add(Look);
	return Part;
}

void ADetroitCharacter::BeginPlay()
{
	using namespace DetroitGameLocal;
	Super::BeginPlay();
	LitMaterial = LoadObject<UMaterialInterface>(nullptr, DetroitUI::LitMaterial);
	TArray<UMaterialInstanceDynamic*> Instances;
	for (const FLook& Look : Looks)
	{
		Instances.Add(MakeLitInstance(LitMaterial, this, Look.Color, Look.Roughness, Look.Metallic, Look.Emissive));
	}
	for (int32 Index = 0; Index < Parts.Num(); ++Index)
	{
		if (Parts[Index] != nullptr && Instances.IsValidIndex(PartLooks[Index]) && Instances[PartLooks[Index]] != nullptr)
		{
			Parts[Index]->SetMaterial(0, Instances[PartLooks[Index]]);
		}
	}
	LedMaterial = MakeLitInstance(LitMaterial, this, FColor(10, 20, 30), 0.2f, 0.0f, FLinearColor(0.05f, 0.55f, 1.0f, 8.0f));
	if (LedMaterial != nullptr)
	{
		Led->SetMaterial(0, LedMaterial);
	}
	SetUpPanel(Jacket, DetroitUI::JacketClass, FVector2D(400.0, 300.0), 0.085f, EDreamWorldWidgetBackend::UERenderer, 0.0f);

	// The camera: a held exposure for a dim room, a little grain and vignette, and the analysis mode as a blendable.
	FPostProcessSettings& Post = Camera->PostProcessSettings;
	Post.bOverride_AutoExposureMethod = true;
	Post.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
	Post.bOverride_AutoExposureMinBrightness = true;
	Post.AutoExposureMinBrightness = 0.0f;
	Post.bOverride_AutoExposureMaxBrightness = true;
	Post.AutoExposureMaxBrightness = 2.0f;
	Post.bOverride_AutoExposureBias = true;
	Post.AutoExposureBias = -0.6f;
	Post.bOverride_VignetteIntensity = true;
	Post.VignetteIntensity = 0.55f;
	Post.bOverride_FilmGrainIntensity = true;
	Post.FilmGrainIntensity = 0.12f;
	Post.bOverride_ColorSaturation = true;
	Post.ColorSaturation = FVector4(0.86, 0.9, 1.0, 1.0);
	if (UMaterialInterface* AnalysisMaterial = LoadObject<UMaterialInterface>(nullptr, DetroitUI::AnalysisMaterial))
	{
		Analysis = UMaterialInstanceDynamic::Create(AnalysisMaterial, this);
		Analysis->SetScalarParameterValue(TEXT("Blend"), 0.0f);
		Post.WeightedBlendables.Array.Add(FWeightedBlendable(1.0f, Analysis));
	}
}

void ADetroitCharacter::SetAnalyzing(bool bInAnalyzing)
{
	bAnalyzing = bInAnalyzing;
}

void ADetroitCharacter::SetRunning(bool bInRunning)
{
	GetCharacterMovement()->MaxWalkSpeed = bInRunning ? 360.0f : 170.0f;
}

void ADetroitCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Clock += DeltaTime;
	// A walk: legs and arms swing in counterphase with the distance covered, the body bobs on each step.
	const float Speed = GetVelocity().Size2D();
	const float Gait = FMath::Clamp(Speed / 170.0f, 0.0f, 2.2f);
	WalkPhase += DeltaTime * Speed * 0.052f;
	const float Swing = FMath::Sin(WalkPhase) * FMath::Min(Gait, 1.5f) * 26.0f;
	HipL->SetRelativeRotation(FRotator(Swing, 0.0, 0.0));
	HipR->SetRelativeRotation(FRotator(-Swing, 0.0, 0.0));
	ShoulderL->SetRelativeRotation(FRotator(-Swing * 0.6f, 0.0, 0.0));
	ShoulderR->SetRelativeRotation(FRotator(Swing * 0.6f, 0.0, 0.0));
	Body->SetRelativeLocation(FVector(0.0, 0.0, FMath::Abs(FMath::Cos(WalkPhase)) * 1.6f * FMath::Min(Gait, 1.0f) + FMath::Sin(Clock * 1.7f) * 0.3f));

	// The LED: blue, turning yellow and quickening while he analyzes.
	LedBlend = FMath::FInterpConstantTo(LedBlend, bAnalyzing ? 1.0f : 0.0f, DeltaTime, 3.0f);
	if (LedMaterial != nullptr)
	{
		const FLinearColor Color = FMath::Lerp(FLinearColor(0.05f, 0.55f, 1.0f), FLinearColor(1.0f, 0.62f, 0.02f), LedBlend);
		const float Pulse = 7.0f + 4.0f * FMath::Sin(Clock * FMath::Lerp(2.2f, 10.0f, LedBlend));
		LedMaterial->SetVectorParameterValue(TEXT("Emissive"), FLinearColor(Color.R, Color.G, Color.B, Pulse));
	}
	Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView, bAnalyzing ? 58.0f : 70.0f, DeltaTime, 3.0f));
}

// ---------------------------------------------------------------------------------------------------------------------

ADetroitPlayerController::ADetroitPlayerController()
{
	bShowMouseCursor = false;
}

void ADetroitPlayerController::BeginPlay()
{
	Super::BeginPlay();
	if (!IsLocalController() || !(FPlatformMisc::SupportsTouchInput() || FParse::Param(FCommandLine::Get(), TEXT("DemoTouch"))))
	{
		return;
	}
	UGameViewportClient* Viewport = GetWorld() != nullptr ? GetWorld()->GetGameViewport() : nullptr;
	if (Viewport == nullptr)
	{
		return;
	}
	const TWeakObjectPtr<ADetroitPlayerController> Weak(this);
	Touch = SNew(SDetroitTouchControls);
	// Only while Connor can be walked: not over the loading screen.
	Touch->SetVisibility(TAttribute<EVisibility>::CreateLambda([Weak]()
	{
		const ADetroitPlayerController* Self = Weak.Get();
		const ADetroitGameMode* Game = Self != nullptr && Self->GetWorld() != nullptr ? Self->GetWorld()->GetAuthGameMode<ADetroitGameMode>() : nullptr;
		return Game != nullptr && Game->IsInvestigating() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
	}));
	// Under the gallery's own buttons (10000), over the game.
	Viewport->AddViewportWidgetContent(Touch.ToSharedRef(), 8000);
	TouchViewport = Viewport;
}

void ADetroitPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
	if (TouchViewport.IsValid() && Touch.IsValid())
	{
		TouchViewport->RemoveViewportWidgetContent(Touch.ToSharedRef());
	}
	Touch.Reset();
	TouchViewport.Reset();
	Super::EndPlay(Reason);
}

void ADetroitPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	ADetroitGameMode* Game = GetWorld() != nullptr ? GetWorld()->GetAuthGameMode<ADetroitGameMode>() : nullptr;
	ADetroitCharacter* Connor = Cast<ADetroitCharacter>(GetPawn());
	if (Game == nullptr || Connor == nullptr || !Game->IsInvestigating())
	{
		if (Touch.IsValid())
		{
			Touch->Reset();
		}
		return;
	}
	auto Down = [this](const FKey& Key) { return IsInputKeyDown(Key) ? 1.0f : 0.0f; };
	const FVector2D TouchMove = Touch.IsValid() ? Touch->GetMove() : FVector2D::ZeroVector;
	const FVector2D TouchLook = Touch.IsValid() ? Touch->ConsumeLook() : FVector2D::ZeroVector;
	const EDetroitTouchAction TouchActions = Touch.IsValid() ? Touch->ConsumeActions() : EDetroitTouchAction::None;
	const int32 TouchClue = Touch.IsValid() ? Touch->ConsumeClue() : INDEX_NONE;
	// A drag across the look area, in Slate units: about a quarter turn for half the screen's width.
	const float TouchDegrees = 0.18f;

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	GetInputMouseDelta(MouseX, MouseY);
	FRotator Control = GetControlRotation();
	// The arrows look too, for a keyboard alone.
	const float KeyYaw = Down(EKeys::Right) - Down(EKeys::Left);
	const float KeyPitch = Down(EKeys::Up) - Down(EKeys::Down);
	Control.Yaw += MouseX * 2.2f + TouchLook.X * TouchDegrees + (GetInputAnalogKeyState(EKeys::Gamepad_RightX) + KeyYaw) * 120.0f * DeltaTime;
	const float Pitch = FRotator::NormalizeAxis(Control.Pitch) + MouseY * 2.2f - TouchLook.Y * TouchDegrees
		+ (GetInputAnalogKeyState(EKeys::Gamepad_RightY) + KeyPitch) * 80.0f * DeltaTime;
	Control.Pitch = FMath::Clamp(Pitch, -55.0f, 40.0f);
	Control.Roll = 0.0;
	SetControlRotation(Control);

	const float Forward = FMath::Clamp(Down(EKeys::W) - Down(EKeys::S) + GetInputAnalogKeyState(EKeys::Gamepad_LeftY) + float(TouchMove.Y), -1.0f, 1.0f);
	const float Right = FMath::Clamp(Down(EKeys::D) - Down(EKeys::A) + GetInputAnalogKeyState(EKeys::Gamepad_LeftX) + float(TouchMove.X), -1.0f, 1.0f);
	const FRotationMatrix Yaw(FRotator(0.0, Control.Yaw, 0.0));
	Connor->AddMovementInput(Yaw.GetUnitAxis(EAxis::X), Forward);
	Connor->AddMovementInput(Yaw.GetUnitAxis(EAxis::Y), Right);
	Connor->SetRunning(IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::Gamepad_LeftThumbstick) || (Touch.IsValid() && Touch->IsRunning()));

	// The touch controls take every touch on the screen, so a tap reaches the game only through their buttons.
	if (WasInputKeyJustPressed(EKeys::Q) || WasInputKeyJustPressed(EKeys::RightMouseButton) || WasInputKeyJustPressed(EKeys::Gamepad_RightTrigger)
		|| EnumHasAnyFlags(TouchActions, EDetroitTouchAction::Analysis))
	{
		Game->ToggleAnalysis();
	}
	if (WasInputKeyJustPressed(EKeys::E) || WasInputKeyJustPressed(EKeys::LeftMouseButton) || WasInputKeyJustPressed(EKeys::Gamepad_FaceButton_Bottom)
		|| EnumHasAnyFlags(TouchActions, EDetroitTouchAction::Analyze))
	{
		Game->TryAnalyze();
	}
	if (WasInputKeyJustPressed(EKeys::Tab) || WasInputKeyJustPressed(EKeys::Gamepad_RightShoulder)
		|| EnumHasAnyFlags(TouchActions, EDetroitTouchAction::Objectives))
	{
		Game->ShowObjectives();
	}
	// 1 to 5: straight to a clue, for looking at the demo without walking it.
	const FKey ClueKeys[] = { EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five };
	for (int32 Index = 0; Index < int32(UE_ARRAY_COUNT(ClueKeys)); ++Index)
	{
		if (WasInputKeyJustPressed(ClueKeys[Index]) || TouchClue == Index)
		{
			Game->GoToClue(Index);
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

ADetroitClue::ADetroitClue()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
	TagWidget = CreateDefaultSubobject<UDreamWorldWidgetComponent>(TEXT("Tag"));
	TagWidget->SetupAttachment(Root);
}

void ADetroitClue::Setup(const FDetroitClueSpec& InSpec)
{
	Spec = InSpec;
	if (UDetroitClueTag* Tag = GetTag())
	{
		Tag->SetTitle(Spec.TitleA, Spec.TitleB, Spec.Suffix);
	}
}

UDetroitClueTag* ADetroitClue::GetTag() const
{
	return TagWidget != nullptr ? Cast<UDetroitClueTag>(TagWidget->GetLoadedWidget()) : nullptr;
}

UDetroitAnalysisPanel* ADetroitClue::GetPanel() const
{
	return PanelWidget != nullptr ? Cast<UDetroitAnalysisPanel>(PanelWidget->GetLoadedWidget()) : nullptr;
}

void ADetroitClue::BeginPlay()
{
	using namespace DetroitGameLocal;
	Super::BeginPlay();
	// The tag is 1000 x 400 design pixels with the marker at its centre, on the clue: 1.6 x 0.64 m in the room.
	SetUpPanel(TagWidget, DetroitUI::ClueTagClass, FVector2D(1000.0, 400.0), 0.16f, EDreamWorldWidgetBackend::DreamUIRenderer, 0.8f);
	if (UDetroitClueTag* Tag = GetTag())
	{
		Tag->SetTitle(Spec.TitleA, Spec.TitleB, Spec.Suffix);
	}
	if (Spec.Kind == EDetroitClueKind::Panel)
	{
		PanelWidget = NewObject<UDreamWorldWidgetComponent>(this, TEXT("Panel"));
		PanelWidget->SetupAttachment(Root);
		PanelWidget->SetRelativeLocation(FVector(0.0, 0.0, 105.0));
		PanelWidget->RegisterComponent();
		SetUpPanel(PanelWidget, DetroitUI::AnalysisPanelClass, FVector2D(1100.0, 560.0), 0.14f, EDreamWorldWidgetBackend::DreamUIRenderer, 0.8f);
		BuildHologram();
	}
}

void ADetroitClue::BuildHologram()
{
	using namespace DetroitGameLocal;
	UMaterialInterface* Holo = LoadObject<UMaterialInterface>(nullptr, DetroitUI::HoloMeshMaterial);
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, DetroitUI::CylinderMesh);
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, DetroitUI::SphereMesh);
	if (Holo == nullptr || Cylinder == nullptr || Sphere == nullptr)
	{
		return;
	}
	// The reconstruction: the bat swung down onto the arm by someone standing over it, and where it struck.
	HoloPivot = NewObject<USceneComponent>(this, TEXT("HoloPivot"));
	HoloPivot->SetupAttachment(Root);
	HoloPivot->SetRelativeLocation(FVector(-60.0, 10.0, 70.0));
	HoloPivot->RegisterComponent();
	auto AddHolo = [this, Holo](UStaticMesh* Mesh, USceneComponent* Parent, const FVector& Location, const FVector& Scale,
		const FRotator& Rotation, const FLinearColor& Color)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
		Part->SetStaticMesh(Mesh);
		Part->SetupAttachment(Parent);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeRotation(Rotation);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->RegisterComponent();
		UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(Holo, this);
		Instance->SetVectorParameterValue(TEXT("HoloColor"), Color);
		Instance->SetScalarParameterValue(TEXT("Fade"), 0.0f);
		Part->SetMaterial(0, Instance);
		HoloMaterials.Add(Instance);
	};
	const FLinearColor Blue(0.08f, 0.5f, 1.0f);
	const FLinearColor Orange(1.0f, 0.25f, 0.04f);
	// The bat lies along the pivot's +X: the handle at the hands, the barrel out where it strikes.
	AddHolo(Cylinder, HoloPivot, FVector(30.0, 0.0, 0.0), FVector(0.045, 0.045, 0.60), FRotator(-90.0, 0.0, 0.0), Blue);
	AddHolo(Cylinder, HoloPivot, FVector(70.0, 0.0, 0.0), FVector(0.07, 0.07, 0.30), FRotator(-90.0, 0.0, 0.0), Blue);
	AddHolo(Sphere, HoloPivot, FVector(85.0, 0.0, 0.0), FVector(0.07, 0.07, 0.07), FRotator::ZeroRotator, Blue);
	for (const FVector& Hit : { FVector(-8.0, 6.0, -26.0), FVector(6.0, -2.0, -27.0), FVector(18.0, -10.0, -26.0) })
	{
		AddHolo(Sphere, Root, Hit + FVector(0.0, 0.0, 30.0), FVector(0.05, 0.05, 0.03), FRotator::ZeroRotator, Orange);
	}
}

void ADetroitClue::Analyze()
{
	if (bAnalyzed || AnalyzeTime >= 0.0f)
	{
		return;
	}
	AnalyzeTime = 0.0f;
	if (UDetroitClueTag* Tag = GetTag())
	{
		Tag->BeginAnalysis();
	}
}

void ADetroitClue::FaceCamera(USceneComponent* Component, float ExtraYaw, float ExtraPitch) const
{
	if (Component == nullptr)
	{
		return;
	}
	// A DreamGUI widget faces its component's -X: the viewer looks along +X. So the component points away from the camera.
	FRotator Rotation = (Component->GetComponentLocation() - DetroitGameLocal::GetCameraLocation(this)).Rotation();
	Rotation.Yaw += ExtraYaw;
	Rotation.Pitch += ExtraPitch;
	Rotation.Roll = 0.0;
	Component->SetWorldRotation(Rotation);
}

void ADetroitClue::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UDetroitClueTag* Tag = GetTag();
	UDetroitAnalysisPanel* Panel = GetPanel();
	if (AnalyzeTime >= 0.0f)
	{
		AnalyzeTime += DeltaTime;
		FaceCamera(TagWidget, 0.0f, 0.0f);
		if (Tag != nullptr)
		{
			Tag->SetAnalysisProgress(AnalyzeTime / AnalyzeDuration);
		}
		if (AnalyzeTime >= AnalyzeDuration)
		{
			AnalyzeTime = -1.0f;
			bAnalyzed = true;
			SinceAnalyzed = 0.0f;
			// From here on the tag stays where it is in the room, turned a little from the camera: it is a thing in 3D.
			FaceCamera(TagWidget, Spec.TagYaw, -6.0f);
			if (Tag != nullptr)
			{
				Tag->PlayUnlock();
			}
			if (PanelWidget != nullptr)
			{
				// Up and to the side of the clue as the camera sees it: the one analyzing it does not stand in front of it.
				const FRotationMatrix Flat(FRotator(0.0, DetroitGameLocal::GetCameraRotation(this).Yaw, 0.0));
				PanelWidget->SetWorldLocation(GetActorLocation() + Flat.GetUnitAxis(EAxis::Y) * 115.0 + FVector(0.0, 0.0, 80.0));
				FaceCamera(PanelWidget, -Spec.TagYaw, -4.0f);
				if (Panel != nullptr)
				{
					Panel->PlayShow();
				}
			}
			OnAnalyzed.Broadcast(this);
		}
	}
	else if (!bAnalyzed)
	{
		FaceCamera(TagWidget, 0.0f, 0.0f);
	}
	if (Tag != nullptr)
	{
		Tag->ShowMarker(bAnalysisMode && !bAnalyzed && AnalyzeTime < 0.0f, bInReach);
		if (bAnalyzed)
		{
			Tag->SetTagShown(bAnalysisMode || SinceAnalyzed < 7.0f);
		}
	}
	if (bAnalyzed)
	{
		SinceAnalyzed += DeltaTime;
		const bool bShowPanel = bAnalysisMode || SinceAnalyzed < 9.0f;
		if (Panel != nullptr)
		{
			Panel->SetPanelShown(bShowPanel);
		}
		if (HoloPivot != nullptr)
		{
			HoloFade = FMath::FInterpConstantTo(HoloFade, bShowPanel ? 1.0f : 0.0f, DeltaTime, 2.0f);
			// A swing every 1.8 s: raised, struck down, held, raised again.
			const float Cycle = FMath::Fmod(SinceAnalyzed, 1.8f);
			float Angle = 60.0f;
			if (Cycle < 0.7f)
			{
				Angle = FMath::InterpEaseInOut(-35.0f, 60.0f, Cycle / 0.7f, 2.0f);
			}
			else if (Cycle < 0.92f)
			{
				Angle = FMath::InterpEaseIn(60.0f, -35.0f, (Cycle - 0.7f) / 0.22f, 2.0f);
			}
			else
			{
				Angle = -35.0f;
			}
			HoloPivot->SetRelativeRotation(FRotator(Angle, 15.0, 0.0));
			const float Strike = Cycle >= 0.92f ? FMath::Clamp(1.0f - (Cycle - 0.92f) / 0.5f, 0.0f, 1.0f) : 0.0f;
			for (int32 Index = 0; Index < HoloMaterials.Num(); ++Index)
			{
				// The bat fades in with the panel; the marks it left flare as it lands.
				const float Fade = Index < 3 ? HoloFade : HoloFade * (0.45f + 0.55f * Strike);
				HoloMaterials[Index]->SetScalarParameterValue(TEXT("Fade"), Fade);
			}
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

ADetroitObjectivePanel::ADetroitObjectivePanel()
{
	Widget = CreateDefaultSubobject<UDreamWorldWidgetComponent>(TEXT("Objectives"));
	SetRootComponent(Widget);
}

void ADetroitObjectivePanel::BeginPlay()
{
	Super::BeginPlay();
	DetroitGameLocal::SetUpPanel(Widget, DetroitUI::ObjectivesClass, FVector2D(900.0, 620.0), 0.18f, EDreamWorldWidgetBackend::DreamUIRenderer, 0.85f);
}

UDetroitObjectives* ADetroitObjectivePanel::GetObjectives() const
{
	return Widget != nullptr ? Cast<UDetroitObjectives>(Widget->GetLoadedWidget()) : nullptr;
}

void ADetroitObjectivePanel::ShowFor(float Hold)
{
	using namespace DetroitGameLocal;
	// Set down in the room ahead of the player and a little to the right, turned from the camera: it stays there.
	const FVector Camera = GetCameraLocation(this);
	FRotator Aim = GetCameraRotation(this);
	// Where the player looks, but never far above or below the eye: a look at the floor does not lay it on the floor.
	Aim.Pitch = FMath::Clamp(FRotator::NormalizeAxis(Aim.Pitch), -8.0, 8.0);
	Aim.Roll = 0.0;
	const FRotationMatrix View(Aim);
	const FVector Location = Camera + View.GetUnitAxis(EAxis::X) * 360.0 + View.GetUnitAxis(EAxis::Y) * 85.0 + View.GetUnitAxis(EAxis::Z) * 12.0;
	FRotator Facing = (Location - Camera).Rotation();
	Facing.Yaw -= 14.0;
	Facing.Pitch -= 3.0;
	Facing.Roll = 0.0;
	SetActorLocationAndRotation(Location, Facing);
	if (UDetroitObjectives* Objectives = GetObjectives())
	{
		Objectives->Show(Hold);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

ADetroitCrimeScene::ADetroitCrimeScene()
{
	PrimaryActorTick.bCanEverTick = true;
	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);
}

UStaticMeshComponent* ADetroitCrimeScene::AddMesh(UStaticMesh* Mesh, const FVector& Location, const FVector& Scale, const FRotator& Rotation,
	UMaterialInterface* Material, bool bCollide)
{
	UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
	Part->SetMobility(EComponentMobility::Movable);
	Part->SetStaticMesh(Mesh);
	Part->SetupAttachment(Root);
	Part->SetRelativeLocation(Location);
	Part->SetRelativeRotation(Rotation);
	Part->SetRelativeScale3D(Scale);
	if (bCollide)
	{
		Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	}
	else
	{
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (Material != nullptr)
	{
		Part->SetMaterial(0, Material);
	}
	Part->RegisterComponent();
	return Part;
}

UStaticMeshComponent* ADetroitCrimeScene::AddBox(const FVector& Center, const FVector& Size, UMaterialInterface* Material, bool bCollide,
	const FRotator& Rotation)
{
	return AddMesh(LoadObject<UStaticMesh>(nullptr, DetroitUI::CubeMesh), Center, Size / 100.0, Rotation, Material, bCollide);
}

UMaterialInstanceDynamic* ADetroitCrimeScene::MakeLit(const FColor& Color, float Roughness, float Metallic, const FLinearColor& Emissive)
{
	return DetroitGameLocal::MakeLitInstance(LoadObject<UMaterialInterface>(nullptr, DetroitUI::LitMaterial), this, Color, Roughness, Metallic, Emissive);
}

UPointLightComponent* ADetroitCrimeScene::AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity, float Radius, bool bShadows)
{
	UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetupAttachment(Root);
	Light->SetRelativeLocation(Location);
	Light->SetIntensityUnits(ELightUnits::Candelas);
	Light->SetIntensity(Intensity);
	Light->SetLightColor(Color);
	Light->SetAttenuationRadius(Radius);
	Light->SetCastShadows(bShadows);
	Light->SetSourceRadius(2.0f);
	Light->RegisterComponent();
	return Light;
}

UDreamWorldWidgetComponent* ADetroitCrimeScene::AddWallWriting(const FVector& Location, const FRotator& Rotation, float Scale)
{
	UDreamWorldWidgetComponent* Writing = NewObject<UDreamWorldWidgetComponent>(this);
	Writing->SetupAttachment(Root);
	Writing->SetRelativeLocationAndRotation(Location, Rotation);
	Writing->RegisterComponent();
	DetroitGameLocal::SetUpPanel(Writing, DetroitUI::WallWritingClass, FVector2D(1600.0, 800.0), Scale, EDreamWorldWidgetBackend::UERenderer, 0.0f);
	return Writing;
}

ADetroitClue* ADetroitCrimeScene::AddClue(const FVector& Location, const FDetroitClueSpec& InSpec)
{
	const FTransform Transform(FRotator::ZeroRotator, GetActorTransform().TransformPosition(Location));
	ADetroitClue* Clue = GetWorld()->SpawnActorDeferred<ADetroitClue>(ADetroitClue::StaticClass(), Transform, this);
	if (Clue == nullptr)
	{
		return nullptr;
	}
	Clue->Setup(InSpec);
	UGameplayStatics::FinishSpawningActor(Clue, Transform);
	Clues.Add(Clue);
	return Clue;
}

void ADetroitCrimeScene::BeginPlay()
{
	Super::BeginPlay();
	BuildRoom();
	BuildProps();
}

void ADetroitCrimeScene::BuildRoom()
{
	// Inside: X -450..450, Y -350..350, Z 0..300.
	UMaterialInterface* Tiles = LoadObject<UMaterialInterface>(nullptr, DetroitUI::TilesMaterial);
	UMaterialInstanceDynamic* Wall = Tiles != nullptr ? UMaterialInstanceDynamic::Create(Tiles, this) : nullptr;
	UMaterialInstanceDynamic* Floor = Tiles != nullptr ? UMaterialInstanceDynamic::Create(Tiles, this) : nullptr;
	if (Wall != nullptr)
	{
		Wall->SetVectorParameterValue(TEXT("TileColor"), FLinearColor(0.13f, 0.16f, 0.165f));
		Wall->SetScalarParameterValue(TEXT("TileSize"), 15.0f);
		Wall->SetScalarParameterValue(TEXT("GroutWidth"), 0.06f);
		Wall->SetScalarParameterValue(TEXT("Grime"), 0.65f);
	}
	if (Floor != nullptr)
	{
		Floor->SetVectorParameterValue(TEXT("TileColor"), FLinearColor(0.075f, 0.075f, 0.07f));
		Floor->SetScalarParameterValue(TEXT("TileSize"), 45.0f);
		Floor->SetScalarParameterValue(TEXT("GroutWidth"), 0.02f);
		Floor->SetScalarParameterValue(TEXT("Grime"), 0.9f);
		Floor->SetScalarParameterValue(TEXT("TileRoughness"), 0.45f);
	}
	UMaterialInstanceDynamic* Ceiling = MakeLit(FColor(58, 61, 64), 0.9f);
	AddBox(FVector(0.0, 0.0, -10.0), FVector(940.0, 740.0, 20.0), Floor);
	AddBox(FVector(0.0, 0.0, 310.0), FVector(940.0, 740.0, 20.0), Ceiling);
	AddBox(FVector(460.0, 0.0, 150.0), FVector(20.0, 740.0, 320.0), Wall);
	AddBox(FVector(-460.0, 0.0, 150.0), FVector(20.0, 740.0, 320.0), Wall);
	AddBox(FVector(0.0, 360.0, 150.0), FVector(940.0, 20.0, 320.0), Wall);
	AddBox(FVector(0.0, -360.0, 150.0), FVector(940.0, 20.0, 320.0), Wall);

	// A door in the -Y wall, and pipes along the ceiling.
	UMaterialInstanceDynamic* DoorPaint = MakeLit(FColor(59, 50, 39), 0.7f);
	UMaterialInstanceDynamic* Metal = MakeLit(FColor(70, 72, 74), 0.45f, 0.8f);
	AddBox(FVector(150.0, -348.0, 105.0), FVector(96.0, 4.0, 210.0), DoorPaint, false);
	AddBox(FVector(150.0, -347.0, 212.0), FVector(110.0, 6.0, 8.0), Metal, false);
	AddBox(FVector(96.0, -347.0, 105.0), FVector(8.0, 6.0, 214.0), Metal, false);
	AddBox(FVector(204.0, -347.0, 105.0), FVector(8.0, 6.0, 214.0), Metal, false);
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, DetroitUI::CylinderMesh);
	AddMesh(Cylinder, FVector(0.0, 330.0, 284.0), FVector(0.08, 0.08, 9.0), FRotator(90.0, 0.0, 0.0), Metal, false);
	AddMesh(Cylinder, FVector(-420.0, 0.0, 290.0), FVector(0.05, 0.05, 7.0), FRotator(0.0, 0.0, 90.0), Metal, false);

	// The bare bulb on its cord, and a cold fill from the corner the door is in.
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, DetroitUI::SphereMesh);
	AddMesh(Cylinder, FVector(0.0, 0.0, 285.0), FVector(0.01, 0.01, 0.3), FRotator::ZeroRotator, Metal, false);
	UStaticMeshComponent* Glass = AddMesh(Sphere, FVector(0.0, 0.0, 266.0), FVector(0.07), FRotator::ZeroRotator,
		MakeLit(FColor(255, 240, 220), 0.2f, 0.0f, FLinearColor(1.0f, 0.9f, 0.75f, 30.0f)), false);
	Glass->SetCastShadow(false);
	BulbIntensity = 60.0f;
	Bulb = AddPointLight(FVector(0.0, 0.0, 255.0), FLinearColor(1.0f, 0.92f, 0.82f), BulbIntensity, 1500.0f, true);
	AddPointLight(FVector(-320.0, -260.0, 220.0), FLinearColor(0.55f, 0.7f, 1.0f), 6.0f, 900.0f, false);

	// rA9, over and over: hierarchies laid on the walls, drawn by UE's renderer so the room's light and fog are on them.
	AddWallWriting(FVector(448.5, 0.0, 165.0), FRotator::ZeroRotator, 0.25f);
	AddWallWriting(FVector(170.0, 348.5, 175.0), FRotator(0.0, 90.0, 0.0), 0.16f);
	AddWallWriting(FVector(-448.5, 160.0, 160.0), FRotator(0.0, 180.0, 0.0), 0.13f);
}

void ADetroitCrimeScene::BuildProps()
{
	UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, DetroitUI::CylinderMesh);
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, DetroitUI::SphereMesh);
	UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, DetroitUI::PlaneMesh);

	// The shrine in the far corner: a crate, a carved figure on it, candles round it.
	UMaterialInstanceDynamic* Crate = MakeLit(FColor(74, 54, 38), 0.8f);
	UMaterialInstanceDynamic* Carving = MakeLit(FColor(154, 84, 40), 0.6f);
	UMaterialInstanceDynamic* Wax = MakeLit(FColor(232, 220, 192), 0.5f);
	UMaterialInstanceDynamic* Flame = MakeLit(FColor(255, 160, 60), 0.5f, 0.0f, FLinearColor(1.0f, 0.5f, 0.12f, 25.0f));
	AddBox(FVector(370.0, 280.0, 22.5), FVector(90.0, 70.0, 45.0), Crate);
	AddMesh(Cylinder, FVector(370.0, 280.0, 47.0), FVector(0.10, 0.10, 0.04), FRotator::ZeroRotator, Carving, false);
	AddMesh(Cylinder, FVector(370.0, 280.0, 62.0), FVector(0.085, 0.085, 0.26), FRotator::ZeroRotator, Carving, false);
	AddMesh(Sphere, FVector(370.0, 280.0, 80.5), FVector(0.09), FRotator::ZeroRotator, Carving, false);
	AddMesh(Cylinder, FVector(370.0, 273.0, 64.0), FVector(0.03, 0.03, 0.18), FRotator(0.0, 0.0, 12.0), Carving, false);
	AddMesh(Cylinder, FVector(370.0, 287.0, 64.0), FVector(0.03, 0.03, 0.18), FRotator(0.0, 0.0, -12.0), Carving, false);
	const FVector4 CandleSpots[] = { FVector4(344.0, 262.0, 0.10, 0.0), FVector4(393.0, 260.0, 0.07, 0.0), FVector4(349.0, 302.0, 0.12, 0.0),
		FVector4(396.0, 303.0, 0.06, 0.0), FVector4(372.0, 254.0, 0.09, 0.0) };
	for (const FVector4& Spot : CandleSpots)
	{
		const double Height = Spot.Z * 100.0;
		AddMesh(Cylinder, FVector(Spot.X, Spot.Y, 45.0 + Height * 0.5), FVector(0.035, 0.035, Spot.Z), FRotator::ZeroRotator, Wax, false);
		UStaticMeshComponent* Tip = AddMesh(Sphere, FVector(Spot.X, Spot.Y, 45.0 + Height + 1.6), FVector(0.016, 0.016, 0.03), FRotator::ZeroRotator, Flame, false);
		Tip->SetCastShadow(false);
		Flames.Add(Tip);
	}
	CandleIntensity = 5.0f;
	Candles = AddPointLight(FVector(370.0, 280.0, 72.0), FLinearColor(1.0f, 0.55f, 0.25f), CandleIntensity, 320.0f, false);

	// A mattress on the floor, a table and a chair.
	UMaterialInstanceDynamic* Fabric = MakeLit(FColor(91, 86, 76), 0.95f);
	UMaterialInstanceDynamic* Wood = MakeLit(FColor(62, 50, 41), 0.75f);
	AddBox(FVector(-280.0, 250.0, 9.0), FVector(210.0, 100.0, 18.0), Fabric);
	AddBox(FVector(-362.0, 250.0, 22.0), FVector(40.0, 62.0, 10.0), MakeLit(FColor(150, 145, 132), 0.95f), false);
	AddBox(FVector(-330.0, -230.0, 74.0), FVector(120.0, 70.0, 4.0), Wood);
	for (const FVector2D& Leg : { FVector2D(-385.0, -260.0), FVector2D(-275.0, -260.0), FVector2D(-385.0, -200.0), FVector2D(-275.0, -200.0) })
	{
		AddBox(FVector(Leg.X, Leg.Y, 36.0), FVector(4.0, 4.0, 72.0), Wood, false);
	}
	AddBox(FVector(-250.0, -190.0, 45.0), FVector(42.0, 42.0, 4.0), Wood);
	AddBox(FVector(-229.0, -190.0, 72.0), FVector(4.0, 42.0, 50.0), Wood, false);

	// The bat against the +Y wall.
	UMaterialInstanceDynamic* BatMetal = MakeLit(FColor(154, 160, 166), 0.35f, 0.85f);
	AddMesh(Cylinder, FVector(60.0, 334.0, 30.0), FVector(0.035, 0.035, 0.60), FRotator(0.0, 0.0, -12.0), BatMetal, false);
	AddMesh(Cylinder, FVector(60.0, 340.0, 70.0), FVector(0.055, 0.055, 0.32), FRotator(0.0, 0.0, -12.0), BatMetal, false);

	// An android's arm on the floor, its casing cracked at the elbow, and the blue blood around it and toward the door.
	UMaterialInstanceDynamic* Casing = MakeLit(FColor(218, 223, 227), 0.25f);
	UMaterialInstanceDynamic* Core = MakeLit(FColor(20, 60, 120), 0.3f, 0.0f, FLinearColor(0.08f, 0.45f, 1.0f, 6.0f));
	AddMesh(Cylinder, FVector(116.0, -150.0, 5.0), FVector(0.10, 0.10, 0.34), FRotator(90.0, 25.0, 0.0), Casing, false);
	AddMesh(Cylinder, FVector(151.0, -177.0, 4.5), FVector(0.09, 0.09, 0.30), FRotator(90.0, -20.0, 0.0), Casing, false);
	AddBox(FVector(180.0, -194.0, 4.0), FVector(9.0, 6.0, 3.5), Casing, false, FRotator(0.0, -20.0, 0.0));
	AddBox(FVector(134.0, -164.0, 6.0), FVector(5.0, 4.0, 4.0), Core, false, FRotator(0.0, 30.0, 0.0));
	AddBox(FVector(138.0, -160.0, 8.0), FVector(3.0, 3.0, 3.0), Core, false, FRotator(20.0, 10.0, 0.0));
	UMaterialInterface* Thirium = LoadObject<UMaterialInterface>(nullptr, DetroitUI::ThiriumMaterial);
	const FVector4 StainSpots[] = { FVector4(132.0, -165.0, 0.9, 1.0), FVector4(96.0, -120.0, 0.45, 2.0), FVector4(62.0, -84.0, 0.32, 3.0),
		FVector4(170.0, -214.0, 0.42, 4.0), FVector4(138.0, -256.0, 0.3, 5.0), FVector4(152.0, -300.0, 0.36, 6.0), FVector4(40.0, -36.0, 0.24, 7.0) };
	for (const FVector4& Spot : StainSpots)
	{
		if (Thirium == nullptr)
		{
			break;
		}
		UMaterialInstanceDynamic* Stain = UMaterialInstanceDynamic::Create(Thirium, this);
		Stain->SetScalarParameterValue(TEXT("Seed"), Spot.W);
		Stain->SetScalarParameterValue(TEXT("Reveal"), 0.0f);
		UStaticMeshComponent* Splat = AddMesh(Plane, FVector(Spot.X, Spot.Y, 0.4), FVector(Spot.Z, Spot.Z, 1.0), FRotator(0.0, Spot.W * 47.0, 0.0), Stain, false);
		Splat->SetCastShadow(false);
		Stains.Add(Stain);
	}

	auto Spec = [](FName Id, const TCHAR* A, const TCHAR* B, const TCHAR* Suffix, EDetroitClueKind Kind, float TagYaw)
	{
		FDetroitClueSpec Result;
		Result.Id = Id;
		Result.TitleA = FText::AsCultureInvariant(A);
		Result.TitleB = FText::AsCultureInvariant(B);
		Result.Suffix = FText::AsCultureInvariant(Suffix);
		Result.Kind = Kind;
		Result.TagYaw = TagYaw;
		return Result;
	};
	AddClue(FVector(370.0, 280.0, 108.0), Spec(TEXT("Statue"), TEXT("RELIGIOUS"), TEXT("OFFERING"), TEXT("?"), EDetroitClueKind::Tag, 22.0f));
	AddClue(FVector(150.0, -292.0, 22.0), Spec(TEXT("Thirium"), TEXT("THIRIUM"), TEXT("BLUE BLOOD"), TEXT(""), EDetroitClueKind::Tag, -18.0f));
	AddClue(FVector(60.0, 322.0, 96.0), Spec(TEXT("Bat"), TEXT("BASEBALL"), TEXT("BAT"), TEXT(""), EDetroitClueKind::Tag, 20.0f));
	AddClue(FVector(134.0, -164.0, 28.0), Spec(TEXT("Arm"), TEXT("ANDROID"), TEXT("ARM"), TEXT(""), EDetroitClueKind::Panel, -16.0f));
	AddClue(FVector(436.0, -80.0, 172.0), Spec(TEXT("Writings"), TEXT("WALL"), TEXT("WRITINGS"), TEXT("?"), EDetroitClueKind::Tag, 16.0f));
}

void ADetroitCrimeScene::SetAnalysisBlend(float Blend)
{
	for (UMaterialInstanceDynamic* Stain : Stains)
	{
		Stain->SetScalarParameterValue(TEXT("Reveal"), Blend);
	}
}

void ADetroitCrimeScene::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Clock += DeltaTime;
	// The bulb dips now and then; the candles never hold still.
	if (Bulb != nullptr)
	{
		const float Slot = FMath::FloorToFloat(Clock * 14.0f);
		const float Noise = FMath::Frac(FMath::Sin(Slot * 12.9898f) * 43758.5453f);
		Bulb->SetIntensity(BulbIntensity * (Noise > 0.94f ? 0.55f : 1.0f));
	}
	if (Candles != nullptr)
	{
		const float Flicker = 0.82f + 0.12f * FMath::Sin(Clock * 13.0f) * FMath::Sin(Clock * 7.3f) + 0.06f * FMath::Sin(Clock * 23.0f);
		Candles->SetIntensity(CandleIntensity * Flicker);
		for (int32 Index = 0; Index < Flames.Num(); ++Index)
		{
			const float Wobble = 1.0f + 0.15f * FMath::Sin(Clock * (9.0f + Index * 1.7f));
			Flames[Index]->SetRelativeScale3D(FVector(0.016, 0.016, 0.03 * Wobble));
		}
	}
}

// ---------------------------------------------------------------------------------------------------------------------

ADetroitGameMode::ADetroitGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	DefaultPawnClass = ADetroitCharacter::StaticClass();
	PlayerControllerClass = ADetroitPlayerController::StaticClass();
}

void ADetroitGameMode::BeginPlay()
{
	Super::BeginPlay();
	// See the NieR test's game mode: a hitch frame does not make an entrance jump to its end.
	if (IConsoleVariable* MaxStep = IConsoleManager::Get().FindConsoleVariable(TEXT("DreamTween.MaxStepSeconds")))
	{
		MaxStep->Set(1.0f / 30.0f, ECVF_SetByGameSetting);
	}
	// Now, not later: the floor has to be there before Connor's first step.
	Scene = GetWorld()->SpawnActor<ADetroitCrimeScene>(FVector::ZeroVector, FRotator::ZeroRotator);
	ObjectivePanel = GetWorld()->SpawnActor<ADetroitObjectivePanel>(FVector(0.0, 0.0, -2000.0), FRotator::ZeroRotator);
	// The loading screen on the first frame drawn, when the player is already there (a map opened from the command line);
	// a frame later otherwise.
	if (GetWorld()->GetFirstPlayerController() != nullptr)
	{
		Boot();
	}
	else
	{
		GetWorldTimerManager().SetTimerForNextTick(this, &ADetroitGameMode::Boot);
	}
}

void ADetroitGameMode::Boot()
{
	UWorld* World = GetWorld();
	APlayerController* Player = World != nullptr ? World->GetFirstPlayerController() : nullptr;
	UDreamScreenUISubsystem* ScreenUI = World != nullptr ? UDreamScreenUISubsystem::Get(World) : nullptr;
	if (Player == nullptr || ScreenUI == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Detroit UI test: no player or no DreamGUI screen subsystem"));
		return;
	}
	ScreenUI->GetOrCreateScreenRoot(Player);
	if (UDreamCanvas* Canvas = ScreenUI->GetScreenCanvas(Player))
	{
		Canvas->SetScaleMode(EDreamCanvasScaleMode::ScaleWithScreenSize);
		Canvas->SetReferenceResolution(FVector2D(1920.0, 1080.0));
		Canvas->SetScreenMatchMode(EDreamCanvasScreenMatchMode::MatchWidthOrHeight);
		Canvas->SetMatchFromWidthToHeight(0.5f);
#if PLATFORM_ANDROID
		// A phone is wider than 16:9: matching halfway cropped the top and bottom of the design.
		Canvas->SetScreenMatchMode(EDreamCanvasScreenMatchMode::Shrink);
#endif
	}
	for (const TCHAR* Path : { DetroitUI::LoadingClass, DetroitUI::HUDClass, DetroitUI::ClueTagClass, DetroitUI::AnalysisPanelClass,
		DetroitUI::ObjectivesClass, DetroitUI::JacketClass, DetroitUI::WallWritingClass })
	{
		UClass* WidgetClass = LoadClass<UDreamUserWidget>(nullptr, Path);
		if (WidgetClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("Detroit UI test: %s is missing; run DUI/UITests/Detroit/_Tools/make_detroit_assets.py"), Path);
			return;
		}
		WidgetClasses.Add(WidgetClass);
	}
	UDreamUIInputModeLibrary::SetInputModeGameOnly(this, 0);
	Player->SetInputMode(FInputModeGameOnly());
	Player->SetShowMouseCursor(false);

	HUD = Cast<UDetroitHUD>(ScreenUI->ShowWidgetOfClass(TEXT("Detroit_HUD"), WidgetClasses[1].Get(), 0, Player));
	if (HUD != nullptr)
	{
		HUD->SetRenderOpacity(0.0f);
	}
	Loading = Cast<UDetroitLoadingScreen>(ScreenUI->ShowWidgetOfClass(TEXT("Detroit_Loading"), WidgetClasses[0].Get(), 100, Player));
	if (Loading != nullptr)
	{
		Loading->OnFinished.AddUObject(this, &ADetroitGameMode::OnLoadingFinished);
		Loading->Play(8.5f);
	}
	else
	{
		OnLoadingFinished();
	}
	if (Scene != nullptr)
	{
		for (ADetroitClue* Clue : Scene->GetClues())
		{
			Clue->OnAnalyzed.AddUObject(this, &ADetroitGameMode::OnClueAnalyzed);
		}
	}
}

void ADetroitGameMode::After(float Seconds, TFunction<void()> Then)
{
	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, FTimerDelegate::CreateWeakLambda(this, [Then = MoveTemp(Then)]() { Then(); }),
		FMath::Max(Seconds, 0.001f), false);
}

void ADetroitGameMode::OnLoadingFinished()
{
	// Not inside the broadcast: the loading screen is ticking when it says it is done.
	After(0.0f, [this]()
	{
		// -DetroitLoadingLoop: the loading screen again and again, for looking at it (the first run after a material
		// changed spends its first second compiling shaders).
		if (Loading != nullptr && FParse::Param(FCommandLine::Get(), TEXT("DetroitLoadingLoop")))
		{
			Loading->Play(8.5f);
			return;
		}
		if (UDreamScreenUISubsystem* ScreenUI = UDreamScreenUISubsystem::Get(GetWorld()))
		{
			ScreenUI->RemoveUI(TEXT("Detroit_Loading"));
		}
		Loading = nullptr;
		bInvestigating = true;
		if (HUD != nullptr)
		{
			HUD->SetClues(Found, Scene != nullptr ? Scene->GetClues().Num() : 0);
			HUD->PlayEntrance();
		}
		After(1.0f, [this]() { ShowObjectives(); });
	});
}

ADetroitCharacter* ADetroitGameMode::GetConnor() const
{
	return Cast<ADetroitCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void ADetroitGameMode::ToggleAnalysis()
{
	if (!bInvestigating || Analyzing.IsValid())
	{
		return;
	}
	bAnalysis = !bAnalysis;
	if (bAnalysis)
	{
		WaveRadius = 0.0f;
		if (const ADetroitCharacter* Connor = GetConnor())
		{
			WaveCenter = Connor->GetActorLocation() - FVector(0.0, 0.0, 60.0);
		}
	}
	if (HUD != nullptr)
	{
		HUD->SetAnalysisMode(bAnalysis);
	}
	if (Scene != nullptr)
	{
		for (ADetroitClue* Clue : Scene->GetClues())
		{
			Clue->SetAnalysisMode(bAnalysis);
		}
	}
}

void ADetroitGameMode::TryAnalyze()
{
	ADetroitClue* Clue = InReach.Get();
	if (!bInvestigating || Analyzing.IsValid() || Clue == nullptr)
	{
		return;
	}
	if (!bAnalysis)
	{
		ToggleAnalysis();
	}
	Analyzing = Clue;
	Clue->Analyze();
	if (ADetroitCharacter* Connor = GetConnor())
	{
		Connor->SetAnalyzing(true);
	}
	if (HUD != nullptr)
	{
		HUD->SetCanAnalyze(false);
	}
}

void ADetroitGameMode::GoToClue(int32 Index)
{
	ADetroitCharacter* Connor = GetConnor();
	if (!bInvestigating || Connor == nullptr || Scene == nullptr || !Scene->GetClues().IsValidIndex(Index))
	{
		return;
	}
	// Set down 170 cm from the clue on the room's side of it, facing it.
	const FVector Clue = Scene->GetClues()[Index]->GetActorLocation();
	const FVector2D Away = FVector2D(-Clue.X, -Clue.Y).GetSafeNormal();
	const FVector Spot(Clue.X + Away.X * 170.0, Clue.Y + Away.Y * 170.0, Connor->GetActorLocation().Z);
	const FRotator Facing(0.0, (FVector(Clue.X, Clue.Y, 0.0) - FVector(Spot.X, Spot.Y, 0.0)).Rotation().Yaw, 0.0);
	Connor->TeleportTo(Spot, Facing);
	if (AController* Controller = Connor->GetController())
	{
		Controller->SetControlRotation(FRotator(-12.0, Facing.Yaw, 0.0));
	}
}

void ADetroitGameMode::ShowObjectives()
{
	if (bInvestigating && ObjectivePanel != nullptr)
	{
		ObjectivePanel->ShowFor(7.0f);
	}
}

void ADetroitGameMode::OnClueAnalyzed(ADetroitClue* Clue)
{
	Analyzing = nullptr;
	if (ADetroitCharacter* Connor = GetConnor())
	{
		Connor->SetAnalyzing(false);
	}
	++Found;
	const int32 Total = Scene != nullptr ? Scene->GetClues().Num() : 0;
	if (HUD != nullptr)
	{
		HUD->SetClues(Found, Total);
		HUD->HoldCounter(5.0f);
	}
	if (Clue != nullptr && Clue->GetSpec().Id == TEXT("Thirium"))
	{
		bThiriumFound = true;
	}
	// The objectives move on -- shown, then changed in front of the player: the blue blood found, then everything.
	const int32 Target = Found >= Total ? 2 : (bThiriumFound ? 1 : 0);
	if (Target > ObjectiveStage)
	{
		const int32 From = ObjectiveStage;
		ObjectiveStage = Target;
		After(1.8f, [this, From, Target]()
		{
			ShowObjectives();
			After(1.0f, [this, From, Target]()
			{
				UDetroitObjectives* Objectives = ObjectivePanel != nullptr ? ObjectivePanel->GetObjectives() : nullptr;
				for (int32 Stage = From + 1; Objectives != nullptr && Stage <= Target; ++Stage)
				{
					Objectives->SetObjective(Stage - 1, EDetroitObjectiveState::Completed, true);
					Objectives->SetObjective(Stage, EDetroitObjectiveState::Active, true);
				}
			});
		});
	}
}

void ADetroitGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ADetroitCharacter* Connor = GetConnor();
	if (!bInvestigating || Connor == nullptr || Scene == nullptr)
	{
		return;
	}
	// The clue in reach: near Connor and ahead of the camera, the most ahead of them.
	ADetroitClue* Best = nullptr;
	if (!Analyzing.IsValid())
	{
		const FVector Ahead = Connor->GetCamera()->GetForwardVector().GetSafeNormal2D();
		float BestScore = -1.0f;
		for (ADetroitClue* Clue : Scene->GetClues())
		{
			if (Clue->IsAnalyzed())
			{
				continue;
			}
			const FVector ToClue = Clue->GetActorLocation() - Connor->GetActorLocation();
			const float Distance = ToClue.Size2D();
			const float Facing = FVector::DotProduct(Ahead, ToClue.GetSafeNormal2D());
			if (Distance > 250.0f || Facing < 0.5f)
			{
				continue;
			}
			const float Score = Facing - Distance / 1000.0f;
			if (Score > BestScore)
			{
				BestScore = Score;
				Best = Clue;
			}
		}
	}
	InReach = Best;
	for (ADetroitClue* Clue : Scene->GetClues())
	{
		Clue->SetInReach(Clue == Best);
	}
	if (HUD != nullptr)
	{
		HUD->SetCanAnalyze(Best != nullptr);
	}

	// Analysis mode comes on behind a ring sweeping out from Connor, and goes off everywhere at once.
	AnalysisBlend = FMath::FInterpConstantTo(AnalysisBlend, bAnalysis ? 1.0f : 0.0f, DeltaTime, 2.5f);
	if (bAnalysis)
	{
		WaveRadius += DeltaTime * 1700.0f;
	}
	if (UMaterialInstanceDynamic* Post = Connor->GetAnalysisMaterial())
	{
		Post->SetScalarParameterValue(TEXT("Blend"), AnalysisBlend);
		Post->SetScalarParameterValue(TEXT("WaveRadius"), WaveRadius);
		Post->SetScalarParameterValue(TEXT("Spread"), FMath::Clamp((WaveRadius - 1100.0f) / 400.0f, 0.0f, 1.0f));
		Post->SetVectorParameterValue(TEXT("WaveCenter"), FLinearColor(float(WaveCenter.X), float(WaveCenter.Y), float(WaveCenter.Z), 0.0f));
	}
	Scene->SetAnalysisBlend(AnalysisBlend);
}
