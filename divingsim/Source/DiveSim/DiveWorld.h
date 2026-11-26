#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DiveTerrain.h"
#include "Simulation/DiveModel.h"
#include "Simulation/WaterSurface.h"
#include "Simulation/VisualMedium.h"
#include "Simulation/EnvironmentLight.h"
#include "Simulation/Sediment.h"
#include "DiveWorld.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class ADirectionalLight;
class AExponentialHeightFog;
class ADivePawn;

struct FDiveBubble
{
    FVector Position;
    float Age=0, StartPressure=1, RadiusMetres=.006f, Phase=0, Side=1;
};
struct FDiveFish
{
    FVector Position=FVector::ZeroVector, Velocity=FVector::ZeroVector, Home=FVector::ZeroVector;
    float Phase=0, TailCycle=0, Scale=1;
    int32 Species=0, Instance=0;
    bool Active=false;
};
struct FDiveRock { FVector Position; float Radius=0; };
USTRUCT()
struct FDivePopulationCell
{
    GENERATED_BODY()
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Components;
};
struct FDiveSilt
{
    FVector Origin,Position;
    Dive::Sediment::Puff Model;
    int32 Side=1;
};
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
    bool FindDiscoveryTestView(bool Plant,FVector& Eye,FRotator& View,FString& Species);
    TArray<FString> PhotographSpecies(const FVector& Eye,const FVector& Forward) const;
    bool TestPopulationStreaming(FString& Detail);
    bool ValidateImagery(FString& Error) const;
    bool ValidateLandscape(FString& Error) const;
    bool ValidateBuildings(FString& Error) const;
    void EmitBreath(const FVector& Position,const FVector& Direction,float Pressure);
    void Configure(const Dive::Config& Config);
    Dive::ScenarioClock::Moment CurrentMoment() const;
    double CurrentHour() const;
    void UpdateDaylight();
    bool ValidateCelestial(FString& Error) const;
    double WaterAmbientAt(const FVector& Eye) const;
    double ExposureBiasAt(const FVector& Eye) const;
    double SurfaceZ(double X,double Y,double Seconds=-1) const;
    double PressureDepth(const FVector& Position,double Seconds) const;
    FVector WaveVelocity(const FVector& Position,double Seconds) const;
    bool ValidateWater(FString& Error) const;
    bool ValidateCaustics(FString& Error) const;
    void SetCausticsTestStrength(float Value);
    void ConfigureCausticsProbe(const Dive::Config& Settings,float Height);
    bool ValidateStorm(FString& Error) const;
    bool ValidateShoreline(FString& Error) const;
    bool ValidateParticles(FString& Detail) const;
    void DisturbSediment(int32 Fin,const FVector& Position,double Speed,double Step);
    void StepSediment(double Step);
    bool ValidateSediment(FString& Detail) const;
    int32 SedimentBorn() const { return SiltBorn; }
    int32 SedimentLive() const { return Silt.Num(); }
    FVector SedimentCenter() const;
    void HideSedimentForTest(bool Hidden);
    UPROPERTY() TObjectPtr<ADivePawn> Diver;
private:
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Ground;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> LandscapeMeshes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CoastMeshes;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BuildingMeshes;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> ShoreMeshes;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> ImageryMaterials;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> AccessRamps;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> AccessParts;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> AccessRails;
    UPROPERTY() TMap<FIntPoint,TObjectPtr<UProceduralMeshComponent>> Chunks;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Water;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> WaterMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> CloudMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> StarMaterial;
    UPROPERTY() TObjectPtr<class UVolumetricCloudComponent> Clouds;
    UPROPERTY() TObjectPtr<class USkyLightComponent> Sky;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rain;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Buoys;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> BuoyLines;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rocks;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Plants;
    UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> Fish;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bubbles;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Particles;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ParticleMaterial;
    TArray<int32> RenderedBubbleIndices;
    FVector ParticleAnchor=FVector::ZeroVector;
    FVector LastParticleViewer=FVector::ZeroVector;
    float LastParticleTime=0,LastParticleWaterTime=0;
    bool bParticleUniformsReady=false;
    float ParticleScale=1;
    double ParticleSimulationSeconds=0;
    bool bNoBubbles=false;
    int32 BubblesBorn=0,BubblesSurfaced=0;
    int32 ParticleCellMoves=0;
    UPROPERTY() TObjectPtr<class UDirectionalLightComponent> Sun;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> SolarWater;
    float CausticsStrength=1;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> CausticsProbe;
    void UpdateCaustics();
    UPROPERTY() TObjectPtr<class UExponentialHeightFogComponent> Fog;
    TArray<FDiveBubble> BubbleState;
    TArray<FDiveSilt> Silt;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SiltVolumes;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> SiltMaterials;
    bool bSiltTestHidden=false;
    double SiltCredit[2]={0,0};
    int32 SiltBorn=0;
    float SiltRenderClock=0;
    void ResetSediment();
    void RenderSediment(float Dt);
    TArray<FDiveFish> FishState;
    TArray<FDiveRock> RockState;
    UPROPERTY() TMap<FIntPoint,FDivePopulationCell> PopulationCells;
    FIntPoint PopulationCell=FIntPoint(MAX_int32,MAX_int32);
    double LastFishRefill=-1;
    bool bPopulationChecked=false;
    FRandomStream Random{1948};
    float Flow=.08f, VisibleDistance=14, WorldTime=0;
    double DayHour=12;
    FVector PopulationOrigin=FVector::ZeroVector;
    FVector PopulationForward=FVector::ForwardVector;
    Dive::WaterSurface Waves;
    Dive::Config WeatherSettings;
    Dive::VisualMedium OpticalMedium;
    Dive::EnvironmentLight Lighting;
    double WaterAmbient=0;
    bool bOpticsSmoke=false;
    uint8 ObservedOpticalMedia=0;
    UPROPERTY() TObjectPtr<class ACelestialVaultDaySequenceActor> Celestial;
    double LastSkyUTC=-1,LastMoonPhaseUTC=-1;
    float MoonPhaseAge=0;
    void CreateSky();
    void ValidateCelestialReferences();
    void UpdateAtmosphere();
    void CreateWater();
    void UpdateWater(const FVector& Player);
    void StreamTerrain(const FVector& Position,bool Immediate=false);
    void CreateChunk(FIntPoint Key);
    void CreateLandscape();
    void CreateCoast();
    void CreateBuildings();
    void CreateShoreline();
    void CreateAccess();
    UMaterialInstanceDynamic* ImageryMaterial(const TCHAR* Base,const FString& TexturePath);
    void Populate();
    void StreamPopulation(const FVector& Player,bool Initial=false);
    void GenerateRocks(FIntPoint Key,TArray<FTransform>& Transforms,TArray<FDiveRock>& States) const;
    void CreatePopulationCell(FIntPoint Key);
    void RefillFish(const FVector& Player,bool Initial);
    void TickFish(float Dt,const FVector& Player);
    void InitializeParticles();
    void UpdateParticles(float Dt,const FVector& Player);
    void UpdateParticleField(const FVector& Player);
    UStaticMeshComponent* Prop(const FString& Name,const FString& MeshPath,
        const FVector& Location,const FVector& Scale,const FString& MaterialPath, bool Collision=true);
};
