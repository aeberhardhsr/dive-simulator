#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "DiveConfigIO.h"
#include "Simulation/Navigation.h"
#include "GameFramework/HUDHitBox.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool ADiveHUD::NavigationSmokeTest(int Phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver||!Diver->Environment) return false;
    const auto& terrain=Diver->Environment->Terrain;
    const bool requestedShore=FParse::Param(FCommandLine::Get(),TEXT("DiveNavShore"));
    auto shot=[this](const TCHAR* label)
    {
        FTimerHandle timer;const FString name=FString::Printf(TEXT("Navigation_%d_%s"),MapSmokeEntry,label);
        GetWorld()->GetTimerManager().SetTimer(timer,FTimerDelegate::CreateWeakLambda(this,[name]()
        { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(name+TEXT(".png")),true,false); }),.25f,false);
    };
    auto hit=[this](double x,double y,FName expected,bool click)
    {
        const auto* box=GetHitBoxAtCoordinates(FVector2D(OffsetX+x*Scale,OffsetY+y*Scale),true);
        if(!box||box->GetName()!=expected) return false;
        if(click) NotifyHitBoxClick(box->GetName());return true;
    };
    auto marker=[&](int entry,bool click)
    {
        const auto survey=terrain.ToSurvey(terrain.EntryList(MapDraftShore)[entry].Position);
        const auto p=Dive::Navigation::Project(survey.X,survey.Y,terrain.MapWest,terrain.MapSouth,terrain.MapEast-terrain.MapWest,terrain.MapNorth-terrain.MapSouth);
        return p.Inside()&&hit(120+p.X*1360,210+p.Y*340,FName(*FString::Printf(TEXT("map_marker_%d"),entry)),click);
    };
    if(Phase==0)
    {
        MapSmokeEntry=Diver->EntryIndex;Diver->PendingConfig.StartOnShore=false;Diver->bEnglish=false;
        NotifyHitBoxClick(TEXT("entry"));return EntryMapOpen()&&MapDraftEntry==MapSmokeEntry&&!MapDraftShore;
    }
    if(Phase==1)
    {
        if(!marker((MapSmokeEntry+1)%3,true)) return false;
        shot(TEXT("Preview_DE"));return MapDraftEntry==(MapSmokeEntry+1)%3&&Diver->EntryIndex==MapSmokeEntry;
    }
    if(Phase==2)
    {
        Diver->ToggleMenu();return !EntryMapOpen()&&Diver->bMenu&&Diver->EntryIndex==MapSmokeEntry&&!Diver->PendingConfig.StartOnShore;
    }
    if(Phase==3) { NotifyHitBoxClick(TEXT("entry"));NotifyHitBoxClick(TEXT("map_mode"));return EntryMapOpen()&&MapDraftShore; }
    if(Phase==4)
    {
        const bool clicked=hit(120+MapSmokeEntry*460+220,650,FName(*FString::Printf(TEXT("map_entry_%d"),MapSmokeEntry)),true);
        shot(TEXT("Shore_DE"));return clicked&&MapDraftEntry==MapSmokeEntry&&!Diver->PendingConfig.StartOnShore;
    }
    if(Phase==5)
    { NotifyHitBoxClick(TEXT("map_mode"));NotifyHitBoxClick(TEXT("map_language"));shot(TEXT("Water_EN"));return !MapDraftShore&&Diver->bEnglish; }
    if(Phase==6)
    {
        for(int i=0;i<3;++i)
            if(!marker(i,false)||!hit(120+i*460+220,650,FName(*FString::Printf(TEXT("map_entry_%d"),i)),false)) return false;
        if(requestedShore) NotifyHitBoxClick(TEXT("map_mode"));
        NotifyHitBoxClick(TEXT("map_apply"));
        return !EntryMapOpen()&&Diver->EntryIndex==MapSmokeEntry&&Diver->PendingConfig.StartOnShore==requestedShore;
    }
    if(Phase==7)
    {
        const auto expected=Diver->PendingConfig;
        Diver->EntryIndex=(MapSmokeEntry+1)%3;Diver->PendingConfig.StartOnShore=!requestedShore;
        Diver->LoadSettings();return DiveConfigIO::Equal(expected,MapSmokeEntry,Diver->PendingConfig,Diver->EntryIndex);
    }
    if(Phase==8)
    {
        Diver->StartDive();const FVector expected=requestedShore?Diver->Environment->ShoreStart(MapSmokeEntry):terrain.Entries[MapSmokeEntry].Position;
        shot(TEXT("Started"));return Diver->bDiveActive&&Diver->EntryLocation.Equals(expected,.01)&&
            FVector::Dist2D(Diver->GetActorLocation(),expected)<.01&&Diver->bWalking==requestedShore;
    }
    if(Phase==9) { Diver->bShowMap=true;shot(TEXT("DiveMap"));return !Diver->bMenu&&Diver->Simulation.Data.ElapsedSeconds>1; }
    if(Phase==10)
    {
        Diver->SetMenu(true);OpenEntryMap();const auto before=Diver->PendingConfig;const int entry=Diver->EntryIndex;
        MapPausedSeconds=Diver->Simulation.Data.ElapsedSeconds;
        NotifyHitBoxClick(TEXT("map_mode"));NotifyHitBoxClick(TEXT("map_entry_2"));NotifyHitBoxClick(TEXT("map_apply"));
        shot(TEXT("Locked"));return EntryMapOpen()&&DiveConfigIO::Equal(before,entry,Diver->PendingConfig,Diver->EntryIndex);
    }
    if(Phase==11)
    { OpenControls();shot(TEXT("Controls"));return !EntryMapOpen()&&ControlsOpen()&&Diver->bMenu&&Diver->Simulation.Data.ElapsedSeconds==MapPausedSeconds; }
    if(Phase==12)
    {
        const bool paused=Diver->Simulation.Data.ElapsedSeconds==MapPausedSeconds;
        CloseControls();Diver->ResumeDive();Diver->bShowMap=true;shot(TEXT("Resumed"));return paused&&!Diver->bMenu;
    }
    return false;
}
