#include "DiveWorld.h"
#include "DivePawn.h"
#include "Simulation/BedDistribution.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(DiveGravel,true);

namespace
{
constexpr double GravelWidth=1600;
constexpr int GravelCandidates=8192;
constexpr int GravelBins=4;
constexpr double GravelBinWidth=GravelWidth/GravelBins;
FIntPoint GravelKey(const FVector& P)
{return FIntPoint(FMath::FloorToInt(P.X/GravelWidth),FMath::FloorToInt(P.Y/GravelWidth));}
}

void ADiveWorld::BeginGravelCell(FIntPoint Key)
{
    check(!GravelBuild.Active&&PendingGravel.Components.IsEmpty());
    GravelBuild=FDiveGravelBuild{};GravelBuild.Key=Key;GravelBuild.Active=true;
    GravelBuild.Aprons=!(bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveNoRockAprons")));
    GravelBuild.Candidates=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveSparseGravel"))?1024:GravelCandidates;

    // Conservative 4 m bins retain source order and the exact contact tests.
    // A denser bed should not scan every rock in the 16 m cell per pebble.
    auto& nearby=GravelBuild.Nearby;
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
            for(int by=y0;by<=y1;++by) for(int bx=x0;bx<=x1;++bx) nearby[by*GravelBins+bx].Add(rock);
        }
    }
    GravelBuild.Random.Initialize(Dive::BedDistribution::Hash(Key.X,Key.Y)^0x35ad902bu);
}

bool ADiveWorld::AdvanceGravelCell(bool Complete)
{
    CSV_SCOPED_TIMING_STAT(DiveGravel,BuildCell);
    check(GravelBuild.Active);
    const auto Key=GravelBuild.Key;
    auto& random=GravelBuild.Random;auto& instances=GravelBuild.Instances;auto& nearby=GravelBuild.Nearby;
    const bool aprons=GravelBuild.Aprons;
    if(GravelBuild.Next<GravelBuild.Candidates)
    {
        const double generatedAt=FPlatformTime::Seconds();
        const int end=Complete?GravelBuild.Candidates:FMath::Min(GravelBuild.Candidates,GravelBuild.Next+1024);
        GravelBatchCandidates+=end-GravelBuild.Next;
        // Independent fixed candidates: material patches control density, while
        // random positions and skewed sizes avoid a regular gravel lattice.
        for(int i=GravelBuild.Next;i<end;++i)
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
            for(const auto& rock:nearby[by*GravelBins+bx])
            {
                const double distanceSquared=FVector::DistSquared2D(rock.Position,FVector(x,y,z));
                // The movement/steering radius encloses the whole boulder, not its
                // partly buried contact footprint. Dressing may approach the mesh;
                // hidden pebbles are harmless, while a full-radius cut leaves halos.
                if(distanceSquared<FMath::Square(aprons?rock.Radius*.45+size*50:rock.Radius+size*100)) {blocked=true;break;}
                if(aprons&&rock.Radius>=126&&distanceSquared<FMath::Square(rock.Radius*1.6+60))
                {
                    const double distance=FMath::Sqrt(distanceSquared);
                    const double band=Dive::BedDistribution::Smooth(rock.Radius*.45,rock.Radius*.85,distance)*
                        (1-Dive::BedDistribution::Smooth(rock.Radius*1.05,rock.Radius*1.6+60,distance));
                    apron=FMath::Max(apron,band);
                }
            }
            if(blocked) continue;
            // Broken skirts of rubble tie outcrops to the bed without filling every
            // sediment patch. Relief is bounded relative to the surveyed bed;
            // this is authored dressing, not a measured erosion model.
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
        GravelBuild.GenerationMs+=(FPlatformTime::Seconds()-generatedAt)*1000;
        GravelBuild.Next=end;
        if(!Complete) return false;
    }
    for(int i=PendingGravel.Components.Num();i<3;++i)
    {
        double phase=FPlatformTime::Seconds();
        auto* component=NewObject<UInstancedStaticMeshComponent>(this);
        component->SetupAttachment(RootComponent);component->SetMobility(EComponentMobility::Static);
        component->SetStaticMesh(Gravel[i]->GetStaticMesh());component->SetMaterial(0,Gravel[i]->GetMaterial(0));
        component->SetCollisionProfileName(TEXT("BlockAll"));component->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        component->SetCanEverAffectNavigation(false);component->SetCullDistances(1200,1500);
        component->SetVisibility(false);
        GravelBuild.SetupMs+=(FPlatformTime::Seconds()-phase)*1000;phase=FPlatformTime::Seconds();
        {CSV_SCOPED_TIMING_STAT(DiveGravel,AddInstances);component->AddInstances(instances[i],false,false,false);}
        GravelBuild.AddMs+=(FPlatformTime::Seconds()-phase)*1000;phase=FPlatformTime::Seconds();
        {CSV_SCOPED_TIMING_STAT(DiveGravel,Register);component->RegisterComponent();}
        GravelBuild.RegisterMs+=(FPlatformTime::Seconds()-phase)*1000;
        PendingGravel.Components.Add(component);++GravelBatchComponents;
        if(!Complete) break;
    }
    if(PendingGravel.Components.Num()!=3) return false;
    for(const auto& component:PendingGravel.Components) component->SetVisibility(true);
    GravelCells.Add(Key,MoveTemp(PendingGravel));PendingGravel.Components.Reset();
    if(bOpticsSmoke) UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_BUILD x=%d y=%d generation_ms=%.3f setup_ms=%.3f add_ms=%.3f register_ms=%.3f"),Key.X,Key.Y,GravelBuild.GenerationMs,GravelBuild.SetupMs,GravelBuild.AddMs,GravelBuild.RegisterMs);
    GravelBuild=FDiveGravelBuild{};return true;
}

