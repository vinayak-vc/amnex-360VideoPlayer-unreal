#include "FlyingGameMode.h"
#include "FlyPawn.h"

AFlyingGameMode::AFlyingGameMode()
{
    DefaultPawnClass = AFlyPawn::StaticClass();
}

void AFlyingGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Warning, TEXT("GameMode BeginPlay"));
}