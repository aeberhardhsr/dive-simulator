#include "DiveLandscapeDetail.h"
#if WITH_EDITOR
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Rendering/NaniteResources.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshCompiler.h"
#include "MeshDescriptionToDynamicMesh.h"
#include "DynamicMeshToMeshDescription.h"
#include "MeshConstraintsUtil.h"
#include "MeshSimplification.h"
#include "ProjectionTargets.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/MetaData.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/SavePackage.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"

namespace
{
FString FallbackStudyPackage(const UStaticMesh* Source)
{ return TEXT("/Game/Diving/LandscapeStudy/Boundary_")+Source->GetName(); }

using FBoundaryEdge=TPair<FVector3f,FVector3f>;
FBoundaryEdge BoundaryEdge(FVector3f A,FVector3f B)
{
    const bool before=A.X!=B.X?A.X<B.X:A.Y!=B.Y?A.Y<B.Y:A.Z<B.Z;
    return before?FBoundaryEdge(A,B):FBoundaryEdge(B,A);
}

FString FallbackStudyKey(const UStaticMesh* Source)
{
    TArray<uint8> bytes;
    const FString file=FPackageName::LongPackageNameToFilename(Source->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension());
    if(!FFileHelper::LoadFileToArray(bytes,*file)) return FString();
    return FSHA1::HashBuffer(bytes.GetData(),bytes.Num()).ToString()+TEXT(":locked-boundary-v1-10cm");
}
}

UStaticMesh* LoadLandscapeFallbackStudy(UStaticMesh* Source)
{
    if(!Source) return nullptr;
    const FString package=FallbackStudyPackage(Source),key=FallbackStudyKey(Source);
    auto* mesh=LoadObject<UStaticMesh>(nullptr,*(package+TEXT(".")+FPackageName::GetLongPackageAssetName(package)));
    if(key.IsEmpty()||!mesh||!mesh->HasValidNaniteData()||!mesh->IsHiResMeshDescriptionValid()||
       mesh->GetPackage()->GetMetaData().GetValue(mesh,TEXT("DiveFallbackSource"))!=key) return nullptr;
    return mesh;
}

