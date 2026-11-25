#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
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
        if(Diver->bLogbook) Logbook(); else if(Diver->bSummary) Summary(); else Menu();
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
    Text(L(TEXT("Vermessener Walensee-Seegrund, Massstab 1:1."),TEXT("Surveyed Walensee lakebed, true scale.")),73,444,17,White);
    Text(TEXT("swissBATHY3D + swissALTI3D / © swisstopo"),73,473,17,Muted);
    Text(L(TEXT("Pressluft, Tarierung und Vitalwerte"),TEXT("Breathing gas, buoyancy and vitals")),73,525,18,White);
    Text(L(TEXT("reagieren auf deinen Tauchgang."),TEXT("respond to the way you dive.")),73,554,18,White);
    Button(TEXT("environment"),bEnvironmentPage?L(TEXT("<   Ausruestung und Tauchplan"),TEXT("<   Equipment and dive plan")):L(TEXT("Wetter und Wasser   >"),TEXT("Weather and water   >")),73,590,437,42);
    Text(L(TEXT("Spielsimulation. Keine reale Tauchplanung."),TEXT("Game simulation. Not real dive planning.")),73,650,15,Muted);
    Button(TEXT("logbook"),L(TEXT("Logbuch ansehen   >"),TEXT("Open logbook   >")),73,720,437,58);
    const float x=650,w=265,gap=28;
    Text(Diver->bDiveActive?L(TEXT("TAUCHGANG PAUSIERT"),TEXT("DIVE PAUSED")):L(TEXT("DEIN TAUCHGANG"),TEXT("YOUR DIVE")),x,160,23,White);
    Text(Diver->bDiveActive?L(TEXT("Tauchplan und Umwelt bleiben waehrend des Tauchgangs fest."),TEXT("Dive plan and environment are fixed during the dive.")):L(TEXT("Werte anklicken, um sie zu wechseln"),TEXT("Click a value to cycle options")),x,198,15,Muted);
    const bool ready=Diver->Environment && Diver->Environment->bTerrainReady;
    if(bEnvironmentPage)
    {
        const FString weather[]={L(TEXT("Heiter"),TEXT("Clear")),L(TEXT("Bedeckt"),TEXT("Overcast")),L(TEXT("Regen"),TEXT("Rain"))};
        Field(TEXT("weather"),L(TEXT("WETTER"),TEXT("WEATHER")),weather[c.Weather],x,250,w);
        Field(TEXT("wind"),L(TEXT("WINDSTAERKE"),TEXT("WIND SPEED")),FString::Printf(TEXT("%.0f m/s"),c.WindMetresPerSecond),x+w+gap,250,w);
        Field(TEXT("winddir"),L(TEXT("WIND WEHT NACH"),TEXT("WIND BLOWS TOWARD")),FString::Printf(TEXT("%03.0f deg"),c.WindDirectionDegrees),x+2*(w+gap),250,w);
        Field(TEXT("surface_temp"),L(TEXT("OBERFLAECHENTEMPERATUR"),TEXT("SURFACE TEMPERATURE")),FString::Printf(TEXT("%.0f C"),c.SurfaceTemperatureC),x,350,w);
        Field(TEXT("bottom_temp"),L(TEXT("TIEFENWASSER"),TEXT("DEEP WATER")),FString::Printf(TEXT("%.0f C"),c.BottomTemperatureC),x+w+gap,350,w);
        Field(TEXT("thermocline"),L(TEXT("SPRUNGSCHICHT / TIEFE"),TEXT("THERMOCLINE / DEPTH")),FString::Printf(TEXT("%.0f m"),c.ThermoclineMetres),x+2*(w+gap),350,w);
        Field(TEXT("thermal_width"),L(TEXT("SCHICHTUEBERGANG"),TEXT("TRANSITION SCALE")),FString::Printf(TEXT("%.1f m"),c.ThermoclineWidthMetres),x,450,w);
        Field(TEXT("visibility"),L(TEXT("SICHTWEITE"),TEXT("VISIBILITY")),FString::Printf(TEXT("%.0f m"),c.VisibilityMetres),x+w+gap,450,w);
        Field(TEXT("current"),L(TEXT("STROEMUNG"),TEXT("CURRENT")),FString::Printf(TEXT("%.2f m/s"),c.CurrentMetresPerSecond),x+2*(w+gap),450,w);
        Field(TEXT("hour"),L(TEXT("TAGESZEIT"),TEXT("TIME OF DAY")),FString::Printf(TEXT("%02d:00"),static_cast<int>(c.Hour)),x,550,w);
        Text(L(TEXT("Einstellbare Spielszenarien; keine aktuellen Messwerte."),TEXT("Configurable game scenarios; not current observations.")),x,655,15,Muted);
    }
    else
    {
    const FString modes[]={L(TEXT("Exploration"),TEXT("Exploration")),L(TEXT("Simulation"),TEXT("Simulation")),L(TEXT("Hardcore *"),TEXT("Hardcore *"))};
    Field(TEXT("mode"),L(TEXT("MODUS"),TEXT("MODE")),modes[static_cast<int>(c.GameMode)],x,250,w);
    Field(TEXT("entry"),L(TEXT("START IM SEE"),TEXT("OFFSHORE START")),ready?Diver->Environment->Terrain.Entries[Diver->EntryIndex].Name:TEXT("Geodata unavailable"),x+w+gap,250,w);
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
    }
    Button(TEXT("start"),Diver->bDiveActive?L(TEXT("Tauchgang fortsetzen"),TEXT("Resume dive")):L(TEXT("Abtauchen   >"),TEXT("Begin dive   >")),x,720,558,58,true);
    if(!ready) Text(Diver->Environment?Diver->Environment->Terrain.Error:TEXT("Geodata unavailable"),x,690,14,Red);
    if(Diver->bDiveActive) Button(TEXT("abort"),L(TEXT("Tauchgang abbrechen"),TEXT("End dive early")),x+586,720,265,58);
    else Button(TEXT("quit"),L(TEXT("Beenden"),TEXT("Quit")),x+586,720,265,58);
    Line(70,815,1530,815,Muted,.5);
    Button(TEXT("language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),70,835,185,40);
    Button(TEXT("quality"),FString::Printf(TEXT("Grafik / Quality: %.0f"),Diver->GraphicsQuality),275,835,235,40);
    Text(L(TEXT("WASD Schwimmen  SPACE/CTRL Hoehe  Q/E BCD  L Licht  C Handgelenk  R Seite  M Karte"),
        TEXT("WASD Swim  SPACE/CTRL Vertical  Q/E BCD  L Torch  C Wrist  R Page  M Map")),650,846,14,Muted);
}

void ADiveHUD::Instruments()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    const float yaw=FMath::Fmod(GetOwningPlayerController()->GetControlRotation().Yaw+360,360);
    Rect(640,32,320,58,Dark);
    Text(FString::Printf(TEXT("N    %03.0f deg    %s"),yaw,*Time(s.ElapsedSeconds)),672,50,20,White);
    Text(TEXT("WALENSEE / ")+Diver->Environment->Terrain.Entries[Diver->EntryIndex].Name,50,46,16,White);
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
    Text(L(TEXT("L Lampe   C Handgelenk   R Seite   M Karte   P Foto   H HUD   ESC Pause"),TEXT("L Torch   C Wrist   R Page   M Map   P Photo   H HUD   ESC Pause")),50,875,13,Muted);
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

}

