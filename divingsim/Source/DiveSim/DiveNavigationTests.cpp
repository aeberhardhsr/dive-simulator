#include "DiveTerrain.h"
#include "Simulation/Navigation.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveNavigationCoordinates,"DiveSim.Navigation.SurveyAndCompass",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveNavigationCoordinates::RunTest(const FString&)
{
    FDiveTerrain terrain;
    const FVector origin=terrain.ToWorld(terrain.OriginEast,terrain.OriginNorth);
    TestTrue(TEXT("Survey north is world +X"),(terrain.ToWorld(terrain.OriginEast,terrain.OriginNorth+1)-origin).Equals(FVector(100,0,0),1e-6));
    TestTrue(TEXT("Survey east is world +Y"),(terrain.ToWorld(terrain.OriginEast+1,terrain.OriginNorth)-origin).Equals(FVector(0,100,0),1e-6));
    for(double bearing:{0.,45.,90.,180.,270.,359.8})
    {
        const FVector delta=FRotator(0,bearing,0).Vector()*100000;
        const auto survey=terrain.ToSurvey(origin+delta);
        const double north=survey.Y-terrain.OriginNorth,east=survey.X-terrain.OriginEast;
        TestTrue(TEXT("A kilometre on the map is a kilometre in the world"),FMath::Abs(FVector2D(north,east).Size()-1000)<1e-6);
        TestEqual(TEXT("View yaw, LV95 grid bearing and compass agree"),Dive::Navigation::DisplayHeading(Dive::Navigation::Bearing(north,east)),Dive::Navigation::DisplayHeading(bearing));
    }
    return true;
}
#endif
