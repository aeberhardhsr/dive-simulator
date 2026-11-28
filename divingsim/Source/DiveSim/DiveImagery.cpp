#include "DiveTerrain.h"
#include "DiveWorld.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#if WITH_EDITOR
#include "TextureCompiler.h"
#endif

bool FDiveTerrain::LoadImagery()
{
    FString json; TSharedPtr<FJsonObject> root; const TArray<TSharedPtr<FJsonValue>>* images=nullptr;
    double schema=0; FString crs;
    if(!FFileHelper::LoadFileToString(json,*(FPaths::ProjectContentDir()/TEXT("Diving/Geodata/imagery.json")))||
        !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(json),root)||!root||
        !root->TryGetNumberField(TEXT("schema"),schema)||schema!=1||
        !root->TryGetStringField(TEXT("horizontal_crs"),crs)||crs!=TEXT("EPSG:2056")||
        !root->TryGetArrayField(TEXT("images"),images)||images->Num()!=4)
    { Error=TEXT("Luftbildmanifest fehlt oder ist ungueltig."); return false; }
    ShoreImagery.Init(FString(),ShoreEntries.Num()); LandscapeImagery.Empty();
    for(const auto& value:*images)
    {
        const auto image=value->AsObject(); FString id,asset;
        double west=0,south=0,east=0,north=0,width=0,height=0;
        if(!image||!image->TryGetStringField(TEXT("id"),id)||!image->TryGetStringField(TEXT("asset"),asset)||
            asset!=TEXT("T_Ortho_")+id||!image->TryGetNumberField(TEXT("west"),west)||
            !image->TryGetNumberField(TEXT("south"),south)||!image->TryGetNumberField(TEXT("east"),east)||
            !image->TryGetNumberField(TEXT("north"),north)||!image->TryGetNumberField(TEXT("width"),width)||
            !image->TryGetNumberField(TEXT("height"),height)||!FMath::IsFinite(west)||!FMath::IsFinite(south)||
            !FMath::IsFinite(east)||!FMath::IsFinite(north))
        { Error=TEXT("Ungueltige Luftbildreferenz."); return false; }
        const int index=ShoreEntries.IndexOfByPredicate([&](const FDiveEntry& e){return e.Id==id;});
        if(id!=TEXT("landscape")&&index==INDEX_NONE) { Error=TEXT("Unbekannter Luftbildbereich."); return false; }
        const auto& grid=index==INDEX_NONE?Landscape:Shores[index];
        FString& target=index==INDEX_NONE?LandscapeImagery:ShoreImagery[index];
        if(!target.IsEmpty()||width!=(index==INDEX_NONE?4096:2048)||height!=2048||
            FMath::Abs(west-grid.West)>.001||FMath::Abs(north-grid.North)>.001||
            FMath::Abs(east-grid.West-grid.Columns*grid.Spacing)>.001||
            FMath::Abs(south-grid.North+grid.Rows*grid.Spacing)>.001)
        { Error=TEXT("Luftbild und Gelaende haben unterschiedliche Ausdehnungen."); return false; }
        target=TEXT("/Game/Diving/Textures/")+asset+TEXT(".")+asset;
    }
    if(LandscapeImagery.IsEmpty()||ShoreImagery.Contains(FString()))
    { Error=TEXT("Luftbildbereiche unvollstaendig."); return false; }
    UE_LOG(LogTemp,Display,TEXT("DIVE_IMAGERY_READY images=4 crs=EPSG2056 bounds=matched"));
    return true;
}

UMaterialInstanceDynamic* ADiveWorld::ImageryMaterial(const TCHAR* Base,const FString& TexturePath)
{
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveNaturalGround")))
    {
        if(FCString::Strcmp(Base,TEXT("M_LandscapePhoto"))==0) Base=TEXT("M_LandscapeNatural");
        else if(FCString::Strcmp(Base,TEXT("M_ShorePhoto"))==0) Base=TEXT("M_ShoreNatural");
        UE_LOG(LogTemp,Display,TEXT("DIVE_NATURAL_GROUND_APPLIED material=%s"),Base);
    }
    const FString path=FString(TEXT("/Game/Diving/Materials/"))+Base+TEXT(".")+Base;
    auto* material=LoadObject<UMaterialInterface>(nullptr,*path);
    auto* texture=LoadObject<UTexture2D>(nullptr,*TexturePath);
    if(!material||!texture)
    {
        bTerrainReady=false; Terrain.Error=TEXT("Luftbild-Assets fehlen: Tools/Build.ps1 -Target Content -OnlyImagery.");
        UE_LOG(LogTemp,Error,TEXT("DIVE_IMAGERY_FAIL missing %s or %s"),Base,*TexturePath); return nullptr;
    }
#if WITH_EDITOR
    // Editor -game may otherwise expose the 32x32 async-compilation placeholder
    // while the Nanite tiles are being loaded. Cooked textures are already built.
    FTextureCompilingManager::Get().FinishCompilation({texture});
#endif
    auto* instance=UMaterialInstanceDynamic::Create(material,this);
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveGroundOff"))) instance->SetScalarParameterValue(TEXT("GroundDetailStrength"),0);
#if WITH_EDITOR
    TArray<UTexture*> groundTextures;
    for(const TCHAR* suffix:{TEXT("Woodland"),TEXT("GrassColor"),TEXT("GrassNormal"),TEXT("GrassARM"),TEXT("ForestColor"),TEXT("ForestNormal"),TEXT("ForestARM")})
        if(auto* ground=instance->K2_GetTextureParameterValue(FName(FString(TEXT("Ground"))+suffix))) groundTextures.Add(ground);
    FTextureCompilingManager::Get().FinishCompilation(groundTextures);
#endif
    instance->SetTextureParameterValue(TEXT("Ortho"),texture); ImageryMaterials.Add(instance);
    return instance;
}

