#pragma once
#include "CoreMinimal.h"
#include "Simulation/DiveModel.h"

// Plain data only: disk/JSON work can run without touching UObjects.
struct FDiveLog
{
    FString Path, Date, Location, Status, Error;
    double Duration=0, MaxDepth=0, Distance=0, CylinderLitres=0;
    int32 Mode=1;
    bool bTest=false;
    bool bBreathingContext=false;
    bool bBreathReserveContext=false;
    bool bRecovered=false;
    TArray<Dive::State> Samples;
    uint32 Warnings=0;
    int32 WarningEvents=0;
};

struct FDiveLogIndex
{
    TArray<FDiveLog> Entries;
    int32 Unreadable=0;
};

namespace DiveLogbook
{
    bool Parse(const FString& Json, FDiveLog& Log);
    // Preserve a validated previous generation before replacing the primary file.
    bool Write(const FString& Path,const FString& Json,FString& Error);
    FDiveLog Read(const FString& Path);
    FDiveLogIndex Scan(const FString& Directory, bool IncludeTests=false);
    int32 NearestSample(const TArray<Dive::State>& Samples,double Seconds);
}
