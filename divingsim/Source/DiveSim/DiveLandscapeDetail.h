#pragma once
#include "CoreMinimal.h"
#if WITH_EDITOR
class FDiveTerrain;
class UStaticMesh;
int32 BakeLandscapeDetailStudy(FDiveTerrain& Terrain,const FString& Params);
int32 AuditLandscapeFallback(FDiveTerrain& Terrain,const FString& Params);
int32 BakeLandscapeFallbackStudy(const FString& Params);
UStaticMesh* LoadLandscapeFallbackStudy(UStaticMesh* Source);
int32 BakeLandscapeResidentStudy(const FString& Params);
UStaticMesh* LoadLandscapeResidentStudy(UStaticMesh* Source);
UStaticMesh* LoadLandscapeDetailStudy(FDiveTerrain& Terrain,FString& Error);
#endif
