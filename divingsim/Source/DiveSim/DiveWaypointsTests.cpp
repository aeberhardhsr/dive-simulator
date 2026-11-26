#include "DiveWaypoints.h"
#include "DiveControls.h"
#include <limits>
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveWaypointPersistence,"DiveSim.Waypoints.PersistenceAndConflict",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveWaypointPersistence::RunTest(const FString&)
{
    const FString path=FPaths::ProjectSavedDir()/TEXT("Automation")/(FGuid::NewGuid().ToString()+TEXT(".json"));
    FDiveWaypoints first;FString id,second;
    TestTrue(TEXT("Missing file starts an empty collection"),first.Load(path));
    TestTrue(TEXT("Save Unicode name and exact surveyed position"),first.Add(TEXT("Felswand Süd"),2730123.456,1220123.789,18.45,false,id));
    TestTrue(TEXT("Save target"),first.Select(id));
    FDiveWaypoints restored;TestTrue(TEXT("Reload"),restored.Load(path));
    TestEqual(TEXT("Target survives restart"),restored.Target,id);
    TestEqual(TEXT("Position retains submetre precision"),restored.Find(id)->East,2730123.456);
    TestEqual(TEXT("Depth survives"),restored.Find(id)->Depth,18.45);
    TestTrue(TEXT("Rename persisted"),restored.Rename(id,TEXT("Wand")));
    TestFalse(TEXT("Stale writer cannot overwrite newer name"),first.Rename(id,TEXT("Veraltet")));
    TestEqual(TEXT("Conflict rolls memory back"),first.Find(id)->Name,FString(TEXT("Felswand Süd")));
    TestEqual(TEXT("Conflict reported"),first.Error,FString(TEXT("conflict")));
    TestTrue(TEXT("Reload after conflict"),first.Load(path));
    TestFalse(TEXT("Duplicate names rejected"),first.Add(TEXT("wand"),2730200,1220100,10,true,second));
    TestTrue(TEXT("Second unique waypoint"),first.Add(TEXT("Kartenpunkt"),2730200,1220100,10,true,second));
    TestTrue(TEXT("Remove active target"),first.Remove(id));
    TestTrue(TEXT("Removed target is cleared"),first.Target.IsEmpty());
    TestTrue(TEXT("Remaining point persists"),restored.Load(path)&&restored.Items.Num()==1&&restored.Items[0].FromMap);
    for(int i=1;i<FDiveWaypoints::Limit;++i)
        TestTrue(TEXT("Capacity slot can be saved"),first.Add(FString::Printf(TEXT("Punkt %02d"),i),2730200+i,1220100,10,true,second));
    TestFalse(TEXT("Thirty-third point is rejected without losing records"),first.Add(TEXT("Overflow"),2730000,1220000,10,false,second));
    TestTrue(TEXT("All 32 points reload"),restored.Load(path)&&restored.Items.Num()==FDiveWaypoints::Limit);
    FString original;FFileHelper::LoadFileToString(original,*path);
    TestFalse(TEXT("Non-finite coordinates are rejected"),first.Add(TEXT("NaN"),std::numeric_limits<double>::quiet_NaN(),1220000,10,false,id));
    FString current;FFileHelper::LoadFileToString(current,*path);TestEqual(TEXT("Invalid change leaves disk unchanged"),current,original);
    FFileHelper::SaveStringToFile(TEXT("{broken"),*path);
    TestFalse(TEXT("Unreadable collection is not editable"),first.Load(path));
    TestFalse(TEXT("Cannot overwrite unreadable collection"),first.Add(TEXT("Test"),2730000,1220000,10,false,id));
    FFileHelper::LoadFileToString(current,*path);TestEqual(TEXT("Broken source is preserved"),current,FString(TEXT("{broken")));
    IFileManager::Get().Delete(*path);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveWaypointValidation,"DiveSim.Waypoints.SchemaBoundsAndLegacyBinding",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveWaypointValidation::RunTest(const FString&)
{
    FDiveWaypoint point{FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower(),TEXT("Fels"),2730000,1220000,14,false};
    TArray<FDiveWaypoint> output;FString target;
    TestTrue(TEXT("Roundtrip"),FDiveWaypoints::Decode(FDiveWaypoints::Encode({point},point.Id),output,target));
    TestFalse(TEXT("Dangling target rejected"),FDiveWaypoints::Decode(FDiveWaypoints::Encode({point},TEXT("missing")),output,target));
    TestFalse(TEXT("Duplicate ids rejected"),FDiveWaypoints::Decode(FDiveWaypoints::Encode({point,point},point.Id),output,target));
    for(double east:{2725999.,2742001.})
    { auto invalid=point;invalid.East=east;TestFalse(TEXT("Outside map rejected"),FDiveWaypoints::Decode(FDiveWaypoints::Encode({invalid},FString()),output,target)); }
    for(double depth:{-1.,201.})
    { auto invalid=point;invalid.Depth=depth;TestFalse(TEXT("Impossible depth rejected"),FDiveWaypoints::Decode(FDiveWaypoints::Encode({invalid},FString()),output,target)); }
    TestFalse(TEXT("Unknown future schema preserved for newer app"),FDiveWaypoints::Decode(FDiveWaypoints::Encode({point},point.Id).Replace(TEXT("\"schema_version\": 1"),TEXT("\"schema_version\": 2")),output,target));
    TestFalse(TEXT("Line break rejected"),FDiveWaypoints::ValidName(TEXT("a\nb")));
    TestFalse(TEXT("Whitespace rejected"),FDiveWaypoints::ValidName(TEXT("  ")));
    FDiveControls controls;auto old=MakeShared<FJsonObject>();auto bindings=MakeShared<FJsonObject>();
    bindings->SetStringField(TEXT("lamp"),TEXT("V"));old->SetObjectField(TEXT("bindings"),bindings);controls.Read(old);
    TestEqual(TEXT("Existing V binding preserved"),controls.Key(EDiveAction::Lamp),EKeys::V);
    TestTrue(TEXT("New waypoint action receives a usable nonconflicting key"),controls.Key(EDiveAction::Waypoints)!=EKeys::V&&FDiveControls::Allowed(controls.Key(EDiveAction::Waypoints)));
    return true;
}
#endif
