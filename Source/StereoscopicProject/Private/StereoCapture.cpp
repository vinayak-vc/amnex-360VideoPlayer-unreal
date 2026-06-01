#include "StereoCapture.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

AStereoCapture::AStereoCapture()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    LeftCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("LeftCapture"));
    LeftCapture->SetupAttachment(Root);
    LeftCapture->bCaptureEveryFrame   = true;
    LeftCapture->bCaptureOnMovement   = false;
    LeftCapture->CaptureSource        = ESceneCaptureSource::SCS_FinalColorLDR;

    RightCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("RightCapture"));
    RightCapture->SetupAttachment(Root);
    RightCapture->bCaptureEveryFrame  = true;
    RightCapture->bCaptureOnMovement  = false;
    RightCapture->CaptureSource       = ESceneCaptureSource::SCS_FinalColorLDR;

    // Defaults
    LeftRenderTarget        = nullptr;
    RightRenderTarget       = nullptr;
    LeftSphereActor         = nullptr;
    RightSphereActor        = nullptr;
    bVideoMode              = true;
    IPD_cm                  = 6.3f;
    ConvergenceDistance_cm  = 0.0f;
    CaptureFOV              = 90.0f;
}

void AStereoCapture::BeginPlay()
{
    Super::BeginPlay();
    InitCaptures();
}

void AStereoCapture::InitCaptures()
{
    if (LeftRenderTarget)  LeftCapture->TextureTarget  = LeftRenderTarget;
    if (RightRenderTarget) RightCapture->TextureTarget = RightRenderTarget;

    LeftCapture->FOVAngle  = CaptureFOV;
    RightCapture->FOVAngle = CaptureFOV;

    if (bVideoMode)
    {
        LeftCapture->PrimitiveRenderMode  = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        RightCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        ApplyShowOnlyLists();
    }
    else
    {
        // Scene mode: render everything
        LeftCapture->PrimitiveRenderMode  = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
        RightCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
        ClearShowOnlyLists();
    }
}

void AStereoCapture::ApplyShowOnlyLists()
{
    LeftCapture->ShowOnlyActors.Empty();
    RightCapture->ShowOnlyActors.Empty();

    if (LeftSphereActor)  LeftCapture->ShowOnlyActors.Add(LeftSphereActor);
    if (RightSphereActor) RightCapture->ShowOnlyActors.Add(RightSphereActor);
}

void AStereoCapture::ClearShowOnlyLists()
{
    LeftCapture->ShowOnlyActors.Empty();
    RightCapture->ShowOnlyActors.Empty();
}

void AStereoCapture::ToggleVideoMode()
{
    bVideoMode = !bVideoMode;
    SetVideoMode(bVideoMode);
}

void AStereoCapture::SetVideoMode(bool bEnable)
{
    bVideoMode = bEnable;
    InitCaptures();
}

void AStereoCapture::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC) return;

    APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
    if (!CamMgr) return;

    const FVector  CamLoc = CamMgr->GetCameraLocation();
    const FRotator CamRot = CamMgr->GetCameraRotation();

    LeftCapture->FOVAngle  = CaptureFOV;
    RightCapture->FOVAngle = CaptureFOV;

    if (bVideoMode)
    {
        // Both captures at same position — UV-based eye separation in material
        LeftCapture->SetWorldLocationAndRotation(CamLoc, CamRot);
        RightCapture->SetWorldLocationAndRotation(CamLoc, CamRot);
    }
    else
    {
        // Scene mode: offset each eye by IPD/2 along camera right axis
        const float HalfIPD = (IPD_cm * 0.5f);  // UE units = cm, so direct
        const FVector RightVec = CamRot.RotateVector(FVector::RightVector);

        FVector LeftEyeLoc  = CamLoc - RightVec * HalfIPD;
        FVector RightEyeLoc = CamLoc + RightVec * HalfIPD;

        FRotator LeftEyeRot  = CamRot;
        FRotator RightEyeRot = CamRot;

        // Optional toe-in for convergence (non-zero ConvergenceDistance_cm)
        if (ConvergenceDistance_cm > 0.0f)
        {
            const FVector FwdVec     = CamRot.RotateVector(FVector::ForwardVector);
            const FVector TargetPoint = CamLoc + FwdVec * ConvergenceDistance_cm;

            LeftEyeRot  = (TargetPoint - LeftEyeLoc).ToOrientationRotator();
            RightEyeRot = (TargetPoint - RightEyeLoc).ToOrientationRotator();
        }

        LeftCapture->SetWorldLocationAndRotation(LeftEyeLoc,  LeftEyeRot);
        RightCapture->SetWorldLocationAndRotation(RightEyeLoc, RightEyeRot);
    }
}
