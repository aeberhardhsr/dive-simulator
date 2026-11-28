#include "DiveDiscoveries.h"
#include "DiveRecording.h"
#include "DiveLogbook.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDivePhotoCodec,"DiveSim.Discoveries.PhotographValidation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDivePhotoCodec::RunTest(const FString&)
{
    FDivePhotograph photo;photo.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);photo.UTC=TEXT("2026-11-26T12:00:00Z");photo.Seconds=2;photo.Depth=5;photo.Species={TEXT("perch"),TEXT("stonewort")};
    auto root=MakeShared<FJsonObject>();TArray<FDivePhotograph> read;
    DiveDiscoveries::EncodePhotos(root,{photo});
    TestTrue(TEXT("Roundtrip"),DiveDiscoveries::DecodePhotos(root,read,3));
    TestTrue(TEXT("Identity and species preserved"),read.Num()==1&&read[0].Id==photo.Id&&read[0].Species==photo.Species);
    TestFalse(TEXT("Beyond dive duration"),DiveDiscoveries::DecodePhotos(root,read,1));
    TestTrue(TEXT("Failure leaves no partial collection"),read.IsEmpty());
    for(const FString bad:{FString(TEXT("../other")),FString(TEXT("C:/x.png")),FString(TEXT("0000000000000000000000000000000g"))})
    { TestFalse(TEXT("Reject unsafe photo identity"),DiveDiscoveries::ValidPhotoId(bad));TestTrue(TEXT("Unsafe identity has no resolved path"),DiveDiscoveries::PhotoPath(bad).IsEmpty()); }
    DiveDiscoveries::EncodePhotos(root,{photo,photo});TestFalse(TEXT("Duplicate photo"),DiveDiscoveries::DecodePhotos(root,read,3));
    auto bad=photo;bad.Species.Add(TEXT("perch"));DiveDiscoveries::EncodePhotos(root,{bad});TestFalse(TEXT("Duplicate species"),DiveDiscoveries::DecodePhotos(root,read,3));
    bad=photo;bad.Species={TEXT("invented")};DiveDiscoveries::EncodePhotos(root,{bad});TestFalse(TEXT("Unknown species"),DiveDiscoveries::DecodePhotos(root,read,3));
    bad=photo;bad.UTC=TEXT("bad-date");DiveDiscoveries::EncodePhotos(root,{bad});TestFalse(TEXT("Invalid UTC"),DiveDiscoveries::DecodePhotos(root,read,3));
    bad=photo;bad.Seconds=1;bad.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);DiveDiscoveries::EncodePhotos(root,{photo,bad});TestFalse(TEXT("Unordered capture times"),DiveDiscoveries::DecodePhotos(root,read,3));
    DiveDiscoveries::EncodePhotos(root,{});TestTrue(TEXT("No discoveries is valid"),DiveDiscoveries::DecodePhotos(root,read,0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveDiscoveryCamera,"DiveSim.Discoveries.CameraAndCatalog",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveDiscoveryCamera::RunTest(const FString&)
{
    const FVector forward(1,0,0);
    TestTrue(TEXT("Near visible size"),DiveDiscoveries::InFrame(FVector(2,0,0),forward,.1,14));
    TestFalse(TEXT("Behind camera"),DiveDiscoveries::InFrame(FVector(-2,0,0),forward,.1,14));
    TestFalse(TEXT("Outside centre cone"),DiveDiscoveries::InFrame(FVector(2,1,0),forward,.1,14));
    TestFalse(TEXT("Too small"),DiveDiscoveries::InFrame(FVector(8,0,0),forward,.1,14));
    TestFalse(TEXT("Low visibility"),DiveDiscoveries::InFrame(FVector(3,0,0),forward,.5,2));
    TestFalse(TEXT("Inside object"),DiveDiscoveries::InFrame(FVector(.1,0,0),forward,.5,14));
    TArray<FDiveSpecies> species;TestTrue(TEXT("Packaged catalog readable"),DiveDiscoveries::LoadCatalog(species));
    TestEqual(TEXT("Eight modeled identities"),species.Num(),8);
    if(species.Num()==8)
    {
        TestEqual(TEXT("Whitefish remains group"),species[3].Scientific,FString(TEXT("Coregonus spp.")));
        TestEqual(TEXT("Stonewort remains family"),species[6].Scientific,FString(TEXT("Characeae")));
        for(int i=0;i<8;++i) TestEqual(TEXT("Stable identity matches population"),DiveDiscoveries::SpeciesIndex(species[i].Id),i);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDivePhotoRecording,"DiveSim.Discoveries.LogAssociation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDivePhotoRecording::RunTest(const FString&)
{
    FDiveRecording recording;recording.Path=FPaths::ProjectSavedDir()/TEXT("Automation")/(FGuid::NewGuid().ToString()+TEXT(".json"));
    recording.Location=TEXT("Murg");recording.StartedUTC=TEXT("2026-11-26T12:00:00Z");recording.Status=TEXT("completed");recording.bTest=true;
    Dive::Model model;model.Reset(recording.Config);model.Step(1,3,0,0,0,0,false);recording.State=model.Data;recording.Samples.Add({model.Data,FVector::ZeroVector});
    FDivePhotograph photo;photo.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits);photo.UTC=recording.StartedUTC;photo.Seconds=.5;photo.Depth=3;photo.Species={TEXT("perch")};recording.Photographs.Add(photo);
    TestTrue(TEXT("Owned snapshot writes with photo"),DiveLogbook::WriteRecording(recording).bSuccess);
    auto log=DiveLogbook::Read(recording.Path);TestTrue(TEXT("Photo associated with correct dive"),log.Error.IsEmpty()&&log.Location==TEXT("Murg")&&log.Photographs.Num()==1&&log.Photographs[0].Id==photo.Id);
    FString json;FFileHelper::LoadFileToString(json,*recording.Path);TSharedPtr<FJsonObject> root;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),root);
    if(root)
    {
        root->RemoveField(TEXT("photographs"));json.Empty();FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json));
        TestFalse(TEXT("Schema 7 needs photograph array"),DiveLogbook::Parse(json,log));
        root->SetNumberField(TEXT("schema_version"),6);json.Empty();FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json));
        TestTrue(TEXT("Schema 6 remains readable with no photos"),DiveLogbook::Parse(json,log)&&log.Photographs.IsEmpty());
    }
    IFileManager::Get().Delete(*recording.Path);return true;
}
#endif
