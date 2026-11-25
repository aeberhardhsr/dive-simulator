#include "DiveTerrain.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FDiveTerrain::Load()
{
    Mosaic.Tiles.clear(); Entries.Reset(); Overview.Reset(); LoadedTiles=0; Error.Empty();
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata");
    FString text;
    TSharedPtr<FJsonObject> manifest;
    if(!FFileHelper::LoadFileToString(text,*(folder/TEXT("manifest.json"))) ||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),manifest) || !manifest)
    { Error=TEXT("Walensee-Geodaten fehlen. Tools/package_bathymetry.py ausfuehren."); return false; }
    double schema=0; const TArray<TSharedPtr<FJsonValue>>* tiles=nullptr;
    if(!manifest->TryGetNumberField(TEXT("schema"),schema)||schema!=2||!manifest->TryGetArrayField(TEXT("tiles"),tiles))
    { Error=TEXT("Unbekanntes Geodatenformat."); return false; }
    double water=0;
    if(!manifest->TryGetNumberField(TEXT("origin_east"),OriginEast)||
       !manifest->TryGetNumberField(TEXT("origin_north"),OriginNorth)||
       !manifest->TryGetNumberField(TEXT("water_elevation_m"),water)||water!=419||
       !FMath::IsFinite(OriginEast)||!FMath::IsFinite(OriginNorth))
    { Error=TEXT("Ungueltiger Koordinaten- oder Wasserhoehenbezug."); return false; }
    for(const auto& value:*tiles)
    {
        const auto tile=value->AsObject();
        if(!tile) continue;
        Dive::HeightTile grid;
        FString file; double cols=0,rows=0,spacing=0;
        if(!tile->TryGetNumberField(TEXT("ncols"),cols)||!tile->TryGetNumberField(TEXT("nrows"),rows)||
           !tile->TryGetNumberField(TEXT("xllcorner"),grid.East)||!tile->TryGetNumberField(TEXT("yllcorner"),grid.North)||
           !tile->TryGetNumberField(TEXT("cellsize"),spacing)||!tile->TryGetStringField(TEXT("file"),file)||
           spacing!=1 || !FMath::IsFinite(cols)||!FMath::IsFinite(rows)||
           !FMath::IsFinite(grid.East)||!FMath::IsFinite(grid.North)||
           cols<1 || rows<1 || cols>1000 || rows>1000 || cols!=FMath::FloorToDouble(cols)||rows!=FMath::FloorToDouble(rows)||
           FPaths::GetCleanFilename(file)!=file)
        { Error=TEXT("Ungueltige Rasterkachel."); return false; }
        grid.Columns=static_cast<int>(cols); grid.Rows=static_cast<int>(rows);
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(folder/file))||bytes.Num()!=grid.Columns*grid.Rows*2)
        { Error=TEXT("Rasterkachel fehlt oder ist unvollstaendig: ")+file; return false; }
        grid.Centimetres.reserve(grid.Columns*grid.Rows);
        for(int i=0;i<bytes.Num();i+=2)
        {
            const uint16 encoded=static_cast<uint16>(bytes[i])|(static_cast<uint16>(bytes[i+1])<<8);
            grid.Centimetres.push_back(static_cast<int16_t>(encoded));
        }
        Mosaic.Tiles.push_back(MoveTemp(grid)); ++LoadedTiles;
    }
    const TArray<TSharedPtr<FJsonValue>>* entries=nullptr;
    if(!manifest->TryGetArrayField(TEXT("entries"),entries)) return false;
    for(const auto& value:*entries)
    {
        const auto entry=value->AsObject(); if(!entry) continue;
        FDiveEntry result; double east=0,north=0;
        if(!entry->TryGetStringField(TEXT("name"),result.Name)||!entry->TryGetNumberField(TEXT("east"),east)||!entry->TryGetNumberField(TEXT("north"),north)) continue;
        result.Position=ToWorld(east,north);
        float z,n,s,e,w;
        if(!Sample(result.Position.X,result.Position.Y,z)||z>-300) continue;
        if(Sample(result.Position.X+500,result.Position.Y,n)&&Sample(result.Position.X-500,result.Position.Y,s)&&
           Sample(result.Position.X,result.Position.Y+500,e)&&Sample(result.Position.X,result.Position.Y-500,w))
            result.Yaw=FMath::RadiansToDegrees(FMath::Atan2(w-e,s-n));
        Entries.Add(result);
    }
    if(Entries.Num()!=3) { Error=TEXT("Einstiegspunkte konnten nicht validiert werden."); return false; }
    Overview.SetNum(MapWidth*MapHeight);
    for(int y=0;y<MapHeight;++y) for(int x=0;x<MapWidth;++x)
    {
        double height;
        const double east=MapWest+(x+.5)*(MapEast-MapWest)/MapWidth;
        const double north=MapNorth-(y+.5)*(MapNorth-MapSouth)/MapHeight;
        Overview[y*MapWidth+x]=Mosaic.Sample(east,north,height)?static_cast<float>(-height):-1.f;
    }
    UE_LOG(LogTemp,Display,TEXT("WALENSEE_DATA_READY tiles=%d entries=%d grid=1m water_reference=419m_LN02"),LoadedTiles,Entries.Num());
    return LoadLandscape();
}

