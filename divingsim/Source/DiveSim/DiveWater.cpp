#include "DiveWorld.h"
#include "DivePawn.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"

void ADiveWorld::CreateWater()
{
    constexpr int N=257;
    TArray<FVector> vertices,normals;
    TArray<int32> indices,boundary;
    TArray<FVector2D> uv;
    TArray<FLinearColor> colors;
    TArray<FProcMeshTangent> tangents;
    auto vertex=[&](double x,double y)
    {
        const int index=vertices.Add(FVector(x,y,0)); normals.Add(FVector::UpVector);
        uv.Add(FVector2D(x/100,y/100)); colors.Add(FLinearColor::White);
        tangents.Add(FProcMeshTangent(1,0,0)); return index;
    };
    // Half-metre geometry around the viewer; the outer rings carry flat water.
    for(int y=0;y<N;++y) for(int x=0;x<N;++x) vertex((x-128)*50.,(y-128)*50.);
    for(int y=0;y<N-1;++y) for(int x=0;x<N-1;++x)
    { const int a=y*N+x; indices.Append({a,a+N,a+1,a+1,a+N,a+N+1}); }
    for(int i=0;i<N-1;++i) boundary.Add(i);
    for(int i=0;i<N-1;++i) boundary.Add(i*N+N-1);
    for(int i=N-1;i>0;--i) boundary.Add((N-1)*N+i);
    for(int i=N-1;i>0;--i) boundary.Add(i*N);
    for(double radius=12800;radius<=1638400;radius*=2)
    {
        TArray<int32> outer;
        for(const int index:boundary)
        { const FVector p=vertices[index]; const double s=radius/FMath::Max(FMath::Abs(p.X),FMath::Abs(p.Y)); outer.Add(vertex(p.X*s,p.Y*s)); }
        for(int i=0;i<boundary.Num();++i)
        { const int j=(i+1)%boundary.Num(); indices.Append({boundary[i],boundary[j],outer[i],boundary[j],outer[j],outer[i]}); }
        boundary=MoveTemp(outer);
    }
    Water->CreateMeshSection_LinearColor(0,vertices,indices,normals,uv,colors,tangents,false);
    // CPU vertices are flat; include up to 0.675 m shader displacement in culling.
    Water->GetProcMeshSection(0)->SectionLocalBox=Water->GetProcMeshSection(0)->SectionLocalBox.ExpandBy(FVector(0,0,100));
    const FProcMeshSection expanded=*Water->GetProcMeshSection(0);
    Water->SetProcMeshSection(0,expanded);
    UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_MESH vertices=%d triangles=%d"),vertices.Num(),indices.Num()/3);
}

double ADiveWorld::SurfaceZ(double X,double Y,double Seconds) const
{
    if(Seconds<0) Seconds=Diver?Diver->Simulation.Data.ElapsedSeconds:0;
    const FVector player=Diver?Diver->GetActorLocation():PopulationOrigin;
    const double d=FVector2D(X-player.X,Y-player.Y).Size()/100;
    const double f=FMath::Clamp((d-32)/16,0.,1.);
    return Waves.Sample(X/100,Y/100,Seconds).Height*100*(1-f*f*(3-2*f));
}

double ADiveWorld::PressureDepth(const FVector& Position,double Seconds) const
{
    // Pressure uses the physical wave field, without the distant rendering fade.
    return Waves.PressureDepth(Position.X/100,Position.Y/100,Position.Z/100,Seconds);
}

FVector ADiveWorld::WaveVelocity(const FVector& Position,double Seconds) const
{
    const auto v=Waves.Velocity(Position.X/100,Position.Y/100,Position.Z/100,Seconds);
    return FVector(v.X,v.Y,v.Z);
}

