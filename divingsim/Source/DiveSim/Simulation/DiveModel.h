#pragma once

// Engine-independent gameplay model. SI units except explicitly named litres/bar.
// This is not validated diving, physiological or decompression software.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include "Weather.h"
#include "ScenarioClock.h"
#include "Torch.h"
#include "EnvironmentScenario.h"
#include "ModeRules.h"
#include "Loadout.h"

namespace Dive
{
constexpr double Gravity = 9.80665;
constexpr double Density = 1000.0;
constexpr double BarPerMetre = Density * Gravity / 100000.0;
constexpr double Pi = 3.14159265358979323846;
inline double Clamp(double v, double a, double b) { return std::clamp(v, a, b); }
inline double Approach(double v, double target, double rate, double dt)
{ return target + (v - target) * std::exp(-rate * dt); }

enum Warning : uint32_t
{
    None=0, LowGas=1, FastAscent=2, HighPO2=4, Cold=8,
    CeilingViolation=16, OutOfGas=32, DepthPlan=64, TimePlan=128, NoBreathingSupply=256,
    EquipmentFailure=512, NarcosisWarning=1024, PanicWarning=2048, DecompressionSymptoms=4096
};
enum class BreathingSource { Regulator=0, Air=1, None=2 };
struct Exposure
{
    double BodyImmersion=1;
    bool MouthSubmerged=true,RegulatorInMouth=true;
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
    double AirTemperatureC = 20.0;
    double BottomTemperatureC = 6.0;
    double VisibilityMetres = 14.0;
    double CurrentMetresPerSecond = 0.08;
    double PlannedDepthMetres = 20.0;
    double PlannedMinutes = 30.0;
    double Hour = 13.0;
    CivilDate Date;
    int TorchModel=0;
    int CylinderMaterial=0,BCDModel=0;
    double WindMetresPerSecond = 3.0;
    double WindDirectionDegrees = 90.0;
    double ThermoclineMetres = 10.0;
    double ThermoclineWidthMetres = 2.0;
    int Weather = 0; // 0 clear, 1 overcast, 2 rain, 3 storm; not live weather.
    bool DrySuit = true;
    bool AutoBuoyancy = false;
    bool ManualExplorationBuoyancy = false; // Old exploration profiles retain assisted trim.
    bool StartOnShore = false; // Legacy/instant start remains compatible.
    bool AutoVisibility = false,AutoTemperature = false;
    CurrentSetting CurrentSource = CurrentSetting::Custom;
    ModeRules Rules() const { return ModeRules::For(GameMode); }
    bool AssistedBuoyancy() const { return GameMode==Mode::Exploration&&!ManualExplorationBuoyancy; }
    void SelectWeather(int Value)
    {
        Weather=std::clamp(Value,0,WeatherScenario::Count-1);
        WindMetresPerSecond=WeatherScenario::At(Weather).Wind;
    }
    void Sanitize()
    {
        auto clean=[](double& v,double fallback,double lo,double hi)
        { v=std::isfinite(v) ? Clamp(v,lo,hi) : fallback; };
        clean(CylinderLitres,12,1,30); clean(StartPressureBar,200,1,300);
        clean(OxygenFraction,.21,.21,.40); clean(WeightKg,6,0,20);
        clean(SurfacePressureBar,.965,.6,1.1);
        clean(SurfaceTemperatureC,18,1,30); clean(BottomTemperatureC,6,1,25);
        clean(AirTemperatureC,20,-10,40);
        clean(VisibilityMetres,14,2,40); clean(CurrentMetresPerSecond,.08,0,.5);
        clean(PlannedDepthMetres,20,3,60); clean(PlannedMinutes,30,1,120);
        clean(Hour,13,0,23.99);
        if(!Date.ScenarioRange()) Date=CivilDate{};
        double utc=0;
        if(!ScenarioClock::StartUTC(Date,Hour,utc)) Hour+=1; // Corrupt/legacy spring-gap input; UI rejects it.
        clean(WindMetresPerSecond,3,0,WeatherScenario::MaximumWind); clean(WindDirectionDegrees,90,0,359.99);
        clean(ThermoclineMetres,10,3,25); clean(ThermoclineWidthMetres,2,.5,5);
        Weather=std::clamp(Weather,0,WeatherScenario::Count-1);
        TorchModel=std::clamp(TorchModel,0,Torch::Count-1);
        CylinderMaterial=std::clamp(CylinderMaterial,0,1);
        BCDModel=std::clamp(BCDModel,0,BCDEquipment::Count-1);
        if(static_cast<int>(GameMode)<0||static_cast<int>(GameMode)>2) GameMode=Mode::Simulation;
        CurrentSource=static_cast<CurrentSetting>(std::clamp(static_cast<int>(CurrentSource),0,4));
    }
    // Keep manual values in the plan; resolve an independent frozen dive config.
    Config Resolved() const
    {
        Config result=*this; result.Sanitize();
        if(result.AutoTemperature)
        {
            result.SurfaceTemperatureC=EnvironmentScenario::Temperature(result.Date,0);
            result.BottomTemperatureC=EnvironmentScenario::Temperature(result.Date,144);
        }
        if(result.AutoVisibility) result.VisibilityMetres=EnvironmentScenario::Visibility(result.Date,result.Weather,result.WindMetresPerSecond);
        switch(result.CurrentSource)
        {
        case CurrentSetting::Automatic: result.CurrentMetresPerSecond=EnvironmentScenario::Current(result.Date,result.Weather,result.WindMetresPerSecond); break;
        case CurrentSetting::None: result.CurrentMetresPerSecond=0; break;
        case CurrentSetting::Light: result.CurrentMetresPerSecond=.08; break;
        case CurrentSetting::Strong: result.CurrentMetresPerSecond=.3; break;
        default: break;
        }
        return result;
    }
};

struct State
{
    double ElapsedSeconds=0, DepthMetres=0, MaxDepthMetres=0;
    double CylinderBar=200, AmbientBar=.965, PPO2=.203;
    double BCDSurfaceLitres=4, BCDLitres=4;
    double BCDInflowLPS=0, BCDOutflowLPS=0; // Actual gas flow, local litres/second.
    double EquipmentDryKg=0,CylinderGasKg=0,BCDGasKg=0,TotalMassKg=0,DisplacedLitres=0,SuitLitres=0;
    double HeartBPM=70, RespiratoryBPM=12, SpO2=98;
    double BreathReserve=1; // Normalized gameplay reserve, not measured oxygen litres.
    double CoreTemperatureC=37, WaterTemperatureC=18;
    double EnvironmentTemperatureC=18,BodyImmersion=1;
    bool RegulatorInMouth=true,MouthSubmerged=true;
    BreathingSource BreathSource=BreathingSource::Regulator;
    double Stress=0, Fatigue=0, Exertion=0;
    // Normalized authored gameplay effects, not diagnostic probabilities.
    double Narcosis=0, Panic=0, ColdStrain=0, DecompressionBurden=0;
    double MotorCapacity=1, VisualImpairment=0;
    double RegulatorTemperatureC=18, FreezeExposure=0;
    bool RegulatorFreeflow=false;
    double FreeflowLocalLPS=0, FreeflowLostLitres=0;
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
        cfg=cfg.Resolved(); Settings=cfg; Data=State{};
        Data.CylinderBar=cfg.StartPressureBar;
        Data.AmbientBar=cfg.SurfacePressureBar;
        Data.PPO2=Data.AmbientBar*cfg.OxygenFraction;
        Data.WaterTemperatureC=cfg.SurfaceTemperatureC;
        Data.RegulatorTemperatureC=cfg.SurfaceTemperatureC;
        // Surface tissues equilibrate with air before switching the selected tank mix.
        Data.NitrogenBar.fill((cfg.SurfacePressureBar-.0627)*.79);
        // Surface preparation uses a fixed, visible 8 L initial fill, not a
        // hidden neutral trim that cancels every ballast/loadout choice.
        double fill=Loadout::InitialBCDLitres*cfg.SurfacePressureBar;
        if(cfg.Rules().LimitedGas)
        {
            fill=std::min(fill,Data.CylinderBar*cfg.CylinderLitres);
            Data.CylinderBar=std::max(0.0,Data.CylinderBar-fill/cfg.CylinderLitres);
        }
        Data.BCDSurfaceLitres=fill;Data.BCDLitres=fill/cfg.SurfacePressureBar;
        UpdateBalance(0);
    }
    double Pressure(double depth) const
    { return Settings.SurfacePressureBar+std::max(0.0,depth)*BarPerMetre; }
    double Temperature(double depth) const
    {
        if(Settings.AutoTemperature) return EnvironmentScenario::Temperature(Settings.Date,depth);
        const double thermocline=1/(1+std::exp(-(depth-Settings.ThermoclineMetres)/Settings.ThermoclineWidthMetres));
        return Settings.SurfaceTemperatureC+(Settings.BottomTemperatureC-Settings.SurfaceTemperatureC)*thermocline;
    }
    void Step(double dt,double depth,double effort,double inflate,double verticalVelocity,
              double distance,bool lamp,Exposure exposure={})
    {
        if (!std::isfinite(dt) || dt<=0 || !std::isfinite(depth) || !std::isfinite(effort)
            || !std::isfinite(inflate) || !std::isfinite(verticalVelocity) || !std::isfinite(distance)
            || !std::isfinite(exposure.BodyImmersion)) return;
        // Split long calls so behaviour remains stable at different render rates.
        while(dt>1e-9)
        {
            const double step=std::min(dt,.02);
            Integrate(step,depth,effort,inflate,verticalVelocity,distance*step/dt,lamp,exposure);
            distance-=distance*step/dt; dt-=step;
        }
    }
private:
    void UpdateBalance(double lungDelta)
    {
        const auto& c=Settings;auto& s=Data;
        const auto balance=Loadout::Evaluate(c.CylinderLitres,c.CylinderMaterial,c.BCDModel,c.DrySuit,c.TorchModel,
            c.WeightKg,s.CylinderBar,s.BCDSurfaceLitres,s.AmbientBar,c.SurfacePressureBar,lungDelta);
        s.EquipmentDryKg=balance.EquipmentDryKg;s.CylinderGasKg=balance.CylinderGasKg;s.BCDGasKg=balance.BCDGasKg;
        s.TotalMassKg=balance.TotalMassKg;s.DisplacedLitres=balance.DisplacedLitres;s.SuitLitres=balance.SuitLitres;
        s.BuoyancyNewtons=(c.AssistedBuoyancy()?lungDelta:balance.NetKg)*Gravity;
    }
    void Integrate(double dt,double depth,double effort,double inflate,double vz,double distance,bool lamp,const Exposure& exposure)
    {
        auto& s=Data; const auto& c=Settings;
        const auto rules=c.Rules();
        const bool unlimited=!rules.LimitedGas;
        s.ElapsedSeconds+=dt; s.DepthMetres=std::max(0.0,depth);
        s.MaxDepthMetres=std::max(s.MaxDepthMetres,s.DepthMetres);
        s.BodyImmersion=Clamp(exposure.BodyImmersion,0,1);
        s.RegulatorInMouth=exposure.RegulatorInMouth; s.MouthSubmerged=exposure.MouthSubmerged;
        const auto previousSource=s.BreathSource;
        s.BreathSource=s.RegulatorInMouth?
            ((unlimited||s.CylinderBar>0)?BreathingSource::Regulator:BreathingSource::None):
            (s.MouthSubmerged?BreathingSource::None:BreathingSource::Air);
        const bool hasBreathingSupply=s.BreathSource!=BreathingSource::None;
        s.AmbientBar=Pressure(depth);
        s.PPO2=s.BreathSource==BreathingSource::Regulator?s.AmbientBar*c.OxygenFraction:
            s.BreathSource==BreathingSource::Air?c.SurfacePressureBar*.21:0;
        s.WaterTemperatureC=Temperature(depth); s.Exertion=Clamp(effort,0,1);
        s.EnvironmentTemperatureC=s.BodyImmersion*s.WaterTemperatureC+(1-s.BodyImmersion)*c.AirTemperatureC;
        s.AscentMetresPerMinute=Approach(s.AscentMetresPerMinute,vz*60,3,dt);
        const double stressTarget=Clamp((s.CylinderBar<50?.35:0)+(depth>c.PlannedDepthMetres?.2:0)
            +(s.AscentMetresPerMinute>9?.35:0)+(!hasBreathingSupply?.7:0),0,1);
        s.Stress=Approach(s.Stress,unlimited?stressTarget*.25:stressTarget,.12,dt);
        s.Fatigue=rules.Physiology?Clamp(s.Fatigue+dt*(s.Exertion*.00035-(1-s.Exertion)*.00012),0,1):0;
        const double panicTarget=Clamp((s.Stress-.45)*1.8+(1-s.BreathReserve)*.5+s.Exertion*s.Stress*.2,0,1);
        s.Panic=rules.Physiology?Approach(s.Panic,panicTarget,.08,dt):0;
        s.HeartBPM=Approach(s.HeartBPM,68+62*s.Exertion+36*s.Stress+12*s.Fatigue+18*s.Panic,.3,dt);
        s.RespiratoryBPM=Approach(s.RespiratoryBPM,12+15*s.Exertion+13*s.Stress+8*s.Panic,.4,dt);
        s.SurfaceRMV=16+24*s.Exertion+18*s.Stress+5*s.Fatigue+12*s.Panic;
        const double breathingLitres=s.BreathSource==BreathingSource::Regulator?s.SurfaceRMV*s.AmbientBar/60*dt:0;
        double injected=std::max(0.0,Clamp(inflate,-1,1))*2.0*s.AmbientBar*dt;
        if(!unlimited) injected=std::min(injected,std::max(0.0,s.CylinderBar*c.CylinderLitres-breathingLitres));
        s.BCDSurfaceLitres+=injected;
        const double vented=std::min(s.BCDSurfaceLitres,std::max(0.0,-inflate)*3*s.AmbientBar*dt);
        s.BCDSurfaceLitres-=vented;
        const double capacity=BCDEquipment::At(c.BCDModel).CapacityLitres;
        const double relief=std::max(0.,s.BCDSurfaceLitres-capacity*s.AmbientBar);
        s.BCDInflowLPS=injected/(s.AmbientBar*dt);
        s.BCDOutflowLPS=(vented+relief)/(s.AmbientBar*dt);
        s.BCDSurfaceLitres=Clamp(s.BCDSurfaceLitres,0,capacity*s.AmbientBar); // overpressure relief
        s.BCDLitres=s.BCDSurfaceLitres/s.AmbientBar;
        if(!unlimited) s.CylinderBar=std::max(0.0,s.CylinderBar-(breathingLitres+injected)/c.CylinderLitres);
        // Expansion cooling / icing is qualitative. The regulator and thresholds
        // are fictional equipment parameters, not a real product failure forecast.
        s.FreeflowLocalLPS=0;
        if(rules.EquipmentFailures)
        {
            const double flow=(breathingLitres+injected)/dt;
            const double target=s.BodyImmersion*s.WaterTemperatureC+(1-s.BodyImmersion)*c.AirTemperatureC
                -3.5*flow;
            s.RegulatorTemperatureC=Approach(s.RegulatorTemperatureC,target,.05,dt);
            s.FreezeExposure=Clamp(s.FreezeExposure+dt*(s.BodyImmersion>.5&&flow>0?
                std::max(0.0,-s.RegulatorTemperatureC)-std::max(0.0,s.RegulatorTemperatureC)*.5:-2),0,120);
            if(s.FreezeExposure>=60) s.RegulatorFreeflow=true;
            if(s.RegulatorFreeflow)
            {
                const double lost=std::min(s.CylinderBar*c.CylinderLitres,6*s.AmbientBar*dt);
                s.CylinderBar=std::max(0.0,s.CylinderBar-lost/c.CylinderLitres);
                s.FreeflowLostLitres+=lost;
                s.FreeflowLocalLPS=lost/(s.AmbientBar*dt);
            }
        }
        s.RemainingGasMinutes=std::max(0.0,(s.CylinderBar-50)*c.CylinderLitres/
            ((s.SurfaceRMV+60*s.FreeflowLocalLPS)*s.AmbientBar));
        if(previousSource!=s.BreathSource&&hasBreathingSupply) s.BreathPhase=0;
        s.BreathPhase=std::fmod(s.BreathPhase+dt*s.RespiratoryBPM/60,1.0);
        const double lungDelta=hasBreathingSupply?.65*std::sin(s.BreathPhase*2*Pi):0;
        UpdateBalance(lungDelta);
        const auto& suit=SuitEquipment::At(c.DrySuit);
        const double suitCooling=suit.Cooling;
        // Qualitative heat-loss model: exposed air area cools more slowly than
        // immersed area. Wind affects air cooling only; no wet-suit drying model.
        const double coldLoad=suitCooling*(s.BodyImmersion*std::max(0.0,22-s.WaterTemperatureC)
            +(1-s.BodyImmersion)*.05*(1+c.WindMetresPerSecond/12)*std::max(0.0,22-c.AirTemperatureC));
        s.CoreTemperatureC=rules.Physiology?Clamp(s.CoreTemperatureC+dt*((37-s.CoreTemperatureC)*.0001-coldLoad),32,37.5):37;
        const double coldTarget=Clamp((18-s.EnvironmentTemperatureC)/18,0,1)*suit.ColdScale;
        s.ColdStrain=rules.Physiology?Approach(s.ColdStrain,coldTarget,1./600,dt):0;
        // A brief loss of supply spends reserve before saturation falls. Demand
        // follows activity; ventilation restores reserve over time, never on a toggle.
        // These constants tune gameplay and are not physiological reference values.
        const double demand=1+2*s.Exertion+.5*s.Stress+.5*s.Fatigue;
        if(!rules.Physiology) s.BreathReserve=1;
        else if(hasBreathingSupply) s.BreathReserve=Approach(s.BreathReserve,1,.12,dt);
        else s.BreathReserve=Clamp(s.BreathReserve-dt*demand/120,0,1);
        const double saturationTarget=45+53*Clamp(s.BreathReserve/.25,0,1);
        const double suppliedTarget=hasBreathingSupply?saturationTarget:std::min(s.SpO2,saturationTarget);
        s.SpO2=Approach(s.SpO2,unlimited?98:suppliedTarget,hasBreathingSupply?.08:.035,dt);
        s.DistanceMetres+=std::max(0.0,distance);
        s.LampBattery=Torch::At(Settings.TorchModel).BatteryAfter(s.LampBattery,dt,lamp);
        if(s.MaxDepthMetres>10 && depth>=3 && depth<=6 && std::abs(vz)<.2)
            s.StopRemainingSeconds=std::max(0.0,s.StopRemainingSeconds-dt);
        if(depth>10) s.StopRemainingSeconds=180;
        double ceilingBar=c.SurfacePressureBar, ndl=99;
        // No-supply tissue kinetics remain the existing approximation; retained
        // alveolar gas and breath-hold physiology are not modeled independently.
        const double inspired=s.BreathSource==BreathingSource::Air?(c.SurfacePressureBar-.0627)*.79:
            (s.AmbientBar-.0627)*(1-c.OxygenFraction);
        for(size_t i=0;rules.Decompression&&i<16;++i)
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
        if(rules.Physiology)
        {
            const double inertPressure=s.BreathSource==BreathingSource::Air?
                c.SurfacePressureBar*.79:s.AmbientBar*(1-c.OxygenFraction);
            const double narcoticTarget=Clamp((inertPressure-2.8)/3.5,0,1);
            s.Narcosis=Approach(s.Narcosis,narcoticTarget,.12,dt);
            const double violation=std::max(0.0,s.CeilingMetres-depth-.5);
            s.DecompressionBurden=Clamp(s.DecompressionBurden+dt*(violation>0?violation/600:-s.DecompressionBurden/1800),0,1);
            const double hypoxia=Clamp((96-s.SpO2)/36,0,1);
            s.MotorCapacity=Clamp(1-.35*s.Fatigue-.25*s.ColdStrain-.25*s.Narcosis
                -.2*s.Panic-.45*s.DecompressionBurden-.7*hypoxia,.15,1);
            s.VisualImpairment=Clamp(.35*s.Narcosis+.25*s.Panic+.35*s.DecompressionBurden+.9*hypoxia,0,1);
        }
        s.Warnings=None;
        if(!unlimited && s.CylinderBar<50) s.Warnings|=LowGas;
        if(!unlimited && s.CylinderBar<=.1) s.Warnings|=OutOfGas;
        if(!hasBreathingSupply) s.Warnings|=NoBreathingSupply;
        if(s.AscentMetresPerMinute>9) s.Warnings|=FastAscent;
        if(s.PPO2>1.4) s.Warnings|=HighPO2;
        if(s.CoreTemperatureC<35.5) s.Warnings|=Cold;
        if(s.CeilingMetres>depth+.5) s.Warnings|=CeilingViolation;
        if(depth>c.PlannedDepthMetres+1) s.Warnings|=DepthPlan;
        if(s.ElapsedSeconds>c.PlannedMinutes*60) s.Warnings|=TimePlan;
        if(s.RegulatorFreeflow) s.Warnings|=EquipmentFailure;
        if(s.Narcosis>.25) s.Warnings|=NarcosisWarning;
        if(s.Panic>.4) s.Warnings|=PanicWarning;
        if(s.DecompressionBurden>.15) s.Warnings|=DecompressionSymptoms;
        if(s.ColdStrain>.5) s.Warnings|=Cold;
    }
};
}
