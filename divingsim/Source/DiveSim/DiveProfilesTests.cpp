#include "DiveProfiles.h"
#include "DiveConfigIO.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"

namespace
{
struct FProfileFixture
{
    FString File=FPaths::ProjectSavedDir()/TEXT("ProfileTests")/(FGuid::NewGuid().ToString()+TEXT(".json"));
    ~FProfileFixture() { IFileManager::Get().Delete(*File); IFileManager::Get().Delete(*(File+TEXT(".tmp"))); }
};
Dive::Config Varied()
{
    Dive::Config c; c.GameMode=Dive::Mode::Hardcore; c.CylinderLitres=15; c.StartPressureBar=250; c.OxygenFraction=.32;
    c.WeightKg=9; c.SurfacePressureBar=.94; c.SurfaceTemperatureC=22; c.BottomTemperatureC=7;
    c.VisibilityMetres=8; c.CurrentMetresPerSecond=.2; c.PlannedDepthMetres=24; c.PlannedMinutes=55;
    c.Hour=23; c.WindMetresPerSecond=8; c.WindDirectionDegrees=240; c.ThermoclineMetres=15;
    c.ThermoclineWidthMetres=3; c.Weather=2; c.DrySuit=false; c.AutoBuoyancy=true; return c;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveConfigPersistence,"DiveSim.Profiles.Configuration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveConfigPersistence::RunTest(const FString&)
{
    const auto original=Varied(); auto obj=MakeShared<FJsonObject>(); DiveConfigIO::Write(obj,original,2);
    Dive::Config restored; int entry=0; FString error;
    TestTrue(TEXT("Strict complete config loads"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestTrue(TEXT("Every config field and entry roundtrips"),DiveConfigIO::Equal(original,2,restored,entry));
    for(const TCHAR* key:{TEXT("weather"),TEXT("entry"),TEXT("mode")})
    {
        DiveConfigIO::Write(obj,original,2); obj->SetNumberField(key,1.5);
        TestFalse(TEXT("Fractional enums/entry rejected"),DiveConfigIO::Read(obj,restored,entry,true,error));
    }
    DiveConfigIO::Write(obj,original,2); obj->SetNumberField(TEXT("cylinder_litres"),99);
    TestFalse(TEXT("Out of range profile rejected"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestTrue(TEXT("Failure leaves destination unchanged"),DiveConfigIO::Equal(original,2,restored,entry));
    TestTrue(TEXT("Legacy settings clamp"),DiveConfigIO::Read(obj,restored,entry,false,error));
    TestEqual(TEXT("Tank clamps to max"),restored.CylinderLitres,30.);
    obj=MakeShared<FJsonObject>(); obj->SetStringField(TEXT("wind_mps"),TEXT("storm"));
    TestTrue(TEXT("Legacy missing fields use defaults"),DiveConfigIO::Read(obj,restored,entry,false,error));
    TestTrue(TEXT("Legacy defaults complete"),DiveConfigIO::Equal(restored,entry,Dive::Config{},0));
    TestFalse(TEXT("Profile requires complete fields"),DiveConfigIO::Read(obj,restored,entry,true,error));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveProfilePersistence,"DiveSim.Profiles.PersistenceAndRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveProfilePersistence::RunTest(const FString&)
{
    FProfileFixture f; FDiveProfiles store; FString id,other;
    TestTrue(TEXT("New store"),store.Load(f.File));
    TestTrue(TEXT("Unicode name saved"),store.Add(TEXT("  Nacht – Mürg 日本  "),Varied(),2,id));
    TestEqual(TEXT("Trim display name"),store.Find(id)->Name,FString(TEXT("Nacht – Mürg 日本")));
    TestTrue(TEXT("Replace config"),store.Replace(id,Dive::Config{},0));
    FDiveProfiles reload; TestTrue(TEXT("Reload from disk"),reload.Load(f.File));
    TestTrue(TEXT("Undo survives restart"),reload.Undo(id));
    TestTrue(TEXT("All old fields restored"),DiveConfigIO::Equal(reload.Find(id)->Config,reload.Find(id)->Entry,Varied(),2));
    TestTrue(TEXT("Rename"),reload.Rename(id,TEXT("Walensee")));
    TestTrue(TEXT("Undo rename"),reload.Undo(id));
    TestEqual(TEXT("Original name restored"),reload.Find(id)->Name,FString(TEXT("Nacht – Mürg 日本")));
    TestTrue(TEXT("Archive"),reload.Archive(id,true));
    TestTrue(TEXT("Reload archive"),store.Load(f.File));
    TestTrue(TEXT("Archived retained"),store.Find(id)->Archived);
    TestFalse(TEXT("Archived names remain reserved"),store.Add(TEXT("nacht – mürg 日本"),Dive::Config{},0,other));
    TestTrue(TEXT("Restore"),store.Archive(id,false));
    TestEqual(TEXT("Stable identity"),store.Items.Num(),1);
    TestFalse(TEXT("Newlines rejected"),store.Rename(id,TEXT("Night\nDive")));
    TestFalse(TEXT("Long names rejected"),store.Rename(id,FString::ChrN(41,TEXT('x'))));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveProfileProtection,"DiveSim.Profiles.FileProtectionAndLimits",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveProfileProtection::RunTest(const FString&)
{
    FProfileFixture f; FDiveProfiles a,b; FString id,other;
    a.Load(f.File); a.Add(TEXT("First"),Dive::Config{},0,id); b.Load(f.File);
    TestTrue(TEXT("First writer succeeds"),a.Rename(id,TEXT("Changed")));
    TestFalse(TEXT("Stale writer rejected"),b.Rename(id,TEXT("Lost update")));
    TestTrue(TEXT("Conflict classified"),b.Error==EProfileError::Conflict);
    TestEqual(TEXT("Memory rolls back"),b.Find(id)->Name,FString(TEXT("First")));
    b.Load(f.File); TestEqual(TEXT("Disk keeps newer version"),b.Find(id)->Name,FString(TEXT("Changed")));
    for(int i=1;i<32;++i) TestTrue(TEXT("Fill active slots"),b.Add(FString::Printf(TEXT("Profile %d"),i),Dive::Config{},0,other));
    TestFalse(TEXT("Active count limited"),b.Add(TEXT("Too many"),Dive::Config{},0,other));
    TestTrue(TEXT("Archive frees active slot"),b.Archive(id,true));
    TestTrue(TEXT("Use freed slot"),b.Add(TEXT("Replacement"),Dive::Config{},0,other));
    TestFalse(TEXT("Restore respects limit"),b.Archive(id,false));
    TestTrue(TEXT("Failed restore preserved archive"),b.Find(id)->Archived);
    FString corrupt=TEXT("{broken json"); FFileHelper::SaveStringToFile(corrupt,*f.File);
    TestFalse(TEXT("Corrupt file load fails"),a.Load(f.File));
    TestFalse(TEXT("Cannot replace corrupt file"),a.Add(TEXT("Overwrite"),Dive::Config{},0,other));
    FString content; FFileHelper::LoadFileToString(content,*f.File);
    TestEqual(TEXT("Corrupt original remains untouched"),content,corrupt);
    FDiveProfiles badPath; badPath.Load(f.File/TEXT("child.json")); // Existing file cannot become a directory.
    TestFalse(TEXT("Unwritable destination rejected"),badPath.Add(TEXT("No disk"),Dive::Config{},0,other));
    TestTrue(TEXT("Write failure classified"),badPath.Error==EProfileError::Write);
    TestEqual(TEXT("Write failure rolls back memory"),badPath.Items.Num(),0);
    return true;
}
#endif
