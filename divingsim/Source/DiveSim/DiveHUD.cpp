#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Styling/CoreStyle.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "Kismet/KismetSystemLibrary.h"

namespace
{
const FLinearColor White(.87,.94,.92,1), Muted(.42,.59,.57,1), Accent(.43,.87,.69,1);
const FLinearColor Dark(.012,.032,.032,.94), Panel(.025,.065,.063,.92), Red(1,.35,.25,1), Amber(1,.73,.3,1);
FString Time(double Seconds) { const int t=FMath::Max(0,FMath::FloorToInt(Seconds)); return FString::Printf(TEXT("%02d:%02d"),t/60,t%60); }
}

void ADiveHUD::Text(const FString& Value,float X,float Y,float Size,FLinearColor Color)
{
    if(!InterfaceFont) InterfaceFont=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    const FSlateFontInfo font(InterfaceFont,FMath::Max(8,FMath::RoundToInt(Size*Scale*.75f)),TEXT("Regular"));
    FCanvasTextItem item(FVector2D(OffsetX+X*Scale,OffsetY+Y*Scale),FText::FromString(Value),font,Color);
    Canvas->DrawItem(item);
}
void ADiveHUD::Rect(float X,float Y,float W,float H,FLinearColor Color)
{ DrawRect(Color,OffsetX+X*Scale,OffsetY+Y*Scale,W*Scale,H*Scale); }
void ADiveHUD::Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness)
{ DrawLine(OffsetX+X*Scale,OffsetY+Y*Scale,OffsetX+X2*Scale,OffsetY+Y2*Scale,Color,Thickness*Scale); }

void ADiveHUD::Button(FName Id,const FString& Label,float X,float Y,float W,float H,bool Highlight)
{
    float mx=0,my=0; GetOwningPlayerController()->GetMousePosition(mx,my);
    const bool hover=mx>=OffsetX+X*Scale && mx<=OffsetX+(X+W)*Scale && my>=OffsetY+Y*Scale && my<=OffsetY+(Y+H)*Scale;
    Rect(X,Y,W,H,Highlight?Accent:(hover?FLinearColor(.07,.15,.13,1):Panel));
    Text(Label,X+18,Y+H*.28f,20,Highlight?Dark:White);
    AddHitBox(FVector2D(OffsetX+X*Scale,OffsetY+Y*Scale),FVector2D(W*Scale,H*Scale),Id,true);
}

void ADiveHUD::Field(FName Id,const FString& Label,const FString& Value,float X,float Y,float W)
{
    Text(Label,X,Y,14,Muted);
    Button(Id,Value+TEXT("   >"),X,Y+25,W,47,false);
}

void ADiveHUD::DrawHUD()
{
    Super::DrawHUD();
    Diver=Cast<ADivePawn>(GetOwningPawn()); if(!Diver || !Canvas) return;
    Scale=FMath::Min(Canvas->SizeX/1600.f,Canvas->SizeY/900.f);
    OffsetX=(Canvas->SizeX-1600*Scale)/2; OffsetY=(Canvas->SizeY-900*Scale)/2;
    if(Diver->bMenu)
    {
        DrawRect(FLinearColor(0,.012,.014,.62),0,0,Canvas->SizeX,Canvas->SizeY);
        if(Diver->bSummary) Summary(); else Menu();
    }
    else
    {
        if(Diver->bShowInstruments) Instruments();
        if(Diver->bShowMap) Map();
        if(GetWorld()->GetTimeSeconds()<Diver->NoticeUntil)
        {
            Rect(320,610,960,48,Dark); Text(Diver->Notice,340,622,17,White);
        }
        if(Diver->Simulation.Data.DepthMetres<.8 && FVector::Dist2D(Diver->GetActorLocation(),Diver->EntryLocation)<1200)
        { Rect(570,680,460,48,Dark); Text(Diver->Local(TEXT("[ F ]  Tauchgang abschliessen"),TEXT("[ F ]  Finish dive")),590,693,20,Accent); }
        Text(TEXT("+"),795,445,14,FLinearColor(.75,.9,.86,.45));
    }
}

