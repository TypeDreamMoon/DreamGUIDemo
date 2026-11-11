// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NocturneDeck.generated.h"

class UDreamCanvas;
class UDreamUserWidget;
class UDreamWidget;
class UDreamUIRenderTargetGeometrySource;
class UDreamWorldWidgetComponent;
class UStaticMeshComponent;
class UTextureRenderTarget2D;

/**
 * The world-space shot's little deck in the star field: three ways the same kind of DreamGUI tree lives in a scene.
 *
 *   a curved screen   the test panel drawn into a render target and laid on a 70-degree slice of a cylinder
 *                     (UDreamUIRenderTargetGeometrySource), clickable through UDreamUIRenderTargetInteraction
 *   a marker          a small world panel behind a wall, marking a crate the wall hides; its canvas half ignores the
 *                     scene's depth (BlendDepth), so it shows through the wall, fainter where the wall is
 *   an engine panel   the music box drawn by the engine's renderer (UERenderer): an ordinary primitive, which a pillar
 *                     cuts into by depth and post process reaches
 *
 * Seen from -X (a world panel faces -X). The director orbits its camera round GetFocus.
 */
UCLASS()
class DREAMGUIDEMO_API ANocturneDeck : public AActor
{
	GENERATED_BODY()

public:
	ANocturneDeck();

	/** Where the camera looks while it circles. */
	FVector GetFocus() const;

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void BuildCurvedScreen();

	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<USceneComponent> Root;
	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UStaticMeshComponent> Wall;
	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UStaticMeshComponent> Pillar;
	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UStaticMeshComponent> Pod;
	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UDreamWorldWidgetComponent> Marker;
	UPROPERTY(VisibleAnywhere, Category = "Nocturne")
	TObjectPtr<UDreamWorldWidgetComponent> EnginePanel;

	TSoftClassPtr<UDreamUserWidget> ScreenClass;
	TSoftClassPtr<UDreamUserWidget> MarkerClass;
	TSoftClassPtr<UDreamUserWidget> EnginePanelClass;

	UPROPERTY(Transient)
	TObjectPtr<UTextureRenderTarget2D> ScreenTarget;
	UPROPERTY(Transient)
	TObjectPtr<UDreamWidget> ScreenRoot;
	UPROPERTY(Transient)
	TObjectPtr<UDreamUserWidget> ScreenContent;
	UPROPERTY(Transient)
	TObjectPtr<AActor> ScreenActor;

	float Age = 0.0f;
	bool bMarkerOverDepth = false;
};
