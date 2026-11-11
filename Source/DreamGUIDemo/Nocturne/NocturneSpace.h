// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NocturneSpace.generated.h"

class UStaticMeshComponent;
class UMaterialBillboardComponent;
class UMaterialInstanceDynamic;
class UDreamWorldWidgetComponent;
class UDreamWidget;
class UDreamUserWidget;

/**
 * NOCTURNE//OS's space: the sky sphere, the six stars of the constellation (NocturneStars.h) with their glows and lines,
 * a few far stars for depth, and the world-space test panel beside the Settings star. Everything is built in the
 * constructor, so a level needs nothing but this actor (the game mode spawns it).
 *
 * Units are centimetres. A world-space panel faces its component's -X, so a camera looking along +X sees its front.
 */
UCLASS()
class DREAMGUIDEMO_API ANocturneSpace : public AActor
{
	GENERATED_BODY()

public:
	ANocturneSpace();

	virtual void BeginPlay() override;

	FVector GetStarLocation(int32 InIndex) const;
	/** The Settings star: where the test shot flies to. */
	FVector GetFocusStarLocation() const;
	/** The point of the test panel the camera ends up looking at. */
	FVector GetPanelLocation() const;
	UDreamWorldWidgetComponent* GetPanel() const { return Panel; }

	/** A widget of the test panel's tree by its .dui id, or null until the tree is loaded. */
	UDreamWidget* FindPanelWidget(const FString& InName) const;

	/** How much a star is lit, 0 to 1: brighter, a larger glow, and its lines brighter with it. */
	void SetStarFocus(int32 InIndex, float InAmount);
	/** How much of a constellation line is drawn, 0 to 1, from its first star toward its second (all of it by default). */
	void SetLineReveal(int32 InLine, float InAmount);

private:
	void ApplyLines();

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UStaticMeshComponent> Sky;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UStaticMeshComponent>> Stars;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UMaterialBillboardComponent>> Glows;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UStaticMeshComponent>> FarStars;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TArray<TObjectPtr<UStaticMeshComponent>> Lines;

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UDreamWorldWidgetComponent> Panel;

	UPROPERTY(EditAnywhere, Category = "Nocturne")
	TSoftClassPtr<UDreamUserWidget> PanelClassToLoad;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> StarMaterials;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> LineMaterials;

	TArray<float> StarFocus;
	TArray<float> LineReveal;
};
