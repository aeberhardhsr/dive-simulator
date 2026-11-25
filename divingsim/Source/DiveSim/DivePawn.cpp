#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveEquipment.h"
#include "DiveAudio.h"
#include "AudioMixerBlueprintLibrary.h"
#include "DiveHUD.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/Guid.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonWriter.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
void SaveEnvironment(const TSharedPtr<FJsonObject>& Obj,const Dive::Config& Config)
{
    Obj->SetNumberField(TEXT("weather"),Config.Weather);
    Obj->SetNumberField(TEXT("wind_mps"),Config.WindMetresPerSecond);
    Obj->SetNumberField(TEXT("wind_direction_degrees"),Config.WindDirectionDegrees);
    Obj->SetNumberField(TEXT("surface_temperature_c"),Config.SurfaceTemperatureC);
    Obj->SetNumberField(TEXT("bottom_temperature_c"),Config.BottomTemperatureC);
    Obj->SetNumberField(TEXT("thermocline_m"),Config.ThermoclineMetres);
    Obj->SetNumberField(TEXT("thermocline_width_m"),Config.ThermoclineWidthMetres);
    Obj->SetNumberField(TEXT("visibility_m"),Config.VisibilityMetres);
    Obj->SetNumberField(TEXT("current_mps"),Config.CurrentMetresPerSecond);
    Obj->SetNumberField(TEXT("hour"),Config.Hour);
}
}

ADivePawn::ADivePawn()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UCapsuleComponent>(TEXT("DiverBody"));
    Body->InitCapsuleSize(28,65); Body->SetCollisionProfileName(TEXT("Pawn"));
    RootComponent=Body;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes"));
    Camera->SetupAttachment(Body); Camera->bUsePawnControlRotation=true;
    Camera->FieldOfView=90;
    Lamp=CreateDefaultSubobject<USpotLightComponent>(TEXT("DiveLamp"));
    Lamp->SetupAttachment(Camera); Lamp->SetRelativeLocation(FVector(25,20,-20));
    Lamp->SetIntensity(14000); Lamp->SetAttenuationRadius(2400);
    Lamp->SetInnerConeAngle(12); Lamp->SetOuterConeAngle(27);
    Lamp->SetLightColor(FLinearColor(.8f,.92f,1));
    Lamp->SetCastShadows(true);
    Audio=CreateDefaultSubobject<UDiveAudio>(TEXT("DiveAudio"));
    AutoPossessPlayer=EAutoReceiveInput::Player0;
}

void ADivePawn::BeginPlay()
{
    Super::BeginPlay();
    bSmokeTest=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"));
    if(!bSmokeTest) LoadSettings();
    if(bSmokeTest)
    {
        FParse::Value(FCommandLine::Get(),TEXT("DiveEntry="),EntryIndex);
        FParse::Value(FCommandLine::Get(),TEXT("DiveScenario="),SmokeScenario);
        if(SmokeScenario==TEXT("stream")) PendingConfig.GameMode=Dive::Mode::Exploration;
        if(SmokeScenario==TEXT("wrist-night")) PendingConfig.Hour=23;
        if(SmokeScenario==TEXT("surface-calm")) PendingConfig.WindMetresPerSecond=0;
        if(SmokeScenario==TEXT("surface-rain")) { PendingConfig.WindMetresPerSecond=12; PendingConfig.Weather=2; }
        if(SmokeScenario==TEXT("audio")) { PendingConfig.WindMetresPerSecond=9; PendingConfig.Weather=2; }
    }
    Environment=GetWorld()->SpawnActor<ADiveWorld>();
    Environment->Diver=this;
    Environment->AddTickPrerequisiteActor(this);
    Equipment=GetWorld()->SpawnActor<ADiveEquipment>(); Equipment->SetOwner(this); Equipment->Initialize(this);
    if(Environment->bTerrainReady)
    {
        EntryIndex=FMath::Clamp(EntryIndex,0,Environment->Terrain.Entries.Num()-1);
        Environment->Configure(PendingConfig);
        Environment->SetEntry(EntryIndex);
        EntryLocation=Environment->Terrain.Entries[EntryIndex].Position;
    }
    SetActorLocation(EntryLocation);
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        PC->SetControlRotation(FRotator(-8,0,0));
        PC->PlayerCameraManager->ViewPitchMin=-85;
        PC->PlayerCameraManager->ViewPitchMax=85;
    }
    Audio->Initialize(Camera); Audio->ApplyLevels(AudioLevels);
    SetMenu(true);
    if(auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_Underwater.M_Underwater")))
        UnderwaterMaterial=UMaterialInstanceDynamic::Create(material,this);
    if(bSmokeTest)
    {
        if(Environment->bTerrainReady) StartDive();
        else { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL missing terrain")); FPlatformMisc::RequestExitWithStatus(false,1); }
    }
}

void ADivePawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    // A scripted run must not capture mouse/shortcut input from another game window.
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))) return;
    Input->BindAxisKey(EKeys::MouseX,this,&ADivePawn::LookYaw);
    Input->BindAxisKey(EKeys::MouseY,this,&ADivePawn::LookPitch);
    Input->BindKey(EKeys::Escape,IE_Pressed,this,&ADivePawn::ToggleMenu);
    Input->BindKey(EKeys::L,IE_Pressed,this,&ADivePawn::ToggleLamp);
    Input->BindKey(EKeys::H,IE_Pressed,this,&ADivePawn::ToggleHUD);
    Input->BindKey(EKeys::C,IE_Pressed,this,&ADivePawn::ToggleComputer);
    Input->BindKey(EKeys::R,IE_Pressed,this,&ADivePawn::NextComputerPage);
    Input->BindKey(EKeys::M,IE_Pressed,this,&ADivePawn::ToggleMap);
    Input->BindKey(EKeys::F,IE_Pressed,this,&ADivePawn::Interact);
    Input->BindKey(EKeys::P,IE_Pressed,this,&ADivePawn::Photograph);
}

