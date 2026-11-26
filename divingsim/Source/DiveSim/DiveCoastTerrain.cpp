#include "DiveTerrain.h"
#include "Simulation/ShoreTransition.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"

bool FDiveTerrain::LoadCoast()
{
    Coast.Tiles.clear();CoastSourceHash.Empty();
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata/Coast");
    FString text;TSharedPtr<FJsonObject> root;const TArray<TSharedPtr<FJsonValue>>* tiles=nullptr;
    auto fail=[this](){Error=TEXT("Ufermodell fehlt oder ist ungueltig: Tools/package_coast.py.");return false;};
    if(!FFileHelper::LoadFileToString(text,*(folder/TEXT("coast.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),root)||!root) return fail();
    double schema=0,water=0,spacing=0,core=0,halo=0;
    if(!root->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
       !root->TryGetNumberField(TEXT("water_elevation_m"),water)||water!=419||
       !root->TryGetNumberField(TEXT("spacing_m"),spacing)||spacing!=1||
       !root->TryGetNumberField(TEXT("tile_core_metres"),core)||core!=128||
       !root->TryGetNumberField(TEXT("halo_cells"),halo)||halo!=1||
       !root->TryGetArrayField(TEXT("tiles"),tiles)||tiles->Num()<1||tiles->Num()>2000) return fail();
    FMD5 digest;text.ReplaceInline(TEXT("\r\n"),TEXT("\n"));FTCHARToUTF8 utf8(*text);
    digest.Update(reinterpret_cast<const uint8*>(utf8.Get()),utf8.Length());
    for(const auto& value:*tiles)
    {
        const auto tile=value->AsObject();double west=0,south=0,cols=0,rows=0;FString file;
        if(!tile||!tile->TryGetNumberField(TEXT("core_west"),west)||!tile->TryGetNumberField(TEXT("core_south"),south)||
           !tile->TryGetNumberField(TEXT("columns"),cols)||cols!=130||!tile->TryGetNumberField(TEXT("rows"),rows)||rows!=130||
           !tile->TryGetStringField(TEXT("file"),file)||FPaths::GetCleanFilename(file)!=file||
           !FMath::IsFinite(west)||!FMath::IsFinite(south)||west<2720000||west>2750000||south<1210000||south>1230000||
           FMath::Fmod(west-4,128.)!=0||FMath::Fmod(south-4,128.)!=0) return fail();
        const auto key=Dive::CoastGrid::Key(west,south);
        if(Coast.Tiles.find(key)!=Coast.Tiles.end()) return fail();
        Dive::SurfaceGrid grid;grid.West=west-1;grid.North=south+129;grid.Rows=grid.Columns=130;grid.Spacing=1;
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(folder/file))||bytes.Num()!=130*130*4) return fail();
        digest.Update(bytes.GetData(),bytes.Num());grid.Heights.resize(130*130);
        FMemory::Memcpy(grid.Heights.data(),bytes.GetData(),bytes.Num());
        for(float h:grid.Heights) if(!FMath::IsFinite(h)||h<-300||h>5000) return fail();
        Coast.Tiles.emplace(key,MoveTemp(grid));
    }
    digest.Update(reinterpret_cast<const uint8*>(Landscape.Heights.data()),Landscape.Heights.size()*sizeof(float));
    for(const auto& shore:Shores) digest.Update(reinterpret_cast<const uint8*>(shore.Heights.data()),shore.Heights.size()*sizeof(float));
    TArray<FString> originals;IFileManager::Get().FindFiles(originals,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/*.i16")),true,false);originals.Sort();
    for(const auto& file:originals)
    {
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata")/file))) return fail();
        digest.Update(bytes.GetData(),bytes.Num());
    }
    uint8 hash[16];digest.Final(hash);CoastSourceHash=BytesToHex(hash,16);
    UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_DATA_READY tiles=%d source=%s"),static_cast<int>(Coast.Tiles.size()),*CoastSourceHash);
    return true;
}

bool FDiveTerrain::InCoast(double X,double Y) const
{ return Coast.Contains(OriginEast+Y/100,OriginNorth+X/100); }

double FDiveTerrain::CoastWeight(double East,double North) const
{
    double distance=Coast.BoundaryDistance(East,North);
    for(const auto& shore:Shores)
    {
        const Dive::ShoreTransition bounds(shore,Landscape);
        distance=FMath::Min(distance,Dive::CoastGrid::RectangleDistance(East,North,bounds.West,bounds.South,bounds.East,bounds.North));
    }
    return Dive::CoastGrid::Weight(distance);
}

bool FDiveTerrain::SampleCoast(double X,double Y,float& Z) const
{
    const double east=OriginEast+Y/100,north=OriginNorth+X/100;double fine=0,coarse=0;
    if(!Coast.Sample(east,north,fine)||!Landscape.SampleTriangle(east,north,coarse)) return false;
    const double weight=CoastWeight(east,north);
    Z=static_cast<float>((coarse+(fine-coarse)*weight)*100);return true;
}

bool FDiveTerrain::SampleVertex(double X,double Y,float& Z) const
{ return Sample(X,Y,Z)||SampleShore(X,Y,Z)||SampleCoast(X,Y,Z); }

FVector FDiveTerrain::SurfaceNormal(double X,double Y) const
{
    float z=0,n=0,s=0,e=0,w=0;SampleVertex(X,Y,z);
    if(!SampleVertex(X+100,Y,n)) n=z;if(!SampleVertex(X-100,Y,s)) s=z;
    if(!SampleVertex(X,Y+100,e)) e=z;if(!SampleVertex(X,Y-100,w)) w=z;
    const FVector fine=FVector(-(n-s)/200,-(e-w)/200,1).GetSafeNormal();
    const double east=OriginEast+Y/100,north=OriginNorth+X/100;
    double blend=1;
    for(const auto& shore:Shores)
    {
        const Dive::ShoreTransition bounds(shore,Landscape);
        if(bounds.Contains(east,north)) { blend=bounds.Weight(east,north);break; }
    }
    double coastHeight=0;
    if(!InShorePatch(X,Y)&&Coast.Sample(east,north,coastHeight)) blend=CoastWeight(east,north);
    if(blend>=1) return fine;
    const auto& land=Landscape;
    const double px=(east-land.West)/land.Spacing-.5,py=(land.North-north)/land.Spacing-.5;
    const int col=FMath::FloorToInt(px),row=FMath::FloorToInt(py);
    if(col<1||row<1||col+2>=land.Columns||row+2>=land.Rows) return fine;
    auto at=[&](int c,int r)
    {
        auto h=[&](int x,int y){return land.Heights[static_cast<size_t>(y)*land.Columns+x];};
        return FVector(-(h(c,r-1)-h(c,r+1))/(2*land.Spacing),-(h(c+1,r)-h(c-1,r))/(2*land.Spacing),1).GetSafeNormal();
    };
    const double tx=px-col,ty=py-row;const FVector a=at(col,row),b=at(col+1,row),c=at(col,row+1),d=at(col+1,row+1);
    const FVector coarse=(tx+ty<=1?a+(b-a)*tx+(c-a)*ty:d+(c-d)*(1-tx)+(b-d)*(1-ty)).GetSafeNormal();
    return FMath::Lerp(coarse,fine,blend).GetSafeNormal();
}
