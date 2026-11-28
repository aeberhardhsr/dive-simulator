#include "DivePawn.h"
#include "DiveBuildings.h"
#include "DiveBodyFrame.h"
#include "DiveWorld.h"
#include "DiveEquipment.h"
#include "DiveAudio.h"
#include "AudioMixerBlueprintLibrary.h"
#include "InputKeyEventArgs.h"
#include "DiveHUD.h"
#include "DiveConfigIO.h"
#include "Simulation/ShoreMovement.h"
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
#include "Materials/Material.h"

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
    Lamp->SetIntensityUnits(ELightUnits::Lumens);
    Lamp->SetIntensity(1000); Lamp->SetAttenuationRadius(2400);
    Lamp->SetInnerConeAngle(12); Lamp->SetOuterConeAngle(27);
    Lamp->SetLightColor(FLinearColor(.8f,.92f,1));
    Lamp->SetCastShadows(true);
    Lamp->bUseInverseSquaredFalloff=true;
    Lamp->SetCastVolumetricShadow(true);
    Lamp->SetVolumetricScatteringIntensity(1);
    Lamp->SetLightFunctionFadeDistance(100000);
    Audio=CreateDefaultSubobject<UDiveAudio>(TEXT("DiveAudio"));
    AutoPossessPlayer=EAutoReceiveInput::Player0;
}

void ADivePawn::BeginPlay()
{
    Super::BeginPlay();
    bSmokeTest=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"));
    if(!bSmokeTest) { PendingConfig.StartOnShore=true; LoadSettings(); }
    if(bSmokeTest)
    {
        FParse::Value(FCommandLine::Get(),TEXT("DiveEntry="),EntryIndex);
        FParse::Value(FCommandLine::Get(),TEXT("DiveScenario="),SmokeScenario);
        bPerformance=SmokeScenario.StartsWith(TEXT("perf-"));
        if(SmokeScenario==TEXT("loadout")||SmokeScenario==TEXT("avatar"))
        {
            FParse::Value(FCommandLine::Get(),TEXT("DiveLoadoutCase="),LoadoutTestCase);
            if(LoadoutTestCase<0||LoadoutTestCase>(SmokeScenario==TEXT("avatar")?5:3)) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
            PendingConfig.WindMetresPerSecond=PendingConfig.CurrentMetresPerSecond=0;
        }
        if(SmokeScenario==TEXT("modes"))
        {
            FParse::Value(FCommandLine::Get(),TEXT("DiveModeCase="),ModeSmokeCase);
            if(ModeSmokeCase!=TEXT("exploration")&&ModeSmokeCase!=TEXT("manual")&&ModeSmokeCase!=TEXT("simulation")&&
                ModeSmokeCase!=TEXT("hardcore")&&ModeSmokeCase!=TEXT("warm")&&ModeSmokeCase!=TEXT("hypoxia"))
            { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL unknown mode fixture")); FPlatformMisc::RequestExitWithStatus(false,1);return; }
            PendingConfig.GameMode=(ModeSmokeCase==TEXT("exploration")||ModeSmokeCase==TEXT("manual"))?Dive::Mode::Exploration:
                (ModeSmokeCase==TEXT("hardcore")||ModeSmokeCase==TEXT("warm"))?Dive::Mode::Hardcore:Dive::Mode::Simulation;
            PendingConfig.ManualExplorationBuoyancy=ModeSmokeCase==TEXT("manual");
            PendingConfig.SurfaceTemperatureC=PendingConfig.BottomTemperatureC=ModeSmokeCase==TEXT("warm")?20:1;
            PendingConfig.CylinderLitres=30;PendingConfig.StartPressureBar=300;
            PendingConfig.CurrentMetresPerSecond=PendingConfig.WindMetresPerSecond=0;
            PendingConfig.PlannedDepthMetres=60;PendingConfig.PlannedMinutes=120;PendingConfig.DrySuit=false;
        }
        if(bPerformance)
        {
            PendingConfig.GameMode=Dive::Mode::Exploration;
            PendingConfig.StartOnShore=SmokeScenario==TEXT("perf-shore");
        }
        if(SmokeScenario==TEXT("stream")) PendingConfig.GameMode=Dive::Mode::Exploration;
        if(SmokeScenario==TEXT("coast"))
        {
            PendingConfig.StartOnShore=false;PendingConfig.GameMode=Dive::Mode::Exploration;
            PendingConfig.CurrentMetresPerSecond=PendingConfig.WindMetresPerSecond=0;
        }
        if(SmokeScenario==TEXT("sediment")) { PendingConfig.CurrentMetresPerSecond=.04;PendingConfig.WindMetresPerSecond=0; }
        if(SmokeScenario==TEXT("wrist-night")) PendingConfig.Hour=23;
        if(SmokeScenario==TEXT("torch"))
        { PendingConfig.Hour=23; PendingConfig.Date={2026,12,23}; PendingConfig.CurrentMetresPerSecond=0; PendingConfig.WindMetresPerSecond=0; }
        if(SmokeScenario==TEXT("daycycle")) { PendingConfig.Hour=17+59./60; PendingConfig.WindMetresPerSecond=0; }
        FParse::Value(FCommandLine::Get(),TEXT("DiveHour="),PendingConfig.Hour);
        FParse::Value(FCommandLine::Get(),TEXT("DiveTorch="),PendingConfig.TorchModel);
        FParse::Value(FCommandLine::Get(),TEXT("DiveVisibility="),PendingConfig.VisibilityMetres);
        if(PendingConfig.TorchModel<0||PendingConfig.TorchModel>=Dive::Torch::Count)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL invalid DiveTorch")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        FString scenarioDate;
        if(FParse::Value(FCommandLine::Get(),TEXT("DiveDate="),scenarioDate)&&!Dive::CivilDate::Parse(TCHAR_TO_UTF8(*scenarioDate),PendingConfig.Date))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL invalid DiveDate")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        double scenarioUTC=0;
        if(!Dive::ScenarioClock::StartUTC(PendingConfig.Date,PendingConfig.Hour,scenarioUTC))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL invalid/nonexistent scenario start time")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        if(SmokeScenario==TEXT("surface-calm")) PendingConfig.WindMetresPerSecond=0;
        if(SmokeScenario==TEXT("surface-rain")) { PendingConfig.WindMetresPerSecond=12; PendingConfig.Weather=2; }
        if(SmokeScenario==TEXT("surface-storm")) PendingConfig.SelectWeather(3);
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveAutomaticEnvironment")))
        { PendingConfig.AutoTemperature=PendingConfig.AutoVisibility=true;PendingConfig.CurrentSource=Dive::CurrentSetting::Automatic; }
        if(SmokeScenario==TEXT("audio")) { PendingConfig.WindMetresPerSecond=9; PendingConfig.Weather=2; }
        if(SmokeScenario==TEXT("shore")||SmokeScenario==TEXT("land-detail")) { PendingConfig.StartOnShore=true; PendingConfig.WindMetresPerSecond=0; PendingConfig.CurrentMetresPerSecond=0; }
        if(SmokeScenario==TEXT("tutorial"))
        {
            PendingConfig.GameMode=Dive::Mode::Exploration; PendingConfig.CylinderLitres=15; PendingConfig.StartPressureBar=250;
            PendingConfig.DrySuit=false; PendingConfig.WeightKg=10; PendingConfig.CurrentMetresPerSecond=.16;
            PendingConfig.Weather=2; PendingConfig.WindMetresPerSecond=6; EntryIndex=2;
        }
    }
    Environment=GetWorld()->SpawnActor<ADiveWorld>();
    Environment->Diver=this;
    Environment->AddTickPrerequisiteActor(this);
    Equipment=GetWorld()->SpawnActor<ADiveEquipment>(); Equipment->SetOwner(this); Equipment->Initialize(this);
    if(Environment->bTerrainReady)
    {
        EntryIndex=FMath::Clamp(EntryIndex,0,Environment->Terrain.Entries.Num()-1);
        Environment->Configure(PendingConfig);
        Environment->SetEntry(EntryIndex,PendingConfig.StartOnShore);
        EntryLocation=PendingConfig.StartOnShore?
            Environment->ShoreStart(EntryIndex)+FVector(0,0,Dive::ShoreMovement::EyeOffset*100):
            Environment->Terrain.Entries[EntryIndex].Position;
    }
    SetActorLocation(EntryLocation);
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        PC->SetControlRotation(FRotator(-8,0,0));
        PC->PlayerCameraManager->ViewPitchMin=-85;
        PC->PlayerCameraManager->ViewPitchMax=85;
    }
    Audio->Initialize(Camera); Audio->ApplyLevels(AudioLevels);
    Camera->PostProcessSettings.bOverride_LumenFrontLayerTranslucencyReflections=true;
    Camera->PostProcessSettings.LumenFrontLayerTranslucencyReflections=Environment->WaterInterfaceEnabled();
    SetMenu(true);
    if(auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_Underwater.M_Underwater")))
    {
        if(bSmokeTest)
        {
            TArray<FMaterialParameterInfo> parameters; TArray<FGuid> ids;
            material->GetAllScalarParameterInfo(parameters,ids);
            const bool ambient=parameters.ContainsByPredicate([](const FMaterialParameterInfo& info) { return info.Name==TEXT("AmbientLight"); });
            const bool native=parameters.ContainsByPredicate([](const FMaterialParameterInfo& info) { return info.Name==TEXT("NativeScattering"); });
            if(!ambient||!native||material->GetMaterial()->BlendableLocation!=BL_SceneColorBeforeDOF||material->GetMaterial()->bDisablePreExposureScale)
            { UE_LOG(LogTemp,Error,TEXT("DIVE_OPTICS_FAIL Underwater material is stale or has the wrong HDR stage")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
            UE_LOG(LogTemp,Display,TEXT("DIVE_OPTICS_PASS stage=BeforeDOF ambient_parameter=1"));
        }
        UnderwaterMaterial=UMaterialInstanceDynamic::Create(material,this);
    }
    if(bSmokeTest)
    {
        if(Environment->bTerrainReady)
        {
            if(SmokeScenario==TEXT("shore-preview"))
            {
                const auto& shore=Environment->Terrain.ShoreEntries[EntryIndex];
                SetActorLocation(Environment->ShoreStart(EntryIndex)+FVector(0,0,80)); bShowInstruments=false; SetMenu(false);
                const bool lookBack=FParse::Param(FCommandLine::Get(),TEXT("DiveShoreLookBack"));
                if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(FRotator(lookBack?0:-12,shore.Yaw+(lookBack?180:0),0));
                if(FParse::Param(FCommandLine::Get(),TEXT("DiveGroundView")))
                    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(FRotator(-60,shore.Yaw+(lookBack?180:0),0));
#if WITH_EDITOR
                if(FParse::Param(FCommandLine::Get(),TEXT("DiveLandscapeDetailView")))
                {
                    double ground=0;
                    if(!Environment->Terrain.Landscape.SampleTriangle(2733236,1223622,ground))
                    { FPlatformMisc::RequestExitWithStatus(false,1);return; }
                    FVector eye=Environment->Terrain.ToWorld(2733236,1223622);eye.Z=ground*100+176;
                    SetActorLocation(eye);
                    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(FRotator(-15,90,0));
                    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_DETAIL_VIEW position=%s yaw=90 pitch=-15"),*eye.ToString());
                }
#endif
                if(FParse::Param(FCommandLine::Get(),TEXT("DiveBuildingsView")))
                {
                    TArray<FDiveBuildingTile> tiles;int32 count=0;FString error;
                    if(!DiveBuildings::Read(tiles,count,error)) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
                    FVector target;double nearest=TNumericLimits<double>::Max();
                    for(const auto& tile:tiles)
                    {
                        const FVector roof=tile.Origin+tile.Probe;
                        const double distance=FVector::DistSquaredXY(roof,Environment->ShoreStart(EntryIndex));
                        if(distance<nearest) { nearest=distance;target=roof; }
                    }
                    const FVector eye=target+FVector(-9000,-9000,6000);
                    SetActorLocation(eye);
                    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation((target-eye).Rotation());
                    UE_LOG(LogTemp,Display,TEXT("DIVE_BUILDINGS_INSPECTION camera=%s roof=%s"),*eye.ToString(),*target.ToString());
                }
            }
            else if(SmokeScenario==TEXT("tutorial")) StartTutorial();
            else if(SmokeScenario!=TEXT("ui")&&SmokeScenario!=TEXT("discoveries")&&SmokeScenario!=TEXT("waypoints")&&SmokeScenario!=TEXT("navigation")&&SmokeScenario!=TEXT("profiles")&&SmokeScenario!=TEXT("config")&&SmokeScenario!=TEXT("environment")&&SmokeScenario!=TEXT("loadout")&&SmokeScenario!=TEXT("avatar")) StartDive();
            if(bPerformance) SetupPerformance();
            if(FParse::Param(FCommandLine::Get(),TEXT("DiveLampOff"))) bLamp=false;
            double battery=100;FParse::Value(FCommandLine::Get(),TEXT("DiveTorchBattery="),battery);
            Simulation.Data.LampBattery=FMath::Clamp(battery,0.,100.);
        }
        else { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL missing terrain")); FPlatformMisc::RequestExitWithStatus(false,1); }
    }
}

void ADivePawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    // The player controller filters real input during unattended tests.
    Input->BindAxisKey(EKeys::MouseX,this,&ADivePawn::LookYaw);
    Input->BindAxisKey(EKeys::MouseY,this,&ADivePawn::LookPitch);
}

void ADivePawn::LookYaw(float Value)
{ if(!bMenu) AddControllerYawInput(Value*MouseSensitivity); }
void ADivePawn::LookPitch(float Value)
{ if(!bMenu) AddControllerPitchInput(Value*MouseSensitivity*(Controls.InvertY?1:-1)); }
void ADivePawn::ToggleLamp() { if(!bMenu) bLamp=!bLamp; }
void ADivePawn::ToggleHUD() { if(!bMenu&&!Simulation.Settings.Rules().PermanentHUD) bShowInstruments=!bShowInstruments; }
void ADivePawn::ToggleComputer() { if(!bMenu) bShowComputer=!bShowComputer; }
void ADivePawn::NextComputerPage() { if(!bMenu&&bShowComputer) ComputerPage=(ComputerPage+1)%4; }
void ADivePawn::ToggleMap() { if(!bMenu) bShowMap=!bShowMap; }
void ADivePawn::ToggleMenu()
{
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->DiscoveriesOpen()) { hud->CloseDiscoveries();return; }
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->WaypointsOpen()) { hud->CloseWaypoints();return; }
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->ConfigInputOpen()) { hud->CloseConfigInput(); return; }
    if(CapturingAction!=INDEX_NONE) { CapturingAction=INDEX_NONE; ControlMessage=Local(TEXT("Belegung unverändert."),TEXT("Binding unchanged.")); return; }
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->ProfilesOpen()) { hud->CloseProfiles(); return; }
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->ControlsOpen()) { hud->CloseControls(); return; }
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->EntryMapOpen()) { hud->CloseEntryMap(); return; }
    if(bLogbook) { bLogbook=false; return; }
    if(bMenu) if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD());hud&&hud->BackToDivePlan()) return;
    SetMenu(!bMenu || !bDiveActive);
}
void ADivePawn::ResumeDive() { if(bDiveActive) SetMenu(false); }

