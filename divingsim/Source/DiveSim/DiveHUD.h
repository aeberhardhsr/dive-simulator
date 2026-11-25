#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DiveHUD.generated.h"

class ADivePawn;
UCLASS()
class DIVESIM_API ADiveHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
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
};
