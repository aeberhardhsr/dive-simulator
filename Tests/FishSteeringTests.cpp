#include "../divingsim/Source/DiveSim/Simulation/FishSteering.h"
#include <iostream>
#include <cstdlib>
using namespace Dive::FishSteering;
int main()
{
    int failures=0;auto check=[&](bool value,const char* message){if(!value){++failures;std::cerr<<message<<'\n';}};
    auto free=[](Vec,Vec){return 1.;};
    check(std::abs(SphereFraction({-2,0,0},{2,0,0},{0,0,0},1)-.25)<1e-12,"Swept sphere detects crossing despite safe endpoint");
    check(SphereFraction({2,0,0},{3,0,0},{0,0,0},1)==1,"Moving away stays clear");
    State stop{1,.2,0};const auto before=stop;auto paused=Advance(stop,{0,0,0},{-1,0,0},{0,0,0},1,0,1,1,free);
    check(stop.Yaw==before.Yaw&&stop.Pitch==before.Pitch&&paused.Position==Vec{0,0,0},"Pause preserves heading and position");
    State accelerate;Advance(accelerate,{0,0,0},{1,0,0},{0,0,0},2,.1,1,.8,free);
    check(std::abs(accelerate.Speed-.08)<1e-12,"Normal acceleration is bounded");
    State seam{Pi-.01,0,.4};auto seamStep=Advance(seam,{0,0,0},Direction(-Pi+.01,0),{0,0,0},.4,.1,1,1,free);
    check(seamStep.Turn<.021&&Dot(seam.Facing(),Direction(-Pi+.01,0))>.999999,"Heading wraps across pi by the short route");
    State reversal{0,0,.4};auto p=Vec{0,0,0};
    for(int i=0;i<180;++i){auto r=Advance(reversal,p,{-1,0,0},{0,0,0},.4,1./60,1.5,.8,free);check(r.Turn<=1.5/60+1e-7,"Reversal has bounded turn rate");p=r.Position;}
    check(Dot(reversal.Facing(),{-1,0,0})>.99,"Fish completes a gradual reversal");
    for(int fps:{30,60,120})
    {
        State s{0,0,.6};Vec pos{-3,0,0};int brakes=0;
        auto rock=[](Vec a,Vec b){return SphereFraction(a,b,{0,0,0},.85);};
        for(int i=0;i<fps*18;++i)
        {
            auto r=Advance(s,pos,Sub({4,0,0},pos),{0,0,0},.6,1./fps,1.6,1,rock);
            check(Length(r.Position)>=.85-1e-9,"Rock never penetrated");check(r.Turn<=1.6/fps+1e-7,"Obstacle turn bounded");
            pos=r.Position;brakes+=r.Braked;
        }
        check(pos[0]>2,"Fish progresses around the obstacle");
        std::cout<<"ROCK_ROUTE fps="<<fps<<" x="<<pos[0]<<" brakes="<<brakes<<'\n';
    }
    State blocked{.7,.1,.4};auto r=Advance(blocked,{0,0,0},{1,0,0},{0,2,0},.4,.1,1,1,[](Vec,Vec){return 0.;});
    check(r.Braked&&blocked.Speed==0&&r.Position==Vec{0,0,0},"Blocked drift stops without penetration");
    check(r.Turn<=.1+1e-7&&std::abs(blocked.Yaw-.7)<.11,"Emergency stop preserves gradual orientation");
    State bank{0,0,.7};Vec bankPosition{-2,0,1};
    auto slope=[](Vec a,Vec b){const double before=a[2]-.5*a[0]-.3,after=b[2]-.5*b[0]-.3;return after>=0?1.:std::clamp(before/(before-after),0.,1.);};
    for(int i=0;i<400;++i)
    {
        auto step=Advance(bank,bankPosition,{1,0,0},{0,.1,0},.7,.05,1.7,1,slope);
        check(step.Position[2]-.5*step.Position[0]>=.3-1e-9,"Sloped bank remains outside the habitat envelope");
        check(step.Turn<=.085001,"Bank avoidance does not flip orientation");bankPosition=step.Position;
    }
    State drift{0,0,.4};Vec driftPosition{};
    for(int i=0;i<100;++i)driftPosition=Advance(drift,driftPosition,{1,0,0},{0,.2,0},.4,.02,1.5,1,free).Position;
    check(std::abs(driftPosition[1]-.4)<1e-9,"Current advects fish independently of swim heading");
    std::cout<<(failures?"FAIL":"PASS")<<": swept fish steering ("<<failures<<" failures)\n";
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
