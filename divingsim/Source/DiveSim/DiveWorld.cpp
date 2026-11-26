#include "DiveWorld.h"
#include "DivePawn.h"
#include "Simulation/Ecology.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/VolumetricCloudComponent.h"
#include "Camera/CameraComponent.h"

namespace
{
const TCHAR* Sphere=TEXT("/Engine/BasicShapes/Sphere.Sphere");
const TCHAR* Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
UStaticMesh* Mesh(const TCHAR* Path,const TCHAR* Fallback=Sphere)
{ auto* value=LoadObject<UStaticMesh>(nullptr,Path); return value?value:LoadObject<UStaticMesh>(nullptr,Fallback); }
UMaterialInterface* Mat(const TCHAR* Name)
{ return LoadObject<UMaterialInterface>(nullptr,*(FString(TEXT("/Game/Diving/Materials/"))+Name+TEXT(".")+Name)); }
}

ADiveWorld::ADiveWorld()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent->SetMobility(EComponentMobility::Static);
    Ground=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Lakebed")); Ground->SetupAttachment(RootComponent);
    Ground->bUseAsyncCooking=true; Ground->SetCollisionProfileName(TEXT("BlockAll"));
    Water=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterSurface")); Water->SetupAttachment(RootComponent);
    Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto create=[this](const TCHAR* name)
    {
        auto* component=CreateDefaultSubobject<UInstancedStaticMeshComponent>(name);
        component->SetupAttachment(RootComponent); component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return component;
    };
    for(int32 i=0;i<3;++i)
    {
        auto* component=create(*FString::Printf(TEXT("LakeRock%d"),i+1));
        component->SetCollisionProfileName(TEXT("BlockAll"));Rocks.Add(component);
        Gravel.Add(create(*FString::Printf(TEXT("Gravel%d"),i+1)));
    }
    for(const auto& profile:Dive::PlantProfiles)
        Plants.Add(create(UTF8_TO_TCHAR(profile.Mesh)));
    for(const auto& profile:Dive::FishProfiles)
    {
        auto* component=create(UTF8_TO_TCHAR(profile.Mesh));
        component->NumCustomDataFloats=13;
        Fish.Add(component);
    }
    Bubbles=create(TEXT("BreathBubbles")); Particles=create(TEXT("SuspendedParticles"));
    Particles->NumCustomDataFloats=3;
    // The raster shader shrinks/wraps the base mesh. Its undeformed geometry
    // must not contribute metre-sized proxies to GI or ray-traced scenes.
    Particles->bAffectDynamicIndirectLighting=false;
    Particles->bAffectDistanceFieldLighting=false;
    Particles->bVisibleInRayTracing=false;
    Rain=create(TEXT("Rain")); Rain->SetCastShadow(false);
    Bubbles->SetCastShadow(false); Particles->SetCastShadow(false);
}

float ADiveWorld::GroundZ(double X,double Y) const
{
    float z=-30000; Terrain.SampleGround(X,Y,z); return z;
}

void ADiveWorld::SetEntry(int32 Index,bool Shore)
{
    if(!bTerrainReady) return;
    const auto& entries=Terrain.EntryList(Shore);
    const auto& entry=entries[FMath::Clamp(Index,0,entries.Num()-1)];
    PopulationOrigin=entry.Position;
    PopulationForward=FRotator(0,entry.Yaw,0).Vector();
    for(auto& pair:Chunks) pair.Value->DestroyComponent();
    Chunks.Reset(); StreamTerrain(entry.Position,true); Populate(); StreamVegetation(entry.Position,true);
}

void ADiveWorld::StreamTerrain(const FVector& Position,bool Immediate)
{
    constexpr double Width=6400;
    const FIntPoint center(FMath::FloorToInt(Position.X/Width),FMath::FloorToInt(Position.Y/Width));
    TArray<FIntPoint> needed;
    for(int y=-1;y<=1;++y) for(int x=-1;x<=1;++x) needed.Add(center+FIntPoint(x,y));
    for(auto it=Chunks.CreateIterator();it;++it)
        if(!needed.Contains(it.Key())) { it.Value()->DestroyComponent(); it.RemoveCurrent(); }
    needed.Sort([center](FIntPoint A,FIntPoint B){return (A-center).SizeSquared()<(B-center).SizeSquared();});
    for(const auto key:needed) if(!Chunks.Contains(key))
    { CreateChunk(key); if(!Immediate) break; }
}