int32 BakeLandscapeFallbackStudy(const FString& Params)
{
    using namespace UE::Geometry;
    for(const TCHAR* path:{TEXT("/Game/Diving/Landscape/SM_Land_04_02.SM_Land_04_02"),
                          TEXT("/Game/Diving/Landscape/SM_Land_05_02.SM_Land_05_02"),
                          TEXT("/Game/Diving/LandscapeStudy/SM_LandDetail_05_02.SM_LandDetail_05_02")})
    {
        TStrongObjectPtr<UStaticMesh> source(LoadObject<UStaticMesh>(nullptr,path));
        const auto* description=source?source->GetMeshDescription(0):nullptr;
        if(!description) return 1;
        const FString packageName=FallbackStudyPackage(source.Get()),key=FallbackStudyKey(source.Get());
        if(key.IsEmpty()) return 1;
        UPackage* package=CreatePackage(*packageName);package->FullyLoad();
        const FString name=FPackageName::GetLongPackageAssetName(packageName);
        auto* mesh=FindObject<UStaticMesh>(package,*name);
        const bool reuse=mesh&&package->GetMetaData().GetValue(mesh,TEXT("DiveFallbackSource"))==key;
        if(FParse::Param(*Params,TEXT("VerifyOnly"))&&!reuse) return 1;
        FDynamicMesh3 original;FMeshDescriptionToDynamicMesh converter;
        converter.bTransformVertexColorsLinearToSRGB=false;converter.Convert(description,original,true);
        TArray<int> boundaryEdges;TSet<FVector3f> boundaryPositions;TSet<FBoundaryEdge> boundarySegments;
        for(int edge:original.EdgeIndicesItr()) if(original.IsBoundaryEdge(edge))
        {
            boundaryEdges.Add(edge);const auto pair=original.GetEdgeV(edge);
            boundaryPositions.Add(FVector3f(original.GetVertex(pair.A)));boundaryPositions.Add(FVector3f(original.GetVertex(pair.B)));
            boundarySegments.Add(BoundaryEdge(FVector3f(original.GetVertex(pair.A)),FVector3f(original.GetVertex(pair.B))));
        }
        if(!reuse)
        {
            FDynamicMesh3 reduced(original);FMeshConstraints constraints;
            FMeshConstraintsUtil::FullyConstrainEdges(constraints,reduced,boundaryEdges);
            FDynamicMeshAABBTree3 spatial(&original,true);FMeshProjectionTarget target(&original,&spatial);
            FQEMSimplification simplify(&reduced);
            simplify.SetExternalConstraints(MoveTemp(constraints));simplify.SetProjectionTarget(&target);
            simplify.GeometricErrorConstraint=FQEMSimplification::EGeometricErrorCriteria::PredictedPointToProjectionTarget;
            simplify.GeometricErrorTolerance=10.; // cm; collapse-point envelope, not a proven global surface bound.
            simplify.ProjectionMode=FQEMSimplification::ETargetProjectionMode::NoProjection;
            simplify.bRetainQuadricMemory=true;
            UE_LOG(LogTemp,Display,TEXT("DIVE_FALLBACK_REDUCE_START source=%s triangles=%d boundary_vertices=%d"),*source->GetName(),original.TriangleCount(),boundaryPositions.Num());
            simplify.SimplifyToTriangleCount(FMath::Max(128,original.TriangleCount()/10));
            for(int vertex:original.VertexIndicesItr()) if(original.IsBoundaryVertex(vertex))
                if(!reduced.IsVertex(vertex)||reduced.GetVertex(vertex)!=original.GetVertex(vertex)) return 1;
            FMeshDescription fallback;FStaticMeshAttributes(fallback).Register();FConversionToMeshDescriptionOptions options;
            options.bUpdateNormals=options.bUpdateUVs=options.bUpdateTangents=options.bUpdateVtxColors=true;
            options.bTransformVtxColorsSRGBToLinear=false;
            FDynamicMeshToMeshDescription output(options);output.Convert(&reduced,fallback,true);
            auto slots=FStaticMeshAttributes(fallback).GetPolygonGroupMaterialSlotNames();
            for(const auto group:fallback.PolygonGroups().GetElementIDs()) slots[group]=TEXT("Terrain");
            if(!mesh) mesh=NewObject<UStaticMesh>(package,*name,RF_Public|RF_Standalone);
            mesh->SetNumSourceModels(1);mesh->GetStaticMaterials()=source->GetStaticMaterials();
            mesh->CreateHiResMeshDescription(FMeshDescription(*description));mesh->CommitHiResMeshDescription();
            mesh->CreateMeshDescription(0,MoveTemp(fallback));mesh->CommitMeshDescription(0);
            mesh->GetSourceModel(0).BuildSettings=source->GetSourceModel(0).BuildSettings;
            mesh->GetHiResSourceModel().BuildSettings=source->GetSourceModel(0).BuildSettings;
            auto settings=source->GetNaniteSettings();settings.FallbackRelativeError=0;settings.FallbackPercentTriangles=1;
            mesh->SetNaniteSettings(settings);mesh->Build(false);FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
            if(!mesh->HasValidNaniteData()) return 1;
            package->GetMetaData().SetValue(mesh,TEXT("DiveFallbackSource"),*key);
        }
        FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
        const auto* render=mesh->GetRenderData();const auto* high=mesh->GetHiResMeshDescription();
        if(!render||render->LODResources.IsEmpty()||!high||high->Triangles().Num()!=description->Triangles().Num()||high->Vertices().Num()!=description->Vertices().Num()) return 1;
        const auto highPositions=FStaticMeshConstAttributes(*high).GetVertexPositions();
        const auto originalPositions=FStaticMeshConstAttributes(*description).GetVertexPositions();
        for(const auto vertex:description->Vertices().GetElementIDs()) if(highPositions[vertex]!=originalPositions[vertex]) return 1;
        TSet<FVector3f> renderedPositions;
        const auto& buffer=render->LODResources[0].VertexBuffers.PositionVertexBuffer;
        for(uint32 i=0;i<buffer.GetNumVertices();++i) renderedPositions.Add(buffer.VertexPosition(i));
        for(const auto& point:boundaryPositions) if(!renderedPositions.Contains(point))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_FALLBACK_BOUNDARY_FAIL mesh=%s point=%s"),*name,*point.ToString());return 1; }
        TMap<FBoundaryEdge,int> edgeCounts;const auto indices=render->LODResources[0].IndexBuffer.GetArrayView();
        for(int i=0;i<indices.Num();i+=3) for(int k=0;k<3;++k)
            ++edgeCounts.FindOrAdd(BoundaryEdge(buffer.VertexPosition(indices[i+k]),buffer.VertexPosition(indices[i+(k+1)%3])));
        for(const auto& edge:boundarySegments) if(edgeCounts.FindRef(edge)!=1) return 1;
        if(!LoadLandscapeFallbackStudy(source.Get())) return 1;
        if(!reuse)
        {
            FAssetRegistryModule::AssetCreated(mesh);package->MarkPackageDirty();
            FSavePackageArgs args;args.TopLevelFlags=RF_Public|RF_Standalone;args.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(package,mesh,*FPackageName::LongPackageNameToFilename(packageName,FPackageName::GetAssetPackageExtension()),args)) return 1;
        }
        UE_LOG(LogTemp,Display,TEXT("DIVE_FALLBACK_BOUNDARY_PASS mesh=%s boundary_vertices=%d boundary_edges=%d source_triangles=%d fallback_triangles=%d reused=%d"),
            *name,boundaryPositions.Num(),boundarySegments.Num(),description->Triangles().Num(),mesh->GetNumTriangles(0),reuse);
    }
    return 0;
}
namespace
{
FString ResidentPackage(const UStaticMesh* Source)
{ return TEXT("/Game/Diving/LandscapeStudy/Resident_")+Source->GetName(); }
bool FullyResident(const UStaticMesh* Mesh)
{
    const auto* data=Mesh?Mesh->GetRenderData():nullptr;
    const auto* n=data?data->NaniteResourcesPtr.Get():nullptr;
    return n&&n->NumRootPages>0&&n->NumRootPages==static_cast<uint32>(n->PageStreamingStates.Num())&&n->StreamablePages.GetBulkDataSize()==0;
}
}

