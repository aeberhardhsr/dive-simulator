#include "DiveWorld.h"
#include "Simulation/Ecology.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshResources.h"

bool ADiveWorld::ValidateEcology(FString& Error) const
{
    if(PopulationCells.Num()!=9) { Error=TEXT("Population window must contain nine cells");return false; }
    for(const auto& pair:PopulationCells)
    {
        if(pair.Value.Components.Num()!=4) { Error=TEXT("Incomplete population cell");return false; }
        for(int i=0;i<4;++i)
            if(!IsValid(pair.Value.Components[i])||pair.Value.Components[i]->GetStaticMesh()!=(i?Plants[i-1]->GetStaticMesh():Rocks->GetStaticMesh()))
            { Error=TEXT("Missing streamed population mesh");return false; }
    }
    for(int32 species=0;species<Fish.Num();++species)
    {
        const auto mesh=Fish[species]->GetStaticMesh();
        if(!mesh||mesh->GetName()!=UTF8_TO_TCHAR(Dive::FishProfiles[species].Mesh))
        { Error=TEXT("Missing species mesh"); return false; }
        const auto material=Fish[species]->GetMaterial(0);
        if(mesh->GetNumLODs()!=4||!material||material->GetName()!=TEXT("MI_Fish_")+mesh->GetName().RightChop(3))
        { Error=TEXT("Missing species PBR material or distance LODs");return false; }
        const auto* render=mesh->GetRenderData();
        if(!render) { Error=TEXT("Missing fish render data");return false; }
        for(const auto& lod:render->LODResources)
            if(lod.VertexBuffers.ColorVertexBuffer.GetNumVertices()==0)
            { Error=TEXT("Fish tint and animation weights missing");return false; }
    }
    for(int32 species=0;species<Plants.Num();++species)
    {
        const auto mesh=Plants[species]->GetStaticMesh();
        if(!mesh||mesh->GetName()!=UTF8_TO_TCHAR(Dive::PlantProfiles[species].Mesh))
        { Error=TEXT("Missing vegetation mesh"); return false; }
    }
    for(const auto& fish:FishState)
    {
        if(!fish.Active) continue;
        float ground;
        if(fish.Position.ContainsNaN()||fish.Velocity.ContainsNaN()||!Terrain.Sample(fish.Position.X,fish.Position.Y,ground))
        { Error=TEXT("Fish outside measured water"); return false; }
        const auto& profile=Dive::FishProfiles[fish.Species];
        const auto band=Dive::FishDepthBand(profile,-ground/100,fish.Scale);
        if(!band.Valid||-fish.Position.Z/100<band.Shallow-.001||-fish.Position.Z/100>band.Deep+.001)
        { Error=TEXT("Fish outside habitat depth"); return false; }
        for(const auto& rock:RockState)
            if(FVector::DistSquared(fish.Position,rock.Position)<FMath::Square(rock.Radius+profile.LengthMetres*50*fish.Scale)-.01)
            { Error=TEXT("Fish intersects rock exclusion volume"); return false; }
    }
    return true;
}

