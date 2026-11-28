#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include "Weather.h"

namespace Dive
{
// Four linear deep-water components. This is a local wind-wave approximation,
// not a measured spectrum or a shallow-water/breaking-wave solver.
struct WaveComponent { double Kx=0,Ky=0,Omega=0,Amplitude=0,Phase=0; };
struct WaveSample { double Height=0,GradientX=0,GradientY=0,VerticalVelocity=0; };
struct WaterVelocity { double X=0,Y=0,Z=0; };
class WaterSurface
{
public:
    static constexpr double MaximumDisplacement=.0025*WeatherScenario::MaximumWind*WeatherScenario::MaximumWind*1.875;
    std::array<WaveComponent,4> Waves;
    void Configure(double Wind,double DirectionDegrees)
    {
        Wind=std::isfinite(Wind)?std::clamp(Wind,0.,WeatherScenario::MaximumWind):3.;
        DirectionDegrees=std::isfinite(DirectionDegrees)?DirectionDegrees:90.;
        constexpr double pi=3.14159265358979323846;
        constexpr std::array<double,4> lengths{20.,9.,4.5,2.1},weights{1.,.5,.25,.125},angles{0.,.28,-.43,.67},phases{.4,1.7,3.1,5.2};
        // Extend wavelengths along with amplitudes above the former 12 m/s
        // range, keeping each component's steepness within the existing bound.
        // This is an authored wind growth rule, not a fetch-limited wave spectrum.
        const double growth=std::max(1.,Wind/12.);
        for(int i=0;i<4;++i)
        {
            const double k=2*pi/(lengths[i]*growth*growth),angle=DirectionDegrees*pi/180+angles[i];
            Waves[i]={k*std::cos(angle),k*std::sin(angle),std::sqrt(9.80665*k),.0025*Wind*Wind*weights[i],phases[i]};
        }
    }
    WaveSample Sample(double X,double Y,double Seconds) const
    {
        WaveSample out;
        for(const auto& w:Waves)
        {
            const double p=w.Kx*X+w.Ky*Y-w.Omega*Seconds+w.Phase;
            const double c=w.Amplitude*std::cos(p);
            out.Height+=w.Amplitude*std::sin(p);
            out.GradientX+=w.Kx*c; out.GradientY+=w.Ky*c;
            out.VerticalVelocity-=w.Omega*c;
        }
        return out;
    }
    double PressureDepth(double X,double Y,double Z,double Seconds) const
    {
        // Wave-induced dynamic pressure decays with depth. Near the moving
        // interface blend to the local water column so air always has zero depth.
        const double meanDepth=std::max(0.,-Z);
        const double localDepth=Sample(X,Y,Seconds).Height-Z;
        if(localDepth<=0) return 0;
        double pressureDepth=-Z;
        for(const auto& w:Waves)
            pressureDepth+=w.Amplitude*std::sin(w.Kx*X+w.Ky*Y-w.Omega*Seconds+w.Phase)*
                std::exp(-std::hypot(w.Kx,w.Ky)*meanDepth);
        const double blend=std::clamp(meanDepth/2.,0.,1.);
        return std::max(0.,localDepth*(1-blend)+pressureDepth*blend);
    }
    WaterVelocity Velocity(double X,double Y,double Z,double Seconds) const
    {
        WaterVelocity out;
        for(const auto& w:Waves)
        {
            const double k=std::hypot(w.Kx,w.Ky);
            if(k<=0) continue;
            const double p=w.Kx*X+w.Ky*Y-w.Omega*Seconds+w.Phase;
            const double speed=w.Amplitude*w.Omega*std::exp(k*std::min(0.,Z));
            out.X+=speed*std::sin(p)*w.Kx/k;
            out.Y+=speed*std::sin(p)*w.Ky/k;
            out.Z-=speed*std::cos(p);
        }
        return out;
    }
    double AmplitudeBound() const
    { double sum=0; for(const auto& w:Waves) sum+=w.Amplitude; return sum; }
};
}
