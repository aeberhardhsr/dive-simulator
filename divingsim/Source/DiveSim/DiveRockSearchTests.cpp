#include "DiveWorld.h"

bool ADiveWorld::TestRockSearch(FString& Detail) const
{
    bool valid=RockSearch.Size()==size_t(RockState.Num());
    int queries=0;int64 candidates=0,exhaustive=0;
    // Sample the entire target window, including the distant not-yet-rendered
    // cells of staged streaming. Independent brute-force reference stays here.
    for(int i=0;i<RockState.Num();i+=FMath::Max(1,RockState.Num()/64))
    {
        const auto& anchor=RockState[i];
        for(const FVector offset:{FVector::ZeroVector,FVector(337,0,80),FVector(-337,800,-80)})
        {
            const FVector p=anchor.Position+offset;
            constexpr float body=80;
            const auto found=RockSearch.Query(p.X,p.Y,p.X,p.Y,body+180);
            ++queries;candidates+=found.size();exhaustive+=RockState.Num();
            FVector full=FVector::ZeroVector,fast=FVector::ZeroVector;
            auto contribution=[&](const FDiveRock& rock)
            {
                const FVector delta=p-rock.Position;const float range=rock.Radius+body+180;
                return delta.SizeSquared()<range*range?delta.GetSafeNormal()*3*(1-delta.Size()/range):FVector::ZeroVector;
            };
            for(int j=0;j<RockState.Num();++j)
            {
                const auto& rock=RockState[j];full+=contribution(rock);
                if(FVector::DistSquared2D(p,rock.Position)<=FMath::Square(rock.Radius+body+180))
                    valid&=std::binary_search(found.begin(),found.end(),j);
            }
            for(const int j:found) {valid&=RockState.IsValidIndex(j);if(RockState.IsValidIndex(j)) fast+=contribution(RockState[j]);}
            valid&=full==fast&&std::adjacent_find(found.begin(),found.end())==found.end();
            const FVector end=p+FVector(1100,-900,170);
            valid&=TraceFishPath(0,1,p,end)==TraceFishPath(0,1,p,end,true);
        }
    }
    valid&=queries>0&&candidates<exhaustive/3;
    Detail=FString::Printf(TEXT("queries=%d candidates=%lld exhaustive=%lld rocks=%d exact_force=1 exact_trace=1"),queries,candidates,exhaustive,RockState.Num());
    UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_SEARCH_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*Detail);
    return valid;
}
