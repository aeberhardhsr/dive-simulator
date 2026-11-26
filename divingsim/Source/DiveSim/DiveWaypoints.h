#pragma once
#include "CoreMinimal.h"

struct FDiveWaypoint
{
    FString Id,Name;
    double East=0,North=0,Depth=0;
    bool FromMap=false;
};

// LV95 metres and depth below the mean water level; independent of world origin.
class FDiveWaypoints
{
public:
    static constexpr int Limit=32;
    TArray<FDiveWaypoint> Items;
    FString Target,Error,Path;
    bool Loaded=false;
    bool Load(const FString& File);
    bool Add(const FString& Name,double East,double North,double Depth,bool FromMap,FString& Id);
    bool Rename(const FString& Id,const FString& Name);
    bool Remove(const FString& Id);
    bool Select(const FString& Id);
    const FDiveWaypoint* Find(const FString& Id) const;
    static bool ValidName(const FString& Name);
    static bool Decode(const FString& Json,TArray<FDiveWaypoint>& Items,FString& Target);
    static FString Encode(const TArray<FDiveWaypoint>& Items,const FString& Target);
private:
    FString Stored;
    bool Save(const TArray<FDiveWaypoint>& Before,const FString& BeforeTarget);
};
