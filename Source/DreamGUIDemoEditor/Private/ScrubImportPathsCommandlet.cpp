// Copyright DreamMoon. All Rights Reserved.

#include "ScrubImportPathsCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorFramework/AssetImportData.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY_STATIC(LogScrubImportPaths, Log, All);

namespace ScrubImportPathsLocal
{
	/** "X:/a/b" or "X:\a\b": a drive letter and at least two separators, so a lone "X:\" in a text is left alone. */
	bool IsAbsolutePath(const FString& Value)
	{
		if (Value.Len() < 5 || !FChar::IsAlpha(Value[0]) || Value[1] != TEXT(':') || (Value[2] != TEXT('/') && Value[2] != TEXT('\\')))
		{
			return false;
		}
		int32 Separators = 0;
		for (const TCHAR C : Value)
		{
			Separators += (C == TEXT('/') || C == TEXT('\\')) ? 1 : 0;
		}
		return Separators >= 2;
	}

	/**
	 * The portable spelling of an absolute path: project-relative when the file is in this project (directly, or
	 * under the same tail as in the checkout it was imported from); otherwise just the file name.
	 */
	FString Portable(const FString& Absolute)
	{
		const FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		FString Path = Absolute.Replace(TEXT("\\"), TEXT("/"));
		FPaths::CollapseRelativeDirectories(Path);

		if (Path.StartsWith(ProjectDir, ESearchCase::IgnoreCase))
		{
			return Path.RightChop(ProjectDir.Len());
		}
		// Another checkout: the shortest tail that names an existing file of this project wins only if it has a
		// folder in it (a bare file name could match an unrelated file at the project root).
		int32 Index = 0;
		while ((Index = Path.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, Index + 1)) != INDEX_NONE)
		{
			const FString Tail = Path.RightChop(Index + 1);
			if (Tail.Contains(TEXT("/")) && FPaths::FileExists(ProjectDir / Tail))
			{
				return Tail;
			}
		}
		return FPaths::GetCleanFilename(Path);
	}

	/** Swaps the object's import data for plain import data with portable, package-relative source files. */
	bool ScrubImportData(UObject* Object, FObjectProperty* Property, TArray<FString>& OutChanges)
	{
		UAssetImportData* Old = Cast<UAssetImportData>(Property->GetObjectPropertyValue_InContainer(Object));
		if (Old == nullptr)
		{
			return false;
		}
		UPackage* Package = Object->GetOutermost();
		const bool bInterchange = Old->GetClass() != UAssetImportData::StaticClass();

		FAssetImportInfo Clean;
		bool bChanged = bInterchange;
		for (const FAssetImportInfo::FSourceFile& File : Old->GetSourceData().SourceFiles)
		{
			const FString Resolved = UAssetImportData::ResolveImportFilename(File.RelativeFilename, Package);
			const FString Relative = Portable(Resolved);
			const FString Stored = Relative.Contains(TEXT("/"))
				? UAssetImportData::SanitizeImportFilename(FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()) / Relative, Package)
				: Relative;
			bChanged |= Stored != File.RelativeFilename;
			Clean.SourceFiles.Emplace(Stored, File.Timestamp, File.FileHash, File.DisplayLabelName);
		}
		if (!bChanged)
		{
			return false;
		}

		const FName Name = Old->GetFName();
		Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
		Old->MarkAsGarbage();
		UAssetImportData* Fresh = NewObject<UAssetImportData>(Object, Name, Old->GetFlags() & RF_PropagateToSubObjects);
		Fresh->SourceData = MoveTemp(Clean);
		Property->SetObjectPropertyValue_InContainer(Object, Fresh);
		OutChanges.Add(FString::Printf(TEXT("%s.%s (%s) -> %s"), *Object->GetName(), *Property->GetName(),
			*Old->GetClass()->GetName(), *Fresh->SourceData.ToJson()));
		return true;
	}

	bool ScrubStrings(UObject* Object, TArray<FString>& OutChanges)
	{
		bool bChanged = false;
		for (TPropertyValueIterator<FStrProperty> It(Object->GetClass(), Object); It; ++It)
		{
			FString* Value = static_cast<FString*>(const_cast<void*>(It.Value()));
			if (Value && IsAbsolutePath(*Value))
			{
				const FString Fixed = Portable(*Value);
				OutChanges.Add(FString::Printf(TEXT("%s.%s: %s -> %s"), *Object->GetName(), *It.Key()->GetName(), **Value, *Fixed));
				*Value = Fixed;
				bChanged = true;
			}
		}
		return bChanged;
	}

	bool ScrubMetaData(UPackage* Package, TArray<FString>& OutChanges)
	{
		bool bChanged = false;
		FMetaData& MetaData = Package->GetMetaData();
		auto Fix = [&](TMap<FName, FString>& Values)
		{
			for (TPair<FName, FString>& Pair : Values)
			{
				if (IsAbsolutePath(Pair.Value))
				{
					const FString Fixed = Portable(Pair.Value);
					OutChanges.Add(FString::Printf(TEXT("metadata %s: %s -> %s"), *Pair.Key.ToString(), *Pair.Value, *Fixed));
					Pair.Value = Fixed;
					bChanged = true;
				}
			}
		};
		for (TPair<FSoftObjectPath, TMap<FName, FString>>& Entry : MetaData.ObjectMetaDataMap)
		{
			Fix(Entry.Value);
		}
		Fix(MetaData.RootMetaDataMap);
		return bChanged;
	}
}

