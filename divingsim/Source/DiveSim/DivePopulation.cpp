#include "DiveWorld.h"
#include "Simulation/Ecology.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
constexpr double CellWidth=9600,RetireDistance=8000;
FIntPoint CellAt(const FVector& P) { return FIntPoint(FMath::FloorToInt(P.X/CellWidth),FMath::FloorToInt(P.Y/CellWidth)); }
uint32 CellSeed(FIntPoint Key) { return uint32(Key.X)*73856093u^uint32(Key.Y)*19349663u^1948u; }
}

void ADiveWorld::GenerateRocks(FIntPoint Key,TArray<FTransform>& Transforms,TArray<FDiveRock>& States) const
{
    FRandomStream random(CellSeed(Key));
    for(int i=0;i<128;++i)
    {
        const double x=(Key.X+random.FRand())*CellWidth,y=(Key.Y+random.FRand())*CellWidth;
        const float size=random.FRandRange(.4f,2.6f);
        const FRotator rotation(random.FRandRange(-15.f,15.f),random.FRandRange(0.f,360.f),0);
        float z;if(!Terrain.Sample(x,y,z)||z>-250||z<-6000) continue;
        const FVector position(x,y,z+size*15);bool blocked=false;
        // Fixed entry/access clearances do not depend on the viewer or load order.
        for(bool shore:{false,true})
            for(const auto& entry:Terrain.EntryList(shore))
                if(FVector::Dist2D(position,entry.Position)<350+size*105) blocked=true;
        for(const auto& access:Accesses)
        {
            const FVector delta=position-access.Origin;
            const double along=FVector::DotProduct(delta,access.Forward);
            const double across=FMath::Abs(delta.X*access.Forward.Y-delta.Y*access.Forward.X);
            if(along>-500-size*105&&along<access.Length+500+size*105&&across<600+size*105) blocked=true;
        }
        if(blocked) continue;
        Transforms.Add(FTransform(rotation,position,FVector(size,size*.85,size*.7)));
        States.Add({position,size*105});
    }
}

void ADiveWorld::CreatePopulationCell(FIntPoint Key)
{
    auto& cell=PopulationCells.Add(Key);
    auto create=[this,&cell](UInstancedStaticMeshComponent* source,bool collision)
    {
        auto* component=NewObject<UInstancedStaticMeshComponent>(this);
        component->SetupAttachment(RootComponent);component->SetStaticMesh(source->GetStaticMesh());
        component->SetMaterial(0,source->GetMaterial(0));
        component->SetCollisionProfileName(collision?TEXT("BlockAll"):TEXT("NoCollision"));
        component->RegisterComponent();cell.Components.Add(component);return component;
    };
    auto* rocks=create(Rocks,true);
    for(const auto& source:Plants) create(source,false);
    TArray<FTransform> transforms;TArray<FDiveRock> states;
    GenerateRocks(Key,transforms,states);
    for(const auto& transform:transforms) rocks->AddInstance(transform);
    // Include neighbouring rock footprints when planting across a cell edge.
    // Only footprints close enough to this cell are kept for the inner loop.
    TArray<FDiveRock> nearby;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
    {
        transforms.Reset();states.Reset();GenerateRocks(Key+FIntPoint(x,y),transforms,states);
        for(const auto& rock:states)
            if(rock.Position.X>=Key.X*CellWidth-rock.Radius&&rock.Position.X<=(Key.X+1)*CellWidth+rock.Radius&&
               rock.Position.Y>=Key.Y*CellWidth-rock.Radius&&rock.Position.Y<=(Key.Y+1)*CellWidth+rock.Radius) nearby.Add(rock);
    }
    FRandomStream random(CellSeed(Key)^0x7a319fb1u);
    for(int i=0;i<648;++i)
    {
        const double x=(Key.X+random.FRand())*CellWidth,y=(Key.Y+random.FRand())*CellWidth;
        const float size=random.FRandRange(.65f,1.2f),yaw=random.FRandRange(0.f,360.f);
        float z,n,e;
        if(!Terrain.Sample(x,y,z)||!Terrain.Sample(x+100,y,n)||!Terrain.Sample(x,y+100,e)) continue;
        if(FMath::Sin(x*.002)+FMath::Cos(y*.003)<-.1) continue;
        const int32 species=i%Plants.Num();
        const float slope=FMath::Sqrt(FMath::Square(n-z)+FMath::Square(e-z))/100;
        if(!Dive::PlantHabitat(Dive::PlantProfiles[species],-z/100,slope,VisibleDistance)) continue;
        bool blocked=false;
        for(const auto& rock:nearby)
            if(FVector::DistSquared2D(rock.Position,FVector(x,y,z))<FMath::Square(rock.Radius)) { blocked=true;break; }
        if(!blocked) cell.Components[species+1]->AddInstance(FTransform(FRotator(0,yaw,0),FVector(x,y,z+2),FVector(size)));
    }
}

