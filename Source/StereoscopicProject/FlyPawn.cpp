#include "FlyPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Blueprint/UserWidget.h"
#include "PointCloudLoaderWidget.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AFlyPawn::AFlyPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    AutoPossessPlayer = EAutoReceiveInput::Player0;

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
    RootComponent = Root;

    CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
    CameraComponent->SetupAttachment(RootComponent);
    CameraComponent->bUsePawnControlRotation = true;

    FloatingMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingMovement"));
    FloatingMovement->UpdatedComponent = RootComponent;
    FloatingMovement->MaxSpeed = 2000.f;
    FloatingMovement->Acceleration = 5000.f;
    FloatingMovement->Deceleration = 5000.f;
}

void AFlyPawn::BeginPlay()
{
    Super::BeginPlay();

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
        if (LookAction)
            EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFlyPawn::Look);
        if (ToggleUIAction)
            EIC->BindAction(ToggleUIAction, ETriggerEvent::Started, this, &AFlyPawn::ToggleUI);
    }
}

void AFlyPawn::MoveForward(const FInputActionValue& Value)
{
    if (FloatingMovement)
        FloatingMovement->AddInputVector(GetActorForwardVector() * Value.Get<float>());
}

void AFlyPawn::MoveBack(const FInputActionValue& Value)
{
    if (FloatingMovement)
        FloatingMovement->AddInputVector(GetActorForwardVector() * -Value.Get<float>());
}

void AFlyPawn::MoveRight(const FInputActionValue& Value)
{
    if (FloatingMovement)
        FloatingMovement->AddInputVector(GetActorRightVector() * Value.Get<float>());
}

void AFlyPawn::MoveLeft(const FInputActionValue& Value)
{
    if (FloatingMovement)
        FloatingMovement->AddInputVector(GetActorRightVector() * -Value.Get<float>());
}

void AFlyPawn::Look(const FInputActionValue& Value)
{
    const FVector2D Axis = Value.Get<FVector2D>();
    AddControllerYawInput(Axis.X);
    AddControllerPitchInput(Axis.Y);
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
