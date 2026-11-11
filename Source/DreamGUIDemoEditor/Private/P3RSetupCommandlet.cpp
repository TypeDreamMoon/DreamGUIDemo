// Copyright DreamMoon. All Rights Reserved.

#include "P3RSetupCommandlet.h"

#include "P3RDirector.h"

#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Core/DreamUIFontData_DistanceField.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Factories/WorldFactory.h"
#include "FileHelpers.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogP3RSetup, Log, All);

namespace P3RSetupLocal
{
	bool Save(UObject* Asset)
	{
		UPackage* Package = Asset->GetOutermost();
		Package->MarkPackageDirty();
		const FString Extension = Asset->IsA<UWorld>() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
		const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), Extension);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		const bool bSaved = UPackage::SavePackage(Package, Asset, *File, Args);
		UE_LOG(LogP3RSetup, Display, TEXT("%s %s"), *Package->GetName(), bSaved ? TEXT("saved") : TEXT("NOT SAVED"));
		return bSaved;
	}

	template <typename T>
	T* LoadOrMake(const FString& PackagePath)
	{
		const FString Name = FPackageName::GetLongPackageAssetName(PackagePath);
		if (T* Existing = LoadObject<T>(nullptr, *(PackagePath + TEXT(".") + Name), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			return Existing;
		}
		UPackage* Package = CreatePackage(*PackagePath);
		T* Made = NewObject<T>(Package, FName(*Name), RF_Public | RF_Standalone);
		FAssetRegistryModule::AssetCreated(Made);
		return Made;
	}
}

UP3RSetupCommandlet::UP3RSetupCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UP3RSetupCommandlet::Main(const FString& Params)
{
	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Values;
	ParseCommandLine(*Params, Tokens, Switches, Values);
	const bool bAll = !Switches.Contains(TEXT("Textures")) && !Switches.Contains(TEXT("Fonts")) && !Switches.Contains(TEXT("Map"));

	bool bOk = true;
	if (bAll || Switches.Contains(TEXT("Textures")))
	{
		bOk &= ImportTextures();
	}
	if (bAll || Switches.Contains(TEXT("Fonts")))
	{
		bOk &= MakeFonts();
	}
	if (bAll || Switches.Contains(TEXT("Map")))
	{
		bOk &= MakeMap();
	}
	UE_LOG(LogP3RSetup, Display, TEXT("P3RSetup: %s"), bOk ? TEXT("done") : TEXT("FAILED"));
	return bOk ? 0 : 1;
}

bool UP3RSetupCommandlet::ImportTextures()
{
	const FString Source = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Art/P3R/Final")));
	TArray<FString> Files;
	IFileManager::Get().FindFiles(Files, *FPaths::Combine(Source, TEXT("*.png")), true, false);
	if (Files.IsEmpty())
	{
		UE_LOG(LogP3RSetup, Error, TEXT("No pictures in %s (run Art/P3R/prepare_art.py)."), *Source);
		return false;
	}

	TArray<UAssetImportTask*> Tasks;
	for (const FString& File : Files)
	{
		UAssetImportTask* Task = NewObject<UAssetImportTask>();
		Task->Filename = FPaths::Combine(Source, File);
		Task->DestinationPath = TEXT("/Game/P3R/Textures");
		Task->DestinationName = FPaths::GetBaseFilename(File);
		Task->bAutomated = true;
		Task->bReplaceExisting = true;
		Task->bSave = false;
		Tasks.Add(Task);
	}
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
	AssetTools.ImportAssetTasks(Tasks);

	bool bOk = true;
	for (const UAssetImportTask* Task : Tasks)
	{
		const FString Path = FString::Printf(TEXT("/Game/P3R/Textures/%s.%s"), *Task->DestinationName, *Task->DestinationName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *Path);
		if (Texture == nullptr)
		{
			UE_LOG(LogP3RSetup, Error, TEXT("%s did not import."), *Task->Filename);
			bOk = false;
			continue;
		}
		Texture->LODGroup = TEXTUREGROUP_UI;
		Texture->CompressionSettings = TC_EditorIcon;
		Texture->MipGenSettings = TMGS_SimpleAverage;
		Texture->SRGB = true;
		Texture->NeverStream = true;
		Texture->PostEditChange();
		bOk &= P3RSetupLocal::Save(Texture);
	}
	return bOk;
}

