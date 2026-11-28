#include "DiveAudio.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundAttenuation.h"

namespace
{
const TCHAR* Names[]={TEXT("A_Inhale"),TEXT("A_Exhale"),TEXT("A_Bubbles"),TEXT("A_Inflate"),TEXT("A_Vent"),TEXT("A_Heart"),TEXT("A_Underwater"),TEXT("A_Surface"),TEXT("A_Wind"),TEXT("A_Rain"),TEXT("A_Warning"),TEXT("A_Vent")};
constexpr int Categories[]={2,2,2,2,2,3,1,1,1,1,4,2};
bool Loop(int i) { return i==3||i==4||i==11||(i>=6&&i<=9); }
}
void UDiveAudio::Initialize(USceneComponent* InListener)
{
    Listener=InListener;
    for(int i=0;i<Count;++i)
    {
        auto* sound=NewObject<UAudioComponent>(GetOwner(),FName(*FString::Printf(TEXT("DiveAudio_%d"),i)));
        sound->bAutoActivate=false; sound->bAutoDestroy=false;
        sound->SetupAttachment(Listener); sound->bAllowSpatialization=i==Bubbles;
        if(i==Bubbles)
        {
            sound->bOverrideAttenuation=true;
            sound->AttenuationOverrides.bSpatialize=true;
            sound->AttenuationOverrides.AttenuationShapeExtents=FVector(30,0,0);
            sound->AttenuationOverrides.FalloffDistance=700;
        }
        sound->RegisterComponent();
        sound->SetSound(LoadObject<USoundWave>(nullptr,*FString::Printf(TEXT("/Game/Diving/Audio/%s.%s"),Names[i],Names[i])));
        sound->SetVolumeMultiplier(0);
        if(i>=Surface&&i<=Rain) sound->SetLowPassFilterEnabled(true);
        Sounds.Add(sound);
    }
}
void UDiveAudio::Reset()
{
    Model=Dive::AudioModel{}; Inhales=Exhales=Heartbeats=Warnings=InflationSteps=VentSteps=0;
    bPaused=true;
    for(int i=0;i<Sounds.Num();++i) { Sounds[i]->Stop(); Gains[i]=0; Sounds[i]->SetVolumeMultiplier(0); }
}
void UDiveAudio::SetPaused(bool Paused)
{
    bPaused=Paused;
    for(int i=0;i<Sounds.Num();++i)
    {
        if(!Paused&&Loop(i)&&!Sounds[i]->IsPlaying()) Sounds[i]->Play();
        Sounds[i]->SetPaused(Paused);
    }
}
void UDiveAudio::ApplyLevels(const Dive::AudioLevels& NewLevels)
{
    Levels=NewLevels; Levels.Sanitize();
    for(int i=0;i<Sounds.Num();++i) Sounds[i]->SetVolumeMultiplier(Gains[i]*Levels.Gain(Categories[i]));
}
void UDiveAudio::Gain(int i,double Value)
{ Gains[i]=Value; Sounds[i]->SetVolumeMultiplier(Value*Levels.Gain(Categories[i])); }
void UDiveAudio::Play(int i,float Pitch)
{ Sounds[i]->SetPitchMultiplier(Pitch); Sounds[i]->Play(); }
bool UDiveAudio::Advance(float Dt,const Dive::State& s,const Dive::Config& c,double EyeDepth)
{
    if(bPaused||Sounds.Num()!=Count) return false;
    const auto a=Model.Step(Dt,s,c,EyeDepth);
    Gain(Inhale,.52+.25*s.Exertion); Gain(Exhale,.48+.2*s.Exertion); Gain(Bubbles,.55*a.Wet);
    Gain(Heart,.12+.6*s.Stress+.2*s.Exertion); Gain(Warning,.65);
    // Smooth continuous channels; single shots remain timed to simulation events.
    Gain(Inflate,Dive::Approach(Gains[Inflate],.65*Dive::Clamp(s.BCDInflowLPS/2,0,1),18,Dt));
    Gain(Vent,Dive::Approach(Gains[Vent],.7*Dive::Clamp(s.BCDOutflowLPS/3,0,1),18,Dt));
    Gain(Freeflow,Dive::Approach(Gains[Freeflow],.85*Dive::Clamp(s.FreeflowLocalLPS/6,0,1),18,Dt));
    Gain(Underwater,Dive::Approach(Gains[Underwater],.55*a.Wet,6,Dt));
    Gain(Surface,Dive::Approach(Gains[Surface],.5*a.Surface,6,Dt));
    Gain(Wind,Dive::Approach(Gains[Wind],.6*a.Wind,6,Dt));
    Gain(Rain,Dive::Approach(Gains[Rain],.55*a.Rain,6,Dt));
    for(int i=Surface;i<=Rain;++i) Sounds[i]->SetLowPassFilterFrequency(a.CutoffHz);
    const float pitch=FMath::Clamp(s.RespiratoryBPM/16.,.65,2.5);
    if(a.Inhale) { Play(Inhale,pitch); ++Inhales; }
    if(a.Exhale)
    {
        Play(Exhale,pitch); ++Exhales;
        Sounds[Bubbles]->SetAbsolute(true,false,false);
        Sounds[Bubbles]->SetWorldLocation(Listener->GetComponentLocation()+Listener->GetForwardVector()*8+FVector(0,0,-12));
        Play(Bubbles,FMath::Clamp(1.+EyeDepth*.003,1.,1.2));
    }
    if(Sounds[Bubbles]->IsPlaying()) Sounds[Bubbles]->AddWorldOffset(FVector(0,0,30*Dt));
    if(a.Heartbeat) { Play(Heart,FMath::Clamp(s.HeartBPM/90.,.8,1.5)); ++Heartbeats; }
    if(a.Warning) { Play(Warning); ++Warnings; }
    InflationSteps+=s.BCDInflowLPS>0; VentSteps+=s.BCDOutflowLPS>0;
    return a.Exhale;
}
bool UDiveAudio::Validate(FString& Error) const
{
    if(Sounds.Num()!=Count) { Error=TEXT("Incomplete audio channels"); return false; }
    for(int i=0;i<Count;++i)
    {
        auto* wave=Cast<USoundWave>(Sounds[i]->Sound);
        if(!wave||wave->Duration<=0||bool(wave->bLooping)!=Loop(i))
        { Error=FString::Printf(TEXT("Missing/invalid sound or loop flag: %s"),Names[i]); return false; }
    }
    return true;
}
