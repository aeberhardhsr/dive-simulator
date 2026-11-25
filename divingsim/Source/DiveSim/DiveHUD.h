#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DiveLogbook.h"
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
    void OpenLogbook(const FString& PreferredPath=FString(),bool IncludeTests=false);
    bool LogbookReady() const { return !bIndexLoading&&!bDetailLoading&&SelectedLog.Error.IsEmpty()&&!SelectedLog.Path.IsEmpty(); }
    bool LogbookMatches(const FString& Path,int32 Samples) const { return LogbookReady()&&SelectedLog.Path==Path&&SelectedLog.Samples.Num()==Samples; }
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
    void Instruments();
    void Summary();
    void Map();
    void Logbook();
    void SelectLog(int32 Index);
    FDiveLogIndex LogIndex;
    FDiveLog SelectedLog;
    TFuture<FDiveLogIndex> IndexTask;
    TFuture<FDiveLog> DetailTask;
    bool bIndexLoading=false,bDetailLoading=false;
    int32 LogSelection=INDEX_NONE,LogPage=0,LogCursor=INDEX_NONE,LogFilter=0;
    FString PreferredLog;
    TArray<int32> VisibleLogs() const;
};
