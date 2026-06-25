#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Camera/CameraComponent.h"
#include "InputActionValue.h"
#include "FlyPawn.generated.h"

class ADisplayClusterRootActor;

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
    void MoveUp(const FInputActionValue& Value);
    void MoveDown(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void ToggleUI();
    virtual void Tick(float DeltaSeconds) override;
    

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
    class UInputAction* MoveUpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* MoveDownAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
    class UInputAction* ToggleUIAction;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="nDisplay")
    ADisplayClusterRootActor* DisplayClusterActor;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="nDisplay")
    ACameraActor* CameraActor;

    // LOD-correction camera: constrained to 16:9 so LocalPlayer::CalcSceneView()
    // returns a projection matrix with correct ScreenSizeFactor regardless of the
    // nDisplay SBS game viewport (3840x1080, aspect ~3.56). Without this, nDisplay's
    // wide viewport inflates ScreenSizeFactor ~4x vs PIE → budget saturates at coarse
    // octree nodes → point cloud renders as large cubes.
    UPROPERTY(VisibleAnywhere, Category = "LOD")
    UCameraComponent* LODCamera;

private:
    // Cached nDisplay rig "Camera_Left" scene component. The LODCamera (Player0's
    // view) is snapped to this every tick so the LiDAR LOD manager — which reads
    // the first local player's LEFT-eye view origin — computes LOD from the rig's
    // left camera position as the FlyPawn flies the rig around.
    UPROPERTY()
    USceneComponent* RigLeftCamera = nullptr;

    USceneComponent* FindRigLeftCamera() const;

    UPROPERTY()
    class UPointCloudLoaderWidget* LoaderWidget;

    FVector CurrentMovementInput;
    bool bUIVisible = false;
};

