#include "DiveBodyFrame.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveHeadComponent,"DiveSim.Body.SceneComponentNearVertical",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveHeadComponent::RunTest(const FString&)
{
    auto* root=NewObject<USceneComponent>();
    auto* head=NewObject<USceneComponent>();
    head->SetupAttachment(root);
    for(float pitch:{89.f,89.8f,89.93f,89.99f,90.f,89.99f,89.8f,45.f,-60.f})
    {
        const auto frame=FDiveBodyFrame::FromEyes(FVector(120034,-327804,416.75),0,127,pitch);
        root->SetWorldTransform(frame.BodyWorld);
        FDiveBodyFrame::ApplyLocalPitch(head,frame.HeadLocal.GetLocation(),frame.HeadPitch);
        TestTrue(TEXT("Engine component preserves the eye anchor near the Euler singularity"),head->GetComponentTransform().TransformPosition(FDiveBodyFrame::Eyes()).Equals(FVector(120034,-327804,416.75),.01));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveBodyLandmarks,"DiveSim.Body.EyesNeckAndGaze",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveBodyLandmarks::RunTest(const FString&)
{
    const FVector eye(120034,-327804,416.75);
    for(float yaw:{-179.f,0.f,91.f,270.f})
    for(float pitch:{-85.f,-65.f,0.f,65.f,85.f,352.f})
    for(float body:{-80.f,-40.f,0.f})
    {
        const auto frame=FDiveBodyFrame::FromEyes(eye,body,yaw,pitch);
        const auto head=frame.HeadLocal*frame.BodyWorld;
        TestTrue(TEXT("Rendered eyes coincide with player camera"),head.TransformPosition(FDiveBodyFrame::Eyes()).Equals(eye,.0001));
        TestTrue(TEXT("Head faces player gaze"),head.GetUnitAxis(EAxis::X).Equals(FDiveBodyFrame::View(pitch,yaw).Vector(),.0001));
        TestTrue(TEXT("Neck joint does not translate inside torso"),frame.HeadLocal.TransformPosition(FDiveBodyFrame::Neck()).Equals(FDiveBodyFrame::Neck(),.0001));
        TestTrue(TEXT("Rendered mouth matches render-independent physical landmark"),head.TransformPosition(FDiveBodyFrame::Mouth()).Equals(FDiveBodyFrame::PhysicalMouth(eye,FRotator(pitch,yaw,0)),.0001));
        const float neckPitch=FRotator::NormalizeAxis(pitch)-frame.BodyPitch;
        TestTrue(TEXT("Shoulders follow extreme gaze within animation limits"),neckPitch>=-60.001f&&neckPitch<=90.001f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveRegulatorLandmarks,"DiveSim.Body.RegulatorEndpoints",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveRegulatorLandmarks::RunTest(const FString&)
{
    for(float pitch:{-85.f,0.f,85.f})
    for(float body:{-80.f,0.f})
    {
        const auto frame=FDiveBodyFrame::FromEyes(FVector(100,200,300),body,127,pitch);
        const auto mouth=frame.RegulatorLocal(1),chest=frame.RegulatorLocal(0);
        TestTrue(TEXT("Second stage at mouth follows head"),mouth.Equals(frame.HeadLocal,.0001));
        TestTrue(TEXT("Removed second stage hangs at torso landmark"),chest.TransformPosition(FDiveBodyFrame::Mouth()).Equals(FVector(18.5,9,135.5),.0001));
        for(int i=0;i<=20;++i)
        {
            const auto pose=frame.RegulatorLocal(i/20.f);
            TestFalse(TEXT("Regulator transition is finite"),pose.ContainsNaN());
            TestTrue(TEXT("Regulator remains rigid throughout transition"),pose.GetScale3D().Equals(FVector::OneVector)&&pose.GetRotation().IsNormalized());
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveMouthWaterline,"DiveSim.Body.MouthWaterline",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveMouthWaterline::RunTest(const FString&)
{
    // Flat surface at Z=0. With eyes 5 cm above water, looking straight ahead
    // leaves the mouth submerged; looking up clears it without moving the eye.
    const FVector eye(0,0,5);
    const auto level=FDiveBodyFrame::PhysicalMouth(eye,FRotator::ZeroRotator);
    const auto up=FDiveBodyFrame::PhysicalMouth(eye,FRotator(85,0,0));
    TestTrue(TEXT("Level mouth is 6.5 cm below the waterline"),FMath::IsNearlyEqual(level.Z,-6.5,.0001));
    TestTrue(TEXT("Upward gaze clears the mouth"),up.Z>7);
    TestTrue(TEXT("Negative pitch wrapping is equivalent"),FDiveBodyFrame::PhysicalMouth(eye,FRotator(275,0,0)).Equals(FDiveBodyFrame::PhysicalMouth(eye,FRotator(-85,0,0)),.0001));
    return true;
}
#endif
