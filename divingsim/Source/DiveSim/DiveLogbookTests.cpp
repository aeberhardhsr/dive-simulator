#include "DiveLogbook.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
FString Fixture(int32 Seconds=2,int32 Version=1,const FString& Status=TEXT("completed"))
{
    auto root=MakeShared<FJsonObject>();
    root->SetNumberField(TEXT("schema_version"),Version);
    root->SetStringField(TEXT("location"),TEXT("Murg / Sued"));
    root->SetStringField(TEXT("status"),Status);
    root->SetNumberField(TEXT("duration_seconds"),Seconds);
    root->SetNumberField(TEXT("maximum_depth_m"),20);
    root->SetNumberField(TEXT("distance_m"),Seconds);
    root->SetNumberField(TEXT("cylinder_litres"),12);
    root->SetNumberField(TEXT("mode"),1);
    if(Version>=2) root->SetStringField(TEXT("started_at_utc"),TEXT("2026-11-25T12:34:56Z"));
    TArray<TSharedPtr<FJsonValue>> rows;
    for(int i=0;i<=Seconds;++i)
    {
        auto s=MakeShared<FJsonObject>();
        const TCHAR* keys[]={TEXT("seconds"),TEXT("depth_m"),TEXT("cylinder_bar"),TEXT("heart_bpm"),TEXT("respiratory_bpm"),TEXT("spo2_percent"),TEXT("water_c"),TEXT("core_c"),TEXT("stress"),TEXT("fatigue"),TEXT("ppo2_bar"),TEXT("ceiling_m"),TEXT("ndl_minutes"),TEXT("distance_m"),TEXT("warnings")};
        const double values[]={double(i),i==Seconds/2?20.:5.,200-i*.01,72,13,98,14,37,.1,.2,.3,0,99,double(i),i==1?double(Dive::FastAscent):0};
        for(int j=0;j<UE_ARRAY_COUNT(keys);++j) s->SetNumberField(keys[j],values[j]);
        if(Version>=3)
        {
            s->SetNumberField(TEXT("environment_c"),14);
            s->SetNumberField(TEXT("body_immersion"),1);
            s->SetNumberField(TEXT("breathing_source"),i==1?2:0);
            s->SetBoolField(TEXT("regulator_in_mouth"),i!=1);
            s->SetBoolField(TEXT("mouth_submerged"),true);
            if(i==1) s->SetNumberField(TEXT("warnings"),Dive::NoBreathingSupply);
        }
        if(Version>=4) s->SetNumberField(TEXT("breath_reserve"),i==1?.4:1);
        rows.Add(MakeShared<FJsonValueObject>(s));
    }
    root->SetArrayField(TEXT("samples"),rows);
    FString json; FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json)); return json;
}
FString EditFixture(TFunction<void(TSharedPtr<FJsonObject>)> Edit)
{
    TSharedPtr<FJsonObject> root; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture()),root);
    Edit(root); FString json; FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json)); return json;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveLogReadTest,"DiveSim.Logbook.ReadAndHistory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveLogReadTest::RunTest(const FString&)
{
    FDiveLog log; log.Path=TEXT("20261125_145655_test.json");
    TestTrue(TEXT("Legacy schema remains readable"),DiveLogbook::Parse(Fixture(),log));
    TestEqual(TEXT("Legacy date is local without UTC claim"),log.Date,FString(TEXT("2026-11-25 14:56")));
    TestEqual(TEXT("Warning transitions counted"),log.WarningEvents,1);
    TestEqual(TEXT("Warning union"),log.Warnings,uint32(Dive::FastAscent));
    FDiveLog breathing;
    TestTrue(TEXT("Breathing-context schema loads"),DiveLogbook::Parse(Fixture(2,3),breathing));
    TestTrue(TEXT("Schema 3 context retained"),breathing.bBreathingContext);
    TestFalse(TEXT("Schema 3 does not invent a reserve"),breathing.bBreathReserveContext);
    TestTrue(TEXT("No-supply source and ninth warning retained"),breathing.Samples.Num()==3&&
        breathing.Samples[1].BreathSource==Dive::BreathingSource::None&&breathing.Warnings==Dive::NoBreathingSupply);
    FDiveLog reserve;
    TestTrue(TEXT("Schema 4 loads reserve"),DiveLogbook::Parse(Fixture(2,4),reserve));
    TestTrue(TEXT("Schema 4 reserve context retained"),reserve.bBreathReserveContext);
    if(reserve.Samples.Num()==3) TestEqual(TEXT("Recorded reserve preserved"),reserve.Samples[1].BreathReserve,.4);
    TestEqual(TEXT("Sample lookup before start"),DiveLogbook::NearestSample(log.Samples,-5),0);
    TestEqual(TEXT("Sample lookup nearer previous"),DiveLogbook::NearestSample(log.Samples,.1),0);
    TestEqual(TEXT("Sample lookup nearer next"),DiveLogbook::NearestSample(log.Samples,.9),1);
    TestEqual(TEXT("Sample lookup after end"),DiveLogbook::NearestSample(log.Samples,10),2);
    TestTrue(TEXT("Two-hour schema 2 log"),DiveLogbook::Parse(Fixture(7200,2),log));
    TestEqual(TEXT("All two-hour samples retained"),log.Samples.Num(),7201);
    TestEqual(TEXT("Explicit UTC date"),log.Date,FString(TEXT("2026-11-25 12:34 UTC")));
    TestEqual(TEXT("Narrow depth peak retained"),log.Samples[3600].DepthMetres,20.);
    TestEqual(TEXT("Distance retained"),log.Samples.Last().DistanceMetres,7200.);
    const FString empty=EditFixture([](auto root){root->SetArrayField(TEXT("samples"),{});});
    TestTrue(TEXT("Empty profile is valid"),DiveLogbook::Parse(empty,log));
    TestEqual(TEXT("Empty profile lookup"),DiveLogbook::NearestSample(log.Samples,0),INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveLogRejectTest,"DiveSim.Logbook.InvalidData",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveLogRejectTest::RunTest(const FString&)
{
    FDiveLog log;
    TestFalse(TEXT("Truncated file"),DiveLogbook::Parse(Fixture().Left(100),log));
    TestFalse(TEXT("Unknown schema"),DiveLogbook::Parse(Fixture(2,99),log));
    for(int invalid=0;invalid<3;++invalid)
    {
        TSharedPtr<FJsonObject> reserve;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture(2,4)),reserve);
        auto sample=reserve->GetArrayField(TEXT("samples"))[0]->AsObject();
        if(invalid==0) sample->RemoveField(TEXT("breath_reserve"));
        else if(invalid==1) sample->SetNumberField(TEXT("breath_reserve"),1.01);
        else sample->SetStringField(TEXT("breath_reserve"),TEXT("0.5"));
        FString bad; FJsonSerializer::Serialize(reserve,TJsonWriterFactory<>::Create(&bad));
        TestFalse(TEXT("Missing, out-of-range or nonnumeric reserve rejected"),DiveLogbook::Parse(bad,log));
    }
    TSharedPtr<FJsonObject> breathing;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture(2,3)),breathing);
    breathing->GetArrayField(TEXT("samples"))[0]->AsObject()->SetNumberField(TEXT("body_immersion"),1.1);
    FString badBreathing; FJsonSerializer::Serialize(breathing,TJsonWriterFactory<>::Create(&badBreathing));
    TestFalse(TEXT("Invalid body immersion rejected"),DiveLogbook::Parse(badBreathing,log));
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture(2,3)),breathing);
    breathing->GetArrayField(TEXT("samples"))[0]->AsObject()->RemoveField(TEXT("breathing_source"));
    badBreathing.Empty(); FJsonSerializer::Serialize(breathing,TJsonWriterFactory<>::Create(&badBreathing));
    TestFalse(TEXT("Missing breathing context cannot become default data"),DiveLogbook::Parse(badBreathing,log));
    TestFalse(TEXT("Fractional mode"),DiveLogbook::Parse(EditFixture([](auto root){root->SetNumberField(TEXT("mode"),1.5);}),log));
    TestFalse(TEXT("Nonobject sample"),DiveLogbook::Parse(EditFixture([](auto root){root->SetArrayField(TEXT("samples"),{MakeShared<FJsonValueString>(TEXT("bad"))});}),log));
    const TCHAR* keys[]={TEXT("heart_bpm"),TEXT("spo2_percent"),TEXT("warnings"),TEXT("seconds")};
    const double badValues[]={-1,101,256,1000};
    for(int i=0;i<4;++i)
        TestFalse(FString::Printf(TEXT("Out-of-range %s"),keys[i]),DiveLogbook::Parse(EditFixture([&](auto root){root->GetArrayField(TEXT("samples"))[0]->AsObject()->SetNumberField(keys[i],badValues[i]);}),log));
    TestFalse(TEXT("Non-monotonic time"),DiveLogbook::Parse(EditFixture([](auto root){root->GetArrayField(TEXT("samples"))[2]->AsObject()->SetNumberField(TEXT("seconds"),.5);}),log));
    TestFalse(TEXT("Missing metric must not become a default vital sign"),DiveLogbook::Parse(EditFixture([](auto root){root->GetArrayField(TEXT("samples"))[0]->AsObject()->RemoveField(TEXT("heart_bpm"));}),log));
    TestTrue(TEXT("Failure explains unreadable file"),!log.Error.IsEmpty());
    TestEqual(TEXT("Failure clears partial samples"),log.Samples.Num(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveLogIndexTest,"DiveSim.Logbook.DiskIndex",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveLogIndexTest::RunTest(const FString&)
{
    // Isolated test files are retained for inspection; never touch the player's logs.
    const FString dir=FPaths::ProjectSavedDir()/TEXT("Automation")/(TEXT("Logbook_")+FGuid::NewGuid().ToString());
    IFileManager::Get().MakeDirectory(*dir,true);
    TestTrue(TEXT("Write completed legacy log"),FFileHelper::SaveStringToFile(Fixture(),*(dir/TEXT("20261124_120000_old.json"))));
    TestTrue(TEXT("Write interrupted log"),FFileHelper::SaveStringToFile(Fixture(2,2,TEXT("interrupted")),*(dir/TEXT("20261125_120000_new.json"))));
    TestTrue(TEXT("Write automated test log"),FFileHelper::SaveStringToFile(Fixture(2,2,TEXT("smoke_pass")),*(dir/TEXT("20261125_130000_test.json"))));
    TestTrue(TEXT("Write bad log"),FFileHelper::SaveStringToFile(TEXT("{broken"),*(dir/TEXT("bad.json"))));
    TestTrue(TEXT("Write pending atomic save"),FFileHelper::SaveStringToFile(TEXT("{pending"),*(dir/TEXT("pending.json.tmp"))));
    const auto index=DiveLogbook::Scan(dir);
    TestEqual(TEXT("Test runs hidden in player logbook"),index.Entries.Num(),2);
    TestEqual(TEXT("Bad log reported, temp file ignored"),index.Unreadable,1);
    if(index.Entries.Num()==2)
    {
        TestEqual(TEXT("Newest first"),index.Entries[0].Status,FString(TEXT("interrupted")));
        TestEqual(TEXT("Index does not retain full profiles"),index.Entries[0].Samples.Num(),0);
        const auto detail=DiveLogbook::Read(index.Entries[0].Path);
        TestEqual(TEXT("Selected detail read from disk"),detail.Samples.Num(),3);
    }
    TestEqual(TEXT("QA can include its own runs"),DiveLogbook::Scan(dir,true).Entries.Num(),3);
    TestTrue(TEXT("Missing directory is empty"),DiveLogbook::Scan(dir/TEXT("missing")).Entries.IsEmpty());
    return true;
}
#endif
