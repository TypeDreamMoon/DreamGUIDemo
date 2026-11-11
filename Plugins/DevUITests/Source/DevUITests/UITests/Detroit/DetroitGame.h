// The Detroit test's game: an android (Connor, built from basic shapes) walked in third person through a small crime
// scene, the clues in it, the objectives panel that floats in the room, and the game mode that loads it all behind the
// loading screen and runs the investigation.
//
// Keys: WASD move, Shift runs, the mouse (or the arrows) looks, Q (or the right mouse button, or RT) toggles analysis
// mode, E (or A) analyzes the clue in reach, Tab (or RB) shows the objectives, 1 to 5 go straight to a clue.

#pragma once

#include "CoreMinimal.h"
#include "DetroitWidgets.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "DetroitGame.generated.h"

class UCameraComponent;
class UDreamWorldWidgetComponent;
class UMaterialInstanceDynamic;
class UMaterialInterface;
class UPointLightComponent;
class USpringArmComponent;
class UStaticMesh;
class UStaticMeshComponent;

/** Connor: a capsule with a body of basic shapes, an over-the-shoulder camera, the LED on his temple. */
UCLASS()
class DEVUITESTS_API ADetroitCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ADetroitCharacter();

	UCameraComponent* GetCamera() const { return Camera; }
	/** The analysis mode's post-process instance on the camera. */
	UMaterialInstanceDynamic* GetAnalysisMaterial() const { return Analysis; }
	/** The LED: blue at rest, yellow while he analyzes. */
	void SetAnalyzing(bool bInAnalyzing);
	void SetRunning(bool bInRunning);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UStaticMeshComponent* AddPart(FName Name, UStaticMesh* PartMesh, USceneComponent* Parent, const FVector& Location, const FVector& Scale,
		const FRotator& Rotation, int32 Look);
	USceneComponent* AddJoint(FName Name, USceneComponent* Parent, const FVector& Location);

	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USpringArmComponent> Arm;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<UCameraComponent> Camera;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> Body;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> HipL;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> HipR;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> ShoulderL;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> ShoulderR;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<UStaticMeshComponent> Led;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<UDreamWorldWidgetComponent> Jacket;
	/** Each part and which of the looks (DetroitGameLocal::Looks) it wears; the instances are made at BeginPlay. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Parts;
	TArray<int32> PartLooks;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> LedMaterial;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> Analysis;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> LitMaterial;
	float WalkPhase = 0.0f;
	float Clock = 0.0f;
	float LedBlend = 0.0f;
	bool bAnalyzing = false;
};

/** Reads the keys, the mouse and a pad every frame and drives Connor and the investigation. */
UCLASS()
class DEVUITESTS_API ADetroitPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ADetroitPlayerController();

protected:
	virtual void PlayerTick(float DeltaTime) override;
};

/** What makes a clue: its name on the tag, whether it opens the HIT MARKS panel, where the tag floats. */
USTRUCT()
struct FDetroitClueSpec
{
	GENERATED_BODY()

	FName Id;
	FText TitleA;
	FText TitleB;
	FText Suffix;
	EDetroitClueKind Kind = EDetroitClueKind::Tag;
	/** How far the tag turns from facing the camera once the clue is analyzed, degrees: it stays put in the room. */
	float TagYaw = 20.0f;
};

/**
 * A clue: an anchor in the room with its tag (Detroit_ClueTag) and, for a Panel clue, the HIT MARKS panel and a hologram
 * of the attack. The game says when analysis mode is on and when the player is in reach; Analyze runs the readout.
 */
UCLASS()
class DEVUITESTS_API ADetroitClue : public AActor
{
	GENERATED_BODY()

public:
	ADetroitClue();

	void Setup(const FDetroitClueSpec& InSpec);
	const FDetroitClueSpec& GetSpec() const { return Spec; }
	bool IsAnalyzed() const { return bAnalyzed; }
	bool IsAnalyzing() const { return AnalyzeTime >= 0.0f; }
	void SetAnalysisMode(bool bOn) { bAnalysisMode = bOn; }
	void SetInReach(bool bIn) { bInReach = bIn; }
	void Analyze();

	TMulticastDelegate<void(ADetroitClue*)> OnAnalyzed;

	static constexpr float AnalyzeDuration = 2.6f;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void FaceCamera(USceneComponent* Component, float ExtraYaw, float ExtraPitch) const;
	void BuildHologram();
	UDetroitClueTag* GetTag() const;
	UDetroitAnalysisPanel* GetPanel() const;

	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<UDreamWorldWidgetComponent> TagWidget;
	UPROPERTY(Transient)
	TObjectPtr<UDreamWorldWidgetComponent> PanelWidget;
	UPROPERTY(Transient)
	TObjectPtr<USceneComponent> HoloPivot;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> HoloMaterials;

