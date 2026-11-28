#pragma once
#include <algorithm>
#include <cmath>

namespace Dive
{
// Authored equipment presets, not specifications of commercial products.
// Cone angles are half angles, as used by Unreal's spot light.
struct Torch
{
    const char* Id;
    const char* Name;
    double Lumens,InnerDegrees,OuterDegrees,RuntimeMinutes;
    double DryKg,DisplacedLitres;
    static constexpr int Count=3;
    static const Torch& At(int index)
    {
        static const Torch models[]={
            {"allround","Allround",1000,12,27,120,.65,.35},
            {"spot","Spot",700,5,8,180,.48,.27},
            {"wide","Wide",3000,45,55,55,1.05,.58}
        };
        return models[std::clamp(index,0,Count-1)];
    }
    double BatteryAfter(double percent,double seconds,bool on) const
    {
        if(!std::isfinite(percent)) percent=0;
        if(!std::isfinite(seconds)||seconds<0) seconds=0;
        return std::clamp(percent-(on?seconds*100/(RuntimeMinutes*60):0),0.,100.);
    }
    double Output(double percent) const
    {
        // Regulated output, followed by a visible low-battery tail.
        return std::isfinite(percent)?Lumens*std::clamp(percent/10.,0.,1.):0;
    }
};
}
