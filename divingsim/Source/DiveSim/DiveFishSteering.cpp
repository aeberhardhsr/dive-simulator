#include "DiveWorld.h"
#include "Simulation/Ecology.h"

double ADiveWorld::TraceFishPath(int32 Species,float Scale,const FVector& Start,const FVector& End) const
{
    if(Start.ContainsNaN()||End.ContainsNaN()) return 0;
    const auto& profile=Dive::FishProfiles[Species];
    const double bodyRadius=profile.LengthMetres*50*Scale;
    const FVector delta=End-Start;const double distance=delta.Size();
    double fraction=1;
    for(const auto& rock:RockState)
    {
        const double radius=rock.Radius+bodyRadius;
        if(FVector::DistSquared(Start,rock.Position)>FMath::Square(radius+distance)) continue;
        const double hit=Dive::FishSteering::SphereFraction(
            {Start.X,Start.Y,Start.Z},{End.X,End.Y,End.Z},{rock.Position.X,rock.Position.Y,rock.Position.Z},radius);
        fraction=FMath::Min(fraction,hit);
    }
    // Quarter-metre samples along the entire path catch banks, habitat changes
    // and missing survey data. Rock intersections above are analytic sweeps.
    const int steps=FMath::Max(1,FMath::CeilToInt(distance/25));double previous=0;
    for(int i=0;i<=steps;++i)
    {
        const double t=FMath::Min(double(i)/steps,fraction);const FVector p=Start+delta*t;
        float ground=0;
        if(!Terrain.Sample(p.X,p.Y,ground)) return previous;
        const auto band=Dive::FishDepthBand(profile,-ground/100,Scale);
        if(!band.Valid||-p.Z/100<band.Shallow-.00001||-p.Z/100>band.Deep+.00001) return previous;
        previous=t;if(t>=fraction) break;
    }
    return fraction;
}
