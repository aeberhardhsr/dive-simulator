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
    const double supportLimit=old.Z-HalfHeight*100+1;
    float ground=0; if(!Environment->SupportZ(old.X,old.Y,ground,supportLimit)) { VelocityMetres=FVector::ZeroVector; return; }
    bWalking=CanStand(old.Z/100,ground/100,surface/100,bWalking);
    const double immersion=Immersion(old.Z/100,surface/100);
    if(bWalking)
    {
        const FRotationMatrix rotation(FRotator(0,pc->GetControlRotation().Yaw,0));
        Wish=(rotation.GetUnitAxis(EAxis::X)*Forward+rotation.GetUnitAxis(EAxis::Y)*Side).GetClampedToMaxSize(1);
        Effort=Wish.Size()*(Sprint>0?.7f:.35f);
        const FVector target=Wish*WalkSpeed(Sprint>0,0)*Simulation.Data.MotorCapacity;
        const FVector horizontal(VelocityMetres.X,VelocityMetres.Y,0);
        const FVector change=(target-horizontal).GetClampedToMaxSize(2.5*Step);
        VelocityMetres=horizontal+change;
        FVector destination=old+VelocityMetres*100*Step;
        float nextGround=0;
        if(!Environment->SupportZ(destination.X,destination.Y,nextGround,supportLimit)) { VelocityMetres=FVector::ZeroVector; return; }
        const double distance=FVector::Dist2D(old,destination),rise=nextGround-ground;
        if(rise>distance*.85+1) { VelocityMetres=FVector::ZeroVector; return; }
        if(rise>=-35) destination.Z=nextGround+FloorClearance*100;
        else { bWalking=false; VelocityMetres.Z=-Dive::Gravity*Step; destination.Z=old.Z+VelocityMetres.Z*100*Step; }
        FHitResult hit; AddActorWorldOffset(destination-old,true,&hit);
        if(hit.bBlockingHit)
        {
            const FVector remaining=FVector::VectorPlaneProject(destination-old,hit.Normal)*(1-hit.Time);
            AddActorWorldOffset(remaining,true);
            VelocityMetres=FVector::VectorPlaneProject(VelocityMetres,hit.Normal);
        }
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
        force.Z+=VerticalForce(Simulation.Data.BuoyancyNewtons,immersion);
        force-=relative*relative.Size()*FMath::Lerp(1.5,static_cast<double>(DragCoefficient),immersion);
        VelocityMetres+=force/MassKg*Step;
        VelocityMetres=VelocityMetres.GetClampedToMaxSize(FMath::Lerp(8.,2.5,immersion));
        FHitResult hit; AddActorWorldOffset(VelocityMetres*100*Step,true,&hit);
        if(hit.bBlockingHit)
        {
            VelocityMetres=FVector::VectorPlaneProject(VelocityMetres,hit.Normal);
            AddActorWorldOffset(VelocityMetres*100*Step*(1-hit.Time),true);
        }
    }
    FVector position=GetActorLocation(); float floor=0;
    if(!Environment->SupportZ(position.X,position.Y,floor,position.Z-HalfHeight*100+1))
    { SetActorLocation(old); VelocityMetres=FVector::ZeroVector; return; }
    if(position.Z<floor+FloorClearance*100)
    { position.Z=floor+FloorClearance*100; VelocityMetres.Z=FMath::Max(0.,VelocityMetres.Z); SetActorLocation(position); }
    const double nextSurface=Environment->SurfaceZ(position.X,position.Y,Simulation.Data.ElapsedSeconds+Step);
    bWalking=CanStand(position.Z/100,floor/100,nextSurface/100,bWalking);
}
