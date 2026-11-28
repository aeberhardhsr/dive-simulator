#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace Dive
{
// Game habitat envelopes, not measured species distributions or population estimates.
// Length/depth/speed are SI. Model mesh length already matches LengthMetres.
struct FishProfile
{
    const char* Mesh;
    double LengthMetres, MinDepth, MaxDepth, BottomClearance;
    double CruiseSpeed, EscapeSpeed, FearRadius, TailHz;
    int Count;
    bool Schooling, BottomDweller;
};
inline constexpr std::array<FishProfile,5> FishProfiles{{
    {"SM_Perch",    .30, 1.2, 24, .45, .32, 1.7, 4.0, 2.0, 24, true,  false},
    {"SM_Roach",    .24, 1.0, 14, .40, .40, 1.6, 4.5, 2.5, 24, true,  false},
    {"SM_Pike",     .75, 1.5, 16, .55, .12, 2.4, 5.5,  .8,  4, false, false},
    {"SM_Whitefish",.38, 5.0, 55, 2.0, .60, 2.1, 5.0, 2.2, 18, true,  false},
    {"SM_Burbot",   .55, 8.0, 80, .18, .14,  .9, 2.8, 1.1,  7, false, true}
}};
struct DepthBand { double Shallow=0, Deep=0; bool Valid=false; };
inline DepthBand FishDepthBand(const FishProfile& Fish,double BottomDepth,double Scale=1)
{
    if(!std::isfinite(BottomDepth)||!std::isfinite(Scale)||Scale<=0) return {};
    const double clearance=Fish.BottomClearance*Scale;
    const double deep=std::min(Fish.MaxDepth,BottomDepth-clearance);
    const double shallow=Fish.BottomDweller?std::max(Fish.MinDepth,BottomDepth-clearance-.65):Fish.MinDepth;
    return {shallow,deep,deep>=shallow};
}
struct PlantProfile { const char* Mesh; double MaxDepth, MaxSlope, MaxHeight; };
inline constexpr std::array<PlantProfile,3> PlantProfiles{{
    {"SM_Pondweed", 10, .55, 1.15},
    {"SM_Stonewort",18, .45,  .45},
    {"SM_Milfoil",   7, .50,  .90}
}};
inline bool PlantHabitat(const PlantProfile& Plant,double BottomDepth,double Slope,double Visibility)
{
    return std::isfinite(BottomDepth)&&std::isfinite(Slope)&&std::isfinite(Visibility)&&
        BottomDepth>Plant.MaxHeight+.3&&BottomDepth<=std::min(Plant.MaxDepth,Visibility*1.3)&&
        Slope>=0&&Slope<Plant.MaxSlope;
}
}
