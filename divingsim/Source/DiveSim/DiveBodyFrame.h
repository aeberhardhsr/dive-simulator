#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

// Shared centimetre-space landmarks for the authored diver. Neck limits are
// animation constraints, not a biomechanical or training model.
struct FDiveBodyFrame
{
    FTransform BodyWorld,HeadLocal;
    float BodyPitch=0,HeadPitch=0;
    static FVector Eyes() { return FVector(8,0,170); }
    static FVector Neck() { return FVector(0,0,149); }
    static FVector Mouth() { return FVector(11.5,0,158.5); }
    static FRotator View(float Pitch,float Yaw)
    { return FRotator(FRotator::NormalizeAxis(Pitch),Yaw,0); }

    static void ApplyLocalPitch(USceneComponent* Component,const FVector& Location,float Pitch)
    {
        Component->SetRelativeLocation(Location);
        Component->SetRelativeRotationExact(FRotator(Pitch,0,0));
        // Exact writes the stored Euler angles after moving the component;
        // refresh the world transform from those exact angles as well.
        Component->UpdateComponentToWorld();
    }

    static FDiveBodyFrame FromEyes(const FVector& EyeWorld,float RequestedBodyPitch,float Yaw,float ViewPitch)
    {
        FDiveBodyFrame result;
        const float pitch=FRotator::NormalizeAxis(ViewPitch);
        // At extreme gaze angles the shoulders follow, keeping the head within
        // its animated neck range while preserving the player's gaze exactly.
        result.BodyPitch=FMath::Clamp(RequestedBodyPitch,pitch-90.f,pitch+60.f);
        result.HeadPitch=pitch-result.BodyPitch;
        const FQuat head=FRotator(result.HeadPitch,0,0).Quaternion();
        result.HeadLocal=FTransform(head,Neck()-head.RotateVector(Neck()));
        const FQuat body=FRotator(result.BodyPitch,Yaw,0).Quaternion();
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
