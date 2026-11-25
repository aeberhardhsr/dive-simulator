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
    // SolarSource is the illuminant ABOVE the atmosphere, not ground irradiance.
    // Sky Atmosphere applies extinction, Earth shadow and the horizon itself.
    double ElevationDegrees=0,SolarSource=0,Diffuse=0;
    // One simulated second advances the local scenario clock by one second.
    // Use elapsed simulation time, so pause/menu stalls and completed dives freeze it.
    static double HourAt(double StartHour,double ElapsedSeconds)
    {
        if(!std::isfinite(StartHour)) StartHour=12;
        if(!std::isfinite(ElapsedSeconds)||ElapsedSeconds<0) ElapsedSeconds=0;
        const double start=std::fmod(std::fmod(StartHour,24.)+24.,24.);
        return std::fmod(start+std::fmod(ElapsedSeconds,86400.)/3600.,24.);
    }
    static EnvironmentLight At(double Hour,int Weather)
    {
        if(!std::isfinite(Hour)) Hour=12;
        Hour=std::fmod(std::fmod(Hour,24.)+24.,24.);
        const double height=std::sin((Hour-6)/12*3.14159265358979323846);
        const double day=std::max(0.,height);
        const auto scenario=WeatherScenario::At(Weather);
        const double elevation=65*height;
        // Authored surface ambient proxy for water scattering/exposure only.
        // Keep a small diffuse source at the horizon; let it decay through
        // twilight instead of abruptly reaching zero at sunset. This is not
        // a measured irradiance curve or a replacement for Sky Atmosphere.
        const double nightFade=std::clamp((elevation+18)/6,0.,1.);
        const double smoothFade=nightFade*nightFade*(3-2*nightFade);
        const double diffuse=elevation>=0 ? .006+.994*day : .006*std::exp(.7*elevation)*smoothFade;
        return {elevation,scenario.DirectLight,diffuse*scenario.DiffuseLight};
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
        // Fixed camera ISO 100, f/4, 1/60 s. Partial compensation preserves
        // the perceived decline in brightness at dusk and with depth. Full
        // inverse compensation washed out the atmospheric twilight sky.
        // The floor still bounds dark adaptation at 5.5 EV compensation.
        const double ambient=WaterAmbient(EyeDepthMetres,VisibilityMetres);
        return 8.5-std::log2(10000*std::pow(ambient,.6)+8);
    }
};
}
