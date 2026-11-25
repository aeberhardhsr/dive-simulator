#pragma once

// Engine-independent gameplay model. SI units except explicitly named litres/bar.
// This is not validated diving, physiological or decompression software.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace Dive
{
constexpr double Gravity = 9.80665;
constexpr double Density = 1000.0;
constexpr double BarPerMetre = Density * Gravity / 100000.0;
constexpr double Pi = 3.14159265358979323846;
inline double Clamp(double v, double a, double b) { return std::clamp(v, a, b); }
inline double Approach(double v, double target, double rate, double dt)
{ return target + (v - target) * std::exp(-rate * dt); }

enum class Mode { Exploration, Simulation, Hardcore };
enum Warning : uint32_t
{
    None=0, LowGas=1, FastAscent=2, HighPO2=4, Cold=8,
    CeilingViolation=16, OutOfGas=32, DepthPlan=64, TimePlan=128
};

struct Config
{
    Mode GameMode = Mode::Simulation;
    double CylinderLitres = 12.0;
    double StartPressureBar = 200.0;
    double OxygenFraction = 0.21;
    double WeightKg = 6.0;
    double SurfacePressureBar = 0.965; // Scenario atmosphere, configurable.
    double SurfaceTemperatureC = 18.0;
    double BottomTemperatureC = 6.0;
    double VisibilityMetres = 14.0;
    double CurrentMetresPerSecond = 0.08;
    double PlannedDepthMetres = 20.0;
    double PlannedMinutes = 30.0;
    double Hour = 13.0;
    double WindMetresPerSecond = 3.0;
    double WindDirectionDegrees = 90.0;
    double ThermoclineMetres = 10.0;
    double ThermoclineWidthMetres = 2.0;
    int Weather = 0; // 0 clear, 1 overcast, 2 rain; scenario presets, not live weather.
    bool DrySuit = true;
    bool AutoBuoyancy = false;
    void Sanitize()
    {
        auto clean=[](double& v,double fallback,double lo,double hi)
        { v=std::isfinite(v) ? Clamp(v,lo,hi) : fallback; };
        clean(CylinderLitres,12,1,30); clean(StartPressureBar,200,1,300);
        clean(OxygenFraction,.21,.21,.40); clean(WeightKg,6,0,20);
        clean(SurfacePressureBar,.965,.6,1.1);
        clean(SurfaceTemperatureC,18,1,30); clean(BottomTemperatureC,6,1,25);
        clean(VisibilityMetres,14,2,40); clean(CurrentMetresPerSecond,.08,0,.5);
        clean(PlannedDepthMetres,20,3,60); clean(PlannedMinutes,30,1,120);
        clean(Hour,13,0,23.99);
        clean(WindMetresPerSecond,3,0,12); clean(WindDirectionDegrees,90,0,359.99);
        clean(ThermoclineMetres,10,3,25); clean(ThermoclineWidthMetres,2,.5,5);
        Weather=std::clamp(Weather,0,2);
    }
};

struct State
{
    double ElapsedSeconds=0, DepthMetres=0, MaxDepthMetres=0;
    double CylinderBar=200, AmbientBar=.965, PPO2=.203;
    double BCDSurfaceLitres=4, BCDLitres=4;
    double BCDInflowLPS=0, BCDOutflowLPS=0; // Actual gas flow, local litres/second.
    double HeartBPM=70, RespiratoryBPM=12, SpO2=98;
    double CoreTemperatureC=37, WaterTemperatureC=18;
    double Stress=0, Fatigue=0, Exertion=0;
    double SurfaceRMV=16, RemainingGasMinutes=0;
    double AscentMetresPerMinute=0, BreathPhase=0, BuoyancyNewtons=0;
    double CeilingMetres=0, NDLMinutes=99, StopRemainingSeconds=180;
    double DistanceMetres=0, LampBattery=100;
    std::array<double,16> NitrogenBar{};
    uint32_t Warnings=0;
};

// Published ZH-L16C nitrogen coefficients; independent exponential integration.
// A single fixed GF=0.80 is a gameplay approximation, not a decompression planner.
inline constexpr std::array<double,16> HalfTimes={4,8,12.5,18.5,27,38.3,54.3,77,109,146,187,239,305,390,498,635};
inline constexpr std::array<double,16> A={1.2599,1,.8618,.7562,.62,.5043,.441,.4,.375,.35,.3295,.3065,.2835,.261,.248,.2327};
inline constexpr std::array<double,16> B={.505,.6514,.7222,.7825,.8126,.8434,.8693,.891,.9092,.9222,.9319,.9403,.9477,.9544,.9602,.9653};

class Model
{
public:
    Config Settings;
    State Data;
    explicit Model(Config cfg={}) { Reset(cfg); }
    void Reset(Config cfg)
    {
        cfg.Sanitize(); Settings=cfg; Data=State{};
        Data.CylinderBar=cfg.StartPressureBar;
        Data.AmbientBar=cfg.SurfacePressureBar;
        Data.PPO2=Data.AmbientBar*cfg.OxygenFraction;
        Data.WaterTemperatureC=cfg.SurfaceTemperatureC;
        // Surface tissues equilibrate with air before switching the selected tank mix.
        Data.NitrogenBar.fill((cfg.SurfacePressureBar-.0627)*.79);
    }
    double Pressure(double depth) const
    { return Settings.SurfacePressureBar+std::max(0.0,depth)*BarPerMetre; }
    double Temperature(double depth) const
    {
        const double thermocline=1/(1+std::exp(-(depth-Settings.ThermoclineMetres)/Settings.ThermoclineWidthMetres));
        return Settings.SurfaceTemperatureC+(Settings.BottomTemperatureC-Settings.SurfaceTemperatureC)*thermocline;
    }
    void Step(double dt,double depth,double effort,double inflate,double verticalVelocity,
              double distance,bool lamp)
    {
        if (!std::isfinite(dt) || dt<=0 || !std::isfinite(depth) || !std::isfinite(effort)
            || !std::isfinite(inflate) || !std::isfinite(verticalVelocity) || !std::isfinite(distance)) return;
        // Split long calls so behaviour remains stable at different render rates.
        while(dt>1e-9)
        {
            const double step=std::min(dt,.02);
            Integrate(step,depth,effort,inflate,verticalVelocity,distance*step/dt,lamp);
            distance-=distance*step/dt; dt-=step;
        }
    }
private:
    void Integrate(double dt,double depth,double effort,double inflate,double vz,double distance,bool lamp)
    {
        auto& s=Data; const auto& c=Settings;
        const bool unlimited=c.GameMode==Mode::Exploration;
        s.ElapsedSeconds+=dt; s.DepthMetres=std::max(0.0,depth);
        s.MaxDepthMetres=std::max(s.MaxDepthMetres,s.DepthMetres);
        s.AmbientBar=Pressure(depth); s.PPO2=s.AmbientBar*c.OxygenFraction;
        s.WaterTemperatureC=Temperature(depth); s.Exertion=Clamp(effort,0,1);
        s.AscentMetresPerMinute=Approach(s.AscentMetresPerMinute,vz*60,3,dt);
        const double stressTarget=Clamp((s.CylinderBar<50?.35:0)+(depth>c.PlannedDepthMetres?.2:0)
            +(s.AscentMetresPerMinute>9?.35:0)+(s.CylinderBar<1?.7:0),0,1);
        s.Stress=Approach(s.Stress,unlimited?stressTarget*.25:stressTarget,.12,dt);
        s.Fatigue=Clamp(s.Fatigue+dt*(s.Exertion*.00035-(1-s.Exertion)*.00012),0,1);
        s.HeartBPM=Approach(s.HeartBPM,68+62*s.Exertion+36*s.Stress+12*s.Fatigue,.3,dt);
        s.RespiratoryBPM=Approach(s.RespiratoryBPM,12+15*s.Exertion+13*s.Stress,.4,dt);
        s.SurfaceRMV=16+24*s.Exertion+18*s.Stress+5*s.Fatigue;
        const double breathingLitres=s.SurfaceRMV*s.AmbientBar/60*dt;
        double injected=std::max(0.0,Clamp(inflate,-1,1))*2.0*s.AmbientBar*dt;
        if(!unlimited) injected=std::min(injected,std::max(0.0,s.CylinderBar*c.CylinderLitres-breathingLitres));
        s.BCDSurfaceLitres+=injected;
        const double vented=std::min(s.BCDSurfaceLitres,std::max(0.0,-inflate)*3*s.AmbientBar*dt);
        s.BCDSurfaceLitres-=vented;
        const double relief=std::max(0.,s.BCDSurfaceLitres-18*s.AmbientBar);
        s.BCDInflowLPS=injected/(s.AmbientBar*dt);
        s.BCDOutflowLPS=(vented+relief)/(s.AmbientBar*dt);
        s.BCDSurfaceLitres=Clamp(s.BCDSurfaceLitres,0,18*s.AmbientBar); // overpressure relief
        s.BCDLitres=s.BCDSurfaceLitres/s.AmbientBar;
        if(!unlimited) s.CylinderBar=std::max(0.0,s.CylinderBar-(breathingLitres+injected)/c.CylinderLitres);
        s.RemainingGasMinutes=std::max(0.0,(s.CylinderBar-50)*c.CylinderLitres/(s.SurfaceRMV*s.AmbientBar));
        s.BreathPhase=std::fmod(s.BreathPhase+dt*s.RespiratoryBPM/60,1.0);
        const double lungDelta=.65*std::sin(s.BreathPhase*2*Pi);
        const double suitLift=(c.DrySuit?3:5)/std::pow(s.AmbientBar/c.SurfacePressureBar,.7);
        const double gasMass=s.CylinderBar*c.CylinderLitres*.0012;
        const double netKg=2+suitLift+s.BCDLitres+lungDelta-c.WeightKg-gasMass;
        s.BuoyancyNewtons=(c.AutoBuoyancy || unlimited)?lungDelta*Gravity:netKg*Gravity;
        const double coldLoad=std::max(0.0,22-s.WaterTemperatureC)*(c.DrySuit?.0000015:.000007);
        s.CoreTemperatureC=Clamp(s.CoreTemperatureC+dt*((37-s.CoreTemperatureC)*.0001-coldLoad),32,37.5);
        s.SpO2=Approach(s.SpO2,(!unlimited && s.CylinderBar<=.1 && depth>.2)?45:98,.035,dt);
        s.DistanceMetres+=std::max(0.0,distance);
        if(lamp) s.LampBattery=std::max(0.0,s.LampBattery-dt/72);
        if(s.MaxDepthMetres>10 && depth>=3 && depth<=6 && std::abs(vz)<.2)
            s.StopRemainingSeconds=std::max(0.0,s.StopRemainingSeconds-dt);
        if(depth>10) s.StopRemainingSeconds=180;
        double ceilingBar=c.SurfacePressureBar, ndl=99;
        const double inspired=(s.AmbientBar-.0627)*(1-c.OxygenFraction);
        for(size_t i=0;i<16;++i)
        {
            const double k=std::log(2.0)/(HalfTimes[i]*60);
            s.NitrogenBar[i]=inspired+(s.NitrogenBar[i]-inspired)*std::exp(-k*dt);
            constexpr double gf=.8;
            const double factor=1+gf*(1/B[i]-1);
            ceilingBar=std::max(ceilingBar,(s.NitrogenBar[i]-gf*A[i])/factor);
            const double surfaceLimit=c.SurfacePressureBar*factor+gf*A[i];
            if(s.NitrogenBar[i]>=surfaceLimit) ndl=0;
            else if(inspired>surfaceLimit)
                ndl=std::min(ndl,-std::log((inspired-surfaceLimit)/(inspired-s.NitrogenBar[i]))/(k*60));
        }
        s.CeilingMetres=unlimited?0:std::max(0.0,(ceilingBar-c.SurfacePressureBar)/BarPerMetre);
        s.NDLMinutes=unlimited?99:Clamp(ndl,0,99);
        s.Warnings=None;
        if(!unlimited && s.CylinderBar<50) s.Warnings|=LowGas;
        if(!unlimited && s.CylinderBar<=.1) s.Warnings|=OutOfGas;
        if(s.AscentMetresPerMinute>9) s.Warnings|=FastAscent;
        if(s.PPO2>1.4) s.Warnings|=HighPO2;
        if(s.CoreTemperatureC<35.5) s.Warnings|=Cold;
        if(s.CeilingMetres>depth+.5) s.Warnings|=CeilingViolation;
        if(depth>c.PlannedDepthMetres+1) s.Warnings|=DepthPlan;
        if(s.ElapsedSeconds>c.PlannedMinutes*60) s.Warnings|=TimePlan;
    }
};
}
