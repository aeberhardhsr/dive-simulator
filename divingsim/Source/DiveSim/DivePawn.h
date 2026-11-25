#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "Simulation/DiveModel.h"
#include "Simulation/AudioModel.h"
#include "DivePawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class USpotLightComponent;
class UAudioComponent;
class UDiveAudio;
class ADiveWorld;
class ADiveEquipment;
class UMaterialInstanceDynamic;

struct FDiveSample
{
    Dive::State State;
    FVector Position;
};

UCLASS(Blueprintable)
class DIVESIM_API ADivePawn : public APawn
{
    GENERATED_BODY()
public:
    ADivePawn();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCapsuleComponent> Body;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpotLightComponent> Lamp;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Diving") float SwimForceNewtons=125;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Diving") float DragCoefficient=85;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Comfort") float MouseSensitivity=.12f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Comfort") bool bCameraMotion=false;

    Dive::Model Simulation;
    Dive::Config PendingConfig;
    TArray<FDiveSample> Samples;
    FVector VelocityMetres=FVector::ZeroVector;
    FVector EntryLocation=FVector(-6500,0,-180);
    TObjectPtr<ADiveWorld> Environment=nullptr;
    UPROPERTY() TObjectPtr<ADiveEquipment> Equipment;
    bool bMenu=true, bDiveActive=false, bShowInstruments=true, bShowComputer=false, bShowMap=false;
    bool bLamp=true, bSummary=false, bEnglish=false, bLogbook=false;
    int32 EntryIndex=0;
    int32 ComputerPage=0;
    FString Outcome, LastLogPath, Notice;
    FString DiveStartedUTC;
    double NoticeUntil=0;
    float GraphicsQuality=2;
    Dive::AudioLevels AudioLevels;
    UPROPERTY() TObjectPtr<UDiveAudio> Audio;

    void StartDive();
    void FinishDive(const FString& Reason);
    void ToggleMenu();
    void ResumeDive();
    void SetMenu(bool Open);
    void SaveSettings();
    void LoadSettings();
    void Photograph();
    bool SaveLog(const FString& Reason);
    FString Local(const TCHAR* German,const TCHAR* English) const { return bEnglish?English:German; }
private:
    float Accumulator=0, TelemetryAccumulator=0;
    bool bSmokeTest=false, bSmokeCaptured=false, bSmokeUnderwaterCaptured=false, bSmokeSecondaryCaptured=false;
    FString SmokeScenario;
    double SmokeLogbookStarted=0,SmokeLogbookTime=0;
    uint32 SmokeLogbookCaptured=0;
    bool bSmokeSurfaceReached=false, bSmokeChunkLimit=true;
    double AudioSmokeClock=0,AudioExportStarted=0;
    bool bAudioMenuChecked=false,bAudioMenuCaptured=false;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> UnderwaterMaterial;
    void LookYaw(float Value);
    void LookPitch(float Value);
    void ToggleLamp();
    void ToggleHUD();
    void ToggleComputer();
    void NextComputerPage();
    void ToggleMap();
    void Interact();
    void SimulateStep(float Step);
    void Notify(const FString& Text);
};
