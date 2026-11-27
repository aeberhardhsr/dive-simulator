#include "DiveWorld.h"
#include "DivePawn.h"
#include "Simulation/Ecology.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "StaticMeshResources.h"

bool ADiveWorld::ValidateEcology(FString& Error) const
{
    if(!ValidateGravel(Error)) return false;
    if(RetiredPopulation.Num()>9*(Rocks.Num()+Plants.Num()))
    {Error=TEXT("Unbounded population retirement");return false;}
    for(const auto& component:RetiredPopulation)
        if(!component||component->IsVisible()||!component->IsRegistered())
        {Error=TEXT("Invalid pending population retirement");return false;}
    if(PopulationCells.IsEmpty()||PopulationCells.Num()>9) { Error=TEXT("Invalid population window size");return false; }
    const FVector player=Diver?Diver->GetActorLocation():PopulationOrigin;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
    {
        const auto key=PopulationCell+FIntPoint(x,y);
        if(PopulationCells.Contains(key)) continue;
        const FVector closest(FMath::Clamp(player.X,key.X*9600.,(key.X+1)*9600.),
            FMath::Clamp(player.Y,key.Y*9600.,(key.Y+1)*9600.),player.Z);
        if(FVector::DistSquared2D(player,closest)<=8000.*8000.)
        {Error=TEXT("Missing population geometry inside the active fish radius");return false;}
    }
    for(const auto& pair:PopulationCells)
    {
        if(FMath::Abs(pair.Key.X-PopulationCell.X)>1||FMath::Abs(pair.Key.Y-PopulationCell.Y)>1)
        {Error=TEXT("Population cell outside the streaming window");return false;}
        if(pair.Value.Components.Num()!=Rocks.Num()+Plants.Num()) { Error=TEXT("Incomplete population cell");return false; }
        for(int i=0;i<pair.Value.Components.Num();++i)
            if(!IsValid(pair.Value.Components[i])||pair.Value.Components[i]->GetStaticMesh()!=(i<Rocks.Num()?Rocks[i]->GetStaticMesh():Plants[i-Rocks.Num()]->GetStaticMesh()))
            { Error=TEXT("Missing streamed population mesh");return false; }
    }
    for(int32 i=0;i<Rocks.Num();++i)
    {
        const auto mesh=Rocks[i]->GetStaticMesh();
        if(!mesh||mesh->GetName()!=FString::Printf(TEXT("SM_LakeRock%d"),i+1)||mesh->GetNumLODs()!=4)
        { Error=TEXT("Missing detailed rock variant or fallback LODs");return false; }
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
            if(lod.VertexBuffers.ColorVertexBuffer.GetNumVertices()==0||lod.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()<3)
            { Error=TEXT("Fish tint, animation weights or fin rig missing");return false; }
    }
    for(int32 species=0;species<Plants.Num();++species)
    {
        const auto mesh=Plants[species]->GetStaticMesh();
        if(!mesh||mesh->GetName()!=UTF8_TO_TCHAR(Dive::PlantProfiles[species].Mesh))
        { Error=TEXT("Missing vegetation mesh"); return false; }
        const auto material=Plants[species]->GetMaterial(0);
        if(mesh->GetNumLODs()!=4||!material||material->GetName()!=TEXT("M_AquaticPlant"))
        {Error=TEXT("Missing aquatic plant surface or LODs");return false;}
        const auto* render=mesh->GetRenderData();
        if(!render) {Error=TEXT("Missing aquatic plant render data");return false;}
        for(const auto& lod:render->LODResources)
            if(lod.VertexBuffers.ColorVertexBuffer.GetNumVertices()==0||lod.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords()<2)
            {Error=TEXT("Missing plant root weights or leaf surface coordinates");return false;}
    }
    for(const auto& fish:FishState)
    {
        if(!fish.Active) continue;
        float ground;
        if(fish.Position.ContainsNaN()||fish.Velocity.ContainsNaN()||!Terrain.Sample(fish.Position.X,fish.Position.Y,ground))
        { Error=TEXT("Fish outside measured water"); return false; }
        if(!FMath::IsFinite(fish.Steering.Yaw)||!FMath::IsFinite(fish.Steering.Pitch)||!FMath::IsFinite(fish.Steering.Speed)||
            fish.Steering.Speed<0||FMath::Abs(fish.Steering.Pitch)>.55001)
        { Error=TEXT("Invalid fish steering state");return false; }
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
        for(const int32 index:RockSearch.Query(fish.Position.X,fish.Position.Y,fish.Position.X,fish.Position.Y,bodyRadius+180))
        {
            const auto& rock=RockState[index];
            const FVector delta=fish.Position-rock.Position;
            const float range=rock.Radius+bodyRadius+180;
            if(delta.SizeSquared()<range*range)
                direction+=delta.GetSafeNormal()*3*(1-delta.Size()/range);
        }
        // Look ahead into the actual bathymetry before advancing the body.
        const auto oldFacing=fish.Steering.Facing();
        const FVector ahead=fish.Position+FVector(oldFacing[0],oldFacing[1],oldFacing[2])*(bodyRadius+speed*.6);
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
        }
        const FVector beforePosition=fish.Position;const double beforeYaw=fish.Steering.Yaw;
        const double turnRate=FMath::Clamp(1.7/FMath::Sqrt(profile.LengthMetres/.3)*(fleeing?2.2:1),.8,4.);
        const auto result=Dive::FishSteering::Advance(fish.Steering,
            {fish.Position.X/100,fish.Position.Y/100,fish.Position.Z/100},
            {direction.X,direction.Y,direction.Z},{0,double(Flow),0},speed/100,Dt,turnRate,fleeing?2.5:.8,
            [&](const auto& a,const auto& b){return TraceFishPath(fish.Species,fish.Scale,FVector(a[0],a[1],a[2])*100,FVector(b[0],b[1],b[2])*100);});
        fish.Position=FVector(result.Position[0],result.Position[1],result.Position[2])*100;
        fish.Velocity=FVector(result.Velocity[0],result.Velocity[1],result.Velocity[2])*100;
        if(bOpticsSmoke&&(result.Turn>turnRate*Dt+.00001||TraceFishPath(fish.Species,fish.Scale,beforePosition,fish.Position)<.999999))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_STEERING_FAIL species=%d turn=%.6f limit=%.6f"),fish.Species,result.Turn,turnRate*Dt);FPlatformMisc::RequestExitWithStatus(false,1);return; }
        const auto facing=fish.Steering.Facing();
        const auto rotation=FVector(facing[0],facing[1],facing[2]).Rotation();
        auto* component=Fish[fish.Species].Get();
        component->UpdateInstanceTransform(fish.Instance,FTransform(rotation,fish.Position,FVector(fish.Scale)),false,false,true);
        const FVector right=FRotationMatrix(rotation).GetUnitAxis(EAxis::Y);
        const FVector up=FRotationMatrix(rotation).GetUnitAxis(EAxis::Z);
        const double yawRate=Dt>0?Dive::FishSteering::Wrap(fish.Steering.Yaw-beforeYaw)/Dt:0;
        fish.Motion.Step(Dt,fish.Velocity.Size()/100,profile.CruiseSpeed,profile.TailHz,yawRate);
        component->SetCustomDataValue(fish.Instance,0,fish.Motion.Tail,false);
        component->SetCustomDataValue(fish.Instance,1,right.X,false);
        component->SetCustomDataValue(fish.Instance,2,right.Y,false);
        component->SetCustomDataValue(fish.Instance,3,fish.Phase,false);
        component->SetCustomDataValue(fish.Instance,4,profile.LengthMetres*7*fish.Scale*fish.Motion.TailAmplitude(),false);
        const float extra[]={float(right.Z),float(up.X),float(up.Y),float(up.Z),float(fish.Motion.Fin),float(fish.Motion.Effort),float(fish.Motion.Turn),float(profile.LengthMetres*100*fish.Scale)};
        for(int i=0;i<8;++i) component->SetCustomDataValue(fish.Instance,5+i,extra[i],false);
    }
    for(const auto& component:Fish) component->MarkRenderStateDirty();
}
