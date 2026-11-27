#include "DiveContacts.h"
#include "DivePose.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "Components/BoxComponent.h"
#include "DivePawn.h"
#include "DiveAvatar.h"
#include "DiveEquipment.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveStoppedKick,"DiveSim.Contacts.StoppedKick",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveStoppedKick::RunTest(const FString&)
{
    FDivePoseState pose;pose.Swim=1;pose.Motion=1;pose.Phase=1.2f;
    const float stopped=pose.Phase;
    for(int i=0;i<100;++i)
    {
        // Reproduce a contact keeping the amplitude at its previous value.
        pose.Motion=1;pose.Advance(.02f,FRotator::ZeroRotator,false,true,false,0,0,true);
        TestEqual(TEXT("No new kick cycle without input, even while the pose is constrained"),pose.Phase,stopped);
    }
    TestTrue(TEXT("Reading the computer remains possible"),pose.Wrist>.99f);
    pose.Advance(.02f,FRotator::ZeroRotator,false,true,true,1,.45f,false);
    TestTrue(TEXT("Active swimming resumes the kick cycle"),pose.Phase>stopped);
    return true;
}

namespace
{
struct FContactWorld
{
    UWorld* World=nullptr;
    FContactWorld()
    {
        auto init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(true)
            .ShouldSimulatePhysics(false).EnableTraceCollision(true).CreateNavigation(false).CreateAISystem(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&init);
    }
    ~FContactWorld() { World->DestroyWorld(false); }
    AActor* Box(const FVector& Center,const FVector& Extent)
    {
        auto* actor=World->SpawnActor<AActor>();auto* box=NewObject<UBoxComponent>(actor);
        actor->SetRootComponent(box);box->SetBoxExtent(Extent);box->SetCollisionProfileName(TEXT("BlockAll"));
        box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);box->RegisterComponent();actor->SetActorLocation(Center);
        return actor;
    }
    ADivePawn* Diver()
    {
        auto* pawn=World->SpawnActor<ADivePawn>();
        pawn->Equipment=World->SpawnActor<ADiveEquipment>();pawn->Equipment->SetOwner(pawn);pawn->Equipment->Initialize(pawn);
        pawn->bDiveActive=true;pawn->bMenu=false;return pawn;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveBlockedHeadTurn,"DiveSim.Contacts.BlockedBodyAllowsHeadTurn",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveBlockedHeadTurn::RunTest(const FString&)
{
    FContactWorld fixture;auto* pawn=fixture.Diver();
    auto* controller=fixture.World->SpawnActor<APlayerController>();controller->Possess(pawn);
    pawn->bWalking=false;pawn->Simulation.Data.DepthMetres=5;pawn->ResetContactPose();
    const auto initial=pawn->PhysicalPose;
    auto build=[&](FDivePoseState& pose,TArray<FDiveContactShape>& shapes)
    {
        FTransform left,right;pawn->Equipment->Avatar->ContactPose(pose,pawn->PhysicalEyes(),shapes,left,right);
        pawn->Equipment->HandContacts(left,right,shapes);
    };
    auto turned=initial;turned.Gaze.Yaw=90;TArray<FDiveContactShape> shapes;build(turned,shapes);
    const auto* cylinder=shapes.FindByPredicate([](const auto& shape){return shape.Name==TEXT("Cylinder");});
    if(!TestNotNull(TEXT("Cylinder exists"),cylinder)) return false;
    auto* obstruction=fixture.Box(cylinder->World.GetLocation(),FVector(2));
    TestFalse(TEXT("Initial body is outside the obstacle"),DiveContacts::Overlap(fixture.World,pawn->ContactShapes,pawn));
    controller->SetControlRotation(FRotator(0,90,0));pawn->AdvanceContactPose(.02f,0);
    TestTrue(TEXT("Blocked cylinder leaves shoulders facing forward"),FMath::Abs(pawn->PhysicalPose.BodyYaw())<.001);
    TestTrue(TEXT("Actual pawn accepts bounded head turn"),FMath::IsNearlyEqual(pawn->PhysicalPose.HeadYaw,65.f,.001f));
    TestTrue(TEXT("Hands and swimming thrust retain shoulder heading during sideways look"),pawn->PhysicalPose.BodyHeadingView().Vector().Equals(FVector::ForwardVector,.001));
    TestTrue(TEXT("Camera uses accepted head direction"),controller->GetControlRotation().Equals(pawn->PhysicalPose.Gaze,.001));
    TestFalse(TEXT("Head-only pose keeps all equipment clear"),DiveContacts::Overlap(fixture.World,pawn->ContactShapes,pawn));
    pawn->AdvanceContactPose(.02f,0);
    TestTrue(TEXT("Neck limit stays fixed while the requested turn is pending"),FMath::IsNearlyEqual(pawn->PhysicalPose.HeadYaw,65.f,.001f)&&pawn->bBlockedGaze);
    const FRotator acceptedView=pawn->PhysicalPose.Gaze;
    // Render new input before another 20 ms collision step. Previously the
    // camera could turn through the body while its physical head stayed put.
    controller->SetControlRotation(FRotator(-12,302,0));
    TestTrue(TEXT("Pending input cannot turn the rendered eye away from its head"),pawn->GetViewRotation().Equals(acceptedView,.001));
    FMinimalViewInfo rendered;
    pawn->Camera->GetCameraView(.005f,rendered);
    TestTrue(TEXT("Camera component uses the accepted pose between physics steps"),rendered.Rotation.Equals(acceptedView,.001));
    pawn->bMenu=true;
    pawn->Camera->GetCameraView(.005f,rendered);
    TestTrue(TEXT("Pause retains the accepted camera pose"),rendered.Rotation.Equals(acceptedView,.001));
    pawn->bMenu=false;
    controller->SetControlRotation(acceptedView); // Keep the original pending 90-degree turn.
    obstruction->SetActorEnableCollision(false);
    pawn->AdvanceContactPose(.02f,0);
    TestTrue(TEXT("Clearing the obstacle releases the preserved full turn"),pawn->PhysicalPose.Gaze.Equals(FRotator(0,90,0),.001)&&FMath::Abs(pawn->PhysicalPose.HeadYaw)<.001&&!pawn->bBlockedGaze);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveHeadObstacle,"DiveSim.Contacts.SidewaysMaskCannotCrossObstacle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveHeadObstacle::RunTest(const FString&)
{
    FContactWorld fixture;auto* pawn=fixture.Diver();
    auto build=[&](FDivePoseState& pose,TArray<FDiveContactShape>& shapes)
    {
        FTransform left,right;pawn->Equipment->Avatar->ContactPose(pose,FVector::ZeroVector,shapes,left,right);
        pawn->Equipment->HandContacts(left,right,shapes);
    };
    FDivePoseState start;start.Swim=1;start.BodyPitch=-80;
    auto end=start;end.LookWithoutBody(FRotator(0,65,0));
    TArray<FDiveContactShape> before,after;build(start,before);build(end,after);
    const auto* mask=after.FindByPredicate([](const auto& shape){return shape.Name==TEXT("Mask");});
    if(!TestNotNull(TEXT("Imported mask exists"),mask)) return false;
    AActor* obstruction=nullptr;
    for(int i=0;i<8&&!obstruction;++i)
    {
        const FVector corner=mask->Size*FVector(i&1?1:-1,i&2?1:-1,i&4?1:-1)*.95;
        auto* candidate=fixture.Box(mask->World.TransformPosition(corner),FVector(.3));
        if(DiveContacts::Overlap(fixture.World,before,pawn,nullptr,.5f)) candidate->SetActorEnableCollision(false);
        else obstruction=candidate;
    }
    if(!TestNotNull(TEXT("Obstacle is clear of the initial rig and inside the turned mask"),obstruction)) return false;
    FName part;
    TestFalse(TEXT("Independent head turn collision-tests its entire path"),DiveContacts::PosePathClear(fixture.World,start,end,before,after,build,pawn,&part));
    TestTrue(TEXT("Mask or head blocks the requested look"),part==TEXT("Mask")||part==TEXT("Head")||part==TEXT("Mouthpiece"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveActualRigContact,"DiveSim.Contacts.ImportedRigMatchesContactJoints",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveActualRigContact::RunTest(const FString&)
{
    FContactWorld fixture;auto* pawn=fixture.Diver();auto* avatar=pawn->Equipment->Avatar.Get();
    for(int variant=0;variant<6;++variant)
    for(int posture=0;posture<4;++posture)
    for(float headYaw:{-65.f,0.f,65.f})
    {
        pawn->PendingConfig.CylinderMaterial=variant/3;pawn->PendingConfig.BCDModel=variant%3;
        pawn->PendingConfig.CylinderLitres=variant%3==0?10:variant%3==1?12:15;pawn->PendingConfig.DrySuit=variant%2==0;
        pawn->Simulation.Reset(pawn->PendingConfig);
        auto& pose=pawn->PhysicalPose;pose=FDivePoseState();
        pose.Swim=posture==0?0:posture==1?.5f:1;pose.BodyPitch=-80*pose.Swim;
        pose.Gaze=FRotator(posture==2?-65:posture==3?85:0,37,0);pose.Wrist=posture%2;
        pose.HeadYaw=headYaw;
        pose.Motion=.7;pose.Phase=1.3;pose.Walking=posture==0;
        pose.KneeTuck=posture==1?.6f:0.f;
        TArray<FDiveContactShape> shapes;FTransform left,right;
        avatar->ContactPose(pose,pawn->PhysicalEyes(),shapes,left,right);pawn->Equipment->HandContacts(left,right,shapes);
        pawn->Equipment->Update(0);
        TestEqual(TEXT("All body and equipment contact members are present"),shapes.Num(),24);
        FString error;const bool valid=avatar->Validate(error);TestTrue(FString(TEXT("Rendered pose is valid: "))+error,valid);
        TArray<UStaticMeshComponent*> meshes,hands;avatar->GetComponents(meshes);pawn->Equipment->GetComponents(hands);meshes.Append(hands);
        const TMap<FName,FName> components={{TEXT("Mask"),TEXT("Equipment_1")},{TEXT("LeftBoot"),TEXT("Equipment_8")},
            {TEXT("RightBoot"),TEXT("Equipment_9")},{TEXT("LeftFin"),TEXT("Equipment_10")},{TEXT("RightFin"),TEXT("Equipment_11")}};
        for(const auto& shape:shapes) if(shape.Box)
        {
            const FName componentName=components.Contains(shape.Name)?components[shape.Name]:shape.Name;
            auto** found=meshes.FindByPredicate([&](auto* mesh){return mesh->GetFName()==componentName;});
            if(!TestNotNull(TEXT("Contact box has a rendered component"),found)) continue;
            const auto& transform=(*found)->GetComponentTransform();
            TestTrue(TEXT("Rendered equipment and physics have the same orientation"),transform.GetRotation().Equals(shape.World.GetRotation(),.0001));
            TestTrue(TEXT("Rendered equipment and physics have the same centre"),
                transform.TransformPosition((*found)->GetStaticMesh()->GetBoundingBox().GetCenter()).Equals(shape.World.GetLocation(),.05));
        }
        const bool leftIsL=avatar->Suit->GetBoneTransformByName(TEXT("upper_l"),EBoneSpaces::ComponentSpace).GetLocation().Y<0;
        for(int side=0;side<2;++side)
        {
            const FString suffix=(side==0)==leftIsL?TEXT("_l"):TEXT("_r");
            const TCHAR* names[4]={side?TEXT("RightUpperArm"):TEXT("LeftUpperArm"),side?TEXT("RightForearm"):TEXT("LeftForearm"),
                side?TEXT("RightThigh"):TEXT("LeftThigh"),side?TEXT("RightShin"):TEXT("LeftShin")};
            const TCHAR* bones[4]={TEXT("upper"),TEXT("lower"),TEXT("thigh"),TEXT("shin")};
            for(int joint=0;joint<4;++joint)
            {
                const auto* shape=shapes.FindByPredicate([&](const auto& s){return s.Name==names[joint];});
                if(!TestNotNull(TEXT("Joint contact shape exists"),shape)) continue;
                const FVector start=shape->World.TransformPosition(FVector(0,0,-(shape->Size.Y-shape->Size.X)));
                const FVector bone=avatar->Suit->GetBoneTransformByName(FName(FString(bones[joint])+suffix),EBoneSpaces::WorldSpace).GetLocation();
                TestTrue(TEXT("Physics capsule starts at rendered joint"),start.Equals(bone,.05));
            }
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveActualEquipmentContact,"DiveSim.Contacts.ImportedCylinderFinAndHand",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveActualEquipmentContact::RunTest(const FString&)
{
    FContactWorld fixture;auto* pawn=fixture.Diver();
    for(int test=0;test<3;++test)
    {
        const FVector base(test*2000,0,0);
        FDivePoseState pose;pose.Swim=1;pose.BodyPitch=-80;
        TArray<FDiveContactShape> shapes;FTransform left,right;
        pawn->Equipment->Avatar->ContactPose(pose,base,shapes,left,right);pawn->Equipment->HandContacts(left,right,shapes);
        const FVector delta=test==0?FVector(0,0,200):test==1?FVector(200,0,0):FVector(-200,0,0);
        fixture.Box(base+(test==0?FVector(-55,0,100):test==1?FVector(100,0,0):FVector(-260,0,0)),
            test==0?FVector(25,60,2):FVector(2,800,800));
        FHitResult hit;FName part;
        const auto moved=DiveContacts::Move(fixture.World,shapes,delta,pawn,hit,&part);
        TestTrue(TEXT("Actual loadout is stopped before crossing the obstacle"),hit.bBlockingHit&&moved.Size()<delta.Size()&&!hit.bStartPenetrating);
        if(test==0) TestTrue(TEXT("Cylinder, not eye capsule, meets ceiling first"),part==TEXT("Cylinder"));
        if(test==1) TestTrue(TEXT("Hand equipment meets forward wall first"),part.ToString().Contains(TEXT("Glove"))||part.ToString().Contains(TEXT("Torch")));
        if(test==2) TestTrue(TEXT("Fin blade meets rear wall first"),part.ToString().Contains(TEXT("Fin")));
        FName overlap;const bool penetrating=DiveContacts::Overlap(fixture.World,shapes,pawn,&overlap);
        TestFalse(FString::Printf(TEXT("Imported contacts remain outside wall: hit=%s overlap=%s moved=%s"),*part.ToString(),*overlap.ToString(),*moved.ToString()),penetrating);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveSweptContact,"DiveSim.Contacts.SweptTranslationAndSlide",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveSweptContact::RunTest(const FString&)
{
    FContactWorld fixture;fixture.Box(FVector(100,0,0),FVector(2,1000,1000));
    TArray<FDiveContactShape> shapes{FDiveContactShape::Capsule(TEXT("Torso"),FVector(0,0,-20),FVector(0,0,20),10)};
    FHitResult hit;FName part;
    const FVector moved=DiveContacts::Move(fixture.World,shapes,FVector(300,100,0),nullptr,hit,&part);
    TestTrue(TEXT("Translation is blocked by the wall"),hit.bBlockingHit&&part==TEXT("Torso"));
    TestTrue(TEXT("Body retains contact clearance"),FMath::Abs(moved.X-86.75)<.02);
    TestTrue(TEXT("Tangential movement survives the collision"),FMath::Abs(moved.Y-100)<.01);
    TestFalse(TEXT("Slid body has no penetrating shape"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    const FVector away=DiveContacts::Move(fixture.World,shapes,FVector(-40,0,0),nullptr,hit);
    TestTrue(TEXT("Body can move away from contact"),away.Equals(FVector(-40,0,0),.01));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveSeparatingContact,"DiveSim.Contacts.SeparateWithoutMissingNextObstacle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveSeparatingContact::RunTest(const FString&)
{
    FContactWorld fixture;const FVector origin(267183,8698,-529);
    fixture.Box(origin-FVector(0,0,10),FVector(1000,1000,10));
    fixture.Box(origin+FVector(0,0,50),FVector(1000,1000,.1));
    TArray<FDiveContactShape> shapes{{TEXT("Fin"),FTransform(origin+FVector(0,0,3.1)),FVector(30,10,3),true}};
    TestFalse(TEXT("The fin itself starts outside the floor"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    FHitResult hit;const FVector moved=DiveContacts::Move(fixture.World,shapes,FVector(0,0,100),nullptr,hit);
    TestTrue(TEXT("Can leave an overlapping query shell"),moved.Z>40);
    TestTrue(TEXT("A thin ceiling beyond the departing floor still blocks"),hit.bBlockingHit&&moved.Z<43);
    TestFalse(TEXT("Separation ends outside the actual geometry"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveSlopeContact,"DiveSim.Contacts.GrazingSlopeAtLakeCoordinates",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveSlopeContact::RunTest(const FString&)
{
    FContactWorld fixture;const FVector origin(52660,830331,-165),normal=FVector(.379,-.025,.925).GetSafeNormal();
    auto* floor=fixture.Box(origin-normal*10,FVector(10000,10000,10));
    floor->SetActorRotation(FQuat::FindBetweenVectors(FVector::UpVector,normal));
    const FQuat rotation=FRotator(-55,7,0).Quaternion();const FVector size(30,10,3);
    const double support=FMath::Abs(FVector::DotProduct(rotation.GetAxisX(),normal))*size.X+
        FMath::Abs(FVector::DotProduct(rotation.GetAxisY(),normal))*size.Y+FMath::Abs(FVector::DotProduct(rotation.GetAxisZ(),normal))*size.Z;
    TArray<FDiveContactShape> shapes{{TEXT("Fin"),FTransform(rotation,origin+normal*(support+.9)),size,true}};
    const FVector start=shapes[0].World.GetLocation();FHitResult hit;
    TestFalse(TEXT("Slope fixture starts clear"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    for(int i=0;i<200;++i) DiveContacts::Move(fixture.World,shapes,FVector(2.14,-.032,-.902),nullptr,hit);
    const FVector moved=shapes[0].World.GetLocation()-start;
    AddInfo(FString::Printf(TEXT("Slope movement %s penetration=%d depth=%f normal=%s support=%f"),*moved.ToString(),hit.bStartPenetrating,hit.PenetrationDepth,*hit.Normal.ToString(),support));
    TestTrue(TEXT("Grazing fin continues down the slope instead of sticking"),moved.X>400);
    TestFalse(TEXT("Slope slide keeps the actual fin outside the floor"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveCylinderContact,"DiveSim.Contacts.EquipmentWinsFirstContact",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveCylinderContact::RunTest(const FString&)
{
    FContactWorld fixture;fixture.Box(FVector(-100,0,0),FVector(2,1000,1000));
    TArray<FDiveContactShape> shapes{
        FDiveContactShape::Capsule(TEXT("Torso"),FVector::ZeroVector,FVector::ZeroVector,20),
        FDiveContactShape::Capsule(TEXT("Cylinder"),FVector(-35,0,-20),FVector(-35,0,20),12)};
    FHitResult hit;FName part;
    const auto moved=DiveContacts::Move(fixture.World,shapes,FVector(-300,0,0),nullptr,hit,&part);
    TestTrue(TEXT("Cylinder blocks before torso"),part==TEXT("Cylinder")&&FMath::Abs(moved.X+49.75)<.02);
    TestFalse(TEXT("All compound members remain clear"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveOrbitContact,"DiveSim.Contacts.ImportedCylinderOrbit",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveOrbitContact::RunTest(const FString&)
{
    FContactWorld fixture;auto* pawn=fixture.Diver();
    auto build=[&](FDivePoseState& pose,TArray<FDiveContactShape>& shapes)
    {
        FTransform left,right;pawn->Equipment->Avatar->ContactPose(pose,FVector::ZeroVector,shapes,left,right);
        pawn->Equipment->HandContacts(left,right,shapes);
    };
    FDivePoseState start;start.Swim=1;start.BodyPitch=-80;
    auto end=start;end.Gaze.Yaw=180;auto middle=start;middle.Gaze.Yaw=90;
    TArray<FDiveContactShape> before,after,half;build(start,before);build(end,after);build(middle,half);
    const auto* cylinder=half.FindByPredicate([](const auto& shape){return shape.Name==TEXT("Cylinder");});
    if(!TestNotNull(TEXT("Imported cylinder is present"),cylinder)) return false;
    fixture.Box(cylinder->World.GetLocation(),FVector(2));
    TestFalse(TEXT("Initial rig pose is clear"),DiveContacts::Overlap(fixture.World,before,pawn));
    TestFalse(TEXT("Final rig pose is clear"),DiveContacts::Overlap(fixture.World,after,pawn));
    TestFalse(TEXT("Whole-body turn checks the cylinder orbit between clear endpoints"),
        DiveContacts::PosePathClear(fixture.World,start,end,before,after,build,pawn));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveTurningContact,"DiveSim.Contacts.RotationSweepsIntermediatePose",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveTurningContact::RunTest(const FString&)
{
    FContactWorld fixture;fixture.Box(FVector(0,40,0),FVector(8,4,8));
    TArray<FDiveContactShape> before{{TEXT("Fin"),FTransform::Identity,FVector(60,5,5),true}},after=before;
    after[0].World.SetRotation(FRotator(0,180,0).Quaternion());
    TestFalse(TEXT("Start of turn is clear"),DiveContacts::Overlap(fixture.World,before,nullptr));
    TestFalse(TEXT("End of turn is also clear"),DiveContacts::Overlap(fixture.World,after,nullptr));
    FName part;
    TestFalse(TEXT("Turn cannot sweep a fin through intervening geometry"),DiveContacts::PoseClear(fixture.World,before,after,nullptr,&part));
    TestTrue(TEXT("Rotating member is identified"),part==TEXT("Fin"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveHorizontalContact,"DiveSim.Contacts.HorizontalClearance",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveHorizontalContact::RunTest(const FString&)
{
    FContactWorld fixture;
    fixture.Box(FVector(0,0,50),FVector(1000,1000,10));fixture.Box(FVector(0,0,-50),FVector(1000,1000,10));
    TArray<FDiveContactShape> shapes{FDiveContactShape::Capsule(TEXT("Body"),FVector(-70,0,0),FVector(70,0,0),20)};
    FHitResult hit;const auto moved=DiveContacts::Move(fixture.World,shapes,FVector(500,0,0),nullptr,hit);
    TestTrue(TEXT("Horizontal body passes an 80 cm high space"),moved.Equals(FVector(500,0,0),.01)&&!hit.bBlockingHit);
    auto standing=shapes;standing[0].World.SetRotation(FQuat::Identity);
    TestFalse(TEXT("Standing transition requires clearance"),DiveContacts::PoseClear(fixture.World,shapes,standing,nullptr));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveThinContact,"DiveSim.Contacts.ThinObstacleAndCorner",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveThinContact::RunTest(const FString&)
{
    FContactWorld fixture;
    fixture.Box(FVector(100,0,0),FVector(.1,1000,1000));fixture.Box(FVector(0,100,0),FVector(1000,.1,1000));
    TArray<FDiveContactShape> shapes{{TEXT("Fin"),FTransform::Identity,FVector(30,10,2),true}};
    FHitResult hit;
    const auto moved=DiveContacts::Move(fixture.World,shapes,FVector(5000,5000,0),nullptr,hit);
    TestTrue(TEXT("High displacement cannot cross a thin wall or corner"),hit.bBlockingHit&&moved.X<70&&moved.Y<90);
    TestTrue(TEXT("Both tangential degrees advance until blocked"),moved.X>68&&moved.Y>88);
    TestFalse(TEXT("Corner leaves the fin outside both walls"),DiveContacts::Overlap(fixture.World,shapes,nullptr));
    return true;
}
#endif
