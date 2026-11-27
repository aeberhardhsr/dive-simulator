#include "DiveWorld.h"
#include "DivePawn.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Misc/SecureHash.h"

bool ADiveWorld::TestPopulationStreaming(FString& Detail)
{
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_TEST_BEGIN"));
    const FVector original=Diver?Diver->GetActorLocation():PopulationOrigin;
    const auto savedFish=FishState;const auto savedRandom=Random;const double savedRefill=LastFishRefill;
    bool valid=true;int retained=0,reloaded=0,nearFish=0,spawned=0;
    // Cluster halos must produce exactly one owner, matching the rendered
    // instances and collision state, including across cell boundaries.
    TSet<FVector> positions;int sizes[3]={};int renderedRocks=0,plants=0,connectedLargePairs=0,rubble=0,rubbleContacts=0;
    int poseProbes=0,poseContacts=0,poseEmbedded=0;
    int rubbleProbes=0;TMap<FVector,FTransform> mainTransforms;
    for(const auto& pair:PopulationCells)
    {
        TArray<FTransform> transforms;TArray<FDiveRock> states;
        GenerateRocks(pair.Key,transforms,states);
        TArray<FTransform> freshTransforms;TArray<FDiveRock> freshStates;
        BuildRockLayout(pair.Key,freshTransforms,freshStates);
        valid&=transforms.Num()==freshTransforms.Num()&&states.Num()==freshStates.Num();
        for(int i=0;i<FMath::Min(transforms.Num(),freshTransforms.Num());++i)
            valid&=transforms[i].Equals(freshTransforms[i],0)&&states[i].Position==freshStates[i].Position&&
                states[i].Radius==freshStates[i].Radius&&states[i].Variant==freshStates[i].Variant&&states[i].Rubble==freshStates[i].Rubble;
        valid&=transforms.Num()==states.Num()&&states.Num()<=196*94;
        int instances[3]={};int contactSamples=0,poseSamples=0;
        for(int i=0;i<states.Num();++i)
        {
            const auto& rock=states[i];
            const auto scale=transforms[i].GetScale3D();
            if(!rock.Rubble) mainTransforms.Add(rock.Position,transforms[i]);
            // The source meshes fit a unit sphere. Rotation and nonuniform
            // scale must remain within the unchanged movement/entry envelope.
            valid&=!transforms[i].ContainsNaN()&&transforms[i].GetRotation().IsNormalized()&&
                scale.GetMin()>0&&scale.GetMax()*100<=rock.Radius;
            const int instance=instances[rock.Variant]++;
            valid&=FMath::FloorToInt(rock.Position.X/9600)==pair.Key.X&&FMath::FloorToInt(rock.Position.Y/9600)==pair.Key.Y;
            valid&=!positions.Contains(rock.Position)&&rock.Radius>=16&&rock.Radius<=337;
            positions.Add(rock.Position);++sizes[rock.Radius<65?0:rock.Radius<126?1:2];
            if(!rock.Rubble&&rock.Radius>=126&&poseSamples++<8)
            {
                ++poseProbes;float bed=0;
                const auto* component=pair.Value.Components[rock.Variant].Get();
                auto* body=component?component->GetBodyInstance(NAME_None,false,instance):nullptr;
                FHitResult upper,lower;
                const FVector above=rock.Position+FVector(0,0,400),below=rock.Position-FVector(0,0,400);
                if(body&&Terrain.Sample(rock.Position.X,rock.Position.Y,bed)&&
                    body->LineTrace(upper,above,below,false,false)&&body->LineTrace(lower,below,above,false,false))
                {
                    ++poseContacts;
                    if(lower.ImpactPoint.Z<=bed+2&&upper.ImpactPoint.Z>bed) ++poseEmbedded;
                }
            }
            if(rock.Rubble)
            {
                ++rubble;
                float bed=0;
                valid&=rock.Radius>=21&&rock.Radius<=68.251&&Terrain.Sample(rock.Position.X,rock.Position.Y,bed);
                valid&=FMath::Abs(rock.Position.Z-bed-transforms[i].GetScale3D().X*5)<.01;
                valid&=transforms[i].GetScale3D().Z>=.1099&&transforms[i].GetScale3D().Z<=.5851;
                if(contactSamples++<12||i%23==0)
                {
                    ++rubbleProbes;
                    const auto* component=pair.Value.Components[rock.Variant].Get();
                    auto* body=component?component->GetBodyInstance(NAME_None,false,instance):nullptr;
                    valid&=body!=nullptr;
                    FHitResult hit;
                    if(body&&body->LineTrace(hit,rock.Position+FVector(0,0,200),rock.Position-FVector(0,0,200),false,false)) ++rubbleContacts;
                }
            }
            if(rock.Radius>=150) for(int j=0;j<i;++j)
                if(states[j].Radius>=150&&FVector::Dist2D(rock.Position,states[j].Position)<.65*(rock.Radius+states[j].Radius))
                    ++connectedLargePairs;
            for(bool shore:{false,true}) for(const auto& entry:Terrain.EntryList(shore))
                valid&=FVector::Dist2D(rock.Position,entry.Position)>=350+rock.Radius;
        }
        for(int i=0;i<pair.Value.Components.Num();++i)
            if(i<Rocks.Num())
            {
                renderedRocks+=pair.Value.Components[i]->GetInstanceCount();
                valid&=Rocks[i]->GetMaterial(0)!=nullptr&&pair.Value.Components[i]->GetMaterial(0)==Rocks[i]->GetMaterial(0);
            }
            else {const int count=pair.Value.Components[i]->GetInstanceCount();plants+=count;valid&=count<=4096;}
    }
    valid&=positions.Num()==RockState.Num()&&renderedRocks==RockState.Num();
    FString plantDetail;valid&=TestPlantBeds(plantDetail);
    const bool rubbleDisabled=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveNoRockRubble"));
    valid&=rubbleDisabled?rubble==0:rubble>0;
    valid&=rubbleDisabled||(rubbleProbes>0&&rubbleContacts==rubbleProbes);
    UE_LOG(LogTemp,Display,TEXT("DIVE_RUBBLE_%s fragments=%d disabled=%d contacts=%d"),valid?TEXT("PASS"):TEXT("FAIL"),rubble,rubbleDisabled,rubbleContacts);
    valid&=connectedLargePairs>8;
    UE_LOG(LogTemp,Display,TEXT("DIVE_FORMATION_%s connected_large_pairs=%d"),connectedLargePairs>8?TEXT("PASS"):TEXT("FAIL"),connectedLargePairs);
    valid&=poseProbes>0&&poseContacts==poseProbes&&poseEmbedded==poseProbes;
    UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_POSE_%s legacy=%d rocks=%d envelope_preserved=1 probes=%d contacts=%d embedded=%d"),valid?TEXT("PASS"):TEXT("FAIL"),
        bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyRockPose")),renderedRocks,poseProbes,poseContacts,poseEmbedded);
    auto orderedStates=RockState;
    orderedStates.Sort([](const FDiveRock& A,const FDiveRock& B)
    {return A.Position.X==B.Position.X?A.Position.Y<B.Position.Y:A.Position.X<B.Position.X;});
    FSHA1 stateDigest;
    for(const auto& rock:orderedStates)
    {
        const double values[]={rock.Position.X,rock.Position.Y,rock.Position.Z,rock.Radius,double(rock.Variant),rock.Rubble?1.:0.};
        stateDigest.Update(reinterpret_cast<const uint8*>(values),sizeof(values));
    }
    stateDigest.Final();uint8 digest[20];stateDigest.GetHash(digest);
    UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_POSE_STATE sha1=%s"),*BytesToHex(digest,20));
    FSHA1 mainDigest;
    for(const auto& rock:orderedStates) if(!rock.Rubble)
    {
        const auto& transform=mainTransforms[rock.Position];
        const auto q=transform.GetRotation();const auto s=transform.GetScale3D();
        const double values[]={rock.Position.X,rock.Position.Y,rock.Position.Z,rock.Radius,double(rock.Variant),q.X,q.Y,q.Z,q.W,s.X,s.Y,s.Z};
        mainDigest.Update(reinterpret_cast<const uint8*>(values),sizeof(values));
    }
    mainDigest.Final();mainDigest.GetHash(digest);
    UE_LOG(LogTemp,Display,TEXT("DIVE_TALUS_MAIN_STATE rocks=%d sha1=%s"),mainTransforms.Num(),*BytesToHex(digest,20));
    UE_LOG(LogTemp,Display,TEXT("DIVE_TALUS_CONTACTS_%s probes=%d contacts=%d"),rubbleProbes==rubbleContacts?TEXT("PASS"):TEXT("FAIL"),rubbleProbes,rubbleContacts);
    auto boundedLayouts=[&]()
    {
        FString searchDetail;if(!TestRockSearch(searchDetail)) return false;
        if(RockLayouts.Num()>25) return false;
        for(const auto& pair:RockLayouts)
            if(FMath::Abs(pair.Key.X-PopulationCell.X)>2||FMath::Abs(pair.Key.Y-PopulationCell.Y)>2) return false;
        return true;
    };
    valid&=boundedLayouts();
    UE_LOG(LogTemp,Display,TEXT("DIVE_HABITAT_%s unique_rocks=%d rendered_rocks=%d small=%d medium=%d large=%d plants=%d"),
        valid?TEXT("PASS"):TEXT("FAIL"),positions.Num(),renderedRocks,sizes[0],sizes[1],sizes[2],plants);
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
                [&](const auto& a,const auto& b)
                {
                    const FVector start=FVector(a[0],a[1],a[2])*100,end=FVector(b[0],b[1],b[2])*100;
                    const double indexed=TraceFishPath(fish.Species,fish.Scale,start,end);
                    valid&=indexed==TraceFishPath(fish.Species,fish.Scale,start,end,true);return indexed;
                });
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
    for(int i=0;i<8&&PopulationCells.Num()!=9;++i) StreamPopulation(before);
    TMap<FIntPoint,TArray<TArray<FTransform>>> snapshots;
    TMap<FIntPoint,UInstancedStaticMeshComponent*> identities;
    for(const auto& pair:PopulationCells) { snapshots.Add(pair.Key,capture(pair.Value));identities.Add(pair.Key,pair.Value.Components[0]); }
    const auto beforeFish=FishState;
    LastFishRefill=WorldTime;StreamPopulation(after);
    valid&=boundedLayouts();
    bool staged=PopulationCells.Num()==7;
    StreamPopulation(after);staged&=PopulationCells.Num()==8;
    StreamPopulation(after);staged&=PopulationCells.Num()==9;
    valid&=staged;
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_STAGED_%s cells=7,8,9"),staged?TEXT("PASS"):TEXT("FAIL"));
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
    // An adjacent-cell teleport may land close to its far edge. Unlike an
    // ordinary boundary crossing, nearby missing geometry must load at once.
    LastFishRefill=WorldTime;StreamPopulation(after+FVector(18400,0,0));
    const bool nearbyTeleport=PopulationCells.Num()==9&&boundedLayouts();valid&=nearbyTeleport;
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_NEAR_TELEPORT_%s immediate=1"),nearbyTeleport?TEXT("PASS"):TEXT("FAIL"));
    LastFishRefill=WorldTime;StreamPopulation(after+FVector(38400,0,0));
    valid&=boundedLayouts()&&PopulationCells.Num()==9;
    for(const auto& pair:snapshots) valid&=!PopulationCells.Contains(pair.Key);
    LastFishRefill=WorldTime;StreamPopulation(before);
    valid&=boundedLayouts()&&PopulationCells.Num()==9;
    for(const auto& pair:snapshots)
    {
        const auto* cell=PopulationCells.Find(pair.Key);
        if(cell&&same(*cell,pair.Value)) ++reloaded;else valid=false;
    }
    valid&=PopulationCells.Num()==9&&retained==6&&reloaded==9&&nearFish>0;
    bool retirementValid=RetiredPopulation.IsEmpty(); // The return teleport flushes obsolete bodies.
    const bool instant=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveImmediatePopulationRetirement"));
    int maximumPending=0;
    for(int i=0;i<20;++i)
    {
        LastFishRefill=WorldTime;StreamPopulation(i%2?before:after);
        maximumPending=FMath::Max(maximumPending,RetiredPopulation.Num());
        retirementValid&=boundedLayouts()&&RetiredPopulation.Num()<=9*(Rocks.Num()+Plants.Num());
        for(const auto& component:RetiredPopulation) retirementValid&=!component->IsVisible();
    }
    for(int i=0;i<9*(Rocks.Num()+Plants.Num())+3;++i)
    {LastFishRefill=WorldTime;StreamPopulation(before);}
    retirementValid&=RetiredPopulation.IsEmpty()&&PopulationCells.Num()==9&&(instant?maximumPending==0:maximumPending>0);
    for(const auto& pair:snapshots)
    {const auto* cell=PopulationCells.Find(pair.Key);retirementValid&=cell&&same(*cell,pair.Value);}
    valid&=retirementValid;
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_RETIREMENT_%s maximum_pending=%d drained=%d immediate=%d layouts_equal=%d"),retirementValid?TEXT("PASS"):TEXT("FAIL"),maximumPending,RetiredPopulation.IsEmpty(),instant,retirementValid);
    // Restore all gameplay state before the ordinary update/render of this frame.
    LastFishRefill=WorldTime;StreamPopulation(original);
    for(int i=0;i<8&&PopulationCells.Num()!=9;++i) StreamPopulation(original);
    valid&=boundedLayouts();
    UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_LAYOUT_%s layouts=%d max=25 fresh_equal=1"),valid?TEXT("PASS"):TEXT("FAIL"),RockLayouts.Num());
    valid&=TestPlantBeds(plantDetail);
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
    FString gravelDetail;valid&=TestGravelStreaming(gravelDetail);
    Detail=FString::Printf(TEXT("retained_cells=%d reloaded_cells=%d unchanged_near_fish=%d distant_spawns=%d active_cells=%d slots=%d"),
        retained,reloaded,nearFish,spawned,PopulationCells.Num(),FishState.Num());
    UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_TEST_END"));
    return valid;
}
