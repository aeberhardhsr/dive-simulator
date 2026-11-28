#pragma once
#include <algorithm>
#include <cmath>

namespace Dive::Sediment
{
// Authored silt response, not a measured Walensee sediment transport model.
// SI inputs. Only a moving submerged fin above a sediment surface can emit.
inline double StirRate(double clearance,double speed,bool submerged,bool sediment)
{
    if(!submerged||!sediment||!std::isfinite(clearance)||!std::isfinite(speed)||clearance<0||clearance>=.8) return 0;
    const double reach=1-clearance/.8;
    return 5*reach*reach*std::clamp((speed-.12)/.65,0.,1.);
}
struct Puff
{
    double Age=0,Strength=1;
    void Step(double dt) { if(std::isfinite(dt)&&dt>0) Age+=dt; }
    double Radius() const { return std::sqrt(.22*.22+.022*Age); }
    // Dilution conserves the integrated radial density before settling loss.
    // Unit-sphere optical depth scales with mass / radius^2.
    double OpticalDepth() const
    { return 8*Strength*std::exp(-Age/16)*std::pow(.22/Radius(),2)*(1-std::exp(-Age/.25))*std::clamp((45-Age)/8,0.,1.); }
    double Lift() const { return .12+.23*(1-std::exp(-Age/2))-.012*Age; }
    bool Alive() const { return Age<45; }
};
}
