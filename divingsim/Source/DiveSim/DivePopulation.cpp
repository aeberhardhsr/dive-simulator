#include "DiveWorld.h"
#include "Simulation/Ecology.h"
#include "Simulation/BedDistribution.h"
#include "Components/InstancedStaticMeshComponent.h"

namespace
{
constexpr double CellWidth=9600,RetireDistance=8000;
FIntPoint CellAt(const FVector& P) { return FIntPoint(FMath::FloorToInt(P.X/CellWidth),FMath::FloorToInt(P.Y/CellWidth)); }
uint32 CellSeed(FIntPoint Key) { return uint32(Key.X)*73856093u^uint32(Key.Y)*19349663u^1948u; }
}

void ADiveWorld::GenerateRocks(FIntPoint Key,TArray<FTransform>& Transforms,TArray<FDiveRock>& States) const
{
    // World-space anchors include a one-anchor halo. Ownership is assigned by
    // final rock position, so a cluster crosses streaming boundaries unchanged.
    constexpr int AnchorsPerCell=12;
    constexpr double AnchorWidth=CellWidth/AnchorsPerCell;
    for(int ay=Key.Y*AnchorsPerCell-1;ay<=(Key.Y+1)*AnchorsPerCell;++ay)
    for(int ax=Key.X*AnchorsPerCell-1;ax<=(Key.X+1)*AnchorsPerCell;++ax)
    {
        FRandomStream random(Dive::BedDistribution::Hash(ax,ay)^0x93e71bc5u);
        if(random.FRand()>.78f) continue;
        const FVector anchor((ax+.5)*AnchorWidth+random.FRandRange(-250.f,250.f),
            (ay+.5)*AnchorWidth+random.FRandRange(-250.f,250.f),0);
        const double bearing=random.FRand()*2*PI;
        TArray<FVector> accepted;TArray<double> radii;
        for(int i=0;i<18;++i)
        {
            const double angle=random.FRand()*2*PI,reach=i?100+FMath::Sqrt(random.FRand())*420:0;
            const double along=FMath::Cos(angle)*reach,across=FMath::Sin(angle)*reach*.65;
            const double x=anchor.X+along*FMath::Cos(bearing)-across*FMath::Sin(bearing);
            const double y=anchor.Y+along*FMath::Sin(bearing)+across*FMath::Cos(bearing);
            const float size=i==0?random.FRandRange(1.2f,2.4f):i<5?random.FRandRange(.5f,1.2f):random.FRandRange(.16f,.62f);
            const float yaw=random.FRand()*2*PI,aspect=random.FRandRange(.72f,1.f),height=random.FRandRange(.55f,.8f);
            const int32 variant=random.RandRange(0,Rocks.Num()-1);
            float z,n,e;
            if(!Terrain.Sample(x,y,z)||z>-250||z<-6000||!Terrain.Sample(x+100,y,e)||!Terrain.Sample(x,y+100,n)) continue;
            const FVector position(x,y,z+size*10);bool blocked=false;
            // Preserve entry and access routes for every size class.
            for(bool shore:{false,true}) for(const auto& entry:Terrain.EntryList(shore))
                if(FVector::Dist2D(position,entry.Position)<350+size*105) blocked=true;
            for(const auto& access:Accesses)
            {
                const FVector delta=position-access.Origin;
                const double alongRoute=FVector::DotProduct(delta,access.Forward);
                const double acrossRoute=FMath::Abs(delta.X*access.Forward.Y-delta.Y*access.Forward.X);
                if(alongRoute>-500-size*105&&alongRoute<access.Length+500+size*105&&acrossRoute<600+size*105) blocked=true;
            }
            // A little overlap allows interlocking rubble without stacked piles.
            for(int j=0;j<accepted.Num();++j)
                if(FVector::DistSquared2D(position,accepted[j])<FMath::Square(.7*(size*105+radii[j]))) blocked=true;
            if(blocked) continue;
            accepted.Add(position);radii.Add(size*105);
            if(CellAt(position)!=Key) continue;
            const FVector normal=FVector(-(e-z)/100,-(n-z)/100,1).GetSafeNormal();
            const FQuat rotation=FRotationMatrix::MakeFromZ(normal).ToQuat()*FQuat(FVector::UpVector,yaw);
            Transforms.Add(FTransform(rotation,position,FVector(size,size*aspect,size*height)));
            States.Add({position,size*105,variant});
        }
    }
}

