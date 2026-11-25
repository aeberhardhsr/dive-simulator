#include "../divingsim/Source/DiveSim/Simulation/DiveModel.h"
#include "../divingsim/Source/DiveSim/Simulation/HeightGrid.h"
#include "../divingsim/Source/DiveSim/Simulation/Ecology.h"
#include "../divingsim/Source/DiveSim/Simulation/WaterSurface.h"
#include "../divingsim/Source/DiveSim/Simulation/AudioModel.h"
#include "../divingsim/Source/DiveSim/Simulation/Tutorial.h"
#include "../divingsim/Source/DiveSim/Simulation/ShoreMovement.h"
#include "../divingsim/Source/DiveSim/Simulation/VisualMedium.h"
#include "../divingsim/Source/DiveSim/Simulation/EnvironmentLight.h"
#include "../divingsim/Source/DiveSim/Simulation/SoakMetrics.h"
#include "../divingsim/Source/DiveSim/Simulation/ConfigInput.h"
#include "../divingsim/Source/DiveSim/Simulation/Bubbles.h"
#include "../divingsim/Source/DiveSim/Simulation/Sediment.h"
#include <iostream>
#include <cstdlib>
#include <limits>

int failures=0;
void check(bool condition,const char* message)
{ if(!condition) { std::cerr<<"FAIL: "<<message<<'\n'; ++failures; } }
void close(double a,double b,double tolerance,const char* message)
{ check(std::abs(a-b)<=tolerance,message); }
void run(Dive::Model& m,double seconds,double depth,double effort=0,double bcd=0)
{ for(int i=0;i<static_cast<int>(seconds*50);++i) m.Step(.02,depth,effort,bcd,0,0,false); }
int main()
{
    {
        using namespace Dive::Sediment;
        close(StirRate(.1,0,true,true),0,0,"Still fins do not stir sediment");
        close(StirRate(.9,1,true,true),0,0,"High fins do not reach sediment");
        close(StirRate(.1,1,false,true),0,0,"Dry fins do not emit underwater silt");
        close(StirRate(.1,1,true,false),0,0,"Rock and platforms do not emit sediment");
        close(StirRate(-.1,1,true,true),0,0,"Invalid negative bed clearance does not emit");
        close(StirRate(.1,std::numeric_limits<double>::infinity(),true,true),0,0,"Non-finite fin speed does not emit");
        close(StirRate(std::numeric_limits<double>::quiet_NaN(),1,true,true),0,0,"Non-finite bed clearance does not emit");
        check(StirRate(.1,1,true,true)>StirRate(.5,1,true,true),"Fin wash weakens with bed clearance");
        check(StirRate(.1,1,true,true)>StirRate(.1,.3,true,true),"Faster strokes stir more sediment");
        Puff a,b;for(int i=0;i<500;++i) a.Step(.02);for(int i=0;i<100;++i) b.Step(.1);
        close(a.OpticalDepth(),b.OpticalDepth(),1e-12,"Sediment dilution independent of integration step");
        check(a.Radius()>.22&&a.OpticalDepth()>0,"Puff expands while remaining visible");
        const double density=a.OpticalDepth();a.Step(20);
        check(a.OpticalDepth()<density&&a.Lift()<b.Lift(),"Sediment settles and thins after stirring stops");
        a.Step(16);check(!a.Alive(),"Sediment lifetime is bounded");
        close(a.OpticalDepth(),0,0,"Expired sediment has zero opacity before removal");
        const double age=a.Age;a.Step(-1);a.Step(std::numeric_limits<double>::quiet_NaN());
        close(a.Age,age,0,"Invalid time increments cannot alter sediment age");
    }
    {
        Dive::Config cfg;Dive::Model standard(cfg);
        close(standard.Data.EquipmentDryKg,34.12,1e-10,"Complete standard gear includes ballast and each rigid item once");
        close(standard.Data.TotalMassKg,117,1e-10,"80 kg body plus 34.12 kg equipment plus 2.88 kg gas");
        close(standard.Data.CylinderBar,200-8*.965/12,1e-10,"Surface preparation actually transfers eight litres from cylinder");
        close(standard.Data.CylinderGasKg+standard.Data.BCDGasKg,2.88,1e-10,"Preparation conserves carried gas mass");
        const double mass=standard.Data.TotalMassKg;
        standard.Step(1,0,0,1,0,0,false,{1,true,false});
        close(standard.Data.TotalMassKg,mass,1e-9,"Inflation transfers gas without inventing or discarding mass");
        const double volume=standard.Data.DisplacedLitres;
        standard.Step(1,0,0,-1,0,0,false,{1,true,false});
        close(mass-standard.Data.TotalMassKg,3*.965*.0012,1e-9,"Vented local litres remove their actual gas mass");
        close(volume-standard.Data.DisplacedLitres,3,1e-9,"Venting changes displaced freshwater volume");
        for(int material=0;material<2;++material) for(double size:{10.,12.,15.}) for(int bcd=0;bcd<3;++bcd)
        {
            cfg.CylinderMaterial=material;cfg.CylinderLitres=size;cfg.BCDModel=bcd;
            Dive::Model m(cfg);m.Step(1,20,0,0,0,0,false,{1,true,false});
            close(m.Data.BuoyancyNewtons,(m.Data.DisplacedLitres-m.Data.TotalMassKg)*9.80665,1e-9,"Archimedes net force is based on the entire loadout");
            const auto initial=m.Data;
            m.Step(10,0,0,1,0,0,false,{1,true,false});
            close(m.Data.BCDLitres,bcd==0?18:bcd==1?12:24,1e-8,"Each BCD has its own physical relief capacity");
            check(m.Data.TotalMassKg<initial.TotalMassKg,"Overpressure relief vents mass rather than creating hidden storage");
        }
        cfg=Dive::Config{};cfg.CylinderMaterial=1;Dive::Model aluminium(cfg);
        Dive::Model steel;
        check(aluminium.Data.TotalMassKg>steel.Data.TotalMassKg&&aluminium.Data.BuoyancyNewtons>steel.Data.BuoyancyNewtons,
            "Aluminium can be heavier on land while more buoyant in water");
        cfg=Dive::Config{};cfg.WeightKg=10;Dive::Model heavy(cfg);
        close(heavy.Data.TotalMassKg-steel.Data.TotalMassKg,4,1e-9,"Ballast changes inertia");
        close(heavy.Data.BuoyancyNewtons-steel.Data.BuoyancyNewtons,(4/11.34-4)*9.80665,1e-9,"Ballast displaces water and is not automatically trimmed away");
        cfg=Dive::Config{};cfg.TorchModel=2;Dive::Model wide(cfg);
        close(wide.Data.TotalMassKg-steel.Data.TotalMassKg,.4,1e-9,"Chosen torch contributes its own mass");
        close(wide.Data.BuoyancyNewtons-steel.Data.BuoyancyNewtons,(.23-.4)*9.80665,1e-9,"Chosen torch contributes displacement and net buoyancy");
        Dive::Model compression;const double suit= compression.Data.SuitLitres;
        compression.Step(.02,30,0,0,0,0,false,{1,true,false});
        check(compression.Data.SuitLitres<suit&&compression.Data.BCDLitres<8,"Depth compresses suit loft and BCD, not solid equipment");
        close(compression.Data.TotalMassKg,117,1e-9,"Compression preserves mass");
        cfg=Dive::Config{};cfg.CylinderLitres=1;cfg.StartPressureBar=1;Dive::Model scarce(cfg);
        close(scarce.Data.CylinderBar,0,1e-12,"Preparation cannot spend more gas than available");
        close(scarce.Data.BCDSurfaceLitres,1,1e-12,"Insufficient gas limits initial fill");
        close(Dive::ShoreMovement::VerticalForce(17,0,117),-117*9.80665,1e-9,"Dry weight uses loaded mass");
        close(Dive::ShoreMovement::VerticalForce(17,1,117),17,1e-9,"Full immersion retains net loadout force");
    }
    {
        Dive::Config c; c.GameMode=static_cast<Dive::Mode>(99); c.Sanitize();
        check(c.GameMode==Dive::Mode::Simulation,"Unknown mode falls back to simulation");
        c.GameMode=Dive::Mode::Exploration; c.WeightKg=20;
        Dive::Model explore(c); const auto tissues=explore.Data.NitrogenBar;
        explore.Step(180,55,1,-1,1,10,true,{1,true,false});
        close(explore.Data.CylinderBar,200,0,"Exploration cannot spend tank gas");
        close(explore.Data.MotorCapacity,1,0,"Exploration has no movement impairment");
        close(explore.Data.VisualImpairment,0,0,"Exploration has no visual impairment");
        close(explore.Data.BreathReserve,1,0,"Exploration reserve remains full without supply");
        check(explore.Data.NitrogenBar==tissues,"Exploration does not run tissue loading");
        check(c.Rules().PermanentHUD&&c.Rules().AutomaticHints&&!c.Rules().EquipmentFailures,"Exploration interface and failure policy");
        check(std::abs(explore.Data.BuoyancyNewtons)<7,"Exploration assists buoyancy by default");
        c.ManualExplorationBuoyancy=true; Dive::Model manual(c); manual.Step(.1,20,0,0,0,0,false);
        check(manual.Data.BuoyancyNewtons<-100,"Exploration permits manual buoyancy with ballast");
        for(auto mode:{Dive::Mode::Simulation,Dive::Mode::Hardcore})
        {
            c.GameMode=mode;c.AutoBuoyancy=true;
            Dive::Model limited(c);limited.Step(.1,20,0,0,0,0,false);
            check(limited.Data.BuoyancyNewtons<-100,"Legacy auto flag cannot bypass manual mode buoyancy");
            check(c.Rules().Decompression&&!c.Rules().PermanentHUD,"Advanced modes have deco and optional HUD");
            check(c.Rules().AutomaticHints==(mode==Dive::Mode::Simulation),"Hardcore disables coaching only");
        }
        c=Dive::Config{};c.CylinderLitres=30;c.StartPressureBar=300;c.PlannedDepthMetres=60;
        Dive::Model shallow(c),deep(c);run(shallow,60,10);run(deep,60,50);
        check(deep.Data.Narcosis>.4&&shallow.Data.Narcosis==0,"Narcosis follows inert partial pressure");
        check(deep.Data.MotorCapacity<shallow.Data.MotorCapacity&&deep.Data.VisualImpairment>.1,"Narcosis has perceptible consequences");
        const double narcosis=deep.Data.Narcosis;run(deep,30,5);
        check(deep.Data.Narcosis<narcosis*.1,"Narcosis recedes after reduced pressure");
        Dive::Model loss(c);loss.Step(70,5,1,0,0,0,false,{1,true,false});
        check(loss.Data.Panic>.4&&loss.Data.SpO2<80&&loss.Data.MotorCapacity<.5&&loss.Data.VisualImpairment>.5,"Prolonged supply loss couples panic and progressive impairment");
        const double impairment=loss.Data.VisualImpairment;run(loss,120,0);
        check(loss.Data.VisualImpairment<impairment*.1,"Restored ventilation gradually recovers capacity");
        c.DrySuit=false;c.SurfaceTemperatureC=c.BottomTemperatureC=4;
        Dive::Model cold(c);run(cold,900,5);
        c.DrySuit=true;Dive::Model dry(c);run(dry,900,5);
        check(cold.Data.ColdStrain>dry.Data.ColdStrain*3&&cold.Data.MotorCapacity<dry.Data.MotorCapacity,"Suit and exposure control cold impairment");
        c.SurfaceTemperatureC=c.BottomTemperatureC=20;
        Dive::Model deco(c);run(deco,600,50);check(deco.Data.CeilingMetres>0,"Exposure creates a ceiling before violation");
        run(deco,60,0);check(deco.Data.DecompressionBurden>.15,"Sustained ceiling violation accumulates impairment");
        const double burden=deco.Data.DecompressionBurden;run(deco,30,30);
        check(deco.Data.DecompressionBurden<burden&&deco.Data.DecompressionBurden>burden*.9,"Decompression strain persists after returning below ceiling");
        c.GameMode=Dive::Mode::Hardcore;c.SurfaceTemperatureC=c.BottomTemperatureC=4;
        Dive::Model fault(c),partition(c);run(fault,90,40,1);partition.Step(90,40,1,0,0,0,false);
        check(fault.Data.RegulatorFreeflow&&fault.Data.FreeflowLostLitres>100,"Hardcore cold high flow causes a real gas leak");
        close(fault.Data.CylinderBar,partition.Data.CylinderBar,1e-7,"Fault timing/gas independent of frame partition");
        close(fault.Data.MotorCapacity,partition.Data.MotorCapacity,1e-7,"Coupled impairment independent of frame partition");
        const double lost=fault.Data.FreeflowLostLitres;const double before=fault.Data.CylinderBar;
        run(fault,1,40,0);check(fault.Data.FreeflowLostLitres>lost&&fault.Data.CylinderBar<before,"Fault persists without strenuous breathing");
        run(fault,300,40);check(fault.Data.CylinderBar==0&&fault.Data.FreeflowLocalLPS==0,"Empty tank stops physical freeflow without negative gas");
        check(fault.Data.FreeflowLostLitres<=9000,"Leaked gas cannot exceed initial tank contents");
        fault.Reset(c);check(!fault.Data.RegulatorFreeflow&&fault.Data.FreeflowLostLitres==0,"A new dive services the regulator");
        c.GameMode=Dive::Mode::Simulation;Dive::Model noFault(c);run(noFault,90,40,1);
        check(!noFault.Data.RegulatorFreeflow&&noFault.Data.CylinderBar>partition.Data.CylinderBar,"Identical simulation exposure cannot trigger equipment faults");
        c.GameMode=Dive::Mode::Hardcore;c.SurfaceTemperatureC=c.BottomTemperatureC=20;
        Dive::Model warm(c);run(warm,90,40,1);
        check(!warm.Data.RegulatorFreeflow,"Warm high flow does not trigger cold icing fault");
    }
    {
        // Independent published data points, not values read back from the table.
        using E=Dive::EnvironmentScenario;
        close(E::Temperature({2026,2,15},0),5.836,1e-12,"Winter surface observation");
        close(E::Temperature({2026,5,15},30),6.121,1e-12,"Spring 30 m observation");
        close(E::Temperature({2026,8,15},0),17.085,1e-12,"Summer surface observation");
        close(E::Temperature({2026,11,15},100),5.430,1e-12,"Autumn deep observation");
        close(E::Temperature({2026,8,15},25),(9.494+7.603)/2,1e-12,"Linear interpolation in depth");
        close(E::Temperature({2026,6,30},0),(8.176+17.085)/2,1e-12,"46 of 92 days between spring and summer");
        close(E::Temperature({2024,2,29},0),5.836+(8.176-5.836)*14/90,1e-12,"Leap year seasonal fraction");
        close(E::Temperature({2026,1,1},0),12.455+(5.836-12.455)*47/92,1e-12,"Previous autumn through new year");
        close(E::Temperature({2000,1,1},-10),E::Temperature({2000,1,1},0),0,"Depth clamps at surface");
        close(E::Temperature({2099,12,31},151),E::Temperature({2099,12,31},144),0,"Deep endpoint extends below observations");
        Dive::Config plan; plan.Date={2026,8,15}; plan.VisibilityMetres=27;plan.CurrentMetresPerSecond=.17;
        const auto manual=plan.Resolved();close(manual.VisibilityMetres,27,0,"Manual visibility unchanged");
        plan.AutoTemperature=plan.AutoVisibility=true;plan.CurrentSource=Dive::CurrentSetting::Automatic;
        const auto resolved=plan.Resolved();
        close(resolved.SurfaceTemperatureC,17.085,1e-12,"Auto surface preview");
        close(resolved.VisibilityMetres,12,1e-12,"Authored summer clear visibility");
        close(resolved.CurrentMetresPerSecond,.072,1e-12,"Authored summer wind current");
        close(plan.VisibilityMetres,27,0,"Resolving preserves manual visibility");
        close(plan.CurrentMetresPerSecond,.17,0,"Resolving preserves manual current");
        close(resolved.Resolved().VisibilityMetres,resolved.VisibilityMetres,0,"Resolve is idempotent");
        Dive::Model model(plan);close(model.Temperature(25),8.5485,1e-12,"Runtime uses entire profile");
        model.Step(.02,25,0,0,0,0,false);
        close(model.Data.WaterTemperatureC,8.5485,1e-12,"Recorded water temperature uses seasonal profile");
        plan.Date={2026,2,15};close(model.Temperature(0),17.085,1e-12,"Active dive date is frozen");
        plan.SelectWeather(3);const auto storm=plan.Resolved();
        check(storm.VisibilityMetres<resolved.VisibilityMetres&&storm.CurrentMetresPerSecond>resolved.CurrentMetresPerSecond,"Storm changes scenario visibility and current");
        for(auto source:{Dive::CurrentSetting::None,Dive::CurrentSetting::Light,Dive::CurrentSetting::Strong})
        { plan.CurrentSource=source;close(plan.Resolved().CurrentMetresPerSecond,source==Dive::CurrentSetting::None?0:source==Dive::CurrentSetting::Light?.08:.3,0,"Current presets"); }
        plan.AutoTemperature=plan.AutoVisibility=false;plan.CurrentSource=Dive::CurrentSetting::Custom;
        close(plan.Resolved().VisibilityMetres,27,0,"Disabling auto restores manual visibility");
        close(plan.Resolved().SurfaceTemperatureC,18,0,"Disabling auto restores manual water");
        close(plan.Resolved().CurrentMetresPerSecond,.17,0,"Custom restores manual current");
        for(int year:{2000,2024,2026,2099})
            for(int day=Dive::CivilDate{year,1,1}.SerialDay();day<=Dive::CivilDate{year,12,31}.SerialDay();++day)
            {
                const auto date=Dive::CivilDate::FromSerialDay(day),next=Dive::CivilDate::FromSerialDay(day+1);
                if(next.ScenarioRange()) check(std::abs(E::Temperature(next,0)-E::Temperature(date,0))<.15,"Daily seasonal continuity");
                for(double depth:{0.,5.,25.,100.,151.}) check(E::Temperature(date,depth)>5&&E::Temperature(date,depth)<18,"Seasonal bounded envelope");
            }
    }
    {
        // Independent pressure * sphere-volume invariant across an ascent.
        const double initial=.006, start=5.965;
        const double gas=start*(4*3.14159265358979323846/3)*initial*initial*initial;
        double previous=initial;
        for(double pressure=start;pressure>=.965;pressure-=.1)
        {
            const double radius=Dive::Bubble::RadiusAt(initial,start,pressure);
            close(pressure*(4*3.14159265358979323846/3)*std::pow(radius,3),gas,1.e-15,"Bubble pressure-volume is conserved during ascent");
            check(radius>=previous-1.e-12,"Ascending bubble expands");previous=radius;
        }
        close(Dive::Bubble::RadiusAt(.01,8,1),.02,1.e-12,"Eightfold volume gives twice the radius");
        check(Dive::Bubble::RadiusAt(.01,1,0)==0,"Invalid bubble pressure cannot generate nonfinite geometry");
    }
    {
        for(int i=0;i<Dive::Torch::Count;++i)
        {
            const auto& lamp=Dive::Torch::At(i);
            check(lamp.InnerDegrees>0&&lamp.InnerDegrees<lamp.OuterDegrees&&lamp.OuterDegrees<90,"Torch cones are valid half angles");
            close(lamp.BatteryAfter(100,lamp.RuntimeMinutes*60,true),0,1.e-8,"Torch reaches empty at rated game runtime");
            close(lamp.BatteryAfter(70,600,false),70,1.e-8,"Switched off torch preserves battery");
            close(lamp.Output(5),lamp.Lumens*.5,1.e-8,"Low battery smoothly reduces output");
            close(lamp.Output(0),0,1.e-8,"Empty battery cannot emit light");
            Dive::Config c;c.TorchModel=i;c.GameMode=Dive::Mode::Exploration;
            Dive::Model model(c);model.Step(10,5,0,0,0,0,true);
            close(model.Data.LampBattery,100-100*10/(lamp.RuntimeMinutes*60),1.e-6,"Simulation uses chosen torch drain even in exploration");
            const double battery=model.Data.LampBattery;model.Step(10,5,0,0,0,0,false);
            close(model.Data.LampBattery,battery,1.e-8,"Simulation preserves switched-off battery");
        }
        check(Dive::Torch::At(2).BatteryAfter(100,3600,true)==0,"High output torch cannot run beyond battery duration");
        check(Dive::Torch::At(1).BatteryAfter(100,3600,true)>60,"Spot torch lasts longer at lower power");
    }
    {
        using Dive::CivilDate; using Dive::ScenarioClock;
        check(CivilDate{1970,1,1}.SerialDay()==0,"Gregorian epoch is stable");
        check(CivilDate::Leap(2000)&&!CivilDate::Leap(2100),"Gregorian century leap rule");
        CivilDate date;
        check(CivilDate::Parse("2024-02-29",date)&&date==CivilDate{2024,2,29},"Leap-day input is accepted");
        check(!CivilDate::Parse("2025-02-29",date)&&date==CivilDate{2024,2,29},"Invalid date leaves the previous date intact");
        check(!CivilDate::Parse("2026-13-01",date)&&!CivilDate::Parse("1999-12-31",date),"Scenario date range is explicit");
        for(int year=2000;year<=2100;++year) for(int month=1;month<=12;++month)
            for(int day=1;day<=CivilDate::DaysInMonth(year,month);++day)
            { const CivilDate d{year,month,day}; check(CivilDate::FromSerialDay(d.SerialDay())==d,"Gregorian date round-trip across a full century"); }
        const auto leap=ScenarioClock::At({2024,2,28},23+59./60,120);
        check(leap.Date==CivilDate{2024,2,29},"Simulation advances into leap day");
        close(leap.Hour,1./60,1.e-9,"Clock keeps the minute after midnight");
        check(ScenarioClock::At({2026,12,31},23+59./60,120).Date==CivilDate{2027,1,1},"Simulation advances into a new year");
        const auto spring=ScenarioClock::At({2026,3,29},1+59./60,120);
        close(spring.Hour,3+1./60,1.e-9,"Swiss spring clock jumps forward without losing elapsed time");
        check(spring.OffsetMinutes==120,"Spring uses CEST");
        const auto autumn=ScenarioClock::At({2026,10,25},2+59./60,120);
        close(autumn.Hour,2+1./60,1.e-9,"Swiss autumn clock repeats its hour while UTC stays continuous");
        check(autumn.OffsetMinutes==60,"Autumn returns to CET");
        double utc=123;
        check(!ScenarioClock::StartUTC({2026,3,29},2.5,utc)&&utc==123,"Nonexistent spring time is rejected atomically");
        check(ScenarioClock::StartUTC({2026,10,25},2.5,utc)&&ScenarioClock::OffsetAtUTC(utc)==120,"Ambiguous autumn input selects the first occurrence");
        const auto before=ScenarioClock::At({2026,10,25},2+59./60,0);
        close(autumn.UTCSeconds-before.UTCSeconds,120,1.e-9,"DST transition never rewinds simulation UTC");
        check(ScenarioClock::At({},13,86400*3).Date==CivilDate{2026,6,24},"Multi-day dives advance the date");
        close(ScenarioClock::At({},13,NAN).Hour,13,1.e-9,"Nonfinite elapsed time cannot poison the calendar");
    }
    {
        const auto& hour=*Dive::FindConfigInput("hour");
        double value=13;
        check(Dive::ParseConfigInput(hour,"23:59",value)==Dive::ConfigInputError::None,"Clock accepts final minute");
        close(value,23+59./60,1e-12,"Clock preserves minutes");
        check(Dive::FormatConfigInput(hour,value)=="23:59","Clock displays minutes");
        for(const char* invalid:{"24:00","12:60","-1:00","12:5","12","13:00x","NaN",""})
        {
            const double previous=value;
            check(Dive::ParseConfigInput(hour,invalid,value)!=Dive::ConfigInputError::None&&value==previous,"Invalid time preserves previous value");
        }
        check(Dive::ParseConfigInput(hour," 7:05 ",value)==Dive::ConfigInputError::None&&Dive::FormatConfigInput(hour,value)=="07:05","Clock accepts one digit hour");
        const auto& weight=*Dive::FindConfigInput("weight");
        check(Dive::ParseConfigInput(weight,"7,5",value)==Dive::ConfigInputError::None&&value==7.5,"Decimal comma accepted");
        for(const char* invalid:{"NaN","inf","1e1","1.2.3","1,2.3","7 kg","-1","20.1","1.25","+","."})
        {
            const double previous=value;
            check(Dive::ParseConfigInput(weight,invalid,value)!=Dive::ConfigInputError::None&&value==previous,"Invalid numeric input does not mutate value");
        }
        check(Dive::ParseConfigInput(*Dive::FindConfigInput("air_temp"),"-3.5",value)==Dive::ConfigInputError::None&&value==-3.5,"Negative air temperature accepted");
        check(Dive::ParseConfigInput(*Dive::FindConfigInput("current"),"0.17",value)==Dive::ConfigInputError::None&&value==.17,"Current precision retained");
        check(!Dive::FindConfigInput("unknown"),"Unknown input rejected");
        for(const auto& input:Dive::ConfigInputs)
        {
            for(double boundary:{input.Minimum,input.Maximum})
            {
                const auto text=Dive::FormatConfigInput(input,boundary);
                check(Dive::ParseConfigInput(input,text,value)==Dive::ConfigInputError::None,"Every advertised boundary is accepted");
                Dive::Config c; c.*(input.Member)=value; c.Sanitize();
                close(c.*(input.Member),value,1e-12,"Accepted input is preserved by simulation sanitization");
            }
        }
    }
    {
        Dive::SoakMetrics m;
        check(!m.Add(0)&&!m.Add(-1)&&!m.Add(std::numeric_limits<double>::quiet_NaN()),"Soak rejects invalid intervals");
        for(int i=0;i<98;++i) m.Add(.010);
        m.Add(.040); m.Add(2.);
        check(m.Frames==100&&m.Over30==2&&m.Over100==1,"Soak retains interior stalls and counts thresholds");
        close(m.Seconds,3.02,1e-10,"Soak wall time accumulated");
        close(m.PercentileUpper(.95),10.25,1e-10,"Soak quarter-ms upper percentile bound");
        close(m.PercentileUpper(.99),40.25,1e-10,"Soak nearest-rank percentile keeps hitch");
        close(m.PercentileUpper(1),2000,1e-10,"Soak overflow reports exact maximum");
    }
    {
        Dive::Config cfg; cfg.SurfaceTemperatureC=6; cfg.BottomTemperatureC=6; cfg.AirTemperatureC=20; cfg.DrySuit=false;
        const Dive::Exposure air{0,false,false},wet{1,true,true},noSupply{1,true,false};
        Dive::Model freeAir(cfg),regulatedAir(cfg),water(cfg),held(cfg);
        const double preparedPressure=freeAir.Data.CylinderBar;
        freeAir.Step(60,0,0,0,0,0,false,air);
        close(freeAir.Data.CylinderBar,preparedPressure,0,"Ambient-air breathing does not consume cylinder gas after preparation");
        check(freeAir.Data.BreathSource==Dive::BreathingSource::Air&&freeAir.Data.SpO2>97,"Ambient air supplies breathing");
        regulatedAir.Step(60,0,0,0,0,0,false,{0,false,true});
        check(regulatedAir.Data.CylinderBar<199,"Regulator still consumes cylinder gas above water");
        water.Step(60,0,0,0,0,0,false,wet);
        check(water.Data.CoreTemperatureC<regulatedAir.Data.CoreTemperatureC,"Immersion cools more than dry air in the same suit");
        close(freeAir.Data.EnvironmentTemperatureC,20,1e-12,"Dry body uses air temperature");
        close(water.Data.EnvironmentTemperatureC,6,1e-12,"Immersed body uses water temperature");
        held.Step(5,10,0,0,0,0,false,noSupply);
        close(held.Data.CylinderBar,preparedPressure,0,"Removed regulator does not invent cylinder consumption");
        check(held.Data.SpO2>97.99&&held.Data.BreathReserve<.99&&(held.Data.Warnings&Dive::NoBreathingSupply),"Brief loss of supply spends reserve before oxygen saturation falls");
        const double spent=held.Data.BreathReserve;
        held.Step(.02,10,0,0,0,0,false,wet);
        check(held.Data.BreathReserve>spent&&held.Data.BreathReserve<.999,"One supplied step cannot refill the reserve instantly");
        held.Step(30,10,0,0,0,0,false,wet);
        check(held.Data.SpO2>95&&!(held.Data.Warnings&Dive::NoBreathingSupply),"Replacing supplied regulator restores breathing");
        const double before=freeAir.Data.CylinderBar;
        freeAir.Step(1,0,0,1,0,0,false,air);
        close(before-freeAir.Data.CylinderBar,2*cfg.SurfacePressureBar/cfg.CylinderLitres,1e-9,"BCD draws gas independently of mouthpiece use");
        Dive::Model empty(cfg); empty.Data.CylinderBar=0;
        empty.Step(5,0,0,0,0,0,false,{0,false,true});
        check(empty.Data.BreathSource==Dive::BreathingSource::None,"Empty regulator cannot supply breathing even above water");
        empty.Step(30,0,0,0,0,0,false,air);
        check(empty.Data.SpO2>95&&empty.Data.BreathSource==Dive::BreathingSource::Air,"Removing empty regulator above water permits air breathing");
        Dive::AudioModel quietAir;
        const auto sound=quietAir.Step(.02,freeAir.Data,cfg,-1);
        check(!sound.Inhale&&!sound.Exhale,"Ambient-air breathing does not play regulator hiss");
        Dive::Model resting(cfg),swimming(cfg),partitioned(cfg),waves(cfg);
        resting.Step(30,10,0,0,0,0,false,noSupply);
        swimming.Step(30,10,1,0,0,0,false,noSupply);
        check(swimming.Data.BreathReserve<resting.Data.BreathReserve,"Swimming spends the same initial reserve faster than resting");
        for(int i=0;i<1500;++i) partitioned.Step(.02,10,1,0,0,0,false,noSupply);
        close(swimming.Data.BreathReserve,partitioned.Data.BreathReserve,1e-10,"Breath reserve is invariant to frame partitioning");
        close(swimming.Data.SpO2,partitioned.Data.SpO2,1e-9,"Reserve-linked saturation is invariant to frame partitioning");
        for(int i=0;i<60;++i) { waves.Step(.5,0,0,0,0,0,false,noSupply); waves.Step(1.5,0,0,0,0,0,false,air); }
        check(waves.Data.SpO2>97.99&&waves.Data.BreathReserve>.9,"Repeated brief wave contact does not deoxygenate an otherwise breathing player");
        swimming.Step(90,10,1,0,0,0,false,noSupply);
        check(swimming.Data.SpO2<60&&swimming.Data.BreathReserve==0,"Prolonged supply loss still reaches the terminal gameplay threshold");
        const double lowSpO2=swimming.Data.SpO2;
        swimming.Step(1,10,0,0,0,0,false,wet);
        check(swimming.Data.BreathReserve>0&&swimming.Data.BreathReserve<.2&&swimming.Data.SpO2<80,"Restoring supply does not reset depleted reserve or saturation");
        swimming.Step(60,10,0,0,0,0,false,wet);
        check(swimming.Data.BreathReserve>.99&&swimming.Data.SpO2>97&&swimming.Data.SpO2>lowSpO2,"Sustained ventilation restores reserve and simulated saturation");
        Dive::AudioModel waveAlerts; Dive::State waveState;
        waveState.Warnings=Dive::NoBreathingSupply;
        check(waveAlerts.Step(.02,waveState,cfg,0).Warning,"First supply loss alerts immediately");
        int repeatedAlerts=0;
        for(int i=0;i<100;++i) { waveState.Warnings=i%2?Dive::NoBreathingSupply:Dive::None; repeatedAlerts+=waveAlerts.Step(.02,waveState,cfg,0).Warning?1:0; }
        check(repeatedAlerts==0,"Rapid wave crossings do not chatter urgent alerts");
        waveState.Warnings=0; waveAlerts.Step(8,waveState,cfg,0);
        waveState.Warnings=Dive::NoBreathingSupply;
        check(waveAlerts.Step(.02,waveState,cfg,0).Warning,"New loss after cooldown still alerts");
    }
    const auto noon=Dive::EnvironmentLight::AtElevation(65,0),night=Dive::EnvironmentLight::AtElevation(-30,0);
    check(noon.SolarSource>0&&noon.ElevationDegrees>0,"Daytime sun is above the horizon");
    check(night.ElevationDegrees<0,"Night has no fictitious above-horizon sun");
    close(night.WaterAmbient(0,14),0,0,"Unlit night water has no ambient emission");
    check(noon.WaterAmbient(20,14)<noon.WaterAmbient(2,14),"Ambient daylight decays with water depth");
    check(noon.WaterAmbient(20,4)<noon.WaterAmbient(20,30),"Turbid water attenuates ambient light faster");
    check(noon.WaterAmbient(100,14)<.003,"Deep water cannot retain surface brightness");
    check(Dive::EnvironmentLight::AtElevation(65,2).WaterAmbient(5,14)<noon.WaterAmbient(5,14),"Rain dims underwater ambient light");
    check(Dive::EnvironmentLight::AtElevation(65,3).SolarSource<Dive::EnvironmentLight::AtElevation(65,2).SolarSource&&
          Dive::EnvironmentLight::AtElevation(65,3).Diffuse<Dive::EnvironmentLight::AtElevation(65,2).Diffuse,"Storm attenuates direct and diffuse daylight");
    close(Dive::EnvironmentLight::AtElevation(-30,3).Diffuse,0,0,"Storm cannot add fictitious night ambient light");
    close(night.SolarSource,noon.SolarSource,0,"Atmospheric sun stays illuminated outside the atmosphere at night");
    const auto horizon=Dive::EnvironmentLight::AtElevation(0,0),twilight=Dive::EnvironmentLight::AtElevation(-8.5,0);
    check(horizon.Diffuse>0 && twilight.Diffuse>0 && twilight.Diffuse<horizon.Diffuse,"Twilight fades gradually beyond the horizon");
    check(std::abs(Dive::EnvironmentLight::AtElevation(1.e-8,0).Diffuse-Dive::EnvironmentLight::AtElevation(-1.e-8,0).Diffuse)<1.e-8,"Ambient source has no discontinuity at sunset");
    double previous=1;
    for(int angle=90;angle>=-90;--angle)
    {
        const auto light=Dive::EnvironmentLight::AtElevation(angle,0);
        check(light.Diffuse>=0 && light.Diffuse<=previous+1.e-12,"Afternoon and twilight ambient decrease monotonically");
        check(std::isfinite(light.ExposureBias(0,14)),"Twilight exposure remains finite");
        previous=light.Diffuse;
    }
    close(Dive::EnvironmentLight::AtElevation(-8.5,0).Diffuse,twilight.Diffuse,1.e-12,"Dawn and dusk use the same scattering approximation");
    close(noon.WaterAmbient(-5,14),noon.WaterAmbient(0,14),0,"Air position cannot amplify underwater source");
    check(std::isfinite(Dive::EnvironmentLight::AtElevation(NAN,0).WaterAmbient(NAN,NAN)),"Lighting handles invalid inputs");
    check(night.ExposureBias(0,14)>noon.ExposureBias(0,14),"Eyes gain sensitivity at night");
    check(horizon.ExposureBias(0,14)-noon.ExposureBias(0,14)<6,"Camera does not fully compensate away dusk darkness");
    close(night.ExposureBias(0,14),11.5,1.e-12,"Unlit-night sensitivity remains bounded after twilight changes");
    check(Dive::EnvironmentLight::AtElevation(-30,0,45,.1).WaterAmbient(0,14)>0,"Above-horizon moon contributes underwater ambient");
    close(Dive::EnvironmentLight::AtElevation(-30,0,-5,.1).WaterAmbient(0,14),0,0,"Below-horizon moon cannot illuminate water");
    check(noon.ExposureBias(40,14)>noon.ExposureBias(2,14),"Eyes gain sensitivity as ambient daylight fades");
    check(noon.ExposureBias(5000,2)<=night.ExposureBias(0,14),"Deep-water exposure cannot exceed the dark limit");
    close(Dive::CameraWaterBlend(-2),0,0,"Air camera is independent of body immersion");
    close(Dive::CameraWaterBlend(0),0,0,"Surface camera retains the air endpoint");
    close(Dive::CameraWaterBlend(.5),1,0,"Submerged camera reaches the water endpoint");
    close(Dive::CameraWaterBlend(.25),.5,1e-12,"Optical transition is symmetric");
    check(Dive::CameraWaterBlend(.001)<.00002,"Waterline transition starts smoothly");
    check(1-Dive::CameraWaterBlend(.499)<.00002,"Waterline transition finishes smoothly");
    close(Dive::VisualMedium::At(-1,0,2).Density,Dive::VisualMedium::At(-1,0,40).Density,0,
          "Underwater visibility must not tint or thicken air fog");
    check(Dive::VisualMedium::At(-1,2,14).Density>Dive::VisualMedium::At(-1,0,14).Density,
          "Rain increases aerial haze");
    close(Dive::VisualMedium::At(3,2,14).Density,Dive::VisualMedium::At(3,0,14).Density,0,
          "Underwater fog does not inherit the aerial weather density");
    check(Dive::VisualMedium::At(-1,3,14).Density>Dive::VisualMedium::At(-1,2,14).Density,"Storm increases aerial haze beyond ordinary rain");
    close(Dive::VisualMedium::At(3,3,14).Density,Dive::VisualMedium::At(3,0,14).Density,0,"Storm preserves independent underwater visibility");
    check(Dive::VisualMedium::At(3,0,2).Density>Dive::VisualMedium::At(3,0,40).Density,
          "Lower underwater visibility increases extinction");
    close(Dive::VisualMedium::At(3,0,14).Density,.35,1e-12,"Retain established submerged fog density");
    check(std::isfinite(Dive::VisualMedium::At(NAN,2,NAN).Density),"Invalid optical inputs stay finite");
    using namespace Dive::ShoreMovement;
    close(Immersion(2,0),0,0,"Dry body displaces no water");
    close(Immersion(0,0),.5,1e-12,"Half immersed upright body");
    close(Immersion(-2,0),1,0,"Submerged body displaces its full volume");
    close(VerticalForce(0,0),-100*Dive::Gravity,1e-10,"Gravity acts fully on land");
    close(VerticalForce(35,1),35,1e-10,"Underwater force preserves existing buoyancy model");
    check(CanStand(.96,0,0,false),"Dry feet can stand");
    check(!CanStand(2,0,0,true),"Airborne body cannot walk in midair");
    check(CanStand(.96,0,1.5,true)&&!CanStand(.96,0,1.5,false),"Shore transition has hysteresis");
    check(!CanStand(.96,0,1.6,true),"Deep water releases walking support");
    close(WalkSpeed(false,0),1.1,1e-12,"Loaded walking pace");
    check(WalkSpeed(true,1)<WalkSpeed(true,0),"Fatigue slows land locomotion");
    Dive::Tutorial lesson; Dive::State lessonState;
    Dive::Tutorial viewLesson; viewLesson.ObserveView(179,20); viewLesson.ObserveView(-179,20);
    close(viewLesson.LookDegrees,2,1e-12,"Tutorial view tracking handles compass wrap without a false full turn");
    viewLesson.ObserveView(-179,20);
    close(viewLesson.LookDegrees,2,1e-12,"Unchanged camera view cannot advance looking lesson");
    auto lessonStep=[&](double distance=0,bool swimming=false,bool computer=false,int page=0,bool lamp=true,bool map=false,double entry=5)
    { lesson.Step(.02,lessonState,distance,swimming,computer,page,lamp,map,entry); };
    lessonStep(10,true,true,1,false,true);
    check(lesson.Stage==Dive::Lesson::Look,"Tutorial cannot skip looking due to unrelated actions");
    lesson.Look(24); lessonStep(); check(lesson.Stage==Dive::Lesson::Look,"Tutorial needs actual looking input");
    lesson.Look(1); lessonStep(); check(lesson.Stage==Dive::Lesson::Swim,"Looking advances tutorial");
    lessonStep(5,false); close(lesson.SwimMetres,0,0,"Current drift does not complete swimming lesson");
    lessonStep(3,true); check(lesson.Stage==Dive::Lesson::Buoyancy,"Swimming covers required distance");
    lessonState.BCDInflowLPS=2;
    for(int i=0;i<40;++i) lessonStep();
    lessonState.BCDInflowLPS=0; lessonState.BCDOutflowLPS=3;
    for(int i=0;i<26;++i) lessonStep();
    check(lesson.Stage==Dive::Lesson::Computer,"Actual inflation then venting advances BCD lesson");
    lessonStep(0,false,true,0); check(lesson.Stage==Dive::Lesson::Computer,"Computer must show requested page");
    lessonStep(0,false,true,1); check(lesson.Stage==Dive::Lesson::Computer,"Computer lesson also requires lowering the wrist");
    lessonStep(); lessonStep(); check(lesson.Stage==Dive::Lesson::Lamp,"Already lit torch cannot skip off/on exercise");
    lessonStep(0,false,false,0,false); lessonStep();
    check(lesson.Stage==Dive::Lesson::Map,"Torch off then on advances lesson");
    lessonStep(0,false,false,0,true,true); lessonStep();
    check(lesson.Stage==Dive::Lesson::Return,"Map must be opened and closed");
    lessonState.DepthMetres=3; lessonStep(); check(lesson.Stage==Dive::Lesson::Return,"Return needs shallow depth");
    lessonState.DepthMetres=.5; lessonStep(0,false,false,0,true,false,20);
    check(lesson.Stage==Dive::Lesson::Return,"Return needs proximity to entry");
    lessonStep(); check(lesson.Stage==Dive::Lesson::Exit,"Surface near entry unlocks exit lesson");
    check(!lesson.Finish(false)&&lesson.Finish(true)&&lesson.Stage==Dive::Lesson::Done,"Only successful finish completes introduction");
    Dive::AudioModel sound;
    Dive::State audioState; Dive::Config audioConfig;
    int inhales=0,exhales=0,beats=0;
    audioState.HeartBPM=90; audioState.RespiratoryBPM=20;
    for(int i=0;i<3000;++i)
    {
        audioState.BreathPhase=std::fmod(i*.02*20/60,1.);
        const auto frame=sound.Step(.02,audioState,audioConfig,2);
        inhales+=frame.Inhale; exhales+=frame.Exhale; beats+=frame.Heartbeat;
    }
    check(inhales==20&&exhales==20,"Audio inhale and exhale follow 20 breaths/minute without duplicate events");
    check(beats==90,"Audio heartbeat follows 90 bpm for one minute");
    Dive::AudioModel emptyAudio; audioState.CylinderBar=0;
    audioState.BreathPhase=0;
    check(!emptyAudio.Step(.02,audioState,audioConfig,10).Inhale,"Empty cylinder cannot produce regulator inhalation");
    audioState.BreathPhase=.6;
    check(!emptyAudio.Step(.02,audioState,audioConfig,10).Exhale,"No new lungful means no exhale bubbles");
    Dive::AudioModel lastBreath; audioState.CylinderBar=1; audioState.BreathPhase=0;
    check(lastBreath.Step(.02,audioState,audioConfig,10).Inhale,"Gas starts a breath");
    audioState.CylinderBar=0; audioState.BreathPhase=.6;
    check(lastBreath.Step(.02,audioState,audioConfig,10).Exhale,"Last lungful can be exhaled after gas exhaustion");
    audioConfig.Weather=2; audioConfig.WindMetresPerSecond=12;
    const auto above=sound.Step(.02,audioState,audioConfig,-.1);
    const auto deepAudio=sound.Step(.02,audioState,audioConfig,20);
    check(above.Wet==0&&above.Wind==1&&above.Rain==1,"Air exposes wind and rain");
    check(deepAudio.Wet==1&&deepAudio.Wind<.001&&deepAudio.Rain<.001&&deepAudio.CutoffHz<1000,"Depth attenuates and low-pass filters surface layers");
    Dive::Config stormConfig; stormConfig.SelectWeather(3); stormConfig.Sanitize();
    check(stormConfig.Weather==3&&stormConfig.WindMetresPerSecond==22,"Storm selection applies strong wind before simulation");
    Dive::AudioModel stormAudio;
    const auto stormAir=stormAudio.Step(.02,audioState,stormConfig,-1),stormDeep=stormAudio.Step(.02,audioState,stormConfig,30);
    check(stormAir.Wind==1&&stormAir.Rain>above.Rain,"Storm wind gain stays bounded while rain intensifies");
    check(stormDeep.Rain<.001&&stormDeep.Wind<.001,"Storm surface sounds attenuate with depth");
    stormConfig.WindMetresPerSecond=0; stormConfig.Sanitize();
    check(stormConfig.Weather==3&&stormConfig.WindMetresPerSecond==0,"Explicit wind override survives sanitizing and loading");
    audioState.Warnings=Dive::LowGas;
    check(sound.Step(.02,audioState,audioConfig,2).Warning,"New warning produces an alert");
    check(!sound.Step(.02,audioState,audioConfig,2).Warning,"Persistent warning does not repeat every frame");
    audioState.Warnings|=Dive::OutOfGas;
    check(sound.Step(.02,audioState,audioConfig,2).Warning,"Out of gas bypasses warning cooldown");
    Dive::AudioLevels levels; levels.Values[0]=0;
    for(size_t i=1;i<5;++i) close(levels.Gain(i),0,0,"Master mute covers every category");
    levels.Values[2]=std::numeric_limits<double>::quiet_NaN(); levels.Sanitize();
    close(levels.Values[2],.85,0,"Invalid audio preferences use finite defaults");
    Dive::Model valves; valves.Data.CylinderBar=0; valves.Data.BCDSurfaceLitres=0;
    valves.Step(.02,10,0,1,0,0,false);
    close(valves.Data.BCDInflowLPS,0,0,"Empty cylinder has no inflation flow or hiss");
    valves.Step(.02,10,0,-1,0,0,false);
    close(valves.Data.BCDOutflowLPS,0,0,"Empty BCD has no venting flow or sound");
    valves.Reset({}); valves.Step(.02,10,0,1,0,0,false);
    close(valves.Data.BCDInflowLPS,2,1e-12,"Inflation audio uses actual local flow");
    valves.Step(.02,10,0,-1,0,0,false);
    close(valves.Data.BCDOutflowLPS,3,1e-12,"Venting audio uses actual local flow");
    Dive::WaterSurface water; water.Configure(0,90);
    close(water.Sample(30,-50,10).Height,0,0,"No wind gives a flat geometric surface");
    close(water.PressureDepth(30,-50,-20,10),20,1e-12,"Calm water preserves hydrostatic depth");
    water.Configure(12,45);
    const double eps=1e-4;
    for(double t=0;t<60;t+=.13)
    {
        const auto s=water.Sample(54,-91,t);
        check(std::abs(s.Height)<=water.AmplitudeBound()+1e-12,"Wave height respects its amplitude envelope");
        close(s.GradientX,(water.Sample(54+eps,-91,t).Height-water.Sample(54-eps,-91,t).Height)/(2*eps),1e-7,"Analytical wave slope matches numerical derivative");
        close(s.VerticalVelocity,(water.Sample(54,-91,t+eps).Height-water.Sample(54,-91,t-eps).Height)/(2*eps),1e-7,"Wave velocity matches time derivative");
        close(water.PressureDepth(54,-91,s.Height+.01,t),0,0,"Camera above wave crest is in air");
        close(water.PressureDepth(54,-91,-100,t),100,1e-10,"Short-wave pressure perturbation decays at depth");
        close(water.Velocity(54,-91,0,t).Z,s.VerticalVelocity,1e-12,"Linear orbital velocity meets surface kinematic condition");
        const auto deepVelocity=water.Velocity(54,-91,-100,t);
        check(std::hypot(deepVelocity.X,deepVelocity.Y)<1e-12&&std::abs(deepVelocity.Z)<1e-12,"Wave orbital motion decays at depth");
    }
    for(const auto& wave:water.Waves)
        close(wave.Omega*wave.Omega,9.80665*std::hypot(wave.Kx,wave.Ky),1e-12,"Deep-water dispersion relation");
    for(double wind:{22.,Dive::WeatherScenario::MaximumWind})
    {
        Dive::WaterSurface storm; storm.Configure(wind,125);
        check(storm.AmplitudeBound()>water.AmplitudeBound()&&storm.AmplitudeBound()<=Dive::WaterSurface::MaximumDisplacement+1e-12,"Storm waves fit expanded rendering envelope");
        for(size_t i=0;i<storm.Waves.size();++i)
        {
            const auto& w=storm.Waves[i]; const auto& old=water.Waves[i];
            check(w.Omega<old.Omega,"Stronger wind increases dominant wave period");
            close(w.Amplitude*std::hypot(w.Kx,w.Ky),old.Amplitude*std::hypot(old.Kx,old.Ky),1e-12,"Wind extension preserves bounded component steepness");
            close(w.Omega*w.Omega,9.80665*std::hypot(w.Kx,w.Ky),1e-12,"Storm retains deep-water dispersion");
        }
        for(double t=0;t<120;t+=.43)
        {
            const auto s=storm.Sample(54,-91,t);
            check(std::abs(s.Height)<=storm.AmplitudeBound()+1e-12,"Storm displacement remains bounded over time");
            close(s.GradientX,(storm.Sample(54+eps,-91,t).Height-storm.Sample(54-eps,-91,t).Height)/(2*eps),1e-7,"Storm slope agrees with rendered height field");
            close(s.VerticalVelocity,(storm.Sample(54,-91,t+eps).Height-storm.Sample(54,-91,t-eps).Height)/(2*eps),1e-7,"Storm surface velocity agrees with moving waves");
            close(storm.PressureDepth(54,-91,s.Height+.01,t),0,0,"Storm wave crest cannot pressurize air");
            close(storm.PressureDepth(54,-91,-1000,t),1000,1e-10,"Storm pressure disturbance still decays in deep water");
        }
    }
    Dive::Config temperature; temperature.SurfaceTemperatureC=22; temperature.BottomTemperatureC=4; temperature.ThermoclineMetres=15;
    Dive::Model thermal(temperature);
    close(thermal.Temperature(15),13,1e-12,"Configurable thermocline midpoint");
    check(thermal.Temperature(2)>thermal.Temperature(25),"Configured lake cools with depth");
    const auto& perch=Dive::FishProfiles[0];
    check(!Dive::FishDepthBand(perch,.5).Valid,"Fish cannot spawn in water shallower than their habitat and body clearance");
    const auto& burbot=Dive::FishProfiles[4];
    const auto bottomBand=Dive::FishDepthBand(burbot,30);
    check(bottomBand.Valid&&bottomBand.Shallow>29&&bottomBand.Deep<30,"Burbot stays close to the bottom");
    check(!Dive::FishDepthBand(burbot,90).Valid,"Bottom fish cannot float above bottom outside their depth envelope");
    check(!Dive::FishDepthBand(perch,std::numeric_limits<double>::quiet_NaN()).Valid,"Missing habitat data is rejected");
    for(const auto& fish:Dive::FishProfiles)
        for(double bottom=0;bottom<200;bottom+=.1)
        {
            const auto band=Dive::FishDepthBand(fish,bottom,1.15);
            // AppleClang may fuse multiply/subtract in the reference expression;
            // the habitat function stores the product first. Allow double rounding,
            // while keeping tolerance far below the centimetre terrain resolution.
            const double deepestAllowed=std::fma(-fish.BottomClearance,1.15,bottom);
            if(band.Valid) check(band.Shallow>=fish.MinDepth&&band.Deep<=fish.MaxDepth&&band.Deep<=deepestAllowed+1e-12,
                "Accepted habitats preserve depth limits and scaled clearance");
        }
    const auto& pondweed=Dive::PlantProfiles[0];
    check(Dive::PlantHabitat(pondweed,5,.2,14),"Shallow lit sediment supports vegetation");
    check(!Dive::PlantHabitat(pondweed,5,.9,14),"Plants do not grow on steep cliffs");
    check(!Dive::PlantHabitat(pondweed,8,.2,3),"Low visibility reduces vegetation depth");
    check(!Dive::PlantHabitat(pondweed,1,.2,14),"Plant tops stay submerged");
    Dive::SurfaceGrid land; land.Columns=2; land.Rows=2; land.West=100; land.North=216; land.Spacing=8;
    land.Heights={10,20,30,40}; double landHeight=0;
    check(land.Sample(108,208,landHeight),"Land raster samples LV95 cell centres");
    close(landHeight,25,1e-9,"Land interpolation preserves north-to-south orientation");
    check(!land.Sample(100,216,landHeight),"Land raster does not extrapolate past cell centres");
    land.Heights[3]=-9999;
    check(!land.Sample(108,208,landHeight),"Land NoData rejected");
    Dive::HeightTile west; west.Columns=2; west.Rows=2; west.East=100; west.North=200;
    west.Centimetres={-100,-200,-300,-400};
    Dive::HeightTile east=west; east.East=102; east.Centimetres={-300,-400,-500,-600};
    Dive::HeightMosaic grid; grid.Tiles={west,east}; double height=0;
    check(grid.Sample(101.5,200.5,height),"Bilinear sample across tile boundary");
    close(height,-4,1e-9,"South row is last raster row");
    check(grid.Sample(102,201,height),"Interpolation straddles adjacent tiles");
    close(height,-3.5,1e-9,"Adjacent tile interpolation preserves metre scale");
    grid.Tiles[1].Centimetres[2]=INT16_MIN;
    check(!grid.Sample(102,201,height),"No interpolation across unmeasured cells");
    check(!grid.Sample(99,200,height),"Outside survey returns no data");
    check(!grid.Sample(1e100,200,height),"Extreme coordinates rejected before integer conversion");
    check(!grid.Sample(std::numeric_limits<double>::quiet_NaN(),200,height),"Nonfinite coordinates rejected");
    Dive::Config c; c.SurfacePressureBar=1;
    Dive::Model surface(c), deep(c);
    const double preparedPressure=surface.Data.CylinderBar;
    close(deep.Pressure(10),1.980665,1e-6,"Freshwater hydrostatic pressure");
    run(surface,60,0); run(deep,60,20);
    close(preparedPressure-surface.Data.CylinderBar,16.0/12,1e-7,"Surface RMV to cylinder pressure");
    close((preparedPressure-deep.Data.CylinderBar)/(preparedPressure-surface.Data.CylinderBar),deep.Pressure(20),1e-6,"Depth multiplies gas use");
    Dive::Model active(c); run(active,60,20,1);
    check(active.Data.CylinderBar<deep.Data.CylinderBar,"Exertion consumes more gas");
    check(active.Data.HeartBPM>deep.Data.HeartBPM,"Exertion raises heart rate");
    Dive::Model inflate(c); run(inflate,10,5,0,1);
    Dive::Model normal(c); run(normal,10,5);
    check(inflate.Data.CylinderBar<normal.Data.CylinderBar,"BCD inflation consumes tank gas");
    check(inflate.Data.BuoyancyNewtons>normal.Data.BuoyancyNewtons,"BCD changes buoyancy");
    auto gas=inflate.Data.BCDSurfaceLitres; inflate.Step(.02,0,0,0,0,0,false);
    close(inflate.Data.BCDLitres,std::min(gas,18.0),1e-7,"Boyle expansion and overpressure relief on ascent");
    Dive::Model boyle(c); boyle.Data.BCDSurfaceLitres=6;
    boyle.Step(.02,20,0,0,0,0,false);
    close(boyle.Data.BCDLitres,6/boyle.Pressure(20),1e-7,"Boyle compression at depth");
    Dive::Model tiny(c), big(c); run(tiny,60,20,.4); big.Step(60,20,.4,0,0,0,false);
    close(tiny.Data.CylinderBar,big.Data.CylinderBar,1e-7,"Frame partition invariance: gas");
    close(tiny.Data.HeartBPM,big.Data.HeartBPM,1e-7,"Frame partition invariance: physiology");
    c.GameMode=Dive::Mode::Exploration; Dive::Model explore(c); run(explore,300,30,1,1);
    close(explore.Data.CylinderBar,200,0,"Exploration unlimited gas");
    close(explore.Data.CeilingMetres,0,0,"Exploration disables deco constraint");
    c.GameMode=Dive::Mode::Simulation; c.StartPressureBar=1; Dive::Model empty(c); run(empty,120,20,1,1);
    close(empty.Data.CylinderBar,0,0,"Pressure cannot become negative");
    check(empty.Data.Warnings&Dive::OutOfGas,"Empty cylinder warning");
    check(empty.Data.SpO2<90,"Empty tank affects simulated oxygenation");
    check(empty.Data.BCDSurfaceLitres<18*empty.Data.AmbientBar,"Empty tank cannot inflate forever");
    c.StartPressureBar=200; Dive::Model tissues(c); const double start=tissues.Data.NitrogenBar[0];
    run(tissues,1800,35);
    check(tissues.Data.NitrogenBar[0]>start,"Nitrogen uptake");
    check(tissues.Data.CeilingMetres>0,"Long deep profile generates ceiling");
    close(tissues.Data.NDLMinutes,0,0,"Ceiling means zero NDL");
    const double loaded=tissues.Data.NitrogenBar[0]; run(tissues,300,0);
    check(tissues.Data.NitrogenBar[0]<loaded,"Nitrogen release");
    Dive::Model stop(c); run(stop,1,12); run(stop,180,5);
    close(stop.Data.StopRemainingSeconds,0,.001,"Stationary 3-minute safety stop");
    Dive::Model ascent(c); ascent.Step(2,8,0,0,.3,0,false);
    check(ascent.Data.Warnings&Dive::FastAscent,"Ascent warning");
    c.CylinderLitres=0; c.VisibilityMetres=std::numeric_limits<double>::quiet_NaN();
    Dive::Model invalid(c); run(invalid,1,10);
    check(std::isfinite(invalid.Data.CylinderBar),"Invalid configuration sanitized");
    auto elapsed=invalid.Data.ElapsedSeconds; invalid.Step(-1,10,0,0,0,0,false);
    close(invalid.Data.ElapsedSeconds,elapsed,0,"Negative time rejected");
    std::cout<<(failures?"FAILED":"PASS")<<": deterministic dive model checks ("<<failures<<" failures)\n";
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
