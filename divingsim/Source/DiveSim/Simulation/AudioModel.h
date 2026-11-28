#pragma once
#include "DiveModel.h"

namespace Dive
{
struct AudioLevels
{
    // Master, environment, equipment/breathing, heartbeat, warnings.
    std::array<double,5> Values={.8,.75,.85,.35,.8};
    void Sanitize()
    {
        const AudioLevels defaults;
        for(size_t i=0;i<Values.size();++i)
            Values[i]=std::isfinite(Values[i])?Clamp(Values[i],0,1):defaults.Values[i];
    }
    double Gain(size_t category) const { return Values[0]*Values[category]; }
};
struct AudioFrame
{
    bool Inhale=false,Exhale=false,Heartbeat=false,Warning=false;
    double Wet=0,Surface=0,Wind=0,Rain=0,CutoffHz=20000;
};
class AudioModel
{
    double PreviousPhase=0,HeartPhase=0,WarningCooldown=0,NoSupplyCooldown=0;
    uint32_t PreviousWarnings=0;
    bool First=true,HasBreath=false;
public:
    AudioFrame Step(double dt,const State& s,const Config& c,double eyeDepth)
    {
        AudioFrame a;
        if(dt<=0||!std::isfinite(dt)) return a;
        const bool gas=s.BreathSource==BreathingSource::Regulator&&(c.GameMode==Mode::Exploration||s.CylinderBar>0);
        if(!s.RegulatorInMouth) HasBreath=false;
        if(First||s.BreathPhase<PreviousPhase)
        { HasBreath=gas; a.Inhale=gas; First=false; }
        // The final lungful can still be exhaled after the cylinder runs empty.
        if(s.BreathPhase>=.52&&PreviousPhase<.52)
        { a.Exhale=HasBreath; HasBreath=false; }
        PreviousPhase=s.BreathPhase;
        HeartPhase+=dt*s.HeartBPM/60;
        if(HeartPhase>=1) { a.Heartbeat=true; HeartPhase=std::fmod(HeartPhase,1.); }
        WarningCooldown=std::max(0.,WarningCooldown-dt);
        NoSupplyCooldown=std::max(0.,NoSupplyCooldown-dt);
        const uint32_t newWarnings=s.Warnings&~PreviousWarnings;
        // First loss of supply is immediate. Repeated wave contact must not
        // retrigger an urgent beep on every surface crossing.
        const bool noSupplyAlert=(newWarnings&NoBreathingSupply)&&NoSupplyCooldown<=0;
        const uint32_t otherWarnings=newWarnings&~uint32_t(NoBreathingSupply);
        a.Warning=noSupplyAlert||(otherWarnings&&((otherWarnings&OutOfGas)||WarningCooldown<=0));
        if(noSupplyAlert) NoSupplyCooldown=8;
        if(a.Warning) WarningCooldown=2;
        PreviousWarnings=s.Warnings;
        a.Wet=Clamp(eyeDepth/.5,0,1);
        const double nearSurface=std::exp(-std::max(0.,eyeDepth)/3);
        a.Surface=(1-a.Wet)+.16*a.Wet*nearSurface;
        a.Wind=Clamp(c.WindMetresPerSecond/12,0,1)*((1-a.Wet)+.04*a.Wet*nearSurface);
        a.Rain=WeatherScenario::At(c.Weather).RainGain*((1-a.Wet)+.28*a.Wet*nearSurface);
        a.CutoffHz=20000+(650-20000)*a.Wet;
        return a;
    }
};
}
