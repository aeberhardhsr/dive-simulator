#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

// Shared centimetre-space landmarks for the authored diver. Neck limits are
// animation constraints, not a biomechanical or training model.
struct FDiveBodyFrame
{
    FTransform BodyWorld,HeadLocal;
    float BodyPitch=0;
    static FVector Eyes() { return FVector(8,0,170); }
    static FVector Neck() { return FVector(0,0,149); }
    static FVector Mouth() { return FVector(11.5,0,158.5); }
    static FRotator View(float Pitch,float Yaw)
    { return FRotator(FRotator::NormalizeAxis(Pitch),Yaw,0); }

    // SceneComponent stores Euler angles. Its usual quaternion conversion
    // snaps a small neighbourhood of +/-90 degrees to the singularity.
    // Extract the actual axes instead so off-origin head meshes stay anchored.
    static void ApplyLocalFrame(USceneComponent* Component,const FTransform& Frame)
    {
        const FQuat q=Frame.GetRotation();
        const FVector x=q.GetAxisX(),y=q.GetAxisY(),z=q.GetAxisZ();
        const double horizontal=FMath::Sqrt(x.X*x.X+x.Y*x.Y);
        const FRotator angles(FMath::RadiansToDegrees(FMath::Atan2(x.Z,horizontal)),
            FMath::RadiansToDegrees(horizontal>1.e-10?FMath::Atan2(x.Y,x.X):FMath::Atan2(-y.X,y.Y)),
            horizontal>1.e-10?FMath::RadiansToDegrees(FMath::Atan2(-y.Z,z.Z)):0.);
        Component->SetRelativeLocation(Frame.GetLocation());
        Component->SetRelativeRotationExact(angles);
        Component->UpdateComponentToWorld();
    }

    static FDiveBodyFrame FromEyes(const FVector& EyeWorld,float RequestedBodyPitch,float Yaw,float ViewPitch,float HeadYaw=0)
    {
        FDiveBodyFrame result;
        const float pitch=FRotator::NormalizeAxis(ViewPitch);
        // At extreme gaze angles the shoulders follow, keeping the head within
        // its animated neck range while preserving the player's gaze exactly.
        result.BodyPitch=FMath::Clamp(RequestedBodyPitch,pitch-90.f,pitch+60.f);
        const FQuat body=FRotator(result.BodyPitch,Yaw-HeadYaw,0).Quaternion();
        const FQuat head=(body.Inverse()*View(pitch,Yaw).Quaternion()).GetNormalized();
        result.HeadLocal=FTransform(head,Neck()-head.RotateVector(Neck()));
        result.BodyWorld=FTransform(body,EyeWorld-body.RotateVector(result.HeadLocal.TransformPosition(Eyes())));
        return result;
    }

    FTransform RegulatorLocal(float InMouthBlend) const
    {
        const float blend=FMath::Clamp(InMouthBlend,0.f,1.f);
        const FVector center=FMath::Lerp(Mouth()+FVector(7,9,-23),HeadLocal.TransformPosition(Mouth()),blend);
        const FQuat rotation=FQuat::Slerp(FQuat::Identity,HeadLocal.GetRotation(),blend).GetNormalized();
        return FTransform(rotation,center-rotation.RotateVector(Mouth()));
    }

    // Physical eye excludes cosmetic breathing bob. This calculation does not
    // depend on render rate, pose interpolation, or whether the avatar is visible.
    static FVector PhysicalMouth(const FVector& PhysicalEye,const FRotator& Gaze)
    { return PhysicalEye+View(Gaze.Pitch,Gaze.Yaw).RotateVector(Mouth()-Eyes()); }
};
