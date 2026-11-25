#pragma once
#include <cmath>

namespace Dive::Navigation
{
// World X is survey north, world Y survey east. Bearings increase clockwise.
inline double Heading(double degrees)
{
    if(!std::isfinite(degrees)) return 0;
    double value=std::fmod(degrees,360.);return value<0?value+360:value;
}
inline int DisplayHeading(double degrees) { return static_cast<int>(std::round(Heading(degrees)))%360; }
inline double Bearing(double north,double east) { return Heading(std::atan2(east,north)*180/3.14159265358979323846); }
struct Point { double X=0,Y=0; bool Inside() const { return X>=0&&X<=1&&Y>=0&&Y<=1; } };
inline Point Project(double east,double north,double west,double south,double width,double height)
{ return {(east-west)/width,1-(north-south)/height}; }
}
