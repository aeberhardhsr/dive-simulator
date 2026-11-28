#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

namespace Dive
{
struct CivilDate
{
    int Year=2026,Month=6,Day=21;
    static bool Leap(int year) { return year%4==0&&(year%100!=0||year%400==0); }
    static int DaysInMonth(int year,int month)
    {
        constexpr int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
        return month>=1&&month<=12?days[month-1]+(month==2&&Leap(year)?1:0):0;
    }
    bool Valid() const { return Year>=1&&Year<=9999&&Month>=1&&Month<=12&&Day>=1&&Day<=DaysInMonth(Year,Month); }
    bool ScenarioRange() const { return Valid()&&Year>=2000&&Year<=2099; }
    static int BeforeYear(int year) { const int y=year-1; return 365*y+y/4-y/100+y/400; }
    int SerialDay() const
    {
        int result=BeforeYear(Year)-BeforeYear(1970)+Day-1;
        for(int m=1;m<Month;++m) result+=DaysInMonth(Year,m);
        return result;
    }
    static CivilDate FromSerialDay(int serial)
    {
        int day=std::clamp(serial+BeforeYear(1970),0,BeforeYear(10000)-1);
        int year=1+day/366;
        while(BeforeYear(year+1)<=day) ++year;
        day-=BeforeYear(year);
        int month=1;
        while(day>=DaysInMonth(year,month)) { day-=DaysInMonth(year,month); ++month; }
        return {year,month,day+1};
    }
    int Weekday() const { const int value=(SerialDay()+4)%7; return value<0?value+7:value; } // Sunday=0.
    std::string Text() const
    {
        char text[16]; std::snprintf(text,sizeof(text),"%04d-%02d-%02d",Year,Month,Day); return text;
    }
    static bool Parse(std::string_view text,CivilDate& result)
    {
        while(!text.empty()&&(text.front()==' '||text.front()=='\t')) text.remove_prefix(1);
        while(!text.empty()&&(text.back()==' '||text.back()=='\t')) text.remove_suffix(1);
        if(text.size()!=10||text[4]!='-'||text[7]!='-') return false;
        CivilDate date{0,0,0};
        for(size_t i=0;i<text.size();++i)
        {
            if(i==4||i==7) continue;
            if(text[i]<'0'||text[i]>'9') return false;
            int& part=i<4?date.Year:i<7?date.Month:date.Day;
            part=part*10+text[i]-'0';
        }
        if(!date.ScenarioRange()) return false;
        result=date; return true;
    }
    bool operator==(const CivilDate& other) const { return Year==other.Year&&Month==other.Month&&Day==other.Day; }
    bool operator!=(const CivilDate& other) const { return !(*this==other); }
};

// Scenario dates 2000-2099 use the current Swiss CET/CEST rule, not the host
// computer's zone. Future changes of legislation are not predicted.
// METAS: last Sunday March/October, transitions at 01:00 UTC.
struct ScenarioClock
{
    struct Moment { CivilDate Date; double Hour=0; int OffsetMinutes=60; double UTCSeconds=0; };
    static double Midnight(CivilDate date) { return date.SerialDay()*86400.; }
    static int LastSunday(int year,int month)
    {
        CivilDate end{year,month,CivilDate::DaysInMonth(year,month)};
        return end.Day-end.Weekday();
    }
    static int OffsetAtUTC(double seconds)
    {
        const auto date=CivilDate::FromSerialDay(static_cast<int>(std::floor(seconds/86400.)));
        const double begin=Midnight({date.Year,3,LastSunday(date.Year,3)})+3600;
        const double end=Midnight({date.Year,10,LastSunday(date.Year,10)})+3600;
        return seconds>=begin&&seconds<end?120:60;
    }
    static bool StartUTC(CivilDate date,double hour,double& seconds)
    {
        if(!date.ScenarioRange()||!std::isfinite(hour)||hour<0||hour>=24) return false;
        const double local=Midnight(date)+hour*3600;
        // First occurrence (CEST) of the repeated autumn hour is explicit policy.
        for(int offset:{120,60})
        {
            const double candidate=local-offset*60;
            if(OffsetAtUTC(candidate)==offset) { seconds=candidate; return true; }
        }
        return false; // Nonexistent spring hour: never silently accept in the UI.
    }
    static Moment At(CivilDate startDate,double startHour,double elapsed)
    {
        double utc=0;
        if(!StartUTC(startDate,startHour,utc)) StartUTC(CivilDate{},13,utc);
        if(!std::isfinite(elapsed)||elapsed<0) elapsed=0;
        // Bound only corrupt/extreme input; ordinary multi-day dives never wrap dates.
        utc=std::min(utc+elapsed,Midnight({9999,12,30}));
        const int offset=OffsetAtUTC(utc);
        const double local=utc+offset*60;
        const int day=static_cast<int>(std::floor(local/86400.));
        return {CivilDate::FromSerialDay(day),(local-day*86400.)/3600.,offset,utc};
    }
};
}
