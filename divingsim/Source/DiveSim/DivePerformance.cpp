#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveWaterScene.h"
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
#if WITH_EDITOR
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Rendering/SkeletalMeshRenderData.h"
#endif

CSV_DEFINE_CATEGORY(DivePerf,true);

#if WITH_EDITOR
namespace
{
bool SaveFPVGeometry(ADivePawn* Pawn,const FString& Directory)
{
    auto* suit=Pawn->Equipment&&Pawn->Equipment->Avatar?Pawn->Equipment->Avatar->Suit.Get():nullptr;
    auto* asset=suit?Cast<USkeletalMesh>(suit->GetSkinnedAsset()):nullptr;
    auto* render=asset?asset->GetResourceForRendering():nullptr;
    if(!render||render->LODRenderData.IsEmpty()) return false;
    auto& lod=render->LODRenderData[0];
    TArray<FMatrix44f> matrices;suit->GetCurrentRefToLocalMatrices(matrices,0);
    TArray<FVector3f> positions;
    USkinnedMeshComponent::ComputeSkinnedPositions(suit,positions,matrices,lod,*lod.GetSkinWeightVertexBuffer());
    const FTransform view=Pawn->Camera->GetComponentTransform();
    TArray<TSharedPtr<FJsonValue>> vertices;
    for(int32 i=0;i<positions.Num();++i)
    {
        const FVector point=view.InverseTransformPosition(suit->GetComponentTransform().TransformPosition(FVector(positions[i])));
        const auto* section=lod.RenderSections.FindByPredicate([i](const auto& s){return i>=static_cast<int32>(s.BaseVertexIndex)&&i<static_cast<int32>(s.BaseVertexIndex+s.NumVertices);});
        int32 influence=0;const auto* weights=lod.GetSkinWeightVertexBuffer();
        for(uint32 j=1;j<weights->GetMaxBoneInfluences();++j)
            if(weights->GetBoneWeight(i,j)>weights->GetBoneWeight(i,influence)) influence=j;
        const int32 localBone=weights->GetBoneIndex(i,influence);
        const FString bone=section&&section->BoneMap.IsValidIndex(localBone)?asset->GetRefSkeleton().GetBoneName(section->BoneMap[localBone]).ToString():TEXT("unknown");
        const FColor color=lod.StaticVertexBuffers.ColorVertexBuffer.GetNumVertices()>static_cast<uint32>(i)?lod.StaticVertexBuffers.ColorVertexBuffer.VertexColor(i):FColor::White;
        TArray<TSharedPtr<FJsonValue>> row;
        for(double n:{point.X,point.Y,point.Z}) row.Add(MakeShared<FJsonValueNumber>(n));
        row.Add(MakeShared<FJsonValueString>(bone));
        for(uint8 n:{color.R,color.G,color.B}) row.Add(MakeShared<FJsonValueNumber>(n));
        vertices.Add(MakeShared<FJsonValueArray>(row));
    }
    TArray<uint32> indices;lod.MultiSizeIndexContainer.GetIndexBuffer(indices);
    TArray<TSharedPtr<FJsonValue>> triangles;
    for(uint32 i:indices) triangles.Add(MakeShared<FJsonValueNumber>(i));
    auto root=MakeShared<FJsonObject>();
    root->SetStringField(TEXT("mesh"),asset->GetPathName());
    root->SetStringField(TEXT("coordinates"),TEXT("camera centimetres: X forward, Y right, Z up; vertex rows XYZ, dominant bone, RGB8"));
    root->SetNumberField(TEXT("horizontal_fov_degrees"),Pawn->Camera->FieldOfView);
    root->SetNumberField(TEXT("body_pitch"),Pawn->PhysicalPose.BodyPitch);
    root->SetNumberField(TEXT("head_yaw"),Pawn->PhysicalPose.HeadYaw);
    root->SetArrayField(TEXT("vertices"),vertices);root->SetArrayField(TEXT("indices"),triangles);
    FString json;
    return FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json))&&FFileHelper::SaveStringToFile(json,*(Directory/TEXT("FPVGeometry.json")));
}
}
#endif

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
    double fixedYaw=0;
    if(FParse::Value(FCommandLine::Get(),TEXT("DivePerfFixedYaw="),fixedYaw)&&FMath::IsFinite(fixedYaw)) PerformanceYaw=fixedYaw;
    if(SmokeScenario!=TEXT("perf-stream")&&SmokeScenario!=TEXT("perf-soak"))
        if(auto* pc=Cast<APlayerController>(GetController()))
        {
            double pitch=SmokeScenario==TEXT("perf-underwater")||SmokeScenario==TEXT("perf-sediment")?-12:-5;
            FParse::Value(FCommandLine::Get(),TEXT("DivePerfPitch="),pitch);
            if(!FMath::IsFinite(pitch)) pitch=-12;
            pc->SetControlRotation(FRotator(FMath::Clamp(pitch,-70.,70.),PerformanceYaw,0));
        }
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
        // Bind later FPV/performance captures to the actual accepted pose, not
        // only to scenario names: blocked turns can change what is on screen.
        auto rotation=[&](const TCHAR* name,const FRotator& value)
        {
            auto object=MakeShared<FJsonObject>();
            object->SetNumberField(TEXT("pitch"),value.Pitch);
            object->SetNumberField(TEXT("yaw"),value.Yaw);
            object->SetNumberField(TEXT("roll"),value.Roll);
            root->SetObjectField(name,object);
        };
        rotation(TEXT("camera_rotation_degrees"),Camera->GetComponentRotation());
        rotation(TEXT("physical_gaze_degrees"),PhysicalPose.Gaze);
        rotation(TEXT("control_rotation_degrees"),GetControlRotation());
        auto eye=MakeShared<FJsonObject>();const FVector position=Camera->GetComponentLocation();
        eye->SetNumberField(TEXT("x"),position.X);eye->SetNumberField(TEXT("y"),position.Y);eye->SetNumberField(TEXT("z"),position.Z);
        root->SetObjectField(TEXT("eye_world_cm"),eye);
        root->SetNumberField(TEXT("head_yaw_degrees"),PhysicalPose.HeadYaw);
        root->SetNumberField(TEXT("body_yaw_degrees"),PhysicalPose.BodyYaw());
        root->SetNumberField(TEXT("blocked_pose_steps"),BlockedPoseSteps);
        double fixedYaw=0;
        root->SetStringField(TEXT("orbit_input"),FParse::Value(FCommandLine::Get(),TEXT("DivePerfFixedYaw="),fixedYaw)&&FMath::IsFinite(fixedYaw)?
            TEXT("fixed_yaw"):TEXT("incremental_6_degrees_per_simulation_second"));
        root->SetBoolField(TEXT("water_preview"),Environment->WaterPreviewEnabled());
        root->SetBoolField(TEXT("gravel_immediate_retirement"),FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelRetirement")));
        root->SetBoolField(TEXT("population_immediate_retirement"),FParse::Param(FCommandLine::Get(),TEXT("DiveImmediatePopulationRetirement")));
        root->SetBoolField(TEXT("legacy_rock_pose"),FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyRockPose")));
        root->SetBoolField(TEXT("legacy_talus"),FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyTalus")));
        root->SetBoolField(TEXT("no_plant_understory"),FParse::Param(FCommandLine::Get(),TEXT("DiveNoPlantUnderstory")));
        root->SetBoolField(TEXT("unlimited_plants"),FParse::Param(FCommandLine::Get(),TEXT("DiveUnlimitedPlants")));
        root->SetBoolField(TEXT("legacy_formations"),FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyFormations")));
        root->SetBoolField(TEXT("foot_deposits"),bool(Environment->Terrain.DepositHeight));
        root->SetBoolField(TEXT("gravel_staged_build"),!FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelBuild")));
        root->SetBoolField(TEXT("population_staged_build"),!FParse::Param(FCommandLine::Get(),TEXT("DiveImmediatePopulationBuild")));
        root->SetBoolField(TEXT("water_interface"),Environment->WaterInterfaceEnabled());
        root->SetBoolField(TEXT("bounded_water"),Environment->BoundedWaterEnabled());
        root->SetBoolField(TEXT("native_scattering"),Environment->NativeWaterScattering());
        root->SetBoolField(TEXT("reflection_transport"),DiveReflectionTransportEnabled());
        root->SetBoolField(TEXT("production_water"),DiveProductionWaterEnabled());
        root->SetBoolField(TEXT("reflection_control"),!DiveProductionWaterEnabled()&&FParse::Param(FCommandLine::Get(),TEXT("DiveReflectionTransportControl")));
        root->SetBoolField(TEXT("reflection_unified"),DiveProductionWaterEnabled()||FParse::Param(FCommandLine::Get(),TEXT("DiveReflectionUnified")));
        root->SetNumberField(TEXT("sediment_born"),Environment->SedimentBorn());
        root->SetNumberField(TEXT("sediment_live"),Environment->SedimentLive());
        for(const TCHAR* name:{TEXT("r.ScreenPercentage"),TEXT("r.SecondaryScreenPercentage.GameViewport"),TEXT("r.DynamicRes.OperationMode"),TEXT("r.VSync"),TEXT("t.MaxFPS"),
            TEXT("sg.ViewDistanceQuality"),TEXT("sg.AntiAliasingQuality"),TEXT("sg.ShadowQuality"),TEXT("sg.GlobalIlluminationQuality"),
            TEXT("sg.ReflectionQuality"),TEXT("sg.PostProcessQuality"),TEXT("sg.TextureQuality"),TEXT("sg.EffectsQuality"),TEXT("sg.FoliageQuality"),
            TEXT("r.Lumen.HardwareRayTracing"),TEXT("r.Lumen.HardwareRayTracing.LightingMode"),TEXT("r.Lumen.HardwareRayTracing.HitLighting.Allowed"),
            TEXT("r.Lumen.TranslucencyReflections.FrontLayer.Allow"),TEXT("r.Lumen.Reflections.ScreenTraces"),TEXT("r.Lumen.HardwareRayTracing.Inline"),TEXT("r.CustomDepth"),TEXT("r.VolumetricFog"),
            TEXT("r.VolumetricFog.DepthDistributionScale"),TEXT("r.VolumetricFog.GridSizeZ"),TEXT("r.VolumetricFog.GridPixelSize")})
            if(auto* variable=IConsoleManager::Get().FindConsoleVariable(name)) root->SetNumberField(name,variable->GetFloat());
        FString json;
        const bool valid=FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json))&&
            FFileHelper::SaveStringToFile(json,*(PerformanceDirectory/TEXT("Metadata.json")))&&IFileManager::Get().FileSize(*csv)>1000;
        if(!valid) { UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL missing capture or metadata")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        FScreenshotRequest::RequestScreenshot(PerformanceDirectory/TEXT("Frame.png"),true,false);
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveTreeAnchorProbe"))&&
           (!Environment||!Environment->SaveVegetationAnchorProbe(PerformanceDirectory)))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_TREE_ANCHOR_PROBE_FAIL"));FPlatformMisc::RequestExitWithStatus(false,1);return; }
#if WITH_EDITOR
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveFPVProbe"))&&!SaveFPVGeometry(this,PerformanceDirectory))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_FPV_PROBE_FAIL geometry export"));FPlatformMisc::RequestExitWithStatus(false,1);return; }
#endif
        PerformancePhase=3; PerformanceStarted=now;
    }
    if(PerformancePhase==3&&now-PerformanceStarted>2)
    {
#if WITH_EDITOR
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveFPVProbe")))
        {
            Equipment->Avatar->Suit->SetVisibility(false);
            PerformancePhase=4;PerformanceStarted=now;return;
        }
#endif
        const bool valid=IFileManager::Get().FileSize(*(PerformanceDirectory/TEXT("Frame.png")))>10000&&bSmokeChunkLimit;
        FinishDive(valid?TEXT("smoke_perf_capture"):TEXT("smoke_perf_failed"));
        UE_LOG(LogTemp,Display,TEXT("DIVE_PERF_%s directory=%s"),valid?TEXT("CAPTURE_READY"):TEXT("FAIL"),*PerformanceDirectory);
        FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
    }
