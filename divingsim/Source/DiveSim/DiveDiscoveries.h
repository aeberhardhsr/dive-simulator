#pragma once
#include "CoreMinimal.h"
class FJsonObject;

struct FDivePhotograph
{
    FString Id,UTC;
    double Seconds=0,Depth=0;
    TArray<FString> Species;
};

struct FDiveSpecies
{
    FString Id,NameDE,NameEN,Scientific,DescriptionDE,DescriptionEN,Source;
};

namespace DiveDiscoveries
{
    constexpr int PhotoLimit=1000;
    // Stable asset identities; taxonomy labels and descriptions live in the catalog.
    const TCHAR* SpeciesId(int Index);
    int SpeciesIndex(const FString& Id);
    bool ValidPhotoId(const FString& Id);
    FString PhotoPath(const FString& Id,bool Test=false);
    bool DecodePhotos(const TSharedPtr<FJsonObject>& Root,TArray<FDivePhotograph>& Out,double Duration);
    void EncodePhotos(const TSharedPtr<FJsonObject>& Root,const TArray<FDivePhotograph>& Photos);
    bool LoadCatalog(TArray<FDiveSpecies>& Out);
    // Authored camera eligibility, not image recognition. Distances in metres.
    bool InFrame(const FVector& Offset,const FVector& Forward,double Radius,double Visibility);
}
