#include "DiveLogbook.h"
#include "DiveRecording.h"
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
    if(Version==6)
    { root->SetStringField(TEXT("loadout_model"),TEXT("freshwater_mass_balance_v1"));root->SetBoolField(TEXT("assisted_buoyancy"),false); }
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
        if(Version>=5)
        {
            s->SetNumberField(TEXT("narcosis"),0.3);
            s->SetNumberField(TEXT("panic"),0.3);
            s->SetNumberField(TEXT("cold_strain"),0.3);
            s->SetNumberField(TEXT("decompression_burden"),0.3);
            s->SetNumberField(TEXT("motor_capacity"),0.6);
            s->SetNumberField(TEXT("visual_impairment"),0.3);
            s->SetNumberField(TEXT("regulator_temperature_c"),0.3);
            s->SetNumberField(TEXT("freeze_exposure"),0.3);
            s->SetNumberField(TEXT("freeflow_local_lps"),6);
            s->SetNumberField(TEXT("freeflow_lost_litres"),100);
            s->SetBoolField(TEXT("regulator_freeflow"),true);
            s->SetNumberField(TEXT("warnings"),Dive::EquipmentFailure|Dive::NarcosisWarning);
        }
        if(Version==6)
        {
            const double gas=(200-i*.01)*12*.0012,mass=80+34.12+gas+.0096;
            s->SetNumberField(TEXT("equipment_dry_kg"),34.12);s->SetNumberField(TEXT("cylinder_gas_kg"),gas);
            s->SetNumberField(TEXT("bcd_gas_kg"),.0096);s->SetNumberField(TEXT("total_mass_kg"),mass);
            s->SetNumberField(TEXT("displaced_litres"),117);s->SetNumberField(TEXT("suit_litres"),7.5);
            s->SetNumberField(TEXT("bcd_surface_litres"),8);s->SetNumberField(TEXT("bcd_litres"),8);
            s->SetNumberField(TEXT("ambient_bar"),1);s->SetNumberField(TEXT("buoyancy_newtons"),(117-mass)*9.80665);
        }
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
    FDiveLog loadout;
    TestTrue(TEXT("Schema 6 retains loadout"),DiveLogbook::Parse(Fixture(2,6),loadout));
    TestTrue(TEXT("Mass and volume survive readback"),loadout.bLoadoutContext&&loadout.Samples.Num()==3&&
        loadout.Samples[0].EquipmentDryKg==34.12&&loadout.Samples[0].DisplacedLitres==117);
    FDiveLog effects;
    TestTrue(TEXT("Schema 5 retains impairments"),DiveLogbook::Parse(Fixture(2,5),effects));
    TestTrue(TEXT("Fault, effects and warning bits roundtrip"),effects.bImpairmentContext&&effects.Samples.Num()==3&&
        effects.Samples[1].RegulatorFreeflow&&effects.Samples[1].MotorCapacity==.6&&effects.Samples[1].FreeflowLostLitres==100&&
        effects.Warnings==(Dive::EquipmentFailure|Dive::NarcosisWarning));
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
    for(const TCHAR* key:{TEXT("total_mass_kg"),TEXT("bcd_litres"),TEXT("cylinder_gas_kg"),TEXT("buoyancy_newtons")})
    {
        TSharedPtr<FJsonObject> root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture(2,6)),root);
        root->GetArrayField(TEXT("samples"))[0]->AsObject()->SetNumberField(key,100);
        FString bad;FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&bad));
        TestFalse(TEXT("Physically inconsistent loadout sample rejected"),DiveLogbook::Parse(bad,log));
        root->GetArrayField(TEXT("samples"))[0]->AsObject()->RemoveField(key);bad.Empty();
        FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&bad));
        TestFalse(TEXT("Missing loadout sample rejected"),DiveLogbook::Parse(bad,log));
    }
    for(const TCHAR* key:{TEXT("motor_capacity"),TEXT("regulator_freeflow"),TEXT("freeflow_lost_litres"),TEXT("narcosis")})
    {
        TSharedPtr<FJsonObject> root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Fixture(2,5)),root);
        root->GetArrayField(TEXT("samples"))[0]->AsObject()->RemoveField(key);
        FString bad;FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&bad));
        TestFalse(TEXT("Schema 5 cannot invent missing effect data"),DiveLogbook::Parse(bad,log));
    }
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
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveLogRecoveryTest,"DiveSim.Logbook.CheckpointRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveLogRecoveryTest::RunTest(const FString&)
{
    const FString dir=FPaths::ProjectSavedDir()/TEXT("Automation")/(TEXT("Recovery_")+FGuid::NewGuid().ToString());
    const FString path=dir/TEXT("session.json");
    FString error;
    TestTrue(TEXT("First checkpoint written"),DiveLogbook::Write(path,Fixture(30,4,TEXT("in_progress")),error));
    TestTrue(TEXT("Second checkpoint written"),DiveLogbook::Write(path,Fixture(60,4,TEXT("in_progress")),error));
    TestEqual(TEXT("Newest checkpoint preferred"),DiveLogbook::Read(path).Duration,60.);
    TestFalse(TEXT("Healthy primary is not recovery"),DiveLogbook::Read(path).bRecovered);
    TestEqual(TEXT("Previous checkpoint retained"),DiveLogbook::Read(path+TEXT(".bak")).Duration,30.);
    TestEqual(TEXT("Backup does not duplicate index entry"),DiveLogbook::Scan(dir).Entries.Num(),1);
    TestFalse(TEXT("Malformed replacement rejected"),DiveLogbook::Write(path,TEXT("{truncated"),error));
    TestTrue(TEXT("Write failure explained"),!error.IsEmpty());
    TestEqual(TEXT("Rejected input leaves primary intact"),DiveLogbook::Read(path).Duration,60.);
    TestTrue(TEXT("Simulate torn primary"),FFileHelper::SaveStringToFile(TEXT("{torn"),*path));
    const auto recovered=DiveLogbook::Read(path);
    TestTrue(TEXT("Damaged primary falls back"),recovered.Error.IsEmpty()&&recovered.bRecovered);
    TestEqual(TEXT("Recovery exposes actual older duration"),recovered.Duration,30.);
    TestEqual(TEXT("Recovery keeps canonical selection path"),recovered.Path,path);
    TestTrue(TEXT("New write repairs damaged primary"),DiveLogbook::Write(path,Fixture(90,4,TEXT("in_progress")),error));
    TestEqual(TEXT("Damaged primary never overwrites healthy backup"),DiveLogbook::Read(path+TEXT(".bak")).Duration,30.);
    TestTrue(TEXT("Simulate interruption after primary removal"),IFileManager::Get().Move(*(path+TEXT(".removed")),*path));
    const auto missing=DiveLogbook::Scan(dir);
    TestEqual(TEXT("Backup-only session remains discoverable"),missing.Entries.Num(),1);
    if(missing.Entries.Num()==1) TestTrue(TEXT("Backup-only entry marked recovered"),missing.Entries[0].bRecovered);
    TestTrue(TEXT("Restore healthy primary"),DiveLogbook::Write(path,Fixture(90,4,TEXT("in_progress")),error));
    // A directory blocks the next backup staging write without modifying player files.
    TestTrue(TEXT("Create blocked backup stage"),IFileManager::Get().MakeDirectory(*(path+TEXT(".bak.tmp")),true));
    TestFalse(TEXT("Cannot lose primary when backup cannot be staged"),DiveLogbook::Write(path,Fixture(120,4,TEXT("completed")),error));
    TestEqual(TEXT("Failed backup retains newest primary"),DiveLogbook::Read(path).Duration,90.);
    TestEqual(TEXT("Uncommitted temporary file never becomes an index entry"),DiveLogbook::Scan(dir).Entries.Num(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveLogCostTest,"DiveSim.Logbook.LongCheckpointCost",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveLogCostTest::RunTest(const FString&)
{
    const FString path=FPaths::ProjectSavedDir()/TEXT("Automation")/(TEXT("LongLog_")+FGuid::NewGuid().ToString())/TEXT("session.json");
    const FString json=Fixture(7200,4,TEXT("in_progress"));
    FString error;
    TestTrue(TEXT("Initial two-hour checkpoint"),DiveLogbook::Write(path,json,error));
    const double started=FPlatformTime::Seconds();
    TestTrue(TEXT("Replace two-hour checkpoint with backup"),DiveLogbook::Write(path,json,error));
    const double milliseconds=(FPlatformTime::Seconds()-started)*1000;
    TestEqual(TEXT("All long-session samples preserved"),DiveLogbook::Read(path).Samples.Num(),7201);
    UE_LOG(LogTemp,Display,TEXT("DIVE_CHECKPOINT_COST samples=7201 json_chars=%d write_ms=%.3f"),json.Len(),milliseconds);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveLogAsyncTest,"DiveSim.Logbook.BackgroundWriter",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveLogAsyncTest::RunTest(const FString&)
{
    FDiveRecording recording;
    recording.Path=FPaths::ProjectSavedDir()/TEXT("Automation")/(TEXT("AsyncLog_")+FGuid::NewGuid().ToString())/TEXT("session.json");
    recording.StartedUTC=FDateTime::UtcNow().ToIso8601(); recording.Location=TEXT("Weesen / West");
    recording.Status=TEXT("in_progress"); recording.bTest=true;
    recording.SurveyOrigin=FVector2D(2726000,1219000);
    // A repeated valid state isolates storage cost; this is not a live dive.
    Dive::Model fixtureModel;fixtureModel.Step(.02,5,0,0,0,0,false);
    for(int i=0;i<=7200;++i)
    {
        auto s=fixtureModel.Data; s.ElapsedSeconds=i; s.DepthMetres=5; s.MaxDepthMetres=5;
        s.DistanceMetres=i; recording.Samples.Add({s,FVector(i*100,i*50,-500)});
    }
    recording.State=recording.Samples.Last().State;
    FDiveRecordingWriter writer;
    auto wait=[&]()
    {
        TOptional<FDiveWriteResult> result;
        const double deadline=FPlatformTime::Seconds()+15;
        while(!result.IsSet()&&FPlatformTime::Seconds()<deadline)
        { result=writer.Poll(); if(!result.IsSet()) FPlatformProcess::Sleep(.001f); }
        TestTrue(TEXT("Worker returns within deadline"),result.IsSet());
        return result;
    };
    const double started=FPlatformTime::Seconds();
    writer.Submit(recording); // Includes copying all 7201 samples on the caller.
    const double submitMilliseconds=(FPlatformTime::Seconds()-started)*1000;
    recording.Samples.Last().State.HeartBPM=117;
    auto first=wait();
    TestTrue(TEXT("Background write succeeds"),first.IsSet()&&first->bSuccess);
    auto saved=DiveLogbook::Read(recording.Path);
    TestEqual(TEXT("Snapshot owns all samples"),saved.Samples.Num(),7201);
    if(!saved.Samples.IsEmpty()) TestEqual(TEXT("Later actor changes cannot mutate queued snapshot"),saved.Samples.Last().HeartBPM,fixtureModel.Data.HeartBPM);
    FString json; FFileHelper::LoadFileToString(json,*recording.Path);
    TSharedPtr<FJsonObject> root; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),root);
    if(root.IsValid())
    {
        const auto last=root->GetArrayField(TEXT("samples")).Last()->AsObject();
        TestEqual(TEXT("East survey coordinate preserved"),last->GetNumberField(TEXT("lv95_east_m")),2729600.);
        TestEqual(TEXT("North survey coordinate preserved"),last->GetNumberField(TEXT("lv95_north_m")),1226200.);
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_LOG_ASYNC_COST samples=7201 submit_ms=%.3f worker_ms=%.3f"),submitMilliseconds,first.IsSet()?first->Milliseconds:-1);
    // An active write plus two submissions must retain just the newest pending
    // snapshot. Finish then discards it and waits before writing the final state.
    writer.Submit(recording);
    recording.Samples.Last().State.HeartBPM=116; writer.Submit(recording);
    recording.Samples.Last().State.HeartBPM=115; writer.Submit(recording);
    TestEqual(TEXT("Queue bounded to one pending snapshot"),writer.PendingCount(),1);
    recording.Status=TEXT("completed"); recording.Samples.Last().State.HeartBPM=114;
    TestTrue(TEXT("Final write completes after earlier worker"),writer.Finish(recording).bSuccess);
    TestEqual(TEXT("No obsolete pending write remains"),writer.PendingCount(),0);
    TestFalse(TEXT("No late worker result can replace final write"),writer.Poll().IsSet());
    saved=DiveLogbook::Read(recording.Path);
    TestEqual(TEXT("Final status wins"),saved.Status,FString(TEXT("completed")));
    if(!saved.Samples.IsEmpty()) TestEqual(TEXT("Final samples win"),saved.Samples.Last().HeartBPM,114.);
    recording.Status=TEXT("in_progress"); recording.Samples.Last().State.SpO2=101;
    AddExpectedMessagePlain(TEXT("DIVE_LOG_SAVE_FAILED ")+recording.Path+TEXT(": Invalid new log: Invalid or unordered sample values"),ELogVerbosity::Warning);
    writer.Submit(recording); const auto failed=wait();
    TestTrue(TEXT("Worker failure reaches caller"),failed.IsSet()&&!failed->bSuccess&&!failed->Error.IsEmpty());
    TestEqual(TEXT("Failed worker preserves completed log"),DiveLogbook::Read(recording.Path).Status,FString(TEXT("completed")));
    recording.Samples.Last().State.SpO2=98;
    recording.Path=FPaths::GetPath(recording.Path)/TEXT("next-session.json");
    writer.Submit(recording); const auto next=wait();
    TestTrue(TEXT("Writer can start another session after failure"),next.IsSet()&&next->bSuccess);
    return true;
}
#endif
