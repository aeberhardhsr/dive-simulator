#pragma once
#include "ScenarioClock.h"
#include "WalenseeTemperatureData.h"
#include <algorithm>
#include <cmath>

namespace Dive
{
enum class CurrentSetting { Custom=0, Automatic=1, None=2, Light=3, Strong=4 };

// Offline scenario model. Seasonal anchor dates and visibility/current rules are
// authored interpolation choices, not observations or a weather forecast.
struct EnvironmentScenario
{
    struct SeasonBlend { int First,Second; double Fraction; };
    static SeasonBlend Blend(CivilDate date)
    {
        if(!date.ScenarioRange()) date=CivilDate{};
        const int day=date.SerialDay();
        const CivilDate anchors[]={{date.Year-1,11,15},{date.Year,2,15},{date.Year,5,15},
            {date.Year,8,15},{date.Year,11,15},{date.Year+1,2,15}};
        for(int i=0;i<5;++i)
        {
            const int a=anchors[i].SerialDay(),b=anchors[i+1].SerialDay();
            if(day<=b) return {(i+3)%4,i%4,static_cast<double>(day-a)/(b-a)};
        }
        return {3,0,0};
    }
    static double Seasonal(CivilDate date,const double (&values)[4])
    {
        const auto s=Blend(date);
        return values[s.First]+(values[s.Second]-values[s.First])*s.Fraction;
    }
    static double Profile(int season,double depth)
    {
        using namespace WalenseeTemperatureData;
        depth=std::isfinite(depth)?std::clamp(depth,Depths[0],Depths[Count-1]):0;
        for(int i=1;i<Count;++i)
            if(depth<=Depths[i])
                return Profiles[season][i-1]+(Profiles[season][i]-Profiles[season][i-1])*
                    (depth-Depths[i-1])/(Depths[i]-Depths[i-1]);
        return Profiles[season][Count-1];
    }
    static double Temperature(CivilDate date,double depth)
    {
        const auto s=Blend(date);
        const double first=Profile(s.First,depth);
        return first+(Profile(s.Second,depth)-first)*s.Fraction;
    }
    static double Visibility(CivilDate date,int weather,double wind)
    {
        constexpr double seasonal[]={18,10,12,15},weatherFactor[]={1,.95,.6,.35};
        wind=std::isfinite(wind)?std::clamp(wind,0.,24.):3;
        return std::clamp(Seasonal(date,seasonal)*weatherFactor[std::clamp(weather,0,3)]/
            (1+.025*std::max(0.,wind-3)),2.,40.);
    }
    static double Current(CivilDate date,int weather,double wind)
    {
        constexpr double seasonal[]={.04,.09,.06,.05},rain[]={0,0,.02,.04};
        wind=std::isfinite(wind)?std::clamp(wind,0.,24.):3;
        return std::clamp(Seasonal(date,seasonal)+.004*wind+rain[std::clamp(weather,0,3)],0.,.5);
    }
};
}
