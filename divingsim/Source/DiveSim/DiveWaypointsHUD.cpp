#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveTextDialog.h"
#include "Simulation/Navigation.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
const FLinearColor White(.87,.94,.92,1),Muted(.42,.59,.57,1),Accent(.43,.87,.69,1),Amber(1,.73,.3,1),Dark(.012,.032,.032,1);
FString ShortLabel(const FString& Name,int Limit)
{
    if(Name.Len()<=Limit) return Name;
    int end=Limit-1;if(Name[end-1]>=0xD800&&Name[end-1]<=0xDBFF) --end;
    return Name.Left(end)+TEXT("…");
}
}
void ADiveHUD::EnsureWaypoints()
{
    if(!Waypoints.Path.IsEmpty()) return;
    const bool test=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"));
    Waypoints.Load(FPaths::ProjectSavedDir()/(test?TEXT("DiveWaypointsTest.json"):TEXT("DiveWaypoints.json")));
}
void ADiveHUD::OpenWaypoints()
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver||!Diver->Environment||bWaypointPage) return;
    bWaypointResume=Diver->bDiveActive&&!Diver->bMenu;Diver->SetMenu(true);EnsureWaypoints();
    bWaypointPage=true;bWaypointPick=bWaypointDelete=false;WaypointMessage.Empty();
    bProfilesPage=bControlsPage=bAvatarView=false;
    if(!Waypoints.Find(WaypointSelected)) WaypointSelected=Waypoints.Target;
    if(!Waypoints.Find(WaypointSelected)&&!Waypoints.Items.IsEmpty()) WaypointSelected=Waypoints.Items[0].Id;
}
void ADiveHUD::CloseWaypoints(bool Resume)
{
    if(WaypointDialog) { CloseWaypointName();if(Resume) return; }
    if(bWaypointPick&&Resume) { bWaypointPick=false;WaypointMessage.Empty();return; }
    const bool resume=Resume&&bWaypointResume;bWaypointPage=bWaypointResume=bWaypointPick=bWaypointDelete=false;
    if(resume&&Diver) Diver->ResumeDive();
}
FString ADiveHUD::WaypointError() const
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    const auto& e=Waypoints.Error;
    if(e.IsEmpty()) return WaypointMessage;
    if(e==TEXT("name")) return L(TEXT("Name: 1–32 Zeichen, keine Zeilenumbrueche."),TEXT("Name: 1–32 characters, no line breaks."));
    if(e==TEXT("duplicate")) return L(TEXT("Dieser Name ist bereits vergeben."),TEXT("This name is already in use."));
    if(e==TEXT("limit")) return L(TEXT("Alle 32 Wegpunkte sind belegt."),TEXT("All 32 waypoint slots are used."));
    if(e==TEXT("conflict")) return L(TEXT("Datei wurde anderswo geaendert. Bitte neu laden."),TEXT("File changed elsewhere. Please reload."));
    if(e==TEXT("read")) return L(TEXT("Wegpunktdatei nicht lesbar; sie bleibt unveraendert."),TEXT("Waypoint file is unreadable; it remains unchanged."));
    return L(TEXT("Nicht gespeichert. Vorheriger Stand bleibt erhalten."),TEXT("Not saved. Previous state is preserved."));
}
void ADiveHUD::OpenWaypointName(bool Rename)
{
    if(WaypointDialog||!Waypoints.Loaded||!GEngine||!GEngine->GameViewport) return;
    const auto* point=Waypoints.Find(WaypointSelected);if(Rename&&!point) return;
    bWaypointRename=Rename;WaypointNameError.Empty();bWaypointDelete=false;
    const TWeakObjectPtr<ADiveHUD> weak(this);
    auto dialog=SNew(SDiveTextDialog).English(Diver->bEnglish)
        .Title(FText::FromString(Diver->Local(Rename?TEXT("Wegpunkt umbenennen"):TEXT("Wegpunkt speichern"),Rename?TEXT("Rename waypoint"):TEXT("Save waypoint"))))
        .Hint(FText::FromString(Diver->Local(TEXT("1–32 Zeichen. Enter speichert; Escape bricht ab."),TEXT("1–32 characters. Enter saves; Escape cancels."))))
        .Initial(FText::FromString(Rename?point->Name:FString()))
        .Error_Lambda([weak](){return weak.IsValid()?FText::FromString(weak->WaypointNameError):FText::GetEmpty();})
        .OnSubmit_Lambda([weak](const FText& value,ETextCommit::Type){if(weak.IsValid()) weak->SubmitWaypointName(value.ToString());})
        .OnCancel_Lambda([weak](){if(weak.IsValid()) weak->CloseWaypointName();});
    WaypointDialog=dialog;GEngine->GameViewport->AddViewportWidgetContent(dialog,100);
    FInputModeUIOnly mode;mode.SetWidgetToFocus(dialog->Editor);GetOwningPlayerController()->SetInputMode(mode);
    FSlateApplication::Get().SetKeyboardFocus(dialog->Editor,EFocusCause::SetDirectly);
}
void ADiveHUD::CloseWaypointName(bool RestoreFocus)
{
    if(WaypointDialog&&GEngine&&GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(WaypointDialog.ToSharedRef());
    WaypointDialog.Reset();if(RestoreFocus&&Diver) Diver->SetMenu(true);
}
bool ADiveHUD::SubmitWaypointName(const FString& Name)
{
    if(!WaypointDialog) return false;FString id;
    const bool saved=bWaypointRename?Waypoints.Rename(WaypointSelected,Name):
        Waypoints.Add(Name,WaypointDraft.East,WaypointDraft.North,WaypointDraft.Depth,WaypointDraft.FromMap,id);
    if(!saved) { WaypointNameError=WaypointError();return false; }
    if(!id.IsEmpty()) { WaypointSelected=id;WaypointPage=(Waypoints.Items.Num()-1)/5; }
    CloseWaypointName();WaypointMessage=Diver->Local(TEXT("Wegpunkt gespeichert."),TEXT("Waypoint saved."));return true;
}
bool ADiveHUD::PickWaypoint(double X,double Y)
{
    if(!bWaypointPage||!bWaypointPick||WaypointDialog||!Waypoints.Loaded) return false;
    const auto& terrain=Diver->Environment->Terrain;
    const double east=terrain.MapWest+(X-120)/1360*(terrain.MapEast-terrain.MapWest);
    const double north=terrain.MapNorth-(Y-210)/340*(terrain.MapNorth-terrain.MapSouth);
    const auto world=terrain.ToWorld(east,north);float ground=0;
    if(X<120||X>1480||Y<210||Y>550||!terrain.Sample(world.X,world.Y,ground)||ground>=0)
    { WaypointMessage=Diver->Local(TEXT("Hier fehlen vermessene Unterwasserdaten. Anderen Punkt waehlen."),TEXT("No surveyed underwater data here. Choose another point."));return false; }
    WaypointDraft={FString(),FString(),east,north,-ground/100,true};bWaypointPick=false;OpenWaypointName(false);return WaypointDialog.IsValid();
}
void ADiveHUD::WaypointClick(FName Id)
{
    if(WaypointDialog) return;
    if(Id!=TEXT("waypoint_delete")) bWaypointDelete=false;
    if(Id==TEXT("map_back")) { CloseWaypoints();return; }
    if(Id==TEXT("map_language")) { Diver->bEnglish=!Diver->bEnglish;WaypointMessage.Empty();Diver->SaveSettings();return; }
    if(Id==TEXT("waypoint_reload")) { Waypoints.Load(Waypoints.Path);WaypointMessage.Empty();bWaypointPick=false;return; }
    if(Id==TEXT("waypoint_prev")) WaypointPage=FMath::Max(0,WaypointPage-1);
    if(Id==TEXT("waypoint_next")) WaypointPage=FMath::Min((Waypoints.Items.Num()-1)/5,WaypointPage+1);
    const FString key=Id.ToString();
    if(key.StartsWith(TEXT("waypoint_row_"))||key.StartsWith(TEXT("waypoint_marker_")))
    { const int i=FCString::Atoi(*key.Mid(key.StartsWith(TEXT("waypoint_row_"))?13:16));if(Waypoints.Items.IsValidIndex(i)) { WaypointSelected=Waypoints.Items[i].Id;bWaypointPick=false; }return; }
    if(!Waypoints.Loaded) return;
    if(Id==TEXT("waypoint_current")&&Diver->bDiveActive)
    {
        const auto position=Diver->GetActorLocation();const auto survey=Diver->Environment->Terrain.ToSurvey(position);
        WaypointDraft={FString(),FString(),survey.X,survey.Y,FMath::Max(0.,-position.Z/100),false};bWaypointPick=false;OpenWaypointName(false);
    }
    if(Id==TEXT("waypoint_map")) { bWaypointPick=!bWaypointPick;WaypointMessage.Empty(); }
    if(Id==TEXT("waypoint_pick"))
    { float x=0,y=0;if(GetOwningPlayerController()->GetMousePosition(x,y)) PickWaypoint((x-OffsetX)/Scale,(y-OffsetY)/Scale); }
    if(Id==TEXT("waypoint_rename")) OpenWaypointName(true);
    if(Id==TEXT("waypoint_target"))
        if(Waypoints.Select(Waypoints.Target==WaypointSelected?FString():WaypointSelected)) WaypointMessage=Diver->Local(TEXT("Navigationsziel gespeichert."),TEXT("Navigation target saved."));
    if(Id==TEXT("waypoint_delete"))
    {
        if(!bWaypointDelete) { bWaypointDelete=true;return; }
        if(Waypoints.Remove(WaypointSelected))
        { WaypointSelected=Waypoints.Items.IsEmpty()?FString():Waypoints.Items[0].Id;WaypointPage=0;WaypointMessage=Diver->Local(TEXT("Wegpunkt geloescht."),TEXT("Waypoint deleted.")); }
        bWaypointDelete=false;
    }
}
void ADiveHUD::DrawWaypoints()
{
    EnsureWaypoints();const auto& terrain=Diver->Environment->Terrain;
    for(int i=0;i<Waypoints.Items.Num();++i)
    {
        const auto& point=Waypoints.Items[i];const auto p=Dive::Navigation::Project(point.East,point.North,terrain.MapWest,terrain.MapSouth,terrain.MapEast-terrain.MapWest,terrain.MapNorth-terrain.MapSouth);
        if(!p.Inside()) continue;const float x=120+p.X*1360,y=210+p.Y*340;
        const bool target=point.Id==Waypoints.Target;const auto color=target?Accent:White;
        Rect(x-5,y-5,10,10,Dark);Line(x-5,y-5,x+5,y+5,color,2);Line(x-5,y+5,x+5,y-5,color,2);
        if(target||(bWaypointPage&&point.Id==WaypointSelected))
        {
            const FString label=ShortLabel(point.Name,18);
            const float width=label.Len()*14.f;
            const float labelX=x+9+width>1480?x-9-width:x+9;
            Text(label,FMath::Max(130.f,labelX),FMath::Min(y+7,530.f),14,color);
        }
        if(bWaypointPage&&!bWaypointPick) AddHitBox(FVector2D(OffsetX+(x-8)*Scale,OffsetY+(y-8)*Scale),FVector2D(16*Scale,16*Scale),FName(*FString::Printf(TEXT("waypoint_marker_%d"),i)),true,5);
    }
    if(!bWaypointPage&&Diver->bDiveActive)
        if(const auto* p=Waypoints.Find(Waypoints.Target))
        {
            const auto here=terrain.ToSurvey(Diver->GetActorLocation());const double north=p->North-here.Y,east=p->East-here.X;
            Text(FString::Printf(TEXT("%s: %s / %.0f m / %03d deg / %.1f m %s"),*Diver->Local(TEXT("Ziel"),TEXT("Target")),*p->Name,FMath::Sqrt(north*north+east*east),
                Dive::Navigation::DisplayHeading(Dive::Navigation::Bearing(north,east)),p->Depth,*Diver->Local(TEXT("Tiefe"),TEXT("depth"))),120,735,16,Accent);
        }
}
void ADiveHUD::WaypointInstrument()
{
    EnsureWaypoints();const auto* p=Waypoints.Find(Waypoints.Target);if(!p) return;
    const auto here=Diver->Environment->Terrain.ToSurvey(Diver->GetActorLocation());const double n=p->North-here.Y,e=p->East-here.X;
    const double distance=FMath::Sqrt(n*n+e*e),bearing=Dive::Navigation::Bearing(n,e);
    const double relative=FMath::DegreesToRadians(bearing-Diver->GetControlRotation().Yaw);
    Rect(640,127,320,77,Dark);
    Text(ShortLabel(p->Name,16),655,137,16,Accent);
    Text(FString::Printf(TEXT("%.0f m / %03d deg / %.1f m"),distance,Dive::Navigation::DisplayHeading(bearing),p->Depth),655,165,15,White);
    if(distance>=1)
    {
        const FVector2D center(932,154),d(FMath::Sin(relative),-FMath::Cos(relative)),s(-d.Y,d.X);
        const auto tip=center+d*13,base=center-d*8;
        Line(tip.X,tip.Y,base.X+s.X*6,base.Y+s.Y*6,Accent,2);Line(tip.X,tip.Y,base.X-s.X*6,base.Y-s.Y*6,Accent,2);
    }
}
void ADiveHUD::WaypointPanel()
{
    auto L=[this](const TCHAR* de,const TCHAR* en){return Diver->Local(de,en);};
    Rect(100,590,1400,225,Dark);
    Text(bWaypointPick&&WaypointMessage.IsEmpty()?L(TEXT("Klicke auf eine vermessene Wasserflaeche. Esc bricht ab."),TEXT("Click surveyed water. Esc cancels.")):WaypointError(),120,590,16,Amber);
    WaypointPage=FMath::Clamp(WaypointPage,0,FMath::Max(0,(Waypoints.Items.Num()-1)/5));
    for(int row=0;row<5;++row)
    {
        const int i=WaypointPage*5+row;if(!Waypoints.Items.IsValidIndex(i)) break;const auto& point=Waypoints.Items[i];
        Button(FName(*FString::Printf(TEXT("waypoint_row_%d"),i)),(point.Id==Waypoints.Target?TEXT("> "):TEXT(""))+ShortLabel(point.Name,18),120,620+row*32,420,29,point.Id==WaypointSelected);
    }
    if(Waypoints.Items.IsEmpty()) Text(L(TEXT("Noch keine Wegpunkte."),TEXT("No waypoints yet.")),120,632,18,Muted);
    Button(TEXT("waypoint_prev"),TEXT("<"),560,620,50,36);Button(TEXT("waypoint_next"),TEXT(">"),560,736,50,36);
    Text(FString::Printf(TEXT("%d/%d"),WaypointPage+1,FMath::Max(1,(Waypoints.Items.Num()+4)/5)),562,685,15,White);
    if(const auto* point=Waypoints.Find(WaypointSelected))
    {
        Text(point->Name,650,620,21,Accent);
        Text(FString::Printf(TEXT("LV95 E %.1f / N %.1f   /   %.1f m %s"),point->East,point->North,point->Depth,
            *L(point->FromMap?TEXT("Bodentiefe"):TEXT("markierte Tiefe"),point->FromMap?TEXT("bed depth"):TEXT("marked depth"))),650,653,16,White);
        Button(TEXT("waypoint_target"),Waypoints.Target==point->Id?L(TEXT("Ziel aufheben"),TEXT("Clear target")):L(TEXT("Als Ziel setzen"),TEXT("Set as target")),650,685,245,38,true);
        Button(TEXT("waypoint_rename"),L(TEXT("Umbenennen"),TEXT("Rename")),915,685,245,38);
        Button(TEXT("waypoint_delete"),bWaypointDelete?L(TEXT("Loeschen bestaetigen"),TEXT("Confirm deletion")):L(TEXT("Loeschen ..."),TEXT("Delete ...")),1180,685,300,38,bWaypointDelete);
    }
    if(Diver->bDiveActive) Button(TEXT("waypoint_current"),L(TEXT("Eigene Position merken"),TEXT("Mark current position")),650,740,285,38);
    Button(TEXT("waypoint_map"),bWaypointPick?L(TEXT("Auswahl abbrechen"),TEXT("Cancel placement")):L(TEXT("Punkt auf Karte setzen"),TEXT("Place point on map")),955,740,300,38,bWaypointPick);
    Button(TEXT("waypoint_reload"),L(TEXT("Neu laden"),TEXT("Reload")),1275,740,205,38);
    Text(L(TEXT("Luftlinie, keine Routenplanung. X = Wegpunkt. Gruen = Ziel. Maximal 32 Punkte."),TEXT("Straight line, no route planning. X = waypoint. Green = target. Up to 32 points.")),120,793,14,Muted);
    if(Diver->bDiveActive) Text(L(TEXT("TAUCHGANG PAUSIERT"),TEXT("DIVE PAUSED")),120,181,17,Accent);
}
