#include "DiveContacts.h"
#include "DivePose.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

namespace
{
FCollisionQueryParams Query(const AActor* Ignore)
{ FCollisionQueryParams q(SCENE_QUERY_STAT(DiveBodyContact),false,Ignore);q.bFindInitialOverlaps=true;return q; }
// Pose queries expand every local box axis by 0.5 cm. One centimetre of
// normal separation also clears the diagonal of that shell (sqrt(3)*0.5).
constexpr double Skin=1.;
}

bool DiveContacts::Overlap(UWorld* World,const TArray<FDiveContactShape>& Shapes,const AActor* Ignore,FName* Part,float Margin)
{
    const auto query=Query(Ignore);
    for(const auto& s:Shapes)
        if(World->OverlapBlockingTestByChannel(s.World.GetLocation(),s.World.GetRotation(),ECC_Pawn,s.Collision(Margin),query))
        { if(Part) *Part=s.Name;return true; }
    return false;
}

bool DiveContacts::PoseClear(UWorld* World,const TArray<FDiveContactShape>& Before,const TArray<FDiveContactShape>& After,const AActor* Ignore,FName* Part)
{
    if(Before.Num()!=After.Num()) return false;
    const auto query=Query(Ignore);
    for(int i=0;i<Before.Num();++i)
    {
        const auto& a=Before[i];const auto& b=After[i];
        if(a.Name!=b.Name||a.Box!=b.Box) return false;
        if(a.World.Equals(b.World,.0001)&&a.Size.Equals(b.Size,.0001)) continue;
        const double radius=FMath::Max(a.Radius(),b.Radius());
        const FVector middle=(a.World.GetLocation()+b.World.GetLocation())*.5;
        const FVector extent=(a.World.GetLocation()-b.World.GetLocation()).GetAbs()*.5+FVector(radius+.5);
        if(!World->OverlapBlockingTestByChannel(middle,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(extent),query)) continue;
        // A sample moves any point by at most 1 cm. The 0.5 cm shell covers
        // the intervening half-step, including rotational corner arcs. This
        // tests the path of a turn, not merely its clear endpoint.
        const double travel=FVector::Distance(a.World.GetLocation(),b.World.GetLocation())+
            radius*a.World.GetRotation().AngularDistance(b.World.GetRotation())+(b.Size-a.Size).Size();
        const int steps=FMath::Max(1,FMath::CeilToInt(travel));
        if(steps>2048) { if(Part) *Part=b.Name;return false; }
        for(int j=1;j<=steps;++j)
        {
            const double t=static_cast<double>(j)/steps;
            auto sample=b;sample.Size=FMath::Lerp(a.Size,b.Size,t);
            const FVector position=FMath::Lerp(a.World.GetLocation(),b.World.GetLocation(),t);
            const FQuat rotation=FQuat::Slerp(a.World.GetRotation(),b.World.GetRotation(),t).GetNormalized();
            if(World->OverlapBlockingTestByChannel(position,rotation,ECC_Pawn,sample.Collision(.5f),query))
            { if(Part) *Part=b.Name;return false; }
        }
    }
    return true;
}

bool DiveContacts::PosePathClear(UWorld* World,const FDivePoseState& Before,FDivePoseState& After,
    const TArray<FDiveContactShape>& Start,TArray<FDiveContactShape>& End,
    TFunctionRef<void(FDivePoseState&,TArray<FDiveContactShape>&)> Build,const AActor* Ignore,FName* Part)
{
    const double yaw=FMath::FindDeltaAngleDegrees(Before.Gaze.Yaw,After.Gaze.Yaw);
    const double pitch=FMath::FindDeltaAngleDegrees(Before.Gaze.Pitch,After.Gaze.Pitch);
    const double bodyYaw=yaw-(After.HeadYaw-Before.HeadYaw);
    const float phase=FMath::FindDeltaAngleRadians(Before.Phase,After.Phase);
    const double angle=FMath::Max(FMath::Abs(bodyYaw),FMath::Max3(FMath::Abs(yaw),FMath::Abs(pitch),static_cast<double>(FMath::Abs(After.BodyPitch-Before.BodyPitch))));
    const double joints=FMath::Max(FMath::Abs(After.KneeTuck-Before.KneeTuck)*100.,
        FMath::Max3(FMath::Abs(After.Swim-Before.Swim)*90.,FMath::Abs(After.Wrist-Before.Wrist)*90.,FMath::Abs(phase)*180./PI));
    const int steps=FMath::Max(1,FMath::CeilToInt(FMath::Max(angle,joints)/2.));
    TArray<FDiveContactShape> previous=Start;
    for(int i=1;i<=steps;++i)
    {
        const float t=static_cast<float>(i)/steps;
        auto pose=After;
        pose.Gaze=FRotator(Before.Gaze.Pitch+pitch*t,Before.Gaze.Yaw+yaw*t,0);
        // Keep the head inside its authored range, including when the shortest
        // gaze arc requires the shoulders to take a slightly longer turn.
        pose.HeadYaw=FMath::Lerp(Before.HeadYaw,After.HeadYaw,t);
        pose.BodyPitch=FMath::Lerp(Before.BodyPitch,After.BodyPitch,t);
        pose.Swim=FMath::Lerp(Before.Swim,After.Swim,t);pose.Wrist=FMath::Lerp(Before.Wrist,After.Wrist,t);
        pose.KneeTuck=FMath::Lerp(Before.KneeTuck,After.KneeTuck,t);
        pose.Motion=FMath::Lerp(Before.Motion,After.Motion,t);pose.Phase=Before.Phase+phase*t;
        // Rebuild the articulated rig along the turn. Interpolating only shape
        // centres would cut across the cylinder's orbit around the eye pivot.
        Build(pose,End);
        if(!PoseClear(World,previous,End,Ignore,Part)) return false;
        if(i==steps) { After.FloorPitch[0]=pose.FloorPitch[0];After.FloorPitch[1]=pose.FloorPitch[1]; }
        else previous=End;
    }
    return true;
}

