#include "../divingsim/Source/DiveSim/Simulation/DiveModel.h"
#include "../divingsim/Source/DiveSim/Simulation/HeightGrid.h"
#include "../divingsim/Source/DiveSim/Simulation/Ecology.h"
#include "../divingsim/Source/DiveSim/Simulation/WaterSurface.h"
#include "../divingsim/Source/DiveSim/Simulation/AudioModel.h"
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
