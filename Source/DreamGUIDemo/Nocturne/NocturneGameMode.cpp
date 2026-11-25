// Copyright DreamMoon. All Rights Reserved.

#include "NocturneGameMode.h"

#include "NocturneDirector.h"
#include "NocturneSpace.h"

#include "Engine/World.h"
#include "Core/Components/DreamCanvas.h"
#include "Core/DreamScreenUISubsystem.h"

ANocturneGameMode::ANocturneGameMode()
{
	DefaultPawnClass = nullptr;
}

void ANocturneGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
#if PLATFORM_ANDROID
	// The menus are authored in 1920 x 1080 units, not Android scene pixels.
	if (UDreamScreenUISubsystem* Screen = UDreamScreenUISubsystem::Get(World))
	{
		Screen->GetOrCreateScreenRootForUserIndex(0);
		if (UDreamCanvas* Canvas = Screen->GetScreenCanvas())
		{
			Canvas->SetReferenceResolution(FVector2D(1920, 1080));
			Canvas->SetScreenMatchMode(EDreamCanvasScreenMatchMode::Shrink);
			Canvas->SetScaleMode(EDreamCanvasScaleMode::ScaleWithScreenSize);
		}
	}
#endif
	ANocturneSpace* Space = World->SpawnActor<ANocturneSpace>();

	// Deferred, so the director knows the space before its own BeginPlay runs.
	ANocturneDirector* Director = World->SpawnActorDeferred<ANocturneDirector>(ANocturneDirector::StaticClass(), FTransform::Identity);
	if (Director != nullptr)
	{
		Director->Setup(Space);
		Director->FinishSpawning(FTransform::Identity);
	}
}
