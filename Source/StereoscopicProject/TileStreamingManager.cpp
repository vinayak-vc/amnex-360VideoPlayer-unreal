#include "TileStreamingManager.h"
#include "LidarPointCloud.h"
#include "LidarPointCloudComponent.h"
#include "LidarPointCloudSettings.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Async/Async.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/PlayerCameraManager.h"

ATileStreamingManager::ATileStreamingManager()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATileStreamingManager::BeginPlay()
{
    Super::BeginPlay();
    if (bAutoInitialize && !TileDirectory.IsEmpty())
        InitializeFromDirectory(TileDirectory);
}

void ATileStreamingManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    TArray<int32> Keys;
    Resident.GetKeys(Keys);
    for (int32 Key : Keys)
        EvictTile(Key);

    Super::EndPlay(EndPlayReason);
}

bool ATileStreamingManager::InitializeFromDirectory(const FString& InDirectory)
{
    Tiles.Reset();
    bInitialized = false;

    TileDirectory = InDirectory;

    // Gather tile files.
    TArray<FString> Files;
    IFileManager& FM = IFileManager::Get();
    FM.FindFiles(Files, *(InDirectory / TEXT("*.laz")), /*Files=*/true, /*Dirs=*/false);
    {
        TArray<FString> LasFiles;
        FM.FindFiles(LasFiles, *(InDirectory / TEXT("*.las")), true, false);
        Files.Append(LasFiles);
    }

    if (Files.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("TileStreaming: no .laz/.las tiles in %s"), *InDirectory);
        return false;
    }

    // First pass: read header bounds (source units, metres for LAS).
    struct FRaw { FString Path; FVector Min; FVector Max; };
    TArray<FRaw> Raws;
    Raws.Reserve(Files.Num());

    for (const FString& File : Files)
    {
        const FString FullPath = InDirectory / File;
        FVector Min, Max;
        if (!ReadLasHeaderBounds(FullPath, Min, Max))
        {
            UE_LOG(LogTemp, Warning, TEXT("TileStreaming: skipping (bad header): %s"), *FullPath);
            continue;
        }
        Raws.Add({ FullPath, Min, Max });
    }

    if (Raws.Num() == 0)
        return false;

    // The LidarPointCloud LAS importer multiplies source units by its own
    // ImportScale and NEGATES Y (LidarPointCloudFileIO_LAS). Read that exact
    // scale and apply the same transform so our pre-load selection bounds live
    // in the SAME frame the points actually render in (and so GlobalOrigin is
    // comparable to each cloud's OriginalCoordinates at FinalizeTile).
    const double Scale = GetDefault<ULidarPointCloudSettings>()->ImportScale;
    ImportScale = (float)Scale; // reflect the real scale used

    // Convert each tile's source bbox into plugin space (scale + Y negate, which
    // swaps min/max Y) and accumulate the global bounds.
    TArray<FBox> PluginBoxes;
    PluginBoxes.Reserve(Raws.Num());
    FBox GlobalPluginBounds(ForceInit);
    for (const FRaw& R : Raws)
    {
        const FVector PMin(R.Min.X * Scale, -R.Max.Y * Scale, R.Min.Z * Scale);
        const FVector PMax(R.Max.X * Scale, -R.Min.Y * Scale, R.Max.Z * Scale);
        const FBox PluginBox(PMin, PMax);
        PluginBoxes.Add(PluginBox);
        GlobalPluginBounds += PluginBox;
    }

    // Shared rebase origin in PLUGIN space — matches Cloud->OriginalCoordinates,
    // so FinalizeTile's (OriginalCoordinates - GlobalOrigin) is correct, and the
    // tile world bounds below are in the rendered frame.
    GlobalOrigin = GlobalPluginBounds.GetCenter();

    for (int32 i = 0; i < Raws.Num(); ++i)
    {
        FPointCloudTile Tile;
        Tile.FilePath = Raws[i].Path;
        Tile.Bounds = PluginBoxes[i].ShiftBy(-GlobalOrigin);
        Tile.Center = Tile.Bounds.GetCenter();
        Tiles.Add(Tile);
    }

    bInitialized = true;
    UE_LOG(LogTemp, Warning, TEXT("TileStreaming: initialized %d tiles from %s"), Tiles.Num(), *InDirectory);
    return true;
}

