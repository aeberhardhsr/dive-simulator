#pragma once
#include "HeightGrid.h"
#include <algorithm>

namespace Dive
{
// Derived render/collision LOD; survey rasters remain unchanged. The outer
// boundary expands to the background vertex lattice, never shrinking a shore.
struct ShoreTransition
{
    double West,South,East,North;
    static constexpr double BandMetres=32;
    ShoreTransition(const SurfaceGrid& shore,const SurfaceGrid& land)
    {
        const double x=land.West+land.Spacing*.5,y=land.North-land.Spacing*.5,s=land.Spacing;
        West=x+std::floor((shore.West-x)/s)*s;
        East=x+std::ceil((shore.West+shore.Columns*shore.Spacing-x)/s)*s;
        South=y+std::floor((shore.North-shore.Rows*shore.Spacing-y)/s)*s;
        North=y+std::ceil((shore.North-y)/s)*s;
    }
    bool Contains(double east,double north,double margin=0) const
    { return east>=West+margin&&east<=East-margin&&north>=South+margin&&north<=North-margin; }
    double Weight(double east,double north) const
    {
        const double distance=std::min({east-West,East-east,north-South,North-north});
        const double t=std::clamp(distance/BandMetres,0.,1.);
        return t*t*(3-2*t);
    }
    bool Sample(const SurfaceGrid& shore,const SurfaceGrid& land,double east,double north,double& metres) const
    {
        if(!Contains(east,north)) return false;
        double coarse=0,fine=0;
        if(!land.SampleTriangle(east,north,coarse)) return false;
        const double x=std::clamp(east,shore.West+shore.Spacing*.5,
            std::nextafter(shore.West+(shore.Columns-.5)*shore.Spacing,shore.West));
        const double y=std::clamp(north,std::nextafter(shore.North-(shore.Rows-.5)*shore.Spacing,shore.North),shore.North-shore.Spacing*.5);
        if(!shore.Sample(x,y,fine)) return false;
        const double weight=Weight(east,north);
        metres=weight==0?coarse:weight==1?fine:coarse+(fine-coarse)*weight;
        return true;
    }
};
}
