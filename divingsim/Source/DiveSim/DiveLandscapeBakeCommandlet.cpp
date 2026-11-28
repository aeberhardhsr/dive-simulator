#include "DiveLandscapeBakeCommandlet.h"

#if WITH_EDITOR
#include "DiveTerrain.h"
#include "DiveLandscapeDetail.h"
#include "DiveLandscapePilot.h"
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
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#endif

UDiveLandscapeBakeCommandlet::UDiveLandscapeBakeCommandlet()
{
    IsClient=false; IsServer=false; IsEditor=true; LogToConsole=true;
}

int32 UDiveLandscapeBakeCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
    return 1;
#else
    FDiveTerrain terrain;
    if(!terrain.Load()) { UE_LOG(LogTemp,Error,TEXT("%s"),*terrain.Error); return 1; }
    if(FParse::Param(*Params,TEXT("LandscapePilot"))) return BakeLandscapePilot(terrain,Params);
    if(FParse::Param(*Params,TEXT("FallbackAudit"))) return AuditLandscapeFallback(terrain,Params);
    if(FParse::Param(*Params,TEXT("ResidentStudy"))) return BakeLandscapeResidentStudy(Params);
    if(FParse::Param(*Params,TEXT("FallbackStudy"))) return BakeLandscapeFallbackStudy(Params);
    if(FParse::Param(*Params,TEXT("DetailStudy"))) return BakeLandscapeDetailStudy(terrain,Params);
    const auto& grid=terrain.Landscape;
    const FString folder=FPaths::ProjectContentDir()/TEXT("Diving/Geodata");
    // Hash the actual inputs, not only their manifest claims. Includes the
    // bathymetry and shore masks which determine the holes in the land mesh.
    FMD5 digest;
    auto input=[&](const FString& file)
    {
        if(file.EndsWith(TEXT(".json")))
        {
            FString text;
            if(!FFileHelper::LoadFileToString(text,*(folder/file))) return false;
            // Git checkouts on Windows/macOS may use different line endings.
            // Metadata formatting must not invalidate otherwise identical meshes.
            text.ReplaceInline(TEXT("\r\n"),TEXT("\n"));
            FTCHARToUTF8 utf8(*text);
            digest.Update(reinterpret_cast<const uint8*>(utf8.Get()),utf8.Length());
            return true;
        }
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(folder/file))) return false;
        digest.Update(bytes.GetData(),bytes.Num()); return true;
    };
    if(!input(TEXT("landscape.f32"))||!input(TEXT("landscape.json"))||
        !input(TEXT("manifest.json"))||!input(TEXT("shoreline.json"))||!input(TEXT("Coast/coast.json"))) return 1;
    TArray<FString> bathy;
    IFileManager::Get().FindFiles(bathy,*(folder/TEXT("*.i16")),true,false); bathy.Sort();
    for(const auto& file:bathy) if(!input(file)) return 1;
    uint8 hash[16]; digest.Final(hash);
    const FString sourceHash=BytesToHex(hash,16);
    constexpr int TileCells=256, Version=4;
    int limit=MAX_int32; FParse::Value(*Params,TEXT("Limit="),limit);
    const bool force=FParse::Param(*Params,TEXT("Force"));
    const bool verifyOnly=FParse::Param(*Params,TEXT("VerifyOnly"));
    TStrongObjectPtr<UMaterialInterface> material(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_LandscapePhoto.M_LandscapePhoto")));
    if(!material) { UE_LOG(LogTemp,Error,TEXT("Build imagery materials first")); return 1; }
    TArray<TSharedPtr<FJsonValue>> tiles;
    int64 triangleTotal=0,vertexTotal=0;
    for(int y0=0;y0<grid.Rows-1;y0+=TileCells) for(int x0=0;x0<grid.Columns-1;x0+=TileCells)
    {
        if(tiles.Num()>=limit) { UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_BAKE_PARTIAL tiles=%d"),tiles.Num()); return 0; }
        const int x1=FMath::Min(x0+TileCells,grid.Columns-1),y1=FMath::Min(y0+TileCells,grid.Rows-1);
        const int nx=x1-x0+1,ny=y1-y0+1;
        const double east=grid.West+(x0+.5)*grid.Spacing,north=grid.North-(y0+.5)*grid.Spacing;
        FVector origin=terrain.ToWorld(east,north); origin.Z=0;
        const FString name=FString::Printf(TEXT("SM_Land_%02d_%02d"),x0/TileCells,y0/TileCells);
        const FString packageName=TEXT("/Game/Diving/Landscape/")+name;
        UPackage* package=CreatePackage(*packageName);
        package->FullyLoad();
        auto* mesh=FindObject<UStaticMesh>(package,*name);
        const FString key=FString::Printf(TEXT("%s:v%d"),*sourceHash,Version);
        const bool reuse=mesh&&!force&&package->GetMetaData().GetValue(mesh,TEXT("DiveLandscapeSource"))==key;
        if(verifyOnly&&!reuse) { UE_LOG(LogTemp,Error,TEXT("Missing/stale landscape asset %s"),*name); return 1; }
        int triangles=0,vertices=0;
        if(!reuse)
        {
            if(!mesh) mesh=NewObject<UStaticMesh>(package,*name,RF_Public|RF_Standalone);
            mesh->SetNumSourceModels(1);
            mesh->GetStaticMaterials().Reset();
            mesh->GetStaticMaterials().Add(FStaticMaterial(material.Get(),TEXT("Terrain"),TEXT("Terrain")));
            FMeshDescription description;
            FStaticMeshAttributes attr(description); attr.Register();
            auto positions=attr.GetVertexPositions();
            auto normals=attr.GetVertexInstanceNormals();
            auto tangents=attr.GetVertexInstanceTangents();
            auto signs=attr.GetVertexInstanceBinormalSigns();
            auto colors=attr.GetVertexInstanceColors();
            auto uvs=attr.GetVertexInstanceUVs(); uvs.SetNumChannels(1);
            auto group=description.CreatePolygonGroup();
            attr.GetPolygonGroupMaterialSlotNames()[group]=TEXT("Terrain");
            TArray<FVertexID> ids; TArray<FVector3f> vertexNormals; TArray<FVector2f> vertexUVs; TArray<bool> land;
            auto height=[&](int x,int y){return grid.Heights[static_cast<size_t>(y)*grid.Columns+x];};
            for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x)
            {
                const auto id=description.CreateVertex(); ids.Add(id);
                const double h=height(x,y);
                positions[id]=FVector3f(-(y-y0)*grid.Spacing*100,(x-x0)*grid.Spacing*100,h*100);
                const int xn=FMath::Max(0,x-1),xp=FMath::Min(grid.Columns-1,x+1);
                const int yn=FMath::Max(0,y-1),yp=FMath::Min(grid.Rows-1,y+1);
                const float dn=(height(x,yn)-height(x,yp))/((yp-yn)*grid.Spacing);
                const float de=(height(xp,y)-height(xn,y))/((xp-xn)*grid.Spacing);
                vertexNormals.Add(FVector3f(-dn,-de,1).GetSafeNormal());
                vertexUVs.Add(FVector2f((x+.5)/grid.Columns,(y+.5)/grid.Rows));
                const FVector p=origin+FVector(positions[id]); float bottom=0;
                land.Add(h>1&&!terrain.Sample(p.X,p.Y,bottom));
            }
            auto triangle=[&](int a,int b,int c)
            {
                const FVector center=origin+FVector((positions[ids[a]]+positions[ids[b]]+positions[ids[c]])/3);
                if(!(land[a]||land[b]||land[c])||terrain.InShorePatch(center.X,center.Y)||terrain.InCoast(center.X,center.Y)) return;
                TArray<FVertexInstanceID> instances;
                for(int index:{a,b,c})
                {
                    const auto instance=description.CreateVertexInstance(ids[index]); instances.Add(instance);
                    normals[instance]=vertexNormals[index];
                    tangents[instance]=FVector3f::CrossProduct(vertexNormals[index],FVector3f(1,0,0)).GetSafeNormal();
                    signs[instance]=1; colors[instance]=FVector4f(1,1,1,1); uvs.Set(instance,0,vertexUVs[index]);
                }
                description.CreatePolygon(group,instances); ++triangles;
            };
            for(int y=0;y<ny-1;++y) for(int x=0;x<nx-1;++x)
            { const int a=y*nx+x; triangle(a,a+nx,a+1); triangle(a+1,a+nx,a+nx+1); }
            vertices=ids.Num();
            if(triangles==0) continue;
            // Mesh descriptions retain the exact original height samples.
            // Nanite selects render detail; no trim is applied to its source.
            mesh->CreateMeshDescription(0,MoveTemp(description)); mesh->CommitMeshDescription(0);
            auto& settings=mesh->GetSourceModel(0).BuildSettings;
            settings.bRecomputeNormals=false; settings.bRecomputeTangents=false;
            settings.bUseFullPrecisionUVs=true; settings.bGenerateLightmapUVs=false;
            settings.bRemoveDegenerates=true;
            settings.DistanceFieldResolutionScale=.25f;
            settings.MaxLumenMeshCards=12;
            FMeshNaniteSettings nanite;
            nanite.bEnabled=true; nanite.PositionPrecision=FDiveTerrain::NanitePositionPrecision;
            nanite.GenerateFallback=ENaniteGenerateFallback::Enabled;
            nanite.FallbackTarget=ENaniteFallbackTarget::RelativeError; nanite.FallbackRelativeError=.25f;
            mesh->SetNaniteSettings(nanite);
            mesh->Build(false);
            FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
            if(!mesh->HasValidNaniteData()) { UE_LOG(LogTemp,Error,TEXT("Nanite data missing: %s"),*name); return 1; }
            package->GetMetaData().SetValue(mesh,TEXT("DiveLandscapeSource"),*key);
            package->GetMetaData().SetValue(mesh,TEXT("DiveLandscapeTriangles"),*FString::FromInt(triangles));
            package->GetMetaData().SetValue(mesh,TEXT("DiveLandscapeVertices"),*FString::FromInt(vertices));
            FAssetRegistryModule::AssetCreated(mesh);
            package->MarkPackageDirty();
            FSavePackageArgs args; args.TopLevelFlags=RF_Public|RF_Standalone; args.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(package,mesh,*FPackageName::LongPackageNameToFilename(packageName,FPackageName::GetAssetPackageExtension()),args))
            { UE_LOG(LogTemp,Error,TEXT("Cannot save %s"),*name); return 1; }
        }
        else
        {
            triangles=FCString::Atoi(*package->GetMetaData().GetValue(mesh,TEXT("DiveLandscapeTriangles")));
            vertices=FCString::Atoi(*package->GetMetaData().GetValue(mesh,TEXT("DiveLandscapeVertices")));
        }
        const auto* description=mesh->GetMeshDescription(0);
        if(mesh->GetNaniteSettings().PositionPrecision!=FDiveTerrain::NanitePositionPrecision) return 1;
        if(!description||description->Vertices().Num()!=nx*ny||description->Triangles().Num()!=triangles||
            !mesh->HasValidNaniteData()||vertices!=nx*ny)
        { UE_LOG(LogTemp,Error,TEXT("Landscape source topology mismatch %s"),*name); return 1; }
        FStaticMeshConstAttributes attributes(*description);
        const auto positions=attributes.GetVertexPositions();
        const auto uvs=attributes.GetVertexInstanceUVs();
        const auto normals=attributes.GetVertexInstanceNormals();
        const auto tangents=attributes.GetVertexInstanceTangents();
        for(const auto id:description->Vertices().GetElementIDs())
        {
            const auto p=positions[id];
            const int col=x0+FMath::RoundToInt(p.Y/(grid.Spacing*100));
            const int row=y0-FMath::RoundToInt(p.X/(grid.Spacing*100));
            if(col<x0||col>x1||row<y0||row>y1||
                FMath::Abs(p.X+(row-y0)*grid.Spacing*100)>.01||
                FMath::Abs(p.Y-(col-x0)*grid.Spacing*100)>.01||
                FMath::Abs(p.Z-static_cast<double>(grid.Heights[static_cast<size_t>(row)*grid.Columns+col])*100)>.02)
            { UE_LOG(LogTemp,Error,TEXT("Landscape source height/position mismatch %s"),*name); return 1; }
        }
        for(const auto id:description->VertexInstances().GetElementIDs())
        {
            const auto p=positions[description->GetVertexInstanceVertex(id)];
            const FVector2f expected((x0+.5+p.Y/(grid.Spacing*100))/grid.Columns,
                                    (y0+.5-p.X/(grid.Spacing*100))/grid.Rows);
            if(!uvs.Get(id,0).Equals(expected,1.e-6f)||FMath::Abs(normals[id].SizeSquared()-1)>.0001||
                normals[id].Z<=0||FMath::Abs(FVector3f::DotProduct(normals[id],tangents[id]))>.0001)
            { UE_LOG(LogTemp,Error,TEXT("Landscape UV/normal/tangent mismatch %s"),*name); return 1; }
        }
        auto entry=MakeShared<FJsonObject>();
        entry->SetStringField(TEXT("asset"),packageName+TEXT(".")+name);
        entry->SetNumberField(TEXT("column"),x0); entry->SetNumberField(TEXT("row"),y0);
        entry->SetNumberField(TEXT("last_column"),x1); entry->SetNumberField(TEXT("last_row"),y1);
        entry->SetNumberField(TEXT("origin_x_cm"),origin.X); entry->SetNumberField(TEXT("origin_y_cm"),origin.Y);
        entry->SetNumberField(TEXT("triangles"),triangles); entry->SetNumberField(TEXT("vertices"),vertices);
        tiles.Add(MakeShared<FJsonValueObject>(entry)); triangleTotal+=triangles; vertexTotal+=vertices;
        UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_BAKE tile=%s reused=%d triangles=%d"),*name,reuse,triangles);
        // Each tile is independent; release mesh descriptions/build scratch
        // before the next tile rather than retaining the full region in RAM.
        CollectGarbage(RF_NoFlags);
    }
    auto root=MakeShared<FJsonObject>();
    root->SetNumberField(TEXT("schema"),1); root->SetNumberField(TEXT("baker_version"),Version);
    root->SetNumberField(TEXT("nanite_position_precision"),FDiveTerrain::NanitePositionPrecision);
    root->SetStringField(TEXT("input_md5"),sourceHash);
    root->SetNumberField(TEXT("spacing_m"),grid.Spacing); root->SetNumberField(TEXT("tile_cells"),TileCells);
    root->SetNumberField(TEXT("triangles"),triangleTotal); root->SetNumberField(TEXT("vertices"),vertexTotal);
    root->SetArrayField(TEXT("tiles"),tiles);
    FString json; FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json));
    if(!verifyOnly&&!FFileHelper::SaveStringToFile(json,*(folder/TEXT("landscape_meshes.json")))) return 1;
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_%s_PASS tiles=%d vertices=%lld triangles=%lld"),verifyOnly?TEXT("VERIFY"):TEXT("BAKE"),tiles.Num(),vertexTotal,triangleTotal);
    return 0;
#endif
}
