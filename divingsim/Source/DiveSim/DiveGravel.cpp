#include "DiveWorld.h"
#include "DivePawn.h"
#include "Simulation/BedDistribution.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

namespace
{
constexpr double GravelWidth=1600;
constexpr int GravelCandidates=8192;
constexpr int GravelBins=4;
constexpr double GravelBinWidth=GravelWidth/GravelBins;
FIntPoint GravelKey(const FVector& P)
{return FIntPoint(FMath::FloorToInt(P.X/GravelWidth),FMath::FloorToInt(P.Y/GravelWidth));}
}

void ADiveWorld::CreateGravelCell(FIntPoint Key)
{
    auto& cell=GravelCells.Add(Key);
    const bool aprons=!(bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveNoRockAprons")));
    const int candidates=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveSparseGravel"))?1024:GravelCandidates;
    TArray<FTransform> instances[3];
    // Conservative 4 m bins retain source order and the exact contact tests.
    // A denser bed should not scan every rock in the 16 m cell per pebble.
    TArray<const FDiveRock*> nearby[GravelBins*GravelBins];
    for(const auto& rock:RockState)
    {
        const double reach=rock.Radius>=126?rock.Radius*1.6+60:rock.Radius+25;
        if(rock.Position.X>=Key.X*GravelWidth-reach&&rock.Position.X<=(Key.X+1)*GravelWidth+reach&&
            rock.Position.Y>=Key.Y*GravelWidth-reach&&rock.Position.Y<=(Key.Y+1)*GravelWidth+reach)
        {
            const int x0=FMath::Clamp(FMath::FloorToInt((rock.Position.X-reach-Key.X*GravelWidth)/GravelBinWidth),0,GravelBins-1);
            const int x1=FMath::Clamp(FMath::FloorToInt((rock.Position.X+reach-Key.X*GravelWidth)/GravelBinWidth),0,GravelBins-1);
            const int y0=FMath::Clamp(FMath::FloorToInt((rock.Position.Y-reach-Key.Y*GravelWidth)/GravelBinWidth),0,GravelBins-1);
            const int y1=FMath::Clamp(FMath::FloorToInt((rock.Position.Y+reach-Key.Y*GravelWidth)/GravelBinWidth),0,GravelBins-1);
            for(int by=y0;by<=y1;++by) for(int bx=x0;bx<=x1;++bx) nearby[by*GravelBins+bx].Add(&rock);
        }
    }
    FRandomStream random(Dive::BedDistribution::Hash(Key.X,Key.Y)^0x35ad902bu);
    // Independent fixed candidates: material patches control density, while
    // random positions and skewed sizes avoid a regular gravel lattice.
    for(int i=0;i<candidates;++i)
    {
        const double x=(Key.X+random.FRand())*GravelWidth,y=(Key.Y+random.FRand())*GravelWidth;
        const float chance=random.FRand(),size=.035f+.21f*FMath::Square(random.FRand());
        const float yaw=random.FRand()*2*PI,aspect=random.FRandRange(.65f,1.f),height=random.FRandRange(.35f,.65f);
        const int variant=random.RandRange(0,2);
        float z,n,e;
        if(!Terrain.Sample(x,y,z)||z>-120||z<-4000) continue;
        const double gravel=Dive::BedDistribution::Gravel(x*.01,y*.01,-z*.01);
        bool blocked=false;double apron=0;
        const int bx=FMath::Clamp(FMath::FloorToInt((x-Key.X*GravelWidth)/GravelBinWidth),0,GravelBins-1);
        const int by=FMath::Clamp(FMath::FloorToInt((y-Key.Y*GravelWidth)/GravelBinWidth),0,GravelBins-1);
        for(const auto* rock:nearby[by*GravelBins+bx])
        {
            const double distanceSquared=FVector::DistSquared2D(rock->Position,FVector(x,y,z));
            // The movement/steering radius encloses the whole boulder, not its
            // partly buried contact footprint. Dressing may approach the mesh;
            // hidden pebbles are harmless, while a full-radius cut leaves halos.
            if(distanceSquared<FMath::Square(aprons?rock->Radius*.45+size*50:rock->Radius+size*100)) {blocked=true;break;}
            if(aprons&&rock->Radius>=126&&distanceSquared<FMath::Square(rock->Radius*1.6+60))
            {
                const double distance=FMath::Sqrt(distanceSquared);
                const double band=Dive::BedDistribution::Smooth(rock->Radius*.45,rock->Radius*.85,distance)*
                    (1-Dive::BedDistribution::Smooth(rock->Radius*1.05,rock->Radius*1.6+60,distance));
                apron=FMath::Max(apron,band);
            }
        }
        if(blocked) continue;
        // Broken skirts of rubble tie outcrops to the bed without filling every
        // sediment patch. The bounded candidates and bed-relative relief stay
        // unchanged; this is authored dressing, not a measured erosion model.
        const double broken=.2+.8*Dive::BedDistribution::Noise(x/230,y/230);
        const double density=FMath::Max(gravel*gravel*.9,apron*broken*.85*(1-Dive::BedDistribution::Smooth(25,40,-z*.01)));
        if(chance>density) continue;
        if(!Terrain.Sample(x+50,y,e)||!Terrain.Sample(x,y+50,n)) continue;
        const FVector normal=FVector(-(e-z)/50,-(n-z)/50,1).GetSafeNormal();
        if(normal.Z<.72) continue;
        const FQuat rotation=FRotationMatrix::MakeFromZ(normal).ToQuat()*FQuat(FVector::UpVector,yaw);
        // Partly buried, flattened stones keep relief within six centimetres
        // of the surveyed bed, including beneath bottom-dwelling fish.
        const FVector position=FVector(x,y,z);
        instances[variant].Add(FTransform(rotation,position,FVector(size,size*aspect,FMath::Min(size*height,.06f))));
    }
    for(int i=0;i<3;++i)
    {
        auto* component=NewObject<UInstancedStaticMeshComponent>(this);
        component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
        component->SetStaticMesh(Gravel[i]->GetStaticMesh());component->SetMaterial(0,Gravel[i]->GetMaterial(0));
        component->SetCollisionProfileName(TEXT("BlockAll"));component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        component->SetCanEverAffectNavigation(false);component->SetCullDistances(1200,1500);
        component->AddInstances(instances[i],false,false,false);
        component->RegisterComponent();cell.Components.Add(component);
    }
}

void ADiveWorld::StreamGravel(const FVector& Player,bool Immediate)
{
    const auto centre=GravelKey(Player);
    if(centre==GravelCell&&GravelCells.Num()==9) return;
    // Entry placement and teleports need a complete window immediately. During
    // swimming only one new distant cell is built per frame; the 15 m draw
    // range remains inside the retained cells at a 16 m boundary crossing.
    Immediate|=GravelCells.IsEmpty()||FMath::Abs(int64(centre.X)-GravelCell.X)>1||FMath::Abs(int64(centre.Y)-GravelCell.Y)>1;
    const double started=FPlatformTime::Seconds();int loaded=0,unloaded=0,count=0;
    for(auto it=GravelCells.CreateIterator();it;++it)
        if(FMath::Abs(it.Key().X-centre.X)>1||FMath::Abs(it.Key().Y-centre.Y)>1)
        {
            for(const auto& component:it.Value().Components) component->DestroyComponent();
            it.RemoveCurrent();++unloaded;
        }
    TArray<FIntPoint> missing;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        if(!GravelCells.Contains(centre+FIntPoint(x,y))) missing.Add(centre+FIntPoint(x,y));
    missing.Sort([&Player](FIntPoint A,FIntPoint B)
    {
        return FVector::DistSquared2D(Player,FVector((A.X+.5)*GravelWidth,(A.Y+.5)*GravelWidth,0))<
            FVector::DistSquared2D(Player,FVector((B.X+.5)*GravelWidth,(B.Y+.5)*GravelWidth,0));
    });
    for(const auto& key:missing)
    {CreateGravelCell(key);++loaded;if(!Immediate) break;}
    GravelCell=centre;
    for(const auto& pair:GravelCells) for(const auto& component:pair.Value.Components) count+=component->GetInstanceCount();
    UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_CELL x=%d y=%d loaded=%d unloaded=%d active=%d stones=%d milliseconds=%.3f"),
        centre.X,centre.Y,loaded,unloaded,GravelCells.Num(),count,(FPlatformTime::Seconds()-started)*1000);
}

