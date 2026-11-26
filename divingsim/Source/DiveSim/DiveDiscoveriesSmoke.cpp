#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveWorld.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUDHitBox.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

bool ADiveHUD::DiscoverySmokeTest(int Phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver||!Diver->Environment) return false;
    auto shot=[this](const TCHAR* label)
    {
        FTimerHandle timer;const FString path=FPaths::ProjectSavedDir()/TEXT("Screenshots")/(FString::Printf(TEXT("Discoveries_%d_%s.png"),Diver->EntryIndex,label));
        GetWorld()->GetTimerManager().SetTimer(timer,FTimerDelegate::CreateWeakLambda(this,[path](){FScreenshotRequest::RequestScreenshot(path,true,false);}),.3f,false);
    };
    auto click=[this](float x,float y,FName expected)
    {
        const auto* box=GetHitBoxAtCoordinates(FVector2D(OffsetX+x*Scale,OffsetY+y*Scale),true);
        if(!box||box->GetName()!=expected) return false;NotifyHitBoxClick(expected);return true;
    };
    auto view=[&](bool plant)
    {
        FVector eye;FRotator rotation;FString species;
        if(!Diver->Environment->FindDiscoveryTestView(plant,eye,rotation,species)) return false;
        Diver->SetActorLocation(eye);GetOwningPlayerController()->SetControlRotation(rotation);
        Diver->Camera->SetWorldRotation(rotation);Diver->Simulation.Data.DepthMetres=FMath::Max(0.,-eye.Z/100);
        Diver->Simulation.Data.MaxDepthMetres=FMath::Max(Diver->Simulation.Data.MaxDepthMetres,Diver->Simulation.Data.DepthMetres);
        return true;
    };
    FString restore;
    if(FParse::Value(FCommandLine::Get(),TEXT("DiveDiscoveryVerify="),restore))
    {
        if(Phase==0) { OpenLogbook(restore,true);return true; }
        if(Phase==1) return LogbookReady()&&SelectedLog.bTest&&SelectedLog.Photographs.Num()==2;
        if(Phase==2) { OpenDiscoveries(true);return Gallery.Num()==2&&PhotographTexture; }
        if(Phase==3) { shot(TEXT("Restart"));return bDiscoveryLog&&PhotographTexture; }
        if(Phase==13) { UE_LOG(LogTemp,Display,TEXT("DIVE_DISCOVERY_RESTART_PASS photos=2"));CloseDiscoveries(); }
        return true;
    }
    if(Phase==0)
    {
        Diver->PendingConfig.GameMode=Dive::Mode::Exploration;Diver->PendingConfig.Hour=12;Diver->PendingConfig.StartOnShore=false;
        Diver->StartDive();Diver->SetMenu(true);return Diver->bDiveActive;
    }
    if(Phase==1) return view(false);
    if(Phase==2||Phase==8)
    {
        const int count=Diver->Photographs.Num();Diver->SetMenu(false);Diver->PerformAction(EDiveAction::Photo);
        const bool pending=Diver->PhotoPending();Diver->Photograph();Diver->SetMenu(true);
        return pending&&Diver->Photographs.Num()==count;
    }
    if(Phase==3)
    {
        if(Diver->PhotoPending()||Diver->Photographs.Num()!=1||Diver->Photographs[0].Species.IsEmpty()) return false;
        OpenDiscoveries();return true;
    }
    if(Phase==4)
    {
        if(bDiscoveryLoading||!PhotographTexture||SpeciesCatalog.Num()!=8) return false;
        shot(TEXT("Gallery_DE"));return true;
    }
    if(Phase==5)
    {
        const int index=DiveDiscoveries::SpeciesIndex(Diver->Photographs[0].Species[0]);
        if(!click(100,265+index*57,FName(*FString::Printf(TEXT("discovery_species_%d"),index)))||DiscoverySpecies!=index||!PhotographTexture) return false;
        shot(TEXT("Species_DE"));return true;
    }
    if(Phase==6)
    { if(!click(1350,845,TEXT("discovery_language"))) return false;shot(TEXT("Species_EN"));return true; }
    if(Phase==7) { Diver->ToggleMenu();return !DiscoveriesOpen()&&Diver->bMenu&&view(true); }
    if(Phase==9)
    {
        if(Diver->Photographs.Num()!=2||Diver->PhotoPending()) return false;
        bool plant=false;for(const auto& s:Diver->Photographs[1].Species) plant|=DiveDiscoveries::SpeciesIndex(s)>=5;
        if(!plant||Diver->Photographs[0].Id==Diver->Photographs[1].Id) return false;
        Diver->FinishDive(TEXT("completed"));const auto log=DiveLogbook::Read(Diver->LastLogPath);
        if(!log.Error.IsEmpty()||log.Photographs.Num()!=2||log.Photographs[1].Species!=Diver->Photographs[1].Species) return false;
        UE_LOG(LogTemp,Display,TEXT("DIVE_DISCOVERY_LOG %s"),*FPaths::ConvertRelativePathToFull(Diver->LastLogPath));
        OpenLogbook(Diver->LastLogPath,true);return true;
    }
    if(Phase==10) { if(!LogbookReady()||SelectedLog.Photographs.Num()!=2) return false;shot(TEXT("Logbook"));return true; }
    if(Phase==11) { return click(1350,70,TEXT("logphotos"))&&bDiscoveriesPage&&bDiscoveryLog&&Gallery.Num()==2&&PhotographTexture; }
    if(Phase==12)
    {
        int species=-1;for(const auto& id:Diver->Photographs[1].Species) if(DiveDiscoveries::SpeciesIndex(id)>=5) species=DiveDiscoveries::SpeciesIndex(id);
        if(species<0||!click(100,265+species*57,FName(*FString::Printf(TEXT("discovery_species_%d"),species)))) return false;
        shot(TEXT("Plant_EN"));return PhotographTexture!=nullptr;
    }
    if(Phase==13) { Diver->ToggleMenu();return !DiscoveriesOpen()&&Diver->bLogbook&&Diver->bMenu; }
    return false;
}
