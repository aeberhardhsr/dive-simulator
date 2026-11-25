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
    Rocks=create(TEXT("Limestone")); Rocks->SetCollisionProfileName(TEXT("BlockAll"));
    for(const auto& profile:Dive::PlantProfiles)
        Plants.Add(create(UTF8_TO_TCHAR(profile.Mesh)));
    for(const auto& profile:Dive::FishProfiles)
    {
        auto* component=create(UTF8_TO_TCHAR(profile.Mesh));
        component->NumCustomDataFloats=5;
        Fish.Add(component);
    }
    Bubbles=create(TEXT("BreathBubbles")); Particles=create(TEXT("SuspendedParticles"));
    Rain=create(TEXT("Rain")); Rain->SetCastShadow(false);
    Bubbles->SetCastShadow(false); Particles->SetCastShadow(false);
}

float ADiveWorld::GroundZ(double X,double Y) const
{
    float z=-30000; Terrain.Sample(X,Y,z); return z;
}

void ADiveWorld::SetEntry(int32 Index,bool Shore)
{
    if(!bTerrainReady) return;
    const auto& entries=Terrain.EntryList(Shore);
    const auto& entry=entries[FMath::Clamp(Index,0,entries.Num()-1)];
    PopulationOrigin=entry.Position;
    PopulationForward=FRotator(0,entry.Yaw,0).Vector();
    for(auto& pair:Chunks) pair.Value->DestroyComponent();
    Chunks.Reset(); StreamTerrain(entry.Position,true); Populate();
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
        float z=0,n=0,s=0,e=0,w=0;
        const bool measured=Terrain.Sample(px,py,z); valid.Add(measured);
        if(!Terrain.Sample(px+100,py,n)) n=z;
        if(!Terrain.Sample(px-100,py,s)) s=z;
        if(!Terrain.Sample(px,py+100,e)) e=z;
        if(!Terrain.Sample(px,py-100,w)) w=z;
        vertices.Add(FVector(x*100,y*100,z)); uv.Add(FVector2D(px/300,py/300));
        normals.Add(FVector(-(n-s)/200,-(e-w)/200,1).GetSafeNormal());
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
    Rocks->ClearInstances(); FishState.Reset(); RockState.Reset();
    Rocks->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Rock.SM_Rock")));
    Rocks->SetMaterial(0,Mat(TEXT("M_RockSurface")));
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
        component->SetMaterial(0,Mat(TEXT("M_Fish")));
    }
    Bubbles->SetStaticMesh(Mesh(Sphere)); Bubbles->SetMaterial(0,Mat(TEXT("M_Bubble")));
    Particles->SetStaticMesh(Mesh(Sphere)); Particles->SetMaterial(0,Mat(TEXT("M_Particle")));
    // Habitat dressing is procedural, not part of the surveyed height data.
    Random.Initialize(static_cast<uint32>(FMath::FloorToInt(PopulationOrigin.X/6400))*73856093u ^
        static_cast<uint32>(FMath::FloorToInt(PopulationOrigin.Y/6400))*19349663u);
    for(int i=0;i<400;++i)
    {
        const double x=PopulationOrigin.X+Random.FRandRange(-8500.f,8500.f);
        const double y=PopulationOrigin.Y+Random.FRandRange(-8500.f,8500.f);
        float z; if(!Terrain.Sample(x,y,z)||z>-250||z<-6000) continue;
        if(FVector2D(x-PopulationOrigin.X,y-PopulationOrigin.Y).Size()<350) continue;
        const float size=Random.FRandRange(.4f,2.6f);
        Rocks->AddInstance(FTransform(FRotator(Random.FRandRange(-15.f,15.f),Random.FRandRange(0.f,360.f),0),
            FVector(x,y,z+size*15),FVector(size,size*.85,size*.7)));
        RockState.Add({FVector(x,y,z+size*15),size*105});
    }
    for(int i=0;i<1800;++i)
    {
        const double x=PopulationOrigin.X+Random.FRandRange(-8000.f,8000.f);
        const double y=PopulationOrigin.Y+Random.FRandRange(-8000.f,8000.f);
        float z,n,e;
        if(!Terrain.Sample(x,y,z)||!Terrain.Sample(x+100,y,n)||!Terrain.Sample(x,y+100,e)) continue;
        if(FMath::Sin(x*.002)+FMath::Cos(y*.003)<-.1) continue;
        const int32 species=i%Plants.Num();
        const float slope=FMath::Sqrt(FMath::Square(n-z)+FMath::Square(e-z))/100;
        if(!Dive::PlantHabitat(Dive::PlantProfiles[species],-z/100,slope,VisibleDistance)) continue;
        bool blocked=false;
        for(const auto& rock:RockState)
            if(FVector::Dist2D(rock.Position,FVector(x,y,z))<rock.Radius) { blocked=true; break; }
        if(blocked) continue;
        const float size=Random.FRandRange(.65f,1.2f);
        Plants[species]->AddInstance(FTransform(FRotator(0,Random.FRandRange(0.f,360.f),0),FVector(x,y,z+2),FVector(size)));
    }
    const FVector center=PopulationOrigin+PopulationForward*1800;
    for(int32 species=0;species<Fish.Num();++species)
    {
        const auto& profile=Dive::FishProfiles[species];
        for(int count=0,attempt=0;count<profile.Count&&attempt<profile.Count*30;++attempt)
        {
            FDiveFish fish; fish.Species=species; fish.Scale=Random.FRandRange(.75f,1.15f);
            FVector pos=center+FVector(Random.FRandRange(-3500.f,3500.f),Random.FRandRange(-3500.f,3500.f),0);
            float z; if(!Terrain.Sample(pos.X,pos.Y,z)) continue;
            const auto band=Dive::FishDepthBand(profile,-z/100,fish.Scale);
            if(!band.Valid) continue;
            pos.Z=-100*Random.FRandRange(band.Shallow,band.Deep);
            bool blocked=false;
            for(const auto& rock:RockState)
                if(FVector::Dist(rock.Position,pos)<rock.Radius+profile.LengthMetres*50*fish.Scale) { blocked=true; break; }
            if(blocked) continue;
            fish.Position=pos; fish.Home=pos;
            fish.Velocity=PopulationForward*profile.CruiseSpeed*100;
            fish.Phase=Random.FRandRange(0.f,6.28f);
            fish.Instance=Fish[species]->AddInstance(FTransform(PopulationForward.Rotation(),pos,FVector(fish.Scale)));
            FishState.Add(fish); ++count;
        }
        UE_LOG(LogTemp,Display,TEXT("DIVE_ECOLOGY fish=%s count=%d"),UTF8_TO_TCHAR(profile.Mesh),Fish[species]->GetInstanceCount());
    }
    for(int32 species=0;species<Plants.Num();++species)
        UE_LOG(LogTemp,Display,TEXT("DIVE_ECOLOGY plant=%s count=%d"),UTF8_TO_TCHAR(Dive::PlantProfiles[species].Mesh),Plants[species]->GetInstanceCount());

}

