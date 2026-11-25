#include "DiveWorld.h"
#include "DivePawn.h"
#include "DiveCelestialActor.h"
#include "CelestialMaths.h"
#include "CelestialVaultDaySequenceActor.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
// EPSG:2056 terrain origin (E2726000,N1219000), transformed to WGS84.
// Native vault: +X east/-Y north. Terrain: +X grid north/+Y grid east.
// Grid north is 1.21247 degrees east of true north at this fixed origin.
constexpr double Latitude=47.11000249573725,Longitude=9.098818483859683;
constexpr double NorthRotation=90-1.2124710174696076;
void SetDate(ACelestialVaultDaySequenceActor* Sky,Dive::CivilDate Date)
{
    Sky->Year=Date.Year; Sky->Month=static_cast<EMonth>(Date.Month); Sky->Day=Date.Day;
}
float SequenceHour(double UTCSeconds,int Day)
{
    // A double just before midnight can round to 24.f. SetTimeOfDay wraps that
    // to zero, but the calendar still belongs to yesterday (a one-day error).
    // Keep the native float in [0,24); the remaining error is below 7 ms.
    return FMath::Min(static_cast<float>((UTCSeconds-Day*86400.)/3600.),std::nextafter(24.f,0.f));
}
}

void ADiveWorld::CreateSky()
{
    const auto clock=CurrentMoment();
    const int day=static_cast<int>(std::floor(clock.UTCSeconds/86400.));
    const float hour=SequenceHour(clock.UTCSeconds,day);
    Celestial=GetWorld()->SpawnActorDeferred<ACelestialVaultDaySequenceActor>(
        ADiveCelestialActor::StaticClass(),FTransform::Identity,this);
    Celestial->bUseCurrentDate=false; Celestial->GMT_TimeZone=0;
    Celestial->DaylightSavingsMode=EDaylightSavingsMode::None;
    Celestial->Latitude=Latitude; Celestial->Longitude=Longitude;
    Celestial->NorthOffset=NorthRotation;
    Celestial->NorthOffsetComponent->SetRelativeRotation(FRotator(0,NorthRotation,0));
    SetDate(Celestial,Dive::CivilDate::FromSerialDay(day));
    Celestial->SetInitialTimeOfDay(hour);
    Celestial->FinishSpawning(FTransform::Identity);
    Celestial->Pause();
    StarMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/CelestialVault/Materials/MI_Stars_EnergyConservative.MI_Stars_EnergyConservative"));
    Celestial->StarsComponent->SetMaterial(0,StarMaterial);
    if(bOpticsSmoke)
    {
        FString hide; FParse::Value(FCommandLine::Get(),TEXT("DiveHideSky="),hide,false);
        struct Layer { const TCHAR* Name; UMeshComponent* Mesh; };
        const Layer layers[]={
            {TEXT("planets"),Celestial->PlanetsComponent},{TEXT("stars"),Celestial->StarsComponent},
            {TEXT("deep"),Celestial->DeepSkyComponent},{TEXT("proxy"),Celestial->VelocityVectorsProxyComponent},
            {TEXT("moon"),Celestial->MoonDiscComponent}};
        for(const auto& layer:layers)
        {
            if(hide.Contains(layer.Name)) layer.Mesh->SetVisibility(false);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SKY_LAYER name=%s object=%s material=%s visible=%d"),
                layer.Name,*layer.Mesh->GetName(),*GetPathNameSafe(layer.Mesh->GetMaterial(0)),layer.Mesh->IsVisible());
        }
    }
    Celestial->AddTickPrerequisiteActor(this);
    Sun=Celestial->SunLightComponent; Sky=Celestial->SkyLightComponent;
    Fog=Celestial->ExponentialHeightFogComponent; Clouds=Celestial->VolumetricCloudComponent;
    // The dive camera owns exposure; avoid unrelated automatic exposure overrides.
    Celestial->GlobalPostProcessVolume->bEnabled=false;
    Sun->SetLightColor(FLinearColor::White);
    Sun->SetVolumetricScatteringIntensity(2);
    Sky->SetLightColor(FLinearColor::White);
    Sky->SetRealTimeCaptureEnabled(true);
    if(bOpticsSmoke&&FParse::Param(FCommandLine::Get(),TEXT("DiveCelestialReferences"))) ValidateCelestialReferences();
}