void ADiveWorld::CreateChunk(FIntPoint Key)
{
    constexpr int N=65;
    TArray<FVector> vertices,normals; TArray<int32> indices;
    TArray<FVector2D> uv; TArray<FLinearColor> colors; TArray<FProcMeshTangent> tangents;
    TArray<bool> valid;
    const FVector origin(Key.X*6400.0,Key.Y*6400.0,0);
    for(int y=0;y<N;++y) for(int x=0;x<N;++x)
    {
        const double px=origin.X+x*100, py=origin.Y+y*100;
        float z=0;
        const bool measured=Terrain.Sample(px,py,z); valid.Add(measured);
        vertices.Add(FVector(x*100,y*100,z)); uv.Add(FVector2D(px/300,py/300));
        normals.Add(Terrain.SurfaceNormal(px,py));
        tangents.Add(FProcMeshTangent(1,0,0)); colors.Add(FLinearColor::White);
    }
    for(int y=0;y<N-1;++y) for(int x=0;x<N-1;++x)
    {
        const int a=y*N+x;
        if(valid[a]&&valid[a+N]&&valid[a+1]) indices.Append({a,a+N,a+1});
        if(valid[a+1]&&valid[a+N]&&valid[a+N+1]) indices.Append({a+1,a+N,a+N+1});
    }
    auto* component=NewObject<UProceduralMeshComponent>(this);
    component->SetupAttachment(RootComponent); component->SetRelativeLocation(origin);
    component->bUseAsyncCooking=true; component->SetCollisionProfileName(TEXT("BlockAll"));
    component->RegisterComponent();
    if(indices.Num()>0) component->CreateMeshSection_LinearColor(0,vertices,indices,normals,uv,colors,tangents,true);
    component->SetMaterial(0,Mat(TEXT("M_Lakebed"))); Chunks.Add(Key,component);
}

UStaticMeshComponent* ADiveWorld::Prop(const FString& Name,const FString& MeshPath,
    const FVector& Location,const FVector& Scale,const FString& MaterialPath,bool Collision)
{
    auto* component=NewObject<UStaticMeshComponent>(this,*Name);
    component->SetupAttachment(RootComponent); component->SetStaticMesh(Mesh(*MeshPath));
    component->SetRelativeLocation(Location); component->SetRelativeScale3D(Scale);
    component->SetMaterial(0,Mat(*MaterialPath));
    component->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
    component->RegisterComponent(); return component;
}

void ADiveWorld::Populate()
{
    FishState.Reset(); RockState.Reset();
    for(int32 i=0;i<Rocks.Num();++i)
    {
        const FString name=FString::Printf(TEXT("SM_LakeRock%d"),i+1);
        Rocks[i]->ClearInstances();
        Rocks[i]->SetStaticMesh(Mesh(*(TEXT("/Game/Diving/Rocks/")+name+TEXT(".")+name)));
        Rocks[i]->SetMaterial(0,Mat(TEXT("M_RockSurface")));
        const FString gravel=FString::Printf(TEXT("SM_Gravel%d"),i+1);
        Gravel[i]->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/Diving/Gravel/")+gravel+TEXT(".")+gravel)));
        Gravel[i]->SetMaterial(0,Mat(TEXT("M_GravelSurface")));
    }
    for(int32 species=0;species<Plants.Num();++species)
    {
        auto* component=Plants[species].Get(); component->ClearInstances();
        const FString name=UTF8_TO_TCHAR(Dive::PlantProfiles[species].Mesh);
        component->SetStaticMesh(Mesh(*(TEXT("/Game/Diving/Meshes/")+name+TEXT(".")+name)));
        component->SetMaterial(0,Mat(TEXT("M_Plant")));
    }
    for(int32 species=0;species<Fish.Num();++species)
    {
        auto* component=Fish[species].Get(); component->ClearInstances();
        const FString name=UTF8_TO_TCHAR(Dive::FishProfiles[species].Mesh);
        component->SetStaticMesh(Mesh(*(TEXT("/Game/Diving/Meshes/")+name+TEXT(".")+name)));
        component->SetMaterial(0,Mat(*(TEXT("MI_Fish_")+name.RightChop(3))));
    }
    InitializeParticles();
    for(auto& pair:PopulationCells)
        for(const auto& component:pair.Value.Components) component->DestroyComponent();
    PopulationCells.Reset();PopulationCell=FIntPoint(MAX_int32,MAX_int32);LastFishRefill=-1;
    for(auto& pair:GravelCells)
        for(const auto& component:pair.Value.Components) component->DestroyComponent();
    GravelCells.Reset();GravelCell=FIntPoint(MAX_int32,MAX_int32);
    for(int32 species=0;species<Fish.Num();++species)
        for(int i=0;i<Dive::FishProfiles[species].Count;++i)
        {
            FDiveFish fish;fish.Species=species;
            fish.Instance=Fish[species]->AddInstance(FTransform(FRotator::ZeroRotator,PopulationOrigin,FVector::ZeroVector));
            FishState.Add(fish);
        }
    Random.Initialize(uint32(FMath::FloorToInt(PopulationOrigin.X/9600))*73856093u^uint32(FMath::FloorToInt(PopulationOrigin.Y/9600))*19349663u^1948u);
    StreamPopulation(PopulationOrigin,true);

}

