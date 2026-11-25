#include "DiveGameMode.h"
#include "DivePawn.h"
#include "DiveHUD.h"
#include "DivePlayerController.h"

ADiveGameMode::ADiveGameMode()
{
    DefaultPawnClass=ADivePawn::StaticClass();
    HUDClass=ADiveHUD::StaticClass();
    PlayerControllerClass=ADivePlayerController::StaticClass();
}
