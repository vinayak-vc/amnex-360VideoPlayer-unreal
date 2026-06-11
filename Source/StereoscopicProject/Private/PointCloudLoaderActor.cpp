#include "PointCloudLoaderActor.h"
#include "IPlatformFilePak.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/Paths.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"
#include "IDisplayCluster.h"
#include "Cluster/DisplayClusterClusterEvent.h"

namespace
{
    // Cluster event identifiers for the synced pak load.
    const TCHAR* PakLoadEventName = TEXT("LoadPointCloudPak");
}

APointCloudLoaderActor::APointCloudLoaderActor()
{
    PrimaryActorTick.bCanEverTick = false;

    PointCloudComponent = CreateDefaultSubobject<ULidarPointCloudComponent>(TEXT("PointCloudComponent"));
    RootComponent = PointCloudComponent;

    FilePath = TEXT("C:/UnrealProject/model/Tikal-13.ply");
}

void APointCloudLoaderActor::BeginPlay()
{
    Super::BeginPlay();

    // Listen for the cluster-replicated pak-load event on every node.
    if (IDisplayCluster::IsAvailable())
    {
        if (IDisplayClusterClusterManager* ClusterMgr = IDisplayCluster::Get().GetClusterMgr())
        {
            ClusterListener = FOnClusterEventJsonListener::CreateUObject(
                this, &APointCloudLoaderActor::HandleClusterEvent);
            ClusterMgr->AddClusterEventJsonListener(ClusterListener);
            bClusterListenerRegistered = true;
        }
    }

    if (!FilePath.IsEmpty())
        LoadFromPath(FilePath);
}

void APointCloudLoaderActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bClusterListenerRegistered && IDisplayCluster::IsAvailable())
    {
        if (IDisplayClusterClusterManager* ClusterMgr = IDisplayCluster::Get().GetClusterMgr())
        {
            ClusterMgr->RemoveClusterEventJsonListener(ClusterListener);
        }
        bClusterListenerRegistered = false;
    }

    Super::EndPlay(EndPlayReason);
}

void APointCloudLoaderActor::LoadFromPath(const FString& InPath)
{
    if (InPath.IsEmpty()) return;

    FilePath = InPath;
    FString TargetPath = InPath;

    if (InPath.EndsWith(TEXT(".ply"), ESearchCase::IgnoreCase))
    {
        FString LasPath = FPaths::ChangeExtension(InPath, TEXT("las"));
        if (!FPaths::FileExists(LasPath))
        {
            FString EnginePythonPath = FPaths::ConvertRelativePathToFull(
                FPaths::EngineDir() / TEXT("Binaries/ThirdParty/Python3/Win64/python.exe"));
            FString ScriptPath = FPaths::ProjectDir() + TEXT("ConvertPlyToLas.py");
            FString Args = FString::Printf(TEXT("\"%s\" \"%s\" \"%s\""), *ScriptPath, *InPath, *LasPath);

            UE_LOG(LogTemp, Warning, TEXT("Converting PLY to LAS..."));
            uint32 ProcessID = 0;
            FProcHandle ProcHandle = FPlatformProcess::CreateProc(
                *EnginePythonPath, *Args, false, true, true, &ProcessID, 0, nullptr, nullptr, nullptr);
            if (ProcHandle.IsValid())
            {
                FPlatformProcess::WaitForProc(ProcHandle);
                FPlatformProcess::CloseProc(ProcHandle);
                UE_LOG(LogTemp, Warning, TEXT("Conversion complete."));
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Python launch failed — ensure engine Python exists."));
            }
        }
        TargetPath = LasPath;
    }

    ULidarPointCloud* PC = ULidarPointCloud::CreateFromFile(TargetPath);
    if (PC)
    {
        PointCloudComponent->SetPointCloud(PC);
        UE_LOG(LogTemp, Warning, TEXT("Loaded point cloud: %s"), *TargetPath);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load point cloud: %s"), *TargetPath);
    }
}

