#include "../divingsim/Source/DiveSim/Simulation/CoastGrid.h"
#include <cstdlib>
#include <iostream>

void check(bool condition,const char* message)
{ if(!condition) { std::cerr<<message<<'\n';std::exit(1); } }

Dive::SurfaceGrid tile(int x,int y)
{
    Dive::SurfaceGrid result;result.West=3+x*128;result.North=133+y*128;
    result.Spacing=1;result.Columns=result.Rows=130;
    for(int row=0;row<130;++row) for(int col=0;col<130;++col)
        result.Heights.push_back(static_cast<float>(-100+(result.West+col+.5)*.125+(result.North-row-.5)*.25));
    return result;
}

int main()
{
    Dive::CoastGrid coast;coast.Tiles.emplace(std::make_pair(0,0),tile(0,0));
    double height=0;
    check(coast.Sample(4,4,height)&&std::abs(height+98.5)<1e-6,"Closed southwest vertex lost");
    check(coast.Sample(132,132,height)&&std::abs(height+50.5)<1e-6,"Closed northeast vertex lost");
    check(!coast.Sample(133,64,height),"Halo must not extend physical domain");
    check(!coast.Sample(NAN,64,height),"NaN accepted");
    check(coast.BoundaryDistance(4,64)==0&&coast.BoundaryDistance(132,64)==0,"Exterior edge not zero");
    check(coast.BoundaryDistance(64,64)==32,"Interior must retain fine source");
    coast.Tiles.emplace(std::make_pair(1,0),tile(1,0));
    check(coast.BoundaryDistance(132,64)==32,"Internal tile seam introduced transition");
    check(coast.Sample(132,64,height)&&std::abs(height-(-100+132*.125+64*.25))<1e-6,"Shared edge changed plane");
    check(coast.Contains(132,64)&&!coast.Contains(260,64),"Half-open ownership incorrect");
    check(coast.BoundaryDistance(128,128)==4,"Northern exterior distance incorrect");
    check(Dive::CoastGrid::Weight(-1)==0&&Dive::CoastGrid::Weight(16)==.5&&Dive::CoastGrid::Weight(40)==1,"Transition weights wrong");
    // Add the northern-left tile: only the diagonal northeast core is absent.
    coast.Tiles.emplace(std::make_pair(0,1),tile(0,1));
    check(std::abs(coast.BoundaryDistance(128,128)-std::sqrt(32.))<1e-6,"Missing diagonal tile must form concave collar");
    std::cout<<"PASS coast lattice, shared edge, closed vertices, domain and collar\n";
}
