#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "Simulation/Navigation.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"

namespace
{
const FLinearColor White(.87,.94,.92,1),Muted(.42,.59,.57,1),Accent(.43,.87,.69,1);
const FLinearColor Dark(.012,.032,.032,.98),Panel(.025,.065,.063,.96),Amber(1,.73,.3,1);
constexpr float MapX=120,MapY=210,MapW=1360,MapH=340;
}

void ADiveHUD::OpenEntryMap()
{
    Diver=Cast<ADivePawn>(GetOwningPawn());
    if(!Diver||!Diver->Environment||!Diver->Environment->bTerrainReady||Diver->Environment->Terrain.Overview.IsEmpty()) return;
    MapDraftEntry=Diver->EntryIndex;MapDraftShore=Diver->PendingConfig.StartOnShore;
    bEntryMapPage=true;bProfilesPage=bControlsPage=bAvatarView=false;
}

void ADiveHUD::MapClick(FName Id)
{
    if(!bEntryMapPage||!Diver) return;
    if(Id==TEXT("map_back")) { CloseEntryMap();return; }
    if(Id==TEXT("map_language")) { Diver->bEnglish=!Diver->bEnglish;Diver->SaveSettings();return; }
    // The same page can be inspected in the pause menu, but cannot change a dive.
    if(Diver->bDiveActive||!Diver->Environment) return;
    if(Id==TEXT("map_mode")) MapDraftShore=!MapDraftShore;
    const auto& entries=Diver->Environment->Terrain.EntryList(MapDraftShore);
    for(int i=0;i<entries.Num();++i)
        if(Id==FName(*FString::Printf(TEXT("map_entry_%d"),i))||Id==FName(*FString::Printf(TEXT("map_marker_%d"),i))) MapDraftEntry=i;
    if(Id==TEXT("map_apply")&&entries.IsValidIndex(MapDraftEntry))
    {
        Diver->EntryIndex=MapDraftEntry;Diver->PendingConfig.StartOnShore=MapDraftShore;
        Diver->SaveSettings();CloseEntryMap();
    }
}