void ADivePawn::SetMenu(bool Open)
{
    if(Open&&!bMenu&&bDiveActive)
        if(auto* pc=Cast<APlayerController>(GetController()))
            if(auto* hud=Cast<ADiveHUD>(pc->GetHUD())) hud->BackToDivePlan();
    bMenu=Open;
    CapturingAction=INDEX_NONE;
    if(auto* PC=Cast<APlayerController>(GetController()))
    {
        PC->bShowMouseCursor=Open;
        PC->bEnableClickEvents=Open;
        if(Open)
        {
            PC->FlushPressedKeys();
            TestHeldKeys.Empty();
            FInputModeGameAndUI Mode;
            Mode.SetHideCursorDuringCapture(false);
            Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
            PC->SetInputMode(Mode);
        }
        else PC->SetInputMode(FInputModeGameOnly());
    }
    Audio->SetPaused(Open);
}

void ADivePawn::ApplyControls()
{
    Controls.Sanitize(); MouseSensitivity=Controls.Sensitivity;
    Camera->SetFieldOfView(Controls.FieldOfView);
    if(auto* PC=Cast<APlayerController>(GetController())) PC->FlushPressedKeys();
}
void ADivePawn::CaptureKey(FKey Key)
{
    if(Key==EKeys::Escape) { ToggleMenu(); return; }
    const int changed=CapturingAction;
    const int swapped=Controls.Assign(changed,Key);
    if(swapped==-2)
    { ControlMessage=Local(TEXT("Diese Taste ist reserviert oder nicht unterstuetzt."),TEXT("This key is reserved or unsupported.")); return; }
    CapturingAction=INDEX_NONE;
    ControlMessage=swapped>=0?Local(TEXT("Getauscht mit: "),TEXT("Swapped with: "))+FDiveControls::Label(swapped,bEnglish):Local(TEXT("Neue Belegung gespeichert."),TEXT("New binding saved."));
    ApplyControls(); SaveSettings();
}
void ADivePawn::PerformAction(EDiveAction Action)
{
    switch(Action)
    {
    case EDiveAction::Lamp: ToggleLamp(); break;
    case EDiveAction::HUD: ToggleHUD(); break;
    case EDiveAction::Computer: ToggleComputer(); break;
    case EDiveAction::ComputerPage: NextComputerPage(); break;
    case EDiveAction::Map: ToggleMap(); break;
    case EDiveAction::Waypoints:
        if(!bMenu) if(auto* pc=Cast<APlayerController>(GetController()))
            if(auto* hud=Cast<ADiveHUD>(pc->GetHUD())) hud->OpenWaypoints();
        break;
    case EDiveAction::Finish: Interact(); break;
    case EDiveAction::Photo: Photograph(); break;
    case EDiveAction::Regulator:
        if(bDiveActive&&!bMenu)
        {
            bRegulatorInMouth=!bRegulatorInMouth;
            Notify(bRegulatorInMouth?Local(TEXT("Atemregler eingesetzt"),TEXT("Regulator in mouth")):
                Local(TEXT("Atemregler abgesetzt - unter Wasser wieder einsetzen"),TEXT("Regulator removed - replace before submerging")));
        }
        break;
    default: break;
    }
}
void ADivePawn::StartTutorial()
{
    if(bDiveActive||!Environment||!Environment->bTerrainReady) return;
    BeforeTutorialConfig=PendingConfig; BeforeTutorialEntry=EntryIndex;
    PendingConfig=Dive::Config{}; PendingConfig.WindMetresPerSecond=0;
    PendingConfig.CurrentMetresPerSecond=0; PendingConfig.VisibilityMetres=18;
    PendingConfig.PlannedDepthMetres=10; EntryIndex=0;
    bTutorialActive=true; Tutorial=Dive::Tutorial{};
    StartDive();
}

