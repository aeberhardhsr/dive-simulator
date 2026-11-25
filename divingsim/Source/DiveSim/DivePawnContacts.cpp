#include "DivePawn.h"
#include "DiveEquipment.h"
#include "DiveAvatar.h"
#include "DiveWorld.h"
#include "GameFramework/PlayerController.h"

FVector ADivePawn::PhysicalEyes() const
{ return GetActorLocation()+FVector(0,0,CameraBaseHeight); }

void ADivePawn::ResetContactPose()
{
    PhysicalPose=FDivePoseState();PhysicalPose.Gaze=GetControlRotation();PhysicalPose.Walking=bWalking;
    RequestedGaze=FDiveBodyFrame::View(PhysicalPose.Gaze.Pitch,PhysicalPose.Gaze.Yaw);bBlockedGaze=false;bStandingUp=false;
    PhysicalPose.Swim=bWalking?0:1;PhysicalPose.BodyPitch=-80*PhysicalPose.Swim;
    BodyContactCount=BlockedPoseSteps=0;LastBodyContact=NAME_None;bContactPoseReady=false;
    if(!Equipment||!Equipment->Avatar) return;
    FTransform left,right;
    for(int i=0;i<101;++i)
    {
        Equipment->Avatar->ContactPose(PhysicalPose,PhysicalEyes(),ContactShapes,left,right);
        Equipment->HandContacts(left,right,ContactShapes);
        if(!DiveContacts::Overlap(GetWorld(),ContactShapes,this,&LastBodyContact,.5f)) { bContactPoseReady=true;break; }
        if(i<100) AddActorWorldOffset(FVector(0,0,2),false);
    }
    if(!bContactPoseReady) UE_LOG(LogTemp,Error,TEXT("DIVE_CONTACT_FAIL initial body overlap part=%s"),*LastBodyContact.ToString());
}

void ADivePawn::AdvanceContactPose(float Step,float Effort)
{
    if(!bContactPoseReady||!Equipment||!Equipment->Avatar) return;
    const float speed=VelocityMetres.Size();
    const bool moving=bWalking?speed>.05:Effort>.04;
    const FVector eyes=PhysicalEyes();float floor=0;
    const bool nearBed=Environment&&Environment->SupportZ(eyes.X,eyes.Y,floor,eyes.Z)&&eyes.Z-floor<170;
    const bool swimming=!bWalking&&!bStandingUp&&(moving||nearBed||Simulation.Data.DepthMetres>.6);
    FDivePoseState desired=PhysicalPose;
    const FRotator current=GetControlRotation();
    if(bBlockedGaze)
    {
        RequestedGaze.Pitch=FMath::Clamp(FRotator::NormalizeAxis(RequestedGaze.Pitch+FMath::FindDeltaAngleDegrees(PhysicalPose.Gaze.Pitch,current.Pitch)),-85.,85.);
        RequestedGaze.Yaw=FRotator::NormalizeAxis(RequestedGaze.Yaw+FMath::FindDeltaAngleDegrees(PhysicalPose.Gaze.Yaw,current.Yaw));
    }
    else RequestedGaze=FDiveBodyFrame::View(current.Pitch,current.Yaw);
    desired.Advance(Step,RequestedGaze,bWalking,swimming,moving,speed,Effort,bShowComputer);
    TArray<FDiveContactShape> candidate;FName contact;
    auto clear=[&]()
    {
        auto build=[&](FDivePoseState& pose,TArray<FDiveContactShape>& shapes)
        {
            FTransform left,right;
            Equipment->Avatar->ContactPose(pose,eyes,shapes,left,right);Equipment->HandContacts(left,right,shapes);
        };
        return DiveContacts::PosePathClear(GetWorld(),PhysicalPose,desired,ContactShapes,candidate,build,this,&contact);
    };
    bool accepted=clear();
    if(!accepted)
    {
        desired.KneeTuck=PhysicalPose.KneeTuck;
        accepted=clear();
        // A fin contacting the bed can stop the kick while the player still
        // reads the computer or turns, provided those motions have clearance.
        if(!accepted) { desired.Phase=PhysicalPose.Phase;desired.Motion=PhysicalPose.Motion;accepted=clear(); }
        if(!accepted) { desired.Wrist=PhysicalPose.Wrist;accepted=clear(); }
        const FString part=contact.ToString();
        if(!accepted&&PhysicalPose.KneeTuck<.999f&&(part.Contains(TEXT("Fin"))||part.Contains(TEXT("Boot"))||part.Contains(TEXT("Shin"))))
        {
            // Retract the heels before a turn/standing transition on a ramp.
            // This joint motion is tested just like every other pose change.
            const auto requested=desired;desired=PhysicalPose;
            desired.KneeTuck=FMath::Min(1.f,PhysicalPose.KneeTuck+Step*2);
            accepted=clear();
            if(!accepted) desired=requested;
        }
        if(!accepted) { desired.Swim=PhysicalPose.Swim;desired.BodyPitch=PhysicalPose.BodyPitch;accepted=clear(); }
        ++BlockedPoseSteps;LastBodyContact=contact;
        if(bSmokeTest&&(BlockedPoseSteps<=3||BlockedPoseSteps%200==0))
            UE_LOG(LogTemp,Display,TEXT("DIVE_CONTACT_POSE part=%s accepted=%d swim=%.3f desired=%.3f walking=%d"),*contact.ToString(),accepted,PhysicalPose.Swim,desired.Swim,bWalking);
    }
    // Keep a blocked turn request until space becomes available. Resetting the
    // controller alone used to discard a single mouse turn permanently.
    bBlockedGaze=!accepted||!desired.Gaze.Equals(RequestedGaze,.001);
    if(accepted) { PhysicalPose=desired;ContactShapes=MoveTemp(candidate); }
    if(auto* pc=Cast<APlayerController>(GetController())) pc->SetControlRotation(PhysicalPose.Gaze);
}

