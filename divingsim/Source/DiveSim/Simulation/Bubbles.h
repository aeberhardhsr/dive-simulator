#pragma once
#include <algorithm>
#include <cmath>

namespace Dive
{
struct Bubble
{
    // Isothermal ideal-gas expansion. Radius is in metres; pressure is absolute.
    static double RadiusAt(double initialRadius,double initialBar,double currentBar)
    {
        if(!std::isfinite(initialRadius)||!std::isfinite(initialBar)||!std::isfinite(currentBar)||
            initialRadius<=0||initialBar<=0||currentBar<=0) return 0;
        return initialRadius*std::cbrt(initialBar/currentBar);
    }
    // Authored terminal-rise approximation for millimetre/centimetre bubbles.
    // Not a resolved multiphase fluid solver.
    static double RiseSpeed(double radius)
    { return std::isfinite(radius)?std::clamp(.18+.1*std::sqrt(std::max(0.,radius)/.008),.18,.34):.18; }
};
}
