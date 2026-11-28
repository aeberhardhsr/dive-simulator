#pragma once
#include "HeightGrid.h"
#include <algorithm>
#include <map>
#include <utility>

namespace Dive
{
// Source cells have a one-cell halo. Cores lie on the background vertex lattice.
// Interior tile borders never cause a height blend; only missing neighbouring
// cores define the outer boundary of the complete coastal surface.
struct CoastGrid
{
    static constexpr int Size=128;
    static constexpr double Offset=4, Band=32;
    std::map<std::pair<int,int>,SurfaceGrid> Tiles;
    static std::pair<int,int> Key(double east,double north)
    { return {static_cast<int>(std::floor((east-Offset)/Size)),static_cast<int>(std::floor((north-Offset)/Size))}; }
    bool Contains(double east,double north) const
    {
        if(!std::isfinite(east)||!std::isfinite(north)) return false;
        const auto key=Key(east,north);
        return Tiles.find(key)!=Tiles.end();
    }
    bool Sample(double east,double north,double& height) const
    {
        if(!std::isfinite(east)||!std::isfinite(north)) return false;
        const auto key=Key(east,north);
        // Closed boundary vertices can belong to a core on either side.
        for(int y=0;y>=-1;--y) for(int x=0;x>=-1;--x)
        {
            const auto found=Tiles.find({key.first+x,key.second+y});
            if(found==Tiles.end()) continue;
            const double west=Offset+found->first.first*Size,south=Offset+found->first.second*Size;
            if(east<west||east>west+Size||north<south||north>south+Size) continue;
            if(found->second.Sample(east,north,height)) return true;
        }
        return false;
    }
    static double RectangleDistance(double east,double north,double west,double south,double right,double top)
    {
        const double x=std::max({west-east,0.,east-right}),y=std::max({south-north,0.,north-top});
        return std::sqrt(x*x+y*y);
    }
    double BoundaryDistance(double east,double north) const
    {
        const auto key=Key(east,north); double distance=Band;
        for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x)
        {
            const std::pair<int,int> other(key.first+x,key.second+y);
            if(Tiles.find(other)!=Tiles.end()) continue;
            const double west=Offset+other.first*Size,south=Offset+other.second*Size;
            distance=std::min(distance,RectangleDistance(east,north,west,south,west+Size,south+Size));
        }
        return distance;
    }
    static double Weight(double distance)
    { const double t=std::clamp(distance/Band,0.,1.);return t*t*(3-2*t); }
};
}
