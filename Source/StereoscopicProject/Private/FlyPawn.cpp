#include "FlyPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "PointCloudLoaderWidget.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "DisplayClusterRootActor.h"

AFlyPawn::AFlyPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = Root;

    // Constrain camera to 16:9 so LiDAR LOD's ScreenSizeFactor matches PIE.
    // nDisplay SBS viewport is 3840x1080 (aspect ~3.56) — without this, M[1][1]
    // in the projection matrix is ~2x too large, ScreenSizeFactor is ~4x too large,
    // and all octree nodes appear huge → budget saturates at coarse level → cubes.
    LODCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("LODCamera"));
    LODCamera->SetupAttachment(RootComponent);
    LODCamera->FieldOfView        = 90.0f;
    LODCamera->AspectRatio        = 16.0f / 9.0f;
    LODCamera->bConstrainAspectRatio = true;
}

void AFlyPawn::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("FlyPawn BeginPlay"));
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
            ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }

    if (PointCloudWidgetClass)
    {
        LoaderWidget = CreateWidget<UPointCloudLoaderWidget>(GetWorld(), PointCloudWidgetClass);
        if (LoaderWidget)
        {
            LoaderWidget->AddToViewport();
            LoaderWidget->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void AFlyPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveForwardAction)
            EIC->BindAction(MoveForwardAction, ETriggerEvent::Triggered, this, &AFlyPawn::MoveForward);
        if (MoveBackAction)
            EIC->BindAction(MoveBackAction, ETriggerEvent::Triggered, this, &AFlyPawn::MoveBack);
        if (MoveRightAction)
            EIC->BindAction(MoveRightAction, ETriggerEvent::Triggered, this, &AFlyPawn::MoveRight);
        if (MoveLeftAction)
            EIC->BindAction(MoveLeftAction, ETriggerEvent::Triggered, this, &AFlyPawn::MoveLeft);
        if (MoveUpAction)
            EIC->BindAction(MoveUpAction, ETriggerEvent::Triggered, this, &AFlyPawn::MoveUp);
        if (MoveDownAction)
            EIC->BindAction(MoveDownAction, ETriggerEvent::Triggered, this, &AFlyPawn::MoveDown);
        if (LookAction)
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFlyPawn::Look);
        if (ToggleUIAction)
            EIC->BindAction(ToggleUIAction, ETriggerEvent::Started, this, &AFlyPawn::ToggleUI);
    }
}

void AFlyPawn::MoveForward(const FInputActionValue& Value)
{
    if (!DisplayClusterActor) return;
    CurrentMovementInput = DisplayClusterActor->GetActorForwardVector() * Value.Get<float>();
}

void AFlyPawn::MoveBack(const FInputActionValue& Value)
{
    if (!DisplayClusterActor) return;
    CurrentMovementInput = DisplayClusterActor->GetActorForwardVector() * -Value.Get<float>();
}

void AFlyPawn::MoveRight(const FInputActionValue& Value)
{
    if (!DisplayClusterActor) return;
    CurrentMovementInput = DisplayClusterActor->GetActorRightVector() * Value.Get<float>();
}

void AFlyPawn::MoveLeft(const FInputActionValue& Value)
{
    if (!DisplayClusterActor) return;
    CurrentMovementInput = DisplayClusterActor->GetActorRightVector() * -Value.Get<float>();
}

void AFlyPawn::MoveUp(const FInputActionValue& Value)
{
    // World-space up — stays level regardless of camera pitch
    CurrentMovementInput = FVector::UpVector * Value.Get<float>();
}

void AFlyPawn::MoveDown(const FInputActionValue& Value)
{
    CurrentMovementInput = FVector::UpVector * -Value.Get<float>();
}

void AFlyPawn::Look(const FInputActionValue& Value)
{
    if (!DisplayClusterActor)
        return;

    FVector2D Axis = Value.Get<FVector2D>();

    FRotator Rot = DisplayClusterActor->GetActorRotation();
    Rot.Yaw   += Axis.X;
    Rot.Pitch  = FMath::Clamp(Rot.Pitch + Axis.Y, -89.f, 89.f);
    DisplayClusterActor->SetActorRotation(Rot);
}

void AFlyPawn::ToggleUI()
{
    if (!LoaderWidget) return;

    bUIVisible = !bUIVisible;
    LoaderWidget->SetVisibility(bUIVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);

    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    if (bUIVisible)
    {
        PC->bShowMouseCursor = true;
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(LoaderWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
    }
    else
    {
        PC->bShowMouseCursor = false;
        PC->SetInputMode(FInputModeGameOnly());
    }
}

USceneComponent* AFlyPawn::FindRigLeftCamera() const
{
    if (!DisplayClusterActor || !DisplayClusterActor->GetRootComponent())
        return nullptr;

    // Match the xform/camera ID defined in the .ndisplay config — same name
    // convention used by PointCloudLoaderWidget::OnSetCameraClicked ("Camera_Left").
    TArray<USceneComponent*> Components;
    DisplayClusterActor->GetRootComponent()->GetChildrenComponents(true, Components);

    for (USceneComponent* Comp : Components)
    {
        if (Comp && Comp->GetName().Contains(TEXT("Camera_Left")))
        {
            return Comp;
        }
    }
    return nullptr;
}

void AFlyPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!DisplayClusterActor)
        return;

    const float MoveSpeed = 2000.f;

    DisplayClusterActor->AddActorWorldOffset(
        CurrentMovementInput * MoveSpeed * DeltaSeconds);

    CurrentMovementInput = FVector::ZeroVector;

    // Sync FlyPawn transform to DisplayClusterActor so the LiDAR LOD manager
    // (via LocalPlayer::CalcSceneView → FlyPawn eye position) uses the correct
    // camera location and direction for distance/ScreenSizeSq calculations.
    SetActorLocationAndRotation(
        DisplayClusterActor->GetActorLocation(),
        DisplayClusterActor->GetActorRotation());

    // Drive the LiDAR LOD off the nDisplay rig's LEFT camera. The LOD manager
    // (FLidarPointCloudViewData::Compute) reads the first local player's LEFT-eye
    // view origin, so snapping the LODCamera (Player0's view) onto the rig's
    // Camera_Left component makes LOD recompute from that exact eye position as
    // the rig flies. Falls back to the rig-origin sync above if not found.
    if (!RigLeftCamera)
    {
        RigLeftCamera = FindRigLeftCamera();
    }
    if (RigLeftCamera && LODCamera)
    {
        LODCamera->SetWorldLocationAndRotation(
            RigLeftCamera->GetComponentLocation(),
            RigLeftCamera->GetComponentRotation());
    }
}