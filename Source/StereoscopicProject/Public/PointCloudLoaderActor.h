#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LidarPointCloud.h"
#include "LidarPointCloudComponent.h"
#include "PointCloudLoaderActor.generated.h"

UCLASS(Blueprintable, BlueprintType)
class STEREOSCOPICPROJECT_API APointCloudLoaderActor : public AActor
{
    GENERATED_BODY()

public:
    APointCloudLoaderActor();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Point Cloud")
    ULidarPointCloudComponent* PointCloudComponent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Point Cloud")
    FString FilePath;

    // Loads a point cloud from a raw file path. For PointForge-converted files,
    // call only after FPFConvert::IsConverted() returns true.
    UFUNCTION(BlueprintCallable, Category = "Point Cloud")
    void LoadFromPath(const FString& InPath);
};
