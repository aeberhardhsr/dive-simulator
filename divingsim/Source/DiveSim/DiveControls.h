#pragma once
#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class FJsonObject;
enum class EDiveAction : uint8
{
    Forward,Backward,Left,Right,Ascend,Descend,Sprint,Inflate,Deflate,
    Lamp,HUD,Computer,ComputerPage,Map,Finish,Photo,Regulator,Count
};

struct FDiveControls
{
    static constexpr int Count=static_cast<int>(EDiveAction::Count);
    TArray<FKey> Keys;
    double Sensitivity=.12,FieldOfView=90;
    bool InvertY=false;
    FDiveControls();
    FKey Key(EDiveAction Action) const { return Keys[static_cast<int>(Action)]; }
    int Find(FKey Key) const { return Keys.IndexOfByKey(Key); }
    // Returns swapped action, INDEX_NONE if none, or -2 for an unsupported key.
    int Assign(int Action,FKey Key);
    static bool Allowed(FKey Key);
    static FString Id(int Action);
    static FString Label(int Action,bool English);
    FString KeyText(EDiveAction Action,bool English) const;
    void Sanitize();
    void Write(const TSharedPtr<FJsonObject>& Root) const;
    void Read(const TSharedPtr<FJsonObject>& Root);
};
