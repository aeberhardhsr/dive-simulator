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

float ADiveWorld::GroundZ(float X,float Y)
{
    const float slope=(X+8000)*.19f;
    const float shelf=440.f/(1.f+FMath::Exp(-(X+3000)/400.f));
    return -260-slope-shelf+75*FMath::Sin(X*.0011f)*FMath::Cos(Y*.0014f)
        +38*FMath::Sin(Y*.003f+X*.002f);
}

void ADiveWorld::CreateTerrain()
{
    constexpr int N=101;
    TArray<FVector> vertices,normals; TArray<int32> indices;
    TArray<FVector2D> uv; TArray<FLinearColor> colors; TArray<FProcMeshTangent> tangents;
    for(int y=0;y<N;++y) for(int x=0;x<N;++x)
    {
        const float px=-10000+x*200.f, py=-10000+y*200.f;
        vertices.Add(FVector(px,py,GroundZ(px,py))); uv.Add(FVector2D(px/300,py/300));
        const FVector normal=FVector(-(GroundZ(px+10,py)-GroundZ(px-10,py))/20,
            -(GroundZ(px,py+10)-GroundZ(px,py-10))/20,1).GetSafeNormal();
        normals.Add(normal); tangents.Add(FProcMeshTangent(1,0,0));
        const float variation=.82f+Random.FRand()*.18f;
        colors.Add(FLinearColor(.25f,.30f,.23f)*variation);
        if(x<N-1 && y<N-1)
        {
            const int a=y*N+x;
            indices.Append({a,a+N,a+1,a+1,a+N,a+N+1});
        }
    }
    Ground->CreateMeshSection_LinearColor(0,vertices,indices,normals,uv,colors,tangents,true);
    Ground->SetMaterial(0,Mat(TEXT("M_Lakebed")));
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
    Rocks->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Rock.SM_Rock")));
    Rocks->SetMaterial(0,Mat(TEXT("M_Rock")));
    Plants->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Pondweed.SM_Pondweed")));
    Plants->SetMaterial(0,Mat(TEXT("M_Plant")));
    Fish->SetStaticMesh(Mesh(TEXT("/Game/Diving/Meshes/SM_Perch.SM_Perch")));
    Fish->SetMaterial(0,Mat(TEXT("M_Fish")));
    Bubbles->SetStaticMesh(Mesh(Sphere)); Bubbles->SetMaterial(0,Mat(TEXT("M_Bubble")));
    Particles->SetStaticMesh(Mesh(Sphere)); Particles->SetMaterial(0,Mat(TEXT("M_Particle")));
    for(int i=0;i<350;++i)
    {
        const float x=Random.FRandRange(-8500,8500), y=Random.FRandRange(-8500,8500);
        const float size=Random.FRandRange(.4,2.6);
        Rocks->AddInstance(FTransform(FRotator(Random.FRandRange(-15,15),Random.FRandRange(0,360),Random.FRandRange(-20,20)),
            FVector(x,y,GroundZ(x,y)+size*15),FVector(size,size*.85,size*.7)));
    }
    // A limestone escarpment and a sheltered shallow vegetation zone.
    for(int i=0;i<25;++i)
    {
        const float y=-7000+i*600.f, x=-2200+200*FMath::Sin(i*.6f);
        Rocks->AddInstance(FTransform(FRotator(12,i*47.f,0),FVector(x,y,GroundZ(x,y)+180),FVector(4,4,5)));
    }
    for(int i=0;i<1600;++i)
    {
        const float x=Random.FRandRange(-8200,-1800), y=Random.FRandRange(-8000,8000);
        if(FMath::Sin(x*.002f)+FMath::Cos(y*.003f)<-.1) continue;
        const float size=Random.FRandRange(.45,1.3);
        Plants->AddInstance(FTransform(FRotator(0,Random.FRandRange(0,360),0),
            FVector(x,y,GroundZ(x,y)+2),FVector(size)));
    }
    for(int i=0;i<54;++i)
    {
        const FVector pos(-5600+Random.FRandRange(-850,1800),-1800+Random.FRandRange(-1500,4200),Random.FRandRange(-700,-280));
        FDiveFish fish; fish.Position=pos; fish.Velocity=FVector(Random.FRandRange(20,50),10,0); fish.Phase=Random.FRandRange(0,6.28);
        FishState.Add(fish);
        Fish->AddInstance(FTransform(FRotator::ZeroRotator,pos,FVector(Random.FRandRange(.8,1.6))));
    }
    for(int i=0;i<3;++i)
    {
        const float y=i*1800-1800;
        Prop(FString::Printf(TEXT("EntryBuoy%d"),i),Sphere,FVector(-6500,y,15),FVector(.55,.55,.75),TEXT("M_Buoy"),false);
        auto* rope=Prop(FString::Printf(TEXT("EntryLine%d"),i),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
            FVector(-6500,y,-350),FVector(.025,.025,7),TEXT("M_Rope"),false);
        rope->SetCastShadow(false);
    }
    auto* log=Prop(TEXT("FallenTree"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
        FVector(-4300,-1000,GroundZ(-4300,-1000)+70),FVector(.55,.55,7),TEXT("M_Wood"));
    log->SetRelativeRotation(FRotator(80,35,0));
    // Shore and distant silhouettes are original procedural scenery, not geodata.
    for(int i=0;i<18;++i)
        Prop(FString::Printf(TEXT("Mountain%d"),i),TEXT("/Game/Diving/Meshes/SM_Rock.SM_Rock"),
            FVector(-17000+Random.FRandRange(-3000,3000),-24000+i*3000,2500),FVector(40,45,Random.FRandRange(35,70)),TEXT("M_Rock"));
}

void ADiveWorld::BeginPlay()
{
    Super::BeginPlay();
    CreateTerrain(); Populate();
    Water->SetStaticMesh(Mesh(TEXT("/Engine/BasicShapes/Plane.Plane")));
    Water->SetRelativeScale3D(FVector(600,600,1)); Water->SetMaterial(0,Mat(TEXT("M_Water")));
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
    const FVector player=Diver?Diver->GetActorLocation():FVector(-6500,0,-200);
    if(Fog)
    {
        const float underwater=FMath::Clamp(static_cast<float>(-player.Z)/70,0.f,1.f);
        Fog->GetComponent()->SetFogDensity(FMath::Lerp(.002f,.35f*14/VisibleDistance,underwater));
    }
    for(int i=0;i<FishState.Num();++i)
    {
        auto& fish=FishState[i];
        const FVector center(-4500+600*FMath::Sin(WorldTime*.03f),i%2?0:-1800,-600);
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
        fish.Position+=(fish.Velocity+FVector(0,Flow*100,0))*dt;
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