bool ADiveWorld::ValidateGravel(FString& Error) const
{
    if(Gravel.Num()!=3||GravelCells.IsEmpty()||GravelCells.Num()>9) {Error=TEXT("Invalid gravel window");return false;}
    int total=0;
    for(const auto& pair:GravelCells)
    {
        if(FMath::Abs(pair.Key.X-GravelCell.X)>1||FMath::Abs(pair.Key.Y-GravelCell.Y)>1)
        {Error=TEXT("Gravel cell outside streaming window");return false;}
        if(pair.Value.Components.Num()!=3) {Error=TEXT("Incomplete gravel cell");return false;}
        for(int i=0;i<3;++i)
        {
            const auto* component=pair.Value.Components[i].Get();
            const auto mesh=component?component->GetStaticMesh():nullptr;
            if(!mesh||mesh->GetName()!=FString::Printf(TEXT("SM_Gravel%d"),i+1)||mesh->GetNumLODs()!=4||
                component->GetCollisionEnabled()!=ECollisionEnabled::QueryOnly)
            {Error=TEXT("Invalid gravel mesh or contact configuration");return false;}
            total+=component->GetInstanceCount();
        }
    }
    if(total>9*GravelCandidates) {Error=TEXT("Gravel candidate bound exceeded");return false;}
    return true;
}