void ADivePawn::StartDive()
{
    if(PhotoPending()||!Environment || !Environment->bTerrainReady) return;
    if(!bTutorialActive) Tutorial=Dive::Tutorial{};
    PendingConfig.Sanitize();
    Simulation.Reset(PendingConfig);
    Environment->Configure(PendingConfig);
    Environment->SetEntry(EntryIndex,PendingConfig.StartOnShore);
    const auto& entry=Environment->Terrain.EntryList(PendingConfig.StartOnShore)[EntryIndex];
    EntryLocation=PendingConfig.StartOnShore?Environment->ShoreStart(EntryIndex):entry.Position;
    if(bSmokeTest&&SmokeScenario==TEXT("torch"))
    {
        bool found=false;
        for(int i=0;i<100;++i)
        {
            const FVector candidate=entry.Position+FRotator(0,entry.Yaw,0).Vector()*(i*100);
            float bed=0;
            if(Environment->Terrain.Sample(candidate.X,candidate.Y,bed)&&bed<-700&&bed>-6000)
            { EntryLocation=candidate;EntryLocation.Z=bed+300;found=true;break; }
        }
        if(!found) { UE_LOG(LogTemp,Error,TEXT("DIVE_SMOKE_FAIL no submerged torch test location")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    }
    Body->SetCapsuleHalfHeight(PendingConfig.StartOnShore?Dive::ShoreMovement::HalfHeight*100:65);
    CameraBaseHeight=PendingConfig.StartOnShore?Dive::ShoreMovement::EyeOffset*100:0;
    Camera->SetRelativeLocation(FVector(0,0,CameraBaseHeight));
    bWalking=PendingConfig.StartOnShore; bEnteredWater=false;
    SetActorLocation(EntryLocation); VelocityMetres=FVector::ZeroVector;
    if(auto* PC=Cast<APlayerController>(GetController())) PC->SetControlRotation(FRotator(bSmokeTest&&SmokeScenario==TEXT("torch")?-55:-8,entry.Yaw,0));
    // Model::Reset prepares the selected loadout at the surface. Entry depth
    // compresses that same gas; ballast is never silently neutralized at spawn.
    Photographs.Reset();
    Samples.Reset(); Accumulator=0; TelemetryAccumulator=0;
    Audio->Reset(); Audio->ApplyLevels(AudioLevels);
    bDiveActive=true; bSummary=false; bLamp=true; bShowMap=false; bShowComputer=false; ComputerPage=0;
    bCameraExposureInitialized=false;
    bRegulatorInMouth=!PendingConfig.StartOnShore;
    ResetContactPose();
    if(!bContactPoseReady)
    {
        bDiveActive=false;SetMenu(true);
        Notify(Local(TEXT("Am Startpunkt ist zu wenig Platz für Taucher und Ausrüstung. Bitte einen anderen Einstieg wählen."),
            TEXT("The start point has insufficient space for the diver and equipment. Please choose another entry.")));
        if(bSmokeTest) FPlatformMisc::RequestExitWithStatus(false,1);
        return;
    }
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
    NextAutomaticHint=20; FreeflowBubbleClock=0; Notice.Empty(); NoticeUntil=0;
    if(Simulation.Settings.Rules().PermanentHUD) bShowInstruments=true;
    if(Simulation.Settings.Rules().AutomaticHints)
        Notify(Local(TEXT("F1 Steuerung und Hilfe  |  ESC Pause"),TEXT("F1 controls and help  |  ESC pause")));
}

void ADivePawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if WITH_EDITOR
    // Explicit diagnostic only: keep the accepted endpoint pose unchanged
    // while comparing the rendered suit with the same view without it.
    if(bPerformance&&PerformancePhase>=2&&FParse::Param(FCommandLine::Get(),TEXT("DiveFPVProbe")))
    { TickPerformance();return; }
#endif
    if(const auto saved=RecordingWriter.Poll(); saved.IsSet())
    {
        if(saved->bSuccess) ++SoakSaves; else ++SoakSaveErrors;
        if(!saved->bSuccess) Notify(Local(TEXT("Zwischenspeichern fehlgeschlagen. Aktuelle Messpunkte nur im Arbeitsspeicher."),
                     TEXT("Autosave failed. Latest log data remains in memory.")));
    }
    if(bPerformance&&SmokeScenario==TEXT("perf-soak")) SteerSoak(DeltaSeconds);
    if(bPerformance&&SmokeScenario!=TEXT("perf-stream")&&SmokeScenario!=TEXT("perf-soak"))
        if(auto* pc=Cast<APlayerController>(GetController()))
        {
            // Incremental input keeps a blocked request from being added anew
            // as an absolute orbit target on every rendered frame.
            FRotator requested=pc->GetControlRotation();
            double fixedYaw=0;
            if(FParse::Value(FCommandLine::Get(),TEXT("DivePerfFixedYaw="),fixedYaw)&&FMath::IsFinite(fixedYaw)) requested.Yaw=fixedYaw;
            else requested.Yaw+=6*FMath::Min(DeltaSeconds,.25f);
            pc->SetControlRotation(requested);
        }
    if(bSmokeTest&&!bSmokeImageryChecked&&GetWorld()->GetTimeSeconds()>2)
    {
        FString error; const bool valid=Environment&&Environment->ValidateImagery(error);
        bSmokeImageryChecked=true;
        UE_LOG(LogTemp,Display,TEXT("DIVE_IMAGERY_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*error);
        if(!valid) { FPlatformMisc::RequestExitWithStatus(false,1); return; }
    }
    if(bSmokeTest&&SmokeScenario==TEXT("shore-preview"))
    {
        ControlsSmokeClock+=DeltaSeconds;
        float previewSeconds=3.f;
        FParse::Value(FCommandLine::Get(),TEXT("DiveShorePreviewSeconds="),previewSeconds);
        previewSeconds=FMath::Clamp(previewSeconds,3.f,60.f);
        if(ControlsSmokeClock>previewSeconds&&!bSmokeCaptured)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_PREVIEW_CAPTURE warmup=%.3f elapsed=%.3f"),previewSeconds,ControlsSmokeClock);
            const FString suffix=FParse::Param(FCommandLine::Get(),TEXT("DiveLandscapeDetailView"))?TEXT("_Detail"):
                FParse::Param(FCommandLine::Get(),TEXT("DiveBuildingsView"))?TEXT("_Buildings"):
                FParse::Param(FCommandLine::Get(),TEXT("DiveGroundView"))?TEXT("_Ground"):
                FParse::Param(FCommandLine::Get(),TEXT("DiveShoreLookBack"))?TEXT("_Land"):TEXT("");
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/Shore_%d%s.png"),EntryIndex,*suffix),true,false);
            bSmokeCaptured=true;
        }
        if(ControlsSmokeClock>previewSeconds+2)
        {
            FString error; bool valid=Environment->ValidateShoreline(error);
            FCollisionQueryParams query(SCENE_QUERY_STAT(ShoreCollision),true,this);
            for(const auto& entry:Environment->Terrain.ShoreEntries)
            {
                FHitResult hit; float ground=0; Environment->SupportZ(entry.Position.X,entry.Position.Y,ground);
                valid&=GetWorld()->LineTraceSingleByChannel(hit,entry.Position+FVector(0,0,300),entry.Position-FVector(0,0,500),ECC_Visibility,query);
                valid&=FMath::Abs(hit.ImpactPoint.Z-ground)<5;
                UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_COLLISION name=%s measured_z=%.2f hit_z=%.2f"),*entry.Name,ground,hit.ImpactPoint.Z);
            }
            UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_%s entry=%d error=%s"),valid?TEXT("PASS"):TEXT("FAIL"),EntryIndex,*error);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SMOKE_%s shore_preview=1"),valid?TEXT("PASS"):TEXT("FAIL"));
            FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
        }
    }
    if(bSmokeTest&&(SmokeScenario==TEXT("ui")||SmokeScenario==TEXT("land-detail")||SmokeScenario==TEXT("coast")||SmokeScenario==TEXT("discoveries")||SmokeScenario==TEXT("waypoints")||SmokeScenario==TEXT("navigation")||SmokeScenario==TEXT("avatar")||SmokeScenario==TEXT("loadout")||SmokeScenario==TEXT("environment")||SmokeScenario==TEXT("config")||SmokeScenario==TEXT("controls")||SmokeScenario==TEXT("tutorial")||SmokeScenario==TEXT("profiles")||SmokeScenario==TEXT("shore")||SmokeScenario==TEXT("breathing")||SmokeScenario==TEXT("reserve-loss")))
    { InputSmokeTick(DeltaSeconds); if(TutorialFinishedAt>0) return; }
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
    if(bSmokeTest&&SmokeScenario==TEXT("recovery")&&SmokeLogbookStarted==0&&GetWorld()->GetTimeSeconds()>2)
    {
        FString path; FParse::Value(FCommandLine::Get(),TEXT("DiveRecoveryLog="),path);
        const auto recovered=DiveLogbook::Read(path);
        const bool expectedBackup=FParse::Param(FCommandLine::Get(),TEXT("DiveExpectBackup"));
        auto* pc=Cast<APlayerController>(GetController());
        auto* hud=pc?Cast<ADiveHUD>(pc->GetHUD()):nullptr;
        if(!hud||!recovered.Error.IsEmpty()||!recovered.bTest||recovered.Status!=TEXT("in_progress")||
            recovered.bRecovered!=expectedBackup||recovered.Samples.IsEmpty())
        { UE_LOG(LogTemp,Error,TEXT("DIVE_RECOVERY_FAIL cannot read expected checkpoint")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        bDiveActive=false; LastLogPath=path; RecoverySampleCount=recovered.Samples.Num();
        hud->OpenLogbook(path,true); SmokeLogbookStarted=GetWorld()->GetTimeSeconds();
        UE_LOG(LogTemp,Display,TEXT("DIVE_RECOVERY_PASS backup=%d seconds=%.3f samples=%d"),
            recovered.bRecovered,recovered.Duration,RecoverySampleCount);
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
                const int32 expectedSamples=RecoverySampleCount>=0?RecoverySampleCount:Samples.Num();
                const bool valid=HUD->LogbookMatches(LastLogPath,expectedSamples)&&HUD->LogRenderedGroups==15&&SmokeLogbookCaptured==15;
                UE_LOG(LogTemp,Display,TEXT("DIVE_LOGBOOK_%s samples=%d groups=%u"),valid?TEXT("PASS"):TEXT("FAIL"),expectedSamples,HUD->LogRenderedGroups);
                FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
            }
        }
        if(GetWorld()->GetTimeSeconds()-SmokeLogbookStarted>45)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_LOGBOOK_FAIL load timeout")); FPlatformMisc::RequestExitWithStatus(false,1); }
        if(Equipment) Equipment->Update(DeltaSeconds);
        return;
    }
    if(bSmokeTest&&SmokeScenario==TEXT("daycycle")) TickDayCycleTest(DeltaSeconds);
    if(bSmokeTest&&SmokeScenario==TEXT("decompression")) TickDecompressionTest(DeltaSeconds);
    if(bDiveActive && !bMenu && !(bSmokeTest&&(SmokeScenario==TEXT("ui")||SmokeScenario==TEXT("decompression")||SmokeScenario==TEXT("fish")||SmokeScenario==TEXT("rocks"))))
    {
        Accumulator+=FMath::Min(DeltaSeconds,.25f);
        while(Accumulator>=.02f && bDiveActive) { SimulateStep(.02f); Accumulator-=.02f; }
    }
    UpdateTorch(); // Current battery output before diagnostics; attachment is refreshed again after camera motion.
    const double smokeDuration=SmokeScenario==TEXT("sediment")?70:(SmokeScenario.StartsWith(TEXT("surface"))||SmokeScenario==TEXT("audio"))?38:(SmokeScenario==TEXT("stream")||SmokeScenario==TEXT("daycycle"))?90:SmokeScenario==TEXT("breathing")?30:SmokeScenario==TEXT("reserve-loss")?150:22;
    if(bSmokeTest && bDiveActive && SmokeScenario!=TEXT("decompression") && SmokeScenario!=TEXT("fish") && SmokeScenario!=TEXT("rocks"))
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
        if(SmokeScenario.StartsWith(TEXT("surface"))&&Simulation.Data.ElapsedSeconds>24&&FParse::Param(FCommandLine::Get(),TEXT("DiveSkyView")))
        {
            double yaw=180,pitch=45;
            FParse::Value(FCommandLine::Get(),TEXT("DiveSkyYaw="),yaw);
            FParse::Value(FCommandLine::Get(),TEXT("DiveSkyPitch="),pitch);
            if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(FRotator(pitch,yaw,0));
        }
        bSmokeSurfaceReached|=GetActorLocation().Z>Environment->SurfaceZ(GetActorLocation().X,GetActorLocation().Y)+2;
        if(SmokeScenario.StartsWith(TEXT("surface"))&&Simulation.Data.ElapsedSeconds>25&&!bSmokeSecondaryCaptured&&
            GetActorLocation().Z>Environment->SurfaceZ(GetActorLocation().X,GetActorLocation().Y)+8)
        {
            bSmokeSecondaryCaptured=true;
            if(SmokeScenario==TEXT("surface-storm"))
            {
                FString error;
                if(!Environment->ValidateStorm(error))
                { UE_LOG(LogTemp,Error,TEXT("DIVE_WEATHER_FAIL %s"),*error); FPlatformMisc::RequestExitWithStatus(false,1); return; }
            }
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/SurfaceAir_%d_%s.png"),EntryIndex,*SmokeScenario),true,false);
        }
        bSmokeChunkLimit&=Environment->TerrainChunkCount()<=9;
        if(SmokeScenario==TEXT("stream"))
            if(auto* PC=Cast<APlayerController>(GetController()))
                PC->SetControlRotation(FRotator(0,Environment->Terrain.Entries[EntryIndex].Yaw,0));
        if(!bPerformance&&SmokeScenario!=TEXT("land-detail")&&SmokeScenario!=TEXT("coast")&&SmokeScenario!=TEXT("shore")&&SmokeScenario!=TEXT("modes")&&Simulation.Data.ElapsedSeconds>smokeDuration-4) bShowMap=SmokeScenario!=TEXT("torch")&&SmokeScenario!=TEXT("daycycle")&&!SmokeScenario.StartsWith(TEXT("surface"))&&!SmokeScenario.StartsWith(TEXT("wrist"));
        if(SmokeScenario.StartsWith(TEXT("wrist")))
        {
            const double t=Simulation.Data.ElapsedSeconds;
            bShowComputer=t>4; bShowInstruments=false;
            ComputerPage=t<10?0:t<14?1:t<18?2:3;
            if(t>13&&!bSmokeSecondaryCaptured)
            {
                bSmokeSecondaryCaptured=true;
                FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/WristVitals_%s.png"),*SmokeScenario),true,false);
            }
        }
    }
    if(bSmokeTest && !bPerformance && SmokeScenario!=TEXT("rocks") && Simulation.Data.ElapsedSeconds>8 && !bSmokeUnderwaterCaptured)
    {
        FString particleDetail;const bool particlesValid=Environment->ValidateParticles(particleDetail);
        UE_LOG(LogTemp,Display,TEXT("DIVE_PARTICLES_%s %s"),particlesValid?TEXT("PASS"):TEXT("FAIL"),*particleDetail);
        if(!particlesValid) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
        const auto& torch=Dive::Torch::At(PendingConfig.TorchModel);
        if(Lamp->IntensityUnits!=ELightUnits::Lumens||FMath::Abs(Lamp->Intensity-torch.Output(Simulation.Data.LampBattery))>.1f||Lamp->IsVisible()!=(bLamp&&Simulation.Data.LampBattery>0)||
            FMath::Abs(Lamp->OuterConeAngle-torch.OuterDegrees)>.01||!Lamp->bUseInverseSquaredFalloff||!TorchFunction)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_LIGHT_FAIL Torch units/intensity/switch disagree")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        UE_LOG(LogTemp,Display,TEXT("DIVE_TORCH_PASS on=%d lumens=%.1f exposure=%.3f model=%s full_cone=%.1f battery=%.4f"),bLamp,Lamp->Intensity,Camera->PostProcessSettings.AutoExposureBias,UTF8_TO_TCHAR(torch.Id),2*Lamp->OuterConeAngle,Simulation.Data.LampBattery);
        if(SmokeScenario==TEXT("torch"))
        {
            FHitResult hit;FCollisionQueryParams query;query.AddIgnoredActor(this);
            const auto eye=Camera->GetComponentLocation();
            const bool contact=GetWorld()->LineTraceSingleByChannel(hit,eye,eye+Camera->GetForwardVector()*2000,ECC_Visibility,query);
            UE_LOG(LogTemp,Display,TEXT("DIVE_TORCH_TARGET hit=%d distance_m=%.3f eye=%s lamp=%s direction=%s"),contact,hit.Distance/100,*eye.ToString(),*Lamp->GetComponentLocation().ToString(),*Lamp->GetForwardVector().ToString());
            if(!contact||hit.Distance<100) { UE_LOG(LogTemp,Error,TEXT("DIVE_LIGHT_FAIL Torch test has no visible nearby target")); FPlatformMisc::RequestExitWithStatus(false,1);return; }
        }
        bSmokeUnderwaterCaptured=true;
        if(SmokeScenario.StartsWith(TEXT("wrist")))
            Equipment->ExportDisplayForTest(FPaths::ProjectSavedDir()/TEXT("Screenshots"),TEXT("WristTexture_")+SmokeScenario+TEXT(".png"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/DiveUnderwater_%d_%s.png"),EntryIndex,*SmokeScenario),true,false);
    }
    if(bSmokeTest && !bPerformance && SmokeScenario!=TEXT("rocks") && Simulation.Data.ElapsedSeconds>smokeDuration-3 && !bSmokeCaptured)
    {
        bSmokeCaptured=true;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Screenshots/DiveSmoke_%d_%s.png"),EntryIndex,*SmokeScenario),true,false);
    }
    if(bSmokeTest && !bPerformance && SmokeScenario!=TEXT("rocks") && SmokeScenario!=TEXT("ui") && SmokeScenario!=TEXT("decompression") && SmokeScenario!=TEXT("land-detail") && SmokeScenario!=TEXT("coast") && SmokeScenario!=TEXT("avatar") && SmokeScenario!=TEXT("loadout") && SmokeScenario!=TEXT("modes") && SmokeScenario!=TEXT("tutorial") && SmokeScenario!=TEXT("shore") && SmokeScenario!=TEXT("reserve-loss") && bDiveActive && Simulation.Data.ElapsedSeconds>=smokeDuration)
    {
        const double displacement=FVector::Dist2D(GetActorLocation(),EntryLocation)/100;
        const bool extended=SmokeScenario.StartsWith(TEXT("surface"))?bSmokeSurfaceReached:
            SmokeScenario==TEXT("stream")?displacement>80:true;
        const bool wristReady=!SmokeScenario.StartsWith(TEXT("wrist"))||
            (Equipment->WristBlend>.95f&&ComputerPage==3&&Equipment->DisplayUpdates>60&&Equipment->DisplayedPages==15);
        const bool audioReady=SmokeScenario!=TEXT("audio")||(Audio->Inhales>4&&Audio->Exhales>4&&Audio->Heartbeats>30&&Audio->InflationSteps>100&&Audio->VentSteps>100);
        FString particleDetail;const bool particlesReady=Environment->ValidateParticles(particleDetail);
        UE_LOG(LogTemp,Display,TEXT("DIVE_PARTICLES_%s final=1 %s"),particlesReady?TEXT("PASS"):TEXT("FAIL"),*particleDetail);
        const bool valid=particlesReady&&(SmokeScenario!=TEXT("daycycle")||DayCycleSmokePhase==5)&&bControlsSmokeValid && audioReady && extended && wristReady && bSmokeChunkLimit && (SmokeScenario==TEXT("torch")?displacement<.5:Simulation.Data.DistanceMetres>5)
            && (SmokeScenario==TEXT("stream")||Simulation.Data.MaxDepthMetres>2.5)
            && (SmokeScenario==TEXT("stream")||Simulation.Data.CylinderBar<(SmokeScenario==TEXT("torch")?199.9:199))
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
    // This fixture teleports to the surveyed bed and can change menu state.
    // Do it before deriving camera medium uniforms from this frame's eye.
    if(bSmokeTest&&SmokeScenario==TEXT("sediment")) TickSedimentTest(DeltaSeconds);
    Lamp->SetVisibility(bLamp && Simulation.Data.LampBattery>0);
    const double breathMotion=bCameraMotion&&!bMenu?std::sin(Simulation.Data.BreathPhase*2*Dive::Pi)*.8:0;
    Camera->SetRelativeLocation(FVector(0,0,CameraBaseHeight+breathMotion));
    if(bSmokeTest&&SmokeScenario==TEXT("caustics")) TickCausticsTest(DeltaSeconds);
    Camera->SetWorldRotation(GetViewRotation());
    Environment->UpdateDaylight();
    const FVector eye=Camera->GetComponentLocation();
    const float depth=FMath::Max(0.f,static_cast<float>((Environment->SurfaceZ(eye.X,eye.Y)-eye.Z)/100));
    const float underwater=static_cast<float>(Dive::CameraWaterBlend(depth));
    if(UnderwaterMaterial)
    {
        UnderwaterMaterial->SetScalarParameterValue(TEXT("VisibilityCm"),Simulation.Settings.VisibilityMetres*100);
        UnderwaterMaterial->SetScalarParameterValue(TEXT("AmbientLight"),Environment->CameraScatteringAmbientAt(eye));
        UnderwaterMaterial->SetScalarParameterValue(TEXT("NativeScattering"),Environment->NativeWaterScattering()?(Environment->BoundedWaterEnabled()?2.f:1.f):0.f);
        UnderwaterMaterial->SetScalarParameterValue(TEXT("EyeDepthCm"),depth*100);
        // UE blends every scalar against the parent material when its weight
        // is fractional. That also interpolates the discrete scattering mode
        // (2 -> 0), re-enabling analytic absorption over integrated water fog.
        // Keep physical inputs/mode intact and fade radiance inside the shader.
        UnderwaterMaterial->SetScalarParameterValue(TEXT("Immersion"),underwater);
        Camera->AddOrUpdateBlendable(UnderwaterMaterial,underwater>0?1.f:0.f);
    }
    auto& PP=Camera->PostProcessSettings;
    Camera->PostProcessBlendWeight=1;
    PP.bOverride_ColorSaturation=true;
    // Water colour comes from path-dependent absorption/scattering. A second
    // constant green tint also coloured zero-length paths and flattened rocks.
    PP.ColorSaturation=FVector4(1,1,1,1);
    const float impairment=bDiveActive?static_cast<float>(Simulation.Data.VisualImpairment):0.f;
    PP.ColorSaturation.W=1-.8f*impairment;
    PP.bOverride_SceneColorTint=true;
    PP.SceneColorTint=FLinearColor::White;
    PP.bOverride_VignetteIntensity=true; PP.VignetteIntensity=.35f+.6f*impairment;
    PP.bOverride_AutoExposureMethod=true; PP.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
    PP.bOverride_AutoExposureApplyPhysicalCameraExposure=true; PP.AutoExposureApplyPhysicalCameraExposure=true;
    PP.bOverride_CameraISO=true; PP.CameraISO=100;
    PP.bOverride_CameraShutterSpeed=true; PP.CameraShutterSpeed=60;
    PP.bOverride_DepthOfFieldFstop=true; PP.DepthOfFieldFstop=4;
    PP.bOverride_AutoExposureBias=true;
    const float targetExposure=Environment->ExposureBiasAt(eye);
    const bool firstExposure=!bCameraExposureInitialized;
    const bool legacyExposure=bSmokeTest&&FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyExposureStart"));
    const float previousExposure=PP.AutoExposureBias;
    // A newly selected scene has no eye-adaptation history. Initialize from
    // its actual lighting; only continuous gameplay adapts from the last frame.
    PP.AutoExposureBias=bDiveActive&&(!firstExposure||legacyExposure)?FMath::FInterpTo(PP.AutoExposureBias,targetExposure,DeltaSeconds,
        targetExposure<PP.AutoExposureBias?2.f:.5f):targetExposure;
    bCameraExposureInitialized=true;
    // Diagnostic receiver comparisons use one exposure at every depth/time.
    if(bSmokeTest&&SmokeScenario==TEXT("caustics")) PP.AutoExposureBias=-4.6f;
    if(bSmokeTest&&firstExposure)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_EXPOSURE_START legacy=%d active=%d previous=%.6f target=%.6f actual=%.6f depth=%.6f eye=%s gaze=%s lamp=%d"),
            legacyExposure,bDiveActive,previousExposure,targetExposure,PP.AutoExposureBias,depth,*eye.ToString(),*Camera->GetComponentRotation().ToString(),bLamp);
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveExposureProbe")))
            FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/ExposureStart.png"),true,false);
    }
    PP.bOverride_MotionBlurAmount=true; PP.MotionBlurAmount=0;
    PP.bOverride_DepthOfFieldFocalDistance=true; PP.DepthOfFieldFocalDistance=0;
    if(Equipment) Equipment->Update(DeltaSeconds);
    UpdateTorch();
    if(bSmokeTest&&SmokeScenario==TEXT("modes")) TickModeTest();
    if(bPerformance) TickPerformance();
    if(bSmokeTest&&SmokeScenario==TEXT("fish")) TickFishInspection(DeltaSeconds);
    if(bSmokeTest&&SmokeScenario==TEXT("rocks")) TickRockInspection(DeltaSeconds);
}

