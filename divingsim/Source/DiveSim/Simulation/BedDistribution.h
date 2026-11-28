#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace Dive
{
// Authored sediment patches, not measured Walensee substrate. Integer hashing
// matches BedSurface.PatchNoise in Lakebed.ush without GPU/CPU sin-hash drift.
struct BedDistribution
{
    static std::uint32_t Hash(std::int32_t x,std::int32_t y)
    {
        std::uint32_t h=std::uint32_t(x)*0x8da6b343u^std::uint32_t(y)*0xd8163841u;
        h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;return h;
    }
    static double Smooth(double a,double b,double x)
    {const double t=std::clamp((x-a)/(b-a),0.,1.);return t*t*(3-2*t);}
    static double Noise(double x,double y)
    {
        if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>1.e8||std::abs(y)>1.e8) return .5;
        const auto ix=std::int32_t(std::floor(x)),iy=std::int32_t(std::floor(y));
        const double fx=Smooth(0,1,x-ix),fy=Smooth(0,1,y-iy);
        auto value=[](std::int32_t a,std::int32_t b){return double(Hash(a,b)&0x00ffffffu)/16777216.;};
        const double a=value(ix,iy),b=value(ix+1,iy),c=value(ix,iy+1),d=value(ix+1,iy+1);
        return (a+(b-a)*fx)*(1-fy)+(c+(d-c)*fx)*fy;
    }
    static double Gravel(double xMetres,double yMetres,double depthMetres)
    {
        if(!std::isfinite(xMetres)||!std::isfinite(yMetres)||!std::isfinite(depthMetres)) return 0;
        const double patch=Noise(xMetres/8.5,yMetres/8.5)+.3*Noise(xMetres/2.7,yMetres/2.7);
        return Smooth(.35,.95,patch)*(1-Smooth(12,55,std::max(0.,depthMetres)));
    }
};
}