void ADiveWorld::CreatePopulationCell(FIntPoint Key)
{
    auto& cell=PopulationCells.Add(Key);
    TArray<TArray<FTransform>> instances;instances.SetNum(Rocks.Num()+Plants.Num());
    auto create=[this,&cell](UInstancedStaticMeshComponent* source,bool collision)
    {
        auto* component=NewObject<UInstancedStaticMeshComponent>(this);
        component->SetupAttachment(RootComponent);component->SetStaticMesh(source->GetStaticMesh());
        component->SetMaterial(0,source->GetMaterial(0));
        component->SetCollisionProfileName(collision?TEXT("BlockAll"):TEXT("NoCollision"));
        cell.Components.Add(component);return component;
    };
    for(const auto& source:Rocks) create(source,true);
    for(const auto& source:Plants) create(source,false);
    TArray<FTransform> transforms;TArray<FDiveRock> states;
    GenerateRocks(Key,transforms,states);
    for(int32 i=0;i<transforms.Num();++i) instances[states[i].Variant].Add(transforms[i]);
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
    for(int i=0;i<4096;++i)
    {
        const double x=(Key.X+random.FRand())*CellWidth,y=(Key.Y+random.FRand())*CellWidth;
        const float size=random.FRandRange(.65f,1.2f),yaw=random.FRandRange(0.f,360.f),chance=random.FRand();
        float z,n,e;
        if(!Terrain.Sample(x,y,z)||!Terrain.Sample(x+100,y,n)||!Terrain.Sample(x,y+100,e)) continue;
        // Continuous patches replace the old repeated sine bands. Nearby
        // plants favour the same species, with scattered mixed growth at edges.
        const double patch=Dive::BedDistribution::Noise(x/900,y/900)+.25*Dive::BedDistribution::Noise(x/230,y/230);
        if(chance>Dive::BedDistribution::Smooth(.38,.78,patch)) continue;
        const int32 dominant=FMath::Min(Plants.Num()-1,int(Dive::BedDistribution::Noise(x/1400+31,y/1400-19)*Plants.Num()));
        const int32 species=i%5?dominant:i%Plants.Num();
        const float slope=FMath::Sqrt(FMath::Square(n-z)+FMath::Square(e-z))/100;
        if(!Dive::PlantHabitat(Dive::PlantProfiles[species],-z/100,slope,VisibleDistance)) continue;
        bool blocked=false;
        for(const auto& rock:nearby)
            if(FVector::DistSquared2D(rock.Position,FVector(x,y,z))<FMath::Square(rock.Radius)) { blocked=true;break; }
        if(!blocked) instances[species+Rocks.Num()].Add(FTransform(FRotator(0,yaw,0),FVector(x,y,z+2),FVector(size)));
    }
    for(int i=0;i<cell.Components.Num();++i)
    {
        cell.Components[i]->AddInstances(instances[i],false,false,false);
        cell.Components[i]->RegisterComponent();
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
            fish.Phase=Random.FRandRange(0.f,2*PI);fish.Motion={};
            fish.Motion.Tail=fish.Phase;fish.Motion.Fin=fish.Phase*.7;
            fish.Steering={FMath::Atan2(fish.Velocity.Y,fish.Velocity.X),0,profile.CruiseSpeed};
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
        int plantCount=0;
        for(const auto& pair:PopulationCells) for(int i=Rocks.Num();i<pair.Value.Components.Num();++i)
            plantCount+=pair.Value.Components[i]->GetInstanceCount();
        UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_CELL x=%d y=%d loaded=%d unloaded=%d active=%d rocks=%d plants=%d milliseconds=%.3f"),
            center.X,center.Y,loaded,unloaded,PopulationCells.Num(),RockState.Num(),plantCount,(FPlatformTime::Seconds()-started)*1000);
    }
    StreamGravel(Player,Initial);
    RefillFish(Player,Initial);
}
