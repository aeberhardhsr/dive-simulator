#include "DiveWorld.h"
#include "DivePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void ADiveWorld::CreateWaterVolume()
{
    LoadLakeFootprint();
    WaterVolume=NewObject<UStaticMeshComponent>(this,TEXT("BoundedWaterMedium"));
    WaterVolume->SetupAttachment(RootComponent);
    WaterVolume->SetMobility(EComponentMobility::Movable);
    WaterVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WaterVolume->SetCastShadow(false);
    WaterVolume->bAffectDistanceFieldLighting=false;
    WaterVolume->bAffectDynamicIndirectLighting=false;
    WaterVolume->SetVisibleInRayTracing(false);
    WaterVolume->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    WaterVolumeMaterial=UMaterialInstanceDynamic::Create(LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Diving/Materials/M_WaterVolume.M_WaterVolume")),this);
    if(LakeFootprint)
    {
        TArray<FMaterialParameterInfo> parameters;TArray<FGuid> ids;
        WaterVolumeMaterial->GetAllTextureParameterInfo(parameters,ids);
        if(!parameters.ContainsByPredicate([](const auto& p){return p.Name==TEXT("LakeGrid");}))
            UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL volume material must be rebuilt with OnlyWaterVolume"));
        WaterVolumeMaterial->SetTextureParameterValue(TEXT("LakeGrid"),LakeFootprint);
        WaterVolumeMaterial->SetVectorParameterValue(TEXT("LakeMap"),FLinearColor(LakeFootprintMap.X,LakeFootprintMap.Y,LakeFootprintMap.Z,LakeFootprintMap.W));
        WaterVolumeMaterial->SetScalarParameterValue(TEXT("LakeEnabled"),1);
        const bool surfaceEnabled=!FParse::Param(FCommandLine::Get(),TEXT("DiveLakeSurfaceOff"));
        int32 surfaceCount=0;
        for(auto* surface:{WaterMaterial.Get(),WaterUnderMaterial.Get()}) if(surface)
        {
            parameters.Reset();ids.Reset();surface->GetAllScalarParameterInfo(parameters,ids);
            if(!parameters.ContainsByPredicate([](const auto& p){return p.Name==TEXT("LakeSurfaceEnabled");}))
                UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_SURFACE_FAIL water surface materials must be rebuilt with OnlyWater"));
            surface->SetTextureParameterValue(TEXT("LakeGrid"),LakeFootprint);
            surface->SetVectorParameterValue(TEXT("LakeMap"),FLinearColor(LakeFootprintMap.X,LakeFootprintMap.Y,LakeFootprintMap.Z,LakeFootprintMap.W));
            surface->SetScalarParameterValue(TEXT("LakeSurfaceEnabled"),surfaceEnabled?1:0);
            ++surfaceCount;
        }
        UE_LOG(LogTemp,Display,TEXT("DIVE_LAKE_SURFACE_READY enabled=%d materials=%d contour=zero_distance"),surfaceEnabled,surfaceCount);
    }
    WaterVolume->SetMaterial(0,WaterVolumeMaterial);
    WaterVolume->RegisterComponent();
}

void ADiveWorld::UpdateWaterVolume(const FVector& Eye)
{
    if(!WaterVolume||!WaterVolumeMaterial||!WaterMaterial) return;
    // Cover the complete fog view distance without an artificial horizontal
    // edge. World-space extinction clips the box to the moving water surface.
    const double extent=(Fog?Fog->VolumetricFogDistance:FMath::Clamp(VisibleDistance*400.,2000.,16000.))+1000;
    const double top=Dive::WaterSurface::MaximumDisplacement*100+20;
    const double bottom=FMath::Min(Eye.Z-extent,-extent);
    WaterVolume->SetWorldLocation(FVector(Eye.X,Eye.Y,(top+bottom)*.5));
    WaterVolume->SetWorldScale3D(FVector(extent*2/100,extent*2/100,(top-bottom)/100));
    for(const FName name:{FName(TEXT("Viewer")),FName(TEXT("Wave0")),FName(TEXT("Wave1")),FName(TEXT("Wave2")),FName(TEXT("Wave3")),FName(TEXT("WavePhase"))})
        WaterVolumeMaterial->SetVectorParameterValue(name,WaterMaterial->K2_GetVectorParameterValue(name));
    WaterVolumeMaterial->SetScalarParameterValue(TEXT("WaveTime"),WaterMaterial->K2_GetScalarParameterValue(TEXT("WaveTime")));
    WaterVolumeMaterial->SetScalarParameterValue(TEXT("VisibilityMetres"),VisibleDistance);
    WaterVolumeMaterial->SetScalarParameterValue(TEXT("DensityGain"),WaterVolumeGain);
}
