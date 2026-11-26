#include "DiveCoastBakeCommandlet.h"
#if WITH_EDITOR
#include "DiveTerrain.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "PhysicsEngine/BodySetup.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/SavePackage.h"
#include "UObject/MetaData.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#endif

UDiveCoastBakeCommandlet::UDiveCoastBakeCommandlet()
{ IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true; }

int32 UDiveCoastBakeCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
    return 1;
#else
    FDiveTerrain terrain;
    if(!terrain.Load()) { UE_LOG(LogTemp,Error,TEXT("%s"),*terrain.Error);return 1; }
    const bool verify=FParse::Param(*Params,TEXT("VerifyOnly")),force=FParse::Param(*Params,TEXT("Force"));
    int limit=MAX_int32;FParse::Value(*Params,TEXT("Limit="),limit);
    TStrongObjectPtr<UMaterialInterface> material(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_ShorePhoto.M_ShorePhoto")));
    if(!material) return 1;
    TArray<TSharedPtr<FJsonValue>> entries;int64 total=0;int assets=0;
    constexpr int N=129,Version=2;
    for(const auto& pair:terrain.Coast.Tiles)
    {
        if(entries.Num()>=limit) { UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_BAKE_PARTIAL"));return 0; }
        const double west=4+pair.first.first*128,south=4+pair.first.second*128,north=south+128;
        FVector origin=terrain.ToWorld(west,north);origin.Z=0;
        const FString name=FString::Printf(TEXT("SM_Coast_%d_%d"),pair.first.first,pair.first.second);
        const FString packageName=TEXT("/Game/Diving/Coast/")+name;
        TArray<FVector3f> points,vertexNormals;TArray<bool> measured;TArray<int32> indices;
        points.Reserve(N*N);vertexNormals.Reserve(N*N);measured.Reserve(N*N);
        for(int row=0;row<N;++row) for(int col=0;col<N;++col)
        {
            const FVector p=terrain.ToWorld(west+col,north-row);float z=0,bed=0;
            if(!terrain.SampleVertex(p.X,p.Y,z)) { UE_LOG(LogTemp,Error,TEXT("Missing coast vertex"));return 1; }
            points.Add(FVector3f(-row*100,col*100,z));vertexNormals.Add(FVector3f(terrain.SurfaceNormal(p.X,p.Y)));
            measured.Add(terrain.Sample(p.X,p.Y,bed));
        }
        auto triangle=[&](int a,int b,int c)
        {
            if(measured[a]&&measured[b]&&measured[c]) return;
            const FVector center=origin+FVector((points[a]+points[b]+points[c])/3);
            if(terrain.InShorePatch(center.X,center.Y)) return;
            indices.Append({a,b,c});
        };
        for(int row=0;row<N-1;++row) for(int col=0;col<N-1;++col)
        {
            const int a=row*N+col;
            // Identical physical diagonal to the streamed bathymetry and shore.
            triangle(a,a+N,a+N+1);triangle(a,a+N+1,a+1);
        }
        const int triangles=indices.Num()/3;
        if(triangles>0)
        {
            UPackage* package=CreatePackage(*packageName);package->FullyLoad();
            auto* mesh=FindObject<UStaticMesh>(package,*name);
            const FString key=FString::Printf(TEXT("v%d:%s"),Version,*terrain.CoastSourceHash);
            const bool reuse=mesh&&!force&&package->GetMetaData().GetValue(mesh,TEXT("DiveCoastSource"))==key;
            if(verify&&!reuse) { UE_LOG(LogTemp,Error,TEXT("Missing/stale coast %s"),*name);return 1; }
            if(!reuse)
            {
                if(!mesh) mesh=NewObject<UStaticMesh>(package,*name,RF_Public|RF_Standalone);
                mesh->SetNumSourceModels(1);mesh->GetStaticMaterials().Reset();
                mesh->GetStaticMaterials().Add(FStaticMaterial(material.Get(),TEXT("Coast"),TEXT("Coast")));
                FMeshDescription description;FStaticMeshAttributes attributes(description);attributes.Register();
                auto positions=attributes.GetVertexPositions();auto normals=attributes.GetVertexInstanceNormals();
                auto tangents=attributes.GetVertexInstanceTangents();auto signs=attributes.GetVertexInstanceBinormalSigns();
                auto colors=attributes.GetVertexInstanceColors();auto uvs=attributes.GetVertexInstanceUVs();uvs.SetNumChannels(1);
                const auto group=description.CreatePolygonGroup();attributes.GetPolygonGroupMaterialSlotNames()[group]=TEXT("Coast");
                TArray<FVertexID> ids;
                for(const auto& p:points) { const auto id=description.CreateVertex();positions[id]=p;ids.Add(id); }
                for(int index=0;index<indices.Num();index+=3)
                {
                    TArray<FVertexInstanceID> corners;
                    for(int k=0;k<3;++k)
                    {
                        const int i=indices[index+k];const auto instance=description.CreateVertexInstance(ids[i]);corners.Add(instance);
                        normals[instance]=vertexNormals[i];tangents[instance]=FVector3f::CrossProduct(vertexNormals[i],FVector3f(1,0,0)).GetSafeNormal();
                        signs[instance]=1;colors[instance]=FVector4f(1,1,1,0); // Full-region photo, matching the land mesh.
                        uvs.Set(instance,0,FVector2f((west+i%N-terrain.Landscape.West)/(terrain.Landscape.Columns*8),
                            (terrain.Landscape.North-north+i/N)/(terrain.Landscape.Rows*8)));
                    }
                    description.CreatePolygon(group,corners);
                }
                mesh->CreateMeshDescription(0,MoveTemp(description));mesh->CommitMeshDescription(0);
                auto& settings=mesh->GetSourceModel(0).BuildSettings;
                settings.bRecomputeNormals=false;settings.bRecomputeTangents=false;settings.bUseFullPrecisionUVs=true;
                settings.bGenerateLightmapUVs=false;settings.bRemoveDegenerates=true;settings.DistanceFieldResolutionScale=.25f;settings.MaxLumenMeshCards=12;
                FMeshNaniteSettings nanite;nanite.bEnabled=true;nanite.PositionPrecision=FDiveTerrain::NanitePositionPrecision;
                nanite.GenerateFallback=ENaniteGenerateFallback::Enabled;
                nanite.FallbackTarget=ENaniteFallbackTarget::PercentTriangles;nanite.FallbackPercentTriangles=1;
                mesh->SetNaniteSettings(nanite);mesh->CreateBodySetup();
                mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;mesh->GetBodySetup()->bDoubleSidedGeometry=true;
                mesh->Build(false);FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
                mesh->GetBodySetup()->InvalidatePhysicsData();mesh->GetBodySetup()->CreatePhysicsMeshes();
                if(!mesh->HasValidNaniteData()) return 1;
                package->GetMetaData().SetValue(mesh,TEXT("DiveCoastSource"),*key);
                FAssetRegistryModule::AssetCreated(mesh);package->MarkPackageDirty();
                FSavePackageArgs args;args.TopLevelFlags=RF_Public|RF_Standalone;args.SaveFlags=SAVE_NoError;
                if(!UPackage::SavePackage(package,mesh,*FPackageName::LongPackageNameToFilename(packageName,FPackageName::GetAssetPackageExtension()),args)) return 1;
            }
            const auto* source=mesh->GetMeshDescription(0);
            FTriMeshCollisionData collision;
            if(!source||source->Triangles().Num()!=triangles||source->Vertices().Num()!=N*N||!mesh->HasValidNaniteData()||
               !mesh->GetBodySetup()||mesh->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple||
               !mesh->GetPhysicsTriMeshData(&collision,true)||collision.Indices.Num()!=triangles) return 1;
            FStaticMeshConstAttributes attributes(*source);const auto positions=attributes.GetVertexPositions();
            for(const auto id:source->Vertices().GetElementIDs())
                if(!positions[id].Equals(points[id.GetValue()],.02f)) return 1;
            ++assets;
            UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_TILE %d/%d %s triangles=%d reused=%d"),entries.Num()+1,static_cast<int>(terrain.Coast.Tiles.size()),*name,triangles,reuse);
        }
        auto entry=MakeShared<FJsonObject>();entry->SetNumberField(TEXT("west"),west);entry->SetNumberField(TEXT("south"),south);
        entry->SetNumberField(TEXT("triangles"),triangles);entry->SetStringField(TEXT("asset"),triangles>0?packageName+TEXT(".")+name:TEXT(""));
        if(triangles>0)
        {
            const FVector probe=origin+(FVector(points[indices[0]])+FVector(points[indices[1]])+FVector(points[indices[2]]))/3;
            entry->SetNumberField(TEXT("probe_x_cm"),probe.X);entry->SetNumberField(TEXT("probe_y_cm"),probe.Y);entry->SetNumberField(TEXT("probe_z_cm"),probe.Z);
        }
        entries.Add(MakeShared<FJsonValueObject>(entry));total+=triangles;
        CollectGarbage(RF_NoFlags);
    }
    auto root=MakeShared<FJsonObject>();root->SetNumberField(TEXT("schema"),1);root->SetNumberField(TEXT("baker_version"),Version);
    root->SetStringField(TEXT("source_md5"),terrain.CoastSourceHash);root->SetArrayField(TEXT("tiles"),entries);
    root->SetNumberField(TEXT("triangles"),total);root->SetNumberField(TEXT("assets"),assets);
    FString json;FJsonSerializer::Serialize(root,TJsonWriterFactory<>::Create(&json));
    if(verify)
    {
        FString saved;
        if(!FFileHelper::LoadFileToString(saved,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/Coast/meshes.json")))) return 1;
        saved.ReplaceInline(TEXT("\r\n"),TEXT("\n"));json.ReplaceInline(TEXT("\r\n"),TEXT("\n"));
        if(saved!=json) { UE_LOG(LogTemp,Error,TEXT("Coast manifest does not match verified geometry"));return 1; }
    }
    if(!verify&&!FFileHelper::SaveStringToFile(json,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/Coast/meshes.json")))) return 1;
    UE_LOG(LogTemp,Display,TEXT("DIVE_COAST_%s_PASS tiles=%d assets=%d triangles=%lld"),verify?TEXT("VERIFY"):TEXT("BAKE"),entries.Num(),assets,total);
    return 0;
#endif
}
