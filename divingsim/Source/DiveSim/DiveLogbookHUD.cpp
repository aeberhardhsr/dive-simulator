#include "DiveHUD.h"
#include "DivePawn.h"
#include "Async/Async.h"
#include "Misc/Paths.h"
#include "GameFramework/PlayerController.h"

namespace
{
const FLinearColor White(.87f,.94f,.92f,1),Muted(.42f,.59f,.57f,1),Accent(.43f,.87f,.69f,1);
const FLinearColor Panel(.025f,.065f,.063f,.92f),Amber(1,.73f,.3f,1),Grid(.08f,.15f,.14f,1);
FString Clock(double seconds)
{ const int t=FMath::Max(0,FMath::FloorToInt(seconds)); return FString::Printf(TEXT("%02d:%02d"),t/60,t%60); }
double Metric(const Dive::State& s,int group,int row)
{
    const double values[4][4]={{s.DepthMetres,s.CylinderBar,s.NDLMinutes,s.CeilingMetres},
        {s.HeartBPM,s.SpO2,s.RespiratoryBPM,s.CoreTemperatureC},
        {s.WaterTemperatureC,s.CoreTemperatureC,s.Stress*100,s.Fatigue*100},
        {s.DistanceMetres,s.PPO2,s.CylinderBar,s.NDLMinutes}};
    return values[group][row];
}
FString Outcome(const FDiveLog& log,bool english)
{
    if(log.Status==TEXT("completed")) return english?TEXT("Completed"):TEXT("Abgeschlossen");
    if(log.Status==TEXT("aborted")) return english?TEXT("Ended early"):TEXT("Abgebrochen");
    if(log.Status==TEXT("interrupted")) return english?TEXT("Interrupted"):TEXT("Unterbrochen");
    if(log.Status==TEXT("in_progress")) return english?TEXT("Autosave / not completed"):TEXT("Zwischenstand / nicht abgeschlossen");
    if(log.Status==TEXT("out_of_gas")) return english?TEXT("Out of breathing gas"):TEXT("Atemgas aufgebraucht");
    if(log.Status==TEXT("no_breathing_supply")) return english?TEXT("No breathing supply"):TEXT("Keine Atemluft");
    return log.Status.Left(40);
}
}

void ADiveHUD::OpenLogbook(const FString& PreferredPath,bool IncludeTests)
{
    if(!Diver) Diver=Cast<ADivePawn>(GetOwningPawn());
    if(!Diver) return;
    Diver->bLogbook=true; Diver->SetMenu(true);
    // A new opening always refreshes disk state, including after closing during a
    // previous load. Worker lambdas own their input; abandoned results are harmless.
    bDetailLoading=false;
    DetailTask=TFuture<FDiveLog>();
    LogIndex=FDiveLogIndex{}; SelectedLog=FDiveLog{};
    LogSelection=INDEX_NONE; LogPage=0; LogFilter=0; LogMetricGroup=0; LogRenderedGroups=0;
    PreferredLog=PreferredPath; bIndexLoading=true;
    const FString directory=FPaths::ProjectSavedDir()/TEXT("DiveLogs");
    IndexTask=Async(EAsyncExecution::ThreadPool,[directory,IncludeTests](){return DiveLogbook::Scan(directory,IncludeTests);});
}

TArray<int32> ADiveHUD::VisibleLogs() const
{
    TArray<int32> visible;
    for(int i=0;i<LogIndex.Entries.Num();++i)
        if(LogFilter==0||LogIndex.Entries[i].Mode==LogFilter-1) visible.Add(i);
    return visible;
}

void ADiveHUD::SelectLog(int32 Index)
{
    if(bDetailLoading||!LogIndex.Entries.IsValidIndex(Index)) return;
    LogSelection=Index; LogCursor=INDEX_NONE; SelectedLog=FDiveLog{};
    bDetailLoading=true;
    const FString path=LogIndex.Entries[Index].Path;
    DetailTask=Async(EAsyncExecution::ThreadPool,[path](){return DiveLogbook::Read(path);});
}

