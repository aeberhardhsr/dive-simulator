#include "DiveWorld.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#if WITH_EDITOR
#include "Engine/Texture2D.h"
#include "TextureCompiler.h"
#endif

void ADiveWorld::CreateLandscape()
{
    FString json; TSharedPtr<FJsonObject> manifest;
    const TArray<TSharedPtr<FJsonValue>>* tiles=nullptr; double schema=0,spacing=0,precision=0;
    const auto& grid=Terrain.Landscape;
    const int expected=FMath::DivideAndRoundUp(grid.Columns-1,256)*FMath::DivideAndRoundUp(grid.Rows-1,256);
    auto fail=[this](const TCHAR* reason)
    {
        bTerrainReady=false; Terrain.Error=reason;
        UE_LOG(LogTemp,Error,TEXT("DIVE_LANDSCAPE_FAIL %s"),reason);
    };
    if(!FFileHelper::LoadFileToString(json,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/landscape_meshes.json")))||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),manifest)||!manifest||
        !manifest->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
        !manifest->TryGetNumberField(TEXT("nanite_position_precision"),precision)||precision!=FDiveTerrain::NanitePositionPrecision||
        !manifest->TryGetNumberField(TEXT("spacing_m"),spacing)||spacing!=grid.Spacing||
        !manifest->TryGetArrayField(TEXT("tiles"),tiles)||tiles->Num()!=expected)
    { fail(TEXT("Landschaftsmeshes fehlen: Tools/Build.ps1 -Target Landscape.")); return; }
    const TCHAR* landscapeMaterial=TEXT("M_LandscapePhoto");
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveFarCliffStudy")))
        landscapeMaterial=TEXT("M_LandscapeCliffStudy");
#endif
    auto* material=ImageryMaterial(landscapeMaterial,Terrain.LandscapeImagery);
    if(!material) return;
    UE_LOG(LogTemp,Display,TEXT("DIVE_LANDSCAPE_MATERIAL name=%s"),landscapeMaterial);
#if WITH_EDITOR
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveFarCliffStudy")))
    {
        auto* coverage=Cast<UTexture2D>(material->K2_GetTextureParameterValue(TEXT("CliffCoverage")));
        if(coverage) FTextureCompilingManager::Get().FinishCompilation({coverage});
        if(!coverage||coverage->GetSizeX()!=4096||coverage->GetSizeY()!=4096||coverage->SRGB||!coverage->NeverStream||
            coverage->CompressionSettings!=TC_VectorDisplacementmap||coverage->MipGenSettings!=TMGS_SimpleAverage||
            coverage->Filter!=TF_Trilinear||coverage->AddressX!=TA_Clamp||coverage->AddressY!=TA_Clamp||
            coverage->GetNumMips()!=13||coverage->GetStreamableResourceState().NumResidentLODs!=13)
        { fail(TEXT("Cliff study requires resident linear decoded coverage.")); return; }
        UE_LOG(LogTemp,Display,TEXT("DIVE_CLIFF_COVERAGE_PASS texture=%s size=4096 mips=13 bytes=%u"),
            *coverage->GetName(),coverage->CalcTextureMemorySizeEnum(TMC_AllMips));
    }
#endif
    TSet<FIntPoint> visited;
    int naniteAssets=0;
    for(const auto& item:*tiles)
    {
        const auto tile=item->AsObject();
        double col=0,row=0,lastCol=0,lastRow=0,x=0,y=0; FString path;
        if(!tile||!tile->TryGetNumberField(TEXT("column"),col)||!tile->TryGetNumberField(TEXT("row"),row)||
            !tile->TryGetNumberField(TEXT("last_column"),lastCol)||!tile->TryGetNumberField(TEXT("last_row"),lastRow)||
            !tile->TryGetNumberField(TEXT("origin_x_cm"),x)||!tile->TryGetNumberField(TEXT("origin_y_cm"),y)||
            !tile->TryGetStringField(TEXT("asset"),path)||!FMath::IsFinite(col)||!FMath::IsFinite(row)||
            !FMath::IsFinite(x)||!FMath::IsFinite(y)||col<0||row<0||col>=grid.Columns-1||row>=grid.Rows-1||
            FMath::Fmod(col,256.)!=0||FMath::Fmod(row,256.)!=0||
            lastCol!=FMath::Min(col+256,static_cast<double>(grid.Columns-1))||
            lastRow!=FMath::Min(row+256,static_cast<double>(grid.Rows-1)))
        { fail(TEXT("Ungueltige Landschaftskachel.")); return; }
        const FIntPoint key(static_cast<int>(col),static_cast<int>(row));
        const FString name=FString::Printf(TEXT("SM_Land_%02d_%02d"),key.X/256,key.Y/256);
        const FVector origin=Terrain.ToWorld(grid.West+(col+.5)*grid.Spacing,grid.North-(row+.5)*grid.Spacing);
        if(visited.Contains(key)||path!=TEXT("/Game/Diving/Landscape/")+name+TEXT(".")+name||
            FMath::Abs(x-origin.X)>.01||FMath::Abs(y-origin.Y)>.01)
        { fail(TEXT("Landschaftskachel falsch registriert.")); return; }
        visited.Add(key);
        auto* mesh=LoadObject<UStaticMesh>(nullptr,*path);
        if(!mesh||mesh->GetNumTriangles(0)<=0) { fail(TEXT("Landschaftsasset oder Fallback-Geometrie fehlt.")); return; }
        naniteAssets+=mesh->HasValidNaniteData()?1:0;
        auto* component=NewObject<UStaticMeshComponent>(this,*name);
        component->SetupAttachment(RootComponent); component->SetMobility(EComponentMobility::Static);
        component->SetRelativeLocation(FVector(x,y,0)); component->SetStaticMesh(mesh);
        component->SetCollisionEnabled(ECollisionEnabled::NoCollision); component->SetMaterial(0,material);
        component->RegisterComponent(); LandscapeMeshes.Add(component);
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_LANDSCAPE_READY tiles=%d source_spacing=8m nanite_assets=%d"),LandscapeMeshes.Num(),naniteAssets);
}

bool ADiveWorld::ValidateLandscape(FString& Error) const
{
    const auto& grid=Terrain.Landscape;
    const int expected=FMath::DivideAndRoundUp(grid.Columns-1,256)*FMath::DivideAndRoundUp(grid.Rows-1,256);
    if(LandscapeMeshes.Num()!=expected) { Error=TEXT("Landscape tile coverage incomplete"); return false; }
    int64 fallbackTriangles=0; int naniteAssets=0;
    for(const auto& item:LandscapeMeshes)
    {
        const auto* component=item.Get();
        const UStaticMesh* mesh=component?component->GetStaticMesh().Get():nullptr;
        // Unsupported target platforms can cook only the conventional fallback.
        // The editor build gate separately requires valid Nanite source assets.
        if(!mesh||mesh->GetNumTriangles(0)<=0||
            component->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||
            !component->GetMaterial(0)||component->GetComponentScale()!=FVector::OneVector)
        { Error=TEXT("Landscape asset/render state invalid"); return false; }
        const auto bounds=mesh->GetBounds();
        if(bounds.BoxExtent.ContainsNaN()||bounds.BoxExtent.X>102401||bounds.BoxExtent.Y>102401||
            bounds.BoxExtent.X<=0||bounds.BoxExtent.Y<=0)
        { Error=TEXT("Landscape tile bounds invalid"); return false; }
        fallbackTriangles+=mesh->GetNumTriangles(0);
        naniteAssets+=mesh->HasValidNaniteData()?1:0;
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_LANDSCAPE_PASS tiles=%d nanite_assets=%d fallback_triangles=%lld"),LandscapeMeshes.Num(),naniteAssets,fallbackTriangles);
    Error.Empty(); return true;
}