void ADiveWorld::UpdateDaylight()
{
    if(!Celestial) return;
    const auto clock=CurrentMoment(); DayHour=clock.Hour;
    if(clock.UTCSeconds==LastSkyUTC) return;
    const int day=static_cast<int>(std::floor(clock.UTCSeconds/86400.));
    const auto date=Dive::CivilDate::FromSerialDay(day);
    const float hour=SequenceHour(clock.UTCSeconds,day);
    if(Celestial->Year!=date.Year||static_cast<int>(Celestial->Month)!=date.Month||Celestial->Day!=date.Day)
    {
        SetDate(Celestial,date);
        // RebuildAll is the plugin's Blueprint-callable refresh API (private in C++).
        // Only rebuild catalogs/motion caches when the UTC date actually changes.
        Celestial->ProcessEvent(Celestial->FindFunctionChecked(TEXT("RebuildAll")),nullptr);
    }
    Celestial->SunLightIntensity=Dive::EnvironmentLight::SunLux*Dive::WeatherScenario::At(WeatherSettings.Weather).DirectLight;
    Celestial->MoonLightIntensity=.1f*Dive::WeatherScenario::At(WeatherSettings.Weather).DirectLight;
    // Native UpdateBodiesMotion reads this transform before updating its angle.
    // Prepare the current angle so a start-time jump has no one-frame direction lag.
    Celestial->CelestialVaultComponent->SetRelativeRotation(FRotator(0,Celestial->GetCelestialVaultAngle(hour),0));
    if(!Celestial->SetTimeOfDay(hour))
    {
        UE_LOG(LogTemp,Error,TEXT("DIVE_LIGHT_FAIL Celestial sequence unavailable"));
        if(bOpticsSmoke) FPlatformMisc::RequestExitWithStatus(false,1);
        return;
    }
    // UE 5.8.4's constructor uses an editor redirector omitted from cooked assets.
    // Keep the canonical star material explicitly assigned after evaluation.
    if(Celestial->StarsComponent->GetMaterial(0)!=StarMaterial)
        Celestial->StarsComponent->SetMaterial(0,StarMaterial);
    // UE 5.8.4 derives MoonAge from a mean synodic period; its phase can differ
    // materially from the ephemeris. Use the actual Sun/Moon/observer triangle.
    // Re-evaluate once per simulation second (and immediately after configuration).
    if(LastSkyUTC<0||FMath::Abs(clock.UTCSeconds-LastMoonPhaseUTC)>=1)
    {
        const double jd=Celestial->GetJulianDate();
        const auto earth=UCelestialMaths::GetPlanetaryBodyKinematicState_AU(jd,EVSOP87BodyType::Earth);
        const auto sunInfo=Celestial->ComputeSunInfo_UsingKnownState(jd,earth);
        const auto moonInfo=Celestial->ComputeMoonInfo_UsingKnownState(jd,earth);
        auto position=[&](const FCelestialBody& body)
        {
            auto relative=UCelestialMaths::RADECToXYZ_RH(body.RAGeocentric*15,body.DECGeocentric,
                UCelestialMaths::AstronomicalUnitsToMeters(body.DistanceInAU)*100);
            relative.Y*=-1; // Native left-handed equatorial frame.
            return Celestial->CelestialVaultComponent->GetComponentTransform().TransformPosition(relative);
        };
        const FVector moonPosition=position(moonInfo);
        const FVector moonToSun=(position(sunInfo)-moonPosition).GetSafeNormal();
        const FVector observer=Diver?Diver->GetPawnViewLocation():FVector::ZeroVector;
        const FVector moonToObserver=(observer-moonPosition).GetSafeNormal();
        const double phaseAngle=FMath::Acos(FMath::Clamp(FVector::DotProduct(moonToSun,moonToObserver),-1.,1.));
        const double elongationRA=FMath::Fmod(moonInfo.RAGeocentric-sunInfo.RAGeocentric+24,24.);
        MoonPhaseAge=.5+(elongationRA<=12?-1:1)*phaseAngle/(2*PI);
        LastMoonPhaseUTC=clock.UTCSeconds;
    }
    CastChecked<ADiveCelestialActor>(Celestial)->ApplyEphemerisPhase(MoonPhaseAge);
    // The template sequence also animates cloud coverage. The selected weather
    // owns these parameters; apply it after sequence evaluation, on the actual MID.
    if(CloudMaterial)
    {
        Clouds->SetMaterial(CloudMaterial);
        const auto weather=Dive::WeatherScenario::At(WeatherSettings.Weather);
        CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"),weather.CloudCoverage);
        CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalDensity"),weather.CloudDensity);
        CloudMaterial->SetScalarParameterValue(TEXT("StormClouds"),0);
    }
    const double elevation=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-Sun->GetForwardVector().Z,-1.,1.)));
    const auto* moon=Celestial->MoonLightComponent.Get();
    const double moonElevation=FMath::RadiansToDegrees(FMath::Asin(FMath::Clamp(-moon->GetForwardVector().Z,-1.,1.)));
    Lighting=Dive::EnvironmentLight::AtElevation(elevation,WeatherSettings.Weather,moonElevation,moon->Intensity);
    LastSkyUTC=clock.UTCSeconds;
}