void ADiveHUD::Menu()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    auto& c=Diver->PendingConfig;
    Text(TEXT("D I V E   /   0 0 1"),70,60,18,Accent);
    Line(70,105,1530,105,Muted,.6);
    Text(TEXT("WALENSEE"),70,185,64,White);
    Text(L(TEXT("Unter der Oberflaeche."),TEXT("Below the surface.")),73,270,28,White);
    Text(L(TEXT("Ruhe. Tiefe. Entdecken."),TEXT("Stillness. Depth. Discovery.")),73,319,20,Muted);
    Text(L(TEXT("SPIELBARER ENTWICKLUNGSSTAND"),TEXT("PLAYABLE DEVELOPMENT BUILD")),73,410,13,Accent);
    Text(L(TEXT("Trainingsbucht mit synthetischem Seegrund."),TEXT("Training bay with synthetic lakebed.")),73,444,17,White);
    Text(L(TEXT("Die reale Walensee-Topografie folgt."),TEXT("Surveyed Walensee topography is upcoming.")),73,473,17,Muted);
    Text(L(TEXT("Pressluft, Tarierung und Vitalwerte"),TEXT("Breathing gas, buoyancy and vitals")),73,525,18,White);
    Text(L(TEXT("reagieren auf deinen Tauchgang."),TEXT("respond to the way you dive.")),73,554,18,White);
    Text(L(TEXT("Spielsimulation. Keine reale Tauchplanung."),TEXT("Game simulation. Not real dive planning.")),73,650,15,Muted);
    const float x=650,w=265,gap=28;
    Text(Diver->bDiveActive?L(TEXT("TAUCHGANG PAUSIERT"),TEXT("DIVE PAUSED")):L(TEXT("DEIN TAUCHGANG"),TEXT("YOUR DIVE")),x,160,23,White);
    Text(L(TEXT("Werte anklicken, um sie zu wechseln"),TEXT("Click a value to cycle options")),x,198,15,Muted);
    const FString modes[]={L(TEXT("Exploration"),TEXT("Exploration")),L(TEXT("Simulation"),TEXT("Simulation")),L(TEXT("Hardcore *"),TEXT("Hardcore *"))};
    Field(TEXT("mode"),L(TEXT("MODUS"),TEXT("MODE")),modes[static_cast<int>(c.GameMode)],x,250,w);
    Field(TEXT("entry"),L(TEXT("EINSTIEG / TESTGELAENDE"),TEXT("ENTRY / TEST TERRAIN")),FString::Printf(TEXT("Bucht %d"),Diver->EntryIndex+1),x+w+gap,250,w);
    Field(TEXT("hour"),L(TEXT("TAGESZEIT"),TEXT("TIME OF DAY")),FString::Printf(TEXT("%02d:00"),static_cast<int>(c.Hour)),x+2*(w+gap),250,w);
    Field(TEXT("tank"),L(TEXT("FLASCHENVOLUMEN"),TEXT("CYLINDER VOLUME")),FString::Printf(TEXT("%.0f L / Air 21%%"),c.CylinderLitres),x,350,w);
    Field(TEXT("pressure"),L(TEXT("STARTDRUCK"),TEXT("START PRESSURE")),FString::Printf(TEXT("%.0f bar"),c.StartPressureBar),x+w+gap,350,w);
    Field(TEXT("weight"),L(TEXT("BLEIGEWICHT"),TEXT("BALLAST")),FString::Printf(TEXT("%.0f kg"),c.WeightKg),x+2*(w+gap),350,w);
    Field(TEXT("suit"),L(TEXT("ANZUG"),TEXT("SUIT")),c.DrySuit?L(TEXT("Trockentauchanzug"),TEXT("Dry suit")):L(TEXT("Nassanzug"),TEXT("Wet suit")),x,450,w);
    Field(TEXT("visibility"),L(TEXT("SICHTWEITE"),TEXT("VISIBILITY")),FString::Printf(TEXT("%.0f m"),c.VisibilityMetres),x+w+gap,450,w);
    Field(TEXT("current"),L(TEXT("STROEMUNG"),TEXT("CURRENT")),FString::Printf(TEXT("%.2f m/s"),c.CurrentMetresPerSecond),x+2*(w+gap),450,w);
    Field(TEXT("depth"),L(TEXT("GEPLANTE TIEFE"),TEXT("PLANNED DEPTH")),FString::Printf(TEXT("%.0f m"),c.PlannedDepthMetres),x,550,w);
    Field(TEXT("duration"),L(TEXT("GEPLANTE DAUER"),TEXT("PLANNED DURATION")),FString::Printf(TEXT("%.0f min"),c.PlannedMinutes),x+w+gap,550,w);
    Field(TEXT("motion"),L(TEXT("KAMERABEWEGUNG"),TEXT("CAMERA MOTION")),Diver->bCameraMotion?L(TEXT("Sanft"),TEXT("Gentle")):L(TEXT("Stabil"),TEXT("Stable")),x+2*(w+gap),550,w);
    Text(c.GameMode==Dive::Mode::Exploration?L(TEXT("Unbegrenztes Atemgas und automatische Tarierung."),TEXT("Unlimited gas and automatic buoyancy.")):
        L(TEXT("Q / E: Jacket fuellen / entleeren. Luft dehnt sich beim Aufstieg aus."),TEXT("Q / E: inflate / deflate BCD. Air expands as you ascend.")),x,655,15,Muted);
    if(c.GameMode==Dive::Mode::Hardcore) Text(L(TEXT("* Bisher wie Simulation; Ausruestungsdefekte sind noch offen."),TEXT("* Currently simulation rules; equipment failures are pending.")),x,681,14,Amber);
    Button(TEXT("start"),Diver->bDiveActive?L(TEXT("Tauchgang fortsetzen"),TEXT("Resume dive")):L(TEXT("Abtauchen   >"),TEXT("Begin dive   >")),x,720,558,58,true);
    if(Diver->bDiveActive) Button(TEXT("abort"),L(TEXT("Tauchgang abbrechen"),TEXT("End dive early")),x+586,720,265,58);
    else Button(TEXT("quit"),L(TEXT("Beenden"),TEXT("Quit")),x+586,720,265,58);
    Line(70,815,1530,815,Muted,.5);
    Button(TEXT("language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),70,835,185,40);
    Button(TEXT("quality"),FString::Printf(TEXT("Grafik / Quality: %.0f"),Diver->GraphicsQuality),275,835,235,40);
    Text(TEXT("WASD  Swim     SPACE / CTRL  Up / Down     Q / E  BCD     L  Light     C  Computer     M  Map"),650,846,14,Muted);
}

void ADiveHUD::Instruments()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    const float yaw=FMath::Fmod(GetOwningPlayerController()->GetControlRotation().Yaw+360,360);
    Rect(640,32,320,58,Dark);
    Text(FString::Printf(TEXT("N    %03.0f deg    %s"),yaw,*Time(s.ElapsedSeconds)),672,50,20,White);
    Text(TEXT("WALENSEE / TRAINING BAY"),50,46,16,White);
    Text(L(TEXT("Einstieg an der Boje  /  F zum Beenden"),TEXT("Entry at buoy  /  F to finish")),50,76,13,Muted);
    Rect(50,705,328,145,Dark); Rect(50,705,3,145,Accent);
    Text(L(TEXT("TIEFE"),TEXT("DEPTH")),75,723,14,Muted);
    Text(FString::Printf(TEXT("%.1f"),s.DepthMetres),72,749,55,White);
    Text(TEXT("m"),230,777,22,Muted);
    Text(FString::Printf(TEXT("MAX %.1f m   /   %.1f C"),s.MaxDepthMetres,s.WaterTemperatureC),76,821,16,Muted);
    Rect(398,705,270,145,Dark);
    Text(L(TEXT("FLASCHENDRUCK"),TEXT("CYLINDER PRESSURE")),420,723,14,Muted);
    Text(Diver->PendingConfig.GameMode==Dive::Mode::Exploration?TEXT("INF"):FString::Printf(TEXT("%.0f"),s.CylinderBar),417,749,55,s.CylinderBar<50?Amber:White);
    Text(TEXT("bar"),565,777,22,Muted);
    Rect(420,824,224,4,Panel); Rect(420,824,224*s.CylinderBar/Diver->PendingConfig.StartPressureBar,4,s.CylinderBar<50?Amber:Accent);
    Rect(688,705,250,145,Dark);
    Text(L(TEXT("NULLZEIT / AUFSTIEG"),TEXT("NDL / ASCENT")),710,723,14,Muted);
    Text(FString::Printf(TEXT("%.0f min"),s.NDLMinutes),707,755,36,White);
    Text(FString::Printf(TEXT("%+.1f m/min"),s.AscentMetresPerMinute),710,815,17,s.AscentMetresPerMinute>9?Amber:Muted);
    Rect(1235,705,315,145,Dark);
    Text(L(TEXT("VITALWERTE  /  SIMULIERT"),TEXT("VITALS  /  SIMULATED")),1257,723,13,Muted);
    Text(FString::Printf(TEXT("%.0f bpm"),s.HeartBPM),1254,759,31,White);
    Text(FString::Printf(TEXT("SpO2  %.0f%%"),s.SpO2),1415,766,17,White);
    Text(FString::Printf(TEXT("%.0f /min    %.1f C    BCD %.1f L"),s.RespiratoryBPM,s.CoreTemperatureC,s.BCDLitres),1257,817,14,Muted);
    Text(L(TEXT("L Lampe   C Computer   M Karte   P Foto   H HUD   ESC Pause"),TEXT("L Torch   C Computer   M Map   P Photo   H HUD   ESC Pause")),50,875,13,Muted);
    if(s.MaxDepthMetres>10 && s.DepthMetres<10 && s.StopRemainingSeconds>0)
    { Rect(1050,110,500,45,Dark); Text(L(TEXT("SICHERHEITSSTOPP  3-6 m   "),TEXT("SAFETY STOP  3-6 m   "))+Time(s.StopRemainingSeconds),1070,123,18,Accent); }
    int row=0;
    auto warning=[&](uint32 Flag,const TCHAR* German,const TCHAR* English)
    { if(s.Warnings&Flag) { Rect(1030,180+row*45,520,38,Dark); Text(TEXT("!  ")+L(German,English),1048,189+row*45,17,Amber); ++row; } };
    warning(Dive::OutOfGas,TEXT("ATEMGAS AUFGEBRAUCHT"),TEXT("OUT OF BREATHING GAS"));
    warning(Dive::LowGas,TEXT("NIEDRIGER FLASCHENDRUCK"),TEXT("LOW CYLINDER PRESSURE"));
    warning(Dive::FastAscent,TEXT("AUFSTIEG VERLANGSAMEN"),TEXT("SLOW YOUR ASCENT"));
    warning(Dive::HighPO2,TEXT("ERHOEHTER SAUERSTOFFPARTIALDRUCK"),TEXT("HIGH OXYGEN PARTIAL PRESSURE"));
    warning(Dive::CeilingViolation,TEXT("DEKOMPRESSIONSTIEFE UNTERSCHRITTEN"),TEXT("ABOVE DECOMPRESSION CEILING"));
    warning(Dive::DepthPlan,TEXT("GEPLANTE TIEFE UEBERSCHRITTEN"),TEXT("BEYOND PLANNED DEPTH"));
    warning(Dive::TimePlan,TEXT("GEPLANTE ZEIT UEBERSCHRITTEN"),TEXT("BEYOND PLANNED TIME"));
    warning(Dive::Cold,TEXT("KOERPERTEMPERATUR NIEDRIG"),TEXT("LOW CORE TEMPERATURE"));
    if(Diver->bShowComputer)
    {
        Rect(1050,420,500,245,Dark);
        Text(L(TEXT("TAUCHCOMPUTER"),TEXT("DIVE COMPUTER")),1075,440,20,Accent);
        Text(FString::Printf(TEXT("ppO2 %.2f bar  |  %.2f bar ambient"),s.PPO2,s.AmbientBar),1075,488,19,White);
        Text(FString::Printf(TEXT("Gas > 50 bar: %.0f min  |  Ceiling %.1f m"),s.RemainingGasMinutes,s.CeilingMetres),1075,527,18,White);
        Text(FString::Printf(TEXT("Stress %.0f%%   Fatigue %.0f%%   Lamp %.0f%%"),s.Stress*100,s.Fatigue*100,s.LampBattery),1075,566,17,Muted);
        Text(L(TEXT("Unvalidiertes Spielmodell / C schliessen"),TEXT("Unvalidated game model / C to close")),1075,622,14,Muted);
    }
}

