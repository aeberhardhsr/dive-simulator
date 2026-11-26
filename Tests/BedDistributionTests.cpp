#include "../divingsim/Source/DiveSim/Simulation/BedDistribution.h"
#include <iostream>
#include <limits>

int main()
{
    using B=Dive::BedDistribution;
    int failures=0;auto check=[&](bool value,const char* message){if(!value){++failures;std::cerr<<message<<'\n';}};
    int sparse=0,dense=0;
    for(int y=-100;y<=100;y+=2) for(int x=-100;x<=100;x+=2)
    {
        const double weight=B::Gravel(x+.17,y+.43,5);
        check(weight>=0&&weight<=1,"Sediment weights must be bounded");
        check(B::Gravel(x+.17,y+.43,30)<=weight,"Gravel contribution fades with depth");
        check(B::Gravel(x+.17,y+.43,60)==0,"Deep sediment has no gravel dressing");
        sparse+=weight<.1;dense+=weight>.8;
    }
    check(sparse>100&&dense>100,"Ground contains distinct sparse and dense areas");
    for(int grid=-20;grid<=20;++grid)
    {
        check(std::abs(B::Noise(grid-1e-7,.71)-B::Noise(grid+1e-7,.71))<1e-6,"No seams across noise cells");
        check(std::abs(B::Gravel(grid*32-1e-6,1,4)-B::Gravel(grid*32+1e-6,1,4))<1e-5,"No sediment seam at streaming boundaries");
    }
    check(B::Gravel(std::numeric_limits<double>::quiet_NaN(),0,1)==0,"Invalid coordinates produce no dressing");
    check(B::Gravel(0,0,std::numeric_limits<double>::infinity())==0,"Invalid depth produces no dressing");
    check(std::isfinite(B::Noise(1e100,0)),"Unbounded coordinates cannot overflow hashing conversion");
    std::cout<<"PASS: bed distribution ("<<failures<<" failures)\n";
    return failures?1:0;
}
