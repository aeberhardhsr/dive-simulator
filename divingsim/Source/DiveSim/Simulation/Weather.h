#pragma once
#include <algorithm>

namespace Dive
{
// Authored scenario values, not observations or a meteorological forecast.
// Weather selection supplies a wind preset; users can edit wind afterwards.
struct WeatherScenario
{
    static constexpr int Count=4;
    static constexpr double MaximumWind=24;
    double Wind,CloudCoverage,CloudDensity,DirectLight,DiffuseLight,AirFog,RainGain;
    int RainDrops;
    static WeatherScenario At(int Weather)
    {
        constexpr WeatherScenario values[]={
            {3,-.65,.008,1,1,.002,0,0},
            {5,.25,.008,.55,.65,.002,0,0},
            {8,.65,.012,.3,.4,.012,1,1600},
            {22,.9,.018,.12,.25,.020,1.4,2400}
        };
        return values[std::clamp(Weather,0,Count-1)];
    }
};
}
