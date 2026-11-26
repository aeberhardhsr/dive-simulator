#pragma once
#include <algorithm>
#include <array>
#include <cmath>

namespace Dive::FishSteering
{
using Vec=std::array<double,3>;
constexpr double Pi=3.14159265358979323846;
inline Vec Add(Vec a,Vec b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
inline Vec Sub(Vec a,Vec b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
inline Vec Scale(Vec a,double s){return {a[0]*s,a[1]*s,a[2]*s};}
inline double Dot(Vec a,Vec b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline double Length(Vec a){return std::sqrt(Dot(a,a));}
inline Vec Unit(Vec a,Vec fallback={1,0,0}){const double l=Length(a);return l>1e-12?Scale(a,1/l):fallback;}
inline double Wrap(double a){return std::remainder(a,2*Pi);}
inline Vec Direction(double yaw,double pitch){return {std::cos(pitch)*std::cos(yaw),std::cos(pitch)*std::sin(yaw),std::sin(pitch)};}
struct State
{
    double Yaw=0,Pitch=0,Speed=0;
    Vec Facing() const{return Direction(Yaw,Pitch);}
};
struct Result { Vec Position,Velocity; bool Braked=false; double Turn=0; };

// First contact along a segment with a conservative obstacle sphere. Returning
// the fraction (rather than checking its endpoint) prevents tunneling.
inline double SphereFraction(Vec start,Vec end,Vec centre,double radius)
{
    const auto offset=Sub(start,centre),delta=Sub(end,start);
    const double c=Dot(offset,offset)-radius*radius,a=Dot(delta,delta);
    if(c<0) return 0;
    if(a<1e-18) return 1;
    const double b=Dot(offset,delta),disc=b*b-a*c;
    if(b>=0||disc<0) return 1;
    const double t=(-b-std::sqrt(disc))/a;
    return t>=0&&t<=1?t:1;
}

// SI units. Trace(start,end) returns a safe fraction in [0,1]. These bounded
// responses are authored game behavior, not measured fish biomechanics.
template<class Trace>
Result Advance(State& state,Vec position,Vec desired,Vec flow,double requestedSpeed,
               double dt,double turnRate,double acceleration,Trace trace)
{
    const auto oldFacing=state.Facing();
    if(!std::isfinite(dt)||dt<=0) return {position,Scale(oldFacing,state.Speed),false,0};
    desired=Unit(desired,oldFacing);
    const double pitch=std::clamp(std::asin(std::clamp(desired[2],-1.,1.)),-.55,.55);
    const double yaw=std::atan2(desired[1],desired[0]);
    Vec chosen=Direction(yaw,pitch);
    const double speed=std::max(.1,std::max(requestedSpeed,state.Speed));
    const double horizon=std::max(1.2,.8/speed)+state.Speed/std::max(.1,acceleration)*.5;
    auto probe=[&](Vec d){return std::clamp(trace(position,Add(position,Scale(Add(Scale(d,speed),flow),horizon))),0.,1.);};
    if(probe(chosen)<.999||probe(oldFacing)<.999)
    {
        double best=-1e30;
        for(double dy:{0.,Pi/6,-Pi/6,Pi/3,-Pi/3,Pi/2,-Pi/2,Pi*.75,-Pi*.75,Pi})
            for(double dp:{0.,.35,-.35})
            {
                const auto candidate=Direction(yaw+dy,std::clamp(pitch+dp,-.55,.55));
                const double score=probe(candidate)*4+Dot(candidate,desired)*.8+Dot(candidate,oldFacing)*.2;
                if(score>best){best=score;chosen=candidate;}
            }
    }
    double dy=Wrap(std::atan2(chosen[1],chosen[0])-state.Yaw);
    double dp=std::asin(std::clamp(chosen[2],-1.,1.))-state.Pitch;
    const double amount=std::hypot(dy,dp),limit=std::max(0.,turnRate)*dt;
    if(amount>limit&&amount>0){dy*=limit/amount;dp*=limit/amount;}
    state.Yaw=Wrap(state.Yaw+dy);state.Pitch=std::clamp(state.Pitch+dp,-.55,.55);
    const auto facing=state.Facing();
    const double clearance=probe(facing)*Length(Add(Scale(facing,speed),flow))*horizon;
    const double safeSpeed=std::sqrt(std::max(0.,2*acceleration*std::max(0.,clearance-.12)));
    const double target=std::min(requestedSpeed*std::clamp(Dot(facing,chosen),.1,1.),safeSpeed);
    state.Speed=std::max(0.,state.Speed+std::clamp(target-state.Speed,-acceleration*dt,acceleration*dt));
    auto velocity=Scale(facing,state.Speed);
    const auto delta=Scale(Add(velocity,flow),dt),next=Add(position,delta);
    const double fraction=std::clamp(trace(position,next),0.,1.);
    const bool braked=fraction<1;
    if(braked){state.Speed=0;velocity={0,0,0};}
    return {Add(position,Scale(delta,braked?std::max(0.,fraction-.001):1)),velocity,braked,
        std::acos(std::clamp(Dot(oldFacing,facing),-1.,1.))};
}
}
