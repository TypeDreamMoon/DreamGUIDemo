// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NocturneVoyage.generated.h"

class UCameraComponent;
class UDreamUserWidget;
class UMaterialBillboardComponent;
class UStaticMeshComponent;
class UNocturneHud;

/**
 * NOCTURNE//OS in flight: what the Depart star launches into. Its own camera carries on from where the menu's dive
 * ended, slowing from the dive to a cruise and swaying a little; four beacons hang ahead of it in the world; the HUD
 * (UNocturneHud) gets the ship's speed, heading and position every frame, the beacons' places on the screen, and a few
 * things that happen on the way -- pickups, two hits, the objective done. Everything runs on the actor's clock.
 */
UCLASS()
class DREAMGUIDEMO_API ANocturneVoyage : public AActor
{
	GENERATED_BODY()

public:
	ANocturneVoyage();

	/** Where the voyage begins and which way it heads; the save slot it is (set before BeginPlay). */
	void Setup(const FVector& InStart, const FRotator& InHeading, int32 InSlot);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UCameraComponent* GetCamera() const { return Camera; }
	double GetCameraSpeed() const { return Speed; }
	UNocturneHud* GetHud() const { return Hud; }

private:
	void UpdateMarkers();

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UStaticMeshComponent>> Beacons;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UMaterialBillboardComponent>> BeaconGlows;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> HudClass;

	UPROPERTY(Transient)
	TObjectPtr<UNocturneHud> Hud;

	FVector Start = FVector::ZeroVector;
	FRotator Heading = FRotator::ZeroRotator;
	FVector Position = FVector::ZeroVector;
	int32 Slot = 0;
	float VoyageTime = 0.0f;
	double Speed = 0.0;
	int32 NextEvent = 0;
};