void ADivePawn::LookYaw(float Value) { if(!bMenu) AddControllerYawInput(Value*MouseSensitivity); }
void ADivePawn::LookPitch(float Value) { if(!bMenu) AddControllerPitchInput(-Value*MouseSensitivity); }
void ADivePawn::ToggleLamp() { if(!bMenu) bLamp=!bLamp; }
void ADivePawn::ToggleHUD() { if(!bMenu) bShowInstruments=!bShowInstruments; }
void ADivePawn::ToggleComputer() { if(!bMenu) bShowComputer=!bShowComputer; }
void ADivePawn::NextComputerPage() { if(!bMenu&&bShowComputer) ComputerPage=(ComputerPage+1)%3; }
void ADivePawn::ToggleMap() { if(!bMenu) bShowMap=!bShowMap; }
void ADivePawn::ToggleMenu() { if(bLogbook) { bLogbook=false; return; } SetMenu(!bMenu || !bDiveActive); }
void ADivePawn::ResumeDive() { if(bDiveActive) SetMenu(false); }

void ADivePawn::SetMenu(bool Open)
{
    bMenu=Open;
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor=Open;
        PC->bEnableClickEvents=Open;
        if(Open)
        {
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Mode);
        }
        else PC->SetInputMode(FInputModeGameOnly());
    }
    Audio->SetPaused(Open);
}

void ADivePawn::StartDive()
{
    if(!Environment || !Environment->bTerrainReady) return;
    PendingConfig.Sanitize();
    Simulation.Reset(PendingConfig);
    Environment->Configure(PendingConfig);
    Environment->SetEntry(EntryIndex);
    const auto& entry=Environment->Terrain.Entries[EntryIndex];
    EntryLocation=entry.Position;
    SetActorLocation(EntryLocation); VelocityMetres=FVector::ZeroVector;
    if(auto* PC=Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(-8,entry.Yaw,0));
    // Start trimmed at entry depth; later depth changes require manual compensation.
    const double pressure=Simulation.Pressure(Environment->PressureDepth(EntryLocation,0));
    const double suit=(PendingConfig.DrySuit?3:5)/std::pow(pressure/PendingConfig.SurfacePressureBar,.7);
    Simulation.Data.BCDSurfaceLitres=FMath::Max(0.0,PendingConfig.WeightKg+
        PendingConfig.StartPressureBar*PendingConfig.CylinderLitres*.0012-2-suit)*pressure;
    Samples.Reset(); Accumulator=0; TelemetryAccumulator=0;
    Audio->Reset(); Audio->ApplyLevels(AudioLevels);
    bDiveActive=true; bSummary=false; bLamp=true; bShowMap=false; bShowComputer=false; ComputerPage=0;
    Outcome.Empty(); LastLogPath.Empty();
    bLogbook=false; DiveStartedUTC=FDateTime::UtcNow().ToIso8601();
    if(!bSmokeTest) SaveSettings();
    SetMenu(false);
    if(bSmokeTest&&SmokeScenario==TEXT("audio"))
    {
        // The unattended test is deliberately hidden; capture its own mix anyway.
        FApp::SetUnfocusedVolumeMultiplier(1);
        IFileManager::Get().Delete(*(FPaths::ProjectSavedDir()/TEXT("AudioQA/DiveMix.wav")));
        UAudioMixerBlueprintLibrary::StartRecordingOutput(this,60);
    }
    Notify(Local(TEXT("WASD schwimmen  |  Q/E tarieren  |  L Lampe  |  ESC Pause"),TEXT("WASD swim  |  Q/E buoyancy  |  L torch  |  ESC pause")));
}

void ADivePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(bSmokeTest&&SmokeScenario==TEXT("audio"))
    {
        AudioSmokeClock+=DeltaSeconds;
        if(AudioExportStarted>0)
        {
            if(AudioSmokeClock-AudioExportStarted>3)
            {
                const bool saved=IFileManager::Get().FileSize(*(FPaths::ProjectSavedDir()/TEXT("AudioQA/DiveMix.wav")))>100000;
                UE_LOG(LogTemp,Display,TEXT("DIVE_AUDIO_%s inhale=%d exhale=%d heart=%d warnings=%d inflate=%d vent=%d"),
                    saved?TEXT("PASS"):TEXT("FAIL"),Audio->Inhales,Audio->Exhales,Audio->Heartbeats,Audio->Warnings,Audio->InflationSteps,Audio->VentSteps);
                FPlatformMisc::RequestExitWithStatus(false,saved?0:1);
            }
            return;
        }
        const bool paused=AudioSmokeClock>=8&&AudioSmokeClock<11;
        if(bMenu!=paused) { SetMenu(paused); UE_LOG(LogTemp,Display,TEXT("DIVE_AUDIO_PAUSE t=%.3f value=%d"),AudioSmokeClock,paused); }
        if(paused&&!bAudioMenuChecked)
        {
            auto* PC=Cast<APlayerController>(GetController());
            auto* HUD=PC?Cast<ADiveHUD>(PC->GetHUD()):nullptr;
            if(!HUD) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
            HUD->NotifyHitBoxClick(TEXT("audio"));
            for(int i=0;i<5;++i) HUD->NotifyHitBoxClick(FName(*FString::Printf(TEXT("audio_minus_%d"),i)));
            const auto changed=AudioLevels;
            AudioLevels=Dive::AudioLevels{}; LoadSettings();
            bool restored=true;
            for(int i=0;i<5;++i) restored&=FMath::Abs(AudioLevels.Values[i]-changed.Values[i])<1e-9;
            HUD->NotifyHitBoxClick(TEXT("language"));
            UE_LOG(LogTemp,Display,TEXT("DIVE_AUDIO_SETTINGS_%s"),restored?TEXT("PASS"):TEXT("FAIL"));
            if(!restored) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
            bAudioMenuChecked=true;
        }
        if(paused&&AudioSmokeClock>9&&!bAudioMenuCaptured)
        {
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/AudioMenu.png"),true,false);
            bAudioMenuCaptured=true;
        }
        const double master=AudioSmokeClock>=17&&AudioSmokeClock<20?0:.8;
        if(AudioLevels.Values[0]!=master)
        { AudioLevels.Values[0]=master; Audio->ApplyLevels(AudioLevels); UE_LOG(LogTemp,Display,TEXT("DIVE_AUDIO_MASTER t=%.3f value=%.2f"),AudioSmokeClock,master); }
    }
    if(bSmokeTest&&SmokeLogbookStarted>0)
    {
        auto* PC=Cast<APlayerController>(GetController());
        auto* HUD=PC?Cast<ADiveHUD>(PC->GetHUD()):nullptr;
        if(HUD&&HUD->LogbookReady())
        {
            SmokeLogbookTime+=DeltaSeconds;
            const int group=FMath::Min(3,FMath::FloorToInt(SmokeLogbookTime/3));
            HUD->LogMetricGroup=group; bEnglish=group>=2;
            if(FMath::Fmod(SmokeLogbookTime,3.)>1.5&&!(SmokeLogbookCaptured&(1u<<group)))
            {
                SmokeLogbookCaptured|=1u<<group;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Logbook_%d.png"),group),true,false);
            }
            if(SmokeLogbookTime>12)
            {
                const bool valid=HUD->LogbookMatches(LastLogPath,Samples.Num())&&HUD->LogRenderedGroups==15&&SmokeLogbookCaptured==15;
                UE_LOG(LogTemp,Display,TEXT("DIVE_LOGBOOK_%s samples=%d groups=%u"),valid?TEXT("PASS"):TEXT("FAIL"),Samples.Num(),HUD->LogRenderedGroups);
                FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
            }
        }
        if(GetWorld()->GetTimeSeconds()-SmokeLogbookStarted>45)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_LOGBOOK_FAIL load timeout")); FPlatformMisc::RequestExitWithStatus(false,1); }
        if(Equipment) Equipment->Update(DeltaSeconds);
        return;
    }
    if(bDiveActive && !bMenu)
    {
        Accumulator+=FMath::Min(DeltaSeconds,.25f);
        while(Accumulator>=.02f && bDiveActive) { SimulateStep(.02f); Accumulator-=.02f; }
    }
    const double smokeDuration=(SmokeScenario.StartsWith(TEXT("surface"))||SmokeScenario==TEXT("audio"))?38:SmokeScenario==TEXT("stream")?90:22;
    if(bSmokeTest && bDiveActive)
    {
        FString audioError;
        if(!Audio->Validate(audioError))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_AUDIO_FAIL %s"),*audioError); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        FString ecologyError;
        if(!Environment->ValidateEcology(ecologyError))
        {
            UE_LOG(LogTemp,Error,TEXT("DIVE_ECOLOGY_FAIL %s"),*ecologyError);
            FPlatformMisc::RequestExitWithStatus(false,1);
            return;
        }
        FString equipmentError;
        if(!Equipment||!Equipment->Validate(equipmentError))
        {
            UE_LOG(LogTemp,Error,TEXT("DIVE_EQUIPMENT_FAIL %s"),*equipmentError);
            FPlatformMisc::RequestExitWithStatus(false,1); return;
        }
        FString waterError;
        if(!Environment->ValidateWater(waterError))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_WATER_FAIL %s"),*waterError); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        bSmokeSurfaceReached|=GetActorLocation().Z>Environment->SurfaceZ(GetActorLocation().X,GetActorLocation().Y)+2;
        if(SmokeScenario.StartsWith(TEXT("surface"))&&Simulation.Data.ElapsedSeconds>25&&!bSmokeSecondaryCaptured&&
            GetActorLocation().Z>Environment->SurfaceZ(GetActorLocation().X,GetActorLocation().Y)+8)
        {
            bSmokeSecondaryCaptured=true;
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/SurfaceAir_%d_%s.png"),EntryIndex,*SmokeScenario),true,false);
        }
        bSmokeChunkLimit&=Environment->TerrainChunkCount()<=9;
        if(SmokeScenario==TEXT("stream"))
            if(auto* PC=Cast<APlayerController>(GetController()))
                PC->SetControlRotation(FRotator(0,Environment->Terrain.Entries[EntryIndex].Yaw,0));
        if(Simulation.Data.ElapsedSeconds>smokeDuration-4) bShowMap=!SmokeScenario.StartsWith(TEXT("surface"))&&!SmokeScenario.StartsWith(TEXT("wrist"));
        if(SmokeScenario.StartsWith(TEXT("wrist")))
        {
            const double t=Simulation.Data.ElapsedSeconds;
            bShowComputer=t>4; bShowInstruments=false;
            ComputerPage=t<10?0:t<16?1:2;
            if(t>13&&!bSmokeSecondaryCaptured)
            {
                bSmokeSecondaryCaptured=true;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/WristVitals_%s.png"),*SmokeScenario),true,false);
            }
        }
    }
    if(bSmokeTest && Simulation.Data.ElapsedSeconds>8 && !bSmokeUnderwaterCaptured)
    {
        bSmokeUnderwaterCaptured=true;
        if(SmokeScenario.StartsWith(TEXT("wrist")))
            Equipment->ExportDisplayForTest(FPaths::ProjectSavedDir()/TEXT("Screenshots"),TEXT("WristTexture_")+SmokeScenario+TEXT(".png"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/DiveUnderwater_%d_%s.png"),EntryIndex,*SmokeScenario),true,false);
    }
    if(bSmokeTest && Simulation.Data.ElapsedSeconds>smokeDuration-3 && !bSmokeCaptured)
    {
        bSmokeCaptured=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/DiveSmoke_%d_%s.png"),EntryIndex,*SmokeScenario),true,false);
    }
    if(bSmokeTest && bDiveActive && Simulation.Data.ElapsedSeconds>=smokeDuration)
    {
        const double displacement=FVector::Dist2D(GetActorLocation(),EntryLocation)/100;
        const bool extended=SmokeScenario.StartsWith(TEXT("surface"))?bSmokeSurfaceReached:
            SmokeScenario==TEXT("stream")?displacement>80:true;
        const bool wristReady=!SmokeScenario.StartsWith(TEXT("wrist"))||
            (Equipment->WristBlend>.95f&&ComputerPage==2&&Equipment->DisplayUpdates>60&&Equipment->DisplayedPages==7);
        const bool audioReady=SmokeScenario!=TEXT("audio")||(Audio->Inhales>4&&Audio->Exhales>4&&Audio->Heartbeats>30&&Audio->InflationSteps>100&&Audio->VentSteps>100);
        const bool valid=audioReady && extended && wristReady && bSmokeChunkLimit && Simulation.Data.DistanceMetres>5
            && (SmokeScenario==TEXT("stream")||Simulation.Data.MaxDepthMetres>2.5)
            && (SmokeScenario==TEXT("stream")||Simulation.Data.CylinderBar<199)
            && Samples.Num()>=static_cast<int>(smokeDuration)-1 && FMath::IsFinite(GetActorLocation().Z);
        FinishDive(valid?TEXT("smoke_pass"):TEXT("smoke_failed"));
        FString persisted;
        FDiveLog readback; readback.Path=LastLogPath;
        const bool saved=FFileHelper::LoadFileToString(persisted,*LastLogPath)&&DiveLogbook::Parse(persisted,readback)&&
            readback.Status==TEXT("smoke_pass")&&readback.Samples.Num()==Samples.Num()&&
            FMath::Abs(readback.Duration-Simulation.Data.ElapsedSeconds)<.001&&
            !readback.Samples.IsEmpty()&&FMath::Abs(readback.Samples.Last().CylinderBar-Simulation.Data.CylinderBar)<.001;
        UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s depth=%.2f distance=%.2f pressure=%.2f samples=%d log=%s"),
            valid&&saved?TEXT("PASS"):TEXT("FAIL"),Simulation.Data.MaxDepthMetres,Simulation.Data.DistanceMetres,
            Simulation.Data.CylinderBar,Samples.Num(),*LastLogPath);
        UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_DETAIL entry=%d scenario=%s displacement=%.2f surface=%d chunks=%d bounded=%d"),
            EntryIndex,*SmokeScenario,displacement,bSmokeSurfaceReached,Environment->TerrainChunkCount(),bSmokeChunkLimit);
        UE_LOG(LogTemp,Display,TEXT("DIVE_EQUIPMENT_CHECK updates=%d wrist=%.3f page=%d rendered_pages=%u"),Equipment->DisplayUpdates,Equipment->WristBlend,ComputerPage,Equipment->DisplayedPages);
        if(SmokeScenario==TEXT("audio")&&valid&&saved)
        {
            const FString path=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("AudioQA"));
            IFileManager::Get().MakeDirectory(*path,true);
            UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("DiveMix"),path);
            AudioExportStarted=AudioSmokeClock;
        }
        else if(SmokeScenario==TEXT("logbook")&&valid&&saved)
        {
            auto* PC=Cast<APlayerController>(GetController());
            if(auto* HUD=PC?Cast<ADiveHUD>(PC->GetHUD()):nullptr)
            { HUD->OpenLogbook(LastLogPath,true); SmokeLogbookStarted=GetWorld()->GetTimeSeconds(); }
            else FPlatformMisc::RequestExitWithStatus(false,1);
        }
        else FPlatformMisc::RequestExitWithStatus(false,valid&&saved?0:1);
    }
    Lamp->SetVisibility(bLamp && Simulation.Data.LampBattery>0);
    const FVector eye=Camera->GetComponentLocation();
    const float depth=FMath::Max(0.f,static_cast<float>((Environment->SurfaceZ(eye.X,eye.Y)-eye.Z)/100));
    const float underwater=FMath::Clamp(depth*2.f,0.f,1.f);
    if(UnderwaterMaterial)
    {
        UnderwaterMaterial->SetScalarParameterValue(TEXT("VisibilityCm"),PendingConfig.VisibilityMetres*100);
        Camera->AddOrUpdateBlendable(UnderwaterMaterial,underwater);
    }
    auto& PP=Camera->PostProcessSettings;
    Camera->PostProcessBlendWeight=1;
    PP.bOverride_ColorSaturation=true;
    PP.ColorSaturation=FMath::Lerp(FVector4(1,1,1,1),FVector4(.68,.85,.9,1),underwater);
    PP.bOverride_SceneColorTint=true;
    PP.SceneColorTint=FMath::Lerp(FLinearColor::White,FLinearColor(.48,.83,.78),underwater);
    PP.bOverride_VignetteIntensity=true; PP.VignetteIntensity=.35f;
    PP.bOverride_AutoExposureMethod=true; PP.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    PP.bOverride_AutoExposureBias=true; PP.AutoExposureBias=8.5f-depth*.018f;
    PP.bOverride_MotionBlurAmount=true; PP.MotionBlurAmount=0;
    PP.bOverride_DepthOfFieldFocalDistance=true; PP.DepthOfFieldFocalDistance=0;
    if(bCameraMotion && !bMenu)
        Camera->SetRelativeLocation(FVector(0,0,static_cast<float>(std::sin(Simulation.Data.BreathPhase*2*Dive::Pi)*.8)));
    else Camera->SetRelativeLocation(FVector::ZeroVector);
    if(Equipment) Equipment->Update(DeltaSeconds);
}

