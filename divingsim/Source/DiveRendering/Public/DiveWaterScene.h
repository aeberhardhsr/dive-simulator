#pragma once
#include "CoreMinimal.h"

class FSceneInterface;
class FTextureResource;
struct FDiveWaterSceneInput
{
    FVector4f Waves[4]={FVector4f(0,0,0,0),FVector4f(0,0,0,0),FVector4f(0,0,0,0),FVector4f(0,0,0,0)};
    FVector4f Phases=FVector4f(0,0,0,0);
    // seconds, wind strength, scalar extinction per metre, caustic strength
    FVector4f Medium=FVector4f(0,0,0,0);
    FTextureResource* FootprintResource=nullptr;
    FVector4f FootprintMap=FVector4f(0,0,0,0);
    TSharedPtr<const TArray<FVector4f>,ESPMode::ThreadSafe> FootprintProbes;
};

DIVERENDERING_API bool DiveWaveTransportEnabled();
DIVERENDERING_API void PublishDiveWaterScene(FSceneInterface* Scene,const FDiveWaterSceneInput& Input);
void InitializeDiveWaterScene();
