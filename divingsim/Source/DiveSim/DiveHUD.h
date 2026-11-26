#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DiveLogbook.h"
#include "DiveProfiles.h"
#include "DiveWaypoints.h"
#include "Async/Future.h"
#include "DiveHUD.generated.h"

class ADivePawn;
UCLASS()
class DIVESIM_API ADiveHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void OpenLogbook(const FString& PreferredPath=FString(),bool IncludeTests=false);
    void OpenDiscoveries(bool SelectedDive=false);
    void CloseDiscoveries();
    bool DiscoveriesOpen() const { return bDiscoveriesPage; }
    void OpenControls();
    void CloseControls() { bControlsPage=false; }
    bool ControlsOpen() const { return bControlsPage; }
    void OpenProfiles();
    bool ProfilesOpen() const { return bProfilesPage; }
    void CloseProfiles(bool IncludingDialog=false);
    bool SubmitProfileName(const FString& Name);
    bool ProfileSmokeTest(int Phase);
    bool ConfigSmokeTest(int Phase);
    bool EnvironmentSmokeTest(int Phase);
    bool LoadoutSmokeTest(int Phase);
    bool AvatarSmokeTest(int Phase);
    bool NavigationSmokeTest(int Phase);
    bool WaypointSmokeTest(int Phase);
    bool DiscoverySmokeTest(int Phase);
    void OpenWaypoints();
    void CloseWaypoints(bool Resume=true);
    bool WaypointsOpen() const { return bWaypointPage; }
    void OpenEntryMap();
    bool EntryMapOpen() const { return bEntryMapPage; }
    void CloseEntryMap() { bEntryMapPage=false; }
    int32 AvatarPausedUpdates=0;
    float AvatarPausedPhase=0;
    bool ConfigInputOpen() const { return ConfigDialog.IsValid(); }
    void CloseConfigInput(bool RestoreFocus=true);
    bool LogbookReady() const { return !bIndexLoading&&!bDetailLoading&&SelectedLog.Error.IsEmpty()&&!SelectedLog.Path.IsEmpty(); }
    bool LogbookMatches(const FString& Path,int32 Samples) const;
    int32 LogMetricGroup=0;
    uint32 LogRenderedGroups=0;
private:
    ADivePawn* Diver=nullptr;
    UPROPERTY() TObjectPtr<UFont> InterfaceFont=nullptr;
    UPROPERTY() TObjectPtr<class UTexture2D> MapTexture=nullptr;
    float Scale=1, OffsetX=0, OffsetY=0;
    void Text(const FString& Value,float X,float Y,float Size,FLinearColor Color);
    void Rect(float X,float Y,float W,float H,FLinearColor Color);
    void Line(float X,float Y,float X2,float Y2,FLinearColor Color,float Thickness=1);
    void Button(FName Id,const FString& Label,float X,float Y,float W,float H,bool Accent=false);
    void Field(FName Id,const FString& Label,const FString& Value,float X,float Y,float W=330);
    void Menu();
    void LoadoutMenu();
    bool OpenConfigInput(FName Id);
    bool SubmitConfigInput(const FString& Value);
    FString ConfigValue(FName Id) const;
    FString ConfigInputTitle() const;
    FString ConfigInputHint() const;
    FName ConfigEditing;
    FString ConfigError;
    TSharedPtr<class SWidget> ConfigDialog;
    TSharedPtr<class SEditableTextBox> ConfigEditor;
    void Instruments();
    void Summary();
    void Map(bool SelectionPage=false);
    void MapClick(FName Id);
    void EnsureWaypoints();
    void WaypointClick(FName Id);
    void WaypointPanel();
    void DrawWaypoints();
    void WaypointInstrument();
    bool PickWaypoint(double X,double Y);
    void OpenWaypointName(bool Rename);
    bool SubmitWaypointName(const FString& Name);
    void CloseWaypointName(bool RestoreFocus=true);
    FString WaypointError() const;
    FDiveWaypoints Waypoints;
    bool bWaypointPage=false,bWaypointResume=false,bWaypointPick=false,bWaypointRename=false,bWaypointDelete=false;
    int32 WaypointPage=0;
    FString WaypointSelected,WaypointMessage,WaypointNameError;
    FDiveWaypoint WaypointDraft;
    TSharedPtr<class SWidget> WaypointDialog;
    double WaypointPausedSeconds=0;
    bool bEntryMapPage=false,MapDraftShore=true;
    int32 MapDraftEntry=0;
    int32 MapSmokeEntry=0;
    double MapPausedSeconds=0;
    void Discoveries();
    void DiscoveryClick(FName Id);
    void SelectPhotograph();
    bool bDiscoveriesPage=false,bDiscoveryLoading=false,bDiscoveryLog=false;
    int32 DiscoverySpecies=-1,DiscoveryPhoto=0,DiscoveryUnreadable=0;
    TArray<FDiveSpecies> SpeciesCatalog;
    struct FGalleryPhoto { FDivePhotograph Photo;FString Location;bool Test=false; };
    TArray<FGalleryPhoto> Gallery;
    TArray<int32> VisiblePhotos() const;
    TFuture<FDiveLogIndex> DiscoveryTask;
    UPROPERTY() TObjectPtr<class UTexture2D> PhotographTexture=nullptr;
    void Logbook();
    void ControlsMenu();
    void TutorialOverlay();
    void ProfilesMenu();
    bool ProfileClick(FName Id);
    void OpenProfileName(bool Rename);
    void CloseProfileName(bool RestoreFocus=true);
    FString ProfileErrorText() const;
    TArray<int32> VisibleProfiles() const;
    void RefreshProfileSelection();
    void SelectLog(int32 Index);
    FDiveLogIndex LogIndex;
    FDiveLog SelectedLog;
    TFuture<FDiveLogIndex> IndexTask;
    TFuture<FDiveLog> DetailTask;
    bool bIndexLoading=false,bDetailLoading=false;
    bool bEnvironmentPage=false,bAudioPage=false,bControlsPage=false,bLoadoutPage=false,bAvatarView=false;
    bool bProfilesPage=false,bProfileArchive=false,bProfileRename=false;
    FDiveProfiles Profiles;
    FString ProfileSelection,ProfileEditingId,ProfileMessage,ProfileNameError;
    int32 ProfilePage=0;
    TSharedPtr<class SWidget> ProfileDialog;
    TSharedPtr<class SEditableTextBox> ProfileNameEditor;
    int32 LogSelection=INDEX_NONE,LogPage=0,LogCursor=INDEX_NONE,LogFilter=0;
    FString PreferredLog;
    TArray<int32> VisibleLogs() const;
};
