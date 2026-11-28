#include "../divingsim/Source/DiveSim/Simulation/RockIndex.h"
#include "../divingsim/Source/DiveSim/Simulation/FishSteering.h"
#include <iostream>
#include <random>
#include <limits>
using namespace Dive;
int main()
{
    int failures=0;auto check=[&](bool value,const char* message){if(!value){++failures;std::cerr<<message<<'\n';}};
    RockIndex index;
    check(index.Query(0,0,0,0,0).empty(),"Empty index");
    struct Rock {FishSteering::Vec Position;double Radius;};
    std::vector<Rock> rocks;
    auto add=[&](double x,double y,double z,double r){rocks.push_back({{x,y,z},r});index.Add(x,y,r);};
    // Exact cell seams, both signs, tangent spheres, oversized footprints.
    add(-800,0,0,100);add(800,0,0,100);add(0,-800,0,100);add(0,800,0,100);add(0,0,20000,100);add(-1600,-1600,0,1601);
    check(index.Query(-900,0,-900,0,0).front()==0,"Negative seam tangent retained");
    check(index.Query(900,0,900,0,0).size()>=2,"Positive seam and large radius retained");
    std::mt19937 rng(1749);std::uniform_real_distribution<double> xy(-50000,50000),z(-5000,1000),radius(10,337),offset(-1500,1500);
    for(int i=0;i<4096;++i) add(xy(rng),xy(rng),z(rng),radius(rng));
    size_t visited=0;
    for(int q=0;q<1600;++q)
    {
        const auto& target=rocks[size_t(q)%rocks.size()];
        auto a=target.Position,b=a;a[0]+=offset(rng);a[1]+=offset(rng);b[0]+=offset(rng);b[1]+=offset(rng);
        const double padding=q%201;const auto candidates=index.Query(a[0],a[1],b[0],b[1],padding);visited+=candidates.size();
        check(std::is_sorted(candidates.begin(),candidates.end())&&std::adjacent_find(candidates.begin(),candidates.end())==candidates.end(),"Unique original order");
        double full=1,fast=1;
        for(size_t i=0;i<rocks.size();++i)
        {
            const auto& r=rocks[i];const double hit=FishSteering::SphereFraction(a,b,r.Position,r.Radius+padding);full=std::min(full,hit);
            if(hit<1) check(std::binary_search(candidates.begin(),candidates.end(),int32_t(i)),"Sweep candidate omitted");
        }
        for(auto i:candidates) fast=std::min(fast,FishSteering::SphereFraction(a,b,rocks[i].Position,rocks[i].Radius+padding));
        check(full==fast,"Exact swept collision fraction agrees with brute force");
    }
    check(visited<rocks.size()*1600/20,"Local queries visit less than five percent of the test world");
    check(index.Query(-1e12,-1e12,1e12,1e12,0).size()==rocks.size(),"Huge query falls back without truncation");
    check(index.Query(std::numeric_limits<double>::quiet_NaN(),0,0,0,0).size()==rocks.size(),"Invalid query conservatively returns all");
    index.Reset();index.Add(10000000,-10000000,20);
    check(index.Size()==1&&index.Query(10000000,-10000000,10000000,-10000000,0)==std::vector<int32_t>{0},"Reset removes stale cells and restores indices");
    check(index.Query(0,0,0,0,0).empty(),"Retired window absent");
    std::cout<<(failures?"FAIL":"PASS")<<": rock index, 1600 brute-force sweep comparisons, "<<visited<<" candidates ("<<failures<<" failures)\n";
    return failures?1:0;
}
