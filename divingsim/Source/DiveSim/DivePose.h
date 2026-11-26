#pragma once
#include "DiveBodyFrame.h"

// Advanced by the fixed simulation step. Rendering and contact queries consume
// the same state; a rejected joint motion can be restored without rewinding gas.
struct FDivePoseState
{
    FRotator Gaze=FRotator::ZeroRotator;
    float Swim=0,BodyPitch=0,HeadYaw=0,Motion=0,Phase=0,Wrist=0,KneeTuck=0;
    float FloorPitch[2]={-90,-90};
    bool Walking=false;

    float BodyYaw() const { return FRotator::NormalizeAxis(Gaze.Yaw-HeadYaw); }
    FRotator BodyHeadingView() const { return FDiveBodyFrame::View(Gaze.Pitch,BodyYaw()); }

    // An authored 65 degree sideways look is available when shoulders/gear
    // cannot turn. The caller still collision-tests the entire articulated rig.
    void LookWithoutBody(const FRotator& View)
    {
        const float yaw=BodyYaw();
        HeadYaw=FMath::Clamp(static_cast<float>(FMath::FindDeltaAngleDegrees(yaw,View.Yaw)),-65.f,65.f);
        Gaze=FDiveBodyFrame::View(FMath::Clamp(static_cast<float>(FRotator::NormalizeAxis(View.Pitch)),
            FMath::Max(-85.f,BodyPitch-60.f),FMath::Min(85.f,BodyPitch+90.f)),yaw+HeadYaw);
    }

    void Advance(float Dt,const FRotator& View,bool IsWalking,bool Swimming,bool Moving,float Speed,float Effort,bool ReadComputer)
    {
        Gaze=FDiveBodyFrame::View(View.Pitch,View.Yaw);HeadYaw=0;Walking=IsWalking;
        Swim=FMath::FInterpTo(Swim,Swimming?1.f:0.f,Dt,3);
        Motion=FMath::FInterpTo(Motion,Moving?1.f:0.f,Dt,5);
        // A contact may temporarily prevent the motion blend from relaxing.
        // Do not start fresh kicks from that retained pose after input stops.
        if(Moving) Phase=FMath::Fmod(Phase+Dt*(Walking?Speed*7:4+4*Effort),2*PI);
        Wrist=FMath::FInterpTo(Wrist,ReadComputer?1.f:0.f,Dt,8);
        KneeTuck=FMath::FInterpTo(KneeTuck,0.f,Dt,2);
        BodyPitch=FMath::FInterpTo(BodyPitch,FMath::Max(-80*Swim,static_cast<float>(Gaze.Pitch)-90),Dt,5);
        BodyPitch=FDiveBodyFrame::FromEyes(FVector::ZeroVector,BodyPitch,Gaze.Yaw,Gaze.Pitch).BodyPitch;
    }

    FTransform Hand(bool Left) const
    {
        if(!Left) return FTransform(FRotator(8,-5,0),FVector(29,24,-20));
        const float reading=FMath::Clamp(Swim/.45f,0.f,1.f);
        return FTransform(FMath::Lerp(FRotator(18,-8,0),FRotator(FMath::Lerp(75.f,60.f,reading),-8,0),Wrist),
            FMath::Lerp(FVector(35,-24,-18),FVector(FMath::Lerp(28.f,26.f,reading),-6,-6),Wrist));
    }
};

struct FDiveArmPose
{
    FVector Shoulder,Elbow,Wrist;
    static FDiveArmPose Solve(const FVector& Shoulder,const FVector& RestElbow,const FVector& RestWrist,
        const FTransform& Body,const FTransform& Hand)
    {
        const double a=FVector::Distance(Shoulder,RestElbow),b=FVector::Distance(RestElbow,RestWrist);
        FVector axis=Body.InverseTransformPosition(Hand.GetLocation())-Shoulder;
        const double distance=FMath::Clamp(axis.Size(),FMath::Abs(a-b)+.01,a+b-.01);axis.Normalize();
        const FVector target=Shoulder+axis*distance;
        const FVector forearm=Body.InverseTransformVectorNoScale(Hand.GetUnitAxis(EAxis::X));
        FVector pole=target-forearm*b-Shoulder;pole-=axis*FVector::DotProduct(pole,axis);
        if(!pole.Normalize()) pole=FVector(0,RestElbow.Y<0?-1:1,-.3).GetSafeNormal();
        const double along=(a*a-b*b+distance*distance)/(2*distance);
        return {Shoulder,Shoulder+axis*along+pole*FMath::Sqrt(FMath::Max(0.,a*a-along*along)),target};
    }
};

struct FDiveLegPose
{
    FVector Hip,Knee,Ankle;
    FRotator Foot;
    static FDiveLegPose Solve(const FVector& Hip,const FVector& RestKnee,const FVector& RestAnkle,
        float Sign,float Swim,float Motion,float Phase,float Tuck=0)
    {
        const double upper=FVector::Distance(Hip,RestKnee),lower=FVector::Distance(RestKnee,RestAnkle);
        const double wave=FMath::Sin(Phase),walk=1-Swim;
        const double transition=FMath::Sin(PI*Swim);
        // Retract the heels while changing from standing to prone. Extending
        // the ankle first would drive a 64 cm blade through the ground.
        const double angle=FMath::DegreesToRadians((walk*17+Swim*12)*wave*Motion-15*transition+40*Tuck);
        const double bend=FMath::DegreesToRadians(FMath::Min(145.,(walk*32+Swim*22)*FMath::Max(0.,-wave)*Motion+Swim*10+60*transition+100*Tuck));
        const FVector knee=Hip+FVector(FMath::Sin(angle)*upper,0,-FMath::Cos(angle)*upper);
        const FVector ankle=knee+FVector(FMath::Sin(angle-bend)*lower,0,-FMath::Cos(angle-bend)*lower);
        return {Hip,knee,ankle,FRotator(FMath::RadiansToDegrees(-angle+bend*.6)-Swim*80,Sign*7,0)};
    }
};