void ADivePawn::SimulateStep(float Step)
{
    auto* PC=Cast<APlayerController>(GetController()); if(!PC) return;
    auto Down=[this,PC](FKey Key)
    {
        if(!bSmokeTest) return PC->IsInputKeyDown(Key)?1.f:0.f;
        const double t=Simulation.Data.ElapsedSeconds;
        if(SmokeScenario==TEXT("stream")) return Key==EKeys::W?1.f:0.f;
        if((SmokeScenario.StartsWith(TEXT("surface"))||SmokeScenario==TEXT("audio")) && t>16) return Key==EKeys::SpaceBar?1.f:0.f;
        if(Key==EKeys::W) return t<10?1.f:0.f;
        if(Key==EKeys::LeftControl) return t<7?.7f:0.f;
        if(Key==EKeys::E) return t<4?.3f:0.f;
        if(Key==EKeys::Q) return t>10 && t<14?1.f:0.f;
        if(Key==EKeys::SpaceBar) return t>16?1.f:0.f;
        return 0.f;
    };
    const float forward=Down(EKeys::W)-Down(EKeys::S);
    const float side=Down(EKeys::D)-Down(EKeys::A);
    const float vertical=Down(EKeys::SpaceBar)-Down(EKeys::LeftControl);
    const float inflate=Down(EKeys::Q)-Down(EKeys::E);
    const float sprint=Down(EKeys::LeftShift);
    const FVector old=GetActorLocation();
    const FRotationMatrix orientation(PC->GetControlRotation());
    FVector wish=orientation.GetUnitAxis(EAxis::X)*forward+orientation.GetUnitAxis(EAxis::Y)*side+FVector::UpVector*vertical;
    wish=wish.GetClampedToMaxSize(1);
    const float effort=wish.Size()*(sprint>0?1.f:.45f);
    const FVector current=FVector(0,PendingConfig.CurrentMetresPerSecond,0)+
        Environment->WaveVelocity(old,Simulation.Data.ElapsedSeconds);
    const FVector relative=VelocityMetres-current;
    const float fatigue=1.f-static_cast<float>(Simulation.Data.Fatigue)*.35f;
    FVector force=wish*SwimForceNewtons*(sprint>0?1.7f:1.f)*fatigue;
    force.Z+=Simulation.Data.BuoyancyNewtons;
    // Gradually lose displaced body volume as the head crosses the surface.
    // Upward swimming can lift the eyes into air, without allowing flight.
    const double now=Simulation.Data.ElapsedSeconds;
    const double surface=Environment->SurfaceZ(old.X,old.Y,now);
    force.Z-=FMath::Clamp((old.Z-surface)/65.0,0.0,1.0)*90*9.80665;
    force-=relative*relative.Size()*DragCoefficient;
    VelocityMetres+=force/100.f*Step;
    VelocityMetres=VelocityMetres.GetClampedToMaxSize(2.5f);
    FHitResult hit;
    AddActorWorldOffset(VelocityMetres*100*Step,true,&hit);
    if(hit.bBlockingHit)
    {
        VelocityMetres=FVector::VectorPlaneProject(VelocityMetres,hit.Normal);
        AddActorWorldOffset(VelocityMetres*100*Step*(1-hit.Time),true);
    }
    FVector position=GetActorLocation();
    float bottom=-30000;
    if(!Environment->Terrain.Sample(position.X,position.Y,bottom))
    {
        position.X=old.X; position.Y=old.Y;
        VelocityMetres.X=0; VelocityMetres.Y=0;
        Environment->Terrain.Sample(old.X,old.Y,bottom);
        if(GetWorld()->GetTimeSeconds()>NoticeUntil)
            Notify(Local(TEXT("Rand der vermessenen Flaeche erreicht"),TEXT("Edge of surveyed lakebed reached")));
    }
    const double nextSurface=Environment->SurfaceZ(position.X,position.Y,now+Step);
    if(position.Z>nextSurface+35) { position.Z=nextSurface+35; VelocityMetres.Z=FMath::Min(0.,VelocityMetres.Z); }
    position.Z=FMath::Max(position.Z,static_cast<double>(bottom+75));
    SetActorLocation(position);
    const double previousDepth=Environment->PressureDepth(old,now);
    const double nextDepth=Environment->PressureDepth(position,now+Step);
    Simulation.Step(Step,nextDepth,effort,inflate,(previousDepth-nextDepth)/Step,(position-old).Size()/100,bLamp);
    const auto& s=Simulation.Data;
    const FVector eye=Camera->GetComponentLocation();
    const double eyeDepth=(Environment->SurfaceZ(eye.X,eye.Y,s.ElapsedSeconds)-eye.Z)/100;
    if(Audio->Advance(Step,s,PendingConfig,eyeDepth)&&eyeDepth>0)
        Environment->EmitBreath(position,Camera->GetForwardVector(),s.AmbientBar);
    TelemetryAccumulator+=Step;
    if(TelemetryAccumulator>=1)
    {
        Samples.Add({s,position}); TelemetryAccumulator-=1;
        if(Samples.Num()%30==0) SaveLog(TEXT("in_progress"));
    }
    if(s.SpO2<60 && PendingConfig.GameMode!=Dive::Mode::Exploration)
        FinishDive(TEXT("out_of_gas"));
}