void ADivePawn::MoveWithContacts(const FVector& Delta,FHitResult& Hit)
{
    if(!bContactPoseReady) { Hit=FHitResult();Hit.bBlockingHit=true;VelocityMetres=FVector::ZeroVector;return; }
    FName part;
    const FVector moved=DiveContacts::Move(GetWorld(),ContactShapes,Delta,this,Hit,&part);
    AddActorWorldOffset(moved,false);
    if(bSmokeTest&&!bPerformance)
    {
        FName overlap;
        if(Hit.bStartPenetrating||DiveContacts::Overlap(GetWorld(),ContactShapes,this,&overlap))
        {
            UE_LOG(LogTemp,Error,TEXT("DIVE_CONTACT_FAIL penetrating shape=%s contact=%s"),*overlap.ToString(),*part.ToString());
            FPlatformMisc::RequestExitWithStatus(false,1);
        }
    }
    if(Hit.bBlockingHit)
    {
        ++BodyContactCount;LastBodyContact=part;
        if(bSmokeTest&&(BodyContactCount<=3||BodyContactCount%200==0))
            UE_LOG(LogTemp,Display,TEXT("DIVE_CONTACT part=%s penetration=%d depth=%.4f time=%.6f normal=%s requested=%s moved=%s position=%s component=%s"),
                *part.ToString(),Hit.bStartPenetrating,Hit.PenetrationDepth,Hit.Time,*Hit.Normal.ToString(),*Delta.ToString(),*moved.ToString(),*GetActorLocation().ToString(),
                Hit.GetComponent()?*Hit.GetComponent()->GetName():TEXT("none"));
        if(Hit.bStartPenetrating) VelocityMetres=FVector::ZeroVector;
        else if(FVector::DotProduct(VelocityMetres,Hit.Normal)<0) VelocityMetres=FVector::VectorPlaneProject(VelocityMetres,Hit.Normal);
    }
}
