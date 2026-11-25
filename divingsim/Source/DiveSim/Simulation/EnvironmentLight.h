#pragma once
#include <algorithm>
#include <cmath>
#include "Weather.h"

namespace Dive
{
// Art-directed solar-day approximation: 06:00 sunrise, 18:00 sunset.
// This is not an astronomical ephemeris or measured Walensee irradiance.
struct EnvironmentLight
{
    static constexpr double SunLux=80000;
    double ElevationDegrees=0,Direct=0,Diffuse=0;
    static EnvironmentLight At(double Hour,int Weather)
    {
        if(!std::isfinite(Hour)) Hour=12;
        Hour=std::fmod(std::fmod(Hour,24.)+24.,24.);
        const double height=std::sin((Hour-6)/12*3.14159265358979323846);
        const double day=std::max(0.,height);
        const auto scenario=WeatherScenario::At(Weather);
        return {65*height,day*scenario.DirectLight,day*scenario.DiffuseLight};
    }
    double WaterAmbient(double EyeDepthMetres,double VisibilityMetres) const
    {
        const double depth=std::isfinite(EyeDepthMetres)?std::max(0.,EyeDepthMetres):0;
        const double visibility=std::isfinite(VisibilityMetres)?std::clamp(VisibilityMetres,2.,40.):14.;
        // Illustrative vertical attenuation, distinct from view-ray extinction.
        return Diffuse*std::exp(-depth*.06*14/visibility);
    }
    double ExposureBias(double EyeDepthMetres,double VisibilityMetres) const
    {
        // Fixed camera ISO 100, f/4, 1/60 s. Bound adaptation in darkness so
        // an unlit scene cannot be amplified without limit.
        return 8.5-std::log2(10000*WaterAmbient(EyeDepthMetres,VisibilityMetres)+8);
    }
};
}
