#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FlyPawn.h"
#include "FlyingGameMode.generated.h"

UCLASS()
class STEREOSCOPICPROJECT_API AFlyingGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AFlyingGameMode();

protected:
    virtual void BeginPlay() override;
};