void ADivePawn::Notify(const FString& Message)
{ Notice=Message; NoticeUntil=GetWorld()->GetTimeSeconds()+8; }

void ADivePawn::Interact()
{
    if(bMenu || !bDiveActive) return;
    if(Simulation.Data.DepthMetres<=.8 && FVector::Dist2D(GetActorLocation(),EntryLocation)<1200)
        FinishDive(TEXT("completed"));
    else Notify(Local(TEXT("Zum Beenden an der Oberflaeche zur Einstiegsboje zurueckkehren."),TEXT("Return to the entry buoy at the surface to finish your dive.")));
}

void ADivePawn::FinishDive(const FString& Reason)
{
    if(!bDiveActive) return;
    Samples.Add({Simulation.Data,GetActorLocation()});
    Outcome=Reason; bDiveActive=false; bSummary=true;
    if(!SaveLog(Reason)) Outcome+=TEXT(" / log_save_failed");
    SetMenu(true);
}

void ADivePawn::EndPlay(const EEndPlayReason::Type Reason)
{
    if(bDiveActive) SaveLog(TEXT("interrupted"));
    Super::EndPlay(Reason);
}

void ADivePawn::Photograph()
{
    if(bMenu) return;
    const FString dir=FPaths::ProjectSavedDir()/TEXT("Photographs");
    IFileManager::Get().MakeDirectory(*dir,true);
    const FString path=dir/(FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT(".png"));
    FScreenshotRequest::RequestScreenshot(path,false,false);
    Notify(Local(TEXT("Foto gespeichert: Saved/Photographs"),TEXT("Photo saved: Saved/Photographs")));
}