void ADiveWorld::BeginPlay()
{
    Super::BeginPlay();
    bOpticsSmoke=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"));
    bTerrainReady=Terrain.Load();
    if(bTerrainReady)
    {
        CreateLandscape();
        CreateBuildings();
        CreateShoreline();
        CreateCoast();
        CreateAccess();
        CreateVegetation();
        if(!bTerrainReady) return;
        SetEntry(0);
        for(int i=0;i<Terrain.Entries.Num();++i)
        {
            const FVector p=Terrain.Entries[i].Position;
            const float bottom=GroundZ(p.X,p.Y);
            Buoys.Add(Prop(FString::Printf(TEXT("EntryBuoy%d"),i),Sphere,FVector(p.X,p.Y,15),FVector(.55,.55,.75),TEXT("M_Buoy"),false));
            BuoyLines.Add(Prop(FString::Printf(TEXT("EntryLine%d"),i),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
                FVector(p.X,p.Y,bottom*.5),FVector(.025,.025,-bottom/100),TEXT("M_Rope"),false));
        }
    }
    else UE_LOG(LogTemp,Error,TEXT("WALENSEE_DATA_FAILED: %s"),*Terrain.Error);
    CreateWater();
    WaterMaterial=UMaterialInstanceDynamic::Create(Mat(TEXT("M_Water")),this);
    Water->SetMaterial(0,WaterMaterial);
    Water->SetCastShadow(false);
    CreateSky();
    CloudMaterial=UMaterialInstanceDynamic::Create(Mat(TEXT("MI_Cloud")),this);
    Clouds->SetMaterial(CloudMaterial);
    Clouds->SetLayerBottomAltitude(1.4f); Clouds->SetLayerHeight(1.2f);
    Rain->SetStaticMesh(Mesh(Sphere)); Rain->SetMaterial(0,Mat(TEXT("M_Rain")));
    Fog->SetWorldLocation(FVector(0,0,-150));
    Fog->SetVolumetricFog(true);
    Fog->SetVolumetricFogScatteringDistribution(.55f);
    Configure(Dive::Config{});
}

