#include "PointCloudPakLibrary.h"

#if WITH_EDITOR
#include "LidarPointCloud.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"

namespace
{
    // Run an external process, block until exit, return true on exit code 0.
    bool RunProcessBlocking(const FString& Exe, const FString& Args)
    {
        UE_LOG(LogTemp, Warning, TEXT("PakTool: launching\n  %s %s"), *Exe, *Args);

        if (!FPaths::FileExists(Exe))
        {
            UE_LOG(LogTemp, Error, TEXT("PakTool: executable not found: %s"), *Exe);
            return false;
        }

        FProcHandle Proc = FPlatformProcess::CreateProc(
            *Exe, *Args, /*bLaunchDetached=*/true, /*bLaunchHidden=*/false,
            /*bLaunchReallyHidden=*/false, nullptr, 0, nullptr, nullptr);

        if (!Proc.IsValid())
        {
            UE_LOG(LogTemp, Error, TEXT("PakTool: failed to launch: %s"), *Exe);
            return false;
        }

        FPlatformProcess::WaitForProc(Proc);
        int32 ReturnCode = -1;
        FPlatformProcess::GetProcReturnCode(Proc, &ReturnCode);
        FPlatformProcess::CloseProc(Proc);

        if (ReturnCode != 0)
        {
            UE_LOG(LogTemp, Error, TEXT("PakTool: process exited with code %d"), ReturnCode);
            return false;
        }
        return true;
    }
}
#endif // WITH_EDITOR

bool UPointCloudPakLibrary::ImportPointCloudToAsset(const FString& SourceFilePath, const FString& DestPackagePath, FString& OutObjectPath)
{
#if WITH_EDITOR
    if (!FPaths::FileExists(SourceFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: source file not found: %s"), *SourceFilePath);
        return false;
    }
    if (!FPackageName::IsValidLongPackageName(DestPackagePath))
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: invalid package path (need /Game/...): %s"), *DestPackagePath);
        return false;
    }

    const FString AssetName = FPackageName::GetShortName(DestPackagePath);

    UPackage* Package = CreatePackage(*DestPackagePath);
    if (!Package)
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: failed to create package: %s"), *DestPackagePath);
        return false;
    }
    Package->FullyLoad();

    // Synchronous import (bUseAsync=false) so the octree is fully built before save.
    ULidarPointCloud* Cloud = ULidarPointCloud::CreateFromFile(
        SourceFilePath,
        FLidarPointCloudAsyncParameters(false),
        /*ImportSettings=*/nullptr,
        Package,
        FName(*AssetName),
        RF_Public | RF_Standalone);

    if (!Cloud)
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: import failed: %s"), *SourceFilePath);
        return false;
    }

    FAssetRegistryModule::AssetCreated(Cloud);
    Cloud->MarkPackageDirty();

    const FString FileName = FPackageName::LongPackageNameToFilename(
        DestPackagePath, FPackageName::GetAssetPackageExtension());

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;

    if (!UPackage::SavePackage(Package, Cloud, *FileName, SaveArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: SavePackage failed: %s"), *FileName);
        return false;
    }

    OutObjectPath = DestPackagePath + TEXT(".") + AssetName;
    UE_LOG(LogTemp, Warning, TEXT("PakTool: imported + saved %s"), *OutObjectPath);
    return true;
#else
    return false;
#endif
}

bool UPointCloudPakLibrary::CookAndPakAsset(const FString& PackagePath, const FString& OutPakFilePath)
{
#if WITH_EDITOR
    const FString ProjectFile = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
    const FString ProjectName = FPaths::GetBaseFilename(ProjectFile);
    const FString ProjectDir  = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    const FString EngineDir   = FPaths::ConvertRelativePathToFull(FPaths::EngineDir());
    const FString EditorCmd   = EngineDir / TEXT("Binaries/Win64/UnrealEditor-Cmd.exe");
    const FString UnrealPak   = EngineDir / TEXT("Binaries/Win64/UnrealPak.exe");

    // /Game/StreamedClouds/PC_Big -> "Content/StreamedClouds" (relative to project)
    FString PackageDir = FPackageName::GetLongPackagePath(PackagePath); // /Game/StreamedClouds
    FString RelContentDir = PackageDir;
    RelContentDir.ReplaceInline(TEXT("/Game/"), TEXT("Content/"));
    const FString AssetName = FPackageName::GetShortName(PackagePath);
    const FString CookDirAbs = ProjectDir / RelContentDir;

    // 1) Cook only that content folder for Windows. (Cook runs as a commandlet,
    //    so the UnrealClaude HTTP server's IsRunningCommandlet guard avoids the
    //    port-3000 clash with the open editor.)
    const FString CookArgs = FString::Printf(
        TEXT("\"%s\" -run=Cook -TargetPlatform=Windows -COOKDIR=\"%s\" -unattended -nullrhi -nosplash"),
        *ProjectFile, *CookDirAbs);
    if (!RunProcessBlocking(EditorCmd, CookArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: cook step failed."));
        return false;
    }

    // 2) Collect this asset's cooked files (.uasset/.uexp/.ubulk/...).
    const FString CookedRoot = ProjectDir / TEXT("Saved/Cooked/Windows") / ProjectName / RelContentDir;
    TArray<FString> CookedFiles;
    IFileManager::Get().FindFiles(CookedFiles, *(CookedRoot / (AssetName + TEXT(".*"))), /*Files=*/true, /*Directories=*/false);
    if (CookedFiles.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: no cooked files at %s (cook failed or wrong path)."), *CookedRoot);
        return false;
    }

    // 3) UnrealPak response file. UnrealPak derives the pak mount point from the
    //    common prefix of the mounted paths, so the runtime resolves /Game/... .
    FString Response;
    for (const FString& File : CookedFiles)
    {
        const FString AbsCooked = CookedRoot / File;
        const FString Mounted   = FString::Printf(TEXT("../../../%s/%s/%s"), *ProjectName, *RelContentDir, *File);
        Response += FString::Printf(TEXT("\"%s\" \"%s\"\r\n"), *AbsCooked, *Mounted);
    }
    const FString RespPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("pak_response.txt"));
    if (!FFileHelper::SaveStringToFile(Response, *RespPath))
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: failed to write response file: %s"), *RespPath);
        return false;
    }

    // 4) Pak it.
    const FString PakArgs = FString::Printf(TEXT("\"%s\" -create=\"%s\" -compress"), *OutPakFilePath, *RespPath);
    if (!RunProcessBlocking(UnrealPak, PakArgs))
    {
        UE_LOG(LogTemp, Error, TEXT("PakTool: UnrealPak step failed."));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT("PakTool: created pak %s"), *OutPakFilePath);
    return true;
#else
    return false;
#endif
}

bool UPointCloudPakLibrary::MakePointCloudPak(const FString& SourceFilePath, const FString& DestPackagePath, const FString& OutPakFilePath)
{
#if WITH_EDITOR
    FString ObjectPath;
    if (!ImportPointCloudToAsset(SourceFilePath, DestPackagePath, ObjectPath))
        return false;

    return CookAndPakAsset(DestPackagePath, OutPakFilePath);
#else
    return false;
#endif
}