bool ADivePawn::SaveLog(const FString& Reason)
{
    const FString dir=FPaths::ProjectSavedDir()/TEXT("DiveLogs");
    IFileManager::Get().MakeDirectory(*dir,true);
    if(LastLogPath.IsEmpty()) LastLogPath=dir/(FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)+TEXT(".json"));
    auto root=MakeShared<FJsonObject>();
    root->SetNumberField(TEXT("schema_version"),2);
    root->SetStringField(TEXT("started_at_utc"),DiveStartedUTC);
    root->SetBoolField(TEXT("is_test"),bSmokeTest);
    root->SetStringField(TEXT("status"),Reason);
    root->SetStringField(TEXT("location"),Environment->Terrain.Entries[EntryIndex].Name);
    root->SetStringField(TEXT("bathymetry"),TEXT("swissBATHY3D Walensee 2020; (c) swisstopo"));
    root->SetNumberField(TEXT("water_elevation_ln02_m"),419);
    root->SetNumberField(TEXT("duration_seconds"),Simulation.Data.ElapsedSeconds);
    root->SetNumberField(TEXT("maximum_depth_m"),Simulation.Data.MaxDepthMetres);
    root->SetNumberField(TEXT("distance_m"),Simulation.Data.DistanceMetres);
    root->SetNumberField(TEXT("cylinder_litres"),PendingConfig.CylinderLitres);
    root->SetNumberField(TEXT("oxygen_fraction"),PendingConfig.OxygenFraction);
    root->SetNumberField(TEXT("mode"),static_cast<int>(PendingConfig.GameMode));
    auto environment=MakeShared<FJsonObject>(); SaveEnvironment(environment,PendingConfig);
    root->SetObjectField(TEXT("environment"),environment);
    TArray<TSharedPtr<FJsonValue>> rows;
    for(const auto& sample:Samples)
    {
        const auto& s=sample.State; auto row=MakeShared<FJsonObject>();
        row->SetNumberField(TEXT("seconds"),s.ElapsedSeconds);
        row->SetNumberField(TEXT("depth_m"),s.DepthMetres);
        row->SetNumberField(TEXT("cylinder_bar"),s.CylinderBar);
        row->SetNumberField(TEXT("heart_bpm"),s.HeartBPM);
        row->SetNumberField(TEXT("respiratory_bpm"),s.RespiratoryBPM);
        row->SetNumberField(TEXT("spo2_percent"),s.SpO2);
        row->SetNumberField(TEXT("water_c"),s.WaterTemperatureC);
        row->SetNumberField(TEXT("core_c"),s.CoreTemperatureC);
        row->SetNumberField(TEXT("stress"),s.Stress);
        row->SetNumberField(TEXT("fatigue"),s.Fatigue);
        row->SetNumberField(TEXT("ppo2_bar"),s.PPO2);
        row->SetNumberField(TEXT("ceiling_m"),s.CeilingMetres);
        row->SetNumberField(TEXT("ndl_minutes"),s.NDLMinutes);
        row->SetNumberField(TEXT("distance_m"),s.DistanceMetres);
        row->SetNumberField(TEXT("warnings"),s.Warnings);
        row->SetNumberField(TEXT("x_m"),sample.Position.X/100);
        row->SetNumberField(TEXT("y_m"),sample.Position.Y/100);
        const FVector2D survey=Environment->Terrain.ToSurvey(sample.Position);
        row->SetNumberField(TEXT("lv95_east_m"),survey.X);
        row->SetNumberField(TEXT("lv95_north_m"),survey.Y);
        rows.Add(MakeShared<FJsonValueObject>(row));
    }
    root->SetArrayField(TEXT("samples"),rows);
    FString json; auto writer=TJsonWriterFactory<>::Create(&json);
    if(!FJsonSerializer::Serialize(root,writer)) return false;
    const FString temp=LastLogPath+TEXT(".tmp");
    if(!FFileHelper::SaveStringToFile(json,*temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) return false;
    return IFileManager::Get().Move(*LastLogPath,*temp,true,true);
}