bool APointCloudLoaderActor::LoadFromPak(const FString& PakFilePath, const FString& AssetPackagePath, const FString& InMountPoint)
{
    // AssetPackagePath MAY be empty — that selects the auto-derive branch below
    // (first ULidarPointCloud found in the mounted pak). Only the pak path is
    // mandatory.
    if (PakFilePath.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("LoadFromPak: empty pak path."));
        return false;
    }

    if (!FPaths::FileExists(PakFilePath))
    {
        UE_LOG(LogTemp, Error, TEXT("LoadFromPak: pak not found on disk: %s"), *PakFilePath);
        return false;
    }

    // Get the pak platform file from the chain (present in a -pak packaged build).
    // If absent (e.g. editor / loose-file build), layer a new one over the current
    // platform file so we can still mount the external pak.
    FPakPlatformFile* PakPlatform = static_cast<FPakPlatformFile*>(
        FPlatformFileManager::Get().FindPlatformFile(FPakPlatformFile::GetTypeName()));

    if (!PakPlatform)
    {
        PakPlatform = new FPakPlatformFile();
        IPlatformFile& LowerLevel = FPlatformFileManager::Get().GetPlatformFile();
        if (!PakPlatform->Initialize(&LowerLevel, TEXT("")))
        {
            UE_LOG(LogTemp, Error, TEXT("LoadFromPak: failed to initialize FPakPlatformFile."));
            delete PakPlatform;
            return false;
        }
        FPlatformFileManager::Get().SetPlatformFile(*PakPlatform);
    }

    // Empty mount point -> use the mount point baked into the pak header (recommended:
    // it matches how the asset was cooked, so /Game/... package paths resolve).
    const TCHAR* MountPointArg = InMountPoint.IsEmpty() ? nullptr : *InMountPoint;
    if (!PakPlatform->Mount(*PakFilePath, /*PakOrder=*/0, MountPointArg))
    {
        UE_LOG(LogTemp, Error, TEXT("LoadFromPak: failed to mount pak: %s"), *PakFilePath);
        return false;
    }
    UE_LOG(LogTemp, Warning, TEXT("LoadFromPak: mounted %s"), *PakFilePath);

    // Resolve the cloud object. LoadObject pulls only the package header; point
    // data streams lazily from the pak as the LOD manager requests nodes.
    ULidarPointCloud* PC = nullptr;
    FString ResolvedPath = AssetPackagePath;

    if (AssetPackagePath.IsEmpty())
    {
        // Derive: enumerate the just-mounted pak's contents and load the first
        // asset that is a ULidarPointCloud. No object path typing needed.
        TArray<FString> PakFiles;
        PakPlatform->GetPrunedFilenamesInPakFile(PakFilePath, PakFiles);

        for (const FString& File : PakFiles)
        {
            if (!File.EndsWith(TEXT(".uasset"), ESearchCase::IgnoreCase))
                continue;

            FString PackageName;
            if (!FPackageName::TryConvertFilenameToLongPackageName(File, PackageName))
                continue;

            // Default object in a package shares the package's short name.
            const FString ObjPath = PackageName + TEXT(".") + FPackageName::GetShortName(PackageName);
            if (ULidarPointCloud* Candidate = LoadObject<ULidarPointCloud>(nullptr, *ObjPath))
            {
                PC = Candidate;
                ResolvedPath = ObjPath;
                break;
            }
        }

        if (!PC)
        {
            UE_LOG(LogTemp, Error,
                TEXT("LoadFromPak: no ULidarPointCloud found in %s. If the pak's directory index was pruned, pass an explicit AssetPackagePath instead."),
                *PakFilePath);
            return false;
        }
    }
    else
    {
        PC = LoadObject<ULidarPointCloud>(nullptr, *AssetPackagePath);
        if (!PC)
        {
            UE_LOG(LogTemp, Error,
                TEXT("LoadFromPak: pak mounted but asset not found: %s (check AssetPackagePath / mount point)."),
                *AssetPackagePath);
            return false;
        }
    }

    PointCloudComponent->SetPointCloud(PC);
    UE_LOG(LogTemp, Warning, TEXT("LoadFromPak: streamed cloud set: %s"), *ResolvedPath);
    return true;
}

void APointCloudLoaderActor::RequestLoadPak(const FString& PakFilePath, const FString& AssetPackagePath, const FString& InMountPoint)
{
    // In a cluster: emit a JSON event. The primary replicates it to all nodes
    // (including this one), and HandleClusterEvent performs the actual load in
    // lockstep at the next frame boundary — keeping every panel in sync.
    if (IDisplayCluster::IsAvailable())
    {
        IDisplayClusterClusterManager* ClusterMgr = IDisplayCluster::Get().GetClusterMgr();
        if (ClusterMgr && IDisplayCluster::Get().GetOperationMode() == EDisplayClusterOperationMode::Cluster)
        {
            FDisplayClusterClusterEventJson Event;
            Event.Name     = PakLoadEventName;
            Event.Type     = TEXT("PointCloud");
            Event.Category = TEXT("Streaming");
            Event.Parameters.Add(TEXT("PakPath"),    PakFilePath);
            Event.Parameters.Add(TEXT("AssetPath"),  AssetPackagePath);
            Event.Parameters.Add(TEXT("MountPoint"), InMountPoint);
            // Reload allowed even if params repeat (e.g. operator re-triggers).
            Event.bShouldDiscardOnRepeat = false;

            // bPrimaryOnly=false -> deliver to every node.
            ClusterMgr->EmitClusterEventJson(Event, /*bPrimaryOnly=*/false);
            return;
        }
    }

    // Standalone / editor / non-cluster: just load locally.
    LoadFromPak(PakFilePath, AssetPackagePath, InMountPoint);
}

void APointCloudLoaderActor::HandleClusterEvent(const FDisplayClusterClusterEventJson& Event)
{
    if (Event.Name != PakLoadEventName)
        return;

    const FString* Pak   = Event.Parameters.Find(TEXT("PakPath"));
    const FString* Asset = Event.Parameters.Find(TEXT("AssetPath"));
    const FString* Mount = Event.Parameters.Find(TEXT("MountPoint"));

    if (Pak && Asset)
    {
        LoadFromPak(*Pak, *Asset, Mount ? *Mount : FString());
    }
}