bool FDiveTerrain::LoadLandscape()
{
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata");
    FString text; TSharedPtr<FJsonObject> manifest;
    if(!FFileHelper::LoadFileToString(text,*(folder/TEXT("landscape.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),manifest)||!manifest)
    { Error=TEXT("Landschaftsdaten fehlen: Tools/prepare_landscape.py ausfuehren."); return false; }
    double rows=0,cols=0,water=0,schema=0; FString file;
    if(!manifest->TryGetNumberField(TEXT("rows"),rows)||!manifest->TryGetNumberField(TEXT("columns"),cols)||
       !manifest->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
       !manifest->TryGetNumberField(TEXT("water_elevation_m"),water)||water!=419||
       !manifest->TryGetNumberField(TEXT("west"),Landscape.West)||
       !manifest->TryGetNumberField(TEXT("north"),Landscape.North)||
       !manifest->TryGetNumberField(TEXT("spacing_m"),Landscape.Spacing)||Landscape.Spacing!=8||
       !manifest->TryGetStringField(TEXT("file"),file)||FPaths::GetCleanFilename(file)!=file||
       !FMath::IsFinite(rows)||!FMath::IsFinite(cols)||rows<2||cols<2||rows>5000||cols>5000||
       rows!=FMath::FloorToDouble(rows)||cols!=FMath::FloorToDouble(cols)||
       !FMath::IsFinite(Landscape.West)||!FMath::IsFinite(Landscape.North))
    { Error=TEXT("Ungueltiges Landschaftsraster."); return false; }
    Landscape.Rows=static_cast<int>(rows); Landscape.Columns=static_cast<int>(cols);
    TArray<uint8> bytes;
    if(!FFileHelper::LoadFileToArray(bytes,*(folder/file))||bytes.Num()!=Landscape.Rows*Landscape.Columns*4)
    { Error=TEXT("Landschaftsraster unvollstaendig."); return false; }
    Landscape.Heights.resize(Landscape.Rows*Landscape.Columns);
    // Supported Windows/macOS targets use little-endian IEEE754 float32.
    FMemory::Memcpy(Landscape.Heights.data(),bytes.GetData(),bytes.Num());
    for(float height:Landscape.Heights) if(!FMath::IsFinite(height)||height<-100||height>5000)
    { Error=TEXT("Ungueltiger Landschaftshoehenwert."); return false; }
    UE_LOG(LogTemp,Display,TEXT("WALENSEE_LAND_READY rows=%d columns=%d spacing=8m"),Landscape.Rows,Landscape.Columns);
    return true;
}

bool FDiveTerrain::Sample(double X,double Y,float& Z) const
{
    double height;
    if(!Mosaic.Sample(OriginEast+Y/100,OriginNorth+X/100,height)) return false;
    Z=static_cast<float>(height*100); return true;
}
