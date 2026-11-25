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
        freeAir.Step(60,0,0,0,0,0,false,air);
        close(freeAir.Data.CylinderBar,200,0,"Ambient-air breathing does not consume cylinder gas");
        check(freeAir.Data.BreathSource==Dive::BreathingSource::Air&&freeAir.Data.SpO2>97,"Ambient air supplies breathing");
        regulatedAir.Step(60,0,0,0,0,0,false,{0,false,true});
        check(regulatedAir.Data.CylinderBar<199,"Regulator still consumes cylinder gas above water");
        water.Step(60,0,0,0,0,0,false,wet);
        check(water.Data.CoreTemperatureC<regulatedAir.Data.CoreTemperatureC,"Immersion cools more than dry air in the same suit");
        close(freeAir.Data.EnvironmentTemperatureC,20,1e-12,"Dry body uses air temperature");
        close(water.Data.EnvironmentTemperatureC,6,1e-12,"Immersed body uses water temperature");
        held.Step(5,10,0,0,0,0,false,noSupply);
        close(held.Data.CylinderBar,200,0,"Removed regulator does not invent cylinder consumption");
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
    const auto noon=Dive::EnvironmentLight::At(12,0),night=Dive::EnvironmentLight::At(23,0);
    check(noon.Direct>0&&noon.ElevationDegrees>0,"Daytime sun is above the horizon");
    check(night.Direct==0&&night.ElevationDegrees<0,"Night has no fictitious above-horizon sun");
    close(night.WaterAmbient(0,14),0,0,"Unlit night water has no ambient emission");
    close(Dive::EnvironmentLight::At(-1,0).Direct,night.Direct,0,"Solar clock wraps negative hours");
    close(Dive::EnvironmentLight::At(36,0).Direct,noon.Direct,0,"Solar clock repeats daily");
    check(noon.WaterAmbient(20,14)<noon.WaterAmbient(2,14),"Ambient daylight decays with water depth");
    check(noon.WaterAmbient(20,4)<noon.WaterAmbient(20,30),"Turbid water attenuates ambient light faster");
    check(noon.WaterAmbient(100,14)<.003,"Deep water cannot retain surface brightness");
    check(Dive::EnvironmentLight::At(12,2).WaterAmbient(5,14)<noon.WaterAmbient(5,14),"Rain dims underwater ambient light");
    close(noon.WaterAmbient(-5,14),noon.WaterAmbient(0,14),0,"Air position cannot amplify underwater source");
    check(std::isfinite(Dive::EnvironmentLight::At(NAN,0).WaterAmbient(NAN,NAN)),"Lighting handles invalid inputs");
    check(night.ExposureBias(0,14)>noon.ExposureBias(0,14),"Eyes gain sensitivity at night");
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
    close(deep.Pressure(10),1.980665,1e-6,"Freshwater hydrostatic pressure");
    run(surface,60,0); run(deep,60,20);
    close(200-surface.Data.CylinderBar,16.0/12,1e-7,"Surface RMV to cylinder pressure");
    close((200-deep.Data.CylinderBar)/(200-surface.Data.CylinderBar),deep.Pressure(20),1e-6,"Depth multiplies gas use");
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
