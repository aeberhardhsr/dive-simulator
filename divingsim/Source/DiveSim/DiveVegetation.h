#pragma once
#include "CoreMinimal.h"
#include "DiveVegetation.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class UInstancedStaticMeshComponent;

struct FDiveTree
{
    float East=0,North=0,Height=0,NativeGround=0;
    uint32 Seed=0;
    uint8 Kind=0,Source=0;
    uint16 Reserved=0;
};
static_assert(sizeof(FDiveTree)==24,"DVT1 record layout changed");

USTRUCT()
struct FDiveTreeCell
{
    GENERATED_BODY()
    UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Trees;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Trunks;
    TArray<FTransform> TrunkTransforms;
};