void ADiveHUD::Map()
{
    Rect(470,160,660,500,Dark);
    Text(Diver->Local(TEXT("TRAININGSBUCHT / SCHEMATISCHE KARTE"),TEXT("TRAINING BAY / SCHEMATIC MAP")),500,182,18,Accent);
    for(int i=0;i<=8;++i)
    {
        Line(520+i*65,240,520+i*65,590,Panel);
        Line(520,240+i*43.75,1040,240+i*43.75,Panel);
    }
    for(int x=0;x<9;++x)
    {
        const float px=-9000+x*2200;
        Text(FString::Printf(TEXT("%.0fm"),-ADiveWorld::GroundZ(px,0)/100),520+x*58,255+x*30,13,Muted);
    }
    auto point=[&](FVector pos,FLinearColor color)
    { Rect(520+(pos.X+10000)/20000*520,240+(pos.Y+10000)/20000*350,9,9,color); };
    for(int i=0;i<Diver->Samples.Num();i+=5) point(Diver->Samples[i].Position,FLinearColor(.2,.4,.35,1));
    point(Diver->EntryLocation,Amber); point(Diver->GetActorLocation(),Accent);
    Text(Diver->Local(TEXT("Gruen: du    Gelb: Einstieg    M: schliessen"),TEXT("Green: you    Yellow: entry    M: close")),500,620,16,White);
}

