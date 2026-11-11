// Copyright DreamMoon. All Rights Reserved.

// Scripted key presses for recording screens that have no director of their own (the NieR and Detroit style studies).

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "DemoKeyScript.generated.h"

/**
 * Plays a list of key presses into a game world, timed from its BeginPlay:
 *   -DemoKeys=1.0:Enter,2.4:Down,3.0:W/2.5
 * is Enter at 1.0 s, Down at 2.4 s, and W held from 3.0 s for 2.5 s. A key without a hold is let go on the next frame,
 * so nothing auto-repeats. Keys are named as in EKeys (Enter, Down, SpaceBar, One, Q...).
 *
 * The presses go through FSlateApplication like a real keyboard: DreamGUI's input pre-processor sees them first, and
 * what it leaves reaches the game viewport and the player controller. Game time runs on the fixed step of an
 * off-screen recording, so a script plays the same on every take.
 */
UCLASS()
class UDemoKeyScript : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	struct FStep
	{
		double Time = 0.0;
		FKey Key;
		bool bDown = true;
	};

	void Send(const FStep& Step) const;

	TArray<FStep> Steps;
	int32 Next = 0;
	double Clock = 0.0;
	bool bPlaying = false;
};
