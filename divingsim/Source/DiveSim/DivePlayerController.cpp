#include "DivePlayerController.h"
#include "DivePawn.h"
#include "DiveHUD.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool ADivePlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    // Unattended tests inject engine events; real desktop input never steers them.
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&!Params.IsSimulatedInput()) return true;
    auto* diver=Cast<ADivePawn>(GetPawn());
    if(diver&&Params.Event==IE_Pressed)
    {
        if(diver->CapturingAction!=INDEX_NONE&&Params.Key!=EKeys::LeftMouseButton)
        { diver->CaptureKey(Params.Key); return true; }
        if(Params.Key==EKeys::Escape) { diver->ToggleMenu(); return true; }
        if(Params.Key==EKeys::F1)
        {
            diver->SetMenu(true);
            if(auto* hud=Cast<ADiveHUD>(GetHUD())) hud->OpenControls();
            return true;
        }
        if(diver->bMenu)
            if(auto* hud=Cast<ADiveHUD>(GetHUD());hud&&hud->HandleMenuKey(Params.Key,IsInputKeyDown(EKeys::LeftShift)||IsInputKeyDown(EKeys::RightShift))) return true;
        const int action=diver->Controls.Find(Params.Key);
        if(action>=static_cast<int>(EDiveAction::Lamp))
        { diver->PerformAction(static_cast<EDiveAction>(action)); return true; }
    }
    return Super::InputKey(Params);
}