bool ADivePawn::ValidateCameraMedium(FString& Error) const
{
    if(!Environment||!Camera||!UnderwaterMaterial)
    { Error=TEXT("Missing underwater camera material");return false; }
    if(!Camera->PostProcessSettings.bOverride_LumenFrontLayerTranslucencyReflections||
        Camera->PostProcessSettings.LumenFrontLayerTranslucencyReflections!=Environment->WaterInterfaceEnabled())
    {Error=TEXT("Water interface reflection mode differs from camera settings");return false;}
    const FVector eye=Camera->GetComponentLocation();
    const double depth=FMath::Max(0.,(Environment->SurfaceZ(eye.X,eye.Y)-eye.Z)/100);
    if(!Camera->PostProcessSettings.SceneColorTint.Equals(FLinearColor::White,1.e-5f)||
        FMath::Abs(UnderwaterMaterial->K2_GetScalarParameterValue(TEXT("NativeScattering"))-(Environment->NativeWaterScattering()?(Environment->BoundedWaterEnabled()?2.f:1.f):0.f))>1.e-5||
        FMath::Abs(UnderwaterMaterial->K2_GetScalarParameterValue(TEXT("EyeDepthCm"))-depth*100)>.01||
        FMath::Abs(UnderwaterMaterial->K2_GetScalarParameterValue(TEXT("Immersion"))-Dive::CameraWaterBlend(depth))>1.e-5||
        FMath::Abs(UnderwaterMaterial->K2_GetScalarParameterValue(TEXT("VisibilityCm"))-Simulation.Settings.VisibilityMetres*100)>.01||
        FMath::Abs(UnderwaterMaterial->K2_GetScalarParameterValue(TEXT("AmbientLight"))-Environment->CameraScatteringAmbientAt(eye))>1.e-5)
    { Error=TEXT("Camera water path uniforms disagree with the actual eye/environment");return false; }
    for(const auto& blend:Camera->PostProcessSettings.WeightedBlendables.Array)
        if(blend.Object==UnderwaterMaterial&&FMath::Abs(blend.Weight-(Dive::CameraWaterBlend(depth)>0?1.:0.))<1.e-5)
            return true;
    Error=TEXT("Underwater camera blend does not match waterline immersion");return false;
}

