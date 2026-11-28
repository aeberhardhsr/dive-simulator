#include "DiveLandscapePilot.h"
#include "DiveTerrain.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Rendering/NaniteResources.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformProperties.h"
#include "Serialization/JsonSerializer.h"
#if WITH_EDITOR
#include "DiveLandscapeDetail.h"
#include "StaticMeshCompiler.h"
#include "HAL/FileManager.h"
#include "Misc/Parse.h"
#endif

namespace
{
const TCHAR* WestPath=TEXT("/Game/Diving/LandscapePilot/SM_Land_04_02.SM_Land_04_02");
const TCHAR* EastPath=TEXT("/Game/Diving/LandscapePilot/SM_LandDetail_05_02.SM_LandDetail_05_02");
constexpr uint32 MaxPilotRootPages=2048; // 64MiB at the engine's 32KiB root-page size.

bool ReadJson(const FString& Path,TSharedPtr<FJsonObject>& Json)
{
    FString text;
    return FFileHelper::LoadFileToString(text,*Path)&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(text),Json)&&Json.IsValid();
}
bool Number(const FJsonObject& Json,const TCHAR* Key,double Expected)
{ double value=0;return Json.TryGetNumberField(Key,value)&&FMath::IsFinite(value)&&value==Expected; }
bool Hash(const FJsonObject& Json,const TCHAR* Key,const FString& Path,TArray<uint8>& Bytes)
{
    FString expected;
    return Json.TryGetStringField(Key,expected)&&expected.Len()==40&&FFileHelper::LoadFileToArray(Bytes,*Path)&&
        FSHA1::HashBuffer(Bytes.GetData(),Bytes.Num()).ToString().Equals(expected,ESearchCase::IgnoreCase);
}
bool CheckMesh(UStaticMesh* Mesh,const FJsonObject& Json,const TCHAR* Prefix,uint32& Pages)
{
#if WITH_EDITOR
    if(Mesh) FStaticMeshCompilingManager::Get().FinishCompilation({Mesh});
#endif
    const auto* data=Mesh?Mesh->GetRenderData():nullptr;
    const auto* nanite=data?data->NaniteResourcesPtr.Get():nullptr;
    if(!nanite||!Mesh->HasValidNaniteData()||nanite->NumRootPages==0||
       nanite->NumRootPages!=static_cast<uint32>(nanite->PageStreamingStates.Num())||
       nanite->StreamablePages.GetBulkDataSize()!=0||
       !Number(Json,*(FString(Prefix)+TEXT("_root_pages")),nanite->NumRootPages)||
       !Number(Json,*(FString(Prefix)+TEXT("_fallback_triangles")),Mesh->GetNumTriangles(0))) return false;
    const auto bounds=Mesh->GetBounds();
    if(bounds.BoxExtent.ContainsNaN()||bounds.BoxExtent.X>102401||bounds.BoxExtent.Y>102401||
       bounds.BoxExtent.X<=0||bounds.BoxExtent.Y<=0) return false;
    Pages+=nanite->NumRootPages;
    return Pages<=MaxPilotRootPages;
}
}

