#pragma once
#include <algorithm>
#include <array>
#include <cmath>

// Independently implemented nitrogen-only game forecast. Fixed GF, selected
// gas throughout, no gas-sufficiency guarantee or repetitive-dive validation.
namespace Dive::Deco
{
inline constexpr std::array<double,16> HalfTimes={4,8,12.5,18.5,27,38.3,54.3,77,109,146,187,239,305,390,498,635};
inline constexpr std::array<double,16> A={1.2599,1,.8618,.7562,.62,.5043,.441,.4,.375,.35,.3295,.3065,.2835,.261,.248,.2327};
inline constexpr std::array<double,16> B={.505,.6514,.7222,.7825,.8126,.8434,.8693,.891,.9092,.9222,.9319,.9403,.9477,.9544,.9602,.9653};
inline constexpr double GF=.8, WaterVapourBar=.0627, AscentMetresPerSecond=.15;
using Tissues=std::array<double,16>;
struct Stop { double Depth=0, Seconds=0; };
struct Plan
{
    std::array<Stop,102> Stops{};
    int Count=0;
    double TotalSeconds=0;
    bool Valid=false;
};
inline double Limit(size_t i,double ambient)
{ return ambient*(1+GF*(1/B[i]-1))+GF*A[i]; }
inline double Ceiling(const Tissues& tissues,double surface,double barPerMetre)
{
    double required=surface;
    for(size_t i=0;i<16;++i)
        required=std::max(required,(tissues[i]-GF*A[i])/(1+GF*(1/B[i]-1)));
    return (required-surface)/barPerMetre;
}

inline Plan Forecast(Tissues tissues,double depth,double surface,double nitrogen,double barPerMetre)
{
    Plan plan;
    if(!std::isfinite(depth)||depth<0||depth>300||!std::isfinite(surface)||surface<.5||surface>1.5||
       !std::isfinite(nitrogen)||nitrogen<.5||nitrogen>1||!std::isfinite(barPerMetre)||barPerMetre<.09||barPerMetre>.11)
        return plan;
    for(double pressure:tissues) if(!std::isfinite(pressure)||pressure<0) return plan;
    // An existing ceiling violation has no valid ascent forecast from here.
    if(Ceiling(tissues,surface,barPerMetre)>depth+1e-7) return plan;
    while(depth>1e-8)
    {
        const double next=std::max(0.,(std::ceil(depth/3.-1e-9)-1)*3.);
        const double travel=(depth-next)/AscentMetresPerSecond;
        const double inspired=(surface+depth*barPerMetre-WaterVapourBar)*nitrogen;
        const double rate=-AscentMetresPerSecond*barPerMetre*nitrogen;
        std::array<double,16> k{},e{},base{};
        double wait=0;
        for(size_t i=0;i<16;++i)
        {
            k[i]=std::log(2.)/(HalfTimes[i]*60.);
            e[i]=std::exp(-k[i]*travel);
            // Exact integration for a linear inspired-pressure ramp. Solve the
            // preceding constant-depth wait analytically, independently per
            // compartment, then take the longest required wait.
            base[i]=inspired+rate*travel-rate/k[i]*(-std::expm1(-k[i]*travel));
            const double excess=(tissues[i]-inspired)*e[i];
            const double allowance=Limit(i,surface+next*barPerMetre)-base[i];
            if(excess>allowance)
            {
                if(excess<=0||allowance<=0) return Plan{};
                wait=std::max(wait,std::log(excess/allowance)/k[i]);
            }
        }
        wait=std::ceil(std::max(0.,wait)-1e-9);
        if(plan.TotalSeconds+wait+travel>21600) return Plan{};
        if(wait>0)
        {
            if(plan.Count>=static_cast<int>(plan.Stops.size())) return Plan{};
            plan.Stops[plan.Count++]={depth,wait};
        }
        for(size_t i=0;i<16;++i)
            tissues[i]=base[i]+(tissues[i]-inspired)*std::exp(-k[i]*(wait+travel));
        if(Ceiling(tissues,surface,barPerMetre)>next+1e-7) return Plan{};
        plan.TotalSeconds+=wait+travel;
        depth=next;
    }
    plan.Valid=true;
    return plan;
}
}
