#pragma once
#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

struct FDiveBuildingTile
{
    FIntPoint Cell;
    FVector Origin,Probe,Normal;
    FString File,Asset,MD5;
    int32 Triangles=0;
    FString Name() const { return FPaths::GetBaseFilename(Asset.Left(Asset.Find(TEXT(".")))); }
};

namespace DiveBuildings
{
inline FString Coordinate(int32 Value)
{ return FString::Printf(TEXT("%c%d"),Value<0?'M':'P',FMath::Abs(Value)); }

inline bool Read(TArray<FDiveBuildingTile>& Tiles,int32& Buildings,FString& Error)
{
    Tiles.Reset();Buildings=0;
    FString text;TSharedPtr<FJsonObject> root;const TArray<TSharedPtr<FJsonValue>>* entries=nullptr;
    double schema=0,cell=0,water=0,count=0,total=0;
    auto fail=[&](){Error=TEXT("Gebaeudedaten fehlen oder sind ungueltig: Tools/prepare_buildings.py.");return false;};
    if(!FFileHelper::LoadFileToString(text,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/buildings.json")))||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),root)||!root||
        !root->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
        !root->TryGetNumberField(TEXT("cell_metres"),cell)||cell!=512||
        !root->TryGetNumberField(TEXT("water_elevation_m"),water)||water!=419||
        !root->TryGetNumberField(TEXT("building_count"),count)||!FMath::IsFinite(count)||count<1||count>1000000||count!=FMath::FloorToDouble(count)||
        !root->TryGetNumberField(TEXT("triangle_count"),total)||!FMath::IsFinite(total)||total<1||total>100000000||
        !root->TryGetArrayField(TEXT("tiles"),entries)||entries->IsEmpty()||entries->Num()>4096) return fail();
    TSet<FIntPoint> seen;int64 actual=0;
    for(const auto& value:*entries)
    {
        const auto object=value->AsObject();const TArray<TSharedPtr<FJsonValue>>* xy=nullptr;
        FDiveBuildingTile tile;double x=0,y=0,n=0,east=0,north=0,wx=0,wy=0;
        if(!object||!object->TryGetArrayField(TEXT("cell"),xy)||xy->Num()!=2||
            !(*xy)[0]->TryGetNumber(x)||!(*xy)[1]->TryGetNumber(y)||
            !FMath::IsFinite(x)||!FMath::IsFinite(y)||FMath::Abs(x)>200||FMath::Abs(y)>200||
            x!=FMath::FloorToDouble(x)||y!=FMath::FloorToDouble(y)||
            !object->TryGetNumberField(TEXT("triangles"),n)||!FMath::IsFinite(n)||n<1||n>2000000||n!=FMath::FloorToDouble(n)||
            !object->TryGetNumberField(TEXT("origin_east"),east)||east!=2726000+x*cell||
            !object->TryGetNumberField(TEXT("origin_north"),north)||north!=1219000+y*cell||
            !object->TryGetNumberField(TEXT("origin_x_cm"),wx)||wx!=y*cell*100||
            !object->TryGetNumberField(TEXT("origin_y_cm"),wy)||wy!=x*cell*100||
            !object->TryGetStringField(TEXT("file"),tile.File)||!object->TryGetStringField(TEXT("asset"),tile.Asset)||
            !object->TryGetStringField(TEXT("md5"),tile.MD5)||tile.MD5.Len()!=32) return fail();
        tile.Cell=FIntPoint(static_cast<int32>(x),static_cast<int32>(y));tile.Triangles=static_cast<int32>(n);
        tile.Origin=FVector(wx,wy,0);
        auto vector=[&](const TCHAR* Key,FVector& Out)
        {
            const TArray<TSharedPtr<FJsonValue>>* array=nullptr;
            if(!object->TryGetArrayField(Key,array)||array->Num()!=3) return false;
            for(int axis=0;axis<3;++axis)
                if(!(*array)[axis]->TryGetNumber(Out[axis])||!FMath::IsFinite(Out[axis])) return false;
            return true;
        };
        if(!vector(TEXT("probe_local_cm"),tile.Probe)||!vector(TEXT("probe_normal"),tile.Normal)||
            FMath::Abs(tile.Normal.SizeSquared()-1)>.001||tile.Probe.GetAbsMax()>1000000) return fail();
        const FString name=TEXT("SM_Buildings_")+Coordinate(tile.Cell.X)+TEXT("_")+Coordinate(tile.Cell.Y);
        if(seen.Contains(tile.Cell)||tile.File!=FString::Printf(TEXT("buildings_%d_%d.bmesh"),tile.Cell.X,tile.Cell.Y)||
            tile.Asset!=TEXT("/Game/Diving/Buildings/")+name+TEXT(".")+name) return fail();
        seen.Add(tile.Cell);actual+=tile.Triangles;Tiles.Add(tile);
    }
    if(actual!=total) return fail();
    Buildings=static_cast<int32>(count);Error.Empty();return true;
}
}