bool ADiveWorld::TestGravelStreaming(FString& Detail)
{
    const FVector original=Diver?Diver->GetActorLocation():PopulationOrigin;
    const auto centre=GravelKey(original);
    const FVector before((centre.X+.5)*GravelWidth,(centre.Y+.5)*GravelWidth,original.Z);
    StreamGravel(before,true);
    auto capture=[](const FDivePopulationCell& cell)
    {
        TArray<FTransform> result;
        for(const auto& component:cell.Components) for(int i=0;i<component->GetInstanceCount();++i)
        {FTransform value;component->GetInstanceTransform(i,value);result.Add(value);}
        return result;
    };
    TMap<FIntPoint,TArray<FTransform>> snapshots;
    TMap<FIntPoint,UInstancedStaticMeshComponent*> identities;
    int contacts=0,edgeStones=0;
    for(const auto& pair:GravelCells)
    {
        snapshots.Add(pair.Key,capture(pair.Value));identities.Add(pair.Key,pair.Value.Components[0]);
        for(const auto& transform:snapshots[pair.Key]) for(const auto& rock:RockState)
            if(rock.Radius>=126&&FVector::DistSquared2D(transform.GetLocation(),rock.Position)<FMath::Square(rock.Radius))
            {++edgeStones;break;}
        for(const auto& component:pair.Value.Components) for(int i=0;i<FMath::Min(8,component->GetInstanceCount());++i)
        {
            FTransform value;component->GetInstanceTransform(i,value,true);
            FHitResult hit;FCollisionQueryParams query;
            const FVector p=value.GetLocation();
            if(component->LineTraceComponent(hit,p+FVector(0,0,30),p-FVector(0,0,50),query)) ++contacts;
        }
    }
    auto same=[&capture](const FDivePopulationCell& cell,const TArray<FTransform>& expected)
    {
        const auto values=capture(cell);if(values.Num()!=expected.Num()) return false;
        for(int i=0;i<values.Num();++i) if(!values[i].Equals(expected[i],.00001)) return false;
        return true;
    };
    bool valid=true;int retained=0,reloaded=0;
    StreamGravel(before+FVector(GravelWidth,0,0));
    valid&=GravelCells.Num()==7;
    StreamGravel(before+FVector(GravelWidth,0,0));
    valid&=GravelCells.Num()==8;
    StreamGravel(before+FVector(GravelWidth,0,0));
    valid&=GravelCells.Num()==9;
    for(const auto& pair:GravelCells) if(const auto* snapshot=snapshots.Find(pair.Key))
    {valid&=identities[pair.Key]==pair.Value.Components[0]&&same(pair.Value,*snapshot);++retained;}
    StreamGravel(before+FVector(GravelWidth*5,0,0));
    for(const auto& pair:snapshots) valid&=!GravelCells.Contains(pair.Key);
    StreamGravel(before);
    for(const auto& pair:snapshots)
    {const auto* cell=GravelCells.Find(pair.Key);if(cell&&same(*cell,pair.Value)) ++reloaded;else valid=false;}
    valid&=retained==6&&reloaded==9&&GravelCells.Num()==9;
    int stones=0;for(const auto& pair:snapshots) stones+=pair.Value.Num();
    valid&=stones==0||contacts>0;
    StreamGravel(original);
    Detail=FString::Printf(TEXT("retained=%d reloaded=%d cells=%d stones=%d contacts=%d staged=7,8,9 edge_stones=%d"),retained,reloaded,GravelCells.Num(),stones,contacts,edgeStones);
    UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*Detail);
    return valid;
}
