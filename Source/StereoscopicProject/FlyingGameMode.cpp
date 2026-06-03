#include "FlyingGameMode.h"
#include "FlyPawn.h"

AFlyingGameMode::AFlyingGameMode()
{
    DefaultPawnClass = AFlyPawn::StaticClass();
}
