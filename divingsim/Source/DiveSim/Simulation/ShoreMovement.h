#pragma once
#include "DiveModel.h"

namespace Dive
{
namespace ShoreMovement
{
// Upright collision proxy, metres. Six centimetres keep the capsule clear
// of the stair ramp; the camera remains 1.76 m above its support surface.
constexpr double HalfHeight=.90, EyeOffset=.80, FloorClearance=.96, MassKg=100;
inline double Immersion(double CentreHeight,double SurfaceHeight)
{ return Clamp((SurfaceHeight-CentreHeight+HalfHeight)/(2*HalfHeight),0,1); }
inline double VerticalForce(double FullBuoyancy,double Immersed)
// FullBuoyancy is already the NET fully submerged force from the dive model.
{ return FullBuoyancy*Immersed-MassKg*Gravity*(1-Immersed); }
inline bool CanStand(double CentreHeight,double GroundHeight,double SurfaceHeight,bool WasStanding)
{
    const double waterDepth=SurfaceHeight-GroundHeight;
    return CentreHeight-GroundHeight<=FloorClearance+.15 &&
        waterDepth<(WasStanding?1.55:1.45);
}
inline double WalkSpeed(bool Sprint,double Fatigue)
{ return (Sprint?1.5:1.1)*(1-Clamp(Fatigue,0,1)*.3); }
}
}
