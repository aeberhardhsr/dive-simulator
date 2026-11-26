#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "Simulation/Navigation.h"
#include "GameFramework/HUDHitBox.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "UnrealClient.h"

bool ADiveHUD::WaypointSmokeTest(int Phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver||!Diver->Environment) return false;
    auto shot=[this](const TCHAR* label)
    {
        FTimerHandle timer;const FString name=FString::Printf(TEXT("Waypoints_%02d_%s"),Diver->EntryIndex,label);
        GetWorld()->GetTimerManager().SetTimer(timer,FTimerDelegate::CreateWeakLambda(this,[name]()
        { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(name+TEXT(".png")),true,false); }),.25f,false);
    };
    auto click=[this](float x,float y,FName expected)
    {
        const auto* box=GetHitBoxAtCoordinates(FVector2D(OffsetX+x*Scale,OffsetY+y*Scale),true);
        if(!box||box->GetName()!=expected)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_WAYPOINT_HIT expected=%s actual=%s"),*expected.ToString(),box?*box->GetName().ToString():TEXT("none"));return false; }
        NotifyHitBoxClick(expected);return true;
    };
    auto tap=[this](FKey key)
    {
        GetOwningPlayerController()->InputKey(FInputKeyEventArgs::CreateSimulated(key,IE_Pressed,1));
        GetOwningPlayerController()->InputKey(FInputKeyEventArgs::CreateSimulated(key,IE_Released,0));
    };
    if(Phase==0)
    {
        FString restore;
        if(FParse::Value(FCommandLine::Get(),TEXT("DiveWaypointVerify="),restore))
        {
            if(!Waypoints.Load(restore)||Waypoints.Items.Num()!=1||Waypoints.Target!=Waypoints.Items[0].Id||
                Waypoints.Items[0].Name!=TEXT("Meine Position")||Waypoints.Items[0].FromMap) return false;
            OpenWaypoints();shot(TEXT("Restart"));return WaypointsOpen();
        }
        Waypoints.Load(FPaths::ProjectSavedDir()/TEXT("Automation")/(TEXT("Waypoints-")+FGuid::NewGuid().ToString()+TEXT(".json")));
        Diver->bEnglish=false;OpenEntryMap();return EntryMapOpen();
    }
    if(Phase==1) { return click(960,125,TEXT("map_waypoints"))&&WaypointsOpen()&&Diver->bMenu; }
    if(Phase==2)
    {
        if(!click(1100,759,TEXT("waypoint_map"))) return false;
        if(PickWaypoint(119,200)||!bWaypointPick||WaypointDialog) return false;
        return true;
    }
    if(Phase==3)
    {
        const auto& terrain=Diver->Environment->Terrain;const auto survey=terrain.ToSurvey(terrain.Entries[Diver->EntryIndex].Position);
        const auto point=Dive::Navigation::Project(survey.X,survey.Y,terrain.MapWest,terrain.MapSouth,terrain.MapEast-terrain.MapWest,terrain.MapNorth-terrain.MapSouth);
        const auto* box=GetHitBoxAtCoordinates(FVector2D(OffsetX+(120+point.X*1360)*Scale,OffsetY+(210+point.Y*340)*Scale),true);
        if(!box||box->GetName()!=TEXT("waypoint_pick")) return false;
        const bool picked=PickWaypoint(120+point.X*1360,210+point.Y*340);shot(TEXT("Name_DE"));return picked&&WaypointDraft.FromMap;
    }
    if(Phase==4)
    {
        if(SubmitWaypointName(TEXT("  "))||!WaypointDialog||!Waypoints.Items.IsEmpty()) return false;
        return SubmitWaypointName(TEXT("Felswand Süd"))&&Waypoints.Items.Num()==1&&Waypoints.Items[0].FromMap;
    }
    if(Phase==5)
    {
        if(!click(760,703,TEXT("waypoint_target"))||Waypoints.Target.IsEmpty()) return false;
        shot(TEXT("Map_DE"));return true;
    }
    if(Phase==6)
    {
        if(!click(1020,703,TEXT("waypoint_rename"))) return false;
        if(!SubmitWaypointName(TEXT("Felswand Süd – Einstieg"))) return false;
        NotifyHitBoxClick(TEXT("map_language"));shot(TEXT("Map_EN"));return Diver->bEnglish;
    }
    if(Phase==7)
    {
        CloseWaypoints();if(!EntryMapOpen()||!Diver->bMenu) return false;CloseEntryMap();
        Diver->PendingConfig.StartOnShore=false;Diver->StartDive();return Diver->bDiveActive&&!Diver->bMenu;
    }
    if(Phase==8)
    {
        tap(Diver->Controls.Key(EDiveAction::Waypoints));WaypointPausedSeconds=Diver->Simulation.Data.ElapsedSeconds;
        return WaypointsOpen()&&Diver->bMenu&&bWaypointResume;
    }
    if(Phase==9)
    {
        if(Diver->Simulation.Data.ElapsedSeconds!=WaypointPausedSeconds||!click(790,759,TEXT("waypoint_current"))) return false;
        const auto survey=Diver->Environment->Terrain.ToSurvey(Diver->GetActorLocation());
        return WaypointDraft.East==survey.X&&WaypointDraft.North==survey.Y&&!WaypointDraft.FromMap&&SubmitWaypointName(TEXT("Meine Position"));
    }
    if(Phase==10)
    {
        if(!click(760,703,TEXT("waypoint_target"))) return false;
        FDiveWaypoints disk;if(!disk.Load(Waypoints.Path)||disk.Items.Num()!=2||disk.Target!=WaypointSelected) return false;
        shot(TEXT("Current_EN"));return Diver->Simulation.Data.ElapsedSeconds==WaypointPausedSeconds;
    }
    if(Phase==11)
    {
        if(!click(1020,703,TEXT("waypoint_rename"))||SubmitWaypointName(TEXT("Felswand Süd – Einstieg"))) return false;
        CloseWaypoints();return !WaypointDialog&&WaypointsOpen()&&Waypoints.Items.Num()==2;
    }
    if(Phase==12)
    {
        if(!click(1300,703,TEXT("waypoint_delete"))||!bWaypointDelete||Waypoints.Items.Num()!=2) return false;
        if(!click(1380,759,TEXT("waypoint_reload"))||bWaypointDelete||Waypoints.Items.Num()!=2) return false;
        CloseWaypoints();Diver->bShowMap=true;shot(TEXT("LiveMap"));return !Diver->bMenu;
    }
    if(Phase==13)
    { return Diver->Simulation.Data.ElapsedSeconds>WaypointPausedSeconds+1; }
    if(Phase==14)
    {
        tap(Diver->Controls.Key(EDiveAction::Waypoints));return WaypointsOpen()&&Diver->bMenu;
    }
    if(Phase==15)
    {
        if(!click(330,634,TEXT("waypoint_row_0"))) return false;
        const bool clicked=click(760,703,TEXT("waypoint_target"));
        UE_LOG(LogTemp,Display,TEXT("DIVE_WAYPOINT_SELECT clicked=%d selected=%s target=%s first=%s error=%s"),clicked,*WaypointSelected,*Waypoints.Target,*Waypoints.Items[0].Id,*Waypoints.Error);
        return clicked&&Waypoints.Target==Waypoints.Items[0].Id;
    }
    if(Phase==16)
    {
        if(!click(1300,703,TEXT("waypoint_delete"))||Waypoints.Items.Num()!=2) return false;
        shot(TEXT("Delete_EN"));return bWaypointDelete;
    }
    if(Phase==17)
    {
        if(!click(1300,703,TEXT("waypoint_delete"))||Waypoints.Items.Num()!=1||!Waypoints.Target.IsEmpty()) return false;
        if(!click(760,703,TEXT("waypoint_target"))) return false;
        const bool persisted=Waypoints.Load(Waypoints.Path)&&Waypoints.Items.Num()==1&&!Waypoints.Target.IsEmpty();
        CloseWaypoints();Diver->bShowMap=false;shot(TEXT("Bearing"));return persisted&&!Diver->bMenu;
    }
    if(Phase==18) { tap(EKeys::F1);shot(TEXT("Controls"));return ControlsOpen()&&!WaypointsOpen(); }
    if(Phase==19)
    {
        if(!click(1320,615,TEXT("bind_17"))) return false;
        tap(EKeys::B);return Diver->Controls.Key(EDiveAction::Waypoints)==EKeys::B;
    }
    if(Phase==20)
    {
        CloseControls();Diver->ResumeDive();tap(EKeys::B);
        const bool open=WaypointsOpen()&&Diver->bMenu;CloseWaypoints();
        UE_LOG(LogTemp,Display,TEXT("DIVE_WAYPOINT_FILE %s"),*FPaths::ConvertRelativePathToFull(Waypoints.Path));
        return open&&!Diver->bMenu;
    }
    return false;
}