void ADivePawn::SimulateStep(float Step)
{
    auto* PC=Cast<APlayerController>(GetController()); if(!PC) return;
    auto Down=[this,PC](EDiveAction Action)
    {
        if(!bSmokeTest||SmokeScenario==TEXT("land-detail")||SmokeScenario==TEXT("coast")||SmokeScenario==TEXT("navigation")||SmokeScenario==TEXT("environment")||SmokeScenario==TEXT("config")||SmokeScenario==TEXT("controls")||SmokeScenario==TEXT("tutorial")||SmokeScenario==TEXT("shore")) return PC->IsInputKeyDown(Controls.Key(Action))?1.f:0.f;
        const double t=Simulation.Data.ElapsedSeconds;
        if(SmokeScenario==TEXT("avatar")&&ControlsSmokePhase>=16&&ControlsSmokePhase<=18)
            return Action==EDiveAction::Ascend?1.f:0.f;
        if(SmokeScenario==TEXT("loadout"))
        {
            if(Action==EDiveAction::Forward) return 1.f;
            if(Action==EDiveAction::Descend&&t>1&&t<5) return .7f;
            if(Action==EDiveAction::Inflate&&t>6&&t<8) return 1.f;
            return 0.f;
        }
        if(SmokeScenario==TEXT("modes"))
        {
            if(Action==EDiveAction::Forward||Action==EDiveAction::Sprint) return 1.f;
            if(Action==EDiveAction::Descend&&t<8) return .5f;
            return 0.f;
        }
        if(SmokeScenario==TEXT("torch")) return 0.f;
        if(bPerformance)
        {
            if(SmokeScenario==TEXT("perf-soak"))
            {
                if(Action==EDiveAction::Forward) return SoakForward;
                if(Action==EDiveAction::Ascend) return FMath::Max(0.f,SoakVertical);
                if(Action==EDiveAction::Descend) return FMath::Max(0.f,-SoakVertical);
                return 0.f;
            }
            if(SmokeScenario==TEXT("perf-stream")) return Action==EDiveAction::Forward||Action==EDiveAction::Sprint?1.f:0.f;
            if(SmokeScenario==TEXT("perf-sediment")) return Action==EDiveAction::Forward?.6f:Action==EDiveAction::Descend?.2f:0.f;
            if(SmokeScenario==TEXT("perf-surface")) return Action==EDiveAction::Ascend?1.f:0.f;
            if(SmokeScenario==TEXT("perf-underwater")) return Action==EDiveAction::Descend&&t<5?.7f:0.f;
            return 0.f;
        }
        if(SmokeScenario==TEXT("reserve-loss"))
        {
            if(Action==EDiveAction::Forward) return t<7?1.f:0.f;
            if(Action==EDiveAction::Descend) return t<7?.7f:0.f;
            if(Action==EDiveAction::Deflate) return t<4?.3f:0.f;
            return 0.f;
        }
        if(SmokeScenario==TEXT("stream")) return Action==EDiveAction::Forward?1.f:0.f;
        if(SmokeScenario==TEXT("sediment"))
        {
            if(Action==EDiveAction::Forward) return t<16?1.f:0.f;
            if(Action==EDiveAction::Descend) return t<16?.2f:0.f;
            if(Action==EDiveAction::Ascend) return t>=16&&t<19?.6f:0.f;
            return 0.f;
        }
        if((SmokeScenario.StartsWith(TEXT("surface"))||SmokeScenario==TEXT("audio")) && t>16) return Action==EDiveAction::Ascend?1.f:0.f;
        if(Action==EDiveAction::Forward) return t<10?1.f:0.f;
        if(Action==EDiveAction::Descend) return t<7?.7f:0.f;
        if(Action==EDiveAction::Deflate) return t<4?.3f:0.f;
        if(Action==EDiveAction::Inflate) return t>10 && t<14?1.f:0.f;
        if(Action==EDiveAction::Ascend) return t>16?1.f:0.f;
        return 0.f;
    };
    const float forward=Down(EDiveAction::Forward)-Down(EDiveAction::Backward);
    const float side=Down(EDiveAction::Right)-Down(EDiveAction::Left);
    const float vertical=Down(EDiveAction::Ascend)-Down(EDiveAction::Descend);
    const float inflate=Down(EDiveAction::Inflate)-Down(EDiveAction::Deflate);
    const float sprint=Down(EDiveAction::Sprint);
    AdvanceContactPose(Step,FMath::Min(1.f,FVector(forward,side,vertical).Size())*(sprint>0?1.f:.45f));
    const FVector old=GetActorLocation();
    const FRotationMatrix orientation(PhysicalPose.BodyHeadingView());
    FVector wish=orientation.GetUnitAxis(EAxis::X)*forward+orientation.GetUnitAxis(EAxis::Y)*side+FVector::UpVector*vertical;
    wish=wish.GetClampedToMaxSize(1);
    float effort=wish.Size()*(sprint>0?1.f:.45f);
    if(PendingConfig.StartOnShore) MoveAtShore(Step,forward,side,vertical,sprint,wish,effort);
    else
    {
    const FVector current=FVector(0,Simulation.Settings.CurrentMetresPerSecond,0)+
        Environment->WaveVelocity(old,Simulation.Data.ElapsedSeconds);
    const FVector relative=VelocityMetres-current;
    const float fatigue=static_cast<float>(Simulation.Data.MotorCapacity);
    FVector force=wish*SwimForceNewtons*(sprint>0?1.7f:1.f)*fatigue;
    force.Z+=Simulation.Data.BuoyancyNewtons;
    // Optical fixture holds vertical position; production modes always use their
    // own buoyancy rules. Gas/vitals still advance normally in this test.
    if(bSmokeTest&&SmokeScenario==TEXT("torch")) force.Z=0;
    // Gradually lose displaced body volume as the head crosses the surface.
    // Upward swimming can lift the eyes into air, without allowing flight.
    const double now=Simulation.Data.ElapsedSeconds;
    const double surface=Environment->SurfaceZ(old.X,old.Y,now);
    force.Z-=FMath::Clamp((old.Z-surface)/65.0,0.0,1.0)*
        (Simulation.Data.TotalMassKg*Dive::Gravity+Simulation.Data.BuoyancyNewtons);
    force-=relative*relative.Size()*DragCoefficient;
    VelocityMetres+=force/Simulation.Data.TotalMassKg*Step;
    VelocityMetres=VelocityMetres.GetClampedToMaxSize(2.5f);
    FHitResult hit;
    MoveWithContacts(VelocityMetres*100*Step,hit);
    if(bSmokeTest&&SmokeScenario==TEXT("loadout")&&!bLoadoutImpulseChecked)
    {
        const double expected=125./Simulation.Data.TotalMassKg*Step*Step*100*std::cos(8*Dive::Pi/180);
        const double moved=FVector::Dist2D(old,GetActorLocation());
        bLoadoutImpulseChecked=true;bLoadoutImpulseValid=!hit.bBlockingHit&&std::abs(moved-expected)<1.e-5;
        UE_LOG(LogTemp,Display,TEXT("DIVE_LOADOUT_IMPULSE_%s mass=%.6f expected_cm=%.9f actual_cm=%.9f"),
            bLoadoutImpulseValid?TEXT("PASS"):TEXT("FAIL"),Simulation.Data.TotalMassKg,expected,moved);
    }
    FVector position=GetActorLocation();
    float bottom=-30000;
    if(!Environment->Terrain.SampleGround(position.X,position.Y,bottom))
    {
        position.X=old.X; position.Y=old.Y;
        VelocityMetres.X=0; VelocityMetres.Y=0;
        Environment->Terrain.SampleGround(old.X,old.Y,bottom);
        if(GetWorld()->GetTimeSeconds()>NoticeUntil)
            Notify(Local(TEXT("Rand des begehbaren Gelaendes erreicht"),TEXT("Edge of playable terrain reached")));
    }
    const double nextSurface=Environment->SurfaceZ(position.X,position.Y,now+Step);
    if(position.Z>nextSurface+35) { position.Z=nextSurface+35; VelocityMetres.Z=FMath::Min(0.,VelocityMetres.Z); }
    FHitResult correction;MoveWithContacts(position-GetActorLocation(),correction);
    }
    const FVector position=GetActorLocation();
    if(bSmokeTest&&SmokeScenario==TEXT("shore")&&FMath::Abs(position.Z-old.Z)>20)
    {
        UE_LOG(LogTemp,Error,TEXT("DIVE_SHORE_FAIL vertical discontinuity %.2f cm"),position.Z-old.Z);
        bControlsSmokeValid=false;
        FPlatformMisc::RequestExitWithStatus(false,1);
    }
    const FVector offset(0,0,PendingConfig.StartOnShore?Dive::ShoreMovement::EyeOffset*100:0);
    const double now=Simulation.Data.ElapsedSeconds;
    const double previousDepth=Environment->PressureDepth(old+offset,now);
    const double nextDepth=Environment->PressureDepth(position+offset,now+Step);
    const FVector physicalEye=position+offset;
    const double localSurface=Environment->SurfaceZ(physicalEye.X,physicalEye.Y,now+Step);
    Dive::Exposure exposure;
    exposure.BodyImmersion=Dive::ShoreMovement::Immersion(
        physicalEye.Z/100-Dive::ShoreMovement::EyeOffset,localSurface/100);
    const FVector physicalMouth=FDiveBodyFrame::PhysicalMouth(physicalEye,PC->GetControlRotation());
    exposure.MouthSubmerged=physicalMouth.Z<Environment->SurfaceZ(physicalMouth.X,physicalMouth.Y,now+Step);
    exposure.RegulatorInMouth=bRegulatorInMouth;
    Simulation.Step(Step,nextDepth,effort,inflate,(previousDepth-nextDepth)/Step,(position-old).Size()/100,bLamp,exposure);
    Environment->StepSediment(Step);
    const auto& s=Simulation.Data;
    if(Simulation.Settings.Rules().AutomaticHints&&!bTutorialActive&&s.ElapsedSeconds>=NextAutomaticHint)
    {
        FString hint;
        if(s.Warnings&Dive::NoBreathingSupply) hint=Local(TEXT("Atmung unterbrochen. Prüfe den Atemregler und deine Position."),TEXT("Breathing interrupted. Check the regulator and your position."));
        else if(s.Warnings&Dive::FastAscent) hint=Local(TEXT("Du steigst schnell auf. Prüfe deinen Auftrieb."),TEXT("You are ascending quickly. Check your buoyancy."));
        else if(s.Warnings&Dive::LowGas) hint=Local(TEXT("Wenig Atemgas. Behalte deinen Rückweg im Blick."),TEXT("Breathing gas is low. Keep track of your return route."));
        else if(s.MotorCapacity<.8) hint=Local(TEXT("Deine Leistungsfähigkeit sinkt. Prüfe Vitalwerte und Tauchprofil."),TEXT("Your capacity is falling. Check vitals and your dive profile."));
        if(!hint.IsEmpty()) { Notify(hint); NextAutomaticHint=s.ElapsedSeconds+30; }
    }
    bEnteredWater|=s.DepthMetres>.2;
    if(bTutorialActive)
    {
        Tutorial.ObserveView(PC->GetControlRotation().Yaw,PC->GetControlRotation().Pitch);
        Tutorial.Step(Step,s,(position-old).Size()/100,wish.SizeSquared()>.01,bShowComputer,ComputerPage,bLamp,bShowMap,FVector::Dist2D(position,EntryLocation)/100);
    }
    Environment->UpdateDaylight();
    const FVector eye=Camera->GetComponentLocation();
    const double eyeDepth=(Environment->SurfaceZ(eye.X,eye.Y,s.ElapsedSeconds)-eye.Z)/100;
    if(Audio->Advance(Step,s,Simulation.Settings,eyeDepth)&&s.MouthSubmerged)
        Environment->EmitBreath(physicalMouth,FDiveBodyFrame::View(PC->GetControlRotation().Pitch,PC->GetControlRotation().Yaw).Vector(),s.AmbientBar);
    FreeflowBubbleClock+=Step;
    if(s.FreeflowLocalLPS>0&&s.MouthSubmerged&&FreeflowBubbleClock>=.2)
    {
        Environment->EmitBreath(physicalMouth,FDiveBodyFrame::View(PC->GetControlRotation().Pitch,PC->GetControlRotation().Yaw).Vector(),s.AmbientBar);
        FreeflowBubbleClock=0;
    }
    TelemetryAccumulator+=Step;
    if(TelemetryAccumulator>=1)
    {
        Samples.Add({s,position}); TelemetryAccumulator-=1;
        if(Samples.Num()%30==0&&!SaveLog(TEXT("in_progress")))
            Notify(Local(TEXT("Zwischenspeichern fehlgeschlagen. Aktuelle Messpunkte nur im Arbeitsspeicher."),
                         TEXT("Autosave failed. Latest log data remains in memory.")));
    }
    if(s.SpO2<60 && Simulation.Settings.Rules().Physiology)
        FinishDive(s.CylinderBar<=.1?TEXT("out_of_gas"):TEXT("no_breathing_supply"));
}

