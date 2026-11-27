#include "DiveHUD.h"
#include "DivePawn.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"

// Real HUD rendering and input routing. Captures are taken after a state has
// been displayed for one second; visual quality still needs human review.
bool ADiveHUD::UISmokeTest(int Phase,bool CaptureOnly)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver||bUIOverflow) return false;
    if(CaptureOnly)
    {
        const TCHAR* names[]={TEXT("Plan_DE"),TEXT("Environment_DE"),TEXT("Loadout_DE"),TEXT("Loadout_EN"),
            TEXT("Plan_EN"),TEXT("Config_DE"),TEXT("Controls_DE"),TEXT("Standard_DE"),TEXT("WarningsMapTutorial_DE"),
            TEXT("Minimal_EN"),TEXT("Compact_EN"),TEXT("PauseWarnings_EN"),TEXT("Resumed_EN"),TEXT("Final_EN")};
        if(Phase<0||Phase>=UE_ARRAY_COUNT(names)) return false;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/
            FString::Printf(TEXT("UI_%s.png"),names[Phase]),true,false);
        return true;
    }
    auto key=[this](FKey code)
    {
        auto* pc=GetOwningPlayerController();
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(code,IE_Pressed,1));
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(code,IE_Released,0));
    };
    auto focus=[&](FName id)
    {
        for(int i=0;i<=ButtonOrder.Num();++i)
        {
            key(EKeys::Tab);
            if(bKeyboardFocus&&KeyboardFocus==id) { key(EKeys::Enter);return true; }
        }
        return false;
    };
    if(Phase==0)
    {
        bUIAudit=true;
        Diver->bEnglish=false;
        return Diver->bMenu&&!Diver->bDiveActive&&ButtonOrder.Contains(TEXT("start"));
    }
    if(Phase==1)
    {
        return focus(TEXT("environment"))&&bEnvironmentPage;
    }
    if(Phase==2)
    {
        // Exercise the same hitbox callback used by pointer input.
        if(!GetHitBoxWithName(TEXT("loadout"))) return false;
        NotifyHitBoxClick(TEXT("loadout"));
        return bLoadoutPage&&!bEnvironmentPage;
    }
    if(Phase==3) { Diver->bEnglish=true;return true; }
    if(Phase==4)
    {
        key(EKeys::Escape);
        return Diver->bMenu&&!bLoadoutPage;
    }
    if(Phase==5)
    {
        Diver->bEnglish=false;
        return OpenConfigInput(TEXT("depth"));
    }
    if(Phase==6)
    {
        if(!SubmitConfigInput(TEXT("23,5"))||FMath::Abs(Diver->PendingConfig.PlannedDepthMetres-23.5)>1.e-6) return false;
        OpenControls();return bControlsPage;
    }
    if(Phase==7)
    {
        if(!focus(TEXT("hud_style"))||Diver->HUDStyle!=1) return false;
        Diver->HUDStyle=2;Diver->SaveSettings();Diver->HUDStyle=0;Diver->LoadSettings();
        if(Diver->HUDStyle!=2) return false;
        key(EKeys::Escape);
        if(bControlsPage||!Diver->bMenu) return false;
        Diver->HUDStyle=0;Diver->StartDive();
        return Diver->bDiveActive&&!Diver->bMenu;
    }
    if(Phase==8)
    {
        // Deliberately synthetic simultaneous alarms stress the available area;
        // they are not evidence for simulation or physiological correctness.
        Diver->Simulation.Data.Warnings=Dive::NoBreathingSupply|Dive::OutOfGas|Dive::EquipmentFailure|
            Dive::CeilingViolation|Dive::HighPO2|Dive::FastAscent|Dive::LowGas|Dive::DepthPlan|
            Dive::TimePlan|Dive::Cold|Dive::NarcosisWarning|Dive::PanicWarning|Dive::DecompressionSymptoms;
        Diver->Simulation.Data.CeilingMetres=3;Diver->Simulation.Data.DecoForecastValid=false;
        Diver->bTutorialActive=true;Diver->Tutorial.Stage=Dive::Lesson::Buoyancy;
        Diver->bShowMap=true;return WarningMessages().Num()==13;
    }
    if(Phase==9) { Diver->HUDStyle=2;Diver->bEnglish=true;return true; }
    if(Phase==10) { Diver->HUDStyle=1;Diver->bShowMap=false;return true; }
    if(Phase==11) { key(EKeys::Escape);return Diver->bMenu; }
    if(Phase==12)
    {
        if(!focus(TEXT("environment"))||!bEnvironmentPage) return false;
        key(EKeys::Escape);
        if(bEnvironmentPage||!Diver->bMenu) return false;
        key(EKeys::Escape);return !Diver->bMenu;
    }
    if(Phase==13)
    {
        Diver->bTutorialActive=false;
        return Diver->bDiveActive&&!Diver->bMenu&&WarningMessages().Num()==13;
    }
    return false;
}
