#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiveTerrain.h"
#include "Simulation/DiveModel.h"
#include "Simulation/WaterSurface.h"
#include "Simulation/VisualMedium.h"
#include "Simulation/EnvironmentLight.h"
#include "DiveWorld.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ADirectionalLight;
class AExponentialHeightFog;
class ADivePawn;

struct FDiveBubble { FVector Position; float Age=0, StartPressure=1, Radius=1; };
struct FDiveFish
{
    FVector Position, Velocity, Home;
    float Phase=0, TailCycle=0, Scale=1;
    int32 Species=0, Instance=0;
};
struct FDiveRock { FVector Position; float Radius=0; };
struct FDiveAccess
{
    FVector Origin=FVector::ZeroVector,Forward=FVector::ForwardVector;
    double Landing=0,Length=0,Drop=0;
    int32 Steps=0;
};

UCLASS()
class DIVESIM_API ADiveWorld : public AActor
{
    GENERATED_BODY()
public:
    ADiveWorld();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    float GroundZ(double X,double Y) const;
    FDiveTerrain Terrain;
    bool bTerrainReady=false;
    void SetEntry(int32 Index,bool Shore=false);
    bool SupportZ(double X,double Y,float& Z,double MaxAccessZ=TNumericLimits<double>::Max()) const;
    FVector ShoreStart(int32 Index) const;
    TArray<FDiveAccess> Accesses;
    int32 TerrainChunkCount() const { return Chunks.Num(); }
    bool ValidateEcology(FString& Error) const;
    bool ValidateImagery(FString& Error) const;
    bool ValidateLandscape(FString& Error) const;
    void EmitBreath(const FVector& Position,const FVector& Direction,float Pressure);
    void Configure(const Dive::Config& Config);
    double WaterAmbientAt(const FVector& Eye) const;
    double ExposureBiasAt(const FVector& Eye) const;
    double SurfaceZ(double X,double Y,double Seconds=-1) const;
    double PressureDepth(const FVector& Position,double Seconds) const;
    FVector WaveVelocity(const FVector& Position,double Seconds) const;
    bool ValidateWater(FString& Error) const;
    bool ValidateShoreline(FString& Error) const;
    UPROPERTY() TObjectPtr<ADivePawn> Diver;
private:
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Ground;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LandscapeMeshes;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> ShoreMeshes;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> ImageryMaterials;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> AccessRamps;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> AccessParts;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> AccessRails;
    UPROPERTY() TMap<FIntPoint,TObjectPtr<UProceduralMeshComponent>> Chunks;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Water;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CloudMaterial;
    UPROPERTY() TObjectPtr<class UVolumetricCloudComponent> Clouds;
    UPROPERTY() TObjectPtr<class ASkyLight> Sky;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rain;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Buoys;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BuoyLines;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rocks;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Plants;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Fish;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bubbles;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Particles;
    UPROPERTY() TObjectPtr<ADirectionalLight> Sun;
    UPROPERTY() TObjectPtr<AExponentialHeightFog> Fog;
    TArray<FDiveBubble> BubbleState;
    TArray<FDiveFish> FishState;
    TArray<FDiveRock> RockState;
    FRandomStream Random{1948};
    float Flow=.08f, VisibleDistance=14, WorldTime=0, DayHour=12;
    FVector PopulationOrigin=FVector::ZeroVector;
    FVector PopulationForward=FVector::ForwardVector;
    Dive::WaterSurface Waves;
    Dive::Config WeatherSettings;
    Dive::VisualMedium OpticalMedium;
    Dive::EnvironmentLight Lighting;
    double WaterAmbient=0;
    bool bOpticsSmoke=false;
    uint8 ObservedOpticalMedia=0;
    void UpdateAtmosphere();
    void CreateWater();
    void UpdateWater(const FVector& Player);
    void StreamTerrain(const FVector& Position,bool Immediate=false);
    void CreateChunk(FIntPoint Key);
    void CreateLandscape();
    void CreateShoreline();
    void CreateAccess();
    UMaterialInstanceDynamic* ImageryMaterial(const TCHAR* Base,const FString& TexturePath);
    void Populate();
    void TickFish(float Dt,const FVector& Player);
    UStaticMeshComponent* Prop(const FString& Name,const FString& MeshPath,
        const FVector& Location,const FVector& Scale,const FString& MaterialPath, bool Collision=true);
};