bool ADiveWorld::ValidateCelestial(FString& Error) const
{
    const auto clock=CurrentMoment();
    const double expectedJD=2440587.5+clock.UTCSeconds/86400.;
    if(!Celestial||!Celestial->IsPaused()||FMath::Abs(Celestial->GetJulianDate()-expectedJD)*86400>.1)
    {
        Error=FString::Printf(TEXT("Celestial UTC clock mismatch: paused=%d native_date=%s native_hour=%.9f error_seconds=%.6f elapsed=%.6f"),
            Celestial&&Celestial->IsPaused(),Celestial?*Celestial->GetDate().ToIso8601():TEXT("missing"),
            Celestial?Celestial->GetTimeOfDay():-1.,Celestial?(Celestial->GetJulianDate()-expectedJD)*86400:0,
            Diver?Diver->Simulation.Data.ElapsedSeconds:0);
        return false;
    }
    if(!StarMaterial||Celestial->StarsComponent->GetMaterial(0)!=StarMaterial)
    { Error=TEXT("Star catalog uses the wrong material"); return false; }
    auto* moonMaterial=Cast<UMaterialInstanceDynamic>(Celestial->MoonDiscComponent->GetMaterial(0));
    if(!moonMaterial||FMath::Abs(moonMaterial->K2_GetScalarParameterValue(TEXT("MoonAge"))-MoonPhaseAge)>1.e-5)
    { Error=TEXT("Moon material does not use the ephemeris phase"); return false; }
    if(Celestial->StarsComponent->GetInstanceCount()<1000||!Celestial->MoonDiscComponent->GetStaticMesh())
    { Error=TEXT("Celestial star catalog or moon mesh missing"); return false; }
    if(FMath::Abs(FMath::Sin(FMath::DegreesToRadians(Lighting.ElevationDegrees))+Sun->GetForwardVector().Z)>1.e-6)
    { Error=TEXT("Solar light and water ambient use different elevations"); return false; }
    return true;
}

void ADiveWorld::ValidateCelestialReferences()
{
    // NASA/JPL Horizons DE441 observer ephemerides, airless, at the terrain origin.
    // Native moon is projected onto a 380,000 km vault: allow 0.15 deg parallax error.
    struct Reference { Dive::CivilDate Date; double Hour,SunAz,SunAlt,MoonAz,MoonAlt,MoonPercent; };
    const Reference references[]={
        {{2026,3,29},1+59./60,28.266442,-35.791002,265.273757,26.313880,82.82843},
        {{2026,3,29},3+1./60,28.847516,-35.627787,265.635379,25.978151,82.83887},
        {{2026,6,21},13,165.667994,65.785494,89.370258,-1.113330,46.15572},
        {{2026,6,21},21,302.723791,2.354863,212.792399,35.370038,48.88827},
        {{2026,6,22},0,339.545009,-16.864573,253.143837,10.895865,49.91980},
        {{2026,12,21},13,189.291480,18.922209,44.430670,-7.459107,90.49632}
    };
    const auto saved=WeatherSettings;
    bool valid=true;
    for(const auto& ref:references)
    {
        WeatherSettings.Date=ref.Date; WeatherSettings.Hour=ref.Hour; LastSkyUTC=-1;
        UpdateDaylight();
        const auto direction=[](double az,double alt)
        {
            return FRotator(alt,az-(90-NorthRotation),0).Vector();
        };
        const auto difference=[](FVector a,FVector b)
        { return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(a,b),-1.,1.))); };
        const double sunError=difference(-Sun->GetForwardVector(),direction(ref.SunAz,ref.SunAlt));
        const double moonError=difference(-Celestial->MoonLightComponent->GetForwardVector(),direction(ref.MoonAz,ref.MoonAlt));
        FString error;
        const double phaseError=FMath::Abs(Celestial->MoonLightComponent->Intensity/.1*100-ref.MoonPercent);
        const bool passed=ValidateCelestial(error)&&sunError<.05&&moonError<.15&&phaseError<.2;
        valid&=passed;
        UE_LOG(LogTemp,Display,TEXT("DIVE_CELESTIAL_REFERENCE %s date=%s hour=%.6f sun_error_deg=%.6f moon_error_deg=%.6f phase_error_percentage_points=%.6f elevation=%.6f reason=%s"),
            passed?TEXT("PASS"):TEXT("FAIL"),UTF8_TO_TCHAR(ref.Date.Text().c_str()),ref.Hour,sunError,moonError,phaseError,Lighting.ElevationDegrees,*error);
    }
    WeatherSettings=saved; LastSkyUTC=-1; UpdateDaylight();
    UE_LOG(LogTemp,Display,TEXT("DIVE_CELESTIAL_%s references=6"),valid?TEXT("PASS"):TEXT("FAIL"));
    if(!valid) FPlatformMisc::RequestExitWithStatus(false,1);
}