void ADivePawn::SaveSettings()
{
    auto obj=MakeShared<FJsonObject>();
    obj->SetNumberField(TEXT("schema_version"),1);
    SaveEnvironment(obj,PendingConfig);
    obj->SetNumberField(TEXT("cylinder_litres"),PendingConfig.CylinderLitres);
    obj->SetNumberField(TEXT("pressure_bar"),PendingConfig.StartPressureBar);
    obj->SetNumberField(TEXT("weight_kg"),PendingConfig.WeightKg);
    obj->SetNumberField(TEXT("visibility_m"),PendingConfig.VisibilityMetres);
    obj->SetNumberField(TEXT("current_mps"),PendingConfig.CurrentMetresPerSecond);
    obj->SetNumberField(TEXT("hour"),PendingConfig.Hour);
    obj->SetNumberField(TEXT("planned_depth_m"),PendingConfig.PlannedDepthMetres);
    obj->SetNumberField(TEXT("planned_minutes"),PendingConfig.PlannedMinutes);
    obj->SetNumberField(TEXT("mode"),static_cast<int>(PendingConfig.GameMode));
    obj->SetBoolField(TEXT("dry_suit"),PendingConfig.DrySuit);
    obj->SetBoolField(TEXT("english"),bEnglish);
    obj->SetBoolField(TEXT("camera_motion"),bCameraMotion);
    obj->SetNumberField(TEXT("entry"),EntryIndex);
    AudioLevels.Sanitize();
    for(int i=0;i<5;++i) obj->SetNumberField(FString::Printf(TEXT("audio_%d"),i),AudioLevels.Values[i]);
    FString json; auto writer=TJsonWriterFactory<>::Create(&json);
    FJsonSerializer::Serialize(obj,writer);
    FFileHelper::SaveStringToFile(json,*(FPaths::ProjectSavedDir()/(bSmokeTest?TEXT("DiveSettingsTest.json"):TEXT("DiveSettings.json"))));
}

