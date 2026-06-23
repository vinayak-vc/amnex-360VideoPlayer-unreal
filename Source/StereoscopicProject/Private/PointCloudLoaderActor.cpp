#include "PointCloudLoaderActor.h"
#include "Misc/Paths.h"
#include "PFConvert.h"

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

    if (!FilePath.IsEmpty() && FPFConvert::IsConverted(FPFConvert::GetCacheDirFor(FilePath)))
        LoadFromPath(FilePath);
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
