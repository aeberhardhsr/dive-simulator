#pragma once
#include "CoreMinimal.h"
#include "DiveDiscoveries.h"
#include "Async/Future.h"
#include "Simulation/DiveModel.h"

struct FDiveSample
{
    Dive::State State;
    FVector Position;
};

// Owned values only. No actor, world, material or shared mutable JSON objects.
struct FDiveRecording
{
    FString Path,StartedUTC,Status,Location;
    bool bTest=false;
    Dive::Config Config;
    Dive::State State;
    FVector2D SurveyOrigin=FVector2D::ZeroVector;
    TArray<FDivePhotograph> Photographs;
    TArray<FDiveSample> Samples;
};

struct FDiveWriteResult
{
    bool bSuccess=false;
    FString Error;
    double Milliseconds=0;
};

namespace DiveLogbook
{
    FDiveWriteResult WriteRecording(const FDiveRecording& Recording);
}

// All methods run on the owning game thread. At most one worker and one pending
// snapshot; slow storage coalesces pending autosaves without growing a job queue.
class FDiveRecordingWriter
{
public:
    ~FDiveRecordingWriter();
    void Submit(FDiveRecording Recording);
    TOptional<FDiveWriteResult> Poll();
    FDiveWriteResult Finish(FDiveRecording Recording);
    int32 PendingCount() const { return Pending.IsSet()?1:0; }
private:
    void StartPending();
    TFuture<FDiveWriteResult> Active;
    TOptional<FDiveRecording> Pending;
};
