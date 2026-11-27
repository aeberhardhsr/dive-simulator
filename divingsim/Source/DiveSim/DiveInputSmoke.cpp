#include "DivePawn.h"
#include "DiveHUD.h"
#include "DiveWorld.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInstanceDynamic.h"

void ADivePawn::TestKey(FKey Key,bool Down)
{
    auto* pc=Cast<APlayerController>(GetController());
    if(!pc) return;
    if(Down==TestHeldKeys.Contains(Key))
    {
        const int action=Controls.Find(Key);
        const bool continuous=action>=0&&action<=static_cast<int>(EDiveAction::Deflate);
        // Toggle actions are consumed by our controller before PlayerInput and
        // intentionally have no held-key state. Never repeat their press edge.
        if(!Down||!continuous||pc->IsInputKeyDown(Key)) return;
        // A desktop focus change can flush engine key state while the smoke
        // driver's intended hold remains active. Restore that synthetic hold.
        UE_LOG(LogTemp,Display,TEXT("DIVE_TEST_KEY_RESTORED key=%s"),*Key.ToString());
    }
    pc->InputKey(FInputKeyEventArgs::CreateSimulated(Key,Down?IE_Pressed:IE_Released,Down?1:0));
    if(Down) TestHeldKeys.Add(Key); else TestHeldKeys.Remove(Key);
}
void ADivePawn::InputSmokeTick(float Dt)
{
    ControlsSmokeClock+=Dt;
    auto* pc=Cast<APlayerController>(GetController());
    auto* hud=pc?Cast<ADiveHUD>(pc->GetHUD()):nullptr;
    if(!pc||!hud) return;
    if(SmokeScenario==TEXT("ui"))
    {
        if(ControlsSmokePhase<28&&ControlsSmokeClock>ControlsSmokePhase+2)
        {
            const bool ok=hud->UISmokeTest(ControlsSmokePhase/2,ControlsSmokePhase%2!=0);
            UE_LOG(LogTemp,Display,TEXT("DIVE_UI_PHASE %d %s"),ControlsSmokePhase,ok?TEXT("PASS"):TEXT("FAIL"));
            if(!ok) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
            ++ControlsSmokePhase;
        }
        if(ControlsSmokePhase==28&&ControlsSmokeClock>31)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS ui=1 phases=14 visual_review_required=1"));
            FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
    if(SmokeScenario==TEXT("coast"))
    {
        auto fail=[&](const TCHAR* reason){UE_LOG(LogTemp,Error,TEXT("DIVE_COAST_ROUTE_FAIL phase=%d %s position=%s forward_down=%d intended_hold=%d"),ControlsSmokePhase,reason,*GetActorLocation().ToString(),pc->IsInputKeyDown(Controls.Key(EDiveAction::Forward)),TestHeldKeys.Contains(Controls.Key(EDiveAction::Forward)));FPlatformMisc::RequestExitWithStatus(false,1);};
        auto shot=[&](const TCHAR* name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/FString::Printf(TEXT("Coast_%d_%s.png"),EntryIndex,name),true,false);};
        auto next=[&](){++ControlsSmokePhase;ControlsSmokeClock=0;};
        const auto survey=Environment->Terrain.ToSurvey(GetActorLocation());float ground=0,measured=0;
        const FVector2D starts[]={{2726078.5,1221674.5},{2734890.5,1219852.5},{2741108.5,1220619.5}};
        const FVector2D targets[]={{2726050.5,1221674.5},{2734890.5,1219828.5},{2741130.5,1220619.5}};
        if(EntryIndex<0||EntryIndex>2) { fail(TEXT("Invalid route"));return; }
        FVector start=Environment->Terrain.ToWorld(starts[EntryIndex].X,starts[EntryIndex].Y);start.Z=-180;
        const FVector target=Environment->Terrain.ToWorld(targets[EntryIndex].X,targets[EntryIndex].Y);
        const FVector direction=(target-start).GetSafeNormal();const double yaw=direction.Rotation().Yaw;
        const double progress=FVector::DotProduct(GetActorLocation()-start,direction)/100;
        if(ControlsSmokeClock>40) { fail(TEXT("Movement timed out"));return; }
        if(ControlsSmokePhase==0&&ControlsSmokeClock>2)
        {
            if(!Environment->Terrain.Sample(start.X,start.Y,measured)) { fail(TEXT("Start lacks original bathymetry"));return; }
            SetActorLocation(start);VelocityMetres=FVector::ZeroVector;pc->SetControlRotation(FRotator(-12,yaw,0));
            next();return;
        }
        if(ControlsSmokePhase==1&&ControlsSmokeClock>1)
        {
            shot(TEXT("Start"));next();return;
        }
        if(ControlsSmokePhase==2&&ControlsSmokeClock>2&&TestLessonStage<1)
        {
            pc->FlushPressedKeys();TestLessonStage=1;
            UE_LOG(LogTemp,Display,TEXT("DIVE_TEST_KEYS_FLUSHED coast=1"));
        }
        if(ControlsSmokePhase==2&&ControlsSmokeClock>.5)
        { pc->SetControlRotation(FRotator(0,yaw,0));TestKey(Controls.Key(EDiveAction::Forward),true);TestKey(Controls.Key(EDiveAction::Sprint),true); }
        if(ControlsSmokePhase==2&&progress>=FVector::Dist2D(start,target)/100)
        {
            const FVector p=GetActorLocation();FHitResult hit;FCollisionQueryParams query(SCENE_QUERY_STAT(CoastRoute),true,this);
            if(Environment->Terrain.Sample(p.X,p.Y,measured)||!Environment->Terrain.SampleGround(p.X,p.Y,ground)||
                !GetWorld()->LineTraceSingleByChannel(hit,FVector(p.X,p.Y,100),FVector(p.X,p.Y,ground-100),ECC_Visibility,query)||
                FMath::Abs(hit.ImpactPoint.Z-ground)>2) { fail(TEXT("Modeled bottom or world collision missing"));return; }
            UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_ROUTE_CROSSED entry=%d east=%.3f north=%.3f depth=%.3f ground=%.3f"),EntryIndex,survey.X,survey.Y,-p.Z/100,ground/100);
            // Exercise the real sediment source with a controlled fin-position
            // probe on the modeled bottom; this is separate from the swim input.
            const int born=Environment->SedimentBorn();
            for(int i=0;i<20;++i) Environment->DisturbSediment(0,FVector(p.X,p.Y,ground+12),3,.1);
            Environment->StepSediment(.1);
            if(Environment->SedimentBorn()<=born||Environment->SedimentLive()==0) { fail(TEXT("Modeled bottom lost sediment source or plume"));return; }
            UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_SEDIMENT_PASS entry=%d born=%d live=%d"),EntryIndex,Environment->SedimentBorn()-born,Environment->SedimentLive());
            TestKey(Controls.Key(EDiveAction::Forward),false);TestKey(Controls.Key(EDiveAction::Sprint),false);
            pc->SetControlRotation(FRotator(-12,yaw,0));next();return;
        }
        if(ControlsSmokePhase==3&&ControlsSmokeClock>2)
        {
            shot(TEXT("Crossed"));next();return;
        }
        if(ControlsSmokePhase==4&&ControlsSmokeClock>.5)
        { pc->SetControlRotation(FRotator(0,yaw+180,0));TestKey(Controls.Key(EDiveAction::Forward),true);TestKey(Controls.Key(EDiveAction::Sprint),true); }
        if(ControlsSmokePhase==4&&progress<=3)
        {
            if(!Environment->Terrain.Sample(GetActorLocation().X,GetActorLocation().Y,measured)||Simulation.Data.DistanceMetres<25)
            { fail(TEXT("Return to surveyed bottom failed"));return; }
            TestKey(Controls.Key(EDiveAction::Forward),false);TestKey(Controls.Key(EDiveAction::Sprint),false);
            pc->SetControlRotation(FRotator(-12,yaw,0));next();return;
        }
        if(ControlsSmokePhase==5&&ControlsSmokeClock>1)
        { shot(TEXT("Returned"));next();return; }
        if(ControlsSmokePhase==6&&ControlsSmokeClock>2)
        {
            if(TestLessonStage!=1) { fail(TEXT("Input reset probe was not exercised"));return; }
            UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_ROUTE_PASS entry=%d crossed_unmeasured=1 returned_measured=1 distance_m=%.3f"),EntryIndex,Simulation.Data.DistanceMetres);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS coast=1"));FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
    if(SmokeScenario==TEXT("discoveries"))
    {
        if(ControlsSmokePhase<28&&ControlsSmokeClock>ControlsSmokePhase+2.5)
        {
            const bool ok=hud->DiscoverySmokeTest(ControlsSmokePhase);
            UE_LOG(LogTemp,Display,TEXT("DIVE_DISCOVERY_PHASE %d %s"),ControlsSmokePhase,ok?TEXT("PASS"):TEXT("FAIL"));
            if(!ok) { UE_LOG(LogTemp,Error,TEXT("DIVE_DISCOVERY_FAIL phase=%d"),ControlsSmokePhase);FPlatformMisc::RequestExitWithStatus(false,1);return; }
            ++ControlsSmokePhase;
        }
        if(ControlsSmokePhase==14&&ControlsSmokeClock>37)
        { UE_LOG(LogTemp,Display,TEXT("DIVE_DISCOVERY_PASS phases=14 entry=%d"),EntryIndex);UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS discoveries=1"));FPlatformMisc::RequestExitWithStatus(false,0); }
        return;
    }
    auto tap=[pc](FKey Key)
    {
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1));
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0));
    };
    auto shot=[this](const FString& Name)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(Name+TEXT(".png")),true,false); };
    if(SmokeScenario==TEXT("waypoints"))
    {
        FString restore;
        if(FParse::Value(FCommandLine::Get(),TEXT("DiveWaypointVerify="),restore))
        {
            if(ControlsSmokePhase==0&&ControlsSmokeClock>2)
            {
                const bool ok=hud->WaypointSmokeTest(0);
                if(!ok) { UE_LOG(LogTemp,Error,TEXT("DIVE_WAYPOINT_FAIL restart"));FPlatformMisc::RequestExitWithStatus(false,1);return; }
                ++ControlsSmokePhase;
            }
            if(ControlsSmokePhase==1&&ControlsSmokeClock>5)
            { UE_LOG(LogTemp,Display,TEXT("DIVE_WAYPOINT_PASS restart=1"));UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS waypoints=1"));FPlatformMisc::RequestExitWithStatus(false,0); }
            return;
        }
        if(ControlsSmokePhase<21&&ControlsSmokeClock>(ControlsSmokePhase+1)*2)
        {
            const bool ok=hud->WaypointSmokeTest(ControlsSmokePhase);
            UE_LOG(LogTemp,Display,TEXT("DIVE_WAYPOINT_PHASE %d %s"),ControlsSmokePhase,ok?TEXT("PASS"):TEXT("FAIL"));
            if(!ok) { UE_LOG(LogTemp,Error,TEXT("DIVE_WAYPOINT_FAIL phase=%d"),ControlsSmokePhase);FPlatformMisc::RequestExitWithStatus(false,1);return; }
            ++ControlsSmokePhase;
        }
        if(ControlsSmokePhase==21&&ControlsSmokeClock>44)
        { UE_LOG(LogTemp,Display,TEXT("DIVE_WAYPOINT_PASS phases=21 entry=%d"),EntryIndex);UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS waypoints=1"));FPlatformMisc::RequestExitWithStatus(false,0); }
        return;
    }
    if(SmokeScenario==TEXT("navigation"))
    {
        if(ControlsSmokePhase<=12&&ControlsSmokeClock>(ControlsSmokePhase+1)*1.5)
        {
            const bool ok=hud->NavigationSmokeTest(ControlsSmokePhase);
            UE_LOG(LogTemp,Display,TEXT("DIVE_NAV_PHASE %d %s"),ControlsSmokePhase,ok?TEXT("PASS"):TEXT("FAIL"));
            if(!ok) { UE_LOG(LogTemp,Error,TEXT("DIVE_NAV_FAIL phase=%d"),ControlsSmokePhase);FPlatformMisc::RequestExitWithStatus(false,1);return; }
            ++ControlsSmokePhase;
        }
        if(ControlsSmokePhase==13&&ControlsSmokeClock>21.5)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_NAV_PASS entry=%d shore=%d selection=1 hitboxes=1 persistence=1 cancel=1 active_guard=1 pause=1"),EntryIndex,PendingConfig.StartOnShore);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS navigation=1"));FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
    if(SmokeScenario==TEXT("avatar"))
    {
        TestLessonAge+=Dt;
        if(ControlsSmokePhase<25&&TestLessonAge>(ControlsSmokePhase==0?1:2))
        {
            // Full-body contacts can legitimately prevent a turn near the bed.
            // Swim upward with ordinary thrust until the requested gaze fits,
            // and give each screenshot its own interval after that transition.
            const float requested=ControlsSmokePhase==16?-65:85;
            if((ControlsSmokePhase==16||ControlsSmokePhase==18)&&
                FMath::Abs(FMath::FindDeltaAngleDegrees(PhysicalPose.Gaze.Pitch,static_cast<double>(requested)))>.5&&TestLessonAge<15) return;
            const bool ok=hud->AvatarSmokeTest(ControlsSmokePhase);
            bControlsSmokeValid&=ok;
            UE_LOG(LogTemp,Display,TEXT("DIVE_AVATAR_PHASE %d %s"),ControlsSmokePhase,ok?TEXT("PASS"):TEXT("FAIL"));
            ++ControlsSmokePhase;TestLessonAge=0;
        }
        if(ControlsSmokePhase==25&&TestLessonAge>2)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s avatar=1 case=%d"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"),LoadoutTestCase);
            FPlatformMisc::RequestExitWithStatus(false,bControlsSmokeValid?0:1);
        }
        return;
    }
    if(SmokeScenario==TEXT("loadout"))
    {
        auto phase=[&](int value)
        {
            const bool ok=hud->LoadoutSmokeTest(value);bControlsSmokeValid&=ok;
            UE_LOG(LogTemp,Display,TEXT("DIVE_LOADOUT_PHASE %d %s"),value,ok?TEXT("PASS"):TEXT("FAIL"));
            ++ControlsSmokePhase;
        };
        if(ControlsSmokePhase==0&&ControlsSmokeClock>1) phase(0);
        if(ControlsSmokePhase==1&&ControlsSmokeClock>3) phase(1);
        if(ControlsSmokePhase==2&&ControlsSmokeClock>5) phase(2);
        if(ControlsSmokePhase==3&&Simulation.Data.ElapsedSeconds>10) { SetMenu(true);phase(3); }
        if(ControlsSmokePhase==4&&ControlsSmokeClock>18) { ResumeDive();++ControlsSmokePhase; }
        if(ControlsSmokePhase==5&&ControlsSmokeClock>20)
        {
            bControlsSmokeValid&=bLoadoutImpulseChecked&&bLoadoutImpulseValid&&Simulation.Data.DistanceMetres>2;
            FinishDive(bControlsSmokeValid?TEXT("smoke_pass"):TEXT("smoke_failed"));
            const auto saved=DiveLogbook::Read(LastLogPath);
            bControlsSmokeValid&=saved.Error.IsEmpty()&&saved.bLoadoutContext&&saved.Samples.Num()==Samples.Num();
            UE_LOG(LogTemp,Display,TEXT("DIVE_LOADOUT_%s case=%d mass=%.6f volume=%.6f log=%s"),
                bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"),LoadoutTestCase,Simulation.Data.TotalMassKg,Simulation.Data.DisplacedLitres,*LastLogPath);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s loadout=1"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"));
            FPlatformMisc::RequestExitWithStatus(false,bControlsSmokeValid?0:1);
        }
        return;
    }
    if(SmokeScenario==TEXT("reserve-loss"))
    {
        const auto& state=Simulation.Data; const double t=state.ElapsedSeconds;
        if(ControlsSmokePhase==0&&t>4)
        {
            TestBreathingGas=state.CylinderBar;
            tap(Controls.Key(EDiveAction::Regulator)); ControlsSmokePhase=1;
        }
        if(ControlsSmokePhase==1&&t>8)
        {
            bControlsSmokeValid&=state.BreathSource==Dive::BreathingSource::None&&state.SpO2>97.99&&state.BreathReserve>.8&&state.BreathReserve<.99;
            shot(TEXT("Reserve_Short")); ControlsSmokePhase=2;
        }
        if(ControlsSmokePhase==2&&t>80)
        {
            bControlsSmokeValid&=state.BreathReserve<.25&&state.SpO2<98&&bDiveActive;
            shot(TEXT("Reserve_Low")); ControlsSmokePhase=3;
        }
        if(ControlsSmokePhase==3&&bSummary)
        {
            FString json; FDiveLog log; log.Path=LastLogPath;
            const bool saved=FFileHelper::LoadFileToString(json,*LastLogPath)&&DiveLogbook::Parse(json,log)&&
                log.Status==TEXT("no_breathing_supply")&&log.bBreathReserveContext&&log.Samples.Num()>60;
            bControlsSmokeValid&=saved&&Outcome==TEXT("no_breathing_supply")&&!bDiveActive&&state.SpO2<60&&
                state.BreathReserve==0&&FMath::Abs(state.CylinderBar-TestBreathingGas)<1.e-8;
            shot(TEXT("Reserve_Summary")); ControlsSmokePhase=4; TestLessonAge=0;
            UE_LOG(LogTemp,Display,TEXT("DIVE_RESERVE_%s seconds=%.2f reserve=%.4f spo2=%.3f gas=%.4f samples=%d"),
                bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"),t,state.BreathReserve,state.SpO2,state.CylinderBar,log.Samples.Num());
        }
        if(ControlsSmokePhase==4)
        {
            TestLessonAge+=Dt;
            if(TestLessonAge>2)
            {
                UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s reserve_loss=1"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"));
                if(bControlsSmokeValid)
                {
                    hud->OpenLogbook(LastLogPath,true);
                    SmokeLogbookStarted=GetWorld()->GetTimeSeconds(); ControlsSmokePhase=5;
                }
                else FPlatformMisc::RequestExitWithStatus(false,1);
            }
        }
        if(ControlsSmokeClock>165) { UE_LOG(LogTemp,Error,TEXT("DIVE_RESERVE_FAIL timeout")); FPlatformMisc::RequestExitWithStatus(false,1); }
        return;
    }
    if(SmokeScenario==TEXT("breathing"))
    {
        const auto& state=Simulation.Data; const double t=state.ElapsedSeconds;
        if(ControlsSmokePhase==0&&t>4)
        {
            bControlsSmokeValid&=state.BreathSource==Dive::BreathingSource::Regulator&&state.CylinderBar<PendingConfig.StartPressureBar;
            TestBreathingGas=state.CylinderBar; tap(Controls.Key(EDiveAction::Regulator)); ControlsSmokePhase=1;
        }
        if(ControlsSmokePhase==1&&t>6.5&&!bSmokeSecondaryCaptured)
        { shot(TEXT("Breathing_NoSupply")); bSmokeSecondaryCaptured=true; }
        if(ControlsSmokePhase==1&&t>7)
        {
            bControlsSmokeValid&=state.BreathSource==Dive::BreathingSource::None&&state.SpO2>97.99&&state.BreathReserve<.99&&
                (state.Warnings&Dive::NoBreathingSupply)&&FMath::Abs(state.CylinderBar-TestBreathingGas)<1.e-8;
            TestBreathingSpO2=state.SpO2;
            TestBreathingReserve=state.BreathReserve;
            tap(Controls.Key(EDiveAction::Regulator)); ControlsSmokePhase=2;
        }
        if(ControlsSmokePhase==2&&t>22&&!state.MouthSubmerged)
        {
            bControlsSmokeValid&=state.BreathSource==Dive::BreathingSource::Regulator&&state.SpO2>=TestBreathingSpO2&&state.BreathReserve>TestBreathingReserve;
            TestBreathingGas=state.CylinderBar; tap(Controls.Key(EDiveAction::Regulator)); ControlsSmokePhase=3;
        }
        if(ControlsSmokePhase==3&&state.MouthSubmerged)
        {
            // Waves can wet the mouth again after the regulator is removed.
            // Assert that transition too; a fixed timestamp cannot assume air.
            bControlsSmokeValid&=state.BreathSource==Dive::BreathingSource::None&&
                (state.Warnings&Dive::NoBreathingSupply)&&FMath::Abs(state.CylinderBar-TestBreathingGas)<1.e-8;
        }
        if(ControlsSmokePhase==3&&t>25&&!state.MouthSubmerged)
        {
            bControlsSmokeValid&=state.BreathSource==Dive::BreathingSource::Air&&
                !(state.Warnings&Dive::NoBreathingSupply)&&state.SpO2>97.99&&FMath::Abs(state.CylinderBar-TestBreathingGas)<1.e-8;
            shot(TEXT("Breathing_Air")); ControlsSmokePhase=4;
            UE_LOG(LogTemp,Display,TEXT("DIVE_BREATHING_%s phases=4 gas=%.4f spo2=%.3f immersion=%.3f reserve=%.4f"),
                bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"),state.CylinderBar,state.SpO2,state.BodyImmersion,state.BreathReserve);
        }
        if(t>29&&ControlsSmokePhase!=4) bControlsSmokeValid=false;
        return;
    }
    if(SmokeScenario==TEXT("shore"))
    {
        TestLessonAge+=Dt;
        const double distance=FVector::Dist2D(GetActorLocation(),EntryLocation)/100;
        auto next=[&](int phase,const TCHAR* image)
        {
            shot(FString::Printf(TEXT("%s_%d"),image,EntryIndex));
            UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_PHASE %d walking=%d depth=%.2f distance=%.2f position=%s"),phase,bWalking,Simulation.Data.DepthMetres,distance,*GetActorLocation().ToString());
            ControlsSmokePhase=phase; TestLessonAge=0;
        };
        if(ControlsSmokePhase==0&&TestLessonAge>1)
        {
            bControlsSmokeValid=bWalking&&!CanFinishDive()&&FMath::Abs(Camera->GetComponentLocation().Z-GetActorLocation().Z-80)<1;
            // Surface preparation transfers eight local litres into the BCD.
            // Ambient-air breathing must leave that prepared pressure unchanged.
            const double preparedPressure=PendingConfig.StartPressureBar-
                (PendingConfig.Rules().LimitedGas?FMath::Min(PendingConfig.StartPressureBar,
                    8*PendingConfig.SurfacePressureBar/PendingConfig.CylinderLitres):0);
            bControlsSmokeValid&=Simulation.Data.BreathSource==Dive::BreathingSource::Air&&
                FMath::Abs(Simulation.Data.CylinderBar-preparedPressure)<1.e-8;
            FString shoreError;
            bControlsSmokeValid&=Environment->ValidateShoreline(shoreError);
            if(!shoreError.IsEmpty()) UE_LOG(LogTemp,Error,TEXT("DIVE_SHORE_FAIL %s"),*shoreError);
            tap(Controls.Key(EDiveAction::Finish));
            bControlsSmokeValid&=bDiveActive;
            hud->NotifyHitBoxClick(TEXT("shore"));
            bControlsSmokeValid&=PendingConfig.StartOnShore; // Active plan is locked.
            SaveSettings(); PendingConfig.StartOnShore=false; LoadSettings();
            bControlsSmokeValid&=PendingConfig.StartOnShore;
            next(1,TEXT("ShoreStart"));
        }
        if(ControlsSmokePhase==1&&TestLessonAge>.2&&!bRegulatorInMouth) tap(Controls.Key(EDiveAction::Regulator));
        if(ControlsSmokePhase==1&&!bWalking&&Simulation.Data.DepthMetres>.2) next(2,TEXT("ShoreWater"));
        if(ControlsSmokePhase==2&&TestLessonAge>7&&Simulation.Data.MaxDepthMetres>3)
        {
            const auto& entry=Environment->Terrain.ShoreEntries[EntryIndex];
            pc->SetControlRotation(FRotator(0,entry.Yaw+180,0));
            TestLessonStage=0;
            next(3,TEXT("ShoreDive"));
        }
        if(ControlsSmokePhase==3&&TestLessonStage==0&&Simulation.Data.DepthMetres<.05)
        {
            // Surface away from the staircase before approaching its ramp.
            // A diagonal shortcut from depth can put the hands below the deck;
            // the full body collider must block that underside, not ignore it.
            TestLessonStage=1;shot(FString::Printf(TEXT("ShoreSurface_%d"),EntryIndex));
            UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_SURFACE entry=%d depth=%.4f distance=%.2f"),EntryIndex,Simulation.Data.DepthMetres,distance);
        }
        if(ControlsSmokePhase==3&&bWalking&&distance<.6)
        {
            bControlsSmokeValid&=CanFinishDive()&&bEnteredWater&&FMath::Abs(GetActorLocation().Z-EntryLocation.Z)<1&&
                PhysicalPose.Walking&&PhysicalPose.Swim<.05;
            tap(Controls.Key(EDiveAction::Regulator));
            next(4,TEXT("ShoreReturn"));
        }
        TestKey(Controls.Key(EDiveAction::Forward),ControlsSmokePhase==1||ControlsSmokePhase==2||(ControlsSmokePhase==3&&TestLessonStage==1));
        TestKey(Controls.Key(EDiveAction::Descend),ControlsSmokePhase==2);
        TestKey(Controls.Key(EDiveAction::Ascend),ControlsSmokePhase==3&&!bWalking);
        if(ControlsSmokePhase==4&&TestLessonAge>.8)
        {
            bControlsSmokeValid&=Simulation.Data.BreathSource==Dive::BreathingSource::Air;
            tap(Controls.Key(EDiveAction::Finish));
            bControlsSmokeValid&=!bDiveActive&&bSummary;
            next(5,TEXT("ShoreComplete"));
        }
        if(ControlsSmokePhase==5&&TestLessonAge>2)
        {
            FString json; FDiveLog log; log.Path=LastLogPath;
            const bool saved=FFileHelper::LoadFileToString(json,*LastLogPath)&&DiveLogbook::Parse(json,log)&&log.Status==TEXT("completed")&&log.Samples.Num()>=20;
            bControlsSmokeValid&=saved&&bSmokeChunkLimit&&Simulation.Data.MaxDepthMetres>3&&Simulation.Data.DistanceMetres>20;
            hud->NotifyHitBoxClick(TEXT("new"));
            hud->NotifyHitBoxClick(TEXT("shore")); bControlsSmokeValid&=!PendingConfig.StartOnShore;
            hud->NotifyHitBoxClick(TEXT("shore")); bControlsSmokeValid&=PendingConfig.StartOnShore;
            UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_%s entry=%d samples=%d depth=%.2f distance=%.2f"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"),EntryIndex,log.Samples.Num(),Simulation.Data.MaxDepthMetres,Simulation.Data.DistanceMetres);
            next(6,TEXT("ShoreMenu"));
        }
        if(ControlsSmokePhase==6&&TestLessonAge>1)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s shore=1"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"));
            FPlatformMisc::RequestExitWithStatus(false,bControlsSmokeValid?0:1);
        }
        if(ControlsSmokeClock>120)
        {
            UE_LOG(LogTemp,Error,TEXT("DIVE_SHORE_FAIL timeout phase=%d walking=%d depth=%.2f distance=%.2f position=%s"),ControlsSmokePhase,bWalking,Simulation.Data.DepthMetres,distance,*GetActorLocation().ToString());
            FPlatformMisc::RequestExitWithStatus(false,1);
        }
        return;
    }
    if(SmokeScenario==TEXT("environment"))
    {
        if(ControlsSmokePhase<8&&ControlsSmokeClock>(ControlsSmokePhase+1)*3)
        {
            if(ControlsSmokePhase==7)
            {
                const double visibility=UnderwaterMaterial?UnderwaterMaterial->K2_GetScalarParameterValue(TEXT("VisibilityCm")):0;
                const double extinction=TorchFunction?TorchFunction->K2_GetScalarParameterValue(TEXT("ExtinctionPerCm")):0;
                const bool valid=FMath::Abs(visibility-640)<.001&&FMath::Abs(extinction-1.45/640)<1.e-7;
                UE_LOG(LogTemp,Display,TEXT("DIVE_ENVIRONMENT_MATERIALS %s visibility_cm=%.6f extinction_per_cm=%.9f"),valid?TEXT("PASS"):TEXT("FAIL"),visibility,extinction);
                if(!valid) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
            }
            const bool passed=hud->EnvironmentSmokeTest(ControlsSmokePhase);
            UE_LOG(LogTemp,Display,TEXT("DIVE_ENVIRONMENT_PHASE %d %s"),ControlsSmokePhase,passed?TEXT("PASS"):TEXT("FAIL"));
            if(!passed) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
            ++ControlsSmokePhase;
        }
        if(ControlsSmokePhase==8&&ControlsSmokeClock>27)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_ENVIRONMENT_PASS ui=1 seasonal=1 manual_restore=1 presets=1 validation=1 persistence=1 profiles=1 active_guard=1"));
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS environment=1"));
            FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
    if(SmokeScenario==TEXT("config"))
    {
        if(ControlsSmokePhase<=10&&ControlsSmokeClock>(ControlsSmokePhase+1)*1.5)
        {
            const bool passed=hud->ConfigSmokeTest(ControlsSmokePhase);
            UE_LOG(LogTemp,Display,TEXT("DIVE_CONFIG_PHASE %d %s"),ControlsSmokePhase,passed?TEXT("PASS"):TEXT("FAIL"));
            if(!passed) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
            ++ControlsSmokePhase;
        }
        if(ControlsSmokePhase==11&&ControlsSmokeClock>18.5)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_CONFIG_PASS calendar=1 dst_gap=1 numeric=1 minute_clock=1 validation=1 persistence=1 cancel=1 active_guard=1"));
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_PASS config=1"));
            FPlatformMisc::RequestExitWithStatus(false,0);
        }
        return;
    }
    if(SmokeScenario==TEXT("profiles"))
    {
        if(ControlsSmokePhase<=10&&ControlsSmokeClock>(ControlsSmokePhase+1)*.8)
        {
            const bool passed=hud->ProfileSmokeTest(ControlsSmokePhase);
            bControlsSmokeValid&=passed;
            UE_LOG(LogTemp,Display,TEXT("DIVE_PROFILE_PHASE %d %s"),ControlsSmokePhase,passed?TEXT("PASS"):TEXT("FAIL"));
            if(!passed) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
            if(ControlsSmokePhase==10) UE_LOG(LogTemp,Display,TEXT("DIVE_PROFILES_PASS persistence=1 undo=1 archive=1 active_guard=1"));
            ++ControlsSmokePhase;
        }
        return;
    }
    if(SmokeScenario==TEXT("tutorial"))
    {
        if(TutorialFinishedAt>0)
        {
            if(GetWorld()->GetTimeSeconds()-TutorialFinishedAt>1&&TestLessonScreenshots==255)
            { shot(TEXT("TutorialComplete")); TestLessonScreenshots|=256; }
            if(GetWorld()->GetTimeSeconds()-TutorialFinishedAt>3)
            {
                FString json; FDiveLog log; log.Path=LastLogPath;
                const bool saved=FFileHelper::LoadFileToString(json,*LastLogPath)&&DiveLogbook::Parse(json,log)&&log.Status==TEXT("completed")&&log.Samples.Num()>=5;
                bTutorialCompleted=false; LoadSettings();
                const bool restored=PendingConfig.CurrentMetresPerSecond==.16&&PendingConfig.CylinderLitres==15&&PendingConfig.StartPressureBar==250&&
                    !PendingConfig.DrySuit&&PendingConfig.WeightKg==10&&PendingConfig.Weather==2&&PendingConfig.WindMetresPerSecond==6&&
                    EntryIndex==2&&PendingConfig.GameMode==Dive::Mode::Exploration;
                const bool valid=saved&&bTutorialCompleted&&TestLessonScreenshots==511&&bSmokeSecondaryCaptured&&bControlsSmokeValid&&restored;
                UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s tutorial=1 snapshots=%u samples=%d"),valid?TEXT("PASS"):TEXT("FAIL"),TestLessonScreenshots,log.Samples.Num());
                FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
            }
            return;
        }
        const int stage=static_cast<int>(Tutorial.Stage);
        if(stage!=TestLessonStage) { TestLessonStage=stage; TestLessonAge=0; UE_LOG(LogTemp,Display,TEXT("DIVE_TUTORIAL_STAGE %d"),stage); }
        TestLessonAge+=Dt;
        if(Tutorial.Stage==Dive::Lesson::Computer&&ControlsSmokePhase==0)
        {
            SaveSettings(); FString json; TSharedPtr<FJsonObject> settings;
            bControlsSmokeValid=FFileHelper::LoadFileToString(json,*(FPaths::ProjectSavedDir()/TEXT("DiveSettingsTest.json")))&&
                FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),settings)&&settings.IsValid();
            if(bControlsSmokeValid) bControlsSmokeValid=settings->GetNumberField(TEXT("cylinder_litres"))==15&&settings->GetNumberField(TEXT("pressure_bar"))==250&&
                !settings->GetBoolField(TEXT("dry_suit"))&&settings->GetNumberField(TEXT("weight_kg"))==10&&settings->GetNumberField(TEXT("entry"))==2&&settings->GetNumberField(TEXT("mode"))==0;
            UE_LOG(LogTemp,Display,TEXT("DIVE_TUTORIAL_SETTINGS_%s"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"));
            ControlsSmokePhase=1;
        }
        if(bShowMap&&!bSmokeSecondaryCaptured)
        { shot(TEXT("TutorialMap")); bSmokeSecondaryCaptured=true; return; }
        if(TestLessonAge>.3&&!(TestLessonScreenshots&(1u<<stage)))
        { shot(FString::Printf(TEXT("Tutorial_%d"),stage)); TestLessonScreenshots|=1u<<stage; }
        const bool act=TestLessonAge>.6;
        const auto lesson=Tutorial.Stage;
        if(act&&lesson==Dive::Lesson::Look)
            pc->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::MouseX,IE_Axis,10));
        TestKey(Controls.Key(EDiveAction::Forward),act&&lesson==Dive::Lesson::Swim);
        TestKey(Controls.Key(EDiveAction::Inflate),act&&lesson==Dive::Lesson::Buoyancy&&Tutorial.InflatedLitres<1.5);
        TestKey(Controls.Key(EDiveAction::Deflate),act&&lesson==Dive::Lesson::Buoyancy&&Tutorial.InflatedLitres>=1.5);
        if(act&&lesson==Dive::Lesson::Computer&&((!Tutorial.SawVitals&&!bShowComputer)||(Tutorial.SawVitals&&bShowComputer))) tap(Controls.Key(EDiveAction::Computer));
        TestKey(Controls.Key(EDiveAction::ComputerPage),act&&lesson==Dive::Lesson::Computer&&bShowComputer&&!Tutorial.SawVitals&&ComputerPage!=1);
        if(act&&lesson==Dive::Lesson::Lamp&&((!Tutorial.SawLampOff&&bLamp)||(Tutorial.SawLampOff&&!bLamp))) tap(Controls.Key(EDiveAction::Lamp));
        if(act&&lesson==Dive::Lesson::Map&&((!Tutorial.SawMap&&!bShowMap)||(Tutorial.SawMap&&bShowMap))) tap(Controls.Key(EDiveAction::Map));
        TestKey(Controls.Key(EDiveAction::Ascend),act&&lesson==Dive::Lesson::Return);
        TestKey(Controls.Key(EDiveAction::Finish),act&&lesson==Dive::Lesson::Exit);
        if(ControlsSmokeClock>100) { UE_LOG(LogTemp,Error,TEXT("DIVE_TUTORIAL_FAIL timeout stage=%d"),stage); FPlatformMisc::RequestExitWithStatus(false,1); }
        return;
    }
    const double t=ControlsSmokeClock;
    if(ControlsSmokePhase==0) { SetMenu(true); ControlsSmokePhase=1; }
    if(ControlsSmokePhase==1&&t>.5)
    {
        tap(EKeys::F1); hud->NotifyHitBoxClick(TEXT("bind_0")); tap(EKeys::I);
        bControlsSmokeValid&=Controls.Key(EDiveAction::Forward)==EKeys::I;
        hud->NotifyHitBoxClick(TEXT("bind_13")); tap(EKeys::Escape);
        bControlsSmokeValid&=bMenu&&CapturingAction==INDEX_NONE&&Controls.Key(EDiveAction::Map)==EKeys::M;
        tap(EKeys::Escape); bControlsSmokeValid&=!hud->ControlsOpen()&&bMenu;
        tap(EKeys::F1); bControlsSmokeValid&=hud->ControlsOpen();
        hud->NotifyHitBoxClick(TEXT("bind_1")); tap(EKeys::I);
        bControlsSmokeValid&=Controls.Key(EDiveAction::Backward)==EKeys::I&&Controls.Key(EDiveAction::Forward)==EKeys::S;
        hud->NotifyHitBoxClick(TEXT("bind_0")); tap(EKeys::I);
        hud->NotifyHitBoxClick(TEXT("bind_9")); tap(EKeys::T);
        hud->NotifyHitBoxClick(TEXT("sensitivity_plus")); hud->NotifyHitBoxClick(TEXT("invert_y")); hud->NotifyHitBoxClick(TEXT("fov_plus"));
        const auto expected=Controls; Controls=FDiveControls{}; LoadSettings();
        bControlsSmokeValid&=Controls.Keys==expected.Keys&&Controls.InvertY&&FMath::Abs(Controls.Sensitivity-.15)<1e-6&&Camera->FieldOfView==95;
        hud->NotifyHitBoxClick(TEXT("controls_language")); ControlsSmokePhase=2;
    }
    if(ControlsSmokePhase==2&&t>1.1) { shot(TEXT("Controls_EN")); ControlsSmokePhase=3; }
    if(ControlsSmokePhase==3&&t>1.6) { hud->NotifyHitBoxClick(TEXT("controls_language")); shot(TEXT("Controls_DE")); ControlsSmokePhase=4; }
    if(ControlsSmokePhase==4&&t>2.2) { hud->NotifyHitBoxClick(TEXT("controls_back")); ResumeDive(); ControlsSmokePhase=5; }
    if(ControlsSmokePhase==5&&t>3)
    { bControlsSmokeValid&=Simulation.Data.DistanceMetres<.3; ControlsSmokePhase=6; }
    if(ControlsSmokePhase==6&&t>4.1) { bControlsSmokeValid&=!bLamp; ControlsSmokePhase=7; }
    if(ControlsSmokePhase==7&&t>5.1)
    {
        bControlsSmokeValid&=bLamp;
        UE_LOG(LogTemp,Display,TEXT("DIVE_INPUT_%s remap=I lamp=T sensitivity=%.2f fov=%.0f"),bControlsSmokeValid?TEXT("PASS"):TEXT("FAIL"),Controls.Sensitivity,Controls.FieldOfView);
        ControlsSmokePhase=8;
    }
    if(t>=14&&t<16&&!bMenu) tap(EKeys::Escape);
    if(t>=16&&bMenu) tap(EKeys::Escape);
    TestKey(EKeys::W,!bMenu&&t<3); // Old forward key must no longer swim.
    // The conflicting T binding swaps the regulator onto L. Exercise L twice
    // so the old lamp key cannot toggle the lamp or leave the regulator removed.
    TestKey(EKeys::L,!bMenu&&((t>3.3&&t<3.5)||(t>3.6&&t<3.8)));
    TestKey(EKeys::T,!bMenu&&((t>=4&&t<4.2)||(t>=5&&t<5.2)));
    TestKey(EKeys::I,!bMenu&&t>=3&&t<13);
    TestKey(Controls.Key(EDiveAction::Descend),!bMenu&&t>=3&&t<10);
    TestKey(Controls.Key(EDiveAction::Deflate),!bMenu&&t>=3&&t<7);
    TestKey(Controls.Key(EDiveAction::Inflate),!bMenu&&t>=13&&t<17);
    TestKey(Controls.Key(EDiveAction::Ascend),!bMenu&&t>=19);
}