bool ATileStreamingManager::ReadLasHeaderBounds(const FString& FilePath, FVector& OutMin, FVector& OutMax)
{
    // LAS public header block: signature "LASF", legacy bbox doubles at fixed
    // offsets (valid LAS 1.0-1.4). LAZ keeps this header uncompressed at the
    // file start, so the same read works for compressed files.
    TUniquePtr<FArchive> Reader(IFileManager::Get().CreateFileReader(*FilePath));
    if (!Reader)
        return false;

    const int64 HeaderSize = 227;
    if (Reader->TotalSize() < HeaderSize)
        return false;

    uint8 Header[227];
    Reader->Serialize(Header, HeaderSize);
    Reader->Close();

    if (Header[0] != 'L' || Header[1] != 'A' || Header[2] != 'S' || Header[3] != 'F')
        return false;

    auto ReadDouble = [&Header](int32 Offset) -> double
    {
        double Value = 0.0;
        FMemory::Memcpy(&Value, &Header[Offset], sizeof(double));
        return Value;
    };

    // Offsets: MaxX 179, MinX 187, MaxY 195, MinY 203, MaxZ 211, MinZ 219.
    const double MaxX = ReadDouble(179);
    const double MinX = ReadDouble(187);
    const double MaxY = ReadDouble(195);
    const double MinY = ReadDouble(203);
    const double MaxZ = ReadDouble(211);
    const double MinZ = ReadDouble(219);

    OutMin = FVector(MinX, MinY, MinZ);
    OutMax = FVector(MaxX, MaxY, MaxZ);
    return true;
}

FVector ATileStreamingManager::GetViewLocation() const
{
    if (APlayerCameraManager* CamMgr = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0))
        return CamMgr->GetCameraLocation();
    return GetActorLocation();
}

void ATileStreamingManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bInitialized)
        UpdateStreaming();
}

void ATileStreamingManager::UpdateStreaming()
{
    const FVector ViewLoc = GetViewLocation();

    const float LoadRadiusSq   = LoadRadius * LoadRadius;
    const float UnloadRadiusSq = UnloadRadius * UnloadRadius;

    // 1) Evict resident tiles beyond the unload radius, or whose async load has
    //    timed out (callback never arrived — e.g. file locked). Safe even while a
    //    build is in flight: PendingClouds keeps the cloud alive until callback.
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    TArray<int32> ResidentKeys;
    Resident.GetKeys(ResidentKeys);
    for (int32 Index : ResidentKeys)
    {
        const FResidentTile* State = Resident.Find(Index);
        const float DistSq = Tiles[Index].Bounds.ComputeSquaredDistanceToPoint(ViewLoc);
        const bool bTimedOut = State && !State->bReady && (Now - State->LoadStartTime) > LoadTimeoutSeconds;
        if (DistSq > UnloadRadiusSq || bTimedOut)
            EvictTile(Index);
    }

    // 2) Candidate tiles within the load radius, nearest first.
    TArray<TPair<float, int32>> Candidates;
    for (int32 Index = 0; Index < Tiles.Num(); ++Index)
    {
        if (Resident.Contains(Index))
            continue;
        const float DistSq = Tiles[Index].Bounds.ComputeSquaredDistanceToPoint(ViewLoc);
        if (DistSq <= LoadRadiusSq)
            Candidates.Add(TPair<float, int32>(DistSq, Index));
    }
    Candidates.Sort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key < B.Key; });

    // 3) Load nearest candidates up to the resident budget.
    for (const TPair<float, int32>& C : Candidates)
    {
        if (Resident.Num() >= MaxResidentTiles)
            break;
        BeginLoadTile(C.Value);
    }
}

