#include "DiveDiscoveries.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"

namespace DiveDiscoveries
{
const TCHAR* SpeciesId(int Index)
{
    static const TCHAR* ids[]={TEXT("perch"),TEXT("roach"),TEXT("pike"),TEXT("whitefish"),TEXT("burbot"),TEXT("pondweed"),TEXT("stonewort"),TEXT("milfoil")};
    return Index>=0&&Index<8?ids[Index]:TEXT("");
}
int SpeciesIndex(const FString& Id)
{ for(int i=0;i<8;++i) if(Id==SpeciesId(i)) return i;return INDEX_NONE; }
bool ValidPhotoId(const FString& Id)
{
    if(Id.Len()!=32) return false;
    for(TCHAR c:Id) if(!((c>='0'&&c<='9')||(c>='A'&&c<='F')||(c>='a'&&c<='f'))) return false;
    return true;
}
FString PhotoPath(const FString& Id,bool Test)
{ return ValidPhotoId(Id)?FPaths::ProjectSavedDir()/(Test?TEXT("PhotographsTest"):TEXT("Photographs"))/(Id+TEXT(".png")):FString(); }
bool DecodePhotos(const TSharedPtr<FJsonObject>& Root,TArray<FDivePhotograph>& Out,double Duration)
{
    Out.Reset();const TArray<TSharedPtr<FJsonValue>>* rows=nullptr;
    if(!Root||!Root->TryGetArrayField(TEXT("photographs"),rows)||rows->Num()>PhotoLimit) return false;
    TArray<FDivePhotograph> result;TSet<FString> ids;double previous=-1;
    auto number=[](const TSharedPtr<FJsonObject>& o,const TCHAR* key,double& value,double max)
    { const auto f=o->TryGetField(key);return f&&f->Type==EJson::Number&&f->TryGetNumber(value)&&FMath::IsFinite(value)&&value>=0&&value<=max; };
    for(const auto& row:*rows)
    {
        const TSharedPtr<FJsonObject>* object=nullptr;
        if(!row||!row->TryGetObject(object)||!object||!object->IsValid()) return false;
        const auto& o=*object;FDivePhotograph p;FDateTime date;
        const TArray<TSharedPtr<FJsonValue>>* species=nullptr;
        if(!o->TryGetStringField(TEXT("id"),p.Id)||!ValidPhotoId(p.Id)||ids.Contains(p.Id.ToLower())||
            !o->TryGetStringField(TEXT("utc"),p.UTC)||p.UTC.Len()>40||!FDateTime::ParseIso8601(*p.UTC,date)||
            !number(o,TEXT("seconds"),p.Seconds,Duration+.05)||p.Seconds<previous||!number(o,TEXT("depth_m"),p.Depth,1000)||
            !o->TryGetArrayField(TEXT("species"),species)||species->Num()>8) return false;
        for(const auto& s:*species)
        { FString id;if(!s||s->Type!=EJson::String||!s->TryGetString(id)||SpeciesIndex(id)==INDEX_NONE||p.Species.Contains(id)) return false;p.Species.Add(id); }
        ids.Add(p.Id.ToLower());previous=p.Seconds;result.Add(MoveTemp(p));
    }
    Out=MoveTemp(result);return true;
}
void EncodePhotos(const TSharedPtr<FJsonObject>& Root,const TArray<FDivePhotograph>& Photos)
{
    TArray<TSharedPtr<FJsonValue>> rows;
    for(const auto& p:Photos)
    {
        auto o=MakeShared<FJsonObject>();o->SetStringField(TEXT("id"),p.Id);o->SetStringField(TEXT("utc"),p.UTC);
        o->SetNumberField(TEXT("seconds"),p.Seconds);o->SetNumberField(TEXT("depth_m"),p.Depth);
        TArray<TSharedPtr<FJsonValue>> species;for(const auto& id:p.Species) species.Add(MakeShared<FJsonValueString>(id));
        o->SetArrayField(TEXT("species"),species);rows.Add(MakeShared<FJsonValueObject>(o));
    }
    Root->SetArrayField(TEXT("photographs"),rows);
}
bool LoadCatalog(TArray<FDiveSpecies>& Out)
{
    Out.Reset();FString json;TSharedPtr<FJsonObject> root;
    if(!FFileHelper::LoadFileToString(json,*(FPaths::ProjectContentDir()/TEXT("Diving/Data/Species.json")))||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),root)||!root) return false;
    double version=0;const TArray<TSharedPtr<FJsonValue>>* rows=nullptr;
    if(!root->TryGetNumberField(TEXT("schema_version"),version)||version!=1||!root->TryGetArrayField(TEXT("species"),rows)||rows->Num()!=8) return false;
    TArray<FDiveSpecies> result;result.SetNum(8);TSet<FString> ids;
    for(const auto& row:*rows)
    {
        const TSharedPtr<FJsonObject>* object=nullptr;if(!row||!row->TryGetObject(object)||!object||!object->IsValid()) return false;
        const auto& o=*object;FDiveSpecies s;
        if(!o->TryGetStringField(TEXT("id"),s.Id)||SpeciesIndex(s.Id)==INDEX_NONE||ids.Contains(s.Id)||
            !o->TryGetStringField(TEXT("de"),s.NameDE)||!o->TryGetStringField(TEXT("en"),s.NameEN)||
            !o->TryGetStringField(TEXT("scientific"),s.Scientific)||!o->TryGetStringField(TEXT("description_de"),s.DescriptionDE)||
            !o->TryGetStringField(TEXT("description_en"),s.DescriptionEN)||!o->TryGetStringField(TEXT("source"),s.Source)) return false;
        if(s.NameDE.Len()>40||s.NameEN.Len()>40||s.Scientific.Len()>60||s.DescriptionDE.Len()>240||s.DescriptionEN.Len()>240||!s.Source.StartsWith(TEXT("https://"))) return false;
        ids.Add(s.Id);result[SpeciesIndex(s.Id)]=MoveTemp(s);
    }
    Out=MoveTemp(result);return true;
}
bool InFrame(const FVector& Offset,const FVector& Forward,double Radius,double Visibility)
{
    const double distance=Offset.Size();
    return FMath::IsFinite(distance)&&FMath::IsFinite(Radius)&&FMath::IsFinite(Visibility)&&distance>.25&&
        distance<=FMath::Min(12.,Visibility*.6)&&Radius/distance>=.025&&
        FVector::DotProduct(Offset/distance,Forward.GetSafeNormal())>=.97;
}
}
