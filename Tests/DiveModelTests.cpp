#include "../divingsim/Source/DiveSim/Simulation/DiveModel.h"
#include "../divingsim/Source/DiveSim/Simulation/HeightGrid.h"
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
