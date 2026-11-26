#include "../divingsim/Source/DiveSim/Simulation/FishMotion.h"
#include <iostream>
#include <limits>
#include <cstdlib>
int main()
{
    int failures=0;
    auto check=[&](bool ok,const char* name){if(!ok){++failures;std::cerr<<name<<'\n';}};
    Dive::FishMotion fish;
    fish.Step(0,2,.3,2,1);
    check(fish.Tail==0&&fish.Fin==0&&fish.Effort==1&&fish.Turn==0,"Paused animation is immutable");
    fish.Step(.1,.3,.3,2,0);
    check(std::abs(fish.Tail-1.2566370614)<1e-8,"Cruise tail cadence");
    check(fish.Fin>0&&fish.Fin<fish.Tail,"Independent fin cadence");
    auto old=fish;
    fish.Step(.02,std::numeric_limits<double>::quiet_NaN(),.3,2,0);
    check(fish.Tail==old.Tail&&fish.Fin==old.Fin,"Invalid motion cannot contaminate phase");
    Dive::FishMotion fast,slow,left,right;
    for(int i=0;i<600;++i){fast.Step(1./60,1.5,.3,2,0);slow.Step(1./60,.03,.3,2,0);left.Step(1./60,.3,.3,2,-2);right.Step(1./60,.3,.3,2,2);}
    check(fast.Effort<=2.5&&fast.Effort>2.49&&slow.Effort<.11,"Speed drives bounded effort");
    check(fast.TailAmplitude()>slow.TailAmplitude(),"Escape increases body stroke amplitude");
    check(left.Turn<-.99&&right.Turn>.99,"Turns preserve direction for asymmetric fins");
    auto run=[](int fps){Dive::FishMotion m;for(int i=0;i<fps*10;++i)m.Step(1./fps,.6,.3,2,.5);return m;};
    const auto a=run(30),b=run(120);
    check(std::abs(a.Tail-b.Tail)<.02&&std::abs(a.Fin-b.Fin)<.02&&std::abs(a.Turn-b.Turn)<1e-8,"Frame-rate consistency");
    for(int i=0;i<720000;++i)fish.Step(.01,.3,.3,2,0);
    check(std::isfinite(fish.Tail)&&fish.Tail>=0&&fish.Tail<6.283186&&fish.Fin>=0&&fish.Fin<6.283186,"Two-hour phases remain bounded");
    std::cout<<(failures?"FAIL":"PASS")<<": fish animation response ("<<failures<<" failures)\n";
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