void ADivePawn::LoadSettings()
{
    FString json; if(!FFileHelper::LoadFileToString(json,*(FPaths::ProjectSavedDir()/(bSmokeTest?TEXT("DiveSettingsTest.json"):TEXT("DiveSettings.json"))))) return;
    TSharedPtr<FJsonObject> obj; if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),obj)||!obj) return;
    obj->TryGetNumberField(TEXT("weather"),PendingConfig.Weather);
    obj->TryGetNumberField(TEXT("wind_mps"),PendingConfig.WindMetresPerSecond);
    obj->TryGetNumberField(TEXT("wind_direction_degrees"),PendingConfig.WindDirectionDegrees);
    obj->TryGetNumberField(TEXT("surface_temperature_c"),PendingConfig.SurfaceTemperatureC);
    obj->TryGetNumberField(TEXT("bottom_temperature_c"),PendingConfig.BottomTemperatureC);
    obj->TryGetNumberField(TEXT("thermocline_m"),PendingConfig.ThermoclineMetres);
    obj->TryGetNumberField(TEXT("thermocline_width_m"),PendingConfig.ThermoclineWidthMetres);
    obj->TryGetNumberField(TEXT("cylinder_litres"),PendingConfig.CylinderLitres);
    obj->TryGetNumberField(TEXT("pressure_bar"),PendingConfig.StartPressureBar);
    obj->TryGetNumberField(TEXT("weight_kg"),PendingConfig.WeightKg);
    obj->TryGetNumberField(TEXT("visibility_m"),PendingConfig.VisibilityMetres);
    obj->TryGetNumberField(TEXT("current_mps"),PendingConfig.CurrentMetresPerSecond);
    obj->TryGetNumberField(TEXT("hour"),PendingConfig.Hour);
    obj->TryGetNumberField(TEXT("planned_depth_m"),PendingConfig.PlannedDepthMetres);
    obj->TryGetNumberField(TEXT("planned_minutes"),PendingConfig.PlannedMinutes);
    int mode=1; obj->TryGetNumberField(TEXT("mode"),mode); PendingConfig.GameMode=static_cast<Dive::Mode>(FMath::Clamp(mode,0,2));
    obj->TryGetBoolField(TEXT("dry_suit"),PendingConfig.DrySuit);
    obj->TryGetBoolField(TEXT("english"),bEnglish);
    obj->TryGetBoolField(TEXT("camera_motion"),bCameraMotion);
    obj->TryGetNumberField(TEXT("entry"),EntryIndex); EntryIndex=FMath::Clamp(EntryIndex,0,2);
    PendingConfig.Sanitize();
    for(int i=0;i<5;++i) obj->TryGetNumberField(FString::Printf(TEXT("audio_%d"),i),AudioLevels.Values[i]);
    AudioLevels.Sanitize();
}
