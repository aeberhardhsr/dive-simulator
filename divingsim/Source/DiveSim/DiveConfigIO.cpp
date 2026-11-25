#include "DiveConfigIO.h"
#include "Dom/JsonObject.h"

namespace
{
struct FNumber { const TCHAR* Name; double Dive::Config::*Member; };
const FNumber Numbers[]={
    {TEXT("cylinder_litres"),&Dive::Config::CylinderLitres},{TEXT("pressure_bar"),&Dive::Config::StartPressureBar},
    {TEXT("oxygen_fraction"),&Dive::Config::OxygenFraction},{TEXT("weight_kg"),&Dive::Config::WeightKg},
    {TEXT("surface_pressure_bar"),&Dive::Config::SurfacePressureBar},{TEXT("surface_temperature_c"),&Dive::Config::SurfaceTemperatureC},
    {TEXT("air_temperature_c"),&Dive::Config::AirTemperatureC},
    {TEXT("bottom_temperature_c"),&Dive::Config::BottomTemperatureC},{TEXT("visibility_m"),&Dive::Config::VisibilityMetres},
    {TEXT("current_mps"),&Dive::Config::CurrentMetresPerSecond},{TEXT("planned_depth_m"),&Dive::Config::PlannedDepthMetres},
    {TEXT("planned_minutes"),&Dive::Config::PlannedMinutes},{TEXT("hour"),&Dive::Config::Hour},
    {TEXT("wind_mps"),&Dive::Config::WindMetresPerSecond},{TEXT("wind_direction_degrees"),&Dive::Config::WindDirectionDegrees},
    {TEXT("thermocline_m"),&Dive::Config::ThermoclineMetres},{TEXT("thermocline_width_m"),&Dive::Config::ThermoclineWidthMetres}
};
}
void DiveConfigIO::Write(const TSharedPtr<FJsonObject>& Obj,const Dive::Config& Config,int32 Entry)
{
    Obj->SetStringField(TEXT("scenario_date"),UTF8_TO_TCHAR(Config.Date.Text().c_str()));
    Obj->SetNumberField(TEXT("torch_model"),Config.TorchModel);
    for(const auto& n:Numbers) Obj->SetNumberField(n.Name,Config.*(n.Member));
    Obj->SetNumberField(TEXT("weather"),Config.Weather); Obj->SetNumberField(TEXT("mode"),static_cast<int>(Config.GameMode));
    Obj->SetNumberField(TEXT("entry"),Entry); Obj->SetBoolField(TEXT("dry_suit"),Config.DrySuit);
    Obj->SetBoolField(TEXT("auto_buoyancy"),Config.AutoBuoyancy);
    Obj->SetBoolField(TEXT("start_on_shore"),Config.StartOnShore);
}
bool DiveConfigIO::Read(const TSharedPtr<FJsonObject>& Obj,Dive::Config& Config,int32& Entry,bool Strict,FString& Error)
{
    if(!Obj) { Error=TEXT("Missing configuration"); return false; }
    Dive::Config candidate; int32 candidateEntry=0;
    for(const auto& n:Numbers)
    {
        const auto value=Obj->TryGetField(n.Name); double number=0;
        const bool valid=value&&value->Type==EJson::Number&&value->TryGetNumber(number)&&FMath::IsFinite(number);
        if(valid) candidate.*(n.Member)=number;
        else if(Strict&&!(FString(n.Name)==TEXT("air_temperature_c")&&!value))
        { Error=FString(TEXT("Missing or invalid "))+n.Name; return false; }
    }
    auto integer=[&](const TCHAR* Name,int32& Target,int32 Max)
    {
        const auto value=Obj->TryGetField(Name); double number=0;
        const bool valid=value&&value->Type==EJson::Number&&value->TryGetNumber(number)&&FMath::IsFinite(number);
        if(Strict&&(!valid||number<0||number>Max||std::floor(number)!=number))
        { Error=FString(TEXT("Missing or invalid "))+Name; return false; }
        if(valid) Target=static_cast<int32>(FMath::Clamp(number,0.,static_cast<double>(Max)));
        return true;
    };
    int32 mode=1;
    if(Obj->HasField(TEXT("torch_model"))&&!integer(TEXT("torch_model"),candidate.TorchModel,Dive::Torch::Count-1)) return false;
    if(!integer(TEXT("mode"),mode,2)||!integer(TEXT("weather"),candidate.Weather,Dive::WeatherScenario::Count-1)||!integer(TEXT("entry"),candidateEntry,2)) return false;
    candidate.GameMode=static_cast<Dive::Mode>(mode);
    auto boolean=[&](const TCHAR* Name,bool& Target)
    {
        const auto value=Obj->TryGetField(Name);
        if(value&&value->Type==EJson::Boolean) { Target=value->AsBool(); return true; }
        if(Strict) { Error=FString(TEXT("Missing or invalid "))+Name; return false; } return true;
    };
    if(!boolean(TEXT("dry_suit"),candidate.DrySuit)||!boolean(TEXT("auto_buoyancy"),candidate.AutoBuoyancy)) return false;
    // Additive field: profiles/settings from before shore access start in water.
    if(Obj->HasField(TEXT("start_on_shore"))&&!boolean(TEXT("start_on_shore"),candidate.StartOnShore)) return false;
    // Additive date: old profiles retain the previous summer scenario.
    if(const auto value=Obj->TryGetField(TEXT("scenario_date")))
    {
        FString date;
        if(!value->TryGetString(date)||!Dive::CivilDate::Parse(TCHAR_TO_UTF8(*date),candidate.Date))
        {
            if(Strict) { Error=TEXT("Invalid scenario_date (YYYY-MM-DD, 2000-2099)"); return false; }
            candidate.Date=Dive::CivilDate{};
        }
    }
    auto clean=candidate; clean.Sanitize();
    if(Strict)
        for(const auto& n:Numbers) if(candidate.*(n.Member)!=clean.*(n.Member))
        { Error=FString(TEXT("Out of range: "))+n.Name; return false; }
    Config=clean; Entry=candidateEntry; Error.Empty(); return true;
}
bool DiveConfigIO::Equal(const Dive::Config& A,int32 EntryA,const Dive::Config& B,int32 EntryB)
{
    for(const auto& n:Numbers) if(A.*(n.Member)!=B.*(n.Member)) return false;
    return A.TorchModel==B.TorchModel&&A.Date==B.Date&&EntryA==EntryB&&A.GameMode==B.GameMode&&A.Weather==B.Weather&&A.DrySuit==B.DrySuit&&A.AutoBuoyancy==B.AutoBuoyancy&&A.StartOnShore==B.StartOnShore;
}
