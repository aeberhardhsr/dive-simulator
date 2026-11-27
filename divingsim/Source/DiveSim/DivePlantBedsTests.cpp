#include "DiveWorld.h"
#include "Simulation/Ecology.h"
#include "Components/InstancedStaticMeshComponent.h"

bool ADiveWorld::TestPlantBeds(FString& Detail) const
{
    const bool legacy=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveLegacyPlantPlacement"));
    bool valid=true;int count=0,shortPlants=0,tallPlants=0;TSet<FVector> roots;
    for(const auto& pair:PopulationCells)
    {
        // Use full neighbouring layouts here, independently of the production
        // footprint pruning. This catches missing halo blockers at cell edges.
        TArray<FDiveRock> nearby;TArray<FTransform> rocks;
        for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) GenerateRocks(pair.Key+FIntPoint(x,y),rocks,nearby);
        TArray<TArray<FTransform>> expected;expected.SetNum(Rocks.Num()+Plants.Num());
        BuildPlantLayout(pair.Key,nearby,expected);
        int cellCount=0;
        for(int species=0;species<Plants.Num();++species)
        {
            const auto* component=pair.Value.Components[species+Rocks.Num()].Get();
            const auto& generated=expected[species+Rocks.Num()];
            valid&=component&&component->GetInstanceCount()==generated.Num();
            if(!component) continue;
            for(int i=0;i<generated.Num();++i)
            {
                ++count;++cellCount;
                const auto& wanted=generated[i];const FVector p=wanted.GetLocation(),scale=wanted.GetScale3D();
                valid&=!roots.Contains(p);roots.Add(p);
                valid&=FMath::FloorToInt(p.X/9600)==pair.Key.X&&FMath::FloorToInt(p.Y/9600)==pair.Key.Y;
                float bed,n,e;const bool sampled=Terrain.Sample(p.X,p.Y,bed)&&Terrain.Sample(p.X+100,p.Y,n)&&Terrain.Sample(p.X,p.Y+100,e);
                valid&=sampled;if(!sampled) continue;
                valid&=FMath::Abs(p.Z-bed-2)<.001;
                valid&=Dive::PlantHabitat(Dive::PlantProfiles[species],-bed/100,FMath::Sqrt(FMath::Square(n-bed)+FMath::Square(e-bed))/100,VisibleDistance);
                FTransform actual;valid&=component->GetInstanceTransform(i,actual);
                // ISM instance storage uses float matrices, even in an LWC world.
                valid&=actual.GetLocation().Equals(p,.25)&&actual.GetScale3D().Equals(scale,1.e-5)&&actual.GetRotation().Equals(wanted.GetRotation(),1.e-5);
                if(!legacy)
                {
                    valid&=scale.X>=.6499&&scale.X<=1.1001&&scale.Y==scale.X&&scale.Z>=(species==1?.1799:.226)&&scale.Z<=1.1001;
                    valid&=-bed/100>=Dive::PlantProfiles[species].MaxHeight*scale.Z+.3;
                    for(bool shore:{false,true}) for(const auto& entry:Terrain.EntryList(shore)) valid&=FVector::DistSquared2D(p,entry.Position)>=350.*350.;
                }
                shortPlants+=scale.Z<.55;tallPlants+=scale.Z>.8;
                for(const auto& rock:nearby)
                    valid&=FVector::DistSquared2D(p,rock.Position)>=FMath::Square(rock.Radius+(legacy?0:10));
            }
        }
        valid&=cellCount<=(legacy?4096:26*26*18);
    }
    Detail=FString::Printf(TEXT("legacy=%d plants=%d short=%d tall=%d unique_roots=%d"),legacy,count,shortPlants,tallPlants,roots.Num());
    UE_LOG(LogTemp,Display,TEXT("DIVE_PLANT_BEDS_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*Detail);
    return valid;
}