UStaticMesh* LoadLandscapeResidentStudy(UStaticMesh* Source)
{
    if(!Source) return nullptr;
    const FString package=ResidentPackage(Source),key=FallbackStudyKey(Source)+TEXT(":resident-v1");
    auto* mesh=LoadObject<UStaticMesh>(nullptr,*(package+TEXT(".")+FPackageName::GetLongPackageAssetName(package)));
    if(!mesh||!FullyResident(mesh)||mesh->GetPackage()->GetMetaData().GetValue(mesh,TEXT("DiveResidentSource"))!=key) return nullptr;
    return mesh;
}

int32 BakeLandscapeResidentStudy(const FString& Params)
{
    for(const TCHAR* name:{TEXT("Boundary_SM_Land_04_02"),TEXT("Boundary_SM_LandDetail_05_02")})
    {
        const FString path=TEXT("/Game/Diving/LandscapeStudy/")+FString(name)+TEXT(".")+name;
        TStrongObjectPtr<UStaticMesh> source(LoadObject<UStaticMesh>(nullptr,*path));
        if(!source||!source->IsHiResMeshDescriptionValid()) return 1;
        const FString packageName=ResidentPackage(source.Get()),key=FallbackStudyKey(source.Get())+TEXT(":resident-v1");
        UPackage* package=CreatePackage(*packageName);package->FullyLoad();
        const FString assetName=FPackageName::GetLongPackageAssetName(packageName);
        auto* mesh=FindObject<UStaticMesh>(package,*assetName);
        const bool reuse=mesh&&package->GetMetaData().GetValue(mesh,TEXT("DiveResidentSource"))==key;
        if(FParse::Param(*Params,TEXT("VerifyOnly"))&&!reuse) return 1;
        if(!reuse)
        {
            if(mesh) return 1; // Preserve stale diagnostic assets for review rather than overwriting.
            mesh=DuplicateObject<UStaticMesh>(source.Get(),package,*assetName);
            // Duplication does not retain CachedMeshDescriptionBounds. Commit
            // the unchanged LOD0 to restore the same source bounds as the control.
            if(!mesh->GetMeshDescription(0)) return 1;
            mesh->CommitMeshDescription(0);
            auto settings=mesh->GetNaniteSettings();settings.TargetMinimumResidencyInKB=MAX_uint32;
            mesh->SetNaniteSettings(settings);mesh->Build(false);
        }
        FStaticMeshCompilingManager::Get().FinishCompilation({mesh});
        if(!FullyResident(mesh)) return 1;
        if(!mesh->GetBounds().Origin.Equals(source->GetBounds().Origin,.01)||
           !mesh->GetBounds().BoxExtent.Equals(source->GetBounds().BoxExtent,.01)) return 1;
        for(int level=0;level<2;++level)
        {
            const auto* a=level?source->GetHiResMeshDescription():source->GetMeshDescription(0);
            const auto* b=level?mesh->GetHiResMeshDescription():mesh->GetMeshDescription(0);
            if(!a||!b||a->Vertices().Num()!=b->Vertices().Num()||a->Triangles().Num()!=b->Triangles().Num()) return 1;
            const auto pa=FStaticMeshConstAttributes(*a).GetVertexPositions(),pb=FStaticMeshConstAttributes(*b).GetVertexPositions();
            for(const auto v:a->Vertices().GetElementIDs()) if(pa[v]!=pb[v]) return 1;
        }
        const auto& old=*source->GetRenderData()->NaniteResourcesPtr;
        const auto& n=*mesh->GetRenderData()->NaniteResourcesPtr;
        UE_LOG(LogTemp,Display,TEXT("DIVE_RESIDENT_BOUNDS mesh=%s source=%s clone=%s source_render=%s clone_render=%s total_pages=%d"),
            *assetName,*source->GetBounds().BoxExtent.ToString(),*mesh->GetBounds().BoxExtent.ToString(),
            *source->GetRenderData()->Bounds.BoxExtent.ToString(),*mesh->GetRenderData()->Bounds.BoxExtent.ToString(),n.PageStreamingStates.Num());
        if(!reuse)
        {
            package->GetMetaData().SetValue(mesh,TEXT("DiveResidentSource"),*key);
            FAssetRegistryModule::AssetCreated(mesh);package->MarkPackageDirty();
            FSavePackageArgs args;args.TopLevelFlags=RF_Public|RF_Standalone;args.SaveFlags=SAVE_NoError;
            if(!UPackage::SavePackage(package,mesh,*FPackageName::LongPackageNameToFilename(packageName,FPackageName::GetAssetPackageExtension()),args)) return 1;
        }
        UE_LOG(LogTemp,Display,TEXT("DIVE_RESIDENT_BAKE_PASS mesh=%s old_root_pages=%u old_root_bytes=%d old_stream_bytes=%lld root_pages=%u root_bytes=%d stream_bytes=%lld source_positions_unchanged=1 reused=%d"),
            *assetName,old.NumRootPages,old.RootData.Num(),old.StreamablePages.GetBulkDataSize(),n.NumRootPages,n.RootData.Num(),n.StreamablePages.GetBulkDataSize(),reuse);
    }
    return 0;
}
#endif