void ADiveWorld::TickFish(float Dt,const FVector& Player)
{
    // Read all neighbours from the same time step; iteration order cannot bias the school.
    const auto previous=FishState;
    for(auto& fish:FishState)
    {
        if(!fish.Active) continue;
        const auto& profile=Dive::FishProfiles[fish.Species];
        const float bodyRadius=profile.LengthMetres*50*fish.Scale;
        const FVector away=fish.Position-Player;
        const float distance=away.Size(), fear=profile.FearRadius*100;
        const bool fleeing=distance<fear;
        const float activity=profile.BottomDweller&&DayHour>7&&DayHour<19?.35f:1.f;
        const float speed=(fleeing?profile.EscapeSpeed:profile.CruiseSpeed*activity)*100;
        FVector desired=fish.Home+FVector(FMath::Sin(WorldTime*.06f+fish.Phase)*220,
            FMath::Cos(WorldTime*.05f+fish.Phase)*220,0)-fish.Position;
        FVector direction=desired.GetSafeNormal()*.35;
        FVector cohesion=FVector::ZeroVector, alignment=FVector::ZeroVector;
        int32 neighbours=0;
        for(const auto& other:previous)
        {
            if(!other.Active||(other.Species==fish.Species&&other.Instance==fish.Instance)) continue;
            const FVector delta=fish.Position-other.Position;
            const float d=delta.Size();
            const float spacing=bodyRadius+Dive::FishProfiles[other.Species].LengthMetres*50*other.Scale+12;
            if(d<spacing&&d>.1) direction+=delta/d*(1-d/spacing)*2;
            if(profile.Schooling&&other.Species==fish.Species&&d<700)
            { cohesion+=other.Position; alignment+=other.Velocity.GetSafeNormal(); ++neighbours; }
        }
        if(neighbours)
            direction+=(cohesion/neighbours-fish.Position).GetSafeNormal()*.55+alignment/neighbours*.8;
        if(fleeing) direction+=away.GetSafeNormal()*4;
        for(const auto& rock:RockState)
        {
            const FVector delta=fish.Position-rock.Position;
            const float range=rock.Radius+bodyRadius+180;
            if(delta.SizeSquared()<range*range)
                direction+=delta.GetSafeNormal()*3*(1-delta.Size()/range);
        }
        // Look ahead into the actual bathymetry before advancing the body.
        const FVector ahead=fish.Position+fish.Velocity.GetSafeNormal()*(bodyRadius+speed*.6);
        float ground;
        if(Terrain.Sample(ahead.X,ahead.Y,ground))
        {
            const auto band=Dive::FishDepthBand(profile,-ground/100,fish.Scale);
            if(band.Valid)
            {
                const double target=profile.BottomDweller?(band.Shallow+band.Deep)*-.5*100:
                    FMath::Clamp(fish.Home.Z,-band.Deep*100,-band.Shallow*100);
                direction.Z+=(target-fish.Position.Z)*.015;
            }
            else direction-=fish.Velocity.GetSafeNormal()*3;
        }
        else direction-=fish.Velocity.GetSafeNormal()*3;
        const FVector target=direction.GetSafeNormal()*speed;
        fish.Velocity=FMath::Lerp(fish.Velocity,target,FMath::Min(1.f,Dt*(fleeing?3.f:1.2f)));
        FVector next=fish.Position+(fish.Velocity+FVector(0,Flow*100,0))*Dt;
        bool valid=Terrain.Sample(next.X,next.Y,ground);
        if(valid)
        {
            const auto band=Dive::FishDepthBand(profile,-ground/100,fish.Scale);
            valid=band.Valid&&next.Z>=-band.Deep*100&&next.Z<=-band.Shallow*100;
        }
        for(const auto& rock:RockState)
            if(FVector::DistSquared(next,rock.Position)<FMath::Square(rock.Radius+bodyRadius)) { valid=false; break; }
        if(valid) fish.Position=next;
        else
        {
            // Do not teleport through a bank/rock or into missing survey data.
            fish.Velocity=-fish.Velocity;
            fish.Home=fish.Position+fish.Velocity.GetSafeNormal()*200;
        }
        const auto rotation=fish.Velocity.Rotation();
        auto* component=Fish[fish.Species].Get();
        component->UpdateInstanceTransform(fish.Instance,FTransform(rotation,fish.Position,FVector(fish.Scale)),false,false,true);
        const FVector right=FRotationMatrix(rotation).GetUnitAxis(EAxis::Y);
        fish.TailCycle=FMath::Fmod(fish.TailCycle+Dt*profile.TailHz*(fleeing?2.2:activity)*2*PI,2*PI);
        component->SetCustomDataValue(fish.Instance,0,fish.TailCycle,false);
        component->SetCustomDataValue(fish.Instance,1,right.X,false);
        component->SetCustomDataValue(fish.Instance,2,right.Y,false);
        component->SetCustomDataValue(fish.Instance,3,fish.Phase,false);
        component->SetCustomDataValue(fish.Instance,4,profile.LengthMetres*7*fish.Scale,false);
    }
    for(const auto& component:Fish) component->MarkRenderStateDirty();
}
