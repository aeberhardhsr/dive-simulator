#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/DiveModel.h"
#include "DiveEquipment.generated.h"

class ADivePawn;
class ADiveAvatar;
class UCanvas;
class UFont;
class UCanvasRenderTarget2D;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;
class UProceduralMeshComponent;

UCLASS()
class DIVESIM_API ADiveEquipment : public AActor
{
    GENERATED_BODY()
public:
    ADiveEquipment();
    void Initialize(ADivePawn* Pawn);
    void Update(float DeltaSeconds);
    bool Validate(FString& Error) const;
    void ExportDisplayForTest(const FString& Directory,const FString& Filename) const;
    int32 DisplayUpdates=0;
    uint32 DisplayedPages=0;
    float WristBlend=0;
    UPROPERTY() TObjectPtr<ADiveAvatar> Avatar;
private:
    UPROPERTY() TObjectPtr<ADivePawn> Diver;
    UPROPERTY() TObjectPtr<USceneComponent> LeftWrist;
    UPROPERTY() TObjectPtr<USceneComponent> RightHand;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Meshes;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Screen;
    UPROPERTY() TObjectPtr<UCanvasRenderTarget2D> Display;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ScreenMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SuitMaterial;
    UPROPERTY() TObjectPtr<UFont> Font;
    Dive::State DisplayState;
    float DisplayTimer=0, Heading=0;
    bool bEnglish=false, bUnlimited=false;
    int32 Page=0;
    int32 TorchModel=INDEX_NONE;
    UFUNCTION() void DrawDisplay(UCanvas* Canvas,int32 Width,int32 Height);
};
