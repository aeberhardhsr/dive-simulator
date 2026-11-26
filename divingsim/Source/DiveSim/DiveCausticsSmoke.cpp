#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveEquipment.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"

void ADivePawn::TickCausticsTest(float Dt)
{
    if(!Environment) return;
    auto* pc=Cast<APlayerController>(GetController());if(!pc) return;
    // Diagnostic receivers only; normal gameplay uses ordinary world objects.
    bDiveActive=false;bMenu=false;bLamp=false;
    if(Equipment) Equipment->SetActorHiddenInGame(true);
    if(pc->GetHUD()) pc->GetHUD()->bShowHUD=false;
    CausticsTestClock+=Dt;
    const int phase=FMath::Max(0,FMath::FloorToInt((CausticsTestClock-15)/5));
    if(phase>=4)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_CAUSTICS_PROBE_PASS captures=4"));
        FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    FString fixture=TEXT("day");FParse::Value(FCommandLine::Get(),TEXT("DiveCausticsCase="),fixture);
    const bool calm=fixture==TEXT("calm"),night=fixture==TEXT("night"),air=fixture==TEXT("air"),deep=fixture==TEXT("deep");
    const bool disabled=phase==1;
    const float height=air?400:deep?-1500:-320;
    const FVector center=Environment->Terrain.Entries[0].Position;
    if(phase!=CausticsTestPhase)
    {
        Dive::Config config;config.Hour=night?0:13;config.Date={2026,6,21};config.WindMetresPerSecond=calm?0:6;
        config.VisibilityMetres=14;config.CurrentMetresPerSecond=0;config.Weather=0;
        Simulation.Data.ElapsedSeconds=phase>=2?22:20;
        if(CausticsTestPhase<0) Environment->ConfigureCausticsProbe(config,height);
        Environment->SetCausticsTestStrength(disabled?0:1);
        CausticsTestPhase=phase;
    }
    const FVector eye(center.X-230,center.Y,height+235);
    const FRotator gaze=FRotator(-48,0,0);
    SetActorLocation(eye-FVector(0,0,60));pc->SetControlRotation(gaze);
    Camera->SetWorldLocationAndRotation(eye,gaze);
    FString error;
    if(!Environment->ValidateCaustics(error))
    { UE_LOG(LogTemp,Error,TEXT("DIVE_CAUSTICS_FAIL %s"),*error);FPlatformMisc::RequestExitWithStatus(false,1);return; }
    if(CausticsTestClock>19+phase*5&&CausticsTestCaptured!=phase)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_CAUSTICS_CAPTURE phase=%d wind=%d night=%d air=%d deep=%d strength=%d clock=%.3f camera=%s"),
            phase,calm?0:6,night,air,deep,!disabled,Simulation.Data.ElapsedSeconds,*eye.ToString());
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Caustics_%02d.png"),phase),false,false);
        CausticsTestCaptured=phase;
    }
}
