#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveConfigIO.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Dom/JsonObject.h"

bool ADiveHUD::LoadoutSmokeTest(int phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver) return false;
    auto& c=Diver->PendingConfig;const int test=Diver->LoadoutTestCase;
    auto shot=[&](const TCHAR* stage) { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/
        FString::Printf(TEXT("Screenshots/Loadout_%d_%s.png"),test,stage),true,false); };
    if(phase==0)
    {
        if(test==1||test==3) NotifyHitBoxClick(TEXT("tank"));
        if(test==2) { NotifyHitBoxClick(TEXT("tank"));NotifyHitBoxClick(TEXT("tank")); }
        if(test==1) { NotifyHitBoxClick(TEXT("cylinder_material"));NotifyHitBoxClick(TEXT("suit")); }
        if(test>0) NotifyHitBoxClick(TEXT("bcd"));
        if(test==1||test==3) NotifyHitBoxClick(TEXT("bcd"));
        if(test>0) NotifyHitBoxClick(TEXT("torch"));
        if(test==1||test==3) NotifyHitBoxClick(TEXT("torch"));
        if(test==3)
        { if(!OpenConfigInput(TEXT("weight"))||!SubmitConfigInput(TEXT("12"))) return false; }
        NotifyHitBoxClick(TEXT("environment"));
        NotifyHitBoxClick(TEXT("loadout"));
        const double expected[]={117,123.97,112.95,128.47};
        const Dive::Model prepared(c);
        if(!bLoadoutPage||bEnvironmentPage||std::abs(prepared.Data.TotalMassKg-expected[test])>1.e-8) return false;
        const auto plan=c;Diver->SaveSettings();c=Dive::Config{};Diver->LoadSettings();
        if(!DiveConfigIO::Equal(plan,0,c,Diver->EntryIndex)) return false;
        Diver->bEnglish=false;shot(TEXT("DE"));
    }
    if(phase==1) { Diver->bEnglish=true;shot(TEXT("EN")); }
    if(phase==2) { NotifyHitBoxClick(TEXT("start"));return Diver->bDiveActive; }
    if(phase==3)
    {
        NotifyHitBoxClick(TEXT("loadout"));
        const auto before=c;
        for(const TCHAR* id:{TEXT("cylinder_material"),TEXT("bcd"),TEXT("torch"),TEXT("tank"),TEXT("weight")}) NotifyHitBoxClick(id);
        if(!DiveConfigIO::Equal(before,0,c,Diver->EntryIndex)||ConfigInputOpen()) return false;
        const auto& s=Diver->Simulation.Data;
        if(std::abs(s.BuoyancyNewtons-(s.DisplacedLitres-s.TotalMassKg)*9.80665)>1.e-6) return false;
        shot(TEXT("Paused"));
    }
    return true;
}
