#include "DiveWaypoints.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

bool FDiveWaypoints::ValidName(const FString& Name)
{
    if(Name.IsEmpty()||Name.Len()>32||Name!=Name.TrimStartAndEnd()) return false;
    for(TCHAR c:Name) if(c<32||c==127||c==0x2028||c==0x2029) return false;
    return true;
}
const FDiveWaypoint* FDiveWaypoints::Find(const FString& Id) const
{ return Items.FindByPredicate([&](const auto& p){return p.Id==Id;}); }
FString FDiveWaypoints::Encode(const TArray<FDiveWaypoint>& Records,const FString& Selected)
{
    auto root=MakeShared<FJsonObject>();root->SetNumberField(TEXT("schema_version"),1);
    root->SetStringField(TEXT("area"),TEXT("walensee_lv95"));root->SetStringField(TEXT("target"),Selected);
    TArray<TSharedPtr<FJsonValue>> rows;
    for(const auto& p:Records)
    {
        auto row=MakeShared<FJsonObject>();row->SetStringField(TEXT("id"),p.Id);row->SetStringField(TEXT("name"),p.Name);
        row->SetNumberField(TEXT("east"),p.East);row->SetNumberField(TEXT("north"),p.North);row->SetNumberField(TEXT("depth"),p.Depth);
        row->SetBoolField(TEXT("from_map"),p.FromMap);rows.Add(MakeShared<FJsonValueObject>(row));
    }
    root->SetArrayField(TEXT("waypoints"),rows);FString json;
    FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json));return json;
}
bool FDiveWaypoints::Decode(const FString& Json,TArray<FDiveWaypoint>& Records,FString& Selected)
{
    if(Json.Len()>65536) return false;
    TSharedPtr<FJsonObject> root;const TArray<TSharedPtr<FJsonValue>>* rows=nullptr;
    double version=0;FString area,target;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),root)||!root||
        !root->TryGetNumberField(TEXT("schema_version"),version)||version!=1||
        !root->TryGetStringField(TEXT("area"),area)||area!=TEXT("walensee_lv95")||
        !root->TryGetStringField(TEXT("target"),target)||
        !root->TryGetArrayField(TEXT("waypoints"),rows)||rows->Num()>Limit) return false;
    TArray<FDiveWaypoint> parsed;TSet<FString> ids,names;
    for(const auto& value:*rows)
    {
        const TSharedPtr<FJsonObject>* row=nullptr;FDiveWaypoint p;FGuid guid;
        if(!value||!value->TryGetObject(row)||!row||!(*row)||
            !(*row)->TryGetStringField(TEXT("id"),p.Id)||!FGuid::ParseExact(p.Id,EGuidFormats::Digits,guid)||
            !(*row)->TryGetStringField(TEXT("name"),p.Name)||!ValidName(p.Name)||
            !(*row)->TryGetNumberField(TEXT("east"),p.East)||!(*row)->TryGetNumberField(TEXT("north"),p.North)||
            !(*row)->TryGetNumberField(TEXT("depth"),p.Depth)||!(*row)->TryGetBoolField(TEXT("from_map"),p.FromMap)||
            !FMath::IsFinite(p.East)||!FMath::IsFinite(p.North)||!FMath::IsFinite(p.Depth)||
            p.East<2726000||p.East>2742000||p.North<1219000||p.North>1223000||p.Depth<0||p.Depth>200||
            ids.Contains(p.Id.ToLower())||names.Contains(p.Name.ToLower())) return false;
        p.Id=p.Id.ToLower();ids.Add(p.Id);names.Add(p.Name.ToLower());parsed.Add(p);
    }
    target=target.ToLower();if(!target.IsEmpty()&&!ids.Contains(target)) return false;
    Records=MoveTemp(parsed);Selected=target;return true;
}
bool FDiveWaypoints::Load(const FString& File)
{
    Path=File;Loaded=false;Items.Empty();Target.Empty();Stored.Empty();Error.Empty();
    if(!IFileManager::Get().FileExists(*Path)) { Loaded=true;return true; }
    if(IFileManager::Get().FileSize(*Path)>65536||!FFileHelper::LoadFileToString(Stored,*Path)||!Decode(Stored,Items,Target))
    { Error=TEXT("read");return false; }
    Loaded=true;return true;
}
bool FDiveWaypoints::Save(const TArray<FDiveWaypoint>& Before,const FString& BeforeTarget)
{
    const FString json=Encode(Items,Target);TArray<FDiveWaypoint> check;FString target,current;
    Error.Empty();
    if(!Loaded||!Decode(json,check,target)) Error=TEXT("invalid");
    else
    {
        const bool exists=IFileManager::Get().FileExists(*Path);
        if((exists&&(!FFileHelper::LoadFileToString(current,*Path)||current!=Stored))||(!exists&&!Stored.IsEmpty())) Error=TEXT("conflict");
        else
        {
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path),true);
            const FString temp=Path+TEXT(".")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".tmp");
            if(!FFileHelper::SaveStringToFile(json,*temp,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)||
                !IFileManager::Get().Move(*Path,*temp,true,true))
            { Error=TEXT("write");IFileManager::Get().Delete(*temp); }
            else { Stored=json;return true; }
        }
    }
    Items=Before;Target=BeforeTarget;return false;
}
bool FDiveWaypoints::Add(const FString& InName,double East,double North,double Depth,bool FromMap,FString& Id)
{
    const FString name=InName.TrimStartAndEnd();Id.Empty();
    if(!Loaded) return false;
    if(!FMath::IsFinite(East)||!FMath::IsFinite(North)||!FMath::IsFinite(Depth)||
        East<2726000||East>2742000||North<1219000||North>1223000||Depth<0||Depth>200)
    { Error=TEXT("invalid");return false; }
    if(!ValidName(name)) { Error=TEXT("name");return false; }
    if(Items.ContainsByPredicate([&](const auto& p){return p.Name.Equals(name,ESearchCase::IgnoreCase);})) { Error=TEXT("duplicate");return false; }
    if(Items.Num()>=Limit) { Error=TEXT("limit");return false; }
    const auto before=Items;const auto target=Target;
    FDiveWaypoint point{FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower(),name,East,North,Depth,FromMap};
    Items.Add(point);if(!Save(before,target)) return false;Id=point.Id;return true;
}
bool FDiveWaypoints::Rename(const FString& Id,const FString& InName)
{
    const FString name=InName.TrimStartAndEnd();if(!Loaded) return false;
    if(!ValidName(name)) { Error=TEXT("name");return false; }
    if(Items.ContainsByPredicate([&](const auto& p){return p.Id!=Id&&p.Name.Equals(name,ESearchCase::IgnoreCase);})) { Error=TEXT("duplicate");return false; }
    const auto before=Items;auto* point=Items.FindByPredicate([&](const auto& p){return p.Id==Id;});
    if(!point) { Error=TEXT("missing");return false; }point->Name=name;return Save(before,Target);
}
bool FDiveWaypoints::Remove(const FString& Id)
{
    if(!Loaded||!Find(Id)) return false;const auto before=Items;const FString target=Target;
    Items.RemoveAll([&](const auto& p){return p.Id==Id;});if(Target==Id) Target.Empty();return Save(before,target);
}
bool FDiveWaypoints::Select(const FString& Id)
{
    if(!Loaded||(!Id.IsEmpty()&&!Find(Id))) return false;const FString before=Target;Target=Id;return Save(Items,before);
}
