#include "DiveLogbook.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

namespace DiveLogbook
{
bool Parse(const FString& Json,FDiveLog& Log)
{
    const FString path=Log.Path; Log=FDiveLog{}; Log.Path=path;
    auto fail=[&](const TCHAR* message) { Log.Error=message; Log.Samples.Reset(); return false; };
    TSharedPtr<FJsonObject> root;
    if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),root)||!root)
        return fail(TEXT("Invalid JSON"));
    double version=0;
    if(!root->TryGetNumberField(TEXT("schema_version"),version)||(version!=1&&version!=2))
        return fail(TEXT("Unsupported log version"));
    auto number=[](const TSharedPtr<FJsonObject>& obj,const TCHAR* key,double& value,double lo,double hi)
    { return obj->TryGetNumberField(key,value)&&FMath::IsFinite(value)&&value>=lo&&value<=hi; };
    double mode=0;
    if(!root->TryGetStringField(TEXT("location"),Log.Location)||Log.Location.Len()>100||
       !root->TryGetStringField(TEXT("status"),Log.Status)||Log.Status.Len()>80||
       !number(root,TEXT("duration_seconds"),Log.Duration,0,604800)||
       !number(root,TEXT("maximum_depth_m"),Log.MaxDepth,0,1000)||
       !number(root,TEXT("distance_m"),Log.Distance,0,1e8)||
       !number(root,TEXT("cylinder_litres"),Log.CylinderLitres,1,100)||
       !number(root,TEXT("mode"),mode,0,2)||mode!=FMath::FloorToDouble(mode))
        return fail(TEXT("Invalid log summary"));
    Log.Mode=static_cast<int32>(mode);
    FString started;
    FDateTime utc;
    if(root->TryGetStringField(TEXT("started_at_utc"),started)&&FDateTime::ParseIso8601(*started,utc))
        Log.Date=utc.ToString(TEXT("%Y-%m-%d %H:%M"))+TEXT(" UTC");
    else
    {
        const FString name=FPaths::GetBaseFilename(Log.Path);
        // Legacy filenames used local wall-clock time, not UTC.
        if(name.Len()>=15&&name.Left(8).IsNumeric()&&name.Mid(9,6).IsNumeric())
            Log.Date=name.Mid(0,4)+TEXT("-")+name.Mid(4,2)+TEXT("-")+name.Mid(6,2)+TEXT(" ")+name.Mid(9,2)+TEXT(":")+name.Mid(11,2);
        else Log.Date=TEXT("--");
    }
    root->TryGetBoolField(TEXT("is_test"),Log.bTest);
    Log.bTest|=Log.Status.StartsWith(TEXT("smoke_"));
    const TArray<TSharedPtr<FJsonValue>>* rows=nullptr;
    if(!root->TryGetArrayField(TEXT("samples"),rows)||rows->Num()>100000)
        return fail(TEXT("Invalid sample array"));
    Log.Samples.Reserve(rows->Num());
    double lastTime=-1, lastDistance=0;
    uint32 previousWarnings=0;
    for(const auto& row:*rows)
    {
        const TSharedPtr<FJsonObject>* object=nullptr;
        if(!row||!row->TryGetObject(object)||!object||!object->IsValid()) return fail(TEXT("Invalid sample"));
        const auto& o=*object;
        Dive::State s;
        double warnings=0;
        if(!number(o,TEXT("seconds"),s.ElapsedSeconds,0,Log.Duration+.05)||s.ElapsedSeconds<lastTime||
           !number(o,TEXT("depth_m"),s.DepthMetres,0,1000)||s.DepthMetres>Log.MaxDepth+.05||
           !number(o,TEXT("cylinder_bar"),s.CylinderBar,0,400)||
           !number(o,TEXT("heart_bpm"),s.HeartBPM,0,300)||
           !number(o,TEXT("respiratory_bpm"),s.RespiratoryBPM,0,150)||
           !number(o,TEXT("spo2_percent"),s.SpO2,0,100)||
           !number(o,TEXT("water_c"),s.WaterTemperatureC,-10,60)||
           !number(o,TEXT("core_c"),s.CoreTemperatureC,0,60)||
           !number(o,TEXT("stress"),s.Stress,0,1)||
           !number(o,TEXT("fatigue"),s.Fatigue,0,1)||
           !number(o,TEXT("ppo2_bar"),s.PPO2,0,30)||
           !number(o,TEXT("ceiling_m"),s.CeilingMetres,0,1000)||
           !number(o,TEXT("ndl_minutes"),s.NDLMinutes,0,1000)||
           !number(o,TEXT("distance_m"),s.DistanceMetres,0,Log.Distance+.05)||s.DistanceMetres<lastDistance||
           !number(o,TEXT("warnings"),warnings,0,255)||warnings!=FMath::FloorToDouble(warnings))
            return fail(TEXT("Invalid or unordered sample values"));
        s.Warnings=static_cast<uint32>(warnings);
        if(s.Warnings&~previousWarnings) ++Log.WarningEvents;
        previousWarnings=s.Warnings; Log.Warnings|=s.Warnings;
        lastTime=s.ElapsedSeconds; lastDistance=s.DistanceMetres;
        Log.Samples.Add(s);
    }
    return true;
}

FDiveLog Read(const FString& Path)
{
    FDiveLog log; log.Path=Path;
    const int64 bytes=IFileManager::Get().FileSize(*Path);
    if(bytes<0||bytes>64*1024*1024) { log.Error=TEXT("Log unavailable or larger than 64 MB"); return log; }
    FString json;
    if(!FFileHelper::LoadFileToString(json,*Path)) log.Error=TEXT("Cannot read log");
    else Parse(json,log);
    return log;
}

FDiveLogIndex Scan(const FString& Directory,bool IncludeTests)
{
    FDiveLogIndex index;
    TArray<FString> names;
    IFileManager::Get().FindFiles(names,*(Directory/TEXT("*.json")),true,false);
    names.Sort([](const FString& a,const FString& b){return a>b;});
    for(const auto& name:names)
    {
        FDiveLog log=Read(Directory/name);
        if(!log.Error.IsEmpty()) { ++index.Unreadable; continue; }
        if(log.bTest&&!IncludeTests) continue;
        log.Samples.Empty(); // Index stores metadata only; one selected profile is loaded on demand.
        index.Entries.Add(MoveTemp(log));
    }
    return index;
}

int32 NearestSample(const TArray<Dive::State>& Samples,double Seconds)
{
    if(Samples.IsEmpty()) return INDEX_NONE;
    int32 lo=0,hi=Samples.Num()-1;
    while(lo<hi) { const int32 mid=(lo+hi)/2; if(Samples[mid].ElapsedSeconds<Seconds) lo=mid+1; else hi=mid; }
    if(lo>0&&FMath::Abs(Samples[lo-1].ElapsedSeconds-Seconds)<FMath::Abs(Samples[lo].ElapsedSeconds-Seconds)) --lo;
    return lo;
}
}