	FDetroitClueSpec Spec;
	float AnalyzeTime = -1.0f;
	float SinceAnalyzed = 0.0f;
	float HoloFade = 0.0f;
	bool bAnalyzed = false;
	bool bAnalysisMode = false;
	bool bInReach = false;
};

/** The objectives floating in the room (Detroit_Objectives), set down in front of the player each time they show. */
UCLASS()
class DEVUITESTS_API ADetroitObjectivePanel : public AActor
{
	GENERATED_BODY()

public:
	ADetroitObjectivePanel();

	void ShowFor(float Hold);
	UDetroitObjectives* GetObjectives() const;

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<UDreamWorldWidgetComponent> Widget;
};

/**
 * The crime scene, built when it begins play: a tiled room lit by a bare bulb, a shrine with candles, a mattress, a table,
 * a bat against the wall, an android's arm in blue blood, writing all over one wall -- and a clue at each of them.
 */
UCLASS()
class DEVUITESTS_API ADetroitCrimeScene : public AActor
{
	GENERATED_BODY()

public:
	ADetroitCrimeScene();

	const TArray<TObjectPtr<ADetroitClue>>& GetClues() const { return Clues; }
	/** Analysis mode, 0..1: the blue blood shows with it. */
	void SetAnalysisBlend(float Blend);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	UStaticMeshComponent* AddMesh(UStaticMesh* InMesh, const FVector& Location, const FVector& Scale, const FRotator& Rotation,
		UMaterialInterface* Material, bool bCollide);
	UStaticMeshComponent* AddBox(const FVector& Center, const FVector& Size, UMaterialInterface* Material, bool bCollide = true,
		const FRotator& Rotation = FRotator::ZeroRotator);
	UMaterialInstanceDynamic* MakeLit(const FColor& Color, float Roughness, float Metallic = 0.0f, const FLinearColor& Emissive = FLinearColor::Transparent);
	UPointLightComponent* AddPointLight(const FVector& Location, const FLinearColor& Color, float Intensity, float Radius, bool bShadows);
	UDreamWorldWidgetComponent* AddWallWriting(const FVector& Location, const FRotator& Rotation, float Scale);
	ADetroitClue* AddClue(const FVector& Location, const FDetroitClueSpec& InSpec);
	void BuildRoom();
	void BuildProps();

	UPROPERTY(VisibleAnywhere, Category = "Detroit")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(Transient)
	TArray<TObjectPtr<ADetroitClue>> Clues;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> Stains;
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Bulb;
	UPROPERTY(Transient)
	TObjectPtr<UPointLightComponent> Candles;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UStaticMeshComponent>> Flames;
	float Clock = 0.0f;
	float BulbIntensity = 0.0f;
	float CandleIntensity = 0.0f;
};

/**
 * The Detroit test map's game mode (/Game/UITests/Detroit/Maps/Detroit_TestMap): the crime scene, Connor, the loading
 * screen, then the investigation -- the HUD, analysis mode, the clues and the objectives.
 */
UCLASS()
class DEVUITESTS_API ADetroitGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADetroitGameMode();

	bool IsInvestigating() const { return bInvestigating; }
	void ToggleAnalysis();
	void TryAnalyze();
	void ShowObjectives();
	/** Puts Connor in front of the Index-th clue, facing it (the 1 to 5 keys). */
	void GoToClue(int32 Index);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	void Boot();
	void OnLoadingFinished();
	void OnClueAnalyzed(ADetroitClue* Clue);
	void After(float Seconds, TFunction<void()> Then);
	ADetroitCharacter* GetConnor() const;

	/** Every widget class, loaded with the map and held while it runs (see the NieR test's game mode). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UClass>> WidgetClasses;
	UPROPERTY(Transient)
	TObjectPtr<ADetroitCrimeScene> Scene;
	UPROPERTY(Transient)
	TObjectPtr<ADetroitObjectivePanel> ObjectivePanel;
	UPROPERTY(Transient)
	TObjectPtr<UDetroitHUD> HUD;
	UPROPERTY(Transient)
	TObjectPtr<UDetroitLoadingScreen> Loading;
	TWeakObjectPtr<ADetroitClue> InReach;
	TWeakObjectPtr<ADetroitClue> Analyzing;
	float AnalysisBlend = 0.0f;
	float WaveRadius = 0.0f;
	FVector WaveCenter = FVector::ZeroVector;
	int32 Found = 0;
	/** 0 at the start, 1 once the blue blood is found, 2 once every clue is: which objective is the active one. */
	int32 ObjectiveStage = 0;
	bool bThiriumFound = false;
	bool bAnalysis = false;
	bool bInvestigating = false;
};