bool ADiveWorld::ValidateImagery(FString& Error) const
{
    if(ImageryMaterials.Num()!=5) { Error=TEXT("Expected land, three entry and coast imagery materials"); return false; }
    for(int i=0;i<5;++i)
    {
        auto* material=ImageryMaterials[i].Get();
        const auto* texture=material?Cast<UTexture2D>(material->K2_GetTextureParameterValue(TEXT("Ortho"))):nullptr;
        const bool regional=i==0||i==4;
        const FString expected=regional?Terrain.LandscapeImagery:Terrain.ShoreImagery[i-1];
        if(!texture||texture->GetPathName()!=expected||texture->GetSizeX()!=(regional?4096:2048)||texture->GetSizeY()!=2048||
            texture->GetNumMips()<10||texture->GetStreamableResourceState().NumResidentLODs!=texture->GetNumMips()||
            !texture->SRGB||!texture->NeverStream||texture->AddressX!=TA_Clamp||texture->AddressY!=TA_Clamp)
        {
            Error=texture?FString::Printf(TEXT("Imagery[%d] %s size=%dx%d mips=%d resident=%d srgb=%d neverstream=%d address=%d/%d"),
                i,*texture->GetName(),texture->GetSizeX(),texture->GetSizeY(),texture->GetNumMips(),
                texture->GetStreamableResourceState().NumResidentLODs,texture->SRGB,texture->NeverStream,
                static_cast<int>(texture->AddressX),static_cast<int>(texture->AddressY)):
                FString::Printf(TEXT("Imagery[%d] texture parameter missing"),i);
            return false;
        }
        UE_LOG(LogTemp,Display,TEXT("DIVE_IMAGERY_TEXTURE %s size=%dx%d mips=%d resident=%d bytes=%u"),
            *texture->GetName(),texture->GetSizeX(),texture->GetSizeY(),texture->GetNumMips(),
            texture->GetStreamableResourceState().NumResidentLODs,texture->CalcTextureMemorySizeEnum(TMC_AllMips));
    }
    const bool groundOff=FParse::Param(FCommandLine::Get(),TEXT("DiveGroundOff"));
    uint64 groundBytes=0;
    for(int i=0;i<ImageryMaterials.Num();++i)
    {
        auto* material=ImageryMaterials[i].Get();
        if(!FMath::IsNearlyEqual(material->K2_GetScalarParameterValue(TEXT("GroundDetailStrength")),groundOff?0.f:1.f))
        { Error=TEXT("Ground detail material strength mismatch"); return false; }
        for(const TCHAR* suffix:{TEXT("Woodland"),TEXT("GrassColor"),TEXT("GrassNormal"),TEXT("GrassARM"),TEXT("ForestColor"),TEXT("ForestNormal"),TEXT("ForestARM")})
        {
            const FString name(suffix);const bool mask=name==TEXT("Woodland"),color=name.EndsWith(TEXT("Color")),normal=name.EndsWith(TEXT("Normal"));
            const auto* texture=Cast<UTexture2D>(material->K2_GetTextureParameterValue(FName(FString(TEXT("Ground"))+name)));
            const auto compression=normal?TC_Normalmap:color?TC_Default:TC_Masks;
            const int size=mask?4096:2048;
            if(!texture||texture->GetName()!=TEXT("T_Ground_")+name||texture->GetSizeX()!=size||texture->GetSizeY()!=size||
                texture->GetNumMips()<12||texture->GetStreamableResourceState().NumResidentLODs!=texture->GetNumMips()||
                texture->SRGB!=color||!texture->NeverStream||texture->CompressionSettings!=compression||
                texture->AddressX!=(mask?TA_Clamp:TA_Wrap)||texture->AddressY!=(mask?TA_Clamp:TA_Wrap))
            { Error=FString::Printf(TEXT("Ground detail texture invalid: material=%d %s"),i,*name); return false; }
            if(i==0) groundBytes+=texture->CalcTextureMemorySizeEnum(TMC_AllMips);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_GROUND_PASS materials=5 textures=7 enabled=%d resident_bytes=%llu"),!groundOff,groundBytes);
    // UVs must be north-up and tied to the same LV95 extents as the images.
    auto registered=[this](UProceduralMeshComponent* mesh,const Dive::SurfaceGrid& grid)
    {
        const auto* section=mesh->GetProcMeshSection(0); if(!section) return false;
        for(int i=0;i<section->ProcVertexBuffer.Num();i+=97)
        {
            const auto& vertex=section->ProcVertexBuffer[i];
            const FVector2D survey=Terrain.ToSurvey(mesh->GetComponentTransform().TransformPosition(vertex.Position));
            const FVector2D expected((survey.X-grid.West)/(grid.Columns*grid.Spacing),
                                     (grid.North-survey.Y)/(grid.Rows*grid.Spacing));
            if(!vertex.UV0.Equals(expected,1.e-5)) return false;
        }
        return true;
    };
    if(!ValidateLandscape(Error)) return false;
    if(!ValidateBuildings(Error)) return false;
    for(int i=0;i<3;++i) if(!registered(ShoreMeshes[i],Terrain.Shores[i]))
    { Error=TEXT("Shore UV registration mismatch"); return false; }
    Error.Empty(); return true;
}
