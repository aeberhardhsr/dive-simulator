#include "DivePawn.h"
#include "DiveHUD.h"
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

void ADivePawn::TestKey(FKey Key,bool Down)
{
    if(Down==TestHeldKeys.Contains(Key)) return;
    if(auto* pc=Cast<APlayerController>(GetController()))
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(Key,Down?IE_Pressed:IE_Released,Down?1:0));
    if(Down) TestHeldKeys.Add(Key); else TestHeldKeys.Remove(Key);
}
void ADivePawn::InputSmokeTick(float Dt)
{
    ControlsSmokeClock+=Dt;
    auto* pc=Cast<APlayerController>(GetController());
    auto* hud=pc?Cast<ADiveHUD>(pc->GetHUD()):nullptr;
    if(!pc||!hud) return;
    auto tap=[pc](FKey Key)
    {
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Pressed,1));
        pc->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0));
    };
    auto shot=[this](const FString& Name)
    { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(Name+TEXT(".png")),true,false); };
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
    TestKey(EKeys::L,!bMenu&&t>3.3&&t<3.5); // Old lamp key must no longer toggle.
    TestKey(EKeys::T,!bMenu&&((t>=4&&t<4.2)||(t>=5&&t<5.2)));
    TestKey(EKeys::I,!bMenu&&t>=3&&t<13);
    TestKey(Controls.Key(EDiveAction::Descend),!bMenu&&t>=3&&t<10);
    TestKey(Controls.Key(EDiveAction::Deflate),!bMenu&&t>=3&&t<7);
    TestKey(Controls.Key(EDiveAction::Inflate),!bMenu&&t>=13&&t<17);
    TestKey(Controls.Key(EDiveAction::Ascend),!bMenu&&t>=19);
}
