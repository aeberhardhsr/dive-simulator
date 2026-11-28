#pragma once
#include <algorithm>
#include <cmath>
#include "Weather.h"

namespace Dive
{
// Render parameters, not a measured radiative-transfer model. A half-metre
// camera transition softens wave crossings; body immersion/pressure are separate.
inline double CameraWaterBlend(double EyeDepthMetres)
{
    if(!std::isfinite(EyeDepthMetres)) return 0;
    const double t=std::clamp(EyeDepthMetres/.5,0.,1.);
    return t*t*(3-2*t);
}
struct VisualMedium
{
    double Water=0,Density=.002,HeightFalloff=.2;
    static VisualMedium At(double EyeDepthMetres,int Weather,double VisibilityMetres)
    {
        VisualMedium out;
        out.Water=CameraWaterBlend(EyeDepthMetres);
        const double visibility=std::isfinite(VisibilityMetres)?std::clamp(VisibilityMetres,2.,40.):14.;
        const double airDensity=WeatherScenario::At(Weather).AirFog;
        out.Density=airDensity*(1-out.Water)+(.35*14/visibility)*out.Water;
        out.HeightFalloff=.2*(1-out.Water)+.01*out.Water;
        return out;
    }
};
}
