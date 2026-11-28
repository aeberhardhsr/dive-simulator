#include "DiveTerrain.h"
#include "Simulation/ShoreTransition.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FDiveTerrain::Load()
{
    LandscapeDetail=Dive::SurfaceGrid{};
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
        if(result.Name==TEXT("Murg / Sued")) result.Name=TEXT("Murg / Süd");
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
    return LoadLandscape()&&LoadShoreline()&&LoadCoast()&&LoadImagery();
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

bool FDiveTerrain::SampleMeasured(double X,double Y,float& Z) const
{
    double height;
    if(!Mosaic.Sample(OriginEast+Y/100,OriginNorth+X/100,height)) return false;
    Z=static_cast<float>(height*100); return true;
}

bool FDiveTerrain::LoadShoreline()
{
    Shores.Reset(); ShoreEntries.Reset();
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata");
    FString text; TSharedPtr<FJsonObject> manifest;
    const TArray<TSharedPtr<FJsonValue>>* sites=nullptr; double schema=0,water=0;
    if(!FFileHelper::LoadFileToString(text,*(folder/TEXT("shoreline.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),manifest)||!manifest||
       !manifest->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
       !manifest->TryGetNumberField(TEXT("water_elevation_m"),water)||water!=419||
       !manifest->TryGetArrayField(TEXT("sites"),sites)||sites->Num()!=3)
    { Error=TEXT("Uferdaten fehlen oder sind ungueltig: Tools/prepare_shoreline.py."); return false; }
    for(const auto& value:*sites)
    {
        const auto obj=value->AsObject(); Dive::SurfaceGrid grid; FDiveEntry entry;
        double south=0,rows=0,cols=0,east=0,north=0,yaw=0; FString file;
        if(!obj||!obj->TryGetStringField(TEXT("file"),file)||FPaths::GetCleanFilename(file)!=file||
           !obj->TryGetStringField(TEXT("name"),entry.Name)||!obj->TryGetStringField(TEXT("id"),entry.Id)||
           !obj->TryGetNumberField(TEXT("west"),grid.West)||!obj->TryGetNumberField(TEXT("south"),south)||
           !obj->TryGetNumberField(TEXT("rows"),rows)||rows!=384||
           !obj->TryGetNumberField(TEXT("columns"),cols)||cols!=384||
           !obj->TryGetNumberField(TEXT("spacing_m"),grid.Spacing)||grid.Spacing!=1||
           !obj->TryGetNumberField(TEXT("start_east"),east)||!obj->TryGetNumberField(TEXT("start_north"),north)||
           !obj->TryGetNumberField(TEXT("yaw"),yaw)||!FMath::IsFinite(grid.West)||!FMath::IsFinite(south)||
           !FMath::IsFinite(east)||!FMath::IsFinite(north)||!FMath::IsFinite(yaw))
        { Error=TEXT("Ungueltige Uferkachel."); return false; }
        grid.Rows=384; grid.Columns=384; grid.North=south+384;
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(folder/file))||bytes.Num()!=384*384*4)
        { Error=TEXT("Uferkachel unvollstaendig."); return false; }
        grid.Heights.resize(384*384); FMemory::Memcpy(grid.Heights.data(),bytes.GetData(),bytes.Num());
        for(float z:grid.Heights) if(!FMath::IsFinite(z)||z<-300||z>4000)
        { Error=TEXT("Ungueltige Uferhoehe."); return false; }
        double ground=0;
        if(!grid.Sample(east,north,ground)||ground<.3)
        { Error=TEXT("Uferstart liegt nicht auf trockenem Boden."); return false; }
        entry.Position=ToWorld(east,north); entry.Yaw=static_cast<float>(yaw);
        Shores.Add(MoveTemp(grid));
        float meshGround=0; SampleGround(entry.Position.X,entry.Position.Y,meshGround);
        entry.Position.Z=meshGround+170; ShoreEntries.Add(entry);
    }
    UE_LOG(LogTemp,Display,TEXT("WALENSEE_SHORE_READY patches=%d resolution=1m modeled_gap=labelled"),Shores.Num());
    return true;
}
bool FDiveTerrain::SampleShore(double X,double Y,float& Z) const
{
    for(const auto& grid:Shores)
    {
        const double east=OriginEast+Y/100,north=OriginNorth+X/100;
        const Dive::ShoreTransition transition(grid,Landscape);
        if(!transition.Contains(east,north)) continue;
        // The surveyed lakebed always wins, including in the outer LOD collar.
        if(Sample(X,Y,Z)) return true;
        double height=0;
        if(transition.Sample(grid,Landscape,east,north,height)) { Z=static_cast<float>(height*100); return true; }
    }
    return false;
}
bool FDiveTerrain::InShorePatch(double X,double Y,double MarginMetres) const
{
    const double east=OriginEast+Y/100,north=OriginNorth+X/100;
    for(const auto& grid:Shores)
        if(Dive::ShoreTransition(grid,Landscape).Contains(east,north,MarginMetres)) return true;
    return false;
}
bool FDiveTerrain::SampleGround(double X,double Y,float& Z) const
{
    if(!LandscapeDetail.Heights.empty())
    {
        const double east=OriginEast+Y/100,north=OriginNorth+X/100;
        double height=0;
        // Sample the actual 2m triangle, not a second, differently oriented
        // 1m triangulation. Outside the tile retain the existing shore/coast
        // collision; only otherwise unsupported inland ground uses 8m faces.
        if(LandscapeDetail.SampleTriangle(east,north,height)||
           (!InCoast(X,Y)&&!InShorePatch(X,Y)&&!Sample(X,Y,Z)&&Landscape.SampleTriangle(east,north,height)))
        { Z=static_cast<float>(height*100);return true; }
    }
    // Collision uses planar triangles on an integer-metre vertex lattice.
    // Interpolate those planes, rather than a bilinear saddle between vertices.
    const double x=FMath::FloorToDouble(X/100)*100,y=FMath::FloorToDouble(Y/100)*100;
    float a=0,b=0,c=0,d=0;
    auto vertex=[this](double px,double py,float& height){return SampleVertex(px,py,height);};
    if(!vertex(x,y,a)||!vertex(x+100,y,b)||!vertex(x,y+100,c)||!vertex(x+100,y+100,d)) return false;
    const double tx=(X-x)/100,ty=(Y-y)/100;
    Z=static_cast<float>(tx+ty<=1?a+(b-a)*tx+(c-a)*ty:d+(c-d)*(1-tx)+(b-d)*(1-ty));
    return true;
}
