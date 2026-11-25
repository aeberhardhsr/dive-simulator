#include "DiveProfiles.h"
#include "DiveConfigIO.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"

namespace
{
TSharedPtr<FJsonObject> WriteState(const FDiveProfileState& State)
{
    auto obj=MakeShared<FJsonObject>(); obj->SetStringField(TEXT("name"),State.Name);
    obj->SetStringField(TEXT("updated_at_utc"),State.UpdatedUTC);
    auto config=MakeShared<FJsonObject>(); DiveConfigIO::Write(config,State.Config,State.Entry);
    obj->SetObjectField(TEXT("configuration"),config); return obj;
}
bool ReadState(const TSharedPtr<FJsonObject>& Obj,FDiveProfileState& State,FString& Error)
{
    const TSharedPtr<FJsonObject>* config=nullptr; FDateTime date;
    if(!Obj||!Obj->TryGetStringField(TEXT("name"),State.Name)||!FDiveProfiles::ValidName(State.Name)||
       !Obj->TryGetStringField(TEXT("updated_at_utc"),State.UpdatedUTC)||!FDateTime::ParseIso8601(*State.UpdatedUTC,date)||
       !Obj->TryGetObjectField(TEXT("configuration"),config)||!config)
    { Error=TEXT("Invalid profile metadata"); return false; }
    return DiveConfigIO::Read(*config,State.Config,State.Entry,true,Error);
}
}
bool FDiveProfiles::ValidName(const FString& Name)
{
    if(Name.IsEmpty()||Name.Len()>40||Name!=Name.TrimStartAndEnd()) return false;
    for(TCHAR c:Name) if(c<32||c==127||c==0x2028||c==0x2029) return false;
    return true;
}
FString FDiveProfiles::Encode(const TArray<FDiveProfile>& Records)
{
    auto root=MakeShared<FJsonObject>(); root->SetNumberField(TEXT("schema_version"),1);
    TArray<TSharedPtr<FJsonValue>> rows;
    for(const auto& p:Records)
    {
        auto obj=WriteState(p); obj->SetStringField(TEXT("id"),p.Id); obj->SetBoolField(TEXT("archived"),p.Archived);
        if(p.Previous.IsSet()) obj->SetObjectField(TEXT("previous"),WriteState(p.Previous.GetValue()));
        rows.Add(MakeShared<FJsonValueObject>(obj));
    }
    root->SetArrayField(TEXT("profiles"),rows);
    FString json; FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json)); return json;
}
bool FDiveProfiles::Decode(const FString& Json,TArray<FDiveProfile>& Records,FString& Error)
{
    if(Json.Len()>512*1024) { Error=TEXT("Profile file exceeds 512 KiB"); return false; }
    TSharedPtr<FJsonObject> root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),root)||!root)
    { Error=TEXT("Invalid profile JSON"); return false; }
    double version=0; const TArray<TSharedPtr<FJsonValue>>* rows=nullptr;
    if(!root->TryGetNumberField(TEXT("schema_version"),version)||version!=1||!root->TryGetArrayField(TEXT("profiles"),rows)||!rows||rows->Num()>MaxTotal)
    { Error=TEXT("Unsupported profile schema or record count"); return false; }
    TArray<FDiveProfile> parsed; TSet<FString> ids,names; int active=0;
    for(const auto& row:*rows)
    {
        const TSharedPtr<FJsonObject>* obj=nullptr; FDiveProfile p; FGuid guid;
        if(!row||!row->TryGetObject(obj)||!obj||!ReadState(*obj,p,Error)||
            !(*obj)->TryGetStringField(TEXT("id"),p.Id)||!FGuid::ParseExact(p.Id,EGuidFormats::Digits,guid)||
            !(*obj)->TryGetBoolField(TEXT("archived"),p.Archived)||ids.Contains(p.Id.ToLower())||names.Contains(p.Name.ToLower()))
        { if(Error.IsEmpty()) Error=TEXT("Invalid or duplicate profile record"); return false; }
        if(!p.Archived&&++active>MaxActive) { Error=TEXT("Too many active profiles"); return false; }
        if((*obj)->HasField(TEXT("previous")))
        {
            const TSharedPtr<FJsonObject>* previous=nullptr; FDiveProfileState state;
            if(!(*obj)->TryGetObjectField(TEXT("previous"),previous)||!previous||!ReadState(*previous,state,Error)) return false;
            p.Previous=state;
        }
        p.Id=p.Id.ToLower(); ids.Add(p.Id); names.Add(p.Name.ToLower()); parsed.Add(MoveTemp(p));
    }
    Records=MoveTemp(parsed); Error.Empty(); return true;
}
bool FDiveProfiles::Load(const FString& File)
{
    Path=File; Items.Reset(); bLoaded=false; Error=EProfileError::None; Detail.Empty(); StoredJson.Empty();
    if(!IFileManager::Get().FileExists(*Path)) { bLoaded=true; return true; }
    const auto size=IFileManager::Get().FileSize(*Path);
    if(size<0||size>512*1024) { Error=EProfileError::Invalid; Detail=TEXT("Invalid profile file size"); return false; }
    FString json;
    if(!FFileHelper::LoadFileToString(json,*Path)) { Error=EProfileError::Read; return false; }
    if(!Decode(json,Items,Detail)) { Error=EProfileError::Invalid; return false; }
    StoredJson=json; bLoaded=true; return true;
}
bool FDiveProfiles::Save()
{
    if(!bLoaded) return false; // Never overwrite a file we could not read.
    const bool exists=IFileManager::Get().FileExists(*Path);
    FString current;
    if((exists&&(!FFileHelper::LoadFileToString(current,*Path)||current!=StoredJson))||(!exists&&!StoredJson.IsEmpty()))
    { Error=EProfileError::Conflict; return false; }
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
    const FString temp=Path+TEXT(".tmp"),json=Encode(Items);
    TArray<FDiveProfile> verified;
    if(!Decode(json,verified,Detail)) { Error=EProfileError::Invalid; return false; }
    if(!FFileHelper::SaveStringToFile(json,*temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)||!IFileManager::Get().Move(*Path,*temp,true,true))
    { Error=EProfileError::Write; return false; }
    StoredJson=json; Error=EProfileError::None; Detail.Empty(); return true;
}
bool FDiveProfiles::Commit(const TArray<FDiveProfile>& Before)
{ if(Save()) return true; Items=Before; return false; }
const FDiveProfile* FDiveProfiles::Find(const FString& Id) const
{ return Items.FindByPredicate([&](const auto& p){return p.Id==Id;}); }
FDiveProfile* FDiveProfiles::Mutable(const FString& Id)
{
    auto* item=Items.FindByPredicate([&](const auto& p){return p.Id==Id;});
    if(!item) Error=EProfileError::Missing; return item;
}
bool FDiveProfiles::NameAvailable(const FString& Name,const FString& ExceptId)
{
    if(!ValidName(Name)) { Error=EProfileError::Name; return false; }
    if(Items.ContainsByPredicate([&](const auto& p){return p.Id!=ExceptId&&p.Name.Equals(Name,ESearchCase::IgnoreCase);}))
    { Error=EProfileError::Duplicate; return false; } return true;
}
bool FDiveProfiles::Add(const FString& InName,const Dive::Config& Config,int32 Entry,FString& CreatedId)
{
    const FString name=InName.TrimStartAndEnd();
    if(!bLoaded||!NameAvailable(name,FString())) return false;
    if(Items.Num()>=MaxTotal||Items.FilterByPredicate([](const auto& p){return !p.Archived;}).Num()>=MaxActive)
    { Error=EProfileError::Limit; return false; }
    const auto before=Items; FDiveProfile profile;
    profile.Id=FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower(); profile.Name=name; profile.UpdatedUTC=FDateTime::UtcNow().ToIso8601();
    profile.Config=Config; profile.Config.Sanitize(); profile.Entry=FMath::Clamp(Entry,0,2);
    const FString id=profile.Id; Items.Add(profile);
    if(!Commit(before)) return false; CreatedId=id; return true;
}
bool FDiveProfiles::Replace(const FString& Id,const Dive::Config& Config,int32 Entry)
{
    if(!bLoaded) return false;
    const auto before=Items; auto* p=Mutable(Id); if(!p||p->Archived) return false;
    if(DiveConfigIO::Equal(p->Config,p->Entry,Config,Entry)) { Error=EProfileError::None; return true; }
    p->Previous=static_cast<const FDiveProfileState&>(*p); p->Config=Config; p->Config.Sanitize(); p->Entry=FMath::Clamp(Entry,0,2);
    p->UpdatedUTC=FDateTime::UtcNow().ToIso8601(); return Commit(before);
}
bool FDiveProfiles::Rename(const FString& Id,const FString& InName)
{
    const FString name=InName.TrimStartAndEnd(); if(!bLoaded||!NameAvailable(name,Id)) return false;
    const auto before=Items; auto* p=Mutable(Id); if(!p) return false;
    if(p->Name==name) { Error=EProfileError::None; return true; }
    p->Previous=static_cast<const FDiveProfileState&>(*p); p->Name=name;
    p->UpdatedUTC=FDateTime::UtcNow().ToIso8601(); return Commit(before);
}
bool FDiveProfiles::Archive(const FString& Id,bool Archived)
{
    if(!bLoaded) return false;
    if(!Archived&&Items.FilterByPredicate([](const auto& p){return !p.Archived;}).Num()>=MaxActive) { Error=EProfileError::Limit; return false; }
    const auto before=Items; auto* p=Mutable(Id); if(!p) return false;
    p->Archived=Archived; return Commit(before);
}
bool FDiveProfiles::Undo(const FString& Id)
{
    if(!bLoaded) return false;
    const auto before=Items; auto* p=Mutable(Id); if(!p) return false;
    if(!p->Previous.IsSet()) { Error=EProfileError::NoPrevious; return false; }
    if(!NameAvailable(p->Previous->Name,Id)) return false;
    const FDiveProfileState old=*p; const auto restored=p->Previous.GetValue();
    static_cast<FDiveProfileState&>(*p)=restored; p->Previous=old; return Commit(before);
}