void ATileStreamingManager::BeginLoadTile(int32 TileIndex)
{
    if (Resident.Contains(TileIndex) || !Tiles.IsValidIndex(TileIndex))
        return;

    const FPointCloudTile& Tile = Tiles[TileIndex];

    // Guard: if the file is gone, CreateFromFile's async Reimport early-returns
    // WITHOUT firing the completion callback, which would otherwise leave a
    // phantom resident entry occupying budget forever.
    if (!FPaths::FileExists(Tile.FilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("TileStreaming: tile file missing: %s"), *Tile.FilePath);
        return;
    }

    // Holder so the completion callback (fired on a worker thread, then marshalled
    // to the game thread) can release the pending strong-ref for the exact cloud
    // it built. Filled in right after CreateFromFile returns, before any build
    // could finish.
    TSharedRef<TWeakObjectPtr<ULidarPointCloud>> CloudHolder = MakeShared<TWeakObjectPtr<ULidarPointCloud>>();
    TWeakObjectPtr<ATileStreamingManager> WeakThis(this);

    FLidarPointCloudAsyncParameters AsyncParams(
        /*bUseAsync=*/true,
        /*Progress=*/nullptr,
        /*Completion=*/[WeakThis, TileIndex, CloudHolder](bool bSuccess)
        {
            AsyncTask(ENamedThreads::GameThread, [WeakThis, TileIndex, CloudHolder, bSuccess]()
            {
                if (ATileStreamingManager* Self = WeakThis.Get())
                {
                    // Release the in-flight strong ref now that the build is done.
                    Self->PendingClouds.Remove(CloudHolder->Get());

                    if (bSuccess)
                        Self->FinalizeTile(TileIndex);
                    else
                        Self->EvictTile(TileIndex);
                }
            });
        });

    ULidarPointCloud* Cloud = ULidarPointCloud::CreateFromFile(Tile.FilePath, AsyncParams);
    if (!Cloud)
    {
        UE_LOG(LogTemp, Error, TEXT("TileStreaming: CreateFromFile failed: %s"), *Tile.FilePath);
        return;
    }

    *CloudHolder = Cloud;
    // Keep the cloud alive through the entire async build, even if the tile is
    // evicted (distance/timeout) before the callback arrives.
    PendingClouds.Add(Cloud);

    // Create + register the renderer component immediately; it fills in as the
    // octree streams. Held in a UPROPERTY map so neither cloud nor component is
    // garbage-collected while resident.
    ULidarPointCloudComponent* Comp = NewObject<ULidarPointCloudComponent>(this);
    Comp->SetupAttachment(RootComponent);
    Comp->RegisterComponent();
    Comp->SetPointCloud(Cloud);

    FResidentTile State;
    State.Cloud = Cloud;
    State.Component = Comp;
    State.bReady = false;
    State.LoadStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    Resident.Add(TileIndex, State);
}

void ATileStreamingManager::FinalizeTile(int32 TileIndex)
{
    FResidentTile* State = Resident.Find(TileIndex);
    if (!State || !State->Cloud)
        return;

    // Rebase so this tile renders at its true position relative to GlobalOrigin
    // (small floats -> no jitter; all tiles share the same frame -> aligned).
    State->Cloud->SetLocationOffset(State->Cloud->OriginalCoordinates - GlobalOrigin);
    State->bReady = true;
}

void ATileStreamingManager::EvictTile(int32 TileIndex)
{
    FResidentTile State;
    if (!Resident.RemoveAndCopyValue(TileIndex, State))
        return;

    if (State.Component)
    {
        State.Component->SetPointCloud(nullptr);
        State.Component->DestroyComponent();
    }
    // Cloud no longer referenced -> eligible for GC, freeing its octree RAM.
}
