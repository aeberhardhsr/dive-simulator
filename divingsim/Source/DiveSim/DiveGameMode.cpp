#include "DiveGameMode.h"
#include "DivePawn.h"
#include "DiveHUD.h"

ADiveGameMode::ADiveGameMode()
{
    DefaultPawnClass=ADivePawn::StaticClass();
    HUDClass=ADiveHUD::StaticClass();
}
