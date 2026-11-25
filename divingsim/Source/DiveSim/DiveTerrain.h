#pragma once
#include "CoreMinimal.h"
#include "Simulation/HeightGrid.h"

struct FDiveEntry { FString Name; FVector Position; float Yaw=0; };

class FDiveTerrain
{
public:
    bool Load();
    bool Sample(double X,double Y,float& Z) const;
    FVector ToWorld(double East,double North) const { return FVector((North-OriginNorth)*100,(East-OriginEast)*100,-180); }
    FVector2D ToSurvey(const FVector& Position) const { return FVector2D(OriginEast+Position.Y/100,OriginNorth+Position.X/100); }
    TArray<FDiveEntry> Entries;
    FString Error;
    int32 LoadedTiles=0;
    // Fixed-size overview, measured depths only. East to right, north up.
    static constexpr int MapWidth=512, MapHeight=128;
    TArray<float> Overview;
    double OriginEast=2726000,OriginNorth=1219000;
    double MapWest=2726000, MapSouth=1219000, MapEast=2742000, MapNorth=1223000;
private:
    Dive::HeightMosaic Mosaic;
};
