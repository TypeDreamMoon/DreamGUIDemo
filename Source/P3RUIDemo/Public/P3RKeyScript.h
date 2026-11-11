// Copyright DreamMoon. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Subsystems/WorldSubsystem.h"
#include "P3RKeyScript.generated.h"

/**
 * Scripted key presses for recordings, timed from the world's BeginPlay:
 *   -P3RKeys=1.0:Tab,2.4:Down,3.0:Enter
 * is Tab at 1.0 s, Down at 2.4 s, Enter at 3.0 s; `time:Key/hold` holds the key. The pointer is scripted the same way:
 * `time:Move@X;Y` moves it to (X, Y) in the 1920 x 1080 design frame (from the top left) over 0.15 s, `time:Click@X;Y`
 * moves it there at once and clicks. Everything goes through FSlateApplication like a real keyboard and mouse, so
 * DreamGUI's input sees it first. Off-screen recordings run on a fixed step, so a script plays the same on every take.
 */
UCLASS()
class P3RUIDEMO_API UP3RKeyScript : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;

private:
	enum class EKind : uint8 { Key, Move, Click };
	struct FStep
	{
		double Time = 0.0;
		EKind Kind = EKind::Key;
		FKey Key;
		bool bDown = true;
		FVector2D Point = FVector2D::ZeroVector;
	};

	void Send(const FStep& Step);
	/** The pointer to a design-frame point, as the platform would report it (screen space). */
	void MovePointer(const FVector2D& Design);
	FVector2D ToScreen(const FVector2D& Design) const;

	/** A glide in progress: from, to, start, length. */
	FVector2D GlideFrom = FVector2D::ZeroVector;
	FVector2D GlideTo = FVector2D::ZeroVector;
	double GlideAt = -1.0;
	FVector2D Pointer = FVector2D(960.0, 540.0);

	TArray<FStep> Steps;
	int32 Next = 0;
	double Clock = 0.0;
	bool bPlaying = false;
};
