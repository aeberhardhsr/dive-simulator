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
    double PreviousPhase=0,HeartPhase=0,WarningCooldown=0;
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
        const uint32_t newWarnings=s.Warnings&~PreviousWarnings;
        // A newly empty cylinder bypasses the chatter cooldown.
        a.Warning=newWarnings&&((newWarnings&(OutOfGas|NoBreathingSupply))||WarningCooldown<=0);
        if(a.Warning) WarningCooldown=2;
        PreviousWarnings=s.Warnings;
        a.Wet=Clamp(eyeDepth/.5,0,1);
        const double nearSurface=std::exp(-std::max(0.,eyeDepth)/3);
        a.Surface=(1-a.Wet)+.16*a.Wet*nearSurface;
        a.Wind=c.WindMetresPerSecond/12*((1-a.Wet)+.04*a.Wet*nearSurface);
        a.Rain=c.Weather==2?((1-a.Wet)+.28*a.Wet*nearSurface):0;
        a.CutoffHz=20000+(650-20000)*a.Wet;
        return a;
    }
};
}
