#include "DiveWorld.h"
#include "DivePawn.h"
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
    Ground=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Lakebed")); Ground->SetupAttachment(RootComponent);
    Ground->bUseAsyncCooking=true; Ground->SetCollisionProfileName(TEXT("BlockAll"));
    Water=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterSurface")); Water->SetupAttachment(RootComponent);
    Water->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    auto create=[this](const TCHAR* name)
    {
        auto* component=CreateDefaultSubobject<UInstancedStaticMeshComponent>(name);
        component->SetupAttachment(RootComponent); component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return component;
    };
    Rocks=create(TEXT("Limestone")); Rocks->SetCollisionProfileName(TEXT("BlockAll"));
    Plants=create(TEXT("Pondweed")); Fish=create(TEXT("Perch"));
    Bubbles=create(TEXT("BreathBubbles")); Particles=create(TEXT("SuspendedParticles"));
    Bubbles->SetCastShadow(false); Particles->SetCastShadow(false);
}

float ADiveWorld::GroundZ(double X,double Y) const
{
    float z=-30000; Terrain.Sample(X,Y,z); return z;
}

void ADiveWorld::SetEntry(int32 Index)
{
    if(!bTerrainReady) return;
    const auto& entry=Terrain.Entries[FMath::Clamp(Index,0,Terrain.Entries.Num()-1)];
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
    Rocks->ClearInstances(); Plants->ClearInstances(); Fish->ClearInstances(); FishState.Reset();
    Rocks->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Rock.SM_Rock")));
    Rocks->SetMaterial(0,Mat(TEXT("M_Rock")));
    Plants->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Pondweed.SM_Pondweed")));
    Plants->SetMaterial(0,Mat(TEXT("M_Plant")));
    Fish->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Perch.SM_Perch")));
    Fish->SetMaterial(0,Mat(TEXT("M_Fish")));
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
    }
    for(int i=0;i<1800;++i)
    {
        const double x=PopulationOrigin.X+Random.FRandRange(-8000.f,8000.f);
        const double y=PopulationOrigin.Y+Random.FRandRange(-8000.f,8000.f);
        float z; if(!Terrain.Sample(x,y,z)||z>-250||z<-1800) continue;
        if(FMath::Sin(x*.002)+FMath::Cos(y*.003)<-.1) continue;
        const float size=Random.FRandRange(.45f,1.3f);
        Plants->AddInstance(FTransform(FRotator(0,Random.FRandRange(0.f,360.f),0),FVector(x,y,z+2),FVector(size)));
    }
    const FVector center=PopulationOrigin+PopulationForward*1200;
    for(int i=0;i<54;++i)
    {
        FVector pos=center+FVector(Random.FRandRange(-1500.f,1500.f),Random.FRandRange(-1500.f,1500.f),0);
        float z; if(!Terrain.Sample(pos.X,pos.Y,z)||z>-350) continue;
        pos.Z=FMath::Max(z+180.f,Random.FRandRange(-900.f,-280.f));
        FDiveFish fish; fish.Position=pos; fish.Velocity=PopulationForward*40; fish.Phase=Random.FRandRange(0.f,6.28f);
        FishState.Add(fish); Fish->AddInstance(FTransform(FRotator::ZeroRotator,pos,FVector(Random.FRandRange(.8f,1.6f))));
    }
}

void ADiveWorld::BeginPlay()
{
    Super::BeginPlay();
    bTerrainReady=Terrain.Load();
    if(bTerrainReady)
    {
        SetEntry(0);
        for(int i=0;i<Terrain.Entries.Num();++i)
        {
            const FVector p=Terrain.Entries[i].Position;
            const float bottom=GroundZ(p.X,p.Y);
            Prop(FString::Printf(TEXT("EntryBuoy%d"),i),Sphere,FVector(p.X,p.Y,15),FVector(.55,.55,.75),TEXT("M_Buoy"),false);
            Prop(FString::Printf(TEXT("EntryLine%d"),i),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
                FVector(p.X,p.Y,bottom*.5),FVector(.025,.025,-bottom/100),TEXT("M_Rope"),false);
        }
    }
    else UE_LOG(LogTemp,Error,TEXT("WALENSEE_DATA_FAILED: %s"),*Terrain.Error);
    Water->SetStaticMesh(Mesh(TEXT("/Engine/BasicShapes/Plane.Plane")));
    Water->SetRelativeLocation(FVector(200000,800000,0));
    Water->SetRelativeScale3D(FVector(10000,36000,1)); Water->SetMaterial(0,Mat(TEXT("M_Water")));
    Water->SetCastShadow(false);
    Sun=GetWorld()->SpawnActor<ADirectionalLight>();
    Sun->SetActorRotation(FRotator(-42,-30,0));
    auto* light=CastChecked<UDirectionalLightComponent>(Sun->GetLightComponent()); light->SetMobility(EComponentMobility::Movable);
    light->SetIntensity(8); light->SetLightColor(FLinearColor(1,.94,.8));
    light->bAtmosphereSunLight=true;
    light->SetVolumetricScatteringIntensity(2);
    GetWorld()->SpawnActor<ASkyAtmosphere>();
    auto* sky=GetWorld()->SpawnActor<ASkyLight>();
    sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    sky->GetLightComponent()->SetIntensity(.8);
    sky->GetLightComponent()->SetLightColor(FLinearColor(.55,.75,.85));
    sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);
    Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();
    Fog->SetActorLocation(FVector(0,0,-150));
    Fog->GetComponent()->SetFogHeightFalloff(.01);
    Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.028,.115,.095));
    Fog->GetComponent()->SetVolumetricFog(true);
    Fog->GetComponent()->SetVolumetricFogScatteringDistribution(.55f);
    Fog->GetComponent()->SetVolumetricFogAlbedo(FColor(75,135,110));
    Configure(14,13,.08);
}

