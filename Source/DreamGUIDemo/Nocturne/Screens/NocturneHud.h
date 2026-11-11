// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NocturneStarScreen.h"
#include "NocturneHud.generated.h"

/**
 * NOCTURNE//OS in flight (DUI/Nocturne/Nocturne_Hud.dui): the heads-up display once a voyage sets off. The voyage
 * (ANocturneVoyage) feeds it every frame -- speed, heading, position, where the beacons are on the screen -- and tells it
 * when something is picked up or the hull is hit; the HUD animates the rest on its own clock.
 */
UCLASS()
class DREAMGUIDEMO_API UNocturneHud : public UNocturneStarScreen
{
	GENERATED_BODY()

public:
	static constexpr int32 MarkerCount = 4;
	static constexpr int32 ToastCount = 3;
	static constexpr int32 DamageCount = 3;

	/** The HUD boots: its parts draw in and the banner names the voyage. */
	void PlayBoot(int32 InSlot);
	/** Speed in m/s, heading in degrees, where the ship is, thrust 0 to 1. */
	void SetFlight(float InSpeed, float InHeading, const FVector& InPosition, float InThrust);
	void SetHull(float InFraction);
	void SetEnergy(float InFraction);
	/** A beacon's bracket: shown or not, where (design units from the screen's centre, +y up), its name and distance. */
	void SetMarker(int32 InIndex, bool bInShown, const FVector2D& InPosition, const FString& InName, float InKilometres);
	void SetObjectiveDistance(float InKilometres);
	/** Something picked up: NocturneItems' index and how many. */
	void ShowToast(int32 InItem, int32 InCount);
	/** The hull is hit for InDamage: the screen's edge flashes and the number floats off the reticle. */
	void Hit(int32 InDamage);
	/** The objective's first line is done. */
	void CompleteObjective();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeOnTick(float DeltaTime) override;
	virtual void PlayContentEntrance(float Delay) override;
	virtual float GetEntrancePulse() const override { return 1.0f; }

private:
	void PlaceCompass(float InHeading);
	void SetNodeText(const TCHAR* Name, const FString& Text) const;
	/** The objective's first line ticked off: run under the sweep. */
	void ApplyObjectiveDone();

	struct FToast
	{
		float Shown = -100.0f;
	};
	struct FDamage
	{
		float Shown = -100.0f;
		FVector2D From = FVector2D::ZeroVector;
	};

	FToast Toasts[ToastCount];
	FDamage Damages[DamageCount];
	int32 NextToast = 0;
	int32 NextDamage = 0;
	float FlashAt = -100.0f;
	/** When the sweep over the objective began; negative while none runs. */
	float SweepAt = -1.0f;
	bool bSweepSwapped = false;
	float BannerAt = -100.0f;
	float Hull = 1.0f;
	/** Whether each beacon's bracket has locked on yet: brackets come after the banner, with a snap. */
	bool MarkerLocked[MarkerCount] = { false, false, false, false };
	int32 ShownSpeed = -1;
	int32 ShownHeading = -1;
	FString ShownCoords;
};
