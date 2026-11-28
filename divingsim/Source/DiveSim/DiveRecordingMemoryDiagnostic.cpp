#include "DiveRecording.h"

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS && PLATFORM_WINDOWS
#include "Async/Async.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

// Opt-in diagnosis, deliberately outside the normal DiveSim acceptance suite.
// This accelerated, NullRHI test cannot establish a rendered-game memory gate.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDiveRecordingMemoryDiagnostic,
    "DiveDiagnostics.RecordingMemory",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FDiveRecordingMemoryDiagnostic::RunTest(const FString&)
{
    FString mode,output;
    if(!FParse::Value(FCommandLine::Get(),TEXT("DiveRecordingMemory="),mode)||
       (mode!=TEXT("control")&&mode!=TEXT("writer"))||
       !FParse::Value(FCommandLine::Get(),TEXT("DiveRecordingMemoryOutput="),output))
    {
        AddError(TEXT("Requires -DiveRecordingMemory=control|writer and a fresh -DiveRecordingMemoryOutput directory."));
        return false;
    }
    if(IFileManager::Get().DirectoryExists(*output))
    { AddError(TEXT("Diagnostic output already exists; refusing to replace evidence."));return false; }
    if(!IFileManager::Get().MakeDirectory(*output,true)) return false;
    FDiveRecording recording;
    recording.Path=output/TEXT("session.json");
    recording.StartedUTC=FDateTime::UtcNow().ToIso8601();
    recording.Location=TEXT("Recording memory diagnostic");
    recording.Status=TEXT("in_progress"); recording.bTest=true;
    recording.SurveyOrigin=FVector2D(2726000,1219000);
    Dive::Model model;model.Step(.02,5,0,0,0,0,false);
    FDiveRecordingWriter writer;
    const double started=FPlatformTime::Seconds();
    FString csv=TEXT("phase,iteration,samples,sample_bytes,elapsed_seconds,commit_bytes,working_set_bytes,write_ms\n");
    auto measure=[&](const TCHAR* phase,int32 iteration,double milliseconds)
    {
        // WindowsPlatformMemory maps UsedVirtual to PROCESS_MEMORY_COUNTERS.PagefileUsage:
        // process commit charge, NOT the reserved virtual address space.
        const auto memory=FPlatformMemory::GetStats();
        csv+=FString::Printf(TEXT("%s,%d,%d,%llu,%.6f,%llu,%llu,%.6f\n"),phase,iteration,
            recording.Samples.Num(),static_cast<uint64>(recording.Samples.GetAllocatedSize()),
            FPlatformTime::Seconds()-started,static_cast<uint64>(memory.UsedVirtual),
            static_cast<uint64>(memory.UsedPhysical),milliseconds);
        if(iteration%30==0) UE_LOG(LogTemp,Display,TEXT("DIVE_RECORDING_MEMORY mode=%s phase=%s iteration=%d samples=%d commit_mib=%.3f"),
            *mode,phase,iteration,recording.Samples.Num(),memory.UsedVirtual/1048576.);
    };
    measure(TEXT("initial"),0,0);
    for(int32 iteration=1;iteration<=360;++iteration)
    {
        const TCHAR* phase=iteration<=240?TEXT("grow"):iteration<=300?TEXT("hold"):TEXT("reset");
        const int32 count=iteration<=240?iteration*30+1:iteration<=300?7201:31;
        if(iteration==301) recording.Samples.Empty();
        while(recording.Samples.Num()<count)
        {
            const int32 index=recording.Samples.Num();
            auto state=model.Data;state.ElapsedSeconds=index;state.DepthMetres=5;state.MaxDepthMetres=5;
            state.DistanceMetres=index;recording.Samples.Add({state,FVector(index*100.,index*50.,-500.)});
        }
        recording.State=recording.Samples.Last().State;
        double milliseconds=0;
        if(mode==TEXT("writer"))
        {
            writer.Submit(recording);
            TOptional<FDiveWriteResult> result;
            const double deadline=FPlatformTime::Seconds()+30;
            while(!result.IsSet()&&FPlatformTime::Seconds()<deadline)
            { result=writer.Poll();if(!result.IsSet()) FPlatformProcess::Sleep(.001f); }
            if(!result.IsSet()||!result->bSuccess)
            {
                FFileHelper::SaveStringToFile(csv,*(output/TEXT("Memory.partial.csv")));
                AddError(result.IsSet()?result->Error:TEXT("Recording worker timeout"));return false;
            }
            milliseconds=result->Milliseconds;
        }
        else
        {
            // Keep the same snapshot copy + thread-pool lifetime, omit JSON and disk IO.
            FDiveRecording snapshot=recording;
            auto active=Async(EAsyncExecution::ThreadPool,[snapshot=MoveTemp(snapshot)](){return snapshot.Samples.Num();});
            TestEqual(TEXT("Control snapshot retains samples"),active.Get(),count);
        }
        measure(phase,iteration,milliseconds);
    }
    recording.Samples.Empty();measure(TEXT("released"),361,0);
    const bool saved=FFileHelper::SaveStringToFile(csv,*(output/TEXT("Memory.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    TestTrue(TEXT("Diagnostic measurements saved"),saved);
    UE_LOG(LogTemp,Display,TEXT("DIVE_RECORDING_MEMORY_DONE mode=%s sample_size=%llu output=%s"),
        *mode,static_cast<uint64>(sizeof(FDiveSample)),*output);
    return saved;
}
#endif