void ADiveWorld::Configure(const Dive::Config& Config)
{
    ResetSediment();
    WeatherSettings=Config.Resolved();
    BubbleState.Reset();RenderedBubbleIndices.Reset();Bubbles->ClearInstances();BubblesBorn=BubblesSurfaced=0;bParticleUniformsReady=false;
    VisibleDistance=WeatherSettings.VisibilityMetres; Flow=WeatherSettings.CurrentMetresPerSecond; DayHour=WeatherSettings.Hour;
    Waves.Configure(WeatherSettings.WindMetresPerSecond,WeatherSettings.WindDirectionDegrees);
    if(WaterMaterial)
    {
        for(int i=0;i<4;++i)
        {
            const auto& w=Waves.Waves[i];
            WaterMaterial->SetVectorParameterValue(FName(*FString::Printf(TEXT("Wave%d"),i)),FLinearColor(w.Kx,w.Ky,w.Omega,w.Amplitude));
        }
        WaterMaterial->SetVectorParameterValue(TEXT("WavePhase"),FLinearColor(Waves.Waves[0].Phase,Waves.Waves[1].Phase,Waves.Waves[2].Phase,Waves.Waves[3].Phase));
        WaterMaterial->SetScalarParameterValue(TEXT("OpticalScale"),14.f/VisibleDistance);
        WaterMaterial->SetScalarParameterValue(TEXT("WindStrength"),FMath::Clamp(WeatherSettings.WindMetresPerSecond/12,0.,1.));
    }
    if(CloudMaterial)
    {
        const auto scenario=Dive::WeatherScenario::At(WeatherSettings.Weather);
        CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalCoverage"),scenario.CloudCoverage);
        CloudMaterial->SetScalarParameterValue(TEXT("Cloud_GlobalDensity"),scenario.CloudDensity);
        CloudMaterial->SetScalarParameterValue(TEXT("StormClouds"),0); // Wind/rain storm without lightning.
    }
    LastSkyUTC=-1; UpdateDaylight();
    // Real-time sky capture contains the current sun/weather brightness.
    if(Sky) Sky->SetIntensity(3.f);
    UpdateAtmosphere();
}

Dive::ScenarioClock::Moment ADiveWorld::CurrentMoment() const
{
    return Dive::ScenarioClock::At(WeatherSettings.Date,WeatherSettings.Hour,Diver?Diver->Simulation.Data.ElapsedSeconds:0);
}
double ADiveWorld::CurrentHour() const { return CurrentMoment().Hour; }

double ADiveWorld::WaterAmbientAt(const FVector& Eye) const
{
    return Lighting.WaterAmbient((SurfaceZ(Eye.X,Eye.Y)-Eye.Z)/100,VisibleDistance);
}

double ADiveWorld::ExposureBiasAt(const FVector& Eye) const
{
    const double target=Lighting.ExposureBias((SurfaceZ(Eye.X,Eye.Y)-Eye.Z)/100,VisibleDistance);
    // A lit torch bounds the authored dark adaptation so its foreground beam
    // does not blow out when turning it on after looking at the night sky.
    return Diver&&Diver->bLamp&&Diver->Simulation.Data.LampBattery>0?FMath::Min(target,5.5):target;
}

void ADiveWorld::UpdateAtmosphere()
{
    if(!Fog) return;
    const FVector eye=Diver?Diver->GetPawnViewLocation():PopulationOrigin;
    OpticalMedium=Dive::VisualMedium::At((SurfaceZ(eye.X,eye.Y)-eye.Z)/100,WeatherSettings.Weather,VisibleDistance);
    WaterAmbient=WaterAmbientAt(eye);
    const float water=static_cast<float>(OpticalMedium.Water);
    auto* fog=Fog.Get();
    fog->SetFogDensity(OpticalMedium.Density);
    fog->SetFogHeightFalloff(OpticalMedium.HeightFalloff);
    // Black additive colour leaves aerial fog lighting to Sky Atmosphere.
    // Its contribution fades out below water, where the lake tint takes over.
    fog->SetFogInscatteringColor(FLinearColor(280,1150,950)*(water*WaterAmbient));
    fog->SetDirectionalInscatteringColor(FLinearColor::Black);
    fog->SetSkyAtmosphereAmbientContributionColorScale(FLinearColor(1-water,1-water,1-water));
    fog->SetVolumetricFogAlbedo(FMath::Lerp(FLinearColor::White,FLinearColor(FColor(75,135,110)),water).ToFColorSRGB());
    // Concentrate underwater froxels on the visible water, instead of kilometres
    // of atmosphere; native shadowed volumetrics provide the torch beam.
    fog->SetVolumetricFogDistance(FMath::Lerp(60000.f,static_cast<float>(FMath::Clamp(static_cast<double>(VisibleDistance)*200,1000.,8000.)),water));
    fog->SetVolumetricFogStartDistance(0);
    fog->SetVolumetricFogNearFadeInDistance(10);
}

void ADiveWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!bTerrainReady) return;
    UpdateDaylight();
    UpdateCaustics();
    if(Diver && Diver->bDiveActive && Diver->bMenu) { UpdateAtmosphere(); UpdateParticleField(Diver->GetActorLocation()); return; }
    const float dt=FMath::Min(DeltaSeconds,.1f); WorldTime+=dt;
    const FVector player=Diver?Diver->GetActorLocation():PopulationOrigin;
    UpdateWater(player);
    if(bTerrainReady)
    {
        StreamTerrain(player);
        StreamPopulation(player);
        StreamVegetation(player);
        if(bOpticsSmoke&&!bVegetationChecked&&WorldTime>2)
        {
            bVegetationChecked=true;FString detail;
            if(FParse::Param(FCommandLine::Get(),TEXT("DiveVegetationChecks"))&&!TestVegetationStreaming(detail))
            { UE_LOG(LogTemp,Error,TEXT("DIVE_VEGETATION_STREAM_FAIL %s"),*detail);FPlatformMisc::RequestExitWithStatus(false,1);return; }
            if(!ValidateVegetation(detail))
            { UE_LOG(LogTemp,Error,TEXT("DIVE_VEGETATION_FAIL %s"),*detail);FPlatformMisc::RequestExitWithStatus(false,1);return; }
        }
        if(bOpticsSmoke&&!bPopulationChecked&&WorldTime>2&&FParse::Param(FCommandLine::Get(),TEXT("DivePopulationChecks")))
        {
            bPopulationChecked=true;FString detail;
            const bool valid=TestPopulationStreaming(detail);
            UE_LOG(LogTemp,Display,TEXT("DIVE_POPULATION_%s %s"),valid?TEXT("PASS"):TEXT("FAIL"),*detail);
            if(!valid) { FPlatformMisc::RequestExitWithStatus(false,1);return; }
        }
    }
    UpdateAtmosphere();
    TickFish(dt,player);
    UpdateParticles(dt,player);
    RenderSediment(dt);
    if(bOpticsSmoke&&Diver)
    {
        if(!Sun||!Sky||FMath::Abs(Sun->Intensity-Lighting.SolarSource*Dive::EnvironmentLight::SunLux)>.02||
            FMath::Abs(Sky->Intensity-3)>1.e-5||
            !Sky->GetLightColor().Equals(FLinearColor::White,1.e-5f))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_LIGHT_FAIL Sun/sky no longer match the configured environment")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        FString celestialError;
        if(!ValidateCelestial(celestialError))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_LIGHT_FAIL %s"),*celestialError); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        // This actor ticks after the pawn: both the current camera transform
        // and its post-process settings have now been updated for this frame.
        if(!Diver->Camera->PostProcessSettings.SceneColorTint.Equals(
            FMath::Lerp(FLinearColor::White,FLinearColor(.48f,.83f,.78f),static_cast<float>(OpticalMedium.Water)),1.e-4f))
        { UE_LOG(LogTemp,Error,TEXT("DIVE_MEDIUM_FAIL Fog and post-process camera medium disagree")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
        const uint8 phase=OpticalMedium.Water<1.e-5?1:OpticalMedium.Water>1-1.e-5?4:2;
        if(!(ObservedOpticalMedia&phase))
        {
            FString error;
            if(!ValidateWater(error))
            { UE_LOG(LogTemp,Error,TEXT("DIVE_MEDIUM_FAIL %s"),*error); FPlatformMisc::RequestExitWithStatus(false,1); return; }
            ObservedOpticalMedia|=phase;
            UE_LOG(LogTemp,Display,TEXT("DIVE_LIGHT_PASS elevation=%.3f sun=%.5f sky=%.3f water_ambient=%.6f"),
                Lighting.ElevationDegrees,Sun->Intensity,Sky->Intensity,WaterAmbient);
            UE_LOG(LogTemp,Display,TEXT("DIVE_MEDIUM_PASS phase=%s weight=%.5f density=%.5f sky=%.5f"),
                phase==1?TEXT("air"):phase==4?TEXT("water"):TEXT("transition"),OpticalMedium.Water,
                Fog->FogDensity,Fog->SkyAtmosphereAmbientContributionColorScale.R);
        }
    }
}
