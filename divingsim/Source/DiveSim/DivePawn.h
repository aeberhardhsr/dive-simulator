#pragma once
#include "CoreMinimal.h"
#include "Async/Future.h"
#include "GameFramework/Pawn.h"
#include "Simulation/DiveModel.h"
#include "Simulation/AudioModel.h"
#include "Simulation/Tutorial.h"
#include "DiveControls.h"
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
    virtual FVector GetPawnViewLocation() const override;

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
    bool bWalking=false;
    bool bRegulatorInMouth=true;
    double TestBreathingGas=0,TestBreathingSpO2=0,TestBreathingReserve=0;
    FString EntryName() const;
    bool CanFinishDive() const;
    bool bEnteredWater=false;
    int32 EntryIndex=0;
    int32 ComputerPage=0;
    FString Outcome, LastLogPath, Notice;
    FString DiveStartedUTC;
    double NoticeUntil=0;
    float GraphicsQuality=2;
    Dive::AudioLevels AudioLevels;
    UPROPERTY() TObjectPtr<UDiveAudio> Audio;
    FDiveControls Controls;
    int32 CapturingAction=INDEX_NONE;
    FString ControlMessage;
    bool bTutorialActive=false,bTutorialCompleted=false;
    Dive::Tutorial Tutorial;

    void StartDive();
    void StartTutorial();
    void PerformAction(EDiveAction Action);
    void CaptureKey(FKey Key);
    void ApplyControls();
    FString KeyText(EDiveAction Action) const { return Controls.KeyText(Action,bEnglish); }
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
    double CameraBaseHeight=0;
    bool bSmokeTest=false, bSmokeCaptured=false, bSmokeUnderwaterCaptured=false, bSmokeSecondaryCaptured=false;
    FString SmokeScenario;
    double SmokeLogbookStarted=0,SmokeLogbookTime=0;
    uint32 SmokeLogbookCaptured=0;
    int32 RecoverySampleCount=-1;
    bool bSmokeSurfaceReached=false, bSmokeChunkLimit=true;
    bool bSmokeImageryChecked=false;
    bool bPerformance=false;
    int32 PerformancePhase=0;
    double PerformanceStarted=0,PerformanceCaptureStarted=0,PerformanceSeconds=60,PerformanceYaw=0;
    double PerformanceNextMemory=0,PerformanceMemoryMB=0;
    FString PerformanceDirectory;
    TSharedFuture<FString> PerformanceWrite;
    double AudioSmokeClock=0,AudioExportStarted=0;
    bool bAudioMenuChecked=false,bAudioMenuCaptured=false;
    Dive::Config BeforeTutorialConfig;
    int32 BeforeTutorialEntry=0;
    double ControlsSmokeClock=0,TutorialFinishedAt=0;
    int32 ControlsSmokePhase=0;
    TSet<FKey> TestHeldKeys;
    bool bControlsSmokeValid=true;
    int32 TestLessonStage=-1;
    double TestLessonAge=0;
    uint32 TestLessonScreenshots=0;
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
    void MoveAtShore(float Step,float Forward,float Side,float Vertical,float Sprint,FVector& Wish,float& Effort);
    void Notify(const FString& Text);
    void InputSmokeTick(float DeltaSeconds);
    void TestKey(FKey Key,bool Down);
    void SetupPerformance();
    void TickPerformance();
};
