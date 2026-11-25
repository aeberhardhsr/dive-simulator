#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveAudio.h"
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
        if(bProfilesPage) ProfilesMenu(); else if(bControlsPage) ControlsMenu(); else if(Diver->bLogbook) Logbook(); else if(Diver->bSummary) Summary(); else Menu();
    }
    else
    {
        if(Diver->bShowInstruments) Instruments();
        if(Diver->bShowMap) Map();
        if(Diver->bTutorialActive) TutorialOverlay();
        if(GetWorld()->GetTimeSeconds()<Diver->NoticeUntil)
        {
            Rect(320,610,960,48,Dark); Text(Diver->Notice,340,622,17,White);
        }
        if(Diver->CanFinishDive())
        { Rect(570,680,460,48,Dark); Text(TEXT("[ ")+Diver->KeyText(EDiveAction::Finish)+TEXT(" ]  ")+Diver->Local(TEXT("Tauchgang abschliessen"),TEXT("Finish dive")),590,693,20,Accent); }
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
    Text(TEXT("swissBATHY3D + swissALTI3D + SWISSIMAGE / © swisstopo"),73,473,13,Muted);
    Text(TEXT("Includes material © (2016-2018) Planet. All rights reserved."),73,499,11,Muted);
    Text(L(TEXT("Pressluft, Tarierung und Vitalwerte"),TEXT("Breathing gas, buoyancy and vitals")),73,525,18,White);
    Text(L(TEXT("reagieren auf deinen Tauchgang."),TEXT("respond to the way you dive.")),73,554,18,White);
    Button(TEXT("environment"),bEnvironmentPage?L(TEXT("<   Ausruestung und Tauchplan"),TEXT("<   Equipment and dive plan")):L(TEXT("Wetter und Wasser   >"),TEXT("Weather and water   >")),73,590,437,42);
    Text(L(TEXT("Spielsimulation. Keine reale Tauchplanung."),TEXT("Game simulation. Not real dive planning.")),73,650,15,Muted);
    if(!Diver->bDiveActive) Button(TEXT("tutorial"),Diver->bTutorialCompleted?L(TEXT("Einfuehrung wiederholen  >"),TEXT("Replay introduction  >")):L(TEXT("Steuerung kennenlernen  >"),TEXT("Learn the controls  >")),73,676,437,32);
    Button(TEXT("logbook"),L(TEXT("Logbuch ansehen   >"),TEXT("Open logbook   >")),73,720,437,58);
    const float x=650,w=265,gap=28;
    Text(bAudioPage?L(TEXT("KLANG UND LAUTSTAERKE"),TEXT("SOUND AND VOLUME")):Diver->bDiveActive?L(TEXT("TAUCHGANG PAUSIERT"),TEXT("DIVE PAUSED")):L(TEXT("DEIN TAUCHGANG"),TEXT("YOUR DIVE")),x,160,23,White);
    Text(bAudioPage?L(TEXT("Auch in der Pause einstellbar. 0 % schaltet den Kanal stumm."),TEXT("Adjustable while paused. 0% mutes the channel.")):Diver->bDiveActive?L(TEXT("Tauchplan und Umwelt bleiben waehrend des Tauchgangs fest."),TEXT("Dive plan and environment are fixed during the dive.")):L(TEXT("Werte anklicken: Zahlen eingeben, Optionen wechseln"),TEXT("Click values to enter numbers or change options")),x,198,15,Muted);
    const bool ready=Diver->Environment && Diver->Environment->bTerrainReady;
    if(bAudioPage)
    {
        const FString labels[]={L(TEXT("GESAMT"),TEXT("MASTER")),L(TEXT("UMGEBUNG"),TEXT("ENVIRONMENT")),L(TEXT("ATEM / AUSRUESTUNG"),TEXT("BREATH / EQUIPMENT")),L(TEXT("HERZSCHLAG"),TEXT("HEARTBEAT")),L(TEXT("WARNSIGNALE"),TEXT("WARNINGS"))};
        for(int i=0;i<5;++i)
        {
            const float fx=x+(i%3)*(w+gap),fy=250+(i/3)*140;
            Text(labels[i],fx,fy,14,Muted);
            Button(FName(*FString::Printf(TEXT("audio_minus_%d"),i)),TEXT("-"),fx,fy+27,55,45);
            Text(FString::Printf(TEXT("%.0f %%"),Diver->AudioLevels.Values[i]*100),fx+85,fy+39,22,White);
            Button(FName(*FString::Printf(TEXT("audio_plus_%d"),i)),TEXT("+"),fx+w-55,fy+27,55,45);
        }
        Text(L(TEXT("Atmung und Puls folgen den simulierten Vitalwerten."),TEXT("Breathing and heartbeat follow the simulated vital signs.")),x,550,17,White);
        Text(L(TEXT("Wind, Regen und Wasser wechseln mit Wetter und Eintauchtiefe."),TEXT("Wind, rain and water blend with weather and immersion depth.")),x,585,16,Muted);
        Text(L(TEXT("Eigene synthetische Klaenge; Klangabstimmung in Entwicklung."),TEXT("Original synthesized sounds; sound design is in development.")),x,655,15,Muted);
    }
    else if(bEnvironmentPage)
    {
        const FString weather[]={L(TEXT("Heiter"),TEXT("Clear")),L(TEXT("Bedeckt"),TEXT("Overcast")),L(TEXT("Regen"),TEXT("Rain")),L(TEXT("Sturm"),TEXT("Storm"))};
        Field(TEXT("weather"),L(TEXT("WETTER"),TEXT("WEATHER")),weather[c.Weather],x,250,w);
        Field(TEXT("wind"),L(TEXT("WINDSTAERKE"),TEXT("WIND SPEED")),ConfigValue(TEXT("wind")),x+w+gap,250,w);
        Field(TEXT("winddir"),L(TEXT("WIND WEHT NACH"),TEXT("WIND BLOWS TOWARD")),ConfigValue(TEXT("winddir")),x+2*(w+gap),250,w);
        auto waterField=[&](FName id,const FString& label,const FString& value,float fx,float fy)
        {
            if(!c.AutoTemperature) { Field(id,label,value,fx,fy,w); return; }
            Text(label,fx,fy,14,Muted); Text(value,fx+12,fy+38,20,FLinearColor::White);
        };
        waterField(TEXT("surface_temp"),L(TEXT("OBERFLAECHENTEMPERATUR"),TEXT("SURFACE TEMPERATURE")),ConfigValue(TEXT("surface_temp")),x,350);
        waterField(TEXT("bottom_temp"),L(TEXT("TIEFENWASSER"),TEXT("DEEP WATER")),ConfigValue(TEXT("bottom_temp")),x+w+gap,350);
        waterField(TEXT("thermocline"),c.AutoTemperature?L(TEXT("VERLAUF"),TEXT("TEMPERATURE CURVE")):L(TEXT("SPRUNGSCHICHT / TIEFE"),TEXT("THERMOCLINE / DEPTH")),c.AutoTemperature?L(TEXT("Saisonprofil"),TEXT("Seasonal profile")):ConfigValue(TEXT("thermocline")),x+2*(w+gap),350);
        waterField(TEXT("thermal_width"),c.AutoTemperature?L(TEXT("PROFILBEREICH"),TEXT("PROFILE DEPTH")):L(TEXT("SCHICHTUEBERGANG"),TEXT("TRANSITION SCALE")),c.AutoTemperature?TEXT("0 – 144 m"):ConfigValue(TEXT("thermal_width")),x,450);
        Field(TEXT("visibility"),L(TEXT("SICHTWEITE"),TEXT("VISIBILITY")),ConfigValue(TEXT("visibility")),x+w+gap,450,w);
        Field(TEXT("current"),L(TEXT("STROEMUNG"),TEXT("CURRENT")),ConfigValue(TEXT("current")),x+2*(w+gap),450,w);
        Field(TEXT("hour"),L(TEXT("TAGESZEIT"),TEXT("TIME OF DAY")),ConfigValue(TEXT("hour")),x,550,w);
        Field(TEXT("date"),L(TEXT("SZENARIODATUM"),TEXT("SCENARIO DATE")),ConfigValue(TEXT("date")),x+2*(w+gap),550,w);
        Field(TEXT("air_temp"),L(TEXT("LUFTTEMPERATUR"),TEXT("AIR TEMPERATURE")),ConfigValue(TEXT("air_temp")),x+w+gap,550,w);
        const FString currentModes[]={L(TEXT("Manuell"),TEXT("Custom")),TEXT("Auto"),L(TEXT("Keine"),TEXT("None")),L(TEXT("Leicht"),TEXT("Light")),L(TEXT("Stark"),TEXT("Strong"))};
        Button(TEXT("auto_water"),L(TEXT("Wasser: "),TEXT("Water: "))+(c.AutoTemperature?TEXT("Auto"):*L(TEXT("Manuell"),TEXT("Custom"))),x,638,w,32);
        Button(TEXT("auto_visibility"),L(TEXT("Sicht: "),TEXT("Visibility: "))+(c.AutoVisibility?TEXT("Auto"):*L(TEXT("Manuell"),TEXT("Custom"))),x+w+gap,638,w,32);
        Button(TEXT("current_source"),L(TEXT("Stroemung: "),TEXT("Current: "))+currentModes[static_cast<int>(c.CurrentSource)],x+2*(w+gap),638,w,32);
        Text(L(TEXT("Automatik: saisonale Profile / Szenariowerte. Keine Live-Messung."),TEXT("Automatic: seasonal profiles / scenario values. No live measurements.")),x,683,15,Muted);
    }
    else
    {
    const FString modes[]={L(TEXT("Exploration"),TEXT("Exploration")),L(TEXT("Simulation"),TEXT("Simulation")),L(TEXT("Hardcore *"),TEXT("Hardcore *"))};
    Field(TEXT("mode"),L(TEXT("MODUS"),TEXT("MODE")),modes[static_cast<int>(c.GameMode)],x,250,w);
    Field(TEXT("entry"),c.StartOnShore?L(TEXT("UFERZUGANG"),TEXT("SHORE ACCESS")):L(TEXT("START IM SEE"),TEXT("OFFSHORE START")),ready?Diver->EntryName():TEXT("Geodata unavailable"),x+w+gap,250,w);
    Field(TEXT("hour"),L(TEXT("TAGESZEIT"),TEXT("TIME OF DAY")),ConfigValue(TEXT("hour")),x+2*(w+gap),250,w);
    Field(TEXT("tank"),L(TEXT("FLASCHENVOLUMEN"),TEXT("CYLINDER VOLUME")),FString::Printf(TEXT("%.0f L / O2 %.0f%%"),c.CylinderLitres,c.OxygenFraction*100),x,350,w);
    Field(TEXT("pressure"),L(TEXT("STARTDRUCK"),TEXT("START PRESSURE")),ConfigValue(TEXT("pressure")),x+w+gap,350,w);
    Field(TEXT("weight"),L(TEXT("BLEIGEWICHT"),TEXT("BALLAST")),ConfigValue(TEXT("weight")),x+2*(w+gap),350,w);
    Field(TEXT("suit"),L(TEXT("ANZUG"),TEXT("SUIT")),c.DrySuit?L(TEXT("Trockentauchanzug"),TEXT("Dry suit")):L(TEXT("Nassanzug"),TEXT("Wet suit")),x,450,w);
    const auto& torch=Dive::Torch::At(c.TorchModel);
    Field(TEXT("torch"),L(TEXT("TAUCHLAMPE"),TEXT("DIVE TORCH")),FString::Printf(TEXT("%s / %.0f lm"),UTF8_TO_TCHAR(torch.Name),torch.Lumens),x+w+gap,450,w);
    Text(L(TEXT("KEGEL / AKKU (SPIELMODELL)"),TEXT("BEAM / BATTERY (GAME MODEL)")),x+2*(w+gap),450,14,Muted);
    Text(FString::Printf(TEXT("%.0f deg / %.0f min"),2*torch.OuterDegrees,torch.RuntimeMinutes),x+2*(w+gap)+18,488,21,White);
    Field(TEXT("depth"),L(TEXT("GEPLANTE TIEFE"),TEXT("PLANNED DEPTH")),ConfigValue(TEXT("depth")),x,550,w);
    Field(TEXT("duration"),L(TEXT("GEPLANTE DAUER"),TEXT("PLANNED DURATION")),ConfigValue(TEXT("duration")),x+w+gap,550,w);
    Field(TEXT("motion"),L(TEXT("KAMERABEWEGUNG"),TEXT("CAMERA MOTION")),Diver->bCameraMotion?L(TEXT("Sanft"),TEXT("Gentle")):L(TEXT("Stabil"),TEXT("Stable")),x+2*(w+gap),550,w);
    Button(TEXT("shore"),c.StartOnShore?L(TEXT("Start: am Ufer"),TEXT("Start: on shore")):L(TEXT("Start: direkt im Wasser"),TEXT("Start: in the water")),x,625,350,32,c.StartOnShore);
    Text(c.GameMode==Dive::Mode::Exploration?L(TEXT("Unbegrenztes Atemgas und automatische Tarierung."),TEXT("Unlimited gas and automatic buoyancy.")):
        Diver->KeyText(EDiveAction::Inflate)+TEXT(" / ")+Diver->KeyText(EDiveAction::Deflate)+L(TEXT(": Jacket fuellen / entleeren. Luft dehnt sich beim Aufstieg aus."),TEXT(": inflate / deflate BCD. Air expands as you ascend.")),x,665,15,Muted);
    if(c.GameMode==Dive::Mode::Hardcore) Text(L(TEXT("* Bisher wie Simulation; Ausruestungsdefekte sind noch offen."),TEXT("* Currently simulation rules; equipment failures are pending.")),x,681,14,Amber);
    }
    Button(TEXT("start"),Diver->bDiveActive?L(TEXT("Tauchgang fortsetzen"),TEXT("Resume dive")):
        c.StartOnShore?L(TEXT("Am Ufer starten   >"),TEXT("Start on shore   >")):L(TEXT("Abtauchen   >"),TEXT("Begin dive   >")),x,720,558,58,true);
    if(!ready) Text(Diver->Environment?Diver->Environment->Terrain.Error:TEXT("Geodata unavailable"),x,690,14,Red);
    if(Diver->bDiveActive) Button(TEXT("abort"),L(TEXT("Tauchgang abbrechen"),TEXT("End dive early")),x+586,720,265,58);
    else Button(TEXT("quit"),L(TEXT("Beenden"),TEXT("Quit")),x+586,720,265,58);
    Line(70,815,1530,815,Muted,.5);
    Button(TEXT("language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),70,835,185,40);
    Button(TEXT("quality"),FString::Printf(TEXT("Grafik / Quality: %.0f"),Diver->GraphicsQuality),275,835,235,40);
    Button(TEXT("audio"),bAudioPage?TEXT("< Audio"):TEXT("Audio >"),530,835,105,40,bAudioPage);
    Button(TEXT("controls"),L(TEXT("F1  Steuerung / Hilfe"),TEXT("F1  Controls / help")),650,835,330,40);
    Button(TEXT("profiles"),L(TEXT("Tauchprofile"),TEXT("Dive profiles")),1010,835,260,40);
    Text(TEXT("ESC Pause"),1310,846,14,Muted);
}

void ADiveHUD::Instruments()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    const float yaw=FMath::Fmod(GetOwningPlayerController()->GetControlRotation().Yaw+360,360);
    const TCHAR* headingsDE[]={TEXT("N"),TEXT("NO"),TEXT("O"),TEXT("SO"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    const TCHAR* headingsEN[]={TEXT("N"),TEXT("NE"),TEXT("E"),TEXT("SE"),TEXT("S"),TEXT("SW"),TEXT("W"),TEXT("NW")};
    const int heading=FMath::RoundToInt(yaw/45)%8;
    Rect(640,32,320,58,Dark);
    Text(FString::Printf(TEXT("%s    %03d deg    %s"),Diver->bEnglish?headingsEN[heading]:headingsDE[heading],FMath::RoundToInt(yaw)%360,*Time(s.ElapsedSeconds)),672,50,20,White);
    const auto clock=Diver->Environment->CurrentMoment();
    const int clockMinutes=FMath::FloorToInt(clock.Hour*60+1.e-8)%1440;
    Text(FString::Printf(TEXT("%s  %02d:%02d %s"),UTF8_TO_TCHAR(clock.Date.Text().c_str()),clockMinutes/60,clockMinutes%60,
        clock.OffsetMinutes==120?TEXT("CEST"):TEXT("CET")),672,100,14,Muted);
    Text(TEXT("WALENSEE / ")+Diver->EntryName(),50,46,16,White);
    Text((Diver->PendingConfig.StartOnShore?L(TEXT("Rueckkehr zum Startpodest  /  "),TEXT("Return to the starting platform  /  ")):L(TEXT("Einstieg an der Boje  /  "),TEXT("Entry at buoy  /  ")))+Diver->KeyText(EDiveAction::Finish)+L(TEXT(" zum Beenden"),TEXT(" to finish")),50,76,13,Muted);
    if(Diver->PendingConfig.StartOnShore) Text(Diver->bWalking?L(TEXT("GEHEN / WATEN"),TEXT("WALKING / WADING")):L(TEXT("SCHWIMMEN"),TEXT("SWIMMING")),50,102,14,Accent);
    const FString source=s.BreathSource==Dive::BreathingSource::Air?L(TEXT("UMGEBUNGSLUFT"),TEXT("AMBIENT AIR")):
        s.BreathSource==Dive::BreathingSource::Regulator?L(TEXT("ATEMREGLER"),TEXT("REGULATOR")):L(TEXT("KEINE ATEMLUFT"),TEXT("NO BREATHING SUPPLY"));
    Text(source+TEXT("   [")+Diver->Controls.KeyText(EDiveAction::Regulator,Diver->bEnglish)+TEXT("] ")+L(TEXT("Regler ein / absetzen"),TEXT("Insert / remove regulator")),50,130,14,s.BreathSource==Dive::BreathingSource::None?Amber:Accent);
    if(s.BreathSource==Dive::BreathingSource::None||s.BreathReserve<.99)
        Text(L(TEXT("Atemreserve (Spiel): "),TEXT("Breath reserve (game): "))+FString::Printf(TEXT("%.0f%%"),s.BreathReserve*100),50,154,14,s.BreathReserve<.25?Amber:Accent);
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
    Text(Diver->KeyText(EDiveAction::Lamp)+L(TEXT(" Lampe   "),TEXT(" Torch   "))+Diver->KeyText(EDiveAction::Computer)+L(TEXT(" Handgelenk   "),TEXT(" Wrist   "))+Diver->KeyText(EDiveAction::Map)+L(TEXT(" Karte   "),TEXT(" Map   "))+TEXT("F1 ")+L(TEXT("Steuerung   ESC Pause"),TEXT("Controls   ESC Pause")),50,875,13,Muted);
    if(s.MaxDepthMetres>10 && s.DepthMetres<10 && s.StopRemainingSeconds>0)
    { Rect(1050,110,500,45,Dark); Text(L(TEXT("SICHERHEITSSTOPP  3-6 m   "),TEXT("SAFETY STOP  3-6 m   "))+Time(s.StopRemainingSeconds),1070,123,18,Accent); }
    int row=0;
    auto warning=[&](uint32 Flag,const TCHAR* German,const TCHAR* English)
    { if(s.Warnings&Flag) { Rect(1030,180+row*45,520,38,Dark); Text(TEXT("!  ")+L(German,English),1048,189+row*45,17,Amber); ++row; } };
    warning(Dive::NoBreathingSupply,TEXT("KEINE ATEMLUFT - REGLER / OBERFLAECHE"),TEXT("NO BREATHING SUPPLY - REGULATOR / SURFACE"));
    warning(Dive::OutOfGas,TEXT("FLASCHE LEER"),TEXT("CYLINDER EMPTY"));
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
    for(const auto& entry:data.EntryList(Diver->PendingConfig.StartOnShore)) point(entry.Position,Amber,7);
    point(Diver->GetActorLocation(),Accent,12);
    Text(Diver->Local(TEXT("Gruen: du    Gelb: Spielstarts    Dunkel: keine Messdaten"),TEXT("Green: you    Yellow: game starts    Dark: no survey data")),210,595,17,White);
    Text(Diver->Local(TEXT("Hell: flach    Dunkelblau: tief    Tiefenlinien: 10 m    Bezugspegel: 419 m LN02"),TEXT("Light: shallow    Dark blue: deep    Contours: 10 m    Water reference: 419 m LN02")),210,631,16,Muted);
    const FVector2D survey=data.ToSurvey(Diver->GetActorLocation());
    Text(FString::Printf(TEXT("LV95  E %.1f / N %.1f     |     %s: %s"),survey.X,survey.Y,*Diver->KeyText(EDiveAction::Map),Diver->bEnglish?TEXT("close"):TEXT("schliessen")),210,678,16,White);
}

void ADiveHUD::Summary()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& s=Diver->Simulation.Data;
    Text(TEXT("D I V E   /   L O G"),120,80,18,Accent);
    Text(Diver->Tutorial.Stage==Dive::Lesson::Done?L(TEXT("Einfuehrung abgeschlossen."),TEXT("Introduction complete.")):L(TEXT("Dein Tauchgang."),TEXT("Your dive.")),120,137,54,White);
    const FString outcome=Diver->Outcome==TEXT("completed")?L(TEXT("Zurueck an der Oberflaeche"),TEXT("Back at the surface")):
        Diver->Outcome==TEXT("no_breathing_supply")?L(TEXT("Abbruch: keine Atemluft"),TEXT("Ended: no breathing supply")):
        Diver->Outcome==TEXT("out_of_gas")?L(TEXT("Abbruch: Flasche leer"),TEXT("Ended: cylinder empty")):Diver->Outcome;
    Text(outcome,122,215,19,Muted);
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
    Super::NotifyHitBoxClick(Id); if(!Diver||ConfigDialog) return;
    if(Id==TEXT("profiles")) { OpenProfiles(); return; }
    if(bProfilesPage) { ProfileClick(Id); return; }
    if(Id==TEXT("controls")) { OpenControls(); return; }
    if(Id==TEXT("tutorial")) { if(!Diver->bDiveActive) { bControlsPage=false; Diver->StartTutorial(); } return; }
    if(bControlsPage)
    {
        if(Id==TEXT("controls_back")) { bControlsPage=false; Diver->CapturingAction=INDEX_NONE; }
        else if(Id==TEXT("controls_reset")) { Diver->Controls=FDiveControls{}; Diver->CapturingAction=INDEX_NONE; Diver->ControlMessage=Diver->Local(TEXT("Standardbelegung wiederhergestellt."),TEXT("Default controls restored.")); }
        else if(Id==TEXT("controls_language")) { Diver->bEnglish=!Diver->bEnglish; Diver->ControlMessage.Empty(); }
        else if(Id.ToString().StartsWith(TEXT("bind_"))) { Diver->CapturingAction=FCString::Atoi(*Id.ToString().Mid(5)); Diver->ControlMessage.Empty(); return; }
        else if(Id==TEXT("sensitivity_minus")) Diver->Controls.Sensitivity-=.03;
        else if(Id==TEXT("sensitivity_plus")) Diver->Controls.Sensitivity+=.03;
        else if(Id==TEXT("invert_y")) Diver->Controls.InvertY=!Diver->Controls.InvertY;
        else if(Id==TEXT("fov_minus")) Diver->Controls.FieldOfView-=5;
        else if(Id==TEXT("fov_plus")) Diver->Controls.FieldOfView+=5;
        Diver->ApplyControls(); Diver->SaveSettings(); return;
    }
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
    if(Id==TEXT("environment")) { bEnvironmentPage=!bEnvironmentPage; bAudioPage=false; return; }
    if(Id==TEXT("audio")) { bAudioPage=!bAudioPage; return; }
    if(Id.ToString().StartsWith(TEXT("audio_minus_"))||Id.ToString().StartsWith(TEXT("audio_plus_")))
    {
        const FString name=Id.ToString();
        const int i=FCString::Atoi(*name.Right(1));
        if(i>=0&&i<5)
        {
            Diver->AudioLevels.Values[i]=FMath::Clamp(Diver->AudioLevels.Values[i]+(name.StartsWith(TEXT("audio_plus_"))?.05:-.05),0.,1.);
            Diver->Audio->ApplyLevels(Diver->AudioLevels); Diver->SaveSettings();
        }
        return;
    }
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
        if(OpenConfigInput(Id)) return;
        if(Id==TEXT("mode")) c.GameMode=static_cast<Dive::Mode>((static_cast<int>(c.GameMode)+1)%3);
        else if(Id==TEXT("entry")) Diver->EntryIndex=(Diver->EntryIndex+1)%3;
        else if(Id==TEXT("shore")) c.StartOnShore=!c.StartOnShore;
        else if(Id==TEXT("tank")) c.CylinderLitres=c.CylinderLitres==10?12:c.CylinderLitres==12?15:10;
        else if(Id==TEXT("suit")) c.DrySuit=!c.DrySuit;
        else if(Id==TEXT("torch")) c.TorchModel=(c.TorchModel+1)%Dive::Torch::Count;
        else if(Id==TEXT("weather")) c.SelectWeather((c.Weather+1)%Dive::WeatherScenario::Count);
        else if(Id==TEXT("auto_water")) c.AutoTemperature=!c.AutoTemperature;
        else if(Id==TEXT("auto_visibility")) c.AutoVisibility=!c.AutoVisibility;
        else if(Id==TEXT("current_source")) c.CurrentSource=static_cast<Dive::CurrentSetting>((static_cast<int>(c.CurrentSource)+1)%5);
    }
    Diver->SaveSettings();
}
