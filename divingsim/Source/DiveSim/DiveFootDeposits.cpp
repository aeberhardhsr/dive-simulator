#include "DiveWorld.h"
#include "Simulation/FootDeposit.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"

bool FDiveTerrain::Sample(double X,double Y,float& Z) const
{
    if(!SampleMeasured(X,Y,Z)) return false;
    if(!DepositHeight) return true;
    const double x=FMath::FloorToDouble(X/100)*100,y=FMath::FloorToDouble(Y/100)*100;
    if(X==x&&Y==y) {Z+=DepositHeight(x,y,Z);return true;}
    // Use the actual rendered metre-grid triangles, including their base
    // elevations. Raw survey sampling remains available as SampleMeasured.
    auto vertex=[this](double px,double py,float& h)
    {if(!SampleMeasured(px,py,h)) return false;h+=DepositHeight(px,py,h);return true;};
    float a=0,b=0,c=0;const double tx=(X-x)/100,ty=(Y-y)/100;
    if(tx+ty<=1)
    {
        if(!vertex(x,y,a)||!vertex(x+100,y,b)||!vertex(x,y+100,c)) return true;
        Z=a+(b-a)*tx+(c-a)*ty;
    }
    else
    {
        if(!vertex(x+100,y+100,a)||!vertex(x,y+100,b)||!vertex(x+100,y,c)) return true;
        Z=a+(b-a)*(1-tx)+(c-a)*(1-ty);
    }
    return true;
}

const FDiveFootCell& ADiveWorld::FootCell(FIntPoint Key) const
{
    if(const auto* found=FootCells.Find(Key)) return *found;
    // Independent of loaded render components and the player's current entry.
    // Recomputing an evicted cell produces exactly the same deposit sources.
    if(FootCells.Num()>=25)
    {
        FIntPoint far=Key;double maximum=-1;
        for(const auto& item:FootCells)
        {const double d=FMath::Square(double(item.Key.X-Key.X))+FMath::Square(double(item.Key.Y-Key.Y));if(d>maximum){maximum=d;far=item.Key;}}
        FootCells.Remove(far);
    }
    TArray<FTransform> transforms;TArray<FDiveRock> rocks;
    BuildRockLayout(Key,transforms,rocks,true);
    auto& cell=FootCells.Add(Key);
    for(int i=0;i<rocks.Num();++i)
    {
        const auto& rock=rocks[i];if(rock.Rubble||rock.Variant<3) continue;
        const int index=cell.Rocks.Add(rock);
        const FVector axis=transforms[i].GetRotation().GetAxisX();
        cell.Bearings.Add(FMath::Atan2(axis.Y,axis.X));
        const double reach=rock.Radius*Dive::FootDeposit::Reach;
        for(int y=FMath::FloorToInt((rock.Position.Y-reach)/400);y<=FMath::FloorToInt((rock.Position.Y+reach)/400);++y)
        for(int x=FMath::FloorToInt((rock.Position.X-reach)/400);x<=FMath::FloorToInt((rock.Position.X+reach)/400);++x)
            cell.Bins.FindOrAdd(FIntPoint(x,y)).Add(index);
    }
    return cell;
}

float ADiveWorld::FootDepositHeight(double X,double Y,float Bed) const
{
    const FIntPoint key(FMath::RoundToInt(X/100),FMath::RoundToInt(Y/100));
    const FIntPoint owner(FMath::FloorToInt(X/9600),FMath::FloorToInt(Y/9600));
    if(const auto* cell=DepositVertices.Find(owner)) if(const float* found=cell->Heights.Find(key)) return *found;
    // Evict whole distant cells rather than invalidating every nearby height.
    // Each owner has at most 96*96 metre-grid vertices; no hidden growth.
    if(!DepositVertices.Contains(owner)&&DepositVertices.Num()>=25)
    {
        FIntPoint far=owner;double maximum=-1;
        for(const auto& item:DepositVertices)
        {const double d=FMath::Square(double(item.Key.X-owner.X))+FMath::Square(double(item.Key.Y-owner.Y));if(d>maximum){maximum=d;far=item.Key;}}
        DepositVertices.Remove(far);
    }
    const float height=ComputeFootDeposit(X,Y,Bed);
    DepositVertices.FindOrAdd(owner).Heights.Add(key,height);return height;
}

