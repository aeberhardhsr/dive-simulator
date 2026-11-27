#include "DivePawn.h"
#include "DiveWorld.h"
#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "GameFramework/PlayerController.h"
#include "UnrealClient.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/Guid.h"
#include "Misc/EngineVersion.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

CSV_DEFINE_CATEGORY(DivePerf,true);

void ADivePawn::SetupPerformance()
{
    int32 quality=2; double scale=100;
    FParse::Value(FCommandLine::Get(),TEXT("DiveQuality="),quality);
    FParse::Value(FCommandLine::Get(),TEXT("DiveRenderScale="),scale);
    FParse::Value(FCommandLine::Get(),TEXT("DivePerfSeconds="),PerformanceSeconds);
    quality=FMath::Clamp(quality,0,3); scale=FMath::Clamp(scale,50.,100.);
    PerformanceSeconds=FMath::Clamp(PerformanceSeconds,10.,7200.);
    GraphicsQuality=quality;
    if(auto* settings=GEngine->GetGameUserSettings())
    {
        settings->SetOverallScalabilityLevel(quality);
        settings->SetVSyncEnabled(false); settings->SetFrameRateLimit(0);
        settings->ApplyNonResolutionSettings(); // Never save benchmark overrides as player preferences.
    }
    GEngine->bSmoothFrameRate=false; GEngine->bUseFixedFrameRate=false;
    auto set=[](const TCHAR* name,float value)
    { if(auto* variable=IConsoleManager::Get().FindConsoleVariable(name)) variable->Set(value,ECVF_SetByCode); };
    set(TEXT("t.MaxFPS"),0); set(TEXT("r.VSync"),0);
    set(TEXT("r.DynamicRes.OperationMode"),0); set(TEXT("r.ScreenPercentage"),scale);
    set(TEXT("r.SecondaryScreenPercentage.GameViewport"),100);
    PerformanceDirectory=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Performance")/
        (SmokeScenario+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    IFileManager::Get().MakeDirectory(*PerformanceDirectory,true);
    PerformanceStarted=FPlatformTime::Seconds();
    if(auto* pc=Cast<APlayerController>(GetController())) PerformanceYaw=pc->GetControlRotation().Yaw;
    if(SmokeScenario==TEXT("perf-soak")) SetupSoak();
    if(SmokeScenario==TEXT("perf-sediment"))
    {
        FVector location=GetActorLocation();location.Z=Environment->GroundZ(location.X,location.Y)+110;
        SetActorLocation(location);VelocityMetres=FVector::ZeroVector;ResetContactPose();
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_PERF_WARMUP seconds=15 capture_seconds=%.0f quality=%d scale=%.2f"),PerformanceSeconds,quality,scale);
}

void ADivePawn::TickPerformance()
{
    if(SmokeScenario==TEXT("perf-soak")) { TickSoak(); return; }
#if CSV_PROFILER
    const double now=FPlatformTime::Seconds();
    auto* profiler=FCsvProfiler::Get();
    if(PerformancePhase==0&&now-PerformanceStarted>=15)
    {
        profiler->BeginCapture(-1,PerformanceDirectory,TEXT("Capture.csv"));
        PerformanceCaptureStarted=now; PerformancePhase=1;
        UE_LOG(LogTemp,Display,TEXT("DIVE_PERF_CAPTURE_BEGIN scenario=%s"),*SmokeScenario);
    }
    if(PerformancePhase==1)
    {
        CSV_CUSTOM_STAT(DivePerf,DepthM,Simulation.Data.DepthMetres,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(DivePerf,DistanceM,Simulation.Data.DistanceMetres,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(DivePerf,SimSeconds,Simulation.Data.ElapsedSeconds,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(DivePerf,TerrainChunks,Environment->TerrainChunkCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(DivePerf,SedimentLive,Environment->SedimentLive(),ECsvCustomStatOp::Set);
        if(now>=PerformanceNextMemory)
        { PerformanceMemoryMB=FPlatformMemory::GetStats().UsedPhysical/1048576.; PerformanceNextMemory=now+1; }
        CSV_CUSTOM_STAT(DivePerf,ProcessMemoryMB,PerformanceMemoryMB,ECsvCustomStatOp::Set);
        if(now-PerformanceCaptureStarted>=PerformanceSeconds)
        {
            PerformanceWrite=profiler->EndCapture(); PerformancePhase=2;
            if(!PerformanceWrite.IsValid()) { UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL capture did not start")); FPlatformMisc::RequestExitWithStatus(false,1); }
        }
    }
    if(PerformancePhase==2&&PerformanceWrite.IsValid()&&PerformanceWrite.IsReady())
    {
        const FString csv=PerformanceWrite.Get();
        const FIntPoint size=GEngine->GameViewport->Viewport->GetSizeXY();
        auto root=MakeShared<FJsonObject>();
        root->SetStringField(TEXT("scenario"),SmokeScenario);
        root->SetStringField(TEXT("csv"),csv);
        root->SetStringField(TEXT("gpu"),FPlatformMisc::GetPrimaryGPUBrand());
        root->SetStringField(TEXT("cpu"),FPlatformMisc::GetCPUBrand());
        root->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
        root->SetNumberField(TEXT("width"),size.X); root->SetNumberField(TEXT("height"),size.Y);
        root->SetNumberField(TEXT("quality"),GraphicsQuality);
        root->SetNumberField(TEXT("warmup_seconds"),15);
        root->SetNumberField(TEXT("requested_capture_seconds"),PerformanceSeconds);
        root->SetNumberField(TEXT("simulation_seconds"),Simulation.Data.ElapsedSeconds);
        root->SetNumberField(TEXT("distance_m"),Simulation.Data.DistanceMetres);
        root->SetNumberField(TEXT("depth_m"),Simulation.Data.DepthMetres);
        root->SetBoolField(TEXT("walking"),bWalking);
        root->SetBoolField(TEXT("water_preview"),Environment->WaterPreviewEnabled());
        root->SetBoolField(TEXT("gravel_immediate_retirement"),FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelRetirement")));
        root->SetBoolField(TEXT("water_interface"),Environment->WaterInterfaceEnabled());
        root->SetBoolField(TEXT("bounded_water"),Environment->BoundedWaterEnabled());
        root->SetBoolField(TEXT("native_scattering"),Environment->NativeWaterScattering());
        root->SetNumberField(TEXT("sediment_born"),Environment->SedimentBorn());
        root->SetNumberField(TEXT("sediment_live"),Environment->SedimentLive());
        for(const TCHAR* name:{TEXT("r.ScreenPercentage"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("r.DynamicRes.OperationMode"),TEXT("r.VSync"),TEXT("t.MaxFPS"),
            TEXT("sg.ViewDistanceQuality"),TEXT("sg.AntiAliasingQuality"),TEXT("sg.ShadowQuality"),TEXT("sg.GlobalIlluminationQuality"),
            TEXT("sg.ReflectionQuality"),TEXT("sg.PostProcessQuality"),TEXT("sg.TextureQuality"),TEXT("sg.EffectsQuality"),TEXT("sg.FoliageQuality"),
            TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Lumen.HardwareRayTracing.LightingMode"),TEXT("r.Lumen.HardwareRayTracing.HitLighting.Allowed"),
            TEXT("r.Lumen.TranslucencyReflections.FrontLayer.Allow"),TEXT("r.Lumen.Reflections.ScreenTraces"),TEXT("r.VolumetricFog"),
            TEXT("r.VolumetricFog.DepthDistributionScale"),TEXT("r.VolumetricFog.GridSizeZ"),TEXT("r.VolumetricFog.GridPixelSize")})
            if(auto* variable=IConsoleManager::Get().FindConsoleVariable(name)) root->SetNumberField(name,variable->GetFloat());
        FString json;
        const bool valid=FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json))&&
            FFileHelper::SaveStringToFile(json,*(PerformanceDirectory/TEXT("Metadata.json")))&&IFileManager::Get().FileSize(*csv)>1000;
        if(!valid) { UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL missing capture or metadata")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        FScreenshotRequest::RequestScreenshot(PerformanceDirectory/TEXT("Frame.png"),true,false);
        PerformancePhase=3; PerformanceStarted=now;
    }
    if(PerformancePhase==3&&now-PerformanceStarted>2)
    {
        const bool valid=IFileManager::Get().FileSize(*(PerformanceDirectory/TEXT("Frame.png")))>10000&&bSmokeChunkLimit;
        FinishDive(valid?TEXT("smoke_perf_capture"):TEXT("smoke_perf_failed"));
        UE_LOG(LogTemp,Display,TEXT("DIVE_PERF_%s directory=%s"),valid?TEXT("CAPTURE_READY"):TEXT("FAIL"),*PerformanceDirectory);
        FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
    }
    if(PerformancePhase==2&&now-PerformanceCaptureStarted>PerformanceSeconds+30)
    { UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL CSV write timeout")); FPlatformMisc::RequestExitWithStatus(false,1); }
#else
    UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL CSV profiler unavailable in this build"));
    FPlatformMisc::RequestExitWithStatus(false,1);
#endif
}
