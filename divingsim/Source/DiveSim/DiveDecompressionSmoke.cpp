#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveWorld.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"

// Accelerated model/render fixture, not a player-controlled 20-minute dive.
// Normal wrist smoke tests separately exercise movement and all four pages.
void ADivePawn::TickDecompressionTest(float Dt)
{
    ControlsSmokeClock+=Dt;
    if(ControlsSmokePhase>0&&!bMenu)
        Simulation.Step(FMath::Min(double(Dt),.1),Simulation.Data.DepthMetres,0,0,0,0,false);
    auto fail=[&](const TCHAR* why) { UE_LOG(LogTemp,Error,TEXT("DIVE_DECOMPRESSION_FAIL phase=%d %s"),ControlsSmokePhase,why);FPlatformMisc::RequestExitWithStatus(false,1); };
    auto shot=[&](const TCHAR* name) {
        Equipment->Update(1);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/FString::Printf(TEXT("Deco_%s.png"),name),true,false);
        Equipment->ExportDisplayForTest(FPaths::ProjectSavedDir()/TEXT("Screenshots"),FString::Printf(TEXT("DecoDisplay_%s.png"),name));
        const auto& s=Simulation.Data;
        UE_LOG(LogTemp,Display,TEXT("DIVE_DECOMPRESSION_STAGE %s valid=%d depth=%.3f ceiling=%.5f stop=%.3f wait=%.1f tts=%.3f page=%d"),name,s.DecoForecastValid,s.DepthMetres,s.CeilingMetres,s.DecoStopMetres,s.DecoStopSeconds,s.TimeToSurfaceSeconds,ComputerPage);
    };
    auto captured=[&](const TCHAR* name) {
        if(TestLessonStage!=ControlsSmokePhase)
        { shot(name);TestLessonStage=ControlsSmokePhase;TestLessonAge=ControlsSmokeClock;return false; }
        // Screenshot requests capture at end of frame. Keep the displayed
        // language, menu and model state stable until that frame has rendered.
        return ControlsSmokeClock-TestLessonAge>.35;
    };
    auto next=[&]() { ++ControlsSmokePhase;ControlsSmokeClock=0; };
    auto place=[&](double depth) { FVector p=GetActorLocation();p.Z=-depth*100;SetActorLocation(p);VelocityMetres=FVector::ZeroVector;ResetContactPose(); };
    if(ControlsSmokePhase==0)
    {
        if(ControlsSmokeClock<3) return;
        FVector spot=GetActorLocation();bool found=false;
        for(int radius=50;radius<=800&&!found;radius+=50)
            for(int angle=0;angle<360&&!found;angle+=30)
            {
                const FVector candidate=GetActorLocation()+FVector(FMath::Cos(FMath::DegreesToRadians(double(angle)))*radius*100,FMath::Sin(FMath::DegreesToRadians(double(angle)))*radius*100,0);
                float ground=0;
                if(Environment->Terrain.Sample(candidate.X,candidate.Y,ground)&&ground<-4000) { spot=candidate;found=true; }
            }
        if(!found) { fail(TEXT("No open-water fixture position"));return; }
        SetActorLocation(spot);place(30);
        PendingConfig.GameMode=Dive::Mode::Simulation;PendingConfig.StartPressureBar=300;PendingConfig.PlannedDepthMetres=40;PendingConfig.PlannedMinutes=60;
        Simulation.Reset(PendingConfig);Simulation.Step(1200,30,0,0,0,0,false);
        if(!Simulation.Data.DecoForecastValid||Simulation.Data.DecoStopMetres<=0) { fail(TEXT("Exposure did not create a stop"));return; }
        bShowMap=false;bShowInstruments=true;bShowComputer=false;bEnglish=false;Notice.Empty();
        next();
    }
    else if(ControlsSmokePhase==1&&ControlsSmokeClock>3)
    { if(!captured(TEXT("Deep_DE"))) return;bEnglish=true;bShowComputer=true;ComputerPage=3;next(); }
    else if(ControlsSmokePhase==2&&ControlsSmokeClock>3)
    {
        if(Equipment->WristBlend<.95) { fail(TEXT("Wrist did not raise"));return; }
        if(!captured(TEXT("Computer_EN"))) return;
        for(int i=0;i<4;++i) { TestKey(Controls.Key(EDiveAction::ComputerPage),true);TestKey(Controls.Key(EDiveAction::ComputerPage),false); }
        if(ComputerPage!=3) { fail(TEXT("Four-page input cycle failed"));return; }
        TestBreathingGas=Simulation.Data.ElapsedSeconds;TestBreathingSpO2=Simulation.Data.DecoStopSeconds;
        SetMenu(true);next();
    }
    else if(ControlsSmokePhase==3&&ControlsSmokeClock>2)
    {
        if(Simulation.Data.ElapsedSeconds!=TestBreathingGas||Simulation.Data.DecoStopSeconds!=TestBreathingSpO2) { fail(TEXT("Pause advanced the forecast"));return; }
        UE_LOG(LogTemp,Display,TEXT("DIVE_DECOMPRESSION_PAUSE_PASS"));
        SetMenu(false);
        const double target=Simulation.Data.DecoStopMetres;
        double depth=Simulation.Data.DepthMetres;
        while(depth>target+1e-8) { const double step=FMath::Min(.1,(depth-target)/.15);depth-=step*.15;Simulation.Step(step,depth,0,0,.15,0,false); }
        Simulation.Step(1,target,0,0,0,0,false);place(target);
        if(!Simulation.Data.DecoForecastValid||Simulation.Data.DecoStopSeconds<=30) { fail(TEXT("No remaining hold at projected stop"));return; }
        bEnglish=false;bShowComputer=false;next();
    }
    else if(ControlsSmokePhase==4&&ControlsSmokeClock>3)
    {
        if(!captured(TEXT("Stop_DE"))) return;const double before=Simulation.Data.DecoStopSeconds;
        Simulation.Step(30,Simulation.Data.DepthMetres,0,0,0,0,false);
        if(Simulation.Data.DecoStopSeconds>=before-25) { fail(TEXT("Holding did not discharge the stop"));return; }
        UE_LOG(LogTemp,Display,TEXT("DIVE_DECOMPRESSION_HOLD_PASS before=%.1f after=%.1f"),before,Simulation.Data.DecoStopSeconds);
        bEnglish=true;bShowComputer=true;next();
    }
    else if(ControlsSmokePhase==5&&ControlsSmokeClock>3)
    {
        if(!captured(TEXT("Progress_EN"))) return;
        for(int i=0;i<10000&&Simulation.Data.DecoStopMetres>0;++i) Simulation.Step(1,Simulation.Data.DecoStopMetres,0,0,0,0,false);
        if(!Simulation.Data.DecoForecastValid||Simulation.Data.DecoStopMetres!=0||Simulation.Data.TimeToSurfaceSeconds>21) { fail(TEXT("Stop did not clear"));return; }
        next();
    }
    else if(ControlsSmokePhase==6&&ControlsSmokeClock>3)
    {
        if(!captured(TEXT("Clear_EN"))) return;Simulation.Reset(PendingConfig);Simulation.Step(1200,30,0,0,0,0,false);
        Simulation.Step(1,0,0,0,0,0,false);place(0);bEnglish=false;bShowComputer=false;
        if(Simulation.Data.DecoForecastValid||!(Simulation.Data.Warnings&Dive::CeilingViolation)) { fail(TEXT("Violation produced an apparently valid forecast"));return; }
        next();
    }
    else if(ControlsSmokePhase==7&&ControlsSmokeClock>3)
    { if(!captured(TEXT("Violation_DE"))) return;next(); }
    else if(ControlsSmokePhase==8&&ControlsSmokeClock>2)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_DECOMPRESSION_PASS stages=6 pause=1 hold=1 input_cycle=4"));
        UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS decompression=1"));
        FPlatformMisc::RequestExitWithStatus(false,0);return;
    }
    AdvanceContactPose(FMath::Min(Dt,.1f),0);
    if(Equipment) Equipment->Update(1);
    if(ControlsSmokeClock>15) fail(TEXT("Fixture phase timed out"));
}