void ADiveHUD::Map(bool SelectionPage)
{
    if(!Diver||!Diver->Environment) return;
    const auto& data=Diver->Environment->Terrain;
    const bool choosing=SelectionPage&&!Diver->bDiveActive;
    const bool shore=choosing?MapDraftShore:Diver->PendingConfig.StartOnShore;
    const int selected=choosing?MapDraftEntry:Diver->EntryIndex;
    const auto& entries=data.EntryList(shore);
    auto L=[this](const TCHAR* de,const TCHAR* en) { return Diver->Local(de,en); };
    auto pixel=[&](const FVector& position)
    {
        const auto survey=data.ToSurvey(position);
        const auto p=Dive::Navigation::Project(survey.X,survey.Y,data.MapWest,data.MapSouth,
            data.MapEast-data.MapWest,data.MapNorth-data.MapSouth);
        return FVector2D(MapX+p.X*MapW,MapY+p.Y*MapH);
    };
    auto inMap=[](const FVector2D& p) { return p.X>=MapX&&p.X<=MapX+MapW&&p.Y>=MapY&&p.Y<=MapY+MapH; };
    if(!MapTexture&&data.Overview.Num()==FDiveTerrain::MapWidth*FDiveTerrain::MapHeight)
    {
        MapTexture=UTexture2D::CreateTransient(FDiveTerrain::MapWidth,FDiveTerrain::MapHeight,PF_B8G8R8A8);
        auto* pixels=static_cast<FColor*>(MapTexture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE));
        for(int i=0;i<data.Overview.Num();++i)
        {
            const float depth=data.Overview[i];
            if(depth<0) { pixels[i]=FColor(11,22,25);continue; }
            FLinearColor color=FMath::Lerp(FLinearColor(.18,.62,.46),FLinearColor(.015,.07,.16),FMath::Clamp(depth/155.f,0.f,1.f));
            const int band=FMath::FloorToInt(depth/10);
            const bool across=i%FDiveTerrain::MapWidth>0&&data.Overview[i-1]>=0&&FMath::FloorToInt(data.Overview[i-1]/10)!=band;
            const bool down=i>=FDiveTerrain::MapWidth&&data.Overview[i-FDiveTerrain::MapWidth]>=0&&FMath::FloorToInt(data.Overview[i-FDiveTerrain::MapWidth]/10)!=band;
            if(across||down) color*=.55f;
            pixels[i]=color.ToFColor(true);
        }
        MapTexture->GetPlatformData()->Mips[0].BulkData.Unlock();
        MapTexture->Filter=TF_Bilinear;MapTexture->SRGB=true;MapTexture->UpdateResource();
    }
    if(!SelectionPage) Rect(0,0,1600,900,FLinearColor(0,.012f,.014f,.78f));
    Rect(80,80,1440,740,Dark);
    Text(choosing?L(TEXT("WALENSEE / EINSTIEG WAEHLEN"),TEXT("WALENSEE / CHOOSE ENTRY")):
        L(TEXT("WALENSEE / TIEFENKARTE"),TEXT("WALENSEE / DEPTH MAP")),120,110,28,Accent);
    if(SelectionPage)
    {
        Button(TEXT("map_language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),1080,105,180,40);
        Button(TEXT("map_back"),L(TEXT("< Zurueck / Esc"),TEXT("< Back / Esc")),1280,105,200,40);
    }
    Text(L(TEXT("GITTERNORD OBEN (LV95)   /   swissBATHY3D 2020 / © swisstopo"),TEXT("GRID NORTH UP (LV95)   /   swissBATHY3D 2020 / © swisstopo")),120,159,14,Muted);
    if(MapTexture) DrawTexture(MapTexture,OffsetX+MapX*Scale,OffsetY+MapY*Scale,MapW*Scale,MapH*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
    Text(TEXT("N"),MapX+16,MapY+12,18,White);Line(MapX+24,MapY+60,MapX+24,MapY+36,White,2);
    Line(MapX+24,MapY+36,MapX+18,MapY+44,White,2);Line(MapX+24,MapY+36,MapX+30,MapY+44,White,2);
    if(!choosing)
        for(int i=0;i<Diver->Samples.Num();i+=5)
        { const auto p=pixel(Diver->Samples[i].Position);if(inMap(p)) Rect(p.X-1,p.Y-1,3,3,Muted); }
    for(int i=0;i<entries.Num();++i)
    {
        const auto p=pixel(entries[i].Position);if(!inMap(p)) continue;
        const auto color=i==selected?Accent:Amber;
        Rect(p.X-12,p.Y-12,24,24,Dark);
        Line(p.X-12,p.Y-12,p.X+12,p.Y-12,color,2);Line(p.X-12,p.Y+12,p.X+12,p.Y+12,color,2);
        Text(FString::FromInt(i+1),p.X-5,p.Y-10,16,color);
        if(choosing) AddHitBox(FVector2D(OffsetX+(p.X-20)*Scale,OffsetY+(p.Y-20)*Scale),FVector2D(40*Scale,40*Scale),
            FName(*FString::Printf(TEXT("map_marker_%d"),i)),true);
    }
    if(!choosing)
    {
        const FVector2D start=pixel(Diver->EntryLocation),here=pixel(Diver->GetActorLocation());
        if(inMap(start))
        {
            Line(start.X-9,start.Y,start.X,start.Y-9,Amber,2);Line(start.X,start.Y-9,start.X+9,start.Y,Amber,2);
            Line(start.X+9,start.Y,start.X,start.Y+9,Amber,2);Line(start.X,start.Y+9,start.X-9,start.Y,Amber,2);
            Text(TEXT("START"),start.X+14,start.Y-20,14,Amber);
        }
        if(inMap(here))
        {
            const double angle=FMath::DegreesToRadians(Diver->GetControlRotation().Yaw);
            const FVector2D direction(FMath::Sin(angle),-FMath::Cos(angle)),side(-direction.Y,direction.X);
            const auto tip=here+direction*14,base=here-direction*7;
            Line(tip.X,tip.Y,base.X+side.X*7,base.Y+side.Y*7,Accent,3);
            Line(tip.X,tip.Y,base.X-side.X*7,base.Y-side.Y*7,Accent,3);
            Line(base.X+side.X*7,base.Y+side.Y*7,base.X-side.X*7,base.Y-side.Y*7,Accent,3);
        }
        const FVector delta=(Diver->EntryLocation-Diver->GetActorLocation())/100;
        const double distance=FVector2D(delta.X,delta.Y).Size();
        Text(distance<1?L(TEXT("Am Startpunkt"),TEXT("At the start point")):
            FString::Printf(TEXT("%s: %.0f m / %03d deg"),*L(TEXT("Zum Start, Luftlinie"),TEXT("To start, straight line")),distance,
                Dive::Navigation::DisplayHeading(Dive::Navigation::Bearing(delta.X,delta.Y))),120,181,17,White);
        Text(inMap(here)?L(TEXT("Gruenes Dreieck: du und Blickrichtung   /   Raute: dein Start"),TEXT("Green triangle: you and heading   /   Diamond: your start")):
            L(TEXT("Deine Position liegt ausserhalb dieses Kartenausschnitts."),TEXT("Your position lies outside this map extent.")),120,712,17,Accent);
    }
    const float kilometre=MapW*1000/(data.MapEast-data.MapWest);
    Line(MapX,570,MapX+kilometre,570,White,2);Line(MapX,566,MapX,574,White,2);Line(MapX+kilometre,566,MapX+kilometre,574,White,2);
    Text(TEXT("1 km"),MapX+kilometre+12,559,14,White);
    Text(L(TEXT("Tiefenlinien 10 m   /   dunkel: keine Messdaten   /   Bezugspegel 419 m LN02"),
        TEXT("Contours 10 m   /   dark: no survey data   /   water reference 419 m LN02")),360,559,14,Muted);
    const float card=(MapW-40)/3;
    for(int i=0;i<entries.Num()&&i<3;++i)
    {
        const float x=MapX+i*(card+20);const auto survey=data.ToSurvey(entries[i].Position);
        const FString title=FString::Printf(TEXT("%d   %s"),i+1,*entries[i].Name);
        if(choosing) Button(FName(*FString::Printf(TEXT("map_entry_%d"),i)),title,x,615,card,80,i==selected);
        else { Rect(x,615,card,80,Panel);Text(title,x+18,635,20,i==selected?Accent:White); }
        Text(FString::Printf(TEXT("LV95 E %.0f / N %.0f"),survey.X,survey.Y),x+18,669,13,choosing&&i==selected?Dark:Muted);
    }
    if(choosing)
    {
        Text(L(TEXT("Punkt oder Karteikarte waehlen; erst Uebernehmen aendert deinen Plan."),TEXT("Select a marker or card; Apply changes your plan.")),120,712,17,Muted);
        Button(TEXT("map_mode"),shore?L(TEXT("Start: am Ufer"),TEXT("Start: on shore")):L(TEXT("Start: direkt im Wasser"),TEXT("Start: offshore")),120,755,420,40);
        Button(TEXT("map_apply"),L(TEXT("Auswahl uebernehmen   >"),TEXT("Apply selection   >")),1060,755,420,40,true);
    }
    else
    {
        const auto survey=data.ToSurvey(Diver->GetActorLocation());
        Text(FString::Printf(TEXT("LV95 E %.1f / N %.1f   /   %s"),survey.X,survey.Y,
            *L(TEXT("Entfernung horizontal; keine Routenplanung."),TEXT("Horizontal distance; no route planning."))),120,755,15,Muted);
        if(!SelectionPage) Text(Diver->KeyText(EDiveAction::Map)+L(TEXT(" schliessen"),TEXT(" close")),1260,755,17,White);
        const auto& state=Diver->Simulation.Data;
        const FString gas=Diver->Simulation.Settings.Rules().LimitedGas?FString::Printf(TEXT("%.0f bar"),state.CylinderBar):TEXT("INF");
        Text(FString::Printf(TEXT("%.1f m   /   %s   /   %s"),state.DepthMetres,*gas,
            *L(Diver->bMenu?TEXT("Tauchgang pausiert"):TEXT("Simulation laeuft weiter"),Diver->bMenu?TEXT("Dive paused"):TEXT("Simulation continues"))),120,789,14,White);
    }
}