void ADiveWorld::RefillFish(const FVector& Player,bool Initial)
{
    for(auto& fish:FishState)
    {
        if(fish.Active&&FVector::Dist2D(fish.Position,Player)>RetireDistance)
        {
            fish.Active=false;
            Fish[fish.Species]->SetCustomDataValue(fish.Instance,4,0,false);
            Fish[fish.Species]->UpdateInstanceTransform(fish.Instance,FTransform(FRotator::ZeroRotator,fish.Position,FVector::ZeroVector),false,true,true);
        }
    }
    if(!Initial&&WorldTime-LastFishRefill<1) return;
    LastFishRefill=WorldTime;
    for(auto& fish:FishState)
    {
        if(fish.Active) continue;
        const auto& profile=Dive::FishProfiles[fish.Species];
        for(int attempt=0;attempt<30;++attempt)
        {
            const float scale=Random.FRandRange(.75f,1.15f);
            const double angle=Random.FRandRange(0.f,2*PI);
            const double radius=Random.FRandRange(6500.f,7500.f);
            FVector position=Player+FVector(FMath::Cos(angle)*radius,FMath::Sin(angle)*radius,0);
            if(Initial) position=Player+PopulationForward*1800+FVector(Random.FRandRange(-3500.f,3500.f),Random.FRandRange(-3500.f,3500.f),0);
            float ground;if(!Terrain.Sample(position.X,position.Y,ground)) continue;
            const auto band=Dive::FishDepthBand(profile,-ground/100,scale);
            if(!band.Valid) continue;
            position.Z=-100*Random.FRandRange(band.Shallow,band.Deep);
            bool blocked=false;
            for(const auto& rock:RockState)
                if(FVector::DistSquared(position,rock.Position)<FMath::Square(rock.Radius+profile.LengthMetres*50*scale)) { blocked=true;break; }
            if(blocked) continue;
            fish.Active=true;fish.Scale=scale;fish.Position=fish.Home=position;
            fish.Velocity=(Initial?PopulationForward:FVector(FMath::Cos(angle+PI),FMath::Sin(angle+PI),0))*profile.CruiseSpeed*100;
            fish.Phase=Random.FRandRange(0.f,2*PI);fish.TailCycle=fish.Phase;
            Fish[fish.Species]->UpdateInstanceTransform(fish.Instance,FTransform(fish.Velocity.Rotation(),position,FVector(scale)),false,true,true);
            break;
        }
    }
}

void ADiveWorld::StreamPopulation(const FVector& Player,bool Initial)
{
    const FIntPoint center=CellAt(Player);
    if(center!=PopulationCell)
    {
        const double started=FPlatformTime::Seconds();
        int loaded=0,unloaded=0;
        for(auto it=PopulationCells.CreateIterator();it;++it)
            if(FMath::Abs(it.Key().X-center.X)>1||FMath::Abs(it.Key().Y-center.Y)>1)
            {
                for(const auto& component:it.Value().Components) component->DestroyComponent();
                it.RemoveCurrent();++unloaded;
            }
        for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
            if(!PopulationCells.Contains(center+FIntPoint(x,y))) { CreatePopulationCell(center+FIntPoint(x,y));++loaded; }
        RockState.Reset();
        // Canonical order also keeps avoidance sums independent of travel direction.
        for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        {
            TArray<FTransform> transforms;GenerateRocks(center+FIntPoint(x,y),transforms,RockState);
        }
        PopulationCell=center;
        UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_CELL x=%d y=%d loaded=%d unloaded=%d active=%d rocks=%d milliseconds=%.3f"),
            center.X,center.Y,loaded,unloaded,PopulationCells.Num(),RockState.Num(),(FPlatformTime::Seconds()-started)*1000);
    }
    RefillFish(Player,Initial);
}
