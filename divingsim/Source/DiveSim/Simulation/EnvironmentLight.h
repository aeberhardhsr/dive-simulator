#pragma once
#include <algorithm>
#include <cmath>
#include "Weather.h"

namespace Dive
{
// Authored underwater ambient/exposure response to the native astronomical sky.
// Not a measured Walensee irradiance model.
struct EnvironmentLight
{
    static constexpr double SunLux=80000;
    // SolarSource is the illuminant ABOVE the atmosphere, not ground irradiance.
    // Sky Atmosphere applies extinction, Earth shadow and the horizon itself.
    double ElevationDegrees=0,SolarSource=0,Diffuse=0;
    static EnvironmentLight AtElevation(double elevation,int Weather,double MoonElevation=-90,double MoonLux=0)
    {
        if(!std::isfinite(elevation)) elevation=0;
        elevation=std::clamp(elevation,-90.,90.);
        const double day=std::max(0.,std::sin(elevation*3.14159265358979323846/180.));
        const auto scenario=WeatherScenario::At(Weather);
        // Authored surface ambient proxy for water scattering/exposure only.
        // Keep a small diffuse source at the horizon; let it decay through
        // twilight instead of abruptly reaching zero at sunset. This is not
        // a measured irradiance curve or a replacement for Sky Atmosphere.
        const double nightFade=std::clamp((elevation+18)/6,0.,1.);
        const double smoothFade=nightFade*nightFade*(3-2*nightFade);
        const double diffuse=elevation>=0 ? .006+.994*day : .006*std::exp(.7*elevation)*smoothFade;
        // MoonLux already includes lunar phase and the selected weather attenuation.
        const double moon=std::isfinite(MoonElevation)&&std::isfinite(MoonLux)?
            std::max(0.,std::sin(std::clamp(MoonElevation,-90.,90.)*3.14159265358979323846/180.))*std::clamp(MoonLux,0.,1.)/SunLux:0;
        return {elevation,scenario.DirectLight,diffuse*scenario.DiffuseLight+moon};
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
        // Bounded additional dark adaptation below civil twilight makes the
        // physically dim native stars/moon visible. This is an authored eye response.
        const double ambient=WaterAmbient(EyeDepthMetres,VisibilityMetres);
        const double night=std::clamp((-ElevationDegrees-6)/12,0.,1.);
        return 8.5-std::log2(10000*std::pow(ambient,.6)+8)+6*night*night*(3-2*night);
    }
};
}