UScrubImportPathsCommandlet::UScrubImportPathsCommandlet()
{
	IsClient = false;
	IsEditor = true;
	IsServer = false;
	LogToConsole = true;
}

int32 UScrubImportPathsCommandlet::Main(const FString& Params)
{
	using namespace ScrubImportPathsLocal;

	TArray<FString> Tokens, Switches;
	TMap<FString, FString> Values;
	ParseCommandLine(*Params, Tokens, Switches, Values);
	const FString Root = Values.Contains(TEXT("Path")) ? Values[TEXT("Path")] : TEXT("/Game");
	const bool bDryRun = Switches.Contains(TEXT("DryRun"));

	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	Registry.SearchAllAssets(true);
	TArray<FAssetData> Assets;
	Registry.GetAssetsByPath(FName(*Root), Assets, true);
	TSet<FName> PackageNames;
	for (const FAssetData& Asset : Assets)
	{
		PackageNames.Add(Asset.PackageName);
	}

	int32 Changed = 0, Failed = 0;
	for (const FName& PackageName : PackageNames)
	{
		UPackage* Package = LoadPackage(nullptr, *PackageName.ToString(), LOAD_NoWarn | LOAD_Quiet);
		if (Package == nullptr)
		{
			UE_LOG(LogScrubImportPaths, Error, TEXT("%s: could not load"), *PackageName.ToString());
			++Failed;
			continue;
		}

		TArray<FString> Changes;
		bool bDirty = false;
		TArray<UObject*> Objects;
		GetObjectsWithPackage(Package, Objects, EGetObjectsFlags::IncludeNestedObjects);
		for (UObject* Object : Objects)
		{
			if (Object->IsA<UAssetImportData>())
			{
				continue;
			}
			for (TFieldIterator<FObjectProperty> It(Object->GetClass()); It; ++It)
			{
				if (It->PropertyClass && It->PropertyClass->IsChildOf<UAssetImportData>())
				{
					bDirty |= ScrubImportData(Object, *It, Changes);
				}
			}
			bDirty |= ScrubStrings(Object, Changes);
		}
		bDirty |= ScrubMetaData(Package, Changes);

		if (!bDirty)
		{
			continue;
		}
		++Changed;
		for (const FString& Change : Changes)
		{
			UE_LOG(LogScrubImportPaths, Display, TEXT("%s: %s"), *PackageName.ToString(), *Change);
		}
		if (bDryRun)
		{
			continue;
		}

		const FString Extension = Package->ContainsMap() ? FPackageName::GetMapPackageExtension() : FPackageName::GetAssetPackageExtension();
		const FString File = FPackageName::LongPackageNameToFilename(PackageName.ToString(), Extension);
		Package->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		if (!UPackage::SavePackage(Package, nullptr, *File, Args))
		{
			UE_LOG(LogScrubImportPaths, Error, TEXT("%s: NOT SAVED"), *PackageName.ToString());
			++Failed;
		}
	}

	UE_LOG(LogScrubImportPaths, Display, TEXT("ScrubImportPaths: %d of %d packages %s, %d failed"),
		Changed, PackageNames.Num(), bDryRun ? TEXT("would change") : TEXT("changed"), Failed);
	return Failed == 0 ? 0 : 1;
}
