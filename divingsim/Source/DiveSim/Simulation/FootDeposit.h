#pragma once
#include "BedDistribution.h"

namespace Dive
{
// Authored deposits attached to existing outcrops, in centimetres. This is
// dressing of the surveyed bed, not a sediment transport or survey model.
struct FootDeposit
{
    static constexpr double Reach=2.35,Maximum=70;
    static double Height(double x,double y,double radius,double bearing,double phase)
    {
        if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(radius)||
           !std::isfinite(bearing)||!std::isfinite(phase)||radius<126||radius>337) return 0;
        const double along=(x*std::cos(bearing)+y*std::sin(bearing))/radius;
        const double across=(-x*std::sin(bearing)+y*std::cos(bearing))/radius;
        const double angle=std::atan2(across,along);
        const double lobe=.88+.12*std::sin(3*angle+phase);
        const double q=std::hypot(along/Reach,across/1.7)/lobe;
        const double taper=1-BedDistribution::Smooth(.18,1,q);
        return std::min(Maximum,radius*.23)*taper;
    }
};
}