bool LoadLandscapePilot(FDiveTerrain& Terrain,UStaticMesh*& West,UStaticMesh*& East,FString& Error)
{
    West=East=nullptr;
    const FString geo=FPaths::ProjectContentDir()/TEXT("Diving/Geodata"),folder=geo/TEXT("LandscapePilot");
    TSharedPtr<FJsonObject> json;
    auto fail=[&](const TCHAR* reason){Error=reason;return false;};
    if(!ReadJson(folder/TEXT("pilot.json"),json)) return fail(TEXT("Landscape pilot manifest missing"));
    const double east=Terrain.Landscape.West+(1280+.5)*8,north=Terrain.Landscape.North-(512+.5)*8;
    const FVector origin=Terrain.ToWorld(east,north);
    if(!Number(*json,TEXT("schema"),2)||!Number(*json,TEXT("width"),1025)||!Number(*json,TEXT("height"),1025)||
       !Number(*json,TEXT("column"),1280)||!Number(*json,TEXT("row"),512)||!Number(*json,TEXT("spacing_m"),2)||
       !Number(*json,TEXT("blend_border_m"),32)||!Number(*json,TEXT("west_vertex_east"),east)||
       !Number(*json,TEXT("north_vertex_north"),north)||!Number(*json,TEXT("origin_x_cm"),origin.X)||
       !Number(*json,TEXT("origin_y_cm"),origin.Y)||Terrain.Landscape.Spacing!=8)
        return fail(TEXT("Landscape pilot registration invalid"));
    TArray<uint8> bytes;
    if(!Hash(*json,TEXT("base_grid_sha1"),geo/TEXT("landscape.f32"),bytes)||
       !Hash(*json,TEXT("native_sha1"),folder/TEXT("detail_05_02.f32"),bytes)||bytes.Num()!=1025*1025*sizeof(float))
        return fail(TEXT("Landscape pilot ground identity invalid"));
    Dive::SurfaceGrid ground;ground.Columns=ground.Rows=1025;ground.Spacing=2;ground.West=east-1;ground.North=north+1;
    ground.Heights.resize(1025*1025);FMemory::Memcpy(ground.Heights.data(),bytes.GetData(),bytes.Num());
    for(float h:ground.Heights) if(!FMath::IsFinite(h)||h<80||h>3000) return fail(TEXT("Landscape pilot height invalid"));
    // Only the final assets are loaded here. Source/study meshes and editor
    // metadata are deliberately not prerequisites for a cooked executable.
    auto* west=LoadObject<UStaticMesh>(nullptr,WestPath);auto* fine=LoadObject<UStaticMesh>(nullptr,EastPath);
    uint32 pages=0;
    if(!CheckMesh(west,*json,TEXT("west"),pages)||!CheckMesh(fine,*json,TEXT("fine"),pages))
        return fail(TEXT("Landscape pilot mesh residency or budget invalid"));
    Terrain.LandscapeDetail=MoveTemp(ground);West=west;East=fine;Error.Empty();
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_PILOT_READY tiles=2 spacing=2m root_pages=%u budget_pages=%u cooked=%d"),pages,MaxPilotRootPages,FPlatformProperties::RequiresCookedData());
    return true;
}

#if WITH_EDITOR
int32 BakeLandscapePilot(FDiveTerrain& Terrain,const FString& Params)
{
    FString error;
    auto* detail=LoadLandscapeDetailStudy(Terrain,error);
    auto* coarse=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Diving/Landscape/SM_Land_04_02.SM_Land_04_02"));
    if(!detail||!LoadLandscapeFallbackStudy(detail)||!LoadLandscapeFallbackStudy(coarse))
    { UE_LOG(LogTemp,Error,TEXT("Landscape pilot source chain stale: %s"),*error);return 1; }
    if(BakeLandscapeResidentStudy(Params)!=0) return 1;
    const FString geo=FPaths::ProjectContentDir()/TEXT("Diving/Geodata"),folder=geo/TEXT("LandscapePilot");
    if(!FParse::Param(*Params,TEXT("VerifyOnly")))
    {
        TSharedPtr<FJsonObject> json;
        if(!ReadJson(geo/TEXT("LandscapeStudy/detail_study.json"),json)) return 1;
        json->SetNumberField(TEXT("schema"),2);json->SetStringField(TEXT("asset"),EastPath);
        json->SetStringField(TEXT("limits"),TEXT("Bounded two-tile inland pilot; unchanged coast/bathymetry. Runtime ground matches the fine source triangles. All other terrain remains at existing resolution."));
        for(const auto& item:{TPair<const TCHAR*,const TCHAR*>(TEXT("west"),WestPath),TPair<const TCHAR*,const TCHAR*>(TEXT("fine"),EastPath)})
        {
            auto* mesh=LoadObject<UStaticMesh>(nullptr,item.Value);
            if(!mesh||!mesh->GetRenderData()||!mesh->GetRenderData()->NaniteResourcesPtr) return 1;
            json->SetNumberField(FString(item.Key)+TEXT("_root_pages"),mesh->GetRenderData()->NaniteResourcesPtr->NumRootPages);
            json->SetNumberField(FString(item.Key)+TEXT("_fallback_triangles"),mesh->GetNumTriangles(0));
        }
        TArray<uint8> bytes;
        if(!FFileHelper::LoadFileToArray(bytes,*(geo/TEXT("LandscapeStudy/detail_05_02.f32")))) return 1;
        IFileManager::Get().MakeDirectory(*folder,true);
        FString text;FJsonSerializer::Serialize(json.ToSharedRef(),TJsonWriterFactory<>::Create(&text));
        if(!FFileHelper::SaveArrayToFile(bytes,*(folder/TEXT("detail_05_02.f32")))||
           !FFileHelper::SaveStringToFile(text,*(folder/TEXT("pilot.json")))) return 1;
    }
    UStaticMesh* west=nullptr;UStaticMesh* east=nullptr;
    if(!LoadLandscapePilot(Terrain,west,east,error)) { UE_LOG(LogTemp,Error,TEXT("%s"),*error);return 1; }
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAND_PILOT_BAKE_PASS"));return 0;
}
#endif
