#pragma once
#include "CoreMinimal.h"
#include "Simulation/HeightGrid.h"
#include "Simulation/CoastGrid.h"

struct FDiveEntry { FString Name,Id; FVector Position; float Yaw=0; };

class FDiveTerrain
{
public:
    bool Load();
    bool LoadLandscape();
    bool LoadShoreline();
    bool LoadCoast();
    bool SampleCoast(double X,double Y,float& Z) const;
    bool SampleVertex(double X,double Y,float& Z) const;
    FVector SurfaceNormal(double X,double Y) const;
    bool InCoast(double X,double Y) const;
    double CoastWeight(double East,double North) const;
    bool LoadImagery();
    bool Sample(double X,double Y,float& Z) const;
    bool SampleShore(double X,double Y,float& Z) const;
    bool SampleGround(double X,double Y,float& Z) const;
    bool InShorePatch(double X,double Y,double MarginMetres=0) const;
    FVector ToWorld(double East,double North) const { return FVector((North-OriginNorth)*100,(East-OriginEast)*100,-180); }
    FVector2D ToSurvey(const FVector& Position) const { return FVector2D(OriginEast+Position.Y/100,OriginNorth+Position.X/100); }
    TArray<FDiveEntry> Entries;
    TArray<FDiveEntry> ShoreEntries;
    const TArray<FDiveEntry>& EntryList(bool Shore) const { return Shore?ShoreEntries:Entries; }
    TArray<Dive::SurfaceGrid> Shores;
    Dive::CoastGrid Coast;
    FString CoastSourceHash;
    FString Error;
    int32 LoadedTiles=0;
    // Fixed-size overview, measured depths only. East to right, north up.
    static constexpr int MapWidth=512, MapHeight=128;
    TArray<float> Overview;
    Dive::SurfaceGrid Landscape;
    FString LandscapeImagery;
    TArray<FString> ShoreImagery;
    double OriginEast=2726000,OriginNorth=1219000;
    double MapWest=2726000, MapSouth=1219000, MapEast=2742000, MapNorth=1223000;
private:
    Dive::HeightMosaic Mosaic;
};
