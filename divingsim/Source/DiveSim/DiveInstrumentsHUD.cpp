#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "GameFramework/PlayerController.h"

namespace
{
const FLinearColor White(.87,.94,.92,1),Muted(.52,.66,.64,1),Accent(.34,.85,.72,1);
const FLinearColor Dark(.012,.032,.032,.94),Panel(.025,.065,.063,.92),Amber(1,.73,.3,1);
FString Clock(double Seconds)
{ const int t=FMath::Max(0,FMath::FloorToInt(Seconds));return FString::Printf(TEXT("%02d:%02d"),t/60,t%60); }
}

TArray<FString> ADiveHUD::WarningMessages() const
{
    TArray<FString> messages;
    const auto& s=Diver->Simulation.Data;
    auto warning=[&](uint32 Flag,const TCHAR* de,const TCHAR* en)
    { if(s.Warnings&Flag) messages.Add(Diver->Local(de,en)); };
    warning(Dive::NoBreathingSupply,TEXT("Keine Atemluft"),TEXT("No breathing supply"));
    warning(Dive::OutOfGas,TEXT("Flasche leer"),TEXT("Cylinder empty"));
    warning(Dive::EquipmentFailure,TEXT("Reglerdefekt / freier Gasfluss"),TEXT("Regulator fault / freeflow"));
    warning(Dive::CeilingViolation,TEXT("Dekompressionstiefe unterschritten"),TEXT("Above decompression ceiling"));
    warning(Dive::HighPO2,TEXT("Erhöhter Sauerstoffpartialdruck"),TEXT("High oxygen partial pressure"));
    warning(Dive::FastAscent,TEXT("Hohe Aufstiegsgeschwindigkeit"),TEXT("High ascent rate"));
    warning(Dive::LowGas,TEXT("Niedriger Flaschendruck"),TEXT("Low cylinder pressure"));
    warning(Dive::DepthPlan,TEXT("Geplante Tiefe überschritten"),TEXT("Beyond planned depth"));
    warning(Dive::TimePlan,TEXT("Geplante Zeit überschritten"),TEXT("Beyond planned time"));
    warning(Dive::Cold,TEXT("Kältebelastung"),TEXT("Cold strain"));
    warning(Dive::NarcosisWarning,TEXT("Narkosewirkung · Spielmodell"),TEXT("Narcosis effect · game model"));
    warning(Dive::PanicWarning,TEXT("Panikreaktion"),TEXT("Panic response"));
    warning(Dive::DecompressionSymptoms,TEXT("Dekobelastung · Spielmodell"),TEXT("Decompression strain · game model"));
    return messages;
}