bool UP3RSetupCommandlet::MakeFonts()
{
	struct FFontSpec
	{
		const TCHAR* Asset;
		const TCHAR* File;      // relative to the project
		const TCHAR* Fallback;  // the CJK font asset, or null for a CJK font itself
	};
	const FFontSpec Specs[] = {
		{ TEXT("F_P3R_CJKHeavy"), TEXT("Art/P3R/Fonts/SourceHanSansSC-Heavy.otf"), nullptr },
		{ TEXT("F_P3R_CJKBold"), TEXT("Art/P3R/Fonts/SourceHanSansSC-Bold.otf"), nullptr },
		{ TEXT("F_P3R_CJKMedium"), TEXT("Art/P3R/Fonts/SourceHanSansSC-Medium.otf"), nullptr },
		{ TEXT("F_P3R_CJKLight"), TEXT("Art/P3R/Fonts/SourceHanSansSC-Light.otf"), nullptr },
		{ TEXT("F_P3R_Heavy"), TEXT("Art/P3R/Fonts/P3R-Heavy.ttf"), TEXT("F_P3R_CJKHeavy") },
		{ TEXT("F_P3R_Bold"), TEXT("Art/P3R/Fonts/P3R-Bold.ttf"), TEXT("F_P3R_CJKBold") },
		{ TEXT("F_P3R_Light"), TEXT("Art/P3R/Fonts/P3R-Light.ttf"), TEXT("F_P3R_CJKMedium") },
		{ TEXT("F_P3R_Thin"), TEXT("Art/P3R/Fonts/P3R-Thin.ttf"), TEXT("F_P3R_CJKLight") },
	};

	bool bOk = true;
	TMap<FString, UDreamUIFontData_DistanceField*> Made;
	for (const FFontSpec& Spec : Specs)
	{
		if (!FPaths::FileExists(FPaths::Combine(FPaths::ProjectDir(), Spec.File)))
		{
			UE_LOG(LogP3RSetup, Error, TEXT("Missing %s (run Art/P3R/Fonts/make_fonts.py)."), Spec.File);
			bOk = false;
			continue;
		}
		UDreamUIFontData_DistanceField* Font = P3RSetupLocal::LoadOrMake<UDreamUIFontData_DistanceField>(FString(TEXT("/Game/P3R/Fonts/")) + Spec.Asset);
		Font->SetFontFilePath(Spec.File, /*bInRelativeToProjectDir*/true);
		TArray<FDreamUIFontFallback> Fallbacks;
		if (Spec.Fallback != nullptr)
		{
			if (UDreamUIFontData_DistanceField* const* Cjk = Made.Find(Spec.Fallback))
			{
				FDreamUIFontFallback Entry;
				Entry.Font = *Cjk;
				Fallbacks.Add(Entry);
			}
		}
		Font->SetFallbacks(Fallbacks);
		Made.Add(Spec.Asset, Font);
		bOk &= P3RSetupLocal::Save(Font);
	}
	return bOk;
}

bool UP3RSetupCommandlet::MakeMap()
{
	const FString PackagePath = TEXT("/Game/P3R/Maps/P3R_Demo");
	UWorld* World = LoadObject<UWorld>(nullptr, *(PackagePath + TEXT(".P3R_Demo")), nullptr, LOAD_NoWarn | LOAD_Quiet);
	if (World == nullptr)
	{
		UWorldFactory* Factory = NewObject<UWorldFactory>();
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		World = Cast<UWorld>(AssetTools.CreateAsset(TEXT("P3R_Demo"), TEXT("/Game/P3R/Maps"), UWorld::StaticClass(), Factory));
	}
	if (World == nullptr)
	{
		UE_LOG(LogP3RSetup, Error, TEXT("Could not make %s."), *PackagePath);
		return false;
	}
	if (AWorldSettings* Settings = World->GetWorldSettings())
	{
		Settings->DefaultGameMode = AP3RGameMode::StaticClass();
	}
	return P3RSetupLocal::Save(World);
}