void ADiveWorld::BeginPlay()
{
    Super::BeginPlay();
    bOpticsSmoke=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"));
    bTerrainReady=Terrain.Load();
    if(bTerrainReady)
    {
        CreateLandscape();
        CreateShoreline();
        CreateAccess();
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
    WeatherSettings=Config; WeatherSettings.Sanitize();
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
}

void ADiveWorld::EmitBreath(const FVector& Position,const FVector& Direction,float Pressure)
{
    for(int i=0;i<18;++i)
    {
        FDiveBubble bubble;
        bubble.Position=Position+Direction*35+FVector(0,Random.FRandRange(-9,9),-20);
        bubble.StartPressure=Pressure; bubble.Age=-i*.018f; bubble.Radius=Random.FRandRange(.012,.032);
        BubbleState.Add(bubble);
    }
}

void ADiveWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!bTerrainReady) return;
    UpdateDaylight();
    if(Diver && Diver->bDiveActive && Diver->bMenu) { UpdateAtmosphere(); return; }
    const float dt=FMath::Min(DeltaSeconds,.1f); WorldTime+=dt;
    const FVector player=Diver?Diver->GetActorLocation():PopulationOrigin;
    UpdateWater(player);
    if(bTerrainReady)
    {
        StreamTerrain(player);
        if(FVector::Dist2D(player,PopulationOrigin)>5500)
        { PopulationOrigin=player; Populate(); }
    }
    UpdateAtmosphere();
    TickFish(dt,player);
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
    Bubbles->ClearInstances();
    for(int i=BubbleState.Num()-1;i>=0;--i)
    {
        auto& b=BubbleState[i]; b.Age+=dt;
        b.Position+=FVector(FMath::Sin(WorldTime*3+i)*3,Flow*100,40+b.Radius*1500)*dt;
        if(b.Age>18 || b.Position.Z>SurfaceZ(b.Position.X,b.Position.Y)) { BubbleState.RemoveAtSwap(i); continue; }
        const float pressure=.965f+PressureDepth(b.Position,Diver?Diver->Simulation.Data.ElapsedSeconds:0)*.0980665f;
        const float radius=b.Radius*FMath::Pow(b.StartPressure/pressure,1.f/3.f);
        Bubbles->AddInstance(FTransform(FRotator::ZeroRotator,b.Position,FVector(radius)));
    }
    Particles->ClearInstances();
    for(int i=0;i<160;++i)
    {
        const float seed=i*137.5f;
        const FVector relative(FMath::Fmod(seed*17+WorldTime*2,2400.f)-1200,
            FMath::Fmod(seed*11+WorldTime*Flow*100,2400.f)-1200,FMath::Fmod(seed*7,1400.f)-700);
        Particles->AddInstance(FTransform(FRotator::ZeroRotator,player+relative,FVector(.0018f)));
    }
}
