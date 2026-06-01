#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "StereoCapture.generated.h"

/**
 * AStereoCapture
 * Two modes controlled by bVideoMode:
 *
 * VIDEO MODE (bVideoMode=true):
 *   Follows player camera. Each capture uses ShowOnly on its assigned
 *   360-sphere actor (left=Mat_Stereo_0, right=Mat_Stereo_1).
 *   No IPD offset — both captures at same position.
 *   Use for equirectangular 360 video playback.
 *
 * SCENE MODE (bVideoMode=false):
 *   Follows player camera with ±IPD/2 horizontal offset per eye.
 *   No ShowOnly — captures full 3D scene.
 *   Produces real geometric parallax → depth/pop-out on OV projector.
 *   Use for any 3D scene content.
 *
 * Output always goes to LeftRenderTarget / RightRenderTarget.
 * WBP_StereoOutput composites them into OV frame for projector.
 */
UCLASS(BlueprintType, Blueprintable)
class STEREOSCOPICPROJECT_API AStereoCapture : public AActor
{
    GENERATED_BODY()

public:
    AStereoCapture();

protected:
    virtual void BeginPlay() override;

public:
    virtual void Tick(float DeltaTime) override;

    // ----------------------------------------------------------------
    // Capture components
    // ----------------------------------------------------------------
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stereo Capture")
    USceneCaptureComponent2D* LeftCapture;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stereo Capture")
    USceneCaptureComponent2D* RightCapture;

    // ----------------------------------------------------------------
    // Render targets
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture")
    UTextureRenderTarget2D* LeftRenderTarget;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture")
    UTextureRenderTarget2D* RightRenderTarget;

    // ----------------------------------------------------------------
    // Mode toggle
    //   true  = Video/360 mode  (ShowOnly sphere actors, no IPD offset)
    //   false = Scene/3D mode   (full scene, IPD offset applied)
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture|Mode")
    bool bVideoMode;

    // ----------------------------------------------------------------
    // VIDEO MODE: sphere actors to show-only per capture
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture|Video Mode",
        meta = (EditCondition = "bVideoMode"))
    AActor* LeftSphereActor;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture|Video Mode",
        meta = (EditCondition = "bVideoMode"))
    AActor* RightSphereActor;

    // ----------------------------------------------------------------
    // SCENE MODE: inter-pupillary distance in cm (default 6.3 cm)
    // Increase for more depth, decrease to reduce eye strain.
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture|Scene Mode",
        meta = (EditCondition = "!bVideoMode", ClampMin = "1.0", ClampMax = "15.0",
                UIMin = "4.0", UIMax = "10.0"))
    float IPD_cm;

    // ----------------------------------------------------------------
    // Convergence distance in cm — point in scene at screen depth.
    // Objects closer pop out, farther recede.
    // 0 = no toe-in (parallel cameras, safest for comfort).
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture|Scene Mode",
        meta = (EditCondition = "!bVideoMode", ClampMin = "0.0", UIMin = "0.0"))
    float ConvergenceDistance_cm;

    // ----------------------------------------------------------------
    // Shared: camera FOV in degrees
    // ----------------------------------------------------------------
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stereo Capture",
        meta = (ClampMin = "10.0", ClampMax = "170.0"))
    float CaptureFOV;

    // ----------------------------------------------------------------
    // Blueprint callable — switch mode at runtime
    // ----------------------------------------------------------------
    UFUNCTION(BlueprintCallable, Category = "Stereo Capture")
    void SetVideoMode(bool bEnable);
    
    UFUNCTION(BlueprintCallable, Category = "Stereo Capture")
    void ToggleVideoMode();

private:
    void InitCaptures();
    void ApplyShowOnlyLists();
    void ClearShowOnlyLists();
};