void ADiveHUD::Instruments()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    const bool minimal=Diver->HUDStyle==2,compact=Diver->HUDStyle==1;
    const float yaw=FMath::Fmod(GetOwningPlayerController()->GetControlRotation().Yaw+360,360);
    const TCHAR* de[]={TEXT("N"),TEXT("NO"),TEXT("O"),TEXT("SO"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    const TCHAR* en[]={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    const int heading=FMath::RoundToInt(yaw/45)%8;
    Rect(642,22,316,minimal?42:68,Dark);
    Text(FString::Printf(TEXT("%s  %03d°     %s"),Diver->bEnglish?en[heading]:de[heading],FMath::RoundToInt(yaw)%360,*Clock(s.ElapsedSeconds)),665,34,19,White);
    if(!minimal)
    {
        const auto clock=Diver->Environment->CurrentMoment();
        const int minute=FMath::FloorToInt(clock.Hour*60+1.e-8)%1440;
        Text(FString::Printf(TEXT("%s · %02d:%02d %s"),UTF8_TO_TCHAR(clock.Date.Text().c_str()),minute/60,minute%60,clock.OffsetMinutes==120?TEXT("CEST"):TEXT("CET")),665,65,12,Muted);
        Rect(26,22,330,76,Dark);
        TextBlock(TEXT("WALENSEE / ")+Diver->EntryName(),40,33,302,14,White,2);
        Text(Diver->bWalking?L(TEXT("GEHEN / WATEN"),TEXT("WALKING / WADING")):L(TEXT("SCHWIMMEN"),TEXT("SWIMMING")),40,76,12,Accent);
        Rect(26,110,330,64,Dark);
        const FString source=s.BreathSource==Dive::BreathingSource::Air?L(TEXT("Umgebungsluft"),TEXT("Ambient air")):s.BreathSource==Dive::BreathingSource::Regulator?L(TEXT("Atemregler aktiv"),TEXT("Regulator active")):L(TEXT("Keine Atemluft"),TEXT("No breathing supply"));
        TextBlock(source+TEXT("  [")+Diver->KeyText(EDiveAction::Regulator)+TEXT("]"),40,121,302,15,s.BreathSource==Dive::BreathingSource::None?Amber:White,1);
        Text(L(TEXT("Atemreserve "),TEXT("Breath reserve "))+FString::Printf(TEXT("%.0f %%"),s.BreathReserve*100),40,149,13,s.BreathReserve<.25?Amber:Muted);
    }
    WaypointInstrument();
    const FString gas=Diver->Simulation.Settings.GameMode==Dive::Mode::Exploration?TEXT("∞"):FString::Printf(TEXT("%.0f bar"),s.CylinderBar);
    if(minimal)
    {
        Rect(500,810,600,62,Dark);
        Text(FString::Printf(TEXT("%.1f m"),s.DepthMetres),520,827,24,White);
        Text(gas,705,827,24,s.CylinderBar<50?Amber:White);
        Text(Clock(s.ElapsedSeconds),905,820,21,White);
        Text(FString::Printf(TEXT("NDL %.0f min"),s.NDLMinutes),905,849,12,Muted);
    }
    else
    {
        const float y=compact?790:766,h=compact?82:106;
        Rect(340,y,920,h,Dark);
        for(int i=1;i<4;++i) Line(340+i*230,y+14,340+i*230,y+h-14,Panel,1);
        const float a=356,b=586,c=816,d=1046;
        Text(L(TEXT("TIEFE"),TEXT("DEPTH")),a,y+12,12,Muted);
        Text(FString::Printf(TEXT("%.1f m"),s.DepthMetres),a,y+34,29,White);
        Text(L(TEXT("FLASCHENDRUCK"),TEXT("CYLINDER PRESSURE")),b,y+12,12,Muted);
        Text(gas,b,y+34,29,s.CylinderBar<50?Amber:White);
        const float pressure=FMath::Clamp(s.CylinderBar/FMath::Max(1.,Diver->Simulation.Settings.StartPressureBar),0.,1.);
        Rect(b,y+h-13,194,3,Panel);Rect(b,y+h-13,194*pressure,3,s.CylinderBar<50?Amber:Accent);
        Text(L(TEXT("NULLZEIT / BCD"),TEXT("NDL / BCD")),c,y+12,12,Muted);
        Text(FString::Printf(TEXT("%.0f min"),s.NDLMinutes),c,y+34,29,White);
        Text(L(TEXT("VITALWERTE"),TEXT("VITALS")),d,y+12,12,Muted);
        Text(FString::Printf(TEXT("%.0f bpm"),s.HeartBPM),d,y+34,29,White);
        if(!compact)
        {
            Text(FString::Printf(TEXT("MAX %.1f m · %.1f °C"),s.MaxDepthMetres,s.WaterTemperatureC),a,y+82,12,Muted);
            Text(FString::Printf(TEXT("%+.1f m/min · BCD %.1f L"),s.AscentMetresPerMinute,s.BCDLitres),c,y+82,12,s.AscentMetresPerMinute>9?Amber:Muted);
            Text(FString::Printf(TEXT("SpO₂ %.0f %% · %.1f °C"),s.SpO2,s.CoreTemperatureC),d,y+82,12,Muted);
        }
    }
    const auto messages=WarningMessages();
    HUDRightBottom=22;
    const int visible=FMath::Min(messages.Num(),6);
    for(int i=0;i<visible;++i)
    {
        Rect(1220,HUDRightBottom,354,52,Dark);Rect(1220,HUDRightBottom,3,52,Amber);
        TextBlock(TEXT("!  ")+messages[i],1234,HUDRightBottom+9,326,15,Amber,2);
        HUDRightBottom+=60;
    }
    if(messages.Num()>visible)
    {
        Text(FString::Printf(TEXT("+ %d  ·  "),messages.Num()-visible)+L(TEXT("Alle Meldungen: Esc"),TEXT("All warnings: Esc")),1234,HUDRightBottom,15,Amber);
        HUDRightBottom+=28;
    }
    FString stop,detail;
    if(s.DecoForecastValid&&s.DecoStopMetres>0)
    {
        stop=L(TEXT("DEKOSTOPP  "),TEXT("DECO STOP  "))+FString::Printf(TEXT("%.1f m · %s"),s.DecoStopMetres,*Clock(s.DecoStopSeconds));
        detail=L(TEXT("Aufstiegsprognose: "),TEXT("Ascent forecast: "))+Clock(s.TimeToSurfaceSeconds);
    }
    else if(!s.DecoForecastValid&&s.CeilingMetres>0) stop=L(TEXT("Deko-Prognose nicht verfügbar"),TEXT("Deco forecast unavailable"));
    else if(s.MaxDepthMetres>10&&s.DepthMetres<10&&s.StopRemainingSeconds>0)
        stop=L(TEXT("Sicherheitsstopp 3–6 m · "),TEXT("Safety stop 3–6 m · "))+Clock(s.StopRemainingSeconds);
    if(!stop.IsEmpty())
    {
        Rect(1220,HUDRightBottom,354,68,Dark);
        TextBlock(stop,1234,HUDRightBottom+10,326,16,Accent,2);
        if(!detail.IsEmpty()) TextBlock(detail,1234,HUDRightBottom+43,326,13,Muted,1);
        HUDRightBottom+=76;
    }
    if(!minimal) TextBlock(Diver->KeyText(EDiveAction::Inflate)+TEXT(" / ")+Diver->KeyText(EDiveAction::Deflate)+L(TEXT(" Jacket    "),TEXT(" BCD    "))+Diver->KeyText(EDiveAction::Lamp)+L(TEXT(" Lampe    "),TEXT(" Torch    "))+Diver->KeyText(EDiveAction::Computer)+L(TEXT(" Handgelenk    "),TEXT(" Wrist    "))+Diver->KeyText(EDiveAction::Map)+L(TEXT(" Karte    F1 Hilfe    Esc Pause"),TEXT(" Map    F1 Help    Esc Pause")),26,881,1000,12,Muted,1);
}

void ADiveHUD::PauseDashboard()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    const FString labels[]={L(TEXT("TIEFE"),TEXT("DEPTH")),L(TEXT("FLASCHENDRUCK"),TEXT("CYLINDER PRESSURE")),L(TEXT("TAUCHZEIT"),TEXT("DIVE TIME"))};
    const FString values[]={FString::Printf(TEXT("%.1f m"),s.DepthMetres),Diver->Simulation.Settings.GameMode==Dive::Mode::Exploration?TEXT("∞"):FString::Printf(TEXT("%.0f bar"),s.CylinderBar),Clock(s.ElapsedSeconds)};
    for(int i=0;i<3;++i)
    {
        Rect(470+i*330,210,310,84,Panel);Text(labels[i],486+i*330,222,13,Muted);Text(values[i],486+i*330,249,27,White);
    }
    Text(L(TEXT("AKTUELLE MELDUNGEN"),TEXT("CURRENT WARNINGS")),470,324,14,Accent);
    const auto messages=WarningMessages();
    if(messages.IsEmpty()) Text(L(TEXT("Keine aktiven Warnungen."),TEXT("No active warnings.")),470,361,19,White);
    for(int i=0;i<messages.Num();++i)
        TextBlock(TEXT("!  ")+messages[i],470+(i/7)*510,360+(i%7)*40,475,15,Amber,2);
    const FString styles[]={L(TEXT("Standard"),TEXT("Standard")),L(TEXT("Kompakt"),TEXT("Compact")),L(TEXT("Minimal"),TEXT("Minimal"))};
    Button(TEXT("hud_style"),TEXT("HUD: ")+styles[FMath::Clamp(Diver->HUDStyle,0,2)],470,658,310,42);
    Button(TEXT("tutorial_overlay"),bTutorialCollapsed?L(TEXT("Tutorial einblenden"),TEXT("Show tutorial")):L(TEXT("Tutorial ausblenden"),TEXT("Hide tutorial")),800,658,310,42);
    TextBlock(L(TEXT("Esc setzt den Tauchgang fort. Ausrüstung, Wasserbedingungen und Logbuch findest du links."),TEXT("Esc resumes the dive. Equipment, water conditions and logbook remain available on the left.")),470,722,970,15,Muted,2);
}
