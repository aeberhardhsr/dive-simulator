#include "DiveHUD.h"
#include "DivePawn.h"
#include "DiveConfigIO.h"
#include "DiveTextDialog.h"
#include "Simulation/ConfigInput.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
const Dive::ConfigInput* Input(FName Id)
{ return Dive::FindConfigInput(TCHAR_TO_UTF8(*Id.ToString().ToLower())); }
FString Number(const Dive::ConfigInput& Spec,double Value)
{ return UTF8_TO_TCHAR(Dive::FormatConfigInput(Spec,Value).c_str()); }
}

FString ADiveHUD::ConfigValue(FName Id) const
{
    if(Id==TEXT("date")&&Diver) return UTF8_TO_TCHAR(Diver->PendingConfig.Date.Text().c_str());
    const auto* spec=Input(Id); if(!spec||!Diver) return FString();
    const FString value=Number(*spec,Diver->PendingConfig.Resolved().*(spec->Member));
    return spec->Clock?value:value+TEXT(" ")+UTF8_TO_TCHAR(spec->Unit);
}
FString ADiveHUD::ConfigInputTitle() const
{
    struct FLabel { const TCHAR* Id; const TCHAR* German; const TCHAR* English; };
    const FLabel labels[]={
        {TEXT("date"),TEXT("Szenariodatum"),TEXT("Scenario date")},
        {TEXT("hour"),TEXT("Tageszeit"),TEXT("Time of day")},
        {TEXT("pressure"),TEXT("Startdruck"),TEXT("Starting pressure")},
        {TEXT("weight"),TEXT("Bleigewicht"),TEXT("Ballast")},
        {TEXT("visibility"),TEXT("Sichtweite"),TEXT("Visibility")},
        {TEXT("current"),TEXT("Strömung"),TEXT("Current")},
        {TEXT("depth"),TEXT("Geplante Tiefe"),TEXT("Planned depth")},
        {TEXT("duration"),TEXT("Geplante Dauer"),TEXT("Planned duration")},
        {TEXT("wind"),TEXT("Windstärke"),TEXT("Wind speed")},
        {TEXT("winddir"),TEXT("Wind weht nach"),TEXT("Wind blows toward")},
        {TEXT("surface_temp"),TEXT("Oberflächentemperatur"),TEXT("Surface temperature")},
        {TEXT("air_temp"),TEXT("Lufttemperatur"),TEXT("Air temperature")},
        {TEXT("bottom_temp"),TEXT("Tiefenwasser"),TEXT("Deep water")},
        {TEXT("thermocline"),TEXT("Sprungschicht: Tiefe"),TEXT("Thermocline depth")},
        {TEXT("thermal_width"),TEXT("Schichtübergang"),TEXT("Transition scale")}
    };
    for(const auto& label:labels) if(ConfigEditing==label.Id) return Diver->Local(label.German,label.English);
    return FString();
}
FString ADiveHUD::ConfigInputHint() const
{
    if(ConfigEditing==TEXT("date")) return Diver->Local(
        TEXT("JJJJ-MM-TT, 2000–2099. Schweizer Ortszeit.\nIm Herbst gilt bei doppelter Uhrzeit die erste (Sommerzeit).\nEnter übernimmt; Escape verwirft."),
        TEXT("YYYY-MM-DD, 2000–2099. Swiss local time.\nRepeated autumn times use the first occurrence (summer time).\nEnter applies; Escape cancels."));
    const auto* spec=Input(ConfigEditing); if(!spec) return FString();
    const FString range=spec->Clock?TEXT("00:00 – 23:59 (HH:MM)"):
        Number(*spec,spec->Minimum)+TEXT(" – ")+Number(*spec,spec->Maximum)+TEXT(" ")+UTF8_TO_TCHAR(spec->Unit);
    return range+TEXT("\n")+(spec->Clock?FString():Diver->Local(TEXT("Dezimalpunkt oder Komma. "),TEXT("Decimal point or comma. ")))+
        Diver->Local(TEXT("Enter übernimmt; Escape verwirft."),TEXT("Enter applies; Escape cancels."));
}
bool ADiveHUD::OpenConfigInput(FName Id)
{
    const auto* spec=Input(Id);
    if((!spec&&Id!=TEXT("date"))||!Diver||Diver->bDiveActive||!Diver->bMenu||ConfigDialog||!GEngine||!GEngine->GameViewport) return false;
    if(Diver->PendingConfig.AutoTemperature&&(Id==TEXT("surface_temp")||Id==TEXT("bottom_temp")||Id==TEXT("thermocline")||Id==TEXT("thermal_width"))) return false;
    ConfigEditing=Id; ConfigError.Empty();
    const TWeakObjectPtr<ADiveHUD> weak(this);
    auto dialog=SNew(SDiveTextDialog).English(Diver->bEnglish)
        .Title(FText::FromString(ConfigInputTitle()))
        .Hint(FText::FromString(ConfigInputHint()))
        .Initial(FText::FromString(Id==TEXT("date")?ConfigValue(Id):Number(*spec,Diver->PendingConfig.Resolved().*(spec->Member))))
        .Error_Lambda([weak](){return weak.IsValid()?FText::FromString(weak->ConfigError):FText::GetEmpty();})
        .OnSubmit_Lambda([weak](const FText& Value,ETextCommit::Type){if(weak.IsValid()) weak->SubmitConfigInput(Value.ToString());})
        .OnCancel_Lambda([weak](){if(weak.IsValid()) weak->CloseConfigInput();});
    ConfigDialog=dialog; ConfigEditor=dialog->Editor;
    GEngine->GameViewport->AddViewportWidgetContent(dialog,100);
    FInputModeUIOnly mode; mode.SetWidgetToFocus(ConfigEditor); GetOwningPlayerController()->SetInputMode(mode);
    FSlateApplication::Get().SetKeyboardFocus(ConfigEditor,EFocusCause::SetDirectly);
    return true;
}
bool ADiveHUD::SubmitConfigInput(const FString& Value)
{
    const auto* spec=Input(ConfigEditing);
    if(!ConfigDialog||(!spec&&ConfigEditing!=TEXT("date"))||!Diver||Diver->bDiveActive) return false;
    auto candidate=Diver->PendingConfig;
    auto clockValid=[&]()
    {
        double utc=0;
        if(Dive::ScenarioClock::StartUTC(candidate.Date,candidate.Hour,utc)) return true;
        ConfigError=Diver->Local(TEXT("Diese Uhrzeit entfällt bei der Sommerzeitumstellung. Bitte eine andere Uhrzeit oder ein anderes Datum wählen."),
            TEXT("This time does not exist during the spring clock change. Choose another time or date."));
        return false;
    };
    if(ConfigEditing==TEXT("date"))
    {
        if(!Dive::CivilDate::Parse(TCHAR_TO_UTF8(*Value),candidate.Date))
        {
            ConfigError=Diver->Local(TEXT("Ein gültiges Datum als JJJJ-MM-TT eingeben (2000–2099)."),TEXT("Enter a valid date as YYYY-MM-DD (2000–2099).")); return false;
        }
        if(!clockValid()) return false;
        Diver->PendingConfig=candidate; Diver->SaveSettings(); CloseConfigInput(); return true;
    }
    double value=0;
    const auto error=Dive::ParseConfigInput(*spec,TCHAR_TO_UTF8(*Value),value);
    if(error!=Dive::ConfigInputError::None)
    {
        ConfigError=error==Dive::ConfigInputError::Range?
            Diver->Local(TEXT("Bitte einen Wert innerhalb des angegebenen Bereichs eingeben."),TEXT("Enter a value within the stated range.")):
            error==Dive::ConfigInputError::Precision?
            FString::Printf(TEXT("%s %d %s"),*Diver->Local(TEXT("Höchstens"),TEXT("Use at most")),spec->Decimals,
                *Diver->Local(TEXT("Nachkommastellen eingeben."),TEXT("decimal places."))):
            spec->Clock?Diver->Local(TEXT("Uhrzeit als HH:MM eingeben, zum Beispiel 07:05."),TEXT("Enter time as HH:MM, for example 07:05.")):
            Diver->Local(TEXT("Eine Zahl ohne Einheit eingeben, zum Beispiel 7,5."),TEXT("Enter a number without a unit, for example 7.5."));
        return false;
    }
    candidate.*(spec->Member)=value;
    if(ConfigEditing==TEXT("visibility")) candidate.AutoVisibility=false;
    if(ConfigEditing==TEXT("current")) candidate.CurrentSource=Dive::CurrentSetting::Custom;
    if(!clockValid()) return false;
    Diver->PendingConfig=candidate;
    Diver->SaveSettings(); CloseConfigInput(); return true;
}
void ADiveHUD::CloseConfigInput(bool RestoreFocus)
{
    if(ConfigDialog&&GEngine&&GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(ConfigDialog.ToSharedRef());
    ConfigDialog.Reset(); ConfigEditor.Reset(); ConfigEditing=NAME_None; ConfigError.Empty();
    if(RestoreFocus&&Diver) Diver->SetMenu(true);
}

bool ADiveHUD::ConfigSmokeTest(int Phase)
{
    Diver=Cast<ADivePawn>(GetOwningPawn()); if(!Diver) return false;
    auto shot=[](const TCHAR* Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Screenshots")/(FString(Name)+TEXT(".png")),true,false);};
    auto key=[](FKey Key)
    {
        const FKeyEvent event(Key,FModifierKeysState(),0,false,0,0);
        FSlateApplication::Get().ProcessKeyDownEvent(event);
        FSlateApplication::Get().ProcessKeyUpEvent(event);
    };
    if(Phase==0)
    {
        for(const auto& spec:Dive::ConfigInputs)
            if(ConfigValue(FName(UTF8_TO_TCHAR(spec.Id))).IsEmpty()) return false;
        Diver->bEnglish=false; NotifyHitBoxClick(TEXT("pressure"));
        if(!ConfigDialog) return false;
        ConfigEditor->SetText(FText::FromString(TEXT("213,5"))); shot(TEXT("ConfigPressure_DE")); return true;
    }
    if(Phase==1)
    {
        const auto before=Diver->PendingConfig;
        ConfigEditor->SetText(FText::FromString(TEXT("301")));
        key(EKeys::Enter);
        const bool rejected=DiveConfigIO::Equal(before,0,Diver->PendingConfig,0)&&!ConfigError.IsEmpty()&&ConfigDialog;
        shot(TEXT("ConfigRange_DE")); return rejected;
    }
    if(Phase==2)
    {
        ConfigEditor->SetText(FText::FromString(TEXT("213,5"))); key(EKeys::Enter);
        if(ConfigDialog||Diver->PendingConfig.StartPressureBar!=213.5) return false;
        NotifyHitBoxClick(TEXT("weight")); if(!SubmitConfigInput(TEXT("7,5"))) return false;
        NotifyHitBoxClick(TEXT("depth")); if(!SubmitConfigInput(TEXT("22.5"))) return false;
        NotifyHitBoxClick(TEXT("duration")); if(!SubmitConfigInput(TEXT("47"))) return false;
        NotifyHitBoxClick(TEXT("hour")); if(!ConfigDialog) return false;
        ConfigEditor->SetText(FText::FromString(TEXT("23:59"))); shot(TEXT("ConfigTime_DE")); return true;
    }
    if(Phase==3)
    {
        if(SubmitConfigInput(TEXT("24:00"))||SubmitConfigInput(TEXT("12:60"))||!SubmitConfigInput(TEXT("23:59"))) return false;
        const auto expected=Diver->PendingConfig; const int entry=Diver->EntryIndex;
        Diver->PendingConfig=Dive::Config{}; Diver->LoadSettings();
        const bool restored=DiveConfigIO::Equal(expected,entry,Diver->PendingConfig,Diver->EntryIndex);
        shot(TEXT("ConfigValues_DE")); return restored&&ConfigValue(TEXT("hour"))==TEXT("23:59");
    }
    if(Phase==4)
    {
        Diver->bEnglish=true; bEnvironmentPage=true;
        for(int weather=1;weather<Dive::WeatherScenario::Count;++weather)
        {
            NotifyHitBoxClick(TEXT("weather"));
            if(Diver->PendingConfig.Weather!=weather||Diver->PendingConfig.WindMetresPerSecond!=Dive::WeatherScenario::At(weather).Wind) return false;
        }
        Diver->SaveSettings(); Diver->PendingConfig.Weather=0; Diver->PendingConfig.WindMetresPerSecond=0; Diver->LoadSettings();
        if(Diver->PendingConfig.Weather!=3||Diver->PendingConfig.WindMetresPerSecond!=22) return false;
        NotifyHitBoxClick(TEXT("air_temp")); if(!ConfigDialog) return false;
        ConfigEditor->SetText(FText::FromString(TEXT("-3.5"))); shot(TEXT("ConfigAir_EN")); return true;
    }
    if(Phase==5)
    {
        const double before=Diver->PendingConfig.AirTemperatureC;
        key(EKeys::Escape); if(ConfigDialog||Diver->PendingConfig.AirTemperatureC!=before) return false;
        NotifyHitBoxClick(TEXT("air_temp")); if(!SubmitConfigInput(TEXT("-3.5"))) return false;
        NotifyHitBoxClick(TEXT("current")); if(!SubmitConfigInput(TEXT("0.17"))) return false;
        NotifyHitBoxClick(TEXT("wind")); if(!SubmitConfigInput(TEXT("21.5"))) return false;
        NotifyHitBoxClick(TEXT("hour")); if(!SubmitConfigInput(TEXT("07:05"))) return false;
        shot(TEXT("ConfigValues_EN")); return ConfigValue(TEXT("hour"))==TEXT("07:05");
    }
    if(Phase==6)
    {
        NotifyHitBoxClick(TEXT("date")); if(!ConfigDialog) return false;
        ConfigEditor->SetText(FText::FromString(TEXT("2024-02-29"))); shot(TEXT("ConfigDate_EN")); return true;
    }
    if(Phase==7)
    {
        const auto before=Diver->PendingConfig;
        if(SubmitConfigInput(TEXT("2025-02-29"))||SubmitConfigInput(TEXT("2100-01-01"))||
            !DiveConfigIO::Equal(before,0,Diver->PendingConfig,0)||!SubmitConfigInput(TEXT("2024-02-29"))) return false;
        const auto expected=Diver->PendingConfig; const int entry=Diver->EntryIndex;
        Diver->PendingConfig=Dive::Config{}; Diver->LoadSettings();
        shot(TEXT("ConfigDateValues_EN"));
        return DiveConfigIO::Equal(expected,entry,Diver->PendingConfig,Diver->EntryIndex)&&ConfigValue(TEXT("date"))==TEXT("2024-02-29");
    }
    if(Phase==8)
    {
        NotifyHitBoxClick(TEXT("hour")); if(!SubmitConfigInput(TEXT("02:30"))) return false;
        const auto before=Diver->PendingConfig;
        NotifyHitBoxClick(TEXT("date"));
        if(SubmitConfigInput(TEXT("2026-03-29"))||!DiveConfigIO::Equal(before,0,Diver->PendingConfig,0)) return false;
        CloseConfigInput(); NotifyHitBoxClick(TEXT("hour")); if(!SubmitConfigInput(TEXT("07:05"))) return false;
        NotifyHitBoxClick(TEXT("date")); if(!SubmitConfigInput(TEXT("2026-03-29"))) return false;
        NotifyHitBoxClick(TEXT("hour"));
        ConfigEditor->SetText(FText::FromString(TEXT("02:30")));
        const bool rejected=!SubmitConfigInput(TEXT("02:30"))&&Diver->PendingConfig.Hour==7+5./60&&!ConfigError.IsEmpty();
        shot(TEXT("ConfigSpringGap_EN")); return rejected;
    }
    if(Phase==9)
    {
        CloseConfigInput(); bEnvironmentPage=false;
        for(int i=1;i<Dive::Torch::Count;++i)
        { NotifyHitBoxClick(TEXT("torch")); if(Diver->PendingConfig.TorchModel!=i) return false; }
        Diver->SaveSettings();Diver->PendingConfig.TorchModel=0;Diver->LoadSettings();
        shot(TEXT("ConfigTorch_EN"));return Diver->PendingConfig.TorchModel==2;
    }
    if(Phase==10)
    {
        CloseConfigInput(); NotifyHitBoxClick(TEXT("date")); if(!SubmitConfigInput(TEXT("2024-02-29"))) return false;
        Diver->StartDive(); Diver->SetMenu(true);
        const auto before=Diver->PendingConfig;
        NotifyHitBoxClick(TEXT("pressure")); NotifyHitBoxClick(TEXT("date")); NotifyHitBoxClick(TEXT("torch"));
        const bool locked=!ConfigDialog&&DiveConfigIO::Equal(before,0,Diver->PendingConfig,0)&&
            Diver->Simulation.Settings.Date==Dive::CivilDate{2024,2,29}&&
            Diver->Simulation.Settings.TorchModel==2&&
            Diver->Simulation.Settings.StartPressureBar==213.5&&Diver->Simulation.Settings.WeightKg==7.5&&
            Diver->Simulation.Settings.PlannedDepthMetres==22.5&&Diver->Simulation.Settings.PlannedMinutes==47&&
            Diver->Simulation.Settings.AirTemperatureC==-3.5&&Diver->Simulation.Settings.CurrentMetresPerSecond==.17&&
            Diver->Simulation.Settings.Weather==3&&Diver->Simulation.Settings.WindMetresPerSecond==21.5;
        Diver->FinishDive(TEXT("smoke_config_completed")); return locked;
    }
    return false;
}
