#include "DivePawn.h"
#include "DiveWorld.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ADivePawn::TickDayCycleTest(float DeltaSeconds)
{
    if(!bDiveActive||!Environment) return;
    const double elapsed=Simulation.Data.ElapsedSeconds;
    const auto clock=Environment->CurrentMoment();
    const double hour=clock.Hour;
    auto fail=[](const TCHAR* why)
    { UE_LOG(LogTemp,Error,TEXT("DIVE_DAYCYCLE_FAIL %s"),why); FPlatformMisc::RequestExitWithStatus(false,1); };
    auto shot=[&](const TCHAR* phase)
    {
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/
            FString::Printf(TEXT("Screenshots/DayCycle_%s_%02d.png"),phase,FMath::FloorToInt(PendingConfig.Hour)),true,false);
    };
    const double expected=Dive::ScenarioClock::At(PendingConfig.Date,PendingConfig.Hour,elapsed).Hour;
    if(FMath::Abs(hour-expected)>1.e-9) { fail(TEXT("World clock is detached from elapsed simulation time")); return; }
    FString skyError;
    if(!Environment->ValidateCelestial(skyError)) { fail(*skyError); return; }
    if(DayCycleSmokePhase==0&&elapsed>=10)
    {
        DayCyclePauseSeconds=elapsed; DayCyclePauseHour=hour; DayCyclePauseWall=0;
        SetMenu(true); DayCycleSmokePhase=1;
    }
    else if(DayCycleSmokePhase==1)
    {
        DayCyclePauseWall+=DeltaSeconds;
        if(!bMenu||elapsed!=DayCyclePauseSeconds||hour!=DayCyclePauseHour)
        { fail(TEXT("Pause advanced the simulation clock")); return; }
        if(DayCyclePauseWall>=4)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_DAYCYCLE_PAUSE_PASS wall=%.3f sim=%.3f hour=%.9f"),DayCyclePauseWall,elapsed,hour);
            SetMenu(false); DayCycleSmokePhase=2;
        }
    }
    else if(DayCycleSmokePhase==2&&elapsed>=25)
    {
        shot(TEXT("Before")); DayCycleSmokePhase=3;
        UE_LOG(LogTemp,Display,TEXT("DIVE_DAYCYCLE_BEFORE elapsed=%.3f hour=%.9f solar_source=%.9f"),elapsed,hour,
            Dive::WeatherScenario::At(PendingConfig.Weather).DirectLight);
    }
    else if(DayCycleSmokePhase==3&&elapsed>=65)
    {
        const int boundary=FMath::FloorToInt(Dive::ScenarioClock::At(PendingConfig.Date,PendingConfig.Hour,65).Hour);
        if(FMath::FloorToInt(hour)!=boundary) { fail(TEXT("Expected hour boundary was not crossed")); return; }
        shot(TEXT("After")); DayCycleSmokePhase=4;
    }
    else if(DayCycleSmokePhase==4&&elapsed>=88)
    {
        UE_LOG(LogTemp,Display,TEXT("DIVE_DAYCYCLE_PASS elapsed=%.3f start=%.9f current=%.9f paused_wall=%.3f solar_source=%.9f date=%s offset_minutes=%d"),
            elapsed,PendingConfig.Hour,hour,DayCyclePauseWall,Dive::WeatherScenario::At(PendingConfig.Weather).DirectLight,UTF8_TO_TCHAR(clock.Date.Text().c_str()),clock.OffsetMinutes);
        DayCycleSmokePhase=5;
    }
}