void ADivePawn::Notify(const FString& Message)
{ Notice=Message; bNoticeEnglish=bEnglish; NoticeUntil=GetWorld()->GetTimeSeconds()+8; }

void ADivePawn::Interact()
{
    if(bMenu || !bDiveActive) return;
    if(CanFinishDive())
        FinishDive(TEXT("completed"));
    else if(PendingConfig.StartOnShore&&!bEnteredWater)
        Notify(Local(TEXT("Erst abtauchen; nach der Rückkehr hier den Tauchgang beenden."),TEXT("Dive first; return here afterwards to finish.")));
    else Notify(PendingConfig.StartOnShore?Local(TEXT("Zum Beenden auf das Startpodest zurückgehen."),TEXT("Return to the starting platform to finish.")):Local(TEXT("Zum Beenden an der Oberfläche zur Einstiegsboje zurückkehren."),TEXT("Return to the entry buoy at the surface to finish your dive.")));
}

FString ADivePawn::EntryName() const
{ return Environment?Environment->Terrain.EntryList(PendingConfig.StartOnShore)[EntryIndex].Name:FString(); }
FVector ADivePawn::GetPawnViewLocation() const { return Camera->GetComponentLocation(); }
FRotator ADivePawn::GetViewRotation() const
{
    // A render frame may fall between fixed collision steps. Pending input is
    // not an accepted head pose and must not expose the inside of the body.
    return bDiveActive?PhysicalPose.Gaze:Super::GetViewRotation();
}
bool ADivePawn::CanFinishDive() const
{
    if(!bDiveActive||Simulation.Data.DepthMetres>.8) return false;
    if(!PendingConfig.StartOnShore) return FVector::Dist2D(GetActorLocation(),EntryLocation)<1200;
    if(!bWalking||!bEnteredWater||!Environment||!Environment->Accesses.IsValidIndex(EntryIndex)) return false;
    const auto& access=Environment->Accesses[EntryIndex];
    const FVector delta=GetActorLocation()-access.Origin;
    const double along=FVector::DotProduct(delta,access.Forward);
    const double across=FVector::DotProduct(delta,FVector(-access.Forward.Y,access.Forward.X,0));
    return along>=-100&&along<=access.Landing&&FMath::Abs(across)<=90&&
        FMath::Abs(GetActorLocation().Z-EntryLocation.Z)<15;
}

