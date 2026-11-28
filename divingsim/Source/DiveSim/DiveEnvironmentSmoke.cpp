#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveConfigIO.h"
#include "DiveWorld.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Engine/World.h"
#include "TimerManager.h"

bool ADiveHUD::EnvironmentSmokeTest(int Phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn());if(!Diver) return false;
    auto& plan=Diver->PendingConfig;
    auto shot=[this](const TCHAR* name)
    {
        // Also capture the settled view for comparison with the first menu frame.
        FTimerHandle timer;
        GetWorld()->GetTimerManager().SetTimer(timer,FTimerDelegate::CreateWeakLambda(this,[name=FString(name)]()
        { FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(name+TEXT(".png")),true,false); }),.5f,false);
    };
    if(Phase==0)
    {
        bEnvironmentPage=true;Diver->bEnglish=false;
        plan.Date={2026,2,15};plan.VisibilityMetres=27;plan.CurrentMetresPerSecond=.17;plan.SurfaceTemperatureC=22;
        NotifyHitBoxClick(TEXT("auto_water"));NotifyHitBoxClick(TEXT("auto_visibility"));NotifyHitBoxClick(TEXT("current_source"));
        NotifyHitBoxClick(TEXT("surface_temp"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots/EnvironmentWinterFirstFrame_DE.png"),true,false);
        shot(TEXT("EnvironmentWinter_DE"));
        if(ConfigValue(TEXT("wind"))!=TEXT("3 m/s")||ConfigValue(TEXT("date"))!=TEXT("2026-02-15")||ConfigValue(TEXT("visibility"))!=TEXT("18 m")) return false;
        return !ConfigDialog&&plan.AutoTemperature&&plan.AutoVisibility&&plan.CurrentSource==Dive::CurrentSetting::Automatic&&
            FMath::Abs(plan.Resolved().SurfaceTemperatureC-5.836)<1e-9;
    }
    if(Phase==1)
    {
        Diver->bEnglish=true;NotifyHitBoxClick(TEXT("date"));if(!SubmitConfigInput(TEXT("2026-08-15"))) return false;
        shot(TEXT("EnvironmentSummer_EN"));
        return FMath::Abs(plan.Resolved().SurfaceTemperatureC-17.085)<1e-9&&plan.SurfaceTemperatureC==22&&plan.VisibilityMetres==27;
    }
    if(Phase==2)
    {
        NotifyHitBoxClick(TEXT("visibility"));if(!ConfigDialog) return false;
        if(SubmitConfigInput(TEXT("41"))||!plan.AutoVisibility) return false;
        CloseConfigInput();if(!plan.AutoVisibility) return false;
        NotifyHitBoxClick(TEXT("visibility"));if(!SubmitConfigInput(TEXT("23"))||plan.AutoVisibility) return false;
        NotifyHitBoxClick(TEXT("auto_water"));
        if(plan.AutoTemperature||plan.Resolved().SurfaceTemperatureC!=22) return false;
        NotifyHitBoxClick(TEXT("current_source"));if(plan.Resolved().CurrentMetresPerSecond!=0) return false;
        NotifyHitBoxClick(TEXT("current_source"));if(plan.Resolved().CurrentMetresPerSecond!=.08) return false;
        NotifyHitBoxClick(TEXT("current_source"));if(plan.Resolved().CurrentMetresPerSecond!=.3) return false;
        NotifyHitBoxClick(TEXT("current_source"));if(plan.Resolved().CurrentMetresPerSecond!=.17) return false;
        shot(TEXT("EnvironmentManual_EN"));return true;
    }
    if(Phase==3)
    {
        NotifyHitBoxClick(TEXT("auto_water"));NotifyHitBoxClick(TEXT("auto_visibility"));NotifyHitBoxClick(TEXT("current_source"));
        plan.SelectWeather(2);Diver->SaveSettings();const auto expected=plan;const int entry=Diver->EntryIndex;
        plan=Dive::Config{};Diver->LoadSettings();
        if(!DiveConfigIO::Equal(expected,entry,plan,Diver->EntryIndex)) return false;
        FDiveProfiles store;const FString path=FPaths::ProjectSavedDir()/TEXT("EnvironmentProfilesTest.json");
        if(!store.Load(path)) return false;
        FString id;if(!store.Add(FString::Printf(TEXT("Environment %d"),store.Items.Num()),plan,entry,id)) return false;
        FDiveProfiles reloaded;if(!reloaded.Load(path)||!reloaded.Find(id)) return false;
        if(!DiveConfigIO::Equal(expected,entry,reloaded.Find(id)->Config,reloaded.Find(id)->Entry)) return false;
        shot(TEXT("EnvironmentRain_EN"));return true;
    }
    if(Phase==4)
    {
        // Exercise the real start path; preserve manual settings in the plan.
        Diver->StartDive();const auto& actual=Diver->Simulation.Settings;
        return plan.VisibilityMetres==23&&actual.AutoVisibility&&actual.AutoTemperature&&
            FMath::Abs(actual.VisibilityMetres-6.4)<1e-9&&FMath::Abs(actual.CurrentMetresPerSecond-.112)<1e-9&&
            FMath::Abs(actual.SurfaceTemperatureC-17.085)<1e-9;
    }
    if(Phase==5)
    {
        shot(TEXT("EnvironmentDive_EN"));
        const auto& state=Diver->Simulation.Data;
        return Diver->bDiveActive&&state.ElapsedSeconds>2&&
            FMath::Abs(state.WaterTemperatureC-Dive::EnvironmentScenario::Temperature({2026,8,15},state.DepthMetres))<1e-9;
    }
    if(Phase==6)
    {
        Diver->SetMenu(true);const auto before=plan;
        NotifyHitBoxClick(TEXT("auto_water"));NotifyHitBoxClick(TEXT("auto_visibility"));NotifyHitBoxClick(TEXT("current_source"));
        NotifyHitBoxClick(TEXT("visibility"));NotifyHitBoxClick(TEXT("date"));
        const bool valid=!ConfigDialog&&DiveConfigIO::Equal(before,0,plan,0);
        shot(TEXT("EnvironmentLocked_EN"));
        FTimerHandle resume;
        GetWorld()->GetTimerManager().SetTimer(resume,FTimerDelegate::CreateWeakLambda(Diver,[pawn=Diver](){pawn->SetMenu(false);}),.75f,false);
        return valid;
    }
    if(Phase==7)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_ENVIRONMENT_RUNTIME temperature=%.6f visibility=%.6f current=%.6f seconds=%.3f"),
            Diver->Simulation.Data.WaterTemperatureC,Diver->Simulation.Settings.VisibilityMetres,
            Diver->Simulation.Settings.CurrentMetresPerSecond,Diver->Simulation.Data.ElapsedSeconds);
        Diver->FinishDive(TEXT("smoke_environment_completed"));return true;
    }
    return false;
}
