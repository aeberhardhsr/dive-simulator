#include "DiveBuildingsBakeCommandlet.h"
#if WITH_EDITOR
#include "DiveBuildings.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/SavePackage.h"
#include "UObject/MetaData.h"
#include "UObject/StrongObjectPtr.h"
#include "Misc/PackageName.h"
#include "Misc/SecureHash.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#endif

UDiveBuildingsBakeCommandlet::UDiveBuildingsBakeCommandlet()
{ IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true; }

int32 UDiveBuildingsBakeCommandlet::Main(const FString& Params)
{
#if !WITH_EDITOR
    return 1;
#else
    TArray<FDiveBuildingTile> tiles;int32 buildings=0;FString error;
    if(!DiveBuildings::Read(tiles,buildings,error)) { UE_LOG(LogTemp,Error,TEXT("%s"),*error);return 1; }
    TStrongObjectPtr<UMaterialInterface> material(LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_Buildings.M_Buildings")));
    if(!material) { UE_LOG(LogTemp,Error,TEXT("Build building materials first."));return 1; }
    const bool verify=FParse::Param(*Params,TEXT("VerifyOnly")),force=FParse::Param(*Params,TEXT("Force"));
    int limit=MAX_int32;FParse::Value(*Params,TEXT("Limit="),limit);int processed=0;int64 total=0;
    for(const auto& tile:tiles)
    {
        if(processed>=limit) break;
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/Buildings")/tile.File))||
            bytes.Num()!=8+tile.Triangles*52||FMemory::Memcmp(bytes.GetData(),"DVB1",4)!=0||
            FMD5::HashBytes(bytes.GetData(),bytes.Num()).ToLower()!=tile.MD5.ToLower())
        { UE_LOG(LogTemp,Error,TEXT("Building binary checksum/length mismatch: %s"),*tile.File);return 1; }
        uint32 count=0;FMemory::Memcpy(&count,bytes.GetData()+4,4);
        if(count!=tile.Triangles) return 1;
        TArray<float> values;values.SetNumUninitialized(tile.Triangles*13);FMemory::Memcpy(values.GetData(),bytes.GetData()+8,values.Num()*4);
        for(float value:values) if(!FMath::IsFinite(value)) return 1;
        const FString packageName=tile.Asset.Left(tile.Asset.Find(TEXT("."))),name=tile.Name();
        UPackage* package=CreatePackage(*packageName);package->FullyLoad();
        auto* mesh=FindObject<UStaticMesh>(package,*name);
        const FString key=FString::Printf(TEXT("v1:%s:%d:%d"),*tile.MD5,tile.Cell.X,tile.Cell.Y);
        const bool reuse=mesh&&!force&&package->GetMetaData().GetValue(mesh,TEXT("DiveBuildingsSource"))==key;
        if(verify&&!reuse) { UE_LOG(LogTemp,Error,TEXT("Missing/stale building asset %s"),*name);return 1; }
        if(!reuse)
        {
            if(!mesh) mesh=NewObject<UStaticMesh>(package,*name,RF_Public|RF_Standalone);
            mesh->SetNumSourceModels(1);mesh->GetStaticMaterials().Reset();
            mesh->GetStaticMaterials().Add(FStaticMaterial(material.Get(),TEXT("Buildings"),TEXT("Buildings")));
            FMeshDescription description;FStaticMeshAttributes attr(description);attr.Register();
            auto positions=attr.GetVertexPositions();auto normals=attr.GetVertexInstanceNormals();
            auto tangents=attr.GetVertexInstanceTangents();auto signs=attr.GetVertexInstanceBinormalSigns();
            auto colors=attr.GetVertexInstanceColors();auto uvs=attr.GetVertexInstanceUVs();uvs.SetNumChannels(1);
            const auto group=description.CreatePolygonGroup();attr.GetPolygonGroupMaterialSlotNames()[group]=TEXT("Buildings");
            for(int triangle=0;triangle<tile.Triangles;++triangle)
            {
                const float* f=values.GetData()+triangle*13;
                const FVector3f a(f[0],f[1],f[2]),b(f[3],f[4],f[5]),c(f[6],f[7],f[8]);
                const FVector3f normal=FVector3f::CrossProduct(c-a,b-a).GetSafeNormal();
                const FVector3f tangent=(b-a).GetSafeNormal();
                if(normal.IsNearlyZero()||tangent.IsNearlyZero())
                { UE_LOG(LogTemp,Error,TEXT("Degenerate building triangle %s:%d"),*name,triangle);return 1; }
                TArray<FVertexInstanceID> instances;
                for(const auto& p:{a,b,c})
                {
                    const auto vertex=description.CreateVertex();positions[vertex]=p;
                    const auto instance=description.CreateVertexInstance(vertex);instances.Add(instance);
                    normals[instance]=normal;tangents[instance]=tangent;signs[instance]=1;
                    colors[instance]=FVector4f(f[9],f[10],f[11],f[12]);
                    const double east=2726000+(tile.Origin.Y+p.Y)/100,north=1219000+(tile.Origin.X+p.X)/100;
                    uvs.Set(instance,0,FVector2f((east-2723000)/22000,(1228000-north)/13000));
                }
                description.CreatePolygon(group,instances);
            }
            mesh->CreateMeshDescription(0,MoveTemp(description));mesh->CommitMeshDescription(0);
            auto& settings=mesh->GetSourceModel(0).BuildSettings;
            settings.bRecomputeNormals=false;settings.bRecomputeTangents=false;
            settings.bUseFullPrecisionUVs=true;settings.bGenerateLightmapUVs=false;
            settings.bRemoveDegenerates=true;settings.DistanceFieldResolutionScale=.5f;settings.MaxLumenMeshCards=32;
            FMeshNaniteSettings nanite;nanite.bEnabled=true;nanite.PositionPrecision=0;
            nanite.GenerateFallback=ENaniteGenerateFallback::Enabled;
            nanite.FallbackTarget=ENaniteFallbackTarget::PercentTriangles;nanite.FallbackPercentTriangles=1;
            mesh->SetNaniteSettings(nanite);
            mesh->CreateBodySetup();mesh->GetBodySetup()->CollisionTraceFlag=CTF_UseComplexAsSimple;
            mesh->GetBodySetup()->bDoubleSidedGeometry=true;
            mesh->Build(false);FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
            mesh->GetBodySetup()->InvalidatePhysicsData();mesh->GetBodySetup()->CreatePhysicsMeshes();
            if(!mesh->HasValidNaniteData()) { UE_LOG(LogTemp,Error,TEXT("Missing Nanite data: %s"),*name);return 1; }
            package->GetMetaData().SetValue(mesh,TEXT("DiveBuildingsSource"),*key);
            FAssetRegistryModule::AssetCreated(mesh);package->MarkPackageDirty();
            FSavePackageArgs args;args.TopLevelFlags=RF_Public|RF_Standalone;args.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(package,mesh,*FPackageName::LongPackageNameToFilename(packageName,FPackageName::GetAssetPackageExtension()),args)) return 1;
        }
        const auto* source=mesh->GetMeshDescription(0);
        if(!source||source->Triangles().Num()!=tile.Triangles||source->Vertices().Num()!=tile.Triangles*3||
            !mesh->HasValidNaniteData()||!mesh->GetBodySetup()||mesh->GetBodySetup()->CollisionTraceFlag!=CTF_UseComplexAsSimple)
        { UE_LOG(LogTemp,Error,TEXT("Building topology/collision mismatch %s"),*name);return 1; }
        FTriMeshCollisionData collision;
        if(!mesh->GetPhysicsTriMeshData(&collision,true)||collision.Indices.Num()!=mesh->GetNumTriangles(0))
        { UE_LOG(LogTemp,Error,TEXT("Building fallback collision incomplete %s"),*name);return 1; }
        FStaticMeshConstAttributes attr(*source);auto positions=attr.GetVertexPositions();
        for(const auto id:source->Vertices().GetElementIDs())
        {
            const int index=id.GetValue(),triangle=index/3,corner=index%3;
            const float* f=values.GetData()+triangle*13+corner*3;
            if(!positions[id].Equals(FVector3f(f[0],f[1],f[2]),.01f)) return 1;
        }
        total+=tile.Triangles;++processed;
        UE_LOG(LogTemp,Display,TEXT("DIVE_BUILDINGS_TILE %d/%d %s triangles=%d reused=%d"),processed,tiles.Num(),*name,tile.Triangles,reuse);
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_BUILDINGS_BAKE_%s buildings=%d tiles=%d triangles=%lld"),processed==tiles.Num()?TEXT("PASS"):TEXT("PARTIAL"),buildings,processed,total);
    return 0;
#endif
}