void ADivePawn::FinishDive(const FString& Reason)
{
    if(!bDiveActive) return;
    if(auto* pc=Cast<APlayerController>(GetController()))
        if(auto* hud=Cast<ADiveHUD>(pc->GetHUD())) { hud->CloseDiscoveries(); hud->CloseControls(); hud->CloseProfiles(true); hud->CloseEntryMap(); }
    Samples.Add({Simulation.Data,GetActorLocation()});
    Outcome=Reason; bDiveActive=false; bSummary=true;
    if(!SaveLog(Reason)) Outcome+=TEXT(" / log_save_failed");
    if(bTutorialActive)
    {
        const bool completed=Tutorial.Finish(Reason==TEXT("completed"));
        bTutorialCompleted|=completed;
        bTutorialActive=false; PendingConfig=BeforeTutorialConfig; EntryIndex=BeforeTutorialEntry;
        SaveSettings();
        if(bSmokeTest&&SmokeScenario==TEXT("tutorial"))
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_TUTORIAL_%s seconds=%.2f stages=%d"),completed?TEXT("PASS"):TEXT("FAIL"),Simulation.Data.ElapsedSeconds,static_cast<int>(Tutorial.Stage));
            if(!completed) FPlatformMisc::RequestExitWithStatus(false,1);
            TutorialFinishedAt=GetWorld()->GetTimeSeconds();
        }
    }
    SetMenu(true);
}