void ADiveWorld::UpdateWater(const FVector& Player)
{
    const double t=Diver?Diver->Simulation.Data.ElapsedSeconds:0;
    Water->SetWorldLocation(FVector(FMath::GridSnap(Player.X,3200.),FMath::GridSnap(Player.Y,3200.),0));
    if(WaterMaterial)
    {
        WaterMaterial->SetScalarParameterValue(TEXT("WaveTime"),t);
        WaterMaterial->SetVectorParameterValue(TEXT("Viewer"),FLinearColor(Player.X/100,Player.Y/100,0,0));
    }
    for(int i=0;i<Buoys.Num();++i)
    {
        const FVector p=Buoys[i]->GetComponentLocation();
        const double z=SurfaceZ(p.X,p.Y,t);
        const FVector normal(-(SurfaceZ(p.X+10,p.Y,t)-SurfaceZ(p.X-10,p.Y,t))/20,
            -(SurfaceZ(p.X,p.Y+10,t)-SurfaceZ(p.X,p.Y-10,t))/20,1);
        Buoys[i]->SetWorldLocation(FVector(p.X,p.Y,z+15));
        Buoys[i]->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector,normal.GetSafeNormal()));
        const double bottom=GroundZ(p.X,p.Y);
        BuoyLines[i]->SetWorldLocation(FVector(p.X,p.Y,(bottom+z)*.5));
        BuoyLines[i]->SetWorldScale3D(FVector(.025,.025,(z-bottom)/100));
    }
    Rain->ClearInstances();
    const FVector eye=Diver?Diver->GetPawnViewLocation():Player;
    if(WeatherSettings.Weather==2&&eye.Z>SurfaceZ(eye.X,eye.Y)-150)
    {
        const double direction=FMath::DegreesToRadians(WeatherSettings.WindDirectionDegrees);
        const FVector velocity(WeatherSettings.WindMetresPerSecond*50*FMath::Cos(direction),
            WeatherSettings.WindMetresPerSecond*50*FMath::Sin(direction),-900);
        const FQuat rotation=FQuat::FindBetweenNormals(FVector::UpVector,velocity.GetSafeNormal());
        for(int i=0;i<1600;++i)
        {
            const FVector p(eye.X+FMath::Fmod(i*137.51+t*velocity.X+1.e8,1200.)-600,
                eye.Y+FMath::Fmod(i*273.17+t*velocity.Y+1.e8,1200.)-600,
                eye.Z+FMath::Fmod(i*91.37+t*velocity.Z+1.e8,1200.)-200);
            if(p.Z>SurfaceZ(p.X,p.Y,t)+10) Rain->AddInstance(FTransform(rotation,p,FVector(.003,.003,.16)));
        }
    }
}

bool ADiveWorld::ValidateWater(FString& Error) const
{
    const auto* fog=Fog?Fog->GetComponent():nullptr;
    const float water=static_cast<float>(OpticalMedium.Water);
    const auto albedo=FMath::Lerp(FLinearColor::White,FLinearColor(FColor(75,135,110)),water).ToFColorSRGB();
    if(!fog||FMath::Abs(fog->FogDensity-OpticalMedium.Density)>1.e-6||
        FMath::Abs(fog->FogHeightFalloff-OpticalMedium.HeightFalloff)>1.e-6||
        !fog->FogInscatteringLuminance.Equals(FLinearColor(280,1150,950)*(water*WaterAmbient),.001f)||
        !fog->SkyAtmosphereAmbientContributionColorScale.Equals(FLinearColor(1-water,1-water,1-water),1.e-5f)||
        fog->VolumetricFogAlbedo!=albedo)
    { Error=TEXT("Air/water fog parameters do not match camera medium"); return false; }
    const auto* section=Water?Water->GetProcMeshSection(0):nullptr;
    if(!section||section->ProcVertexBuffer.Num()<66000||!WaterMaterial||!CloudMaterial||!Rain->GetMaterial(0))
    { Error=TEXT("Missing water geometry or environment materials"); return false; }
    if(section->SectionLocalBox.Min.Z>-100||section->SectionLocalBox.Max.Z<100)
    { Error=TEXT("Wave displacement outside culling bounds"); return false; }
    for(int i=0;i<4;++i)
    {
        const auto& w=Waves.Waves[i];
        const FLinearColor value=WaterMaterial->K2_GetVectorParameterValue(FName(*FString::Printf(TEXT("Wave%d"),i)));
        if(FMath::Abs(value.R-w.Kx)>1.e-5||FMath::Abs(value.G-w.Ky)>1.e-5||FMath::Abs(value.B-w.Omega)>1.e-5||FMath::Abs(value.A-w.Amplitude)>1.e-5)
        { Error=TEXT("CPU and material wave coefficients differ"); return false; }
    }
    return true;
}
