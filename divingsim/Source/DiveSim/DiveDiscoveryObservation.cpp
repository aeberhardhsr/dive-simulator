#include "DiveWorld.h"
#include "DivePawn.h"
#include "DiveDiscoveries.h"
#include "DiveEquipment.h"
#include "Simulation/Ecology.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"

bool ADiveWorld::FindDiscoveryTestView(bool Plant,FVector& Eye,FRotator& View,FString& Species)
{
    if(!bOpticsSmoke||!Diver) return false;
    auto candidate=[&](const FVector& point,int index)
    {
        if(point.Z>-220||FVector::Dist2D(point,Diver->EntryLocation)>6500) return false;
        for(int angle=0;angle<8;++angle)
        {
            const FVector eye=point+FRotator(0,angle*45,0).Vector()*180+FVector(0,0,30);
            if(eye.Z<GroundZ(eye.X,eye.Y)+75) continue;
            const FVector forward=(point-eye).GetSafeNormal();const FString id=DiveDiscoveries::SpeciesId(index);
            if(!PhotographSpecies(eye,forward).Contains(id)) continue;
            // A paused camera relocation bypasses ordinary movement streaming.
            // Prepare the real terrain around its destination before photographing.
            StreamTerrain(eye,true);
            if(!PhotographSpecies(eye,forward).Contains(id)) continue;
            const auto before=Lighting;const bool lamp=Diver->Lamp->IsVisible();
            Lighting=Dive::EnvironmentLight::AtElevation(-30,0);Diver->Lamp->SetVisibility(false);
            const bool dark=PhotographSpecies(eye,forward).IsEmpty();
            const FTransform lampTransform=Diver->Lamp->GetRelativeTransform();const float intensity=Diver->Lamp->Intensity;
            Diver->Lamp->SetWorldLocationAndRotation(eye,forward.Rotation());Diver->Lamp->SetIntensity(1000);Diver->Lamp->SetVisibility(true);
            const bool torch=PhotographSpecies(eye,forward).Contains(id);
            Diver->Lamp->SetRelativeTransform(lampTransform);Diver->Lamp->SetIntensity(intensity);
            Lighting=before;Diver->Lamp->SetVisibility(lamp);
            const float visibility=VisibleDistance;VisibleDistance=.5;
            const bool fog=PhotographSpecies(eye,forward).IsEmpty();VisibleDistance=visibility;
            auto* blocker=NewObject<UBoxComponent>(this);blocker->SetBoxExtent(FVector(5,400,400));blocker->SetCollisionProfileName(TEXT("BlockAll"));
            blocker->RegisterComponent();blocker->SetWorldLocationAndRotation(eye+forward*40,forward.Rotation());
            const bool occluded=PhotographSpecies(eye,forward).IsEmpty();blocker->DestroyComponent();
            FDiveSilt puff;puff.Origin=puff.Position=eye+forward*40;puff.Model.Age=1;Silt.Add(puff);
            const bool silt=PhotographSpecies(eye,forward).IsEmpty();Silt.Pop();
            if(!dark||!torch||!fog||!occluded||!silt) return false;
            Eye=eye;View=forward.Rotation();Species=id;
            UE_LOG(LogTemp,Display,TEXT("DIVE_DISCOVERY_VISIBILITY_PASS species=%s dark=1 torch=1 fog=1 occluded=1 silt=1 natural_population=1"),*id);return true;
        }
        return false;
    };
    if(!Plant)
    { for(const auto& fish:FishState) if(fish.Active&&candidate(fish.Position,fish.Species)) return true; }
    else for(const auto& cell:PopulationCells)
        for(int species=0;species<3;++species)
        {
            auto* mesh=cell.Value.Components[species+1].Get();
            for(int i=0;i<mesh->GetInstanceCount();++i)
            {
                FTransform transform;mesh->GetInstanceTransform(i,transform,true);
                if(candidate(transform.TransformPosition(mesh->GetStaticMesh()->GetBounds().Origin),species+5)) return true;
            }
        }
    return false;
}

TArray<FString> ADiveWorld::PhotographSpecies(const FVector& Eye,const FVector& Forward) const
{
    TArray<FString> result;
    if(!Diver||Eye.Z>=SurfaceZ(Eye.X,Eye.Y)) return result;
    FCollisionQueryParams query(SCENE_QUERY_STAT(DivePhotograph),true,Diver);
    if(Diver->Equipment) query.AddIgnoredActor(Diver->Equipment);
    auto visible=[&](const FVector& point,double radius)
    {
        const FVector offset=(point-Eye)/100;
        if(!DiveDiscoveries::InFrame(offset,Forward,radius,VisibleDistance)) return false;
        bool lit=WaterAmbientAt(point)>=.01;
        if(!lit&&Diver->Lamp&&Diver->Lamp->IsVisible())
        {
            const FVector ray=(point-Diver->Lamp->GetComponentLocation())/100;
            const double range=ray.Size();
            const double solidAngle=2*PI*(1-FMath::Cos(FMath::DegreesToRadians(Diver->Lamp->OuterConeAngle)));
            const double illuminance=Diver->Lamp->Intensity/FMath::Max(.01,solidAngle)*FMath::Exp(-1.45*range/VisibleDistance)/FMath::Max(.01,range*range);
            lit=range>.1&&range<8&&illuminance>=5&&
                FVector::DotProduct(ray/range,Diver->Lamp->GetForwardVector())>=FMath::Cos(FMath::DegreesToRadians(Diver->Lamp->InnerConeAngle))&&
                !GetWorld()->LineTraceTestByChannel(Diver->Lamp->GetComponentLocation(),point,ECC_Visibility,query);
        }
        if(!lit||GetWorld()->LineTraceTestByChannel(Eye,point,ECC_Visibility,query)) return false;
        // Conservative rejection through a dense local sediment puff. These are
        // the same live puffs as the renderer, not an unrelated global flag.
        for(const auto& puff:Silt)
            if(puff.Model.OpticalDepth()>.3&&FMath::PointDistToSegment(puff.Position,Eye,point)<puff.Model.Radius()*100) return false;
        return true;
    };
    for(const auto& fish:FishState)
        if(fish.Active&&fish.Species>=0&&fish.Species<5&&!result.Contains(DiveDiscoveries::SpeciesId(fish.Species))&&
            visible(fish.Position,Dive::FishProfiles[fish.Species].LengthMetres*fish.Scale*.3)) result.Add(DiveDiscoveries::SpeciesId(fish.Species));
    for(const auto& cell:PopulationCells)
        for(int species=0;species<3;++species)
        {
            const FString id=DiveDiscoveries::SpeciesId(species+5);
            if(result.Contains(id)||!cell.Value.Components.IsValidIndex(species+1)) continue;
            auto* mesh=cell.Value.Components[species+1].Get();if(!mesh||!mesh->GetStaticMesh()) continue;
            const auto bounds=mesh->GetStaticMesh()->GetBounds();
            for(int i=0;i<mesh->GetInstanceCount();++i)
            {
                FTransform transform;if(!mesh->GetInstanceTransform(i,transform,true)) continue;
                const FVector center=transform.TransformPosition(bounds.Origin);
                const double radius=bounds.BoxExtent.GetMax()*transform.GetScale3D().GetMax()/100;
                if(visible(center,radius)) { result.Add(id);break; }
            }
        }
    result.Sort();return result;
}
