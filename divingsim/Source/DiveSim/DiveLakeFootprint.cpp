#include "DiveWorld.h"
#include "DiveWaterScene.h"
#include "Engine/Texture2D.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

void ADiveWorld::LoadLakeFootprint()
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveLakeFootprint"))) return;
    if(!DiveWaveTransportEnabled())
        UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL requires explicit wave transport study"));
    // This pinned raster has one-metre cells, north-to-south rows and three island holes.
    // A different survey/grid requires a new offline validation and updated mapping.
    constexpr int32 Width=15586,Height=3003;
    TArray<uint8> bytes;
    const FString path=FPaths::ProjectContentDir()/TEXT("Diving/Geodata/walensee-distance.f16");
    if(!FFileHelper::LoadFileToArray(bytes,*path)||bytes.Num()!=Width*Height*2)
        UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL missing or invalid raster: %s"),*path);
    FSHAHash hash;FSHA1::HashBuffer(bytes.GetData(),bytes.Num(),hash.Hash);
    if(!hash.ToString().Equals(TEXT("347b45bdce8f0ba71ac9646b0d533f1f9fa87155"),ESearchCase::IgnoreCase)||
        Terrain.OriginEast!=2726000||Terrain.OriginNorth!=1219000)
        UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL raster identity or coordinate origin changed"));
    LakeFootprint=UTexture2D::CreateTransient(Width,Height,PF_R16F,TEXT("WalenseeSignedShoreDistance"),bytes);
    if(!LakeFootprint) UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL texture allocation"));
    LakeFootprint->SRGB=false;LakeFootprint->NeverStream=true;
    LakeFootprint->Filter=TF_Bilinear;LakeFootprint->AddressX=TA_Clamp;LakeFootprint->AddressY=TA_Clamp;
    LakeFootprint->UpdateResource();
    LakeFootprintMap=FVector4f(-129.f,3350.f,1.f/Width,1.f/Height);
    FString probePath;
    if(FParse::Value(FCommandLine::Get(),TEXT("DiveLakeFootprintProbeFile="),probePath))
    {
        TArray<uint8> probeBytes;
        if(!FFileHelper::LoadFileToArray(probeBytes,*probePath)||probeBytes.Num()<16||probeBytes.Num()%16||probeBytes.Num()>1600000)
            UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL probe data"));
        auto probes=MakeShared<TArray<FVector4f>,ESPMode::ThreadSafe>();
        probes->SetNumUninitialized(probeBytes.Num()/16);
        FMemory::Memcpy(probes->GetData(),probeBytes.GetData(),probeBytes.Num());
        for(const auto& p:*probes)
            if(p.ContainsNaN()) UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_FOOTPRINT_FAIL nonfinite probe"));
        LakeFootprintProbes=probes;
    }
    UE_LOG(LogTemp,Display,TEXT("DIVE_LAKE_FOOTPRINT_READY width=%d height=%d format=R16F srgb=0 mips=1 bytes=%d probes=%d"),
        Width,Height,bytes.Num(),LakeFootprintProbes?LakeFootprintProbes->Num():0);
}