void ADivePawn::EndPlay(const EEndPlayReason::Type Reason)
{
    FScreenshotRequest::OnScreenshotRequestProcessed().Remove(PhotoProcessedHandle);
    PendingPhoto.Reset();
    if(bDiveActive) SaveLog(TEXT("interrupted"));
    Super::EndPlay(Reason);
}

bool ADivePawn::SaveLog(const FString& Reason)
{
    const double started=FPlatformTime::Seconds();
    const FString dir=FPaths::ProjectSavedDir()/TEXT("DiveLogs");
    if(LastLogPath.IsEmpty()) LastLogPath=dir/(FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)+TEXT(".json"));
    FDiveRecording recording;
    recording.Path=LastLogPath; recording.StartedUTC=DiveStartedUTC; recording.Status=Reason;
    recording.Location=EntryName(); recording.bTest=bSmokeTest; recording.Config=Simulation.Settings;
    recording.State=Simulation.Data; recording.SurveyOrigin=Environment->Terrain.ToSurvey(FVector::ZeroVector);
    recording.Samples=Samples; recording.Photographs=Photographs;
    if(Reason==TEXT("in_progress"))
    {
        RecordingWriter.Submit(MoveTemp(recording));
        if(bSmokeTest) { UE_LOG(LogTemp,Display,TEXT("DIVE_LOG_SUBMIT samples=%d milliseconds=%.3f"),Samples.Num(),(FPlatformTime::Seconds()-started)*1000); }
        return true; // Queued; the result is reported by Poll on a later game tick.
    }
    return RecordingWriter.Finish(MoveTemp(recording)).bSuccess;
}

void ADivePawn::SaveSettings()
{
    auto obj=MakeShared<FJsonObject>();
    obj->SetNumberField(TEXT("schema_version"),1);
    const Dive::Config& savedConfig=bTutorialActive?BeforeTutorialConfig:PendingConfig;
    DiveConfigIO::Write(obj,savedConfig,bTutorialActive?BeforeTutorialEntry:EntryIndex);
    obj->SetBoolField(TEXT("english"),bEnglish);
    obj->SetBoolField(TEXT("camera_motion"),bCameraMotion);
    obj->SetNumberField(TEXT("hud_style"),FMath::Clamp(HUDStyle,0,2));
    AudioLevels.Sanitize();
    for(int i=0;i<5;++i) obj->SetNumberField(FString::Printf(TEXT("audio_%d"),i),AudioLevels.Values[i]);
    Controls.Sanitize(); Controls.Write(obj);
    obj->SetBoolField(TEXT("tutorial_completed"),bTutorialCompleted);
    FString json; auto writer=TJsonWriterFactory<>::Create(&json);
    FJsonSerializer::Serialize(obj,writer);
    const FString path=FPaths::ProjectSavedDir()/(bSmokeTest?TEXT("DiveSettingsTest.json"):TEXT("DiveSettings.json"));
    const FString temp=path+TEXT(".tmp");
    if(!FFileHelper::SaveStringToFile(json,*temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)||!IFileManager::Get().Move(*path,*temp,true,true))
    { ControlMessage=Local(TEXT("Einstellungen konnten nicht gespeichert werden."),TEXT("Settings could not be saved.")); Notify(ControlMessage); }
}

void ADivePawn::LoadSettings()
{
    FString json; if(!FFileHelper::LoadFileToString(json,*(FPaths::ProjectSavedDir()/(bSmokeTest?TEXT("DiveSettingsTest.json"):TEXT("DiveSettings.json"))))) return;
    TSharedPtr<FJsonObject> obj; if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),obj)||!obj) return;
    FString configError; DiveConfigIO::Read(obj,PendingConfig,EntryIndex,false,configError);
    obj->TryGetBoolField(TEXT("english"),bEnglish);
    obj->TryGetBoolField(TEXT("camera_motion"),bCameraMotion);
    double hudStyle=0;
    if(obj->TryGetNumberField(TEXT("hud_style"),hudStyle)&&FMath::IsFinite(hudStyle))
        HUDStyle=FMath::RoundToInt(FMath::Clamp(hudStyle,0.,2.));
    for(int i=0;i<5;++i) obj->TryGetNumberField(FString::Printf(TEXT("audio_%d"),i),AudioLevels.Values[i]);
    AudioLevels.Sanitize();
    Controls.Read(obj); ApplyControls();
    obj->TryGetBoolField(TEXT("tutorial_completed"),bTutorialCompleted);
}
