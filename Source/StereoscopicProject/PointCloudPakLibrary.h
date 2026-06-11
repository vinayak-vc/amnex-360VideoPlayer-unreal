#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PointCloudPakLibrary.generated.h"

/**
 * Editor-only authoring helpers (Machine 2) for turning a raw LiDAR file into a
 * standalone, streamable .pak that the shipped skeleton build mounts at runtime
 * (see APointCloudLoaderActor::LoadFromPak). Intended to be driven from a
 * one-click Editor Utility Widget.
 *
 * All functions no-op (return false) outside the editor.
 */
UCLASS()
class STEREOSCOPICPROJECT_API UPointCloudPakLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    // Imports a .laz/.las/.ply/etc. synchronously into a new ULidarPointCloud
    // asset at DestPackagePath (e.g. "/Game/StreamedClouds/PC_Big") and saves it.
    // OutObjectPath returns the full object path (".../PC_Big.PC_Big").
    UFUNCTION(BlueprintCallable, Category = "Point Cloud|Authoring")
    static bool ImportPointCloudToAsset(const FString& SourceFilePath, const FString& DestPackagePath, FString& OutObjectPath);

    // Cooks the content folder containing PackagePath for Windows, then paks the
    // cooked files of that asset into OutPakFilePath via UnrealPak.
    UFUNCTION(BlueprintCallable, Category = "Point Cloud|Authoring")
    static bool CookAndPakAsset(const FString& PackagePath, const FString& OutPakFilePath);

    // One-click: import -> save -> cook -> pak. DestPackagePath e.g.
    // "/Game/StreamedClouds/PC_Big"; OutPakFilePath e.g. "C:/out/PointCloud_P.pak".
    UFUNCTION(BlueprintCallable, Category = "Point Cloud|Authoring")
    static bool MakePointCloudPak(const FString& SourceFilePath, const FString& DestPackagePath, const FString& OutPakFilePath);
};
