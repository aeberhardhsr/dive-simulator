#include "DiveWorld.h"

bool ADiveWorld::TestRockSearch(FString& Detail) const
{
    bool valid=RockSearch.Size()==size_t(RockState.Num());
    int queries=0;int64 candidates=0,exhaustive=0;
    int missing=0,forceMismatch=0,traceMismatch=0;double forceDelta=0,traceDelta=0;
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
                {
                    const bool contained=std::binary_search(found.begin(),found.end(),j);
                    missing+=!contained;valid&=contained;
                }
            }
            for(const int j:found) {valid&=RockState.IsValidIndex(j);if(RockState.IsValidIndex(j)) fast+=contribution(RockState[j]);}
            // Optimized game code may round the two accumulation loops a few
            // double ULPs differently. Do not confuse that with a missing rock.
            valid&=(full-fast).Size()<=1.e-12&&std::adjacent_find(found.begin(),found.end())==found.end();
            forceMismatch+=full!=fast;forceDelta=FMath::Max(forceDelta,(full-fast).Size());
            const FVector end=p+FVector(1100,-900,170);
            const double indexed=TraceFishPath(0,1,p,end),reference=TraceFishPath(0,1,p,end,true);
            traceMismatch+=indexed!=reference;traceDelta=FMath::Max(traceDelta,FMath::Abs(indexed-reference));valid&=indexed==reference;
        }
    }
    if(RockState.IsEmpty())
    {
        // A surveyed window can legitimately contain no rocks. Check both a
        // local query and the complete-index fallback for stale retired IDs.
        queries=2;
        valid&=RockSearch.Query(PopulationCell.X*9600.,PopulationCell.Y*9600.,
            (PopulationCell.X+1)*9600.,(PopulationCell.Y+1)*9600.,260).empty();
        valid&=RockSearch.Query(0,0,0,0,0,true).empty();
    }
    else valid&=queries>0&&candidates<exhaustive/3;
    Detail=FString::Printf(TEXT("queries=%d candidates=%lld exhaustive=%lld rocks=%d force_tolerance=1e-12 exact_trace=1"),queries,candidates,exhaustive,RockState.Num());
    UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_SEARCH_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*Detail);
    UE_LOG(LogTemp,Display,TEXT("DIVE_ROCK_SEARCH_DIAGNOSTIC missing=%d force_mismatch=%d force_delta=%.17g trace_mismatch=%d trace_delta=%.17g"),missing,forceMismatch,forceDelta,traceMismatch,traceDelta);
    return valid;
}
