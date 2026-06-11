#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TileStreamingManager.generated.h"

class ULidarPointCloud;
class ULidarPointCloudComponent;

// One source tile discovered on disk.
USTRUCT()
struct FPointCloudTile
{
    GENERATED_BODY()

    // Absolute path to the tile file (.laz/.las).
    UPROPERTY()
    FString FilePath;

    // World-space (UE units, rebased by GlobalOrigin) AABB used for selection.
    FBox Bounds = FBox(ForceInit);

    // Center of Bounds, cached for distance tests.
    FVector Center = FVector::ZeroVector;
};

// Runtime state for a tile that is loaded or loading.
USTRUCT()
struct FResidentTile
{
    GENERATED_BODY()

    UPROPERTY()
    ULidarPointCloud* Cloud = nullptr;

    UPROPERTY()
    ULidarPointCloudComponent* Component = nullptr;

    // Set true once the async build completed and LocationOffset was rebased.
    bool bReady = false;

    // World time (s) the async load started — used to time out stuck loads.
    float LoadStartTime = 0.0f;
};

/**
 * PROTOTYPE — streams a large point cloud that was split into spatial tiles by
 * an external (non-UE) tool (e.g. LAStools `lastile` / PDAL splitter). Loads the
 * tiles nearest the camera via ULidarPointCloud::CreateFromFile (async), each as
 * its own LidarPointCloudComponent (which self-LODs), and evicts far tiles to
 * keep RAM bounded. Works in nDisplay SBS because it only manages which points
 * exist; UE handles stereo rendering + per-cloud LOD (driven by the left camera).
 *
 * No cook, no pak, no UE on the conversion machine — only tiling.
 */
UCLASS()
class STEREOSCOPICPROJECT_API ATileStreamingManager : public AActor
{
    GENERATED_BODY()

public:
    ATileStreamingManager();

    // Folder containing the .laz/.las tiles. Settable at runtime then call
    // InitializeFromDirectory (or set before BeginPlay + bAutoInitialize).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    FString TileDirectory;

    // Scan TileDirectory at BeginPlay.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    bool bAutoInitialize = true;

    // Source-unit -> UE-unit scale. MUST match the LidarPointCloud import scale
    // (default 100 = metres -> centimetres). Used for tile bounds + rebasing.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    float ImportScale = 100.0f;

    // Load tiles whose bounds are within this distance of the camera (UE units).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    float LoadRadius = 30000.0f;

    // Evict resident tiles once beyond this distance (hysteresis vs LoadRadius).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    float UnloadRadius = 45000.0f;

    // Hard cap on simultaneously resident tiles (RAM/draw budget).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    int32 MaxResidentTiles = 16;

    // If an async tile load hasn't completed within this many seconds, treat it
    // as failed (e.g. missing/locked file whose callback never fires), evict it,
    // and free its budget slot so it can be retried.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tile Streaming")
    float LoadTimeoutSeconds = 30.0f;

    // Scan the directory, read tile headers, compute GlobalOrigin. Safe to call
    // at runtime (e.g. from the loader UI after the operator browses a folder).
    UFUNCTION(BlueprintCallable, Category = "Tile Streaming")
    bool InitializeFromDirectory(const FString& InDirectory);

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

private:
    // Reads the LAS/LAZ public-header bounding box (works on .laz — header is
    // uncompressed). Returns source-unit min/max. False on malformed header.
    static bool ReadLasHeaderBounds(const FString& FilePath, FVector& OutMin, FVector& OutMax);

    FVector GetViewLocation() const;

    void UpdateStreaming();
    void BeginLoadTile(int32 TileIndex);
    void EvictTile(int32 TileIndex);
    void FinalizeTile(int32 TileIndex);

    UPROPERTY()
    TArray<FPointCloudTile> Tiles;

    UPROPERTY()
    TMap<int32, FResidentTile> Resident;

    // Strong refs to clouds whose async build is still in flight. Keeps each
    // cloud alive (not GC'd) until its worker-thread CompletionCallback fires,
    // even if the tile is evicted from Resident mid-build. Released in the
    // game-thread completion handler.
    UPROPERTY()
    TSet<TObjectPtr<ULidarPointCloud>> PendingClouds;

    // Shared rebase origin (UE units) so all tiles render near the world origin
    // with float precision and stay mutually aligned.
    FVector GlobalOrigin = FVector::ZeroVector;

    bool bInitialized = false;
};
