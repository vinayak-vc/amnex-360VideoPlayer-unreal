#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "FlyPawn.generated.h"

UCLASS()
class STEREOSCOPICPROJECT_API AFlyPawn : public APawn
{
    GENERATED_BODY()

public:
    AFlyPawn();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class UPointCloudLoaderWidget> PointCloudWidgetClass;

protected:
    virtual void BeginPlay() override;
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    void MoveForward(const FInputActionValue& Value);
    void MoveBack(const FInputActionValue& Value);
    void MoveRight(const FInputActionValue& Value);
    void MoveLeft(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void ToggleUI();

    // Mapping context
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputMappingContext* DefaultMappingContext;

    // Individual move actions — reuses existing IA_MoveForward/Back/Right/Left
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveForwardAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveBackAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveRightAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveLeftAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* ToggleUIAction;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
    class UCameraComponent* CameraComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
    class UFloatingPawnMovement* FloatingMovement;

private:
    UPROPERTY()
    class UPointCloudLoaderWidget* LoaderWidget;

    bool bUIVisible = false;
};
