#include "DiveRecording.h"
#include "DiveLogbook.h"
#include "Async/Async.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace
{
void SaveEnvironment(const TSharedPtr<FJsonObject>& Obj,const Dive::Config& Config)
{
    Obj->SetNumberField(TEXT("weather"),Config.Weather);
    Obj->SetNumberField(TEXT("wind_mps"),Config.WindMetresPerSecond);
    Obj->SetNumberField(TEXT("wind_direction_degrees"),Config.WindDirectionDegrees);
    Obj->SetNumberField(TEXT("surface_temperature_c"),Config.SurfaceTemperatureC);
    Obj->SetNumberField(TEXT("air_temperature_c"),Config.AirTemperatureC);
    Obj->SetNumberField(TEXT("bottom_temperature_c"),Config.BottomTemperatureC);
    Obj->SetNumberField(TEXT("thermocline_m"),Config.ThermoclineMetres);
    Obj->SetNumberField(TEXT("thermocline_width_m"),Config.ThermoclineWidthMetres);
    Obj->SetNumberField(TEXT("visibility_m"),Config.VisibilityMetres);
    Obj->SetNumberField(TEXT("current_mps"),Config.CurrentMetresPerSecond);
    Obj->SetNumberField(TEXT("hour"),Config.Hour);
    Obj->SetStringField(TEXT("scenario_date"),UTF8_TO_TCHAR(Config.Date.Text().c_str()));
    Obj->SetStringField(TEXT("scenario_timezone"),TEXT("Europe/Zurich"));
    const auto clock=Dive::ScenarioClock::At(Config.Date,Config.Hour,0);
    Obj->SetNumberField(TEXT("scenario_start_utc_seconds"),clock.UTCSeconds);
    Obj->SetNumberField(TEXT("scenario_utc_offset_minutes"),clock.OffsetMinutes);
}
}

