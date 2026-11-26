#pragma once
#include <algorithm>
#include <cmath>

namespace Dive
{
// Authored animation response, not a measured locomotion/propulsion model.
struct FishMotion
{
    double Tail=0, Fin=0, Effort=1, Turn=0;
    void Step(double dt,double speed,double cruise,double tailHz,double yawRate)
    {
        if(!std::isfinite(dt)||dt<=0||!std::isfinite(speed)||!std::isfinite(cruise)||
           cruise<=0||!std::isfinite(tailHz)||tailHz<=0||!std::isfinite(yawRate)) return;
        const double response=1-std::exp(-4*dt);
        const double old=Effort;
        Effort+=(std::clamp(speed/cruise,0.,2.5)-Effort)*response;
        Turn+=(std::clamp(yawRate/1.5,-1.,1.)-Turn)*response;
        const double average=(old+Effort)*.5;
        constexpr double tau=6.283185307179586;
        Tail=std::fmod(Tail+dt*tailHz*(.25+.75*average)*tau,tau);
        Fin=std::fmod(Fin+dt*tailHz*(.45+.25*average)*tau,tau);
    }
    double TailAmplitude() const { return .5+.5*std::clamp(Effort,0.,2.); }
};
}
