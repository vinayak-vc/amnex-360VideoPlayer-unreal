#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LidarPointCloud.h"
#include "LidarPointCloudComponent.h"
#include "Cluster/IDisplayClusterClusterManager.h"
#include "PointCloudLoaderActor.generated.h"

struct FDisplayClusterClusterEventJson;

UCLASS(Blueprintable, BlueprintType)
class STEREOSCOPICPROJECT_API APointCloudLoaderActor : public AActor
{
    GENERATED_BODY()

public:
    APointCloudLoaderActor();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Point Cloud")
    ULidarPointCloudComponent* PointCloudComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Cloud")
    FString FilePath;

    // Small dev clouds only: imports the raw file in-memory on the game thread.
    // Do NOT use for large (multi-GB) clouds — use LoadFromPak instead.
    UFUNCTION(BlueprintCallable, Category = "Point Cloud")
    void LoadFromPath(const FString& InPath);

    // Production path for large clouds. Mounts an external, pre-cooked .pak that
    // lives on the local disk (NOT shipped inside the build), then loads the
    // streamed ULidarPointCloud asset from it. The octree streams node-by-node
    // from the pak on disk, so RAM stays bounded regardless of cloud size.
    //   PakFilePath        - absolute path to the .pak on the target machine
    //   AssetPackagePath   - object path the cloud was cooked under,
    //                        e.g. "/Game/StreamedClouds/PC_Big.PC_Big".
    //                        Leave EMPTY to auto-derive: the first
    //                        ULidarPointCloud found in the mounted pak is used.
    //   InMountPoint       - optional mount-point override; empty = use the
    //                        mount point baked into the pak (recommended).
    UFUNCTION(BlueprintCallable, Category = "Point Cloud")
    bool LoadFromPak(const FString& PakFilePath, const FString& AssetPackagePath, const FString& InMountPoint);

    // Cluster-synced entry point. Call this from the UI (on whichever node has
    // focus). In an nDisplay cluster it emits a JSON cluster event so EVERY node
    // mounts + loads the same pak in lockstep; outside a cluster it just calls
    // LoadFromPak locally. The pak file must already exist at PakFilePath on
    // every node's local disk.
    UFUNCTION(BlueprintCallable, Category = "Point Cloud")
    void RequestLoadPak(const FString& PakFilePath, const FString& AssetPackagePath, const FString& InMountPoint);

private:
    // Receives the replicated cluster event on every node and performs the load.
    void HandleClusterEvent(const FDisplayClusterClusterEventJson& Event);

    FOnClusterEventJsonListener ClusterListener;
    bool bClusterListenerRegistered = false;
};
