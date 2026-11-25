#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveLogbook.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/EngineVersion.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void ADivePawn::SetupSoak()
{
    PerformanceSeconds=FMath::Clamp(PerformanceSeconds,180.,7200.);
    // The entry's local depth gradient can lead along the shore. These headings
    // follow open-water corridors, still checked against the actual raster below.
    constexpr double headings[]={90,0,-90};
    PerformanceYaw=headings[FMath::Clamp(EntryIndex,0,2)];
    const FVector forward=FRotator(0,PerformanceYaw,0).Vector();
    SoakEnds[0]=EntryLocation+forward*1600;
    SoakEnds[1]=EntryLocation+forward*24000;
    const FVector side(-forward.Y,forward.X,0);
    // Check the measured corridor before sending the diver down this course.
    for(int along=0;along<=240;along+=8) for(int across=-4;across<=4;across+=4)
    {
        const FVector p=EntryLocation+forward*(along*100)+side*(across*100);
        float ground=0;
        if(!Environment->Terrain.Sample(p.X,p.Y,ground)||ground>=0)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_SOAK_FAIL course leaves surveyed water")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    }
}

void ADivePawn::SteerSoak(float DeltaSeconds)
{
    if(PerformancePhase>=2) { SoakForward=0; SoakVertical=0; return; }
    auto* pc=Cast<APlayerController>(GetController()); if(!pc) return;
    const FVector position=GetActorLocation();
    if(FVector::Dist2D(position,SoakEnds[SoakTarget])<300)
    { SoakTarget=1-SoakTarget; ++SoakTurns; }
    const double yaw=(SoakEnds[SoakTarget]-position).Rotation().Yaw;
    const double current=FMath::FixedTurn(pc->GetControlRotation().Yaw,yaw,90*DeltaSeconds);
    pc->SetControlRotation(FRotator(0,current,0));
    SoakForward=FMath::Abs(FMath::FindDeltaAngleDegrees(current,yaw))<30?1.f:0.f;
    const bool atSurface=FMath::Fmod(Simulation.Data.ElapsedSeconds,300.)>=240;
    const double depth=8.;
    double targetZ=FMath::Max(Environment->GroundZ(position.X,position.Y)+100.,
        Environment->SurfaceZ(position.X,position.Y)-depth*100);
    if(!atSurface)
    {
        FHitResult obstacle;
        FCollisionQueryParams query(SCENE_QUERY_STAT(SoakObstacle),false,this);
        const FVector ahead=FRotator(0,current,0).Vector()*200;
        const bool blocked=GetWorld()->SweepSingleByChannel(obstacle,position,position+ahead,FQuat::Identity,
            ECC_WorldStatic,FCollisionShape::MakeCapsule(30,68),query);
        if(blocked)
        {
            if(Simulation.Data.ElapsedSeconds>=SoakAvoidUntil) ++SoakAvoidances;
            SoakAvoidZ=FMath::Min(position.Z+150,Environment->SurfaceZ(position.X,position.Y)+20);
            SoakAvoidUntil=Simulation.Data.ElapsedSeconds+6;
        }
        if(Simulation.Data.ElapsedSeconds<SoakAvoidUntil)
        { targetZ=FMath::Max(targetZ,SoakAvoidZ); SoakForward*=.6f; }
    }
    SoakVertical=FMath::Clamp(static_cast<float>((targetZ-position.Z)/150),-.8f,.8f);
    // Keep swimming upward while treading water; a weak position controller
    // alone cannot counter loss of body displacement above the waterline.
    if(atSurface) { SoakForward*=.35f; SoakVertical=1; }
}

bool ADivePawn::ReportSoak(bool Final)
{
    auto obj=MakeShared<FJsonObject>();
    const auto size=GEngine->GameViewport->Viewport->GetSizeXY();
    const double memory=FPlatformMemory::GetStats().UsedPhysical/1048576.;
    SoakMemoryMax=FMath::Max(SoakMemoryMax,memory);
    obj->SetNumberField(TEXT("schema"),1); obj->SetBoolField(TEXT("final"),Final);
    obj->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
    obj->SetStringField(TEXT("gpu"),FPlatformMisc::GetPrimaryGPUBrand());
    obj->SetStringField(TEXT("cpu"),FPlatformMisc::GetCPUBrand());
    obj->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
    obj->SetStringField(TEXT("mode"),TEXT("Exploration"));
    obj->SetStringField(TEXT("location"),EntryName());
    obj->SetNumberField(TEXT("course_yaw_degrees"),PerformanceYaw);
    obj->SetNumberField(TEXT("near_waypoint_m"),16); obj->SetNumberField(TEXT("far_waypoint_m"),240);
    obj->SetNumberField(TEXT("width"),size.X); obj->SetNumberField(TEXT("height"),size.Y);
    obj->SetNumberField(TEXT("quality"),GraphicsQuality);
    obj->SetNumberField(TEXT("requested_seconds"),PerformanceSeconds);
    obj->SetNumberField(TEXT("warmup_seconds"),15);
    obj->SetNumberField(TEXT("measured_simulation_seconds"),(Final?SoakSimEnd:Simulation.Data.ElapsedSeconds)-SoakSimStart);
    obj->SetNumberField(TEXT("simulation_seconds"),Simulation.Data.ElapsedSeconds);
    obj->SetNumberField(TEXT("distance_m"),Simulation.Data.DistanceMetres-SoakDistanceStart);
    obj->SetNumberField(TEXT("depth_m"),Simulation.Data.DepthMetres);
    obj->SetNumberField(TEXT("minimum_depth_m"),SoakMinDepth); obj->SetNumberField(TEXT("maximum_depth_m"),SoakMaxDepth);
    obj->SetBoolField(TEXT("surface_reached"),bSmokeSurfaceReached);
    obj->SetNumberField(TEXT("terrain_chunks"),Environment->TerrainChunkCount());
    obj->SetNumberField(TEXT("visited_chunk_centers"),SoakTiles.Num());
    obj->SetNumberField(TEXT("turns"),SoakTurns);
    obj->SetNumberField(TEXT("obstacle_avoidances"),SoakAvoidances);
    obj->SetNumberField(TEXT("samples"),Samples.Num());
    obj->SetNumberField(TEXT("successful_autosaves"),SoakSaves);
    obj->SetNumberField(TEXT("failed_autosaves"),SoakSaveErrors);
    obj->SetNumberField(TEXT("pending_autosaves"),RecordingWriter.PendingCount());
    obj->SetNumberField(TEXT("memory_start_mb"),SoakMemoryStart);
    obj->SetNumberField(TEXT("memory_mb"),memory); obj->SetNumberField(TEXT("memory_max_mb"),SoakMemoryMax);
    obj->SetNumberField(TEXT("finalization_ms"),SoakFinalizationMS);
    obj->SetStringField(TEXT("log_path"),FPaths::ConvertRelativePathToFull(LastLogPath));
    if(Final) obj->SetStringField(TEXT("outcome"),Outcome);
    auto metrics=[](const Dive::SoakMetrics& data)
    {
        auto result=MakeShared<FJsonObject>();
        result->SetNumberField(TEXT("frames"),data.Frames); result->SetNumberField(TEXT("wall_seconds"),data.Seconds);
        result->SetNumberField(TEXT("average_fps"),data.Seconds>0?data.Frames/data.Seconds:0);
        result->SetNumberField(TEXT("p95_upper_ms"),data.PercentileUpper(.95));
        result->SetNumberField(TEXT("p99_upper_ms"),data.PercentileUpper(.99));
        result->SetNumberField(TEXT("maximum_ms"),data.MaximumMS);
        result->SetNumberField(TEXT("over_16_67_ms"),data.Over60); result->SetNumberField(TEXT("over_33_33_ms"),data.Over30);
        result->SetNumberField(TEXT("over_100_ms"),data.Over100);
        return result;
    };
    obj->SetObjectField(TEXT("total"),metrics(SoakTotal)); obj->SetObjectField(TEXT("interval"),metrics(SoakWindow));
    for(const TCHAR* name:{TEXT("r.ScreenPercentage"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("r.DynamicRes.OperationMode"),TEXT("r.VSync"),TEXT("t.MaxFPS"),
        TEXT("sg.ViewDistanceQuality"),TEXT("sg.AntiAliasingQuality"),TEXT("sg.ShadowQuality"),TEXT("sg.GlobalIlluminationQuality"),
        TEXT("sg.ReflectionQuality"),TEXT("sg.PostProcessQuality"),TEXT("sg.TextureQuality"),TEXT("sg.EffectsQuality"),TEXT("sg.FoliageQuality")})
        if(auto* variable=IConsoleManager::Get().FindConsoleVariable(name)) obj->SetNumberField(name,variable->GetFloat());
    FString json;
    if(!FJsonSerializer::Serialize(obj,TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&json))) return false;
    const FString path=PerformanceDirectory/(Final?TEXT("Result.json"):TEXT("Intervals.jsonl"));
    if(!FFileHelper::SaveStringToFile(json+TEXT("\n"),*path,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
        &IFileManager::Get(),Final?0:FILEWRITE_Append)) return false;
    UE_LOG(LogTemp,Display,TEXT("DIVE_SOAK_PROGRESS wall=%.1f sim=%.1f distance=%.1f turns=%d saves=%d memory_mb=%.1f max_ms=%.2f"),
        SoakTotal.Seconds,Simulation.Data.ElapsedSeconds-SoakSimStart,Simulation.Data.DistanceMetres-SoakDistanceStart,
        SoakTurns,SoakSaves,memory,SoakWindow.MaximumMS);
    SoakWindow=Dive::SoakMetrics{};
    return true;
}

void ADivePawn::TickSoak()
{
    const double now=FPlatformTime::Seconds();
    auto fail=[](const TCHAR* why)
    { UE_LOG(LogTemp,Error,TEXT("DIVE_SOAK_FAIL %s"),why); FPlatformMisc::RequestExitWithStatus(false,1); };
    if(PerformancePhase==0&&now-PerformanceStarted>=15)
    {
        PerformanceCaptureStarted=now; SoakLastTick=now; SoakNextReport=now+30;
        SoakSimStart=Simulation.Data.ElapsedSeconds; SoakDistanceStart=Simulation.Data.DistanceMetres;
        SoakMemoryStart=FPlatformMemory::GetStats().UsedPhysical/1048576.; SoakMemoryMax=SoakMemoryStart;
        PerformancePhase=1;
        UE_LOG(LogTemp,Display,TEXT("DIVE_SOAK_BEGIN directory=%s seconds=%.0f"),*PerformanceDirectory,PerformanceSeconds);
        FScreenshotRequest::RequestScreenshot(PerformanceDirectory/TEXT("Start.png"),true,false);
        return;
    }
    if(PerformancePhase==1)
    {
        const double dt=now-SoakLastTick; SoakLastTick=now;
        const auto position=GetActorLocation();
        if(!SoakTotal.Add(dt)||!SoakWindow.Add(dt)||position.ContainsNaN()||!FMath::IsFinite(Simulation.Data.DepthMetres)||
            Environment->TerrainChunkCount()<1||Environment->TerrainChunkCount()>9||!bDiveActive||SoakSaveErrors>0)
        { fail(TEXT("invalid state, terrain count or failed autosave")); return; }
        SoakMinDepth=FMath::Min(SoakMinDepth,Simulation.Data.DepthMetres);
        SoakMaxDepth=FMath::Max(SoakMaxDepth,Simulation.Data.DepthMetres);
        SoakTiles.Add(FIntPoint(FMath::FloorToInt(position.X/6400),FMath::FloorToInt(position.Y/6400)));
        if(now>=SoakNextReport)
        { if(!ReportSoak(false)) { fail(TEXT("interval write failed")); return; } SoakNextReport=now+30; }
        if(SoakTotal.Seconds>=(SoakScreenshot+1)*600)
        {
            ++SoakScreenshot;
            FScreenshotRequest::RequestScreenshot(PerformanceDirectory/FString::Printf(TEXT("Frame_%03d.png"),SoakScreenshot),true,false);
        }
        if(SoakTotal.Seconds>=PerformanceSeconds&&Simulation.Data.ElapsedSeconds-SoakSimStart>=PerformanceSeconds)
        {
            SoakSimEnd=Simulation.Data.ElapsedSeconds; PerformancePhase=2; PerformanceStarted=now;
            FScreenshotRequest::RequestScreenshot(PerformanceDirectory/TEXT("Final.png"),true,false);
        }
    }
    else if(PerformancePhase==2&&now-PerformanceStarted>=2)
    {
        PerformancePhase=3;
        const double finishStarted=FPlatformTime::Seconds();
        FinishDive(TEXT("smoke_soak_completed"));
        SoakFinalizationMS=(FPlatformTime::Seconds()-finishStarted)*1000;
        const auto log=DiveLogbook::Read(LastLogPath);
        const bool valid=log.Error.IsEmpty()&&log.Status==TEXT("smoke_soak_completed")&&log.bTest&&
            log.Samples.Num()==Samples.Num()&&Samples.Num()>=FMath::FloorToInt(Simulation.Data.ElapsedSeconds)&&
            SoakTurns>=FMath::Max(1,FMath::FloorToInt(PerformanceSeconds/500))&&SoakTiles.Num()>=3&&
            Simulation.Data.DistanceMetres-SoakDistanceStart>=PerformanceSeconds*.3&&SoakSaveErrors==0&&
            SoakSaves>=FMath::FloorToInt(PerformanceSeconds/30)&&bSmokeChunkLimit&&
            (PerformanceSeconds<300||(bSmokeSurfaceReached&&SoakMinDepth<.2&&SoakMaxDepth>5))&&
            IFileManager::Get().FileSize(*(PerformanceDirectory/TEXT("Start.png")))>10000&&
            IFileManager::Get().FileSize(*(PerformanceDirectory/TEXT("Final.png")))>10000;
        if(!ReportSoak(true)||!valid) { fail(TEXT("final log, travel, streaming or screenshots incomplete")); return; }
        UE_LOG(LogTemp,Display,TEXT("DIVE_SOAK_PASS directory=%s"),*PerformanceDirectory);
        FPlatformMisc::RequestExitWithStatus(false,0);
    }
}
