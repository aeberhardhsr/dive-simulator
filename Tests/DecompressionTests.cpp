#include "../divingsim/Source/DiveSim/Simulation/DiveModel.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace
{
int failures=0;
void check(bool value,const char* reason)
{ if(!value) { ++failures;std::cerr<<"FAIL "<<reason<<'\n'; } }
// Independent numerical oracle: RK4 at <= 0.25 s. Neither the production
// forecast, its closed-form ramp nor its wait solver is used for integration.
void integrate(Dive::Deco::Tissues& tissues,double from,double to,double seconds,double surface,double n2,bool verify=false)
{
    const int steps=std::max(1,static_cast<int>(std::ceil(seconds/.25)));
    const double dt=seconds/steps;
    for(int j=0;j<steps;++j)
    {
        const auto inspired=[&](double t) { return (surface+(from+(to-from)*t/seconds)*Dive::BarPerMetre-.0627)*n2; };
        for(size_t i=0;i<16;++i)
        {
            const double rate=std::log(2.)/(Dive::HalfTimes[i]*60.);
            const double p=tissues[i],time=j*dt;
            const double a=rate*(inspired(time)-p);
            const double b=rate*(inspired(time+dt/2)-(p+a*dt/2));
            const double c=rate*(inspired(time+dt/2)-(p+b*dt/2));
            const double d=rate*(inspired(time+dt)-(p+c*dt));
            tissues[i]+=dt*(a+2*b+2*c+d)/6;
            if(verify)
            {
                const double depth=from+(to-from)*(j+1)/steps;
                const double ambient=surface+depth*Dive::BarPerMetre;
                const double allowed=ambient+.8*(Dive::A[i]+ambient/Dive::B[i]-ambient);
                check(tissues[i]<=allowed+1e-8,"Forecast crosses compartment limit during ascent/hold");
            }
        }
    }
}
bool aboveLimit(const Dive::Deco::Tissues& tissues,double depth,double surface)
{
    const double ambient=surface+depth*Dive::BarPerMetre;
    for(size_t i=0;i<16;++i)
        if(tissues[i]>ambient+.8*(Dive::A[i]+ambient/Dive::B[i]-ambient)+1e-8) return true;
    return false;
}
void profile(double depth,double minutes,double n2,double surface)
{
    Dive::Deco::Tissues initial;initial.fill((surface-.0627)*.79);
    integrate(initial,depth,depth,minutes*60,surface,n2);
    const auto original=initial;
    const auto plan=Dive::Deco::Forecast(initial,depth,surface,n2,Dive::BarPerMetre);
    check(plan.Valid,"Profile has a finite forecast");check(initial==original,"Forecast does not mutate live tissues");
    if(!plan.Valid) return;
    double total=0;int stop=0;
    while(depth>1e-8)
    {
        const double next=std::max(0.,(std::ceil(depth/3.-1e-9)-1)*3.);
        const double travel=(depth-next)/.15;
        if(stop<plan.Count&&std::abs(plan.Stops[stop].Depth-depth)<1e-7)
        {
            const double wait=plan.Stops[stop].Seconds;
            auto shorter=initial;
            if(wait>1) integrate(shorter,depth,depth,wait-1,surface,n2);
            integrate(shorter,depth,next,travel,surface,n2);
            check(aboveLimit(shorter,next,surface),"Stop is minimal to whole-second resolution");
            integrate(initial,depth,depth,wait,surface,n2,true);
            total+=wait;++stop;
        }
        integrate(initial,depth,next,travel,surface,n2,true);
        total+=travel;depth=next;
    }
    check(stop==plan.Count,"Every projected stop was visited");
    check(std::abs(total-plan.TotalSeconds)<1e-6,"TTS includes travel and all stops");
    check(!aboveLimit(initial,0,surface),"Forecast reaches surface within model limits");
    std::cout<<"PROFILE_PASS stops="<<plan.Count<<" tts_seconds="<<plan.TotalSeconds<<" n2="<<n2<<" surface="<<surface<<'\n';
}
}
int main()
{
    for(double surface:{.965,1.01325})
        for(double n2:{.79,.68})
            for(const auto& p:std::array<std::array<double,2>,5>{{{12,8},{30,20},{35,40},{50,30},{35.7,70}}})
                profile(p[0],p[1],n2,surface);
    Dive::Deco::Tissues t;t.fill((.965-.0627)*.79);
    check(Dive::Deco::Forecast(t,0,.965,.79,Dive::BarPerMetre).Valid,"Surface forecast");
    auto direct=Dive::Deco::Forecast(t,12,.965,.79,Dive::BarPerMetre);
    check(direct.Valid&&direct.Count==0&&std::abs(direct.TotalSeconds-80)<1e-6,"Unloaded ascent is travel only");
    check(!Dive::Deco::Forecast(t,std::numeric_limits<double>::quiet_NaN(),.965,.79,Dive::BarPerMetre).Valid,"NaN rejected");
    t.fill(5);
    check(!Dive::Deco::Forecast(t,0,.965,.79,Dive::BarPerMetre).Valid,"Ceiling violation cannot produce a valid ascent");
    t.fill(13);
    check(!Dive::Deco::Forecast(t,150,.965,.79,Dive::BarPerMetre).Valid,"Forecast exceeding six hours is explicitly unavailable");
    t[0]=std::numeric_limits<double>::infinity();
    check(!Dive::Deco::Forecast(t,150,.965,.79,Dive::BarPerMetre).Valid,"Nonfinite tissue pressure rejected");
    Dive::Config cfg;cfg.StartPressureBar=300;cfg.PlannedDepthMetres=50;
    Dive::Model model(cfg);model.Step(1200,35,0,0,0,0,false);
    check(model.Data.DecoForecastValid&&model.Data.DecoStopMetres>0&&model.Data.DecoStopSeconds>0,"Live model publishes stop forecast");
    const auto at= model.Data.ElapsedSeconds;
    const auto first=model.Data.DecoStopSeconds;
    model.Step(.2,35,0,0,0,0,false);
    check(model.Data.DecoStopSeconds==first,"Forecast refresh is bounded to one Hz");
    check(model.Data.ElapsedSeconds>at,"Simulation continues between forecasts");
    cfg.GameMode=Dive::Mode::Exploration;model.Reset(cfg);model.Step(1,35,0,0,0,0,false);
    check(!model.Data.DecoForecastValid&&model.Data.DecoStopMetres==0,"Exploration has no decompression forecast");
    cfg=Dive::Config{};cfg.SurfacePressureBar=1;cfg.StartPressureBar=170+8./12.;
    model.Reset(cfg);model.Step(60,0,0,0,0,0,false);
    check(std::abs(model.Data.RemainingGasMinutes-89)<1e-6,"Gas estimate is time to the 50 bar reserve, not empty");
    model.Reset(cfg);model.Step(.1,20,0,0,0,0,false);
    check(std::abs(model.Data.PPO2-.6218793)<1e-7,"ppO2 follows air fraction and freshwater ambient pressure");
    Dive::Model calm(cfg),stressed(cfg);stressed.Data.Stress=1;
    calm.Step(2,20,0,0,0,0,false);stressed.Step(2,20,0,0,0,0,false);
    check(stressed.Data.CylinderBar<calm.Data.CylinderBar&&stressed.Data.SurfaceRMV>calm.Data.SurfaceRMV,"Stress increases consumption at identical depth and activity");
    model.Reset(cfg);model.Data.CylinderBar=50.1;model.Step(.02,0,0,0,0,0,false);
    check(!(model.Data.Warnings&Dive::LowGas),"No low-gas warning above reserve");
    model.Data.CylinderBar=49.9;model.Step(.02,0,0,0,0,0,false);
    check((model.Data.Warnings&Dive::LowGas)&&model.Data.RemainingGasMinutes==0,"Below-reserve warning and estimate agree");
    cfg.StartPressureBar=300;model.Reset(cfg);model.Step(.02,30,0,0,0,0,false);
    const double ndl=model.Data.NDLMinutes*60;
    check(ndl>60&&ndl<5940,"NDL is finite for this profile");
    model.Step(ndl-1,30,0,0,0,0,false);check(model.Data.CeilingMetres==0,"Ceiling is clear just before predicted NDL");
    model.Step(2,30,0,0,0,0,false);check(model.Data.CeilingMetres>0,"Exposure crosses the ceiling after predicted NDL");
    std::cout<<(failures?"FAILED":"PASS")<<": independent numerical decompression checks ("<<failures<<" failures)\n";
    return failures?EXIT_FAILURE:EXIT_SUCCESS;
}