float ADiveWorld::ComputeFootDeposit(double X,double Y,float Bed) const
{
    using B=Dive::BedDistribution;
    const double depth=-Bed*.01;
    const double depthWeight=B::Smooth(2.5,4,depth)*(1-B::Smooth(35,55,depth));
    if(depthWeight<=0) return 0;
    // No raised vertices along mixed survey/coast triangles or data holes.
    float neighbours[3][3];
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        if(!Terrain.SampleMeasured(X+x*100,Y+y*100,neighbours[y+1][x+1])) return 0;
    const double slope=FMath::Sqrt(FMath::Square((neighbours[1][2]-neighbours[1][0])/200.)+
                                  FMath::Square((neighbours[2][1]-neighbours[0][1])/200.));
    const double slopeWeight=1-B::Smooth(.3,.65,slope);
    if(slopeWeight<=0) return 0;
    double clearance=1;
    for(bool shore:{false,true}) for(const auto& entry:Terrain.EntryList(shore))
        clearance=FMath::Min(clearance,B::Smooth(550,850,FVector::Dist2D(FVector(X,Y,0),entry.Position)));
    for(const auto& access:Accesses)
    {
        const FVector d=FVector(X,Y,0)-access.Origin;
        const double along=FVector::DotProduct(d,access.Forward);
        const double across=FMath::Abs(d.X*access.Forward.Y-d.Y*access.Forward.X);
        if(along>-800&&along<access.Length+800) clearance=FMath::Min(clearance,B::Smooth(800,1100,across));
    }
    if(clearance<=0) return 0;
    double height=0;
    constexpr double maxReach=337*Dive::FootDeposit::Reach;
    const FIntPoint bin(FMath::FloorToInt(X/400),FMath::FloorToInt(Y/400));
    for(int y=FMath::FloorToInt((Y-maxReach)/9600);y<=FMath::FloorToInt((Y+maxReach)/9600);++y)
    for(int x=FMath::FloorToInt((X-maxReach)/9600);x<=FMath::FloorToInt((X+maxReach)/9600);++x)
    {
        const auto& cell=FootCell(FIntPoint(x,y));
        if(const auto* found=cell.Bins.Find(bin)) for(const int index:*found)
        {
            const auto& rock=cell.Rocks[index];
            const double phase=B::Noise(rock.Position.X/711,rock.Position.Y/711)*2*PI;
            height=FMath::Max(height,Dive::FootDeposit::Height(X-rock.Position.X,Y-rock.Position.Y,rock.Radius,cell.Bearings[index],phase));
        }
    }
    return height*depthWeight*slopeWeight*clearance;
}

bool ADiveWorld::TestFootDeposits(FString& Detail) const
{
    if(!Terrain.DepositHeight) return true;
    int probes=0,hits=0,raised=0,edges=0;double maximum=0;
    bool valid=true;
    TArray<FVector> rebuildSamples;
    int sources=0;
    for(const auto& rock:RockState) if(rock.Variant>=3)
    {
        const auto& cell=FootCell(FIntPoint(FMath::FloorToInt(rock.Position.X/9600),FMath::FloorToInt(rock.Position.Y/9600)));
        const bool found=cell.Rocks.ContainsByPredicate([&](const FDiveRock& source)
        {return source.Position==rock.Position&&source.Radius==rock.Radius&&source.Variant==rock.Variant;});
        valid&=found;++sources;
    }
    for(const auto& item:Chunks)
    {
        auto* component=item.Value.Get();
        for(int y=3;y<64;y+=5) for(int x=3;x<64;x+=5) for(int side=0;side<2;++side)
        {
            const double px=item.Key.X*6400.+x*100+(side?69:31),py=item.Key.Y*6400.+y*100+(side?77:23);
            float expected=0,measured=0;
            if(!Terrain.Sample(px,py,expected)||!Terrain.SampleMeasured(px,py,measured)) continue;
            float a=0,b=0,c=0;
            const double vx=item.Key.X*6400.+x*100,vy=item.Key.Y*6400.+y*100;
            // A raw survey interpolation can exist where a render triangle has a hole;
            // the separate coastal mesh owns the mixed-data boundary.
            if(!Terrain.SampleMeasured(vx+(side?100:0),vy+(side?100:0),a)||
               !Terrain.SampleMeasured(vx+100,vy,b)||!Terrain.SampleMeasured(vx,vy+100,c))
            {++edges;valid&=FMath::Abs(expected-measured)<.001;continue;}
            FHitResult hit;FCollisionQueryParams query;
            ++probes;
            const bool contact=component->LineTraceComponent(hit,FVector(px,py,expected+150),FVector(px,py,expected-150),query);
            if(contact&&FMath::Abs(hit.ImpactPoint.Z-expected)<.1) ++hits;
            else
            {
                UE_LOG(LogTemp,Display,TEXT("DIVE_FOOT_CONTACT_DIAGNOSTIC x=%.3f y=%.3f expected=%.3f hit=%d actual=%.3f"),px,py,expected,contact?1:0,hit.ImpactPoint.Z);
            }
            const double gx=FMath::FloorToDouble(px/100)*100,gy=FMath::FloorToDouble(py/100)*100;
            float raw=0;Terrain.SampleMeasured(gx,gy,raw);
            const double offset=FootDepositHeight(gx,gy,raw);
            valid&=offset>=0&&offset<=Dive::FootDeposit::Maximum+.001;
            raised+=offset>5;maximum=FMath::Max(maximum,offset);
            if(offset>5&&rebuildSamples.Num()<64) rebuildSamples.Add(FVector(gx,gy,offset));
        }
    }
    int cached=0;for(const auto& cell:DepositVertices) {cached+=cell.Value.Heights.Num();valid&=cell.Value.Heights.Num()<=96*96;}
    valid&=sources>0&&probes>100&&hits==probes&&raised>0&&FootCells.Num()<=25&&DepositVertices.Num()<=25;
    FootCells.Reset();DepositVertices.Reset();
    bool rebuilt=true;
    for(const auto& p:rebuildSamples)
    {float raw=0;rebuilt&=Terrain.SampleMeasured(p.X,p.Y,raw)&&FootDepositHeight(p.X,p.Y,raw)==p.Z;}
    valid&=rebuilt;
    Detail=FString::Printf(TEXT("probes=%d contacts=%d raised=%d maximum_cm=%.3f cached_vertices=%d unchanged_edges=%d rebuild_equal=%d source_cores=%d"),probes,hits,raised,maximum,cached,edges,rebuilt?1:0,sources);
    UE_LOG(LogTemp,Display,TEXT("DIVE_FOOT_DEPOSIT_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*Detail);
    return valid;
}
