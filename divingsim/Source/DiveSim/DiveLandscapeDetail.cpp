#include "DiveLandscapeDetail.h"
#if WITH_EDITOR
#include "DiveTerrain.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "UObject/MetaData.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"

namespace
{
constexpr int DetailGridSize=1025,DetailColumn=1280,DetailRow=512,DetailBakerVersion=1;
const TCHAR* DetailPackageName=TEXT("/Game/Diving/LandscapeStudy/SM_LandDetail_05_02");
const TCHAR* DetailAssetName=TEXT("/Game/Diving/LandscapeStudy/SM_LandDetail_05_02.SM_LandDetail_05_02");
struct FDetailInput { TArray<float> Heights; FString Key; };

void InstallGround(FDiveTerrain& Terrain,const FDetailInput& Input)
{
    auto& grid=Terrain.LandscapeDetail;
    grid.Columns=grid.Rows=DetailGridSize;grid.Spacing=2;
    // SurfaceGrid addresses cell centres; the prepared values are vertices.
    grid.West=Terrain.Landscape.West+(DetailColumn+.5)*8-1;
    grid.North=Terrain.Landscape.North-(DetailRow+.5)*8+1;
    grid.Heights.assign(Input.Heights.GetData(),Input.Heights.GetData()+Input.Heights.Num());
}

bool HashMatches(const TArray<uint8>& Bytes,const FString& Expected)
{
    return FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString().Equals(Expected,ESearchCase::IgnoreCase);
}

bool ReadInput(FDiveTerrain& Terrain,FDetailInput& Input)
{
    FString text;TSharedPtr<FJsonObject> json;
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata");
    if(!FFileHelper::LoadFileToString(text,*(folder/TEXT("LandscapeStudy/detail_study.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),json)||!json) return false;
    if(json->GetIntegerField(TEXT("schema"))!=1||json->GetIntegerField(TEXT("width"))!=DetailGridSize||json->GetIntegerField(TEXT("height"))!=DetailGridSize||
       json->GetIntegerField(TEXT("column"))!=DetailColumn||json->GetIntegerField(TEXT("row"))!=DetailRow||
       json->GetNumberField(TEXT("spacing_m"))!=2||json->GetNumberField(TEXT("blend_border_m"))!=32||
       json->GetStringField(TEXT("asset"))!=DetailAssetName||json->GetStringField(TEXT("file"))!=TEXT("detail_05_02.f32")||
       Terrain.Landscape.Spacing!=8) return false;
    const double east=Terrain.Landscape.West+(DetailColumn+.5)*8,north=Terrain.Landscape.North-(DetailRow+.5)*8;
    const FVector origin=Terrain.ToWorld(east,north);
    if(json->GetNumberField(TEXT("west_vertex_east"))!=east||json->GetNumberField(TEXT("north_vertex_north"))!=north||
       json->GetNumberField(TEXT("origin_x_cm"))!=origin.X||json->GetNumberField(TEXT("origin_y_cm"))!=origin.Y) return false;
    TArray<uint8> bytes;
    if(!FFileHelper::LoadFileToArray(bytes,*(folder/TEXT("landscape.f32")))||!HashMatches(bytes,json->GetStringField(TEXT("base_grid_sha1")))) return false;
    if(!FFileHelper::LoadFileToArray(bytes,*(FPaths::ProjectContentDir()/TEXT("Diving/Landscape/SM_Land_05_02.uasset")))||
       !HashMatches(bytes,json->GetStringField(TEXT("base_asset_sha1")))) return false;
    if(!FFileHelper::LoadFileToArray(bytes,*(folder/TEXT("LandscapeStudy/detail_05_02.f32")))||bytes.Num()!=DetailGridSize*DetailGridSize*sizeof(float)||
       !HashMatches(bytes,json->GetStringField(TEXT("native_sha1")))) return false;
    Input.Heights.SetNumUninitialized(DetailGridSize*DetailGridSize);FMemory::Memcpy(Input.Heights.GetData(),bytes.GetData(),bytes.Num());
    for(float h:Input.Heights) if(!FMath::IsFinite(h)||h<80||h>3000) return false;
    Input.Key=FString::Printf(TEXT("%s:%s:v%d"),*json->GetStringField(TEXT("sha256")),*json->GetStringField(TEXT("base_grid_sha256")),DetailBakerVersion);
    return true;
}

float CoarseHeight(const FDiveTerrain& Terrain,int X,int Y)
{ return Terrain.Landscape.Heights[static_cast<size_t>(DetailRow+Y)*Terrain.Landscape.Columns+DetailColumn+X]; }

FVector3f CoarseNormal(const FDiveTerrain& Terrain,int X,int Y)
{
    const float dn=(CoarseHeight(Terrain,X,Y-1)-CoarseHeight(Terrain,X,Y+1))/16;
    const float de=(CoarseHeight(Terrain,X+1,Y)-CoarseHeight(Terrain,X-1,Y))/16;
    return FVector3f(-dn,-de,1).GetSafeNormal();
}

FVector3f BoundaryNormal(const FDiveTerrain& Terrain,int X,int Y)
{
    const int c=FMath::Min(X/4,255),r=FMath::Min(Y/4,255);
    const float u=X/4.f-c,v=Y/4.f-r;
    const FVector3f a=CoarseNormal(Terrain,c,r),b=CoarseNormal(Terrain,c+1,r),
        c0=CoarseNormal(Terrain,c,r+1),d=CoarseNormal(Terrain,c+1,r+1);
    return (u+v<=1?a+u*(b-a)+v*(c0-a):d+(1-u)*(c0-d)+(1-v)*(b-d)).GetSafeNormal();
}
}

int32 BakeLandscapeDetailStudy(FDiveTerrain& Terrain,const FString& Params)
{
    FDetailInput input;
    if(!ReadInput(Terrain,input)) { UE_LOG(LogTemp,Error,TEXT("DIVE_LAND_DETAIL_FAIL input"));return 1; }
    TStrongObjectPtr<UStaticMesh> base(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Diving/Landscape/SM_Land_05_02.SM_Land_05_02")));
    const auto* original=base?base->GetMeshDescription(0):nullptr;
    if(!original||original->Vertices().Num()!=66049||original->Triangles().Num()!=131072) return 1;
    UPackage* package=CreatePackage(DetailPackageName);package->FullyLoad();
    auto* mesh=FindObject<UStaticMesh>(package,TEXT("SM_LandDetail_05_02"));
    const bool reuse=mesh&&package->GetMetaData().GetValue(mesh,TEXT("DiveLandscapeDetailSource"))==input.Key;
    if(FParse::Param(*Params,TEXT("VerifyOnly"))&&!reuse) return 1;
    if(!reuse)
    {
        if(!mesh) mesh=NewObject<UStaticMesh>(package,TEXT("SM_LandDetail_05_02"),RF_Public|RF_Standalone);
        mesh->SetNumSourceModels(1);mesh->GetStaticMaterials()=base->GetStaticMaterials();
        FMeshDescription description;FStaticMeshAttributes attr(description);attr.Register();
        auto positions=attr.GetVertexPositions();auto normals=attr.GetVertexInstanceNormals();
        auto tangents=attr.GetVertexInstanceTangents();auto signs=attr.GetVertexInstanceBinormalSigns();
        auto colors=attr.GetVertexInstanceColors();auto uvs=attr.GetVertexInstanceUVs();uvs.SetNumChannels(1);
        auto group=description.CreatePolygonGroup();attr.GetPolygonGroupMaterialSlotNames()[group]=TEXT("Terrain");
        TArray<FVertexID> ids;TArray<FVector3f> vertexNormals;
        ids.Reserve(DetailGridSize*DetailGridSize);vertexNormals.Reserve(DetailGridSize*DetailGridSize);
        auto height=[&](int x,int y){return input.Heights[y*DetailGridSize+x];};
        for(int y=0;y<DetailGridSize;++y) for(int x=0;x<DetailGridSize;++x)
        {
            const auto id=description.CreateVertex();ids.Add(id);positions[id]=FVector3f(-y*200,x*200,height(x,y)*100);
            const int xn=FMath::Max(x-1,0),xp=FMath::Min(x+1,DetailGridSize-1),yn=FMath::Max(y-1,0),yp=FMath::Min(y+1,DetailGridSize-1);
            const float dn=(height(x,yn)-height(x,yp))/((yp-yn)*2),de=(height(xp,y)-height(xn,y))/((xp-xn)*2);
            const float t=FMath::Clamp(FMath::Min(FMath::Min(x,DetailGridSize-1-x),FMath::Min(y,DetailGridSize-1-y))/16.f,0.f,1.f);
            vertexNormals.Add(FMath::Lerp(BoundaryNormal(Terrain,x,y),FVector3f(-dn,-de,1).GetSafeNormal(),t*t*(3-2*t)).GetSafeNormal());
        }
        auto triangle=[&](int a,int b,int c)
        {
            TArray<FVertexInstanceID> instances;
            for(int index:{a,b,c})
            {
                const auto instance=description.CreateVertexInstance(ids[index]);instances.Add(instance);
                normals[instance]=vertexNormals[index];tangents[instance]=FVector3f::CrossProduct(vertexNormals[index],FVector3f(1,0,0)).GetSafeNormal();
                signs[instance]=1;colors[instance]=FVector4f(1,1,1,1);
                uvs.Set(instance,0,FVector2f((DetailColumn+.5+(index%DetailGridSize)/4.)/Terrain.Landscape.Columns,(DetailRow+.5+(index/DetailGridSize)/4.)/Terrain.Landscape.Rows));
            }
            description.CreatePolygon(group,instances);
        };
        for(int y=0;y<DetailGridSize-1;++y) for(int x=0;x<DetailGridSize-1;++x)
        { const int a=y*DetailGridSize+x;triangle(a,a+DetailGridSize,a+1);triangle(a+1,a+DetailGridSize,a+DetailGridSize+1); }
        mesh->CreateMeshDescription(0,MoveTemp(description));mesh->CommitMeshDescription(0);
        auto& settings=mesh->GetSourceModel(0).BuildSettings;
        settings.bRecomputeNormals=false;settings.bRecomputeTangents=false;settings.bUseFullPrecisionUVs=true;
        settings.bGenerateLightmapUVs=false;settings.bRemoveDegenerates=true;settings.DistanceFieldResolutionScale=.25f;settings.MaxLumenMeshCards=12;
        FMeshNaniteSettings nanite;nanite.bEnabled=true;nanite.PositionPrecision=FDiveTerrain::NanitePositionPrecision;
        nanite.GenerateFallback=ENaniteGenerateFallback::Enabled;nanite.FallbackTarget=ENaniteFallbackTarget::RelativeError;nanite.FallbackRelativeError=.25f;
        mesh->SetNaniteSettings(nanite);mesh->Build(false);FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
        if(!mesh->HasValidNaniteData()) return 1;
        package->GetMetaData().SetValue(mesh,TEXT("DiveLandscapeDetailSource"),*input.Key);
        FAssetRegistryModule::AssetCreated(mesh);package->MarkPackageDirty();
        FSavePackageArgs args;args.TopLevelFlags=RF_Public|RF_Standalone;args.SaveFlags=SAVE_NoError;
        if(!UPackage::SavePackage(package,mesh,*FPackageName::LongPackageNameToFilename(DetailPackageName,FPackageName::GetAssetPackageExtension()),args)) return 1;
    }
    const auto* description=mesh->GetMeshDescription(0);
    if(!description||description->Vertices().Num()!=DetailGridSize*DetailGridSize||description->Triangles().Num()!=2*(DetailGridSize-1)*(DetailGridSize-1)||!mesh->HasValidNaniteData()) return 1;
    FStaticMeshConstAttributes attr(*description);auto positions=attr.GetVertexPositions();
    auto normals=attr.GetVertexInstanceNormals();auto uvs=attr.GetVertexInstanceUVs();auto tangents=attr.GetVertexInstanceTangents();
    double edgeError=0,sourceError=0;
    for(const auto id:description->Vertices().GetElementIDs())
    {
        const auto p=positions[id];const int x=FMath::RoundToInt(p.Y/200),y=FMath::RoundToInt(-p.X/200);
        if(x<0||y<0||x>=DetailGridSize||y>=DetailGridSize||FMath::Abs(p.Y-x*200)>.01||FMath::Abs(p.X+y*200)>.01) return 1;
        sourceError=FMath::Max(sourceError,FMath::Abs(p.Z-input.Heights[y*DetailGridSize+x]*100.));
        if(x==0||y==0||x==DetailGridSize-1||y==DetailGridSize-1)
        {
            const int c=FMath::Min(x/4,255),r=FMath::Min(y/4,255);const double u=x/4.-c,v=y/4.-r;
            const double a=CoarseHeight(Terrain,c,r),b=CoarseHeight(Terrain,c+1,r),c0=CoarseHeight(Terrain,c,r+1),d=CoarseHeight(Terrain,c+1,r+1);
            const double expected=u+v<=1?a+u*(b-a)+v*(c0-a):d+(1-u)*(c0-d)+(1-v)*(b-d);
            edgeError=FMath::Max(edgeError,FMath::Abs(p.Z-expected*100));
        }
    }
    for(const auto id:description->VertexInstances().GetElementIDs())
    {
        const auto p=positions[description->GetVertexInstanceVertex(id)];
        const FVector2f expected((DetailColumn+.5+p.Y/800)/Terrain.Landscape.Columns,(DetailRow+.5-p.X/800)/Terrain.Landscape.Rows);
        if(!uvs.Get(id,0).Equals(expected,1.e-6f)||FMath::Abs(normals[id].SizeSquared()-1)>.0001||normals[id].Z<=0||
           FMath::Abs(FVector3f::DotProduct(normals[id],tangents[id]))>.0001) return 1;
        const int x=FMath::RoundToInt(p.Y/200),y=FMath::RoundToInt(-p.X/200);
        if((x==0||y==0||x==DetailGridSize-1||y==DetailGridSize-1)&&!normals[id].Equals(BoundaryNormal(Terrain,x,y),.00001f)) return 1;
    }
    if(sourceError>.02||edgeError>.02) return 1;
    // Compare runtime ground against planes from the saved mesh itself. Both
    // triangle halves matter: bilinear interpolation can hide large errors.
    TArray<float> shoreBefore;
    for(const auto& entry:Terrain.ShoreEntries)
    { float z=0;if(!Terrain.SampleGround(entry.Position.X,entry.Position.Y,z)) return 1;shoreBefore.Add(z); }
    InstallGround(Terrain,input);
    const FVector origin=Terrain.ToWorld(Terrain.LandscapeDetail.West+1,Terrain.LandscapeDetail.North-1);
    double groundError=0;int groundProbes=0;
    for(const auto triangle:description->Triangles().GetElementIDs())
    {
        const auto corners=description->GetTriangleVertices(triangle);
        const FVector a(positions[corners[0]]),b(positions[corners[1]]),c(positions[corners[2]]);
        for(const FVector weights:{FVector(1./3,1./3,1./3),FVector(.17,.29,.54)})
        {
            const FVector p=a*weights.X+b*weights.Y+c*weights.Z;
            float z=0;
            if(!Terrain.SampleGround(origin.X+p.X,origin.Y+p.Y,z)) return 1;
            groundError=FMath::Max(groundError,FMath::Abs(z-p.Z));++groundProbes;
        }
    }
    double seamError=0;int seamProbes=0;
    for(int i=0;i<=4096;++i) for(int side=0;side<4;++side)
    {
        const double along=i*.5;
        // On both sides of each edge, including fractional coarse cells.
        for(double offset:{-.001,0.,.001})
        {
            const double col=side<2?(side==0?offset:2048-offset):along;
            const double row=side>=2?(side==2?offset:2048-offset):along;
            const double east=Terrain.LandscapeDetail.West+1+col,north=Terrain.LandscapeDetail.North-1-row;
            const FVector p=Terrain.ToWorld(east,north);float z=0;double coarse=0;
            if(!Terrain.SampleGround(p.X,p.Y,z)||!Terrain.Landscape.SampleTriangle(east,north,coarse)) return 1;
            seamError=FMath::Max(seamError,FMath::Abs(z-coarse*100));++seamProbes;
        }
    }
    for(int i=0;i<Terrain.ShoreEntries.Num();++i)
    { const auto& p=Terrain.ShoreEntries[i].Position;float z=0;if(!Terrain.SampleGround(p.X,p.Y,z)||z!=shoreBefore[i]) return 1; }
    if(groundError>.03||seamError>.03) { UE_LOG(LogTemp,Error,TEXT("DIVE_LAND_DETAIL_GROUND_FAIL max_error_cm=%.6f seam_error_cm=%.6f"),groundError,seamError);return 1; }
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_DETAIL_GROUND_PASS probes=%d max_error_cm=%.6f seam_probes=%d seam_error_cm=%.6f shore_unchanged=%d"),groundProbes,groundError,seamProbes,seamError,shoreBefore.Num());
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_DETAIL_BAKE_PASS vertices=%d triangles=%d edge_error_cm=%.6f source_error_cm=%.6f reused=%d"),
        DetailGridSize*DetailGridSize,2*(DetailGridSize-1)*(DetailGridSize-1),edgeError,sourceError,reuse);
    return 0;
}

UStaticMesh* LoadLandscapeDetailStudy(FDiveTerrain& Terrain,FString& Error)
{
    FDetailInput input;
    if(!ReadInput(Terrain,input)) { Error=TEXT("Landscape detail input mismatch");return nullptr; }
    auto* mesh=LoadObject<UStaticMesh>(nullptr,DetailAssetName);
    if(!mesh||!mesh->HasValidNaniteData()||mesh->GetNumTriangles(0)<=0||
       mesh->GetNaniteSettings().PositionPrecision!=FDiveTerrain::NanitePositionPrecision||
       mesh->GetPackage()->GetMetaData().GetValue(mesh,TEXT("DiveLandscapeDetailSource"))!=input.Key)
    { Error=TEXT("Landscape detail asset missing/stale");return nullptr; }
    InstallGround(Terrain,input);
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_DETAIL_READY tile=05_02 spacing=2m boundary=32m source=%s"),*input.Key);
    return mesh;
}
#endif