namespace DiveLogbook
{
FDiveWriteResult WriteRecording(const FDiveRecording& Recording)
{
    FDiveWriteResult result;
    const double started=FPlatformTime::Seconds();
    auto root=MakeShared<FJsonObject>();
    root->SetNumberField(TEXT("schema_version"),4);
    root->SetStringField(TEXT("physiology_model"),TEXT("breath_reserve_v1"));
    root->SetStringField(TEXT("started_at_utc"),Recording.StartedUTC);
    root->SetBoolField(TEXT("is_test"),Recording.bTest);
    root->SetStringField(TEXT("status"),Recording.Status);
    root->SetStringField(TEXT("location"),Recording.Location);
    root->SetBoolField(TEXT("start_on_shore"),Recording.Config.StartOnShore);
    root->SetStringField(TEXT("bathymetry"),TEXT("swissBATHY3D Walensee 2020; (c) swisstopo"));
    root->SetNumberField(TEXT("water_elevation_ln02_m"),419);
    root->SetNumberField(TEXT("duration_seconds"),Recording.State.ElapsedSeconds);
    root->SetNumberField(TEXT("maximum_depth_m"),Recording.State.MaxDepthMetres);
    root->SetNumberField(TEXT("distance_m"),Recording.State.DistanceMetres);
    root->SetNumberField(TEXT("cylinder_litres"),Recording.Config.CylinderLitres);
    root->SetNumberField(TEXT("oxygen_fraction"),Recording.Config.OxygenFraction);
    root->SetNumberField(TEXT("mode"),static_cast<int>(Recording.Config.GameMode));
    const auto& torch=Dive::Torch::At(Recording.Config.TorchModel);
    auto lamp=MakeShared<FJsonObject>();
    lamp->SetStringField(TEXT("model"),UTF8_TO_TCHAR(torch.Id));
    lamp->SetNumberField(TEXT("nominal_lumens"),torch.Lumens);
    lamp->SetNumberField(TEXT("full_beam_degrees"),2*torch.OuterDegrees);
    lamp->SetNumberField(TEXT("runtime_minutes"),torch.RuntimeMinutes);
    lamp->SetNumberField(TEXT("remaining_percent"),Recording.State.LampBattery);
    root->SetObjectField(TEXT("torch"),lamp);
    auto environment=MakeShared<FJsonObject>(); SaveEnvironment(environment,Recording.Config);
    root->SetObjectField(TEXT("environment"),environment);
    TArray<TSharedPtr<FJsonValue>> rows;
    for(const auto& sample:Recording.Samples)
    {
        const auto& s=sample.State; auto row=MakeShared<FJsonObject>();
        row->SetNumberField(TEXT("seconds"),s.ElapsedSeconds);
        row->SetNumberField(TEXT("depth_m"),s.DepthMetres);
        row->SetNumberField(TEXT("cylinder_bar"),s.CylinderBar);
        row->SetNumberField(TEXT("heart_bpm"),s.HeartBPM);
        row->SetNumberField(TEXT("respiratory_bpm"),s.RespiratoryBPM);
        row->SetNumberField(TEXT("spo2_percent"),s.SpO2);
        row->SetNumberField(TEXT("breath_reserve"),s.BreathReserve);
        row->SetNumberField(TEXT("water_c"),s.WaterTemperatureC);
        row->SetNumberField(TEXT("environment_c"),s.EnvironmentTemperatureC);
        row->SetNumberField(TEXT("body_immersion"),s.BodyImmersion);
        row->SetNumberField(TEXT("breathing_source"),static_cast<int>(s.BreathSource));
        row->SetBoolField(TEXT("regulator_in_mouth"),s.RegulatorInMouth);
        row->SetBoolField(TEXT("mouth_submerged"),s.MouthSubmerged);
        row->SetNumberField(TEXT("core_c"),s.CoreTemperatureC);
        row->SetNumberField(TEXT("stress"),s.Stress);
        row->SetNumberField(TEXT("fatigue"),s.Fatigue);
        row->SetNumberField(TEXT("ppo2_bar"),s.PPO2);
        row->SetNumberField(TEXT("ceiling_m"),s.CeilingMetres);
        row->SetNumberField(TEXT("ndl_minutes"),s.NDLMinutes);
        row->SetNumberField(TEXT("distance_m"),s.DistanceMetres);
        row->SetNumberField(TEXT("warnings"),s.Warnings);
        row->SetNumberField(TEXT("lamp_battery_percent"),s.LampBattery);
        row->SetNumberField(TEXT("x_m"),sample.Position.X/100);
        row->SetNumberField(TEXT("y_m"),sample.Position.Y/100);
        const FVector2D survey=FVector2D(Recording.SurveyOrigin.X+sample.Position.Y/100,Recording.SurveyOrigin.Y+sample.Position.X/100);
        row->SetNumberField(TEXT("lv95_east_m"),survey.X);
        row->SetNumberField(TEXT("lv95_north_m"),survey.Y);
        rows.Add(MakeShared<FJsonValueObject>(row));
    }
    root->SetArrayField(TEXT("samples"),rows);
    FString json; auto writer=TJsonWriterFactory<>::Create(&json);
    if(!FJsonSerializer::Serialize(root,writer)) { result.Error=TEXT("Cannot serialize log"); return result; }
    result.bSuccess=Write(Recording.Path,json,result.Error);
    result.Milliseconds=(FPlatformTime::Seconds()-started)*1000;
    if(!result.bSuccess) { UE_LOG(LogTemp,Warning,TEXT("DIVE_LOG_SAVE_FAILED %s: %s"),*Recording.Path,*result.Error); }
    else if(Recording.bTest) { UE_LOG(LogTemp,Display,TEXT("DIVE_LOG_SAVED status=%s seconds=%.3f samples=%d path=%s"),
        *Recording.Status,Recording.State.ElapsedSeconds,Recording.Samples.Num(),*Recording.Path);
        UE_LOG(LogTemp,Display,TEXT("DIVE_LOG_WORK samples=%d milliseconds=%.3f"),Recording.Samples.Num(),result.Milliseconds); }
    return result;
}
}

FDiveRecordingWriter::~FDiveRecordingWriter()
{
    if(Active.IsValid()) Active.Wait();
}
void FDiveRecordingWriter::StartPending()
{
    if(Active.IsValid()||!Pending.IsSet()) return;
    FDiveRecording recording=MoveTemp(Pending.GetValue()); Pending.Reset();
    Active=Async(EAsyncExecution::ThreadPool,[recording=MoveTemp(recording)]()
    { return DiveLogbook::WriteRecording(recording); });
}
void FDiveRecordingWriter::Submit(FDiveRecording Recording)
{
    Pending.Emplace(MoveTemp(Recording)); StartPending();
}
TOptional<FDiveWriteResult> FDiveRecordingWriter::Poll()
{
    TOptional<FDiveWriteResult> result;
    if(Active.IsValid()&&Active.IsReady())
    { result=Active.Get(); Active=TFuture<FDiveWriteResult>(); }
    StartPending();
    return result;
}
FDiveWriteResult FDiveRecordingWriter::Finish(FDiveRecording Recording)
{
    // Final snapshot contains the latest samples, so queued autosaves are obsolete.
    Pending.Reset();
    if(Active.IsValid()) { Active.Wait(); Active.Get(); Active=TFuture<FDiveWriteResult>(); }
    return DiveLogbook::WriteRecording(Recording);
}
