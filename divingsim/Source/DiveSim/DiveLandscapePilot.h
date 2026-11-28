#pragma once
#include "CoreMinimal.h"
class FDiveTerrain;
class UStaticMesh;
bool LoadLandscapePilot(FDiveTerrain& Terrain,UStaticMesh*& West,UStaticMesh*& East,FString& Error);
#if WITH_EDITOR
int32 BakeLandscapePilot(FDiveTerrain& Terrain,const FString& Params);
#endif
