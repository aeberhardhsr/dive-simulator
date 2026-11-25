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
    Dive::Config c; c.GameMode=Dive::Mode::Hardcore; c.CylinderLitres=15; c.StartPressureBar=213.5; c.OxygenFraction=.32;
    c.WeightKg=7.5; c.SurfacePressureBar=.94; c.SurfaceTemperatureC=22; c.BottomTemperatureC=7; c.AirTemperatureC=-3.5;
    c.VisibilityMetres=8; c.CurrentMetresPerSecond=.17; c.PlannedDepthMetres=22.5; c.PlannedMinutes=47;
    c.TorchModel=2; c.Date={2024,2,29}; c.Hour=23+59./60; c.WindMetresPerSecond=8; c.WindDirectionDegrees=240; c.ThermoclineMetres=15;
    c.ThermoclineWidthMetres=3; c.Weather=2; c.DrySuit=false; c.AutoBuoyancy=true; c.StartOnShore=true; return c;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveConfigPersistence,"DiveSim.Profiles.Configuration",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveConfigPersistence::RunTest(const FString&)
{
    for(int flags=0;flags<4;++flags) for(int source=0;source<5;++source)
    {
        auto original=Varied(); original.AutoTemperature=(flags&1)!=0;original.AutoVisibility=(flags&2)!=0;
        original.CurrentSource=static_cast<Dive::CurrentSetting>(source);
        auto obj=MakeShared<FJsonObject>();DiveConfigIO::Write(obj,original,2);
        Dive::Config loaded;int entry=0;FString error;
        TestTrue(TEXT("Environment choices load"),DiveConfigIO::Read(obj,loaded,entry,true,error));
        TestTrue(TEXT("Environment choices and manual values roundtrip"),DiveConfigIO::Equal(original,2,loaded,entry));
        for(const TCHAR* key:{TEXT("auto_temperature"),TEXT("auto_visibility"),TEXT("current_source")})
        {
            DiveConfigIO::Write(obj,original,2);obj->SetStringField(key,TEXT("true"));
            TestFalse(TEXT("Invalid environment field type rejected"),DiveConfigIO::Read(obj,loaded,entry,true,error));
            TestTrue(TEXT("Invalid environment choice is atomic"),DiveConfigIO::Equal(original,2,loaded,entry));
        }
        for(double invalid:{-1.,1.5,5.})
        {
            DiveConfigIO::Write(obj,original,2);obj->SetNumberField(TEXT("current_source"),invalid);
            TestFalse(TEXT("Unknown/fractional current source rejected"),DiveConfigIO::Read(obj,loaded,entry,true,error));
        }
        DiveConfigIO::Write(obj,original,2);
        for(const TCHAR* key:{TEXT("auto_temperature"),TEXT("auto_visibility"),TEXT("current_source")}) obj->RemoveField(key);
        TestTrue(TEXT("Legacy environment loads"),DiveConfigIO::Read(obj,loaded,entry,true,error));
        original.AutoTemperature=original.AutoVisibility=false;original.CurrentSource=Dive::CurrentSetting::Custom;
        TestTrue(TEXT("Legacy remains manual"),DiveConfigIO::Equal(original,2,loaded,entry));
    }
    for(int weather=0;weather<Dive::WeatherScenario::Count;++weather)
    {
        auto scenario=Varied(); scenario.SelectWeather(weather);
        auto stored=MakeShared<FJsonObject>(); DiveConfigIO::Write(stored,scenario,1);
        Dive::Config loaded; int loadedEntry=0; FString message;
        TestTrue(TEXT("All legacy and storm weather values load"),DiveConfigIO::Read(stored,loaded,loadedEntry,true,message));
        TestTrue(TEXT("Weather and its wind roundtrip"),DiveConfigIO::Equal(scenario,1,loaded,loadedEntry));
        scenario.WindMetresPerSecond=21.5; DiveConfigIO::Write(stored,scenario,1);
        TestTrue(TEXT("Custom wind beyond former limit loads"),DiveConfigIO::Read(stored,loaded,loadedEntry,true,message));
        TestEqual(TEXT("Loading preserves explicit wind"),loaded.WindMetresPerSecond,21.5);
        stored->SetNumberField(TEXT("weather"),Dive::WeatherScenario::Count);
        TestFalse(TEXT("Unknown weather rejected"),DiveConfigIO::Read(stored,loaded,loadedEntry,true,message));
    }
    const auto original=Varied(); auto obj=MakeShared<FJsonObject>(); DiveConfigIO::Write(obj,original,2);
    Dive::Config restored; int entry=0; FString error;
    TestTrue(TEXT("Strict complete config loads"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestTrue(TEXT("Every config field and entry roundtrips"),DiveConfigIO::Equal(original,2,restored,entry));
    obj->RemoveField(TEXT("torch_model"));
    TestTrue(TEXT("Legacy torch defaults load"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestEqual(TEXT("Legacy allround torch"),restored.TorchModel,0);
    for(double invalid:{-1.,1.5,3.,999.})
    {
        const auto before=restored;obj->SetNumberField(TEXT("torch_model"),invalid);
        TestFalse(TEXT("Invalid torch is rejected"),DiveConfigIO::Read(obj,restored,entry,true,error));
        TestTrue(TEXT("Invalid torch is atomic"),DiveConfigIO::Equal(before,2,restored,entry));
    }
    DiveConfigIO::Write(obj,original,2);
    obj->RemoveField(TEXT("scenario_date"));
    TestTrue(TEXT("Legacy profile without date loads"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestTrue(TEXT("Legacy summer date defaults"),restored.Date==Dive::CivilDate{});
    for(const TCHAR* invalid:{TEXT("2025-02-29"),TEXT("2026-13-01"),TEXT("2100-01-01"),TEXT("2026-6-1")})
    {
        const auto before=restored;
        obj->SetStringField(TEXT("scenario_date"),invalid);
        TestFalse(TEXT("Invalid scenario date rejected"),DiveConfigIO::Read(obj,restored,entry,true,error));
        TestTrue(TEXT("Invalid date is atomic"),DiveConfigIO::Equal(before,2,restored,entry));
    }
    obj->SetStringField(TEXT("scenario_date"),TEXT("2026-03-29")); obj->SetNumberField(TEXT("hour"),2.5);
    TestFalse(TEXT("Nonexistent spring hour rejected"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestTrue(TEXT("Corrupt settings repair spring gap"),DiveConfigIO::Read(obj,restored,entry,false,error));
    TestEqual(TEXT("Spring gap moves forward one hour"),restored.Hour,3.5);
    DiveConfigIO::Write(obj,original,2);
    obj->RemoveField(TEXT("start_on_shore"));
    TestTrue(TEXT("Legacy profile remains readable"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestFalse(TEXT("Legacy profile retains its offshore start"),restored.StartOnShore);
    obj->RemoveField(TEXT("air_temperature_c"));
    TestTrue(TEXT("Legacy profile without air temperature loads"),DiveConfigIO::Read(obj,restored,entry,true,error));
    TestEqual(TEXT("Legacy air temperature defaults"),restored.AirTemperatureC,20.);
    DiveConfigIO::Write(obj,original,2); DiveConfigIO::Read(obj,restored,entry,true,error);
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