bool DiveContacts::Sweep(UWorld* World,const TArray<FDiveContactShape>& Shapes,const FVector& Delta,const AActor* Ignore,FHitResult& Hit,FName* Part)
{
    Hit=FHitResult(1.f);const auto query=Query(Ignore);bool blocked=false;
    for(const auto& s:Shapes)
    {
        FHitResult candidate;
        // Detect grazing contact before the actual shape touches a triangle.
        // This smaller shell fits inside the 0.5 cm clearance of accepted poses.
        bool found=World->SweepSingleByChannel(candidate,s.World.GetLocation(),s.World.GetLocation()+Delta,
            s.World.GetRotation(),ECC_Pawn,s.Collision(.25f),query);
        if(found&&candidate.Distance<.1&&FVector::DotProduct(Delta,candidate.ImpactNormal)>0)
        {
            // A separating shape may still touch the numerical query shell.
            // Re-sweep the actual shape, rather than ignoring the component:
            // another face or an obstacle farther along the path still blocks.
            found=World->SweepSingleByChannel(candidate,s.World.GetLocation(),s.World.GetLocation()+Delta,
                s.World.GetRotation(),ECC_Pawn,s.Collision(),query);
        }
        if(found&&(!blocked||candidate.Time<Hit.Time||candidate.bStartPenetrating))
        { blocked=true;Hit=candidate;if(Part) *Part=s.Name;if(Hit.bStartPenetrating) break; }
    }
    return blocked;
}

FVector DiveContacts::Move(UWorld* World,TArray<FDiveContactShape>& Shapes,const FVector& Delta,const AActor* Ignore,FHitResult& Hit,FName* Part)
{
    FVector moved=FVector::ZeroVector,remaining=Delta;Hit=FHitResult(1.f);
    TArray<FVector,TInlineAllocator<4>> normals;
    for(int iteration=0;iteration<4&&!remaining.IsNearlyZero(.0001);++iteration)
    {
        FHitResult contact;FName part;
        const bool blocked=Sweep(World,Shapes,remaining,Ignore,contact,&part);
        if(blocked&&contact.bStartPenetrating) { Hit=contact;if(Part) *Part=part;break; }
        const double approach=blocked?FMath::Max(.000001,-FVector::DotProduct(remaining,contact.Normal)):1.;
        const double fraction=blocked?FMath::Clamp(contact.Time-Skin/approach,0.,1.):1.;
        const FVector step=remaining*fraction;moved+=step;
        for(auto& s:Shapes) s.World.AddToTranslation(step);
        if(!blocked) break;
        // A valid joint pose may use part of the translation skin. Restore
        // that separation with another compound sweep before sliding; otherwise
        // a grazing triangle hit can repeatedly report time zero at lake-scale
        // coordinates and prevent all tangential movement.
        const double gap=contact.Time*approach;
        if(fraction==0&&gap<Skin-.01)
        {
            const FVector separation=contact.Normal*(Skin-gap);
            FHitResult clearance;
            const bool obstructed=Sweep(World,Shapes,separation,Ignore,clearance);
            const double amount=obstructed?(clearance.bStartPenetrating?0.:FMath::Max(0.,static_cast<double>(clearance.Time)-.01)):1.;
            const FVector correction=separation*amount;moved+=correction;
            for(auto& s:Shapes) s.World.AddToTranslation(correction);
        }
        Hit=contact;if(Part) *Part=part;normals.Add(contact.Normal);
        remaining*=1-fraction;
        for(const FVector& normal:normals)
            if(FVector::DotProduct(remaining,normal)<0) remaining=FVector::VectorPlaneProject(remaining,normal);
    }
    return moved;
}
