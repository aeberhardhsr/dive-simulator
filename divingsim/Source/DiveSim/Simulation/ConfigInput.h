#pragma once
#include "DiveModel.h"
#include <string>
#include <string_view>
#include <cstdio>

namespace Dive
{
// User-facing numeric inputs. Parsing never clamps or changes a value on error.
struct ConfigInput
{
    const char* Id;
    double Config::* Member;
    double Minimum,Maximum;
    int Decimals;
    const char* Unit;
    bool Clock=false;
};
inline constexpr ConfigInput ConfigInputs[]={
    {"hour",&Config::Hour,0,23+59./60,0,"",true},
    {"pressure",&Config::StartPressureBar,1,300,1,"bar"},
    {"weight",&Config::WeightKg,0,20,1,"kg"},
    {"visibility",&Config::VisibilityMetres,2,40,1,"m"},
    {"current",&Config::CurrentMetresPerSecond,0,.5,2,"m/s"},
    {"depth",&Config::PlannedDepthMetres,3,60,1,"m"},
    {"duration",&Config::PlannedMinutes,1,120,1,"min"},
    {"wind",&Config::WindMetresPerSecond,0,12,1,"m/s"},
    {"winddir",&Config::WindDirectionDegrees,0,359.9,1,"deg"},
    {"surface_temp",&Config::SurfaceTemperatureC,1,30,1,"C"},
    {"air_temp",&Config::AirTemperatureC,-10,40,1,"C"},
    {"bottom_temp",&Config::BottomTemperatureC,1,25,1,"C"},
    {"thermocline",&Config::ThermoclineMetres,3,25,1,"m"},
    {"thermal_width",&Config::ThermoclineWidthMetres,.5,5,1,"m"}
};
inline const ConfigInput* FindConfigInput(std::string_view id)
{
    for(const auto& input:ConfigInputs) if(id==input.Id) return &input;
    return nullptr;
}
enum class ConfigInputError { None,Format,Precision,Range };
inline ConfigInputError ParseConfigInput(const ConfigInput& input,std::string_view text,double& value)
{
    const auto space=[](char c){return c==' '||c=='\t'||c=='\r'||c=='\n';};
    while(!text.empty()&&space(text.front())) text.remove_prefix(1);
    while(!text.empty()&&space(text.back())) text.remove_suffix(1);
    if(text.empty()||text.size()>32) return ConfigInputError::Format;
    double parsed=0;
    if(input.Clock)
    {
        const auto colon=text.find(':');
        if((colon!=1&&colon!=2)||text.size()!=colon+3) return ConfigInputError::Format;
        int hours=0,minutes=0;
        for(size_t i=0;i<text.size();++i)
        {
            if(i==colon) continue;
            if(text[i]<'0'||text[i]>'9') return ConfigInputError::Format;
            auto& part=i<colon?hours:minutes; part=part*10+(text[i]-'0');
        }
        if(hours>23||minutes>59) return ConfigInputError::Range;
        parsed=hours+minutes/60.;
    }
    else
    {
        bool negative=false,decimal=false; int digits=0,places=0;
        if(text.front()=='-'||text.front()=='+') { negative=text.front()=='-'; text.remove_prefix(1); }
        for(char c:text)
        {
            if(c=='.'||c==',') { if(decimal) return ConfigInputError::Format; decimal=true; continue; }
            if(c<'0'||c>'9') return ConfigInputError::Format;
            parsed=parsed*10+(c-'0'); ++digits; if(decimal) ++places;
        }
        if(!digits) return ConfigInputError::Format;
        if(places>input.Decimals) return ConfigInputError::Precision;
        parsed/=std::pow(10.,places); if(negative) parsed=-parsed;
    }
    if(!std::isfinite(parsed)||parsed<input.Minimum||parsed>input.Maximum) return ConfigInputError::Range;
    value=parsed; return ConfigInputError::None;
}
inline std::string FormatConfigInput(const ConfigInput& input,double value)
{
    char buffer[64];
    if(input.Clock)
    {
        const int minute=static_cast<int>(std::round(Clamp(value,0,23+59./60)*60));
        std::snprintf(buffer,sizeof(buffer),"%02d:%02d",minute/60,minute%60);
    }
    else
    {
        std::snprintf(buffer,sizeof(buffer),"%.*f",input.Decimals,value);
        // Compact displayed values while preserving the field's accepted precision.
        std::string number(buffer);
        if(input.Decimals) { while(number.back()=='0') number.pop_back(); if(number.back()=='.'||number.back()==',') number.pop_back(); }
        return number;
    }
    return buffer;
}
}