void ADiveWorld::Configure(float Visibility,float Hour,float Current)
{
    VisibleDistance=Visibility; Flow=Current;
    if(Fog) Fog->GetComponent()->SetFogDensity(.35f*14/Visibility);
    const float daylight=FMath::Max(.02f,FMath::Sin((Hour-6)/12*PI));
    if(Sun)
    {
        Sun->SetActorRotation(FRotator(-FMath::Max(5.f,daylight*65),Hour*15-180,0));
        Sun->GetLightComponent()->SetIntensity(daylight*8);
    }
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
    if(Diver && Diver->bDiveActive && Diver->bMenu) return;
    const float dt=FMath::Min(DeltaSeconds,.1f); WorldTime+=dt;
    const FVector player=Diver?Diver->GetActorLocation():PopulationOrigin;
    if(bTerrainReady)
    {
        StreamTerrain(player);
        if(FVector::Dist2D(player,PopulationOrigin)>5500)
        { PopulationOrigin=player; Populate(); }
    }
    if(Fog)
    {
        const float underwater=FMath::Clamp(static_cast<float>(-player.Z)/70,0.f,1.f);
        Fog->GetComponent()->SetFogDensity(FMath::Lerp(.002f,.35f*14/VisibleDistance,underwater));
    }
    for(int i=0;i<FishState.Num();++i)
    {
        auto& fish=FishState[i];
        const FVector center=PopulationOrigin+PopulationForward*1200+FVector(600*FMath::Sin(WorldTime*.03f),i%2?0:600,-400);
        FVector steer=(center-fish.Position)*.025;
        FVector separation=FVector::ZeroVector;
        for(int j=0;j<FishState.Num();++j)
        {
            if(i==j) continue;
            const FVector away=fish.Position-FishState[j].Position;
            const float distance=away.Size();
            if(distance<90 && distance>1) separation+=away/distance*(90-distance)*.8;
        }
        steer+=separation;
        const FVector away=fish.Position-player;
        if(away.Size()<450) steer+=away.GetSafeNormal()*(450-away.Size())*.5;
        steer.Z+=FMath::Sin(WorldTime+fish.Phase)*4;
        fish.Velocity=(fish.Velocity+steer*dt).GetClampedToMaxSize(away.Size()<450?180:65);
        const FVector next=fish.Position+(fish.Velocity+FVector(0,Flow*100,0))*dt;
        float ground;
        if(Terrain.Sample(next.X,next.Y,ground)) fish.Position=next;
        else fish.Velocity=-fish.Velocity;
        fish.Position.Z=FMath::Clamp(fish.Position.Z,static_cast<double>(GroundZ(fish.Position.X,fish.Position.Y)+100),-100.0);
        FTransform transform; Fish->GetInstanceTransform(i,transform);
        transform.SetLocation(fish.Position); transform.SetRotation(fish.Velocity.Rotation().Quaternion());
        Fish->UpdateInstanceTransform(i,transform,false,i==FishState.Num()-1,true);
    }
    Bubbles->ClearInstances();
    for(int i=BubbleState.Num()-1;i>=0;--i)
    {
        auto& b=BubbleState[i]; b.Age+=dt;
        b.Position+=FVector(FMath::Sin(WorldTime*3+i)*3,Flow*100,40+b.Radius*1500)*dt;
        if(b.Age>18 || b.Position.Z>0) { BubbleState.RemoveAtSwap(i); continue; }
        const float pressure=.965f+FMath::Max(0.f,static_cast<float>(-b.Position.Z/100))*.0980665f;
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
