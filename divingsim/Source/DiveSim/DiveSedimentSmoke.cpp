#include "DivePawn.h"
#include "DiveWorld.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ADivePawn::TickSedimentTest(float Dt)
{
    if(!bDiveActive||!Environment) return;
    auto* pc=Cast<APlayerController>(GetController());if(!pc) return;
    const double t=Simulation.Data.ElapsedSeconds;
    auto fail=[](const TCHAR* why)
    { UE_LOG(LogTemp,Error,TEXT("DIVE_SEDIMENT_FAIL %s"),why);FPlatformMisc::RequestExitWithStatus(false,1); };
    auto shot=[&](const TCHAR* label)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Sediment_%d_%s.png"),EntryIndex,label),true,false); };
    FString detail;if(!Environment->ValidateSediment(detail)) { fail(*detail);return; }
    if(SiltSmokePhase==0)
    {
        // Initial test placement above actual surveyed terrain; subsequent
        // stirring uses ordinary input, accepted joints and world collision.
        FVector location=GetActorLocation();location.Z=Environment->GroundZ(location.X,location.Y)+110;
        SetActorLocation(location);VelocityMetres=FVector::ZeroVector;ResetContactPose();
        SiltSmokePhase=1;shot(TEXT("Start"));
    }
    else if(SiltSmokePhase==1&&t>=16)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_SEDIMENT_STIR %s"),*detail);
        if(Environment->SedimentBorn()==0) { fail(TEXT("Actual fin strokes emitted no sediment"));return; }
        pc->SetControlRotation((Environment->SedimentCenter()-PhysicalEyes()).Rotation());
        SiltSmokePhase=2;
    }
    else if(SiltSmokePhase==2&&t>=21)
    {
        FVector2D pixel;const bool projected=pc->ProjectWorldLocationToScreen(Environment->SedimentCenter(),pixel);
        UE_LOG(LogTemp,Display,TEXT("DIVE_SEDIMENT_VIEW projected=%d pixel=%s center=%s eyes=%s gaze=%s head_yaw=%.3f body_yaw=%.3f"),projected,*pixel.ToString(),
            *Environment->SedimentCenter().ToString(),*PhysicalEyes().ToString(),*PhysicalPose.Gaze.ToString(),PhysicalPose.HeadYaw,PhysicalPose.BodyYaw());
        SiltBornAtStop=Environment->SedimentBorn();shot(TEXT("Cloud"));SiltSmokePhase=3;
    }
    else if(SiltSmokePhase==3&&t>=23)
    { SiltPauseDetail=detail;SiltPauseSeconds=t;SiltPauseWall=0;SetMenu(true);if(pc->GetHUD()) pc->GetHUD()->bShowHUD=false;SiltSmokePhase=4; }
    else if(SiltSmokePhase==4)
    {
        const double before=SiltPauseWall;SiltPauseWall+=Dt;
        if(t!=SiltPauseSeconds||detail!=SiltPauseDetail) { fail(TEXT("Pause advanced sediment"));return; }
        if(before<1&&SiltPauseWall>=1) shot(TEXT("Visible"));
        if(before<2&&SiltPauseWall>=2) Environment->HideSedimentForTest(true);
        if(before<3&&SiltPauseWall>=3) shot(TEXT("Hidden"));
        if(SiltPauseWall>=4)
        {
            Environment->HideSedimentForTest(false);
            if(pc->GetHUD()) pc->GetHUD()->bShowHUD=true;
            SetMenu(false);SiltSmokePhase=5;UE_LOG(LogTemp,Display,TEXT("DIVE_SEDIMENT_PAUSE_PASS %s"),*detail);
            if(FParse::Param(FCommandLine::Get(),TEXT("DiveSedimentProbe")))
            { UE_LOG(LogTemp,Display,TEXT("DIVE_SEDIMENT_PROBE_COMPLETE"));FPlatformMisc::RequestExitWithStatus(false,0); }
        }
    }
    else if(SiltSmokePhase==5&&t>=35) { shot(TEXT("Settling"));SiltSmokePhase=6; }
    else if(SiltSmokePhase==6&&t>=67)
    {
        if(Environment->SedimentLive()!=0||Environment->SedimentBorn()>SiltBornAtStop+2)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_SEDIMENT_STOP_DETAIL %s born_at_stop=%d"),*detail,SiltBornAtStop);fail(TEXT("Stopped fins kept emitting or old clouds did not expire"));return; }
        shot(TEXT("Clear"));SiltSmokePhase=7;
        UE_LOG(LogTemp,Display,TEXT("DIVE_SEDIMENT_PASS %s"),*detail);
    }
}
