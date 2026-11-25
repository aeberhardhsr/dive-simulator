#pragma once
#include "DiveModel.h"

namespace Dive
{
enum class Lesson { Look,Swim,Buoyancy,Computer,Lamp,Map,Return,Exit,Done };
struct Tutorial
{
    Lesson Stage=Lesson::Look;
    double LookDegrees=0,SwimMetres=0,InflatedLitres=0,VentedLitres=0;
    bool SawVitals=false,SawLampOff=false,SawMap=false;
    bool HasView=false;
    double LastYaw=0,LastPitch=0;
    void Look(double Degrees) { if(Stage==Lesson::Look) LookDegrees+=std::abs(Degrees); }
    void ObserveView(double Yaw,double Pitch)
    {
        if(!std::isfinite(Yaw)||!std::isfinite(Pitch)) return;
        if(HasView) Look(std::abs(std::remainder(Yaw-LastYaw,360.))+std::abs(std::remainder(Pitch-LastPitch,360.)));
        LastYaw=Yaw; LastPitch=Pitch; HasView=true;
    }
    void Step(double dt,const State& s,double SwimDistance,bool Swimming,bool Computer,int Page,bool Lamp,bool Map,double DistanceToEntry)
    {
        switch(Stage)
        {
        case Lesson::Look: if(LookDegrees>=25) Stage=Lesson::Swim; break;
        case Lesson::Swim:
            if(Swimming) SwimMetres+=std::max(0.,SwimDistance);
            if(SwimMetres>=3) Stage=Lesson::Buoyancy;
            break;
        case Lesson::Buoyancy:
            InflatedLitres+=s.BCDInflowLPS*dt;
            if(InflatedLitres>=1.5) VentedLitres+=s.BCDOutflowLPS*dt;
            if(VentedLitres>=1.5) Stage=Lesson::Computer;
            break;
        case Lesson::Computer:
            SawVitals|=Computer&&Page==1;
            if(SawVitals&&!Computer) Stage=Lesson::Lamp;
            break;
        case Lesson::Lamp:
            SawLampOff|=!Lamp;
            if(SawLampOff&&Lamp) Stage=Lesson::Map;
            break;
        case Lesson::Map:
            SawMap|=Map;
            if(SawMap&&!Map) Stage=Lesson::Return;
            break;
        case Lesson::Return: if(s.DepthMetres<=.8&&DistanceToEntry<12) Stage=Lesson::Exit; break;
        default: break;
        }
    }
    bool Finish(bool Returned) { if(Stage==Lesson::Exit&&Returned) { Stage=Lesson::Done; return true; } return false; }
};
}
