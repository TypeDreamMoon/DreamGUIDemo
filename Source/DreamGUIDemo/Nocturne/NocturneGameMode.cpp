// Copyright DreamMoon. All Rights Reserved.

#include "NocturneGameMode.h"

#include "NocturneDirector.h"
#include "NocturneSpace.h"

#include "Engine/World.h"

ANocturneGameMode::ANocturneGameMode()
{
	DefaultPawnClass = nullptr;
}

void ANocturneGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	ANocturneSpace* Space = World->SpawnActor<ANocturneSpace>();

	// Deferred, so the director knows the space before its own BeginPlay runs.
	ANocturneDirector* Director = World->SpawnActorDeferred<ANocturneDirector>(ANocturneDirector::StaticClass(), FTransform::Identity);
	if (Director != nullptr)
	{
		Director->Setup(Space);
		Director->FinishSpawning(FTransform::Identity);
	}
}