void ADiveWorld::CancelGravelCell()
{
    for(const auto& component:PendingGravel.Components) RetiredGravel.Add(component);
    PendingGravel.Components.Reset();GravelBuild=FDiveGravelBuild{};
}

void ADiveWorld::StreamGravel(const FVector& Player,bool Immediate)
{
    GravelBatchCandidates=0;GravelBatchComponents=0;
    const auto centre=GravelKey(Player);
    if(GravelBuild.Active&&(FMath::Abs(GravelBuild.Key.X-centre.X)>1||FMath::Abs(GravelBuild.Key.Y-centre.Y)>1)) CancelGravelCell();
    // Entry placement and teleports need a complete window immediately. During
    // swimming preparation is bounded to 1024 candidates or one component
    // per frame. Cells inside the 15 m draw range always finish immediately.
    Immediate|=GravelCells.IsEmpty()||FMath::Abs(int64(centre.X)-GravelCell.X)>1||FMath::Abs(int64(centre.Y)-GravelCell.Y)>1;
    const bool instantRetire=Immediate||(bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelRetirement")));
    auto retire=[this,instantRetire]()
    {
        CSV_SCOPED_TIMING_STAT(DiveGravel,Retire);
        const double started=FPlatformTime::Seconds();
        // Old cells are outside the 15 m render range. Hide their proxies now,
        // but spread body removal and the resulting Chaos work over frames.
        // Fast oscillation is bounded; entry/teleport requests flush immediately.
        const int count=FMath::Min(RetiredGravel.Num(),instantRetire?RetiredGravel.Num():FMath::Max(2,RetiredGravel.Num()-27));
        for(int i=0;i<count;++i) RetiredGravel[i]->DestroyComponent();
        RetiredGravel.RemoveAt(0,count,EAllowShrinking::No);
        if(count&&bOpticsSmoke) UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_RETIRE removed=%d pending=%d immediate=%d milliseconds=%.3f"),count,RetiredGravel.Num(),instantRetire,(FPlatformTime::Seconds()-started)*1000);
    };
    if(centre==GravelCell&&GravelCells.Num()==9) {retire();return;}
    CSV_SCOPED_TIMING_STAT(DiveGravel,Stream);
    const double started=FPlatformTime::Seconds();int loaded=0,unloaded=0,count=0;
    for(auto it=GravelCells.CreateIterator();it;++it)
        if(FMath::Abs(it.Key().X-centre.X)>1||FMath::Abs(it.Key().Y-centre.Y)>1)
        {
            for(const auto& component:it.Value().Components)
            {component->SetVisibility(false);RetiredGravel.Add(component);}
            it.RemoveCurrent();++unloaded;
        }
    retire();
    TArray<FIntPoint> missing;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        if(!GravelCells.Contains(centre+FIntPoint(x,y))) missing.Add(centre+FIntPoint(x,y));
    missing.Sort([&Player](FIntPoint A,FIntPoint B)
    {
        return FVector::DistSquared2D(Player,FVector((A.X+.5)*GravelWidth,(A.Y+.5)*GravelWidth,0))<
            FVector::DistSquared2D(Player,FVector((B.X+.5)*GravelWidth,(B.Y+.5)*GravelWidth,0));
    });
    auto near=[&Player](FIntPoint key)
    {
        const FVector closest(FMath::Clamp(Player.X,key.X*GravelWidth,(key.X+1)*GravelWidth),
            FMath::Clamp(Player.Y,key.Y*GravelWidth,(key.Y+1)*GravelWidth),Player.Z);
        return FVector::DistSquared2D(Player,closest)<=1500.*1500.;
    };
    // Finish every potentially visible cell before returning, including after
    // short teleports. Only distant preparation may span multiple frames.
    for(const auto key:missing) if(Immediate||near(key))
    {
        if(GravelBuild.Active&&GravelBuild.Key!=key) {CancelGravelCell();retire();}
        if(!GravelBuild.Active) BeginGravelCell(key);
        if(AdvanceGravelCell(true)) ++loaded;
    }
    if(!Immediate)
    {
        if(!GravelBuild.Active) for(const auto key:missing) if(!GravelCells.Contains(key)) {BeginGravelCell(key);break;}
        if(GravelBuild.Active)
        {
            const bool legacy=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelBuild"));
            if(AdvanceGravelCell(legacy)) ++loaded;
        }
    }
    CSV_CUSTOM_STAT(DiveGravel,BuildCandidates,GravelBatchCandidates,ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(DiveGravel,BuildComponents,GravelBatchComponents,ECsvCustomStatOp::Set);
    GravelCell=centre;
    for(const auto& pair:GravelCells) for(const auto& component:pair.Value.Components) count+=component->GetInstanceCount();
    UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_CELL x=%d y=%d loaded=%d unloaded=%d active=%d stones=%d milliseconds=%.3f"),
        centre.X,centre.Y,loaded,unloaded,GravelCells.Num(),count,(FPlatformTime::Seconds()-started)*1000);
}

bool ADiveWorld::ValidateGravel(FString& Error) const
{
    if(PendingGravel.Components.Num()>2||(!GravelBuild.Active&&!PendingGravel.Components.IsEmpty())) {Error=TEXT("Invalid staged gravel build");return false;}
    for(const auto& component:PendingGravel.Components)
        if(!component||component->IsVisible()||!component->IsRegistered()) {Error=TEXT("Invalid hidden gravel build");return false;}
    if(RetiredGravel.Num()>27) {Error=TEXT("Unbounded gravel retirement");return false;}
    for(const auto& component:RetiredGravel)
        if(!component||component->IsVisible()||!component->IsRegistered())
        {Error=TEXT("Invalid pending gravel retirement");return false;}
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
    const bool legacyBuild=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelBuild"));
    const auto edgePoint=before+FVector(GravelWidth*.501,0,0);
    int stagedSteps=0;bool sawSeven=false,sawEight=false;
    do
    {
        StreamGravel(edgePoint);++stagedSteps;
        sawSeven|=GravelCells.Num()==7;sawEight|=GravelCells.Num()==8;
        valid&=legacyBuild||(GravelBatchCandidates<=1024&&GravelBatchComponents<=1);
        FString issue;valid&=ValidateGravel(issue);
    } while(GravelCells.Num()!=9&&stagedSteps<40);
    const int expectedSteps=legacyBuild?3:3*((bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveSparseGravel"))?1:8)+3);
    valid&=sawSeven&&sawEight&&GravelCells.Num()==9&&stagedSteps==expectedSteps;
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
    bool retirementValid=RetiredGravel.IsEmpty(); // The far teleport above flushes old bodies.
    const bool instant=bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveImmediateGravelRetirement"));
    int maximumPending=0;
    // Stress rapid reversals at a cell edge, then drain without moving. Pending
    // components must stay hidden and the complete layout must recover exactly.
    for(int i=0;i<20;++i)
    {
        const FVector p=before+FVector(i%2?0:GravelWidth,0,0);
        StreamGravel(p);
        maximumPending=FMath::Max(maximumPending,RetiredGravel.Num());
        retirementValid&=RetiredGravel.Num()<=27;
        for(const auto& component:RetiredGravel) retirementValid&=!component->IsVisible();
    }
    for(int i=0;i<64;++i) StreamGravel(before);
    retirementValid&=RetiredGravel.IsEmpty()&&GravelCells.Num()==9&&(instant?maximumPending==0:maximumPending>0);
    for(const auto& pair:snapshots)
    {const auto* cell=GravelCells.Find(pair.Key);retirementValid&=cell&&same(*cell,pair.Value);}
    valid&=retirementValid;
    UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_RETIREMENT_%s maximum_pending=%d drained=%d immediate=%d"),retirementValid?TEXT("PASS"):TEXT("FAIL"),maximumPending,RetiredGravel.IsEmpty(),instant);
    if(!legacyBuild)
    {
        // Abort after one hidden component has been registered, then verify
        // that the original complete window and all transforms are restored.
        for(int step=0;step<12&&PendingGravel.Components.IsEmpty();++step) StreamGravel(edgePoint);
        valid&=GravelBuild.Active&&PendingGravel.Components.Num()==1;
        StreamGravel(before,true);
        valid&=!GravelBuild.Active&&PendingGravel.Components.IsEmpty()&&RetiredGravel.IsEmpty();
        for(const auto& pair:snapshots)
        {const auto* cell=GravelCells.Find(pair.Key);valid&=cell&&same(*cell,pair.Value);}
    }
    // Move into a cell while it is being prepared. The complete near cell
    // must appear immediately, including all query collision bodies.
    StreamGravel(edgePoint);
    const auto nearPoint=before+FVector(GravelWidth*1.49,0,0);
    StreamGravel(nearPoint);
    const FIntPoint nearKey=centre+FIntPoint(2,0);
    valid&=GravelCells.Contains(nearKey)&&GravelCells[nearKey].Components.Num()==3;
    StreamGravel(original,true);
    UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_STAGING_%s steps=%d legacy=%d restored=%d"),valid?TEXT("PASS"):TEXT("FAIL"),stagedSteps,legacyBuild?1:0,GravelBuild.Active?0:1);
    Detail=FString::Printf(TEXT("retained=%d reloaded=%d cells=%d stones=%d contacts=%d completed=7,8,9 edge_stones=%d"),retained,reloaded,GravelCells.Num(),stones,contacts,edgeStones);
    UE_LOG(LogTemp,Display,TEXT("DIVE_GRAVEL_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*Detail);
    return valid;
}
