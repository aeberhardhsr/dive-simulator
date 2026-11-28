#pragma once
#include "CoreMinimal.h"
#include "CollisionShape.h"

class UWorld;
class AActor;
struct FDivePoseState;

struct FDiveContactShape
{
    FName Name;
    FTransform World;
    FVector Size=FVector::ZeroVector; // capsule: radius, half-height; box: half-extents
    bool Box=false;
    FCollisionShape Collision(float Margin=0) const
    { return Box?FCollisionShape::MakeBox(Size+FVector(Margin)):FCollisionShape::MakeCapsule(Size.X+Margin,Size.Y+Margin); }
    double Radius() const { return Box?Size.Size():Size.Y; }
    static FDiveContactShape Capsule(FName Name,const FVector& A,const FVector& B,double Radius)
    {
        return {Name,FTransform(FQuat::FindBetweenVectors(FVector::UpVector,(B-A).GetSafeNormal(UE_SMALL_NUMBER,FVector::UpVector)),(A+B)*.5),
            FVector(Radius,FVector::Distance(A,B)*.5+Radius,0),false};
    }
    static FDiveContactShape Bounds(FName Name,const FTransform& Transform,const FBox& Bounds)
    {
        return {Name,FTransform(Transform.GetRotation(),Transform.TransformPosition(Bounds.GetCenter())),
            Bounds.GetExtent()*Transform.GetScale3D().GetAbs(),true};
    }
};

namespace DiveContacts
{
    bool Overlap(UWorld* World,const TArray<FDiveContactShape>& Shapes,const AActor* Ignore,FName* Part=nullptr,float Margin=0);
    bool PoseClear(UWorld* World,const TArray<FDiveContactShape>& Before,const TArray<FDiveContactShape>& After,const AActor* Ignore,FName* Part=nullptr);
    bool PosePathClear(UWorld* World,const FDivePoseState& Before,FDivePoseState& After,
        const TArray<FDiveContactShape>& Start,TArray<FDiveContactShape>& End,
        TFunctionRef<void(FDivePoseState&,TArray<FDiveContactShape>&)> Build,const AActor* Ignore,FName* Part=nullptr);
    bool Sweep(UWorld* World,const TArray<FDiveContactShape>& Shapes,const FVector& Delta,const AActor* Ignore,FHitResult& Hit,FName* Part=nullptr);
    FVector Move(UWorld* World,TArray<FDiveContactShape>& Shapes,const FVector& Delta,const AActor* Ignore,FHitResult& Hit,FName* Part=nullptr);
}
