#pragma once
#include "CoreMinimal.h"
#include "Async/Future.h"
#include "GameFramework/Pawn.h"
#include "Simulation/DiveModel.h"
#include "Simulation/AudioModel.h"
#include "Simulation/Tutorial.h"
#include "DiveControls.h"
#include "DiveRecording.h"
#include "DivePose.h"
#include "DiveContacts.h"
#include "Simulation/SoakMetrics.h"
#include "DivePawn.generated.h"

class UCameraComponent;
class UCapsuleComponent;
class USpotLightComponent;
class UAudioComponent;
class UDiveAudio;
class ADiveWorld;
class ADiveEquipment;
class UMaterialInstanceDynamic;

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
    int32 LoadoutTestCase=0;
    bool bLoadoutImpulseChecked=false,bLoadoutImpulseValid=false;
    TArray<FDiveSample> Samples;
    FVector VelocityMetres=FVector::ZeroVector;
    FDivePoseState PhysicalPose;
    FVector PhysicalEyes() const;
    int32 BodyContactCount=0,BlockedPoseSteps=0;
    FName LastBodyContact;
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
    double NextAutomaticHint=0,FreeflowBubbleClock=0;
    float GraphicsQuality=2;
    int32 HUDStyle=0; // Standard, compact, minimal; mode visibility rules still apply.
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
    bool PhotoPending() const { return PendingPhoto.IsSet(); }
    TArray<FDivePhotograph> Photographs;
    bool TestPhotographs() const { return bSmokeTest; }
    bool SaveLog(const FString& Reason);
    bool ValidateCameraMedium(FString& Error) const;
    FString Local(const TCHAR* German,const TCHAR* English) const { return bEnglish?English:German; }
private:
#if WITH_DEV_AUTOMATION_TESTS
    friend class FDiveBlockedHeadTurn;
#endif
    FDiveRecordingWriter RecordingWriter;
    TOptional<FDivePhotograph> PendingPhoto;
    FDelegateHandle PhotoProcessedHandle;
    void PhotoProcessed();
    float Accumulator=0, TelemetryAccumulator=0;
    double CameraBaseHeight=0;
    TArray<FDiveContactShape> ContactShapes;
    bool bContactPoseReady=false,bStandingUp=false;
    bool bBlockedGaze=false;
    FRotator RequestedGaze=FRotator::ZeroRotator;
    void ResetContactPose();
    void AdvanceContactPose(float Step,float Effort);
    void MoveWithContacts(const FVector& Delta,FHitResult& Hit);
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
    Dive::SoakMetrics SoakTotal,SoakWindow;
    double SoakLastTick=0,SoakNextReport=0,SoakSimStart=0,SoakDistanceStart=0,SoakFinalizationMS=0;
    double SoakSimEnd=0;
    double SoakAvoidUntil=0,SoakAvoidZ=0;
    int32 SoakAvoidances=0;
    double SoakMemoryStart=0,SoakMemoryMax=0,SoakMinDepth=1e9,SoakMaxDepth=0;
    int32 SoakTarget=1,SoakTurns=0,SoakSaves=0,SoakSaveErrors=0,SoakScreenshot=0;
    float SoakForward=0,SoakVertical=0;
    FVector SoakEnds[2];
    TSet<FIntPoint> SoakTiles;
    void SetupSoak();
    void SteerSoak(float DeltaSeconds);
    void TickSoak();
    bool ReportSoak(bool Final);
    int32 DayCycleSmokePhase=0;
    double DayCyclePauseWall=0,DayCyclePauseSeconds=0,DayCyclePauseHour=0;
    void TickDayCycleTest(float DeltaSeconds);
    void TickSedimentTest(float Dt);
    void TickCausticsTest(float Dt);
    double CausticsTestClock=0;
    int32 CausticsTestPhase=-1,CausticsTestCaptured=-1;
    int32 SiltSmokePhase=0,SiltBornAtStop=0;
    double SiltPauseWall=0,SiltPauseSeconds=0;
    FString SiltPauseDetail;
    FString ModeSmokeCase;
    int32 ModeSmokePhase=0;
    double ModeSmokeCapturedAt=0;
    bool bModeSmokeValid=true;
    void TickModeTest();
    void TickDecompressionTest(float Dt);
    void TickFishInspection(float Dt);
    void TickRockInspection(float Dt);
    void UpdateTorch();
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
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> TorchFunction;
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
