#pragma once
#include "CoreMinimal.h"
#include "Simulation/DiveModel.h"

struct FDiveProfileState
{
    FString Name,UpdatedUTC;
    Dive::Config Config;
    int32 Entry=0;
};
struct FDiveProfile : FDiveProfileState
{
    FString Id;
    bool Archived=false;
    TOptional<FDiveProfileState> Previous;
};
enum class EProfileError { None,Read,Invalid,Write,Conflict,Name,Duplicate,Limit,Missing,NoPrevious };

class FDiveProfiles
{
public:
    static constexpr int MaxActive=32,MaxTotal=128;
    TArray<FDiveProfile> Items;
    FString Path,Detail;
    EProfileError Error=EProfileError::None;
    bool bLoaded=false;
    bool Load(const FString& File);
    bool Add(const FString& Name,const Dive::Config& Config,int32 Entry,FString& CreatedId);
    bool Replace(const FString& Id,const Dive::Config& Config,int32 Entry);
    bool Rename(const FString& Id,const FString& Name);
    bool Archive(const FString& Id,bool Archived);
    bool Undo(const FString& Id);
    const FDiveProfile* Find(const FString& Id) const;
    static bool ValidName(const FString& Name);
    static bool Decode(const FString& Json,TArray<FDiveProfile>& Items,FString& Error);
    static FString Encode(const TArray<FDiveProfile>& Items);
private:
    FString StoredJson;
    bool Save();
    bool Commit(const TArray<FDiveProfile>& Before);
    bool NameAvailable(const FString& Name,const FString& ExceptId=FString());
    FDiveProfile* Mutable(const FString& Id);
};
