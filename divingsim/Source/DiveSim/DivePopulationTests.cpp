#include "DiveWorld.h"
#include "DivePawn.h"
#include "Components/InstancedStaticMeshComponent.h"

bool ADiveWorld::TestPopulationStreaming(FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_TEST_BEGIN"));
    const FVector original=Diver?Diver->GetActorLocation():PopulationOrigin;
    const auto savedFish=FishState;const auto savedRandom=Random;const double savedRefill=LastFishRefill;
    bool valid=true;int retained=0,reloaded=0,nearFish=0,spawned=0;
    // Exercise the actual world pause gate, including the GPU instance values.
    if(Diver&&Diver->bDiveActive)
    {
        TArray<TArray<float>> gpu;for(const auto& component:Fish) gpu.Add(component->PerInstanceSMCustomData);
        const bool menu=Diver->bMenu;const auto clock=WorldTime;
        Diver->bMenu=true;Tick(.1f);Diver->bMenu=menu;
        bool paused=WorldTime==clock;
        for(int i=0;i<FishState.Num();++i)
            paused&=FishState[i].Motion.Tail==savedFish[i].Motion.Tail&&FishState[i].Motion.Fin==savedFish[i].Motion.Fin&&FishState[i].Position==savedFish[i].Position&&
                FishState[i].Steering.Yaw==savedFish[i].Steering.Yaw&&FishState[i].Steering.Pitch==savedFish[i].Steering.Pitch&&FishState[i].Steering.Speed==savedFish[i].Steering.Speed;
        for(int i=0;i<Fish.Num();++i) paused&=gpu[i]==Fish[i]->PerInstanceSMCustomData;
        UE_LOG(LogTemp,Display,TEXT("DIVE_FIN_PAUSE_%s gpu=1 phases=1 position=1"),paused?TEXT("PASS"):TEXT("FAIL"));
        valid&=paused;
    }
    else valid=false;
    // Exercise the portable steering against actual loaded bathymetry and rocks
    // without moving gameplay instances or changing the random stream.
    TSet<int32> tested;double largestTurn=0;
    for(const auto& fish:savedFish)
    {
        if(!fish.Active||tested.Contains(fish.Species)) continue;tested.Add(fish.Species);
        auto state=fish.Steering;Dive::FishSteering::Vec position={fish.Position.X/100,fish.Position.Y/100,fish.Position.Z/100};
        const auto desired=Dive::FishSteering::Scale(state.Facing(),-1);double turned=0;
        for(int step=0;step<60;++step)
        {
            const auto result=Dive::FishSteering::Advance(state,position,desired,{0,double(Flow),0},.5,.05,1.5,.8,
                [&](const auto& a,const auto& b){return TraceFishPath(fish.Species,fish.Scale,FVector(a[0],a[1],a[2])*100,FVector(b[0],b[1],b[2])*100);});
            valid&=result.Turn<=.075001;largestTurn=FMath::Max(largestTurn,result.Turn);turned+=result.Turn;
            const FVector a(position[0]*100,position[1]*100,position[2]*100),b(result.Position[0]*100,result.Position[1]*100,result.Position[2]*100);
            valid&=TraceFishPath(fish.Species,fish.Scale,a,b)>.999999;position=result.Position;
        }
        valid&=turned>.1;
    }
    valid&=tested.Num()>0;
    UE_LOG(LogTemp,Display,TEXT("DIVE_STEERING_WORLD_%s species=%d steps_per_species=60 max_turn=%.6f limit=0.075001"),valid?TEXT("PASS"):TEXT("FAIL"),tested.Num(),largestTurn);
    auto capture=[](const FDivePopulationCell& cell)
    {
        TArray<TArray<FTransform>> result;
        for(const auto& component:cell.Components)
        {
            auto& list=result.AddDefaulted_GetRef();
            for(int i=0;i<component->GetInstanceCount();++i)
            { FTransform transform;component->GetInstanceTransform(i,transform);list.Add(transform); }
        }
        return result;
    };
    auto same=[&](const FDivePopulationCell& cell,const TArray<TArray<FTransform>>& expected)
    {
        const auto actual=capture(cell);if(actual.Num()!=expected.Num()) return false;
        for(int c=0;c<actual.Num();++c)
        {
            if(actual[c].Num()!=expected[c].Num()) return false;
            for(int i=0;i<actual[c].Num();++i) if(!actual[c][i].Equals(expected[c][i],0)) return false;
        }
        return true;
    };
    FVector anchor=original;
    for(const auto& fish:FishState) if(fish.Active) { anchor=fish.Position;break; }
    // Cross a real cell boundary two centimetres apart, close to an active fish.
    // This fixture changes the stream query, not the player's gameplay position.
    const double boundary=FMath::RoundToDouble(anchor.X/9600)*9600;
    const FVector before(boundary-1,anchor.Y,anchor.Z),after(boundary+1,anchor.Y,anchor.Z);
    LastFishRefill=WorldTime;StreamPopulation(before);
    TMap<FIntPoint,TArray<TArray<FTransform>>> snapshots;
    TMap<FIntPoint,UInstancedStaticMeshComponent*> identities;
    for(const auto& pair:PopulationCells) { snapshots.Add(pair.Key,capture(pair.Value));identities.Add(pair.Key,pair.Value.Components[0]); }
    const auto beforeFish=FishState;
    LastFishRefill=WorldTime;StreamPopulation(after);
    for(const auto& pair:PopulationCells)
        if(const auto* expected=snapshots.Find(pair.Key))
        { ++retained;valid&=identities[pair.Key]==pair.Value.Components[0]&&same(pair.Value,*expected); }
    for(int i=0;i<beforeFish.Num();++i)
        if(beforeFish[i].Active&&FVector::Dist2D(beforeFish[i].Position,after)<7900)
        {
            ++nearFish;const auto& a=beforeFish[i];const auto& b=FishState[i];
            valid&=b.Active&&a.Position==b.Position&&a.Velocity==b.Velocity&&a.Home==b.Home&&
                a.Instance==b.Instance&&a.Motion.Tail==b.Motion.Tail&&a.Motion.Fin==b.Motion.Fin&&
                a.Motion.Effort==b.Motion.Effort&&a.Motion.Turn==b.Motion.Turn&&a.Phase==b.Phase&&a.Scale==b.Scale&&
                a.Steering.Yaw==b.Steering.Yaw&&a.Steering.Pitch==b.Steering.Pitch&&a.Steering.Speed==b.Steering.Speed;
        }
    LastFishRefill=WorldTime;StreamPopulation(after+FVector(38400,0,0));
    for(const auto& pair:snapshots) valid&=!PopulationCells.Contains(pair.Key);
    LastFishRefill=WorldTime;StreamPopulation(before);
    for(const auto& pair:snapshots)
    {
        const auto* cell=PopulationCells.Find(pair.Key);
        if(cell&&same(*cell,pair.Value)) ++reloaded;else valid=false;
    }
    valid&=PopulationCells.Num()==9&&retained==6&&reloaded==9&&nearFish>0;
    // Restore all gameplay state before the ordinary update/render of this frame.
    LastFishRefill=WorldTime;StreamPopulation(original);
    for(auto& fish:FishState) fish.Active=false;
    LastFishRefill=WorldTime-2;RefillFish(original,false);
    for(const auto& fish:FishState) if(fish.Active)
    {
        ++spawned;const double distance=FVector::Dist2D(fish.Position,original);
        valid&=distance>=6499.99&&distance<=7500.01;
    }
    valid&=spawned>0&&FishState.Num()==77;
    FishState=savedFish;Random=savedRandom;LastFishRefill=savedRefill;
    for(const auto& fish:FishState)
    {
        const auto facing=fish.Steering.Facing();
        Fish[fish.Species]->UpdateInstanceTransform(fish.Instance,
            FTransform(fish.Active?FVector(facing[0],facing[1],facing[2]).Rotation():FRotator::ZeroRotator,
                fish.Active?fish.Position:original,FVector(fish.Active?fish.Scale:0)),false,true,true);
    }
    Detail=FString::Printf(TEXT("retained_cells=%d reloaded_cells=%d unchanged_near_fish=%d distant_spawns=%d active_cells=%d slots=%d"),
        retained,reloaded,nearFish,spawned,PopulationCells.Num(),FishState.Num());
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_TEST_END"));
    return valid;
}