void ADiveHUD::Logbook()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    if(bIndexLoading&&IndexTask.IsReady())
    {
        LogIndex=IndexTask.Get(); bIndexLoading=false;
        int32 selection=LogIndex.Entries.IndexOfByPredicate([this](const FDiveLog& item){return item.Path==PreferredLog;});
        if(selection==INDEX_NONE&&!LogIndex.Entries.IsEmpty()) selection=0;
        if(selection!=INDEX_NONE) { LogPage=selection/7; SelectLog(selection); }
    }
    if(bDetailLoading&&DetailTask.IsReady())
    { SelectedLog=DetailTask.Get(); bDetailLoading=false; LogCursor=SelectedLog.Samples.Num()-1; }
    // Opaque enough to make data readable over both daylight and night scenes.
    Rect(40,25,1520,855,FLinearColor(.008f,.026f,.027f,.98f));
    Text(L(TEXT("D I V E   /   L O G B U C H"),TEXT("D I V E   /   L O G B O O K")),70,52,20,Accent);
    Text(L(TEXT("Deine Tauchgaenge"),TEXT("Your dives")),70,95,35,White);
    const FString filters[]={L(TEXT("Alle Modi"),TEXT("All modes")),TEXT("Exploration"),TEXT("Simulation"),TEXT("Hardcore")};
    Button(TEXT("logfilter"),filters[LogFilter]+TEXT("   >"),70,151,370,42);
    const auto visible=VisibleLogs();
    LogPage=FMath::Clamp(LogPage,0,FMath::Max(0,(visible.Num()-1)/7));
    for(int row=0;row<7;++row)
    {
        const int p=LogPage*7+row; if(!visible.IsValidIndex(p)) break;
        const int index=visible[p]; const auto& item=LogIndex.Entries[index];
        const float y=210+row*73;
        Button(FName(*FString::Printf(TEXT("logrow_%d"),index)),TEXT(""),70,y,370,65);
        if(index==LogSelection) Rect(70,y,3,65,Accent);
        Text(item.Date,85,y+8,16,index==LogSelection?Accent:White);
        Text(FString::Printf(TEXT("%s  |  %s  |  %.1f m"),*item.Location.Left(23),*Clock(item.Duration),item.MaxDepth),85,y+35,14,Muted);
    }
    if(visible.IsEmpty()) Text(bIndexLoading?L(TEXT("Tauchgaenge werden geladen ..."),TEXT("Loading dives ...")):
        L(TEXT("Keine Tauchgaenge in dieser Auswahl."),TEXT("No dives in this selection.")),75,227,16,Muted);
    if(LogPage>0) Button(TEXT("logprev"),TEXT("<"),70,739,68,40);
    Text(FString::Printf(TEXT("%d / %d   (%d)"),visible.IsEmpty()?0:LogPage+1,FMath::Max(1,(visible.Num()+6)/7),visible.Num()),155,752,16,Muted);
    if((LogPage+1)*7<visible.Num()) Button(TEXT("lognext"),TEXT(">"),372,739,68,40);
    if(LogIndex.Unreadable>0) Text(FString::FromInt(LogIndex.Unreadable)+L(TEXT(" Dateien nicht lesbar / nicht unterstuetzt"),TEXT(" files unreadable / unsupported")),70,794,14,Amber);
    Button(TEXT("logback"),L(TEXT("<  Zurueck   (Esc)"),TEXT("<  Back   (Esc)")),70,825,370,40);
    Button(TEXT("loglanguage"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),1270,825,260,40);

    if(bIndexLoading||bDetailLoading)
    { Text(L(TEXT("Profil wird geladen ..."),TEXT("Loading profile ...")),510,200,24,Muted); return; }
    if(LogSelection==INDEX_NONE)
    {
        Text(L(TEXT("Dein naechster Tauchgang beginnt hier."),TEXT("Your next dive starts here.")),510,208,28,White);
        Text(L(TEXT("Nach dem Tauchen findest du hier Verlaeufe und Warnungen."),TEXT("After diving, find your profiles and warnings here.")),510,260,19,Muted);
        return;
    }
    const auto& log=SelectedLog;
    if(!log.Error.IsEmpty())
    { Text(L(TEXT("Dieses Protokoll konnte nicht geladen werden."),TEXT("This log could not be loaded.")),510,208,24,Amber); return; }
    Text(log.Location.Left(42),510,53,31,White);
    Text(log.Date+TEXT("   /   ")+Outcome(log,Diver->bEnglish),510,100,16,Muted);
    const TCHAR* modes[]={TEXT("Exploration"),TEXT("Simulation"),TEXT("Hardcore")};
    Text(FString::Printf(TEXT("%s    /    %.0f L    /    %d %s"),modes[log.Mode],log.CylinderLitres,log.Samples.Num(),*L(TEXT("Messpunkte"),TEXT("samples"))),510,131,16,Muted);
    const FString titles[]={L(TEXT("DAUER"),TEXT("DURATION")),L(TEXT("MAX. TIEFE"),TEXT("MAX. DEPTH")),L(TEXT("STRECKE"),TEXT("DISTANCE")),L(TEXT("WARNEREIGNISSE"),TEXT("WARNING EVENTS"))};
    const FString values[]={Clock(log.Duration),FString::Printf(TEXT("%.1f m"),log.MaxDepth),FString::Printf(TEXT("%.0f m"),log.Distance),FString::FromInt(log.WarningEvents)};
    for(int i=0;i<4;++i) { Rect(510+i*258,175,244,93,Panel); Text(titles[i],525+i*258,188,13,Muted); Text(values[i],525+i*258,218,29,White); }
    const FString groups[]={L(TEXT("Tauchen"),TEXT("Dive")),L(TEXT("Vitalwerte"),TEXT("Vitals")),L(TEXT("Temperatur / Stress"),TEXT("Temperature / stress")),L(TEXT("Strecke / Gas"),TEXT("Distance / gas"))};
    for(int i=0;i<4;++i) Button(FName(*FString::Printf(TEXT("logmetric_%d"),i)),groups[i],510+i*258,286,244,43,LogMetricGroup==i);
    if(log.Samples.IsEmpty()) { Text(L(TEXT("Keine Messpunkte gespeichert."),TEXT("No samples recorded.")),510,375,24,Muted); return; }
    LogRenderedGroups|=1u<<LogMetricGroup;
    float mx=0,my=0; GetOwningPlayerController()->GetMousePosition(mx,my);
    mx=(mx-OffsetX)/Scale; my=(my-OffsetY)/Scale;
    constexpr float x=635,width=885,top=390,rowHeight=82,height=53;
    if(mx>=x&&mx<=x+width&&my>=top&&my<=top+3*rowHeight+height)
        LogCursor=DiveLogbook::NearestSample(log.Samples,(mx-x)/width*FMath::Max(1.,log.Duration));
    LogCursor=FMath::Clamp(LogCursor,0,log.Samples.Num()-1);
    const auto& sample=log.Samples[LogCursor];
    Text(L(TEXT("Zeitpunkt  "),TEXT("Time  "))+Clock(sample.ElapsedSeconds),510,348,20,Accent);
    Text(L(TEXT("Maus ueber die Kurven bewegen"),TEXT("Move the pointer across the curves")),1015,351,15,Muted);
    const TCHAR* de[4][4]={{TEXT("Tiefe / m"),TEXT("Druck / bar"),TEXT("Nullzeit / min"),TEXT("Dekotiefe / m")},
        {TEXT("Puls / bpm"),TEXT("SpO2 / %"),TEXT("Atmung / min"),TEXT("Kern / C")},
        {TEXT("Wasser / C"),TEXT("Kern / C"),TEXT("Stress / %"),TEXT("Ermuedung / %")},
        {TEXT("Strecke / m"),TEXT("ppO2 / bar"),TEXT("Druck / bar"),TEXT("Nullzeit / min")}};
    const TCHAR* en[4][4]={{TEXT("Depth / m"),TEXT("Gas / bar"),TEXT("NDL / min"),TEXT("Ceiling / m")},
        {TEXT("Pulse / bpm"),TEXT("SpO2 / %"),TEXT("Breaths / min"),TEXT("Core / C")},
        {TEXT("Water / C"),TEXT("Core / C"),TEXT("Stress / %"),TEXT("Fatigue / %")},
        {TEXT("Distance / m"),TEXT("ppO2 / bar"),TEXT("Gas / bar"),TEXT("NDL / min")}};
    for(int row=0;row<4;++row)
    {
        const float y=top+row*rowHeight;
        double lo=Metric(log.Samples[0],LogMetricGroup,row),hi=lo;
        for(const auto& s:log.Samples) { const double v=Metric(s,LogMetricGroup,row); lo=FMath::Min(lo,v); hi=FMath::Max(hi,v); }
        const bool depth=LogMetricGroup==0&&(row==0||row==3);
        if(depth) { lo=0; hi=FMath::Max(5.,FMath::CeilToDouble(hi/5)*5); }
        else { const double pad=FMath::Max(.1,(hi-lo)*.1); lo=(LogMetricGroup==2&&row==0)?lo-pad:FMath::Max(0.,lo-pad); hi+=pad; }
        auto px=[&](double t){return x+FMath::Clamp(t/FMath::Max(1.,log.Duration),0.,1.)*width;};
        auto py=[&](double v){const double f=(v-lo)/(hi-lo); return y+(depth?f:1-f)*height;};
        Rect(x,y,width,height,Panel); Line(x,y+height*.5,x+width,y+height*.5,Grid);
        Text(L(de[LogMetricGroup][row],en[LogMetricGroup][row]),510,y,14,Muted);
        Text(FString::Printf(TEXT("%.*f"),LogMetricGroup==3&&row==1?2:1,Metric(sample,LogMetricGroup,row)),510,y+22,25,White);
        Text(FString::Printf(TEXT("%.1f"),depth?lo:hi),x+4,y,11,Muted);
        Text(FString::Printf(TEXT("%.1f"),depth?hi:lo),x+4,y+height-13,11,Muted);
        // Pixel bins preserve extrema in long logs instead of dropping narrow peaks.
        int i=0; double previousX=0,previousY=0; bool first=true;
        while(i<log.Samples.Num())
        {
            const int bin=FMath::FloorToInt(px(log.Samples[i].ElapsedSeconds));
            const double firstY=py(Metric(log.Samples[i],LogMetricGroup,row));
            double minimum=firstY,maximum=firstY,lastY=firstY;
            do { lastY=py(Metric(log.Samples[i],LogMetricGroup,row)); minimum=FMath::Min(minimum,lastY); maximum=FMath::Max(maximum,lastY); ++i; }
            while(i<log.Samples.Num()&&FMath::FloorToInt(px(log.Samples[i].ElapsedSeconds))==bin);
            if(!first) Line(previousX,previousY,bin,firstY,Accent,1.5);
            Line(bin,minimum,bin,FMath::Max(minimum+1,maximum),Accent,1.5);
            previousX=bin; previousY=lastY; first=false;
        }
        Line(px(sample.ElapsedSeconds),y,px(sample.ElapsedSeconds),y+height,White,.8f);
    }
    Text(TEXT("00:00"),x,top+3*rowHeight+height+5,12,Muted);
    Text(Clock(log.Duration),x+width-55,top+3*rowHeight+height+5,12,Muted);
    uint32 previous=0;
    for(const auto& s:log.Samples)
    { if(s.Warnings&~previous) Rect(x+s.ElapsedSeconds/FMath::Max(1.,log.Duration)*width,718,3,9,Amber); previous=s.Warnings; }
    Button(TEXT("logeventprev"),TEXT("< !"),510,734,70,36);
    Button(TEXT("logeventnext"),TEXT("! >"),590,734,70,36);
    FString warnings;
    const TCHAR* warnDE[]={TEXT("Druck niedrig"),TEXT("Aufstieg schnell"),TEXT("ppO2 hoch"),TEXT("Unterkuehlt"),TEXT("Dekotiefe unterschritten"),TEXT("Flasche leer"),TEXT("Tiefenplan"),TEXT("Zeitplan")};
    const TCHAR* warnEN[]={TEXT("Low gas"),TEXT("Fast ascent"),TEXT("High ppO2"),TEXT("Cold"),TEXT("Above ceiling"),TEXT("Cylinder empty"),TEXT("Depth plan"),TEXT("Time plan")};
    int count=0;
    for(int bit=0;bit<8;++bit) if(sample.Warnings&(1u<<bit))
    { if(count++) warnings+=count==5?TEXT("\n"):TEXT(" / "); warnings+=L(warnDE[bit],warnEN[bit]); }
    if(sample.Warnings&Dive::NoBreathingSupply)
    { if(!warnings.IsEmpty()) warnings+=TEXT(" / "); warnings+=L(TEXT("Keine Atemluft"),TEXT("No breathing supply")); }
    Text(warnings.IsEmpty()?L(TEXT("Keine aktive Warnung an diesem Zeitpunkt"),TEXT("No active warning at this time")):warnings,685,742,14,warnings.IsEmpty()?Muted:Amber);
    if(log.bBreathingContext)
    {
        const TCHAR* breathingDE[]={TEXT("Atemregler"),TEXT("Umgebungsluft"),TEXT("Keine Atemluft")};
        const TCHAR* breathingEN[]={TEXT("Regulator"),TEXT("Ambient air"),TEXT("No breathing supply")};
        const int source=static_cast<int>(sample.BreathSource);
        Text(L(breathingDE[source],breathingEN[source])+FString::Printf(TEXT("    %.0f%% "),sample.BodyImmersion*100)+
            L(TEXT("eingetaucht    Umgebung "),TEXT("immersed    Environment "))+
            FString::Printf(TEXT("%.1f C"),sample.EnvironmentTemperatureC)+
            (log.bBreathReserveContext?L(TEXT("    Atemreserve "),TEXT("    Breath reserve "))+FString::Printf(TEXT("%.0f%%"),sample.BreathReserve*100):TEXT("")),510,796,16,Accent);
    }
    Text(L(TEXT("Kurven: gespeicherte Spielwerte. Gelbe Marken: neue Warnungen."),TEXT("Curves: recorded game values. Yellow marks: new warnings.")),510,834,14,Muted);
}