void ADiveHUD::Summary()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    Text(TEXT("D I V E   /   L O G"),120,80,18,Accent);
    Text(L(TEXT("Dein Tauchgang."),TEXT("Your dive.")),120,137,54,White);
    Text(Diver->Outcome==TEXT("completed")?L(TEXT("Zurueck an der Oberflaeche"),TEXT("Back at the surface")):Diver->Outcome,122,215,19,Muted);
    const FString titles[]={L(TEXT("DAUER"),TEXT("DURATION")),L(TEXT("MAXIMALE TIEFE"),TEXT("MAXIMUM DEPTH")),L(TEXT("RESTDRUCK"),TEXT("GAS REMAINING")),L(TEXT("STRECKE"),TEXT("DISTANCE"))};
    const FString values[]={Time(s.ElapsedSeconds),FString::Printf(TEXT("%.1f m"),s.MaxDepthMetres),FString::Printf(TEXT("%.0f bar"),s.CylinderBar),FString::Printf(TEXT("%.0f m"),s.DistanceMetres)};
    for(int i=0;i<4;++i)
    { Rect(120+i*345,290,325,130,Panel); Text(titles[i],140+i*345,310,14,Muted); Text(values[i],137+i*345,347,37,White); }
    Text(L(TEXT("TIEFENPROFIL"),TEXT("DEPTH PROFILE")),120,458,16,Accent);
    Rect(120,500,1360,195,Panel);
    for(int i=0;i<5;++i) Line(120,500+i*48,1480,500+i*48,FLinearColor(.08,.15,.14,1));
    const float range=FMath::Max(5.0,s.MaxDepthMetres);
    for(int i=1;i<Diver->Samples.Num();++i)
    {
        const auto& a=Diver->Samples[i-1].State; const auto& b=Diver->Samples[i].State;
        Line(120+a.ElapsedSeconds/FMath::Max(1.0,s.ElapsedSeconds)*1360,500+a.DepthMetres/range*190,
            120+b.ElapsedSeconds/FMath::Max(1.0,s.ElapsedSeconds)*1360,500+b.DepthMetres/range*190,Accent,2);
    }
    Text(L(TEXT("Telemetrie als JSON: divingsim/Saved/DiveLogs"),TEXT("JSON telemetry: divingsim/Saved/DiveLogs")),120,717,16,Muted);
    Button(TEXT("new"),L(TEXT("Naechsten Tauchgang planen  >"),TEXT("Plan another dive  >")),120,785,540,60,true);
    Button(TEXT("quit"),L(TEXT("Beenden"),TEXT("Quit")),1215,785,265,60);
}

