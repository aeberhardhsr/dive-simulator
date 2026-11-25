#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Simulation/AudioModel.h"
#include "DiveAudio.generated.h"

class UAudioComponent;
class USceneComponent;
UCLASS()
class DIVESIM_API UDiveAudio : public UActorComponent
{
    GENERATED_BODY()
public:
    void Initialize(USceneComponent* Listener);
    void Reset();
    void SetPaused(bool Paused);
    void ApplyLevels(const Dive::AudioLevels& Levels);
    bool Advance(float Dt,const Dive::State& State,const Dive::Config& Config,double EyeDepth);
    bool Validate(FString& Error) const;
    int32 Inhales=0,Exhales=0,Heartbeats=0,Warnings=0,InflationSteps=0,VentSteps=0;
private:
    enum Channel { Inhale,Exhale,Bubbles,Inflate,Vent,Heart,Underwater,Surface,Wind,Rain,Warning,Count };
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Sounds;
    UPROPERTY() TObjectPtr<USceneComponent> Listener;
    Dive::AudioModel Model;
    Dive::AudioLevels Levels;
    double Gains[Count]={};
    bool bPaused=true;
    void Gain(int Index,double Value);
    void Play(int Index,float Pitch=1);
};
