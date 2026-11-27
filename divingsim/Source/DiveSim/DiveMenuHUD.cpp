#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"

namespace
{
const FLinearColor White(.87,.94,.92,1),Muted(.52,.66,.64,1),Accent(.34,.85,.72,1);
const FLinearColor Panel(.012,.032,.032,.94),Red(1,.35,.25,1);
}

void ADiveHUD::Menu()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    auto& c=Diver->PendingConfig;
    const bool ready=Diver->Environment&&Diver->Environment->bTerrainReady;
    const float x=470,w=310,gap=20;
    Rect(440,42,1050,730,Panel);
    Text(TEXT("D I V E  /  0 0 1"),110,51,14,Accent);
    Text(TEXT("WALENSEE"),108,80,43,White);
    Text(L(TEXT("Unter der Oberfläche."),TEXT("Below the surface.")),110,139,18,White);
    Text(L(TEXT("Ruhe. Tiefe. Entdecken."),TEXT("Stillness. Depth. Discovery.")),110,166,14,Muted);
    Button(TEXT("plan"),L(TEXT("01   Tauchplan"),TEXT("01   Dive plan")),110,220,290,48,!bEnvironmentPage&&!bLoadoutPage&&!bAudioPage);
    Button(TEXT("environment"),L(TEXT("02   Wetter & Wasser"),TEXT("02   Weather & water")),110,280,290,48,bEnvironmentPage&&!bAudioPage);
    Button(TEXT("loadout"),L(TEXT("03   Ausrüstung & Tarierung"),TEXT("03   Equipment & buoyancy")),110,340,290,48,bLoadoutPage);
    Line(110,416,400,416,Muted,.5);
    Text(L(TEXT("ARCHIV & ENTDECKUNGEN"),TEXT("ARCHIVE & DISCOVERIES")),110,437,13,Muted);
    Button(TEXT("logbook"),L(TEXT("Logbuch ansehen"),TEXT("Open logbook")),110,472,290,40);
    Button(TEXT("profiles"),L(TEXT("Tauchprofile"),TEXT("Dive presets")),110,522,290,40);
    Button(TEXT("discoveries"),L(TEXT("Arten & Fotos"),TEXT("Species & photographs")),110,572,290,40);
    if(!Diver->bDiveActive) Button(TEXT("tutorial"),Diver->bTutorialCompleted?L(TEXT("Einführung wiederholen"),TEXT("Replay introduction")):L(TEXT("Steuerung kennenlernen"),TEXT("Learn the controls")),110,632,290,40);
    TextBlock(L(TEXT("Spielsimulation. Keine reale Tauchplanung."),TEXT("Game simulation. Not real dive planning.")),110,678,290,13,Muted,2);
    TextBlock(TEXT("© swisstopo\nswissBATHY3D / swissALTI3D / swissTLM3D\nSWISSIMAGE / swissBUILDINGS3D\nIncludes material © (2016–2018) Planet.\nAll rights reserved."),110,720,290,11,Muted,5);

    Text(Diver->bDiveActive?L(TEXT("TAUCHGANG PAUSIERT"),TEXT("DIVE PAUSED")):L(TEXT("DEIN NÄCHSTER TAUCHGANG"),TEXT("YOUR NEXT DIVE")),x,66,13,Muted);
    Text(bLoadoutPage?L(TEXT("Ausrüstung & Tarierung"),TEXT("Equipment & buoyancy")):bAudioPage?L(TEXT("Klang & Lautstärke"),TEXT("Sound & volume")):bEnvironmentPage?L(TEXT("Wetter & Wasserbedingungen"),TEXT("Weather & water conditions")):Diver->bDiveActive?L(TEXT("Dein Tauchgang"),TEXT("Your dive")):L(TEXT("Dein Tauchplan"),TEXT("Your dive plan")),x,96,27,White);
    TextBlock(Diver->bDiveActive?L(TEXT("Der Tauchgang bleibt pausiert. Ausrüstung und Bedingungen sind gesperrt."),TEXT("The dive remains paused. Equipment and conditions are locked.")):L(TEXT("Feld auswählen: Zahlen eingeben oder Optionen wechseln. Änderungen werden gespeichert."),TEXT("Select a field to enter a number or change an option. Changes are saved.")),x,137,970,14,Muted,1);
    Line(x,171,1460,171,Muted,.5);

    if(bLoadoutPage) LoadoutMenu();
    else if(bAudioPage)
    {
        const FString labels[]={L(TEXT("GESAMT"),TEXT("MASTER")),L(TEXT("UMGEBUNG"),TEXT("ENVIRONMENT")),L(TEXT("ATEM & AUSRÜSTUNG"),TEXT("BREATH & EQUIPMENT")),L(TEXT("HERZSCHLAG"),TEXT("HEARTBEAT")),L(TEXT("WARNSIGNALE"),TEXT("WARNINGS"))};
        for(int i=0;i<5;++i)
        {
            const float fx=x+(i%3)*(w+gap),fy=218+(i/3)*145;
            Text(labels[i],fx,fy,14,Muted);
            Button(FName(*FString::Printf(TEXT("audio_minus_%d"),i)),TEXT("−"),fx,fy+34,52,42);
            Text(FString::Printf(TEXT("%.0f %%"),Diver->AudioLevels.Values[i]*100),fx+110,fy+43,24,White);
            Button(FName(*FString::Printf(TEXT("audio_plus_%d"),i)),TEXT("+"),fx+w-52,fy+34,52,42);
        }
        TextBlock(L(TEXT("Atmung und Puls folgen den simulierten Vitalwerten. Wind, Regen und Wasser wechseln mit Wetter und Eintauchtiefe."),TEXT("Breathing and heartbeat follow the simulated vital signs. Wind, rain and water follow the weather and immersion depth.")),x,550,850,18,White,3);
        TextBlock(L(TEXT("Auch während der Pause einstellbar. 0 % schaltet den jeweiligen Kanal stumm."),TEXT("Adjustable while paused. Set a channel to 0% to mute it.")),x,654,850,16,Muted,2);
    }
    else if(bEnvironmentPage)
    {
        const float ew=310;
        Text(L(TEXT("01 · ATMOSPHÄRE & DATUM"),TEXT("01 · ATMOSPHERE & DATE")),x,194,13,Accent);
        const FString weather[]={L(TEXT("Heiter"),TEXT("Clear")),L(TEXT("Bedeckt"),TEXT("Overcast")),L(TEXT("Regen"),TEXT("Rain")),L(TEXT("Sturm"),TEXT("Storm"))};
        Field(TEXT("weather"),L(TEXT("WETTER"),TEXT("WEATHER")),weather[c.Weather],x,220,ew);
        Field(TEXT("wind"),L(TEXT("WINDSTÄRKE"),TEXT("WIND SPEED")),ConfigValue(TEXT("wind")),x+w+gap,220,ew);
        Field(TEXT("winddir"),L(TEXT("WIND WEHT NACH"),TEXT("WIND BLOWS TOWARD")),ConfigValue(TEXT("winddir")),x+2*(w+gap),220,ew);
        Field(TEXT("air_temp"),L(TEXT("LUFTTEMPERATUR"),TEXT("AIR TEMPERATURE")),ConfigValue(TEXT("air_temp")),x,310,ew);
        Field(TEXT("date"),L(TEXT("SZENARIODATUM"),TEXT("SCENARIO DATE")),ConfigValue(TEXT("date")),x+w+gap,310,ew);
        Field(TEXT("hour"),L(TEXT("TAGESZEIT"),TEXT("TIME OF DAY")),ConfigValue(TEXT("hour")),x+2*(w+gap),310,ew);
        Text(L(TEXT("02 · WASSERSCHICHTUNG & SICHT"),TEXT("02 · WATER LAYERS & VISIBILITY")),x,410,13,Accent);
        auto water=[&](FName id,const FString& label,const FString& value,float fx,float fy)
        {
            if(!c.AutoTemperature) {Field(id,label,value,fx,fy,w);return;}
            Rect(fx,fy,w,78,FLinearColor(.025,.065,.063,.92));
            Text(label,fx+14,fy+10,13,Muted);TextBlock(value,fx+14,fy+32,w-28,18,White,2);
        };
        water(TEXT("surface_temp"),L(TEXT("OBERFLÄCHE"),TEXT("SURFACE")),ConfigValue(TEXT("surface_temp")),x,438);
        water(TEXT("bottom_temp"),L(TEXT("TIEFENWASSER"),TEXT("DEEP WATER")),ConfigValue(TEXT("bottom_temp")),x+w+gap,438);
        water(TEXT("thermocline"),L(TEXT("SPRUNGSCHICHT"),TEXT("THERMOCLINE")),c.AutoTemperature?L(TEXT("Saisonprofil"),TEXT("Seasonal profile")):ConfigValue(TEXT("thermocline")),x+2*(w+gap),438);
        water(TEXT("thermal_width"),c.AutoTemperature?L(TEXT("PROFILBEREICH"),TEXT("PROFILE DEPTH")):L(TEXT("SCHICHTÜBERGANG"),TEXT("TRANSITION SCALE")),c.AutoTemperature?TEXT("0–144 m"):ConfigValue(TEXT("thermal_width")),x,528);
        Field(TEXT("visibility"),L(TEXT("SICHTWEITE"),TEXT("VISIBILITY")),ConfigValue(TEXT("visibility")),x+w+gap,528,w);
        Field(TEXT("current"),L(TEXT("STRÖMUNG"),TEXT("CURRENT")),ConfigValue(TEXT("current")),x+2*(w+gap),528,w);
        const FString currentModes[]={L(TEXT("Manuell"),TEXT("Custom")),TEXT("Auto"),L(TEXT("Keine"),TEXT("None")),L(TEXT("Leicht"),TEXT("Light")),L(TEXT("Stark"),TEXT("Strong"))};
        Button(TEXT("auto_water"),L(TEXT("Wasser: "),TEXT("Water: "))+(c.AutoTemperature?TEXT("Auto"):*L(TEXT("Manuell"),TEXT("Custom"))),x,634,w,38,false,!Diver->bDiveActive);
        Button(TEXT("auto_visibility"),L(TEXT("Sicht: "),TEXT("Visibility: "))+(c.AutoVisibility?TEXT("Auto"):*L(TEXT("Manuell"),TEXT("Custom"))),x+w+gap,634,w,38,false,!Diver->bDiveActive);
        Button(TEXT("current_source"),L(TEXT("Strömung: "),TEXT("Current: "))+currentModes[static_cast<int>(c.CurrentSource)],x+2*(w+gap),634,w,38,false,!Diver->bDiveActive);
        TextBlock(L(TEXT("Automatik verwendet saisonale Profile und Szenariowerte. Keine Live-Messung."),TEXT("Automatic settings use seasonal profiles and scenario values. No live measurements.")),x,698,950,15,Muted,2);
    }
    else if(Diver->bDiveActive) PauseDashboard();
    else
    {
        Text(L(TEXT("01 · EINSTIEG & MODUS"),TEXT("01 · ENTRY & MODE")),x,194,13,Accent);
        const FString modes[]={TEXT("Exploration"),TEXT("Simulation"),TEXT("Hardcore")};
        Field(TEXT("mode"),L(TEXT("MODUS"),TEXT("MODE")),modes[static_cast<int>(c.GameMode)],x,220,w);
        Field(TEXT("entry"),c.StartOnShore?L(TEXT("UFERZUGANG"),TEXT("SHORE ACCESS")):L(TEXT("START IM SEE"),TEXT("OFFSHORE START")),ready?Diver->EntryName():L(TEXT("Gelände wird geladen"),TEXT("Loading terrain")),x+w+gap,220,w);
        Field(TEXT("hour"),L(TEXT("TAGESZEIT"),TEXT("TIME OF DAY")),ConfigValue(TEXT("hour")),x+2*(w+gap),220,w);
        Text(L(TEXT("02 · GAS & AUSRÜSTUNG"),TEXT("02 · GAS & EQUIPMENT")),x,316,13,Accent);
        Field(TEXT("tank"),L(TEXT("FLASCHENVOLUMEN"),TEXT("CYLINDER VOLUME")),FString::Printf(TEXT("%.0f L / O₂ %.0f %%"),c.CylinderLitres,c.OxygenFraction*100),x,342,w);
        Field(TEXT("pressure"),L(TEXT("STARTDRUCK"),TEXT("START PRESSURE")),ConfigValue(TEXT("pressure")),x+w+gap,342,w);
        Field(TEXT("weight"),L(TEXT("BLEIGEWICHT"),TEXT("BALLAST")),ConfigValue(TEXT("weight")),x+2*(w+gap),342,w);
        Field(TEXT("suit"),L(TEXT("ANZUG"),TEXT("SUIT")),c.DrySuit?L(TEXT("Trockentauchanzug"),TEXT("Dry suit")):L(TEXT("Nassanzug"),TEXT("Wet suit")),x,432,w);
        const auto& torch=Dive::Torch::At(c.TorchModel);
        Field(TEXT("torch"),L(TEXT("TAUCHLAMPE"),TEXT("DIVE TORCH")),FString::Printf(TEXT("%s / %.0f lm"),UTF8_TO_TCHAR(torch.Name),torch.Lumens),x+w+gap,432,w);
        Field(TEXT("motion"),L(TEXT("KAMERABEWEGUNG"),TEXT("CAMERA MOTION")),Diver->bCameraMotion?L(TEXT("Sanft"),TEXT("Gentle")):L(TEXT("Stabil"),TEXT("Stable")),x+2*(w+gap),432,w);
        Text(L(TEXT("03 · GEPLANTES PROFIL"),TEXT("03 · PLANNED PROFILE")),x,528,13,Accent);
        Field(TEXT("depth"),L(TEXT("GEPLANTE TIEFE"),TEXT("PLANNED DEPTH")),ConfigValue(TEXT("depth")),x,554,w);
        Field(TEXT("duration"),L(TEXT("GEPLANTE DAUER"),TEXT("PLANNED DURATION")),ConfigValue(TEXT("duration")),x+w+gap,554,w);
        const Dive::Model prepared(c);
        const double net=prepared.Data.DisplacedLitres-prepared.Data.TotalMassKg;
        Text(L(TEXT("TARIER-CHECK · SÜSSWASSER"),TEXT("BUOYANCY CHECK · FRESH WATER")),x+2*(w+gap)+14,564,13,Muted);
        Text(FString::Printf(TEXT("%+.2f kg"),net),x+2*(w+gap)+14,591,24,Accent);
        Button(TEXT("shore"),c.StartOnShore?L(TEXT("Start: am Ufer"),TEXT("Start: on shore")):L(TEXT("Start: direkt im Wasser"),TEXT("Start: in the water")),x,652,w,36,c.StartOnShore,!Diver->bDiveActive);
        if(c.GameMode==Dive::Mode::Exploration)
            Button(TEXT("exploration_trim"),c.ManualExplorationBuoyancy?L(TEXT("Tarierung: manuell"),TEXT("Buoyancy: manual")):L(TEXT("Tarierung: automatisch"),TEXT("Buoyancy: assisted")),x+w+gap,652,w,36,false,!Diver->bDiveActive);
        TextBlock(c.GameMode==Dive::Mode::Exploration?L(TEXT("Unbegrenztes Atemgas. Keine körperlichen Folgen. Instrumente bleiben sichtbar."),TEXT("Unlimited gas. No physiological consequences. Instruments remain visible.")):
            c.GameMode==Dive::Mode::Hardcore?L(TEXT("Körperliche Folgen und mögliche Reglerdefekte. Keine automatischen Hilfetexte."),TEXT("Physiological effects and possible regulator faults. No automatic coaching.")):
            L(TEXT("Körperliche Folgen und Dekompression aktiv. Hilfetexte unterstützen dich."),TEXT("Physiological effects and decompression enabled. Coaching is available.")),x,708,970,15,Muted,2);
    }
    Line(110,806,1490,806,Muted,.5);
    Button(TEXT("language"),Diver->bEnglish?TEXT("Language: EN"):TEXT("Sprache: DE"),110,826,165,40);
    const TCHAR* qualityDE[]={TEXT("Niedrig"),TEXT("Mittel"),TEXT("Hoch"),TEXT("Sehr hoch")};
    const TCHAR* qualityEN[]={TEXT("Low"),TEXT("Medium"),TEXT("High"),TEXT("Epic")};
    const int32 quality=FMath::Clamp(FMath::RoundToInt(Diver->GraphicsQuality),0,3);
    Button(TEXT("quality"),L(TEXT("Grafik: "),TEXT("Graphics: "))+(Diver->bEnglish?qualityEN[quality]:qualityDE[quality]),290,826,190,40);
    Button(TEXT("audio"),TEXT("Audio"),495,826,100,40,bAudioPage);
    Button(TEXT("controls"),L(TEXT("F1  Steuerung & Hilfe"),TEXT("F1  Controls & help")),610,826,245,40);
    Button(Diver->bDiveActive?TEXT("abort"):TEXT("quit"),Diver->bDiveActive?L(TEXT("Tauchgang beenden"),TEXT("End dive")):L(TEXT("Beenden"),TEXT("Quit")),870,826,200,40);
    Button(TEXT("start"),Diver->bDiveActive?L(TEXT("Tauchgang fortsetzen  →"),TEXT("Resume dive  →")):c.StartOnShore?L(TEXT("Am Ufer starten  →"),TEXT("Start on shore  →")):L(TEXT("Abtauchen  →"),TEXT("Begin dive  →")),1170,820,320,50,true,ready||Diver->bDiveActive);
    if(!ready) TextBlock(Diver->Environment?Diver->Environment->Terrain.Error:L(TEXT("Gelände wird geladen"),TEXT("Loading terrain")),470,778,990,13,Red,1);
}
