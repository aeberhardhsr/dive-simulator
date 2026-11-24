#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiveWorld.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class ADirectionalLight;
class AExponentialHeightFog;
class ADivePawn;

struct FDiveBubble { FVector Position; float Age=0, StartPressure=1, Radius=1; };
struct FDiveFish { FVector Position, Velocity; float Phase=0; };

UCLASS()
class DIVESIM_API ADiveWorld : public AActor
{
    GENERATED_BODY()
public:
    ADiveWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    static float GroundZ(float X,float Y);
    void EmitBreath(const FVector& Position,const FVector& Direction,float Pressure);
    void Configure(float Visibility,float Hour,float Current);
    UPROPERTY() TObjectPtr<ADivePawn> Diver;
private:
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Ground;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Water;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rocks;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Plants;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Fish;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bubbles;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Particles;
    UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
    UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;
    TArray<FDiveBubble> BubbleState;
    TArray<FDiveFish> FishState;
    FRandomStream Random{1948};
    float Flow=.08f, VisibleDistance=14, WorldTime=0;
    void CreateTerrain();
    void Populate();
    UStaticMeshComponent* Prop(const FString& Name,const FString& MeshPath,
        const FVector& Location,const FVector& Scale,const FString& MaterialPath, bool Collision=true);
};
