#include "DivePawn.h"
#include "DiveWorld.h"
#include "GameFramework/PlayerController.h"
#include "Simulation/ShoreMovement.h"

void ADivePawn::MoveAtShore(float Step,float Forward,float Side,float Vertical,float Sprint,FVector& Wish,float& Effort)
{
    using namespace Dive::ShoreMovement;
    auto* pc=Cast<APlayerController>(GetController()); if(!pc) return;
    const FVector old=GetActorLocation();
    const double surface=Environment->SurfaceZ(old.X,old.Y,Simulation.Data.ElapsedSeconds);
    // During the prone-to-standing transition, the eye can be above a landing
    // while the old nominal capsule foot is still below its top. Consider that
    // nearby support, then raise the body through swept movement before standing.
    const double supportAllowance=(!bWalking||!PhysicalPose.Walking||bStandingUp)?45:1;
    const double supportLimit=old.Z-HalfHeight*100+supportAllowance;
    float ground=0; if(!Environment->SupportZ(old.X,old.Y,ground,supportLimit)) { VelocityMetres=FVector::ZeroVector; return; }
    const bool canStand=CanStand(old.Z/100,ground/100,surface/100,bWalking);
    bStandingUp=canStand&&old.Z<ground+FloorClearance*100-1;
    bWalking=canStand&&!bStandingUp;
    if(bStandingUp)
    {
        FHitResult lift;
        MoveWithContacts(FVector(0,0,FMath::Min(ground+FloorClearance*100-old.Z,Step*150.)),lift);
        VelocityMetres.Z=FMath::Max(0.,VelocityMetres.Z);
    }
    const double immersion=Immersion(old.Z/100,surface/100);
    if(bWalking)
    {
        const FRotationMatrix rotation(FRotator(0,PhysicalPose.BodyYaw(),0));
        Wish=(rotation.GetUnitAxis(EAxis::X)*Forward+rotation.GetUnitAxis(EAxis::Y)*Side).GetClampedToMaxSize(1);
        Effort=Wish.Size()*(Sprint>0?.7f:.35f);
        const FVector target=Wish*WalkSpeed(Sprint>0,0)*Simulation.Data.MotorCapacity;
        const FVector horizontal(VelocityMetres.X,VelocityMetres.Y,0);
        const FVector change=(target-horizontal).GetClampedToMaxSize(250/Simulation.Data.TotalMassKg*Step);
        VelocityMetres=horizontal+change;
        FVector destination=old+VelocityMetres*100*Step;
        float nextGround=0;
        if(!Environment->SupportZ(destination.X,destination.Y,nextGround,supportLimit)) { VelocityMetres=FVector::ZeroVector; return; }
        const double distance=FVector::Dist2D(old,destination),rise=nextGround-ground;
        if(rise>distance*.85+1) { VelocityMetres=FVector::ZeroVector; return; }
        if(rise>=-35) destination.Z=nextGround+FloorClearance*100;
        else { bWalking=false; VelocityMetres.Z=-Dive::Gravity*Step; destination.Z=old.Z+VelocityMetres.Z*100*Step; }
        FHitResult hit; MoveWithContacts(destination-old,hit);
    }
    else
    {
        const FRotationMatrix rotation(pc->GetControlRotation());
        Wish=(rotation.GetUnitAxis(EAxis::X)*Forward+rotation.GetUnitAxis(EAxis::Y)*Side+FVector::UpVector*Vertical).GetClampedToMaxSize(1);
        Effort=Wish.Size()*(Sprint>0?1.f:.45f);
        const FVector current=(FVector(0,Simulation.Settings.CurrentMetresPerSecond,0)+Environment->WaveVelocity(old,Simulation.Data.ElapsedSeconds))*immersion;
        const FVector relative=VelocityMetres-current;
        const double fatigue=Simulation.Data.MotorCapacity;
        FVector force=Wish*SwimForceNewtons*(Sprint>0?1.7:1)*fatigue*immersion;
        force.Z+=VerticalForce(Simulation.Data.BuoyancyNewtons,immersion,Simulation.Data.TotalMassKg);
        if(bStandingUp) force.Z=FMath::Max(0.,force.Z);
        force-=relative*relative.Size()*FMath::Lerp(1.5,static_cast<double>(DragCoefficient),immersion);
        VelocityMetres+=force/Simulation.Data.TotalMassKg*Step;
        VelocityMetres=VelocityMetres.GetClampedToMaxSize(FMath::Lerp(8.,2.5,immersion));
        FHitResult hit; MoveWithContacts(VelocityMetres*100*Step,hit);
    }
    FVector position=GetActorLocation(); float floor=0;
    if(!Environment->SupportZ(position.X,position.Y,floor,position.Z-HalfHeight*100+supportAllowance))
    { FHitResult hit;MoveWithContacts(old-position,hit); VelocityMetres=FVector::ZeroVector; return; }
    if(bWalking&&position.Z<floor+FloorClearance*100)
    { FHitResult hit;MoveWithContacts(FVector(0,0,floor+FloorClearance*100-position.Z),hit);VelocityMetres.Z=FMath::Max(0.,VelocityMetres.Z);position=GetActorLocation(); }
    const double nextSurface=Environment->SurfaceZ(position.X,position.Y,Simulation.Data.ElapsedSeconds+Step);
    bWalking=CanStand(position.Z/100,floor/100,nextSurface/100,bWalking)&&position.Z>=floor+FloorClearance*100-1;
}