void ADiveHUD::NotifyHitBoxClick(FName Id)
{
    Super::NotifyHitBoxClick(Id); if(!Diver) return;
    auto& c=Diver->PendingConfig;
    if(Id==TEXT("start")) { if(Diver->bDiveActive) Diver->ResumeDive(); else Diver->StartDive(); return; }
    if(Id==TEXT("abort")) { Diver->FinishDive(TEXT("aborted")); return; }
    if(Id==TEXT("new")) { Diver->bSummary=false; return; }
    if(Id==TEXT("quit")) { UKismetSystemLibrary::QuitGame(this,GetOwningPlayerController(),EQuitPreference::Quit,false); return; }
    if(Id==TEXT("language")) Diver->bEnglish=!Diver->bEnglish;
    else if(Id==TEXT("motion")) Diver->bCameraMotion=!Diver->bCameraMotion;
    else if(Id==TEXT("quality"))
    {
        Diver->GraphicsQuality=FMath::Fmod(Diver->GraphicsQuality+1,4.f);
        if(auto* settings=GEngine->GetGameUserSettings())
        { settings->SetOverallScalabilityLevel(Diver->GraphicsQuality); settings->ApplyNonResolutionSettings(); settings->SaveSettings(); }
    }
    else if(!Diver->bDiveActive)
    {
        if(Id==TEXT("mode")) c.GameMode=static_cast<Dive::Mode>((static_cast<int>(c.GameMode)+1)%3);
        else if(Id==TEXT("entry")) Diver->EntryIndex=(Diver->EntryIndex+1)%3;
        else if(Id==TEXT("hour")) c.Hour=FMath::Fmod(c.Hour+3,24.0);
        else if(Id==TEXT("tank")) c.CylinderLitres=c.CylinderLitres==10?12:c.CylinderLitres==12?15:10;
        else if(Id==TEXT("pressure")) c.StartPressureBar=c.StartPressureBar>=250?100:c.StartPressureBar+50;
        else if(Id==TEXT("weight")) c.WeightKg=c.WeightKg>=12?2:c.WeightKg+2;
        else if(Id==TEXT("suit")) c.DrySuit=!c.DrySuit;
        else if(Id==TEXT("visibility")) c.VisibilityMetres=c.VisibilityMetres>=22?6:c.VisibilityMetres+4;
        else if(Id==TEXT("current")) c.CurrentMetresPerSecond=c.CurrentMetresPerSecond>=.24?0:c.CurrentMetresPerSecond+.08;
        else if(Id==TEXT("depth")) c.PlannedDepthMetres=c.PlannedDepthMetres>=35?10:c.PlannedDepthMetres+5;
        else if(Id==TEXT("duration")) c.PlannedMinutes=c.PlannedMinutes>=60?10:c.PlannedMinutes+10;
    }
    Diver->SaveSettings();
}
