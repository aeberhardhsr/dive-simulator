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
    CloseDiscoveries();
    Diver=Cast<ADivePawn>(GetOwningPawn());
    if(!Diver||!Diver->Environment||!Diver->Environment->bTerrainReady||Diver->Environment->Terrain.Overview.IsEmpty()) return;
    MapDraftEntry=Diver->EntryIndex;MapDraftShore=Diver->PendingConfig.StartOnShore;
    bEntryMapPage=true;bProfilesPage=bControlsPage=bAvatarView=false;
}

void ADiveHUD::MapClick(FName Id)
{
    if(!bEntryMapPage||!Diver) return;
    if(Id==TEXT("map_waypoints")) { OpenWaypoints();return; }
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
    const bool choosing=SelectionPage&&!Diver->bDiveActive&&!bWaypointPage;
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
    if(!SelectionPage)
    {
        // In-dive navigation stays between the instruments and warning column.
        // The full-size selection/waypoint editor below keeps its own hit boxes.
        constexpr float left=358,top=310,width=820,height=205;
        auto locate=[&](const FVector& position)
        {
            const auto survey=data.ToSurvey(position);
            const auto p=Dive::Navigation::Project(survey.X,survey.Y,data.MapWest,data.MapSouth,data.MapEast-data.MapWest,data.MapNorth-data.MapSouth);
            return FVector2D(left+p.X*width,top+p.Y*height);
        };
        auto inside=[](const FVector2D& p){return p.X>=358&&p.X<=1178&&p.Y>=310&&p.Y<=515;};
        Rect(340,220,856,440,Dark);
        Text(L(TEXT("WALENSEE · TIEFENKARTE"),TEXT("WALENSEE · DEPTH MAP")),358,241,23,Accent);
        TextBlock(Diver->KeyText(EDiveAction::Map)+L(TEXT(" schliessen"),TEXT(" close")),1010,245,168,14,White,1);
        Text(L(TEXT("Gitternord oben · Tiefenlinien 10 m · © swisstopo / swissBATHY3D"),TEXT("Grid north up · 10 m contours · © swisstopo / swissBATHY3D")),358,281,12,Muted);
        if(MapTexture) DrawTexture(MapTexture,OffsetX+left*Scale,OffsetY+top*Scale,width*Scale,height*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
        for(int i=0;i<Diver->Samples.Num();i+=5)
        { const auto p=locate(Diver->Samples[i].Position);if(inside(p)) Rect(p.X-1,p.Y-1,2,2,Muted); }
        for(int i=0;i<entries.Num();++i)
        {
            const auto p=locate(entries[i].Position);if(!inside(p)) continue;
            Rect(p.X-9,p.Y-10,18,22,Dark);Text(FString::FromInt(i+1),p.X-4,p.Y-9,14,Amber);
        }
        const auto here=locate(Diver->GetActorLocation()),start=locate(Diver->EntryLocation);
        if(inside(start))
        {
            Line(start.X-7,start.Y,start.X,start.Y-7,Amber,2);Line(start.X,start.Y-7,start.X+7,start.Y,Amber,2);
            Line(start.X+7,start.Y,start.X,start.Y+7,Amber,2);Line(start.X,start.Y+7,start.X-7,start.Y,Amber,2);
        }
        if(inside(here))
        {
            const double angle=FMath::DegreesToRadians(Diver->GetControlRotation().Yaw);
            const FVector2D direction(FMath::Sin(angle),-FMath::Cos(angle)),side(-direction.Y,direction.X);
            const auto tip=here+direction*11,base=here-direction*6;
            Line(tip.X,tip.Y,base.X+side.X*6,base.Y+side.Y*6,Accent,3);
            Line(tip.X,tip.Y,base.X-side.X*6,base.Y-side.Y*6,Accent,3);
        }
        EnsureWaypoints();
        for(const auto& point:Waypoints.Items)
        {
            const auto p=Dive::Navigation::Project(point.East,point.North,data.MapWest,data.MapSouth,data.MapEast-data.MapWest,data.MapNorth-data.MapSouth);
            if(!p.Inside()) continue;
            const float px=left+p.X*width,py=top+p.Y*height;
            const auto color=point.Id==Waypoints.Target?Accent:White;
            Line(px-4,py-4,px+4,py+4,color,2);Line(px-4,py+4,px+4,py-4,color,2);
        }
        const float kilometre=width*1000/(data.MapEast-data.MapWest);
        Line(left,530,left+kilometre,530,White,2);Text(TEXT("1 km"),left+kilometre+10,520,12,Muted);
        TextBlock(inside(here)?L(TEXT("Grün: Position & Blickrichtung · Gelb: Spielstarts · Raute: dein Start"),TEXT("Green: position & heading · Yellow: entries · Diamond: your start")):L(TEXT("Deine Position liegt ausserhalb der Karte."),TEXT("Your position is outside this map.")),520,521,658,12,Accent,1);
        const FVector delta=(Diver->EntryLocation-Diver->GetActorLocation())/100;
        TextBlock(FString::Printf(TEXT("%s: %.0f m · %03d°"),*L(TEXT("Zum Start, Luftlinie"),TEXT("To start, straight line")),FVector2D(delta.X,delta.Y).Size(),Dive::Navigation::DisplayHeading(Dive::Navigation::Bearing(delta.X,delta.Y))),358,553,820,16,White,1);
        if(const auto* point=Waypoints.Find(Waypoints.Target))
            TextBlock(L(TEXT("Navigationsziel: "),TEXT("Navigation target: "))+point->Name,358,583,820,16,Accent,1);
        TextBlock(Diver->KeyText(EDiveAction::Waypoints)+L(TEXT(" Wegpunkte bearbeiten (Pause) · Simulation läuft weiter"),TEXT(" Edit waypoints (pause) · Simulation continues")),358,628,820,13,Muted,1);
        return;
    }
    Rect(80,80,1440,740,Dark);
    Text(bWaypointPage?L(TEXT("WALENSEE / DEINE WEGPUNKTE"),TEXT("WALENSEE / YOUR WAYPOINTS")):choosing?L(TEXT("WALENSEE / EINSTIEG WÄHLEN"),TEXT("WALENSEE / CHOOSE ENTRY")):
        L(TEXT("WALENSEE / TIEFENKARTE"),TEXT("WALENSEE / DEPTH MAP")),120,110,28,Accent);
    if(SelectionPage)
    {
        if(!bWaypointPage) Button(TEXT("map_waypoints"),L(TEXT("Wegpunkte"),TEXT("Waypoints")),875,105,185,40);
        Button(TEXT("map_language"),Diver->bEnglish?TEXT("EN / Deutsch"):TEXT("DE / English"),1080,105,180,40);
        Button(TEXT("map_back"),L(TEXT("< Zurück / Esc"),TEXT("< Back / Esc")),1280,105,200,40);
    }
    Text(L(TEXT("GITTERNORD OBEN (LV95)   /   swissBATHY3D 2020 / © swisstopo"),TEXT("GRID NORTH UP (LV95)   /   swissBATHY3D 2020 / © swisstopo")),120,159,14,Muted);
    if(MapTexture) DrawTexture(MapTexture,OffsetX+MapX*Scale,OffsetY+MapY*Scale,MapW*Scale,MapH*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);
    Text(TEXT("N"),MapX+16,MapY+12,18,White);Line(MapX+24,MapY+60,MapX+24,MapY+36,White,2);
    Line(MapX+24,MapY+36,MapX+18,MapY+44,White,2);Line(MapX+24,MapY+36,MapX+30,MapY+44,White,2);
    if(bWaypointPage&&bWaypointPick) AddHitBox(FVector2D(OffsetX+MapX*Scale,OffsetY+MapY*Scale),FVector2D(MapW*Scale,MapH*Scale),TEXT("waypoint_pick"),true,10);
    if(!choosing&&Diver->bDiveActive)
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
    if(!choosing&&Diver->bDiveActive)
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
        if(!bWaypointPage) Text(distance<1?L(TEXT("Am Startpunkt"),TEXT("At the start point")):
            FString::Printf(TEXT("%s: %.0f m / %03d°"),*L(TEXT("Zum Start, Luftlinie"),TEXT("To start, straight line")),distance,
                Dive::Navigation::DisplayHeading(Dive::Navigation::Bearing(delta.X,delta.Y))),120,181,17,White);
        Text(inMap(here)?L(TEXT("Grünes Dreieck: du und Blickrichtung   /   Raute: dein Start"),TEXT("Green triangle: you and heading   /   Diamond: your start")):
            L(TEXT("Deine Position liegt ausserhalb dieses Kartenausschnitts."),TEXT("Your position lies outside this map extent.")),120,712,17,Accent);
    }
    DrawWaypoints();
    const float kilometre=MapW*1000/(data.MapEast-data.MapWest);
    Line(MapX,570,MapX+kilometre,570,White,2);Line(MapX,566,MapX,574,White,2);Line(MapX+kilometre,566,MapX+kilometre,574,White,2);
    Text(TEXT("1 km"),MapX+kilometre+12,559,14,White);
    Text(L(TEXT("Tiefenlinien 10 m   /   dunkel: keine Messdaten   /   Bezugspegel 419 m LN02"),
        TEXT("Contours 10 m   /   dark: no survey data   /   water reference 419 m LN02")),360,559,14,Muted);
    if(bWaypointPage) { WaypointPanel();return; }
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
        Text(L(TEXT("Punkt oder Karteikarte wählen; erst Übernehmen ändert deinen Plan."),TEXT("Select a marker or card; Apply changes your plan.")),120,712,17,Muted);
        Button(TEXT("map_mode"),shore?L(TEXT("Start: am Ufer"),TEXT("Start: on shore")):L(TEXT("Start: direkt im Wasser"),TEXT("Start: offshore")),120,755,420,40);
        Button(TEXT("map_apply"),L(TEXT("Auswahl übernehmen   >"),TEXT("Apply selection   >")),1060,755,420,40,true);
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
            *L(Diver->bMenu?TEXT("Tauchgang pausiert"):TEXT("Simulation läuft weiter"),Diver->bMenu?TEXT("Dive paused"):TEXT("Simulation continues"))),120,789,14,White);
        Text(Diver->KeyText(EDiveAction::Waypoints)+L(TEXT(" Wegpunkte bearbeiten (Pause)"),TEXT(" edit waypoints (pause)")),850,789,14,Accent);
    }
}
