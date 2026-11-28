#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
class FDiveTerrain;
class UStaticMesh;
int32 BakeLandscapeDetailStudy(FDiveTerrain& Terrain,const FString& Params);
UStaticMesh* LoadLandscapeDetailStudy(FDiveTerrain& Terrain,FString& Error);
#endif