#if WITH_EDITOR
    if(PerformancePhase==4&&now-PerformanceStarted>1)
    {
        FScreenshotRequest::RequestScreenshot(PerformanceDirectory/TEXT("Frame_NoSuit.png"),true,false);
        PerformancePhase=5;PerformanceStarted=now;
    }
    if(PerformancePhase==5&&now-PerformanceStarted>2)
    {
        Equipment->Avatar->Suit->SetVisibility(true);
        FScreenshotRequest::RequestScreenshot(PerformanceDirectory/TEXT("Frame_Restored.png"),true,false);
        PerformancePhase=6;PerformanceStarted=now;
    }
    if(PerformancePhase==6&&now-PerformanceStarted>2)
    {
        bool valid=bSmokeChunkLimit;
        for(const TCHAR* name:{TEXT("Frame.png"),TEXT("Frame_NoSuit.png"),TEXT("Frame_Restored.png"),TEXT("FPVGeometry.json")})
            valid&=IFileManager::Get().FileSize(*(PerformanceDirectory/name))>10000;
        UE_LOG(LogTemp,Display,TEXT("DIVE_FPV_PROBE_%s head_yaw=%.3f body_pitch=%.3f"),valid?TEXT("PASS"):TEXT("FAIL"),PhysicalPose.HeadYaw,PhysicalPose.BodyPitch);
        FinishDive(TEXT("smoke_fpv_probe"));
        UE_LOG(LogTemp,Display,TEXT("DIVE_PERF_%s directory=%s"),valid?TEXT("CAPTURE_READY"):TEXT("FAIL"),*PerformanceDirectory);
        FPlatformMisc::RequestExitWithStatus(false,valid?0:1);
    }
#endif
    if(PerformancePhase==2&&now-PerformanceCaptureStarted>PerformanceSeconds+30)
    { UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL CSV write timeout")); FPlatformMisc::RequestExitWithStatus(false,1); }
#else
    UE_LOG(LogTemp,Error,TEXT("DIVE_PERF_FAIL CSV profiler unavailable in this build"));
    FPlatformMisc::RequestExitWithStatus(false,1);
#endif
}
