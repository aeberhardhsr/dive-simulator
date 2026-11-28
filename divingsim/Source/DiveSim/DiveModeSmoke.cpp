#include "DivePawn.h"
#include "DiveLogbook.h"
#include "DiveWorld.h"
#include "DiveAudio.h"
#include "DiveHUD.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Engine/GameViewportClient.h"
#include "Misc/Paths.h"

void ADivePawn::TickModeTest()
{
    if(ModeSmokePhase>=5)
    {
        const double elapsed=GetWorld()->GetTimeSeconds()-ModeSmokeCapturedAt;
        if(ModeSmokePhase==5&&elapsed>.5)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Mode_%s_Menu.png"),*ModeSmokeCase),true,false);
            ModeSmokePhase=6;
        }
        if(elapsed>2)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_MODE_%s case=%s log=%s"),bModeSmokeValid?TEXT("PASS"):TEXT("FAIL"),*ModeSmokeCase,*LastLogPath);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s modes=1"),bModeSmokeValid?TEXT("PASS"):TEXT("FAIL"));
            FPlatformMisc::RequestExitWithStatus(false,bModeSmokeValid?0:1);
        }
        return;
    }
    if(!bDiveActive) { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL unexpected mode fixture termination"));FPlatformMisc::RequestExitWithStatus(false,1);return; }
    const auto& s=Simulation.Data;const auto rules=Simulation.Settings.Rules();
    auto key=[&](EDiveAction action) { TestKey(Controls.Key(action),true);TestKey(Controls.Key(action),false); };
    auto shot=[&](const TCHAR* stage) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/
        FString::Printf(TEXT("Screenshots/Mode_%s_%s.png"),*ModeSmokeCase,stage),true,false); };
    if(ModeSmokePhase==0&&s.ElapsedSeconds>3)
    {
        bModeSmokeValid&=rules.AutomaticHints?!Notice.IsEmpty():Notice.IsEmpty();
        bShowInstruments=true;key(EDiveAction::HUD);
        bModeSmokeValid&=bShowInstruments==rules.PermanentHUD;
        bShowComputer=true;
        if(ModeSmokeCase==TEXT("exploration")||ModeSmokeCase==TEXT("hypoxia")) key(EDiveAction::Regulator);
        ModeSmokePhase=1;
    }
    if(ModeSmokePhase==1&&s.ElapsedSeconds>6) { shot(TEXT("Baseline"));ModeSmokePhase=2; }
    if(ModeSmokePhase==2&&s.ElapsedSeconds>10)
    { if(!rules.PermanentHUD) key(EDiveAction::HUD); bModeSmokeValid&=bShowInstruments;ModeSmokePhase=3; }
    const bool exploration=Simulation.Settings.GameMode==Dive::Mode::Exploration;
    const bool ready=ModeSmokeCase==TEXT("hardcore")?s.RegulatorFreeflow&&s.FreeflowLostLitres>100:
        ModeSmokeCase==TEXT("hypoxia")?s.VisualImpairment>.4:s.ElapsedSeconds>(exploration?18:65);
    if(ModeSmokePhase==3&&ready)
    {
        if(exploration) bModeSmokeValid&=s.CylinderBar==300&&s.MotorCapacity==1&&s.VisualImpairment==0&&s.NDLMinutes==99;
        else bModeSmokeValid&=s.CylinderBar<300&&s.MotorCapacity<1;
        bModeSmokeValid&=Simulation.Settings.AssistedBuoyancy()==(ModeSmokeCase==TEXT("exploration"));
        bModeSmokeValid&=s.RegulatorFreeflow==(ModeSmokeCase==TEXT("hardcore"));
        if(ModeSmokeCase==TEXT("hardcore")) bModeSmokeValid&=s.FreeflowLocalLPS>0&&Notice.IsEmpty()&&(s.Warnings&Dive::EquipmentFailure);
        if(ModeSmokeCase==TEXT("hypoxia")) bModeSmokeValid&=s.MotorCapacity<.7&&s.Panic>.4&&s.SpO2>60;
        bModeSmokeValid&=FMath::Abs(Camera->PostProcessSettings.VignetteIntensity-(.35+.6*s.VisualImpairment))<.001;
        bModeSmokeValid&=FMath::Abs(Camera->PostProcessSettings.ColorSaturation.W-(1-.8*s.VisualImpairment))<.001;
        FString error;bModeSmokeValid&=Audio->Validate(error);
        shot(TEXT("Effects"));ModeSmokeCapturedAt=s.ElapsedSeconds;ModeSmokePhase=4;
        UE_LOG(LogTemp,Display,TEXT("DIVE_MODE_STATE case=%s seconds=%.3f depth=%.3f gas=%.3f motor=%.5f visual=%.5f panic=%.5f freeflow=%d lost=%.3f vignette=%.5f hints=%d"),
            *ModeSmokeCase,s.ElapsedSeconds,s.DepthMetres,s.CylinderBar,s.MotorCapacity,s.VisualImpairment,s.Panic,s.RegulatorFreeflow,s.FreeflowLostLitres,Camera->PostProcessSettings.VignetteIntensity,rules.AutomaticHints);
    }
    if(ModeSmokePhase==4&&s.ElapsedSeconds>ModeSmokeCapturedAt+3)
    {
        bModeSmokeValid&=Samples.Num()>15&&Environment->TerrainChunkCount()<=9&&s.DistanceMetres>1;
        FinishDive(bModeSmokeValid?TEXT("smoke_pass"):TEXT("smoke_failed"));
        const auto read=DiveLogbook::Read(LastLogPath);
        bModeSmokeValid&=read.Error.IsEmpty()&&read.bImpairmentContext&&read.Samples.Num()==Samples.Num()&&
            !read.Samples.IsEmpty()&&read.Samples.Last().RegulatorFreeflow==s.RegulatorFreeflow;
        auto* pc=Cast<APlayerController>(GetController());auto* hud=pc?Cast<ADiveHUD>(pc->GetHUD()):nullptr;
        if(hud)
        {
            hud->NotifyHitBoxClick(TEXT("new"));
            const auto selected=PendingConfig.GameMode;
            for(int i=0;i<3;++i) hud->NotifyHitBoxClick(TEXT("mode"));
            bModeSmokeValid&=PendingConfig.GameMode==selected;
            if(exploration)
            {
                const bool manual=PendingConfig.ManualExplorationBuoyancy;
                hud->NotifyHitBoxClick(TEXT("exploration_trim"));
                bModeSmokeValid&=PendingConfig.ManualExplorationBuoyancy!=manual;
                hud->NotifyHitBoxClick(TEXT("exploration_trim"));
                bModeSmokeValid&=PendingConfig.ManualExplorationBuoyancy==manual;
            }
        }
        else bModeSmokeValid=false;
        bEnglish=ModeSmokeCase==TEXT("manual")||ModeSmokeCase==TEXT("warm")||ModeSmokeCase==TEXT("hypoxia");
        ModeSmokeCapturedAt=GetWorld()->GetTimeSeconds();ModeSmokePhase=5;
    }
    else if(s.ElapsedSeconds>110) { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL mode condition not reached case=%s"),*ModeSmokeCase);FPlatformMisc::RequestExitWithStatus(false,1); }
}
