#pragma once
#include "CoreMinimal.h"
#include "Simulation/DiveModel.h"
class FJsonObject;
namespace DiveConfigIO
{
    void Write(const TSharedPtr<FJsonObject>& Object,const Dive::Config& Config,int32 Entry);
    bool Read(const TSharedPtr<FJsonObject>& Object,Dive::Config& Config,int32& Entry,bool Strict,FString& Error);
    bool Equal(const Dive::Config& A,int32 EntryA,const Dive::Config& B,int32 EntryB);
}