void ADiveHUD::Map()
{
    const auto& data=Diver->Environment->Terrain;
    if(!MapTexture && data.Overview.Num()>0)
    {
        MapTexture=UTexture2D::CreateTransient(FDiveTerrain::MapWidth,FDiveTerrain::MapHeight,PF_B8G8R8A8);
        auto* pixels=static_cast<FColor*>(MapTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
        for(int i=0;i<data.Overview.Num();++i)
        {
            const float depth=data.Overview[i];
            if(depth<0) { pixels[i]=FColor(11,22,25); continue; }
            const float t=FMath::Clamp(depth/155.f,0.f,1.f);
            FLinearColor color=FMath::Lerp(FLinearColor(.18,.62,.46),FLinearColor(.015,.07,.16),t);
            // Ten-metre contours are derived from the measured raster.
            if(i%FDiveTerrain::MapWidth>0 && data.Overview[i-1]>=0 &&
               FMath::FloorToInt(depth/10)!=FMath::FloorToInt(data.Overview[i-1]/10)) color*=.55f;
            pixels[i]=color.ToFColor(true);
        }
        MapTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
        MapTexture->Filter=TF_Bilinear; MapTexture->SRGB=true; MapTexture->UpdateResource();
    }
    Rect(180,150,1240,580,Dark);
    Text(Diver->Local(TEXT("WALENSEE / VERMESSENE TIEFENKARTE"),TEXT("WALENSEE / SURVEYED DEPTH MAP")),210,180,23,Accent);
    Text(TEXT("N ^      swissBATHY3D 2020 / © swisstopo"),210,222,15,Muted);
    if(MapTexture) DrawTexture(MapTexture,OffsetX+210*Scale,OffsetY+270*Scale,1180*Scale,295*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
    auto point=[&](FVector pos,FLinearColor color,float size)
    {
        const FVector2D survey=data.ToSurvey(pos);
        const float x=210+(survey.X-data.MapWest)/(data.MapEast-data.MapWest)*1180;
        const float y=270+(data.MapNorth-survey.Y)/(data.MapNorth-data.MapSouth)*295;
        Rect(x-size*.5f,y-size*.5f,size,size,color);
    };
    for(int i=0;i<Diver->Samples.Num();i+=5) point(Diver->Samples[i].Position,FLinearColor(.75,.85,.8,1),3);
    for(const auto& entry:data.Entries) point(entry.Position,Amber,7);
    point(Diver->GetActorLocation(),Accent,12);
    Text(Diver->Local(TEXT("Gruen: du    Gelb: Spielstarts    Dunkel: keine Messdaten"),TEXT("Green: you    Yellow: game starts    Dark: no survey data")),210,595,17,White);
    Text(Diver->Local(TEXT("Hell: flach    Dunkelblau: tief    Tiefenlinien: 10 m    Bezugspegel: 419 m LN02"),TEXT("Light: shallow    Dark blue: deep    Contours: 10 m    Water reference: 419 m LN02")),210,631,16,Muted);
    const FVector2D survey=data.ToSurvey(Diver->GetActorLocation());
    Text(FString::Printf(TEXT("LV95  E %.1f / N %.1f     |     M: %s"),survey.X,survey.Y,Diver->bEnglish?TEXT("close"):TEXT("schliessen")),210,678,16,White);
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
    Button(TEXT("logbook"),L(TEXT("Logbuch / Verlaeufe"),TEXT("Logbook / profiles")),690,785,490,60);
    Button(TEXT("quit"),L(TEXT("Beenden"),TEXT("Quit")),1215,785,265,60);
}

void ADiveHUD::NotifyHitBoxClick(FName Id)
{
    Super::NotifyHitBoxClick(Id); if(!Diver) return;
    if(Id==TEXT("logbook")) { OpenLogbook(Diver->bSummary?Diver->LastLogPath:FString()); return; }
    if(Diver->bLogbook)
    {
        if(Id==TEXT("logback")) Diver->bLogbook=false;
        else if(Id==TEXT("loglanguage")) { Diver->bEnglish=!Diver->bEnglish; Diver->SaveSettings(); }
        else if(Id==TEXT("logprev")) LogPage=FMath::Max(0,LogPage-1);
        else if(Id==TEXT("lognext")) LogPage=FMath::Min((VisibleLogs().Num()-1)/7,LogPage+1);
        else if(Id==TEXT("logfilter")&&!bIndexLoading&&!bDetailLoading)
        {
            LogFilter=(LogFilter+1)%4; LogPage=0;
            const auto visible=VisibleLogs();
            if(!visible.IsEmpty()) SelectLog(visible[0]);
            else { LogSelection=INDEX_NONE; SelectedLog=FDiveLog{}; }
        }
        else if(Id.ToString().StartsWith(TEXT("logrow_"))) SelectLog(FCString::Atoi(*Id.ToString().Mid(7)));
        else if(Id.ToString().StartsWith(TEXT("logmetric_"))) LogMetricGroup=FMath::Clamp(FCString::Atoi(*Id.ToString().Mid(10)),0,3);
        else if(Id==TEXT("logeventprev")||Id==TEXT("logeventnext"))
        {
            const int direction=Id==TEXT("logeventnext")?1:-1;
            for(int i=LogCursor+direction;SelectedLog.Samples.IsValidIndex(i);i+=direction)
            {
                const uint32 previous=i>0?SelectedLog.Samples[i-1].Warnings:0;
                if(SelectedLog.Samples[i].Warnings&~previous) { LogCursor=i; break; }
            }
        }
        return;
    }
    auto& c=Diver->PendingConfig;
    if(Id==TEXT("environment")) { bEnvironmentPage=!bEnvironmentPage; return; }
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
        else if(Id==TEXT("weather")) c.Weather=(c.Weather+1)%3;
        else if(Id==TEXT("wind")) c.WindMetresPerSecond=c.WindMetresPerSecond>=12?0:c.WindMetresPerSecond+3;
        else if(Id==TEXT("winddir")) c.WindDirectionDegrees=FMath::Fmod(c.WindDirectionDegrees+45,360.);
        else if(Id==TEXT("surface_temp")) c.SurfaceTemperatureC=c.SurfaceTemperatureC>=24?6:c.SurfaceTemperatureC+2;
        else if(Id==TEXT("bottom_temp")) c.BottomTemperatureC=c.BottomTemperatureC>=12?4:c.BottomTemperatureC+2;
        else if(Id==TEXT("thermocline")) c.ThermoclineMetres=c.ThermoclineMetres>=25?5:c.ThermoclineMetres+5;
        else if(Id==TEXT("thermal_width")) c.ThermoclineWidthMetres=c.ThermoclineWidthMetres>=5?.5:c.ThermoclineWidthMetres+.5;
    }
    Diver->SaveSettings();
}
