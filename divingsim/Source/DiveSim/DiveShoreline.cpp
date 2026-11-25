#include "DiveWorld.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

void ADiveWorld::CreateShoreline()
{
    for(const auto& grid:Terrain.Shores)
    {
        // Integer-metre vertices align with streamed bathymetry mesh vertices.
        // Full bathymetry triangles remain exclusively in the streamed mesh.
        constexpr int N=383;
        TArray<FVector> vertices,normals; TArray<int32> indices;
        TArray<FVector2D> uv; TArray<FLinearColor> colors; TArray<FProcMeshTangent> tangents;
        TArray<bool> measured;
        const FVector origin=Terrain.ToWorld(grid.West+1,grid.North-1);
        for(int row=0;row<N;++row) for(int col=0;col<N;++col)
        {
            const double east=grid.West+1+col,north=grid.North-1-row;
            FVector p=Terrain.ToWorld(east,north); float z=0,bed=0,n=0,s=0,e=0,w=0;
            Terrain.SampleShore(p.X,p.Y,z);
            measured.Add(Terrain.Sample(p.X,p.Y,bed));
            if(!Terrain.SampleShore(p.X+100,p.Y,n)) n=z;
            if(!Terrain.SampleShore(p.X-100,p.Y,s)) s=z;
            if(!Terrain.SampleShore(p.X,p.Y+100,e)) e=z;
            if(!Terrain.SampleShore(p.X,p.Y-100,w)) w=z;
            const FVector normal=FVector(-(n-s)/200,-(e-w)/200,1).GetSafeNormal();
            vertices.Add(FVector(p.X-origin.X,p.Y-origin.Y,z)); normals.Add(normal);
            uv.Add(FVector2D(east/3,north/3)); tangents.Add(FProcMeshTangent(1,0,0));
            const float green=FMath::Clamp((z-100)/400.f,0.f,1.f)*FMath::Clamp(static_cast<float>((normal.Z-.6)*3),0.f,1.f);
            colors.Add(FMath::Lerp(FLinearColor(.24f,.23f,.19f),FLinearColor(.08f,.16f,.055f),green));
        }
        for(int row=0;row<N-1;++row) for(int col=0;col<N-1;++col)
        {
            const int a=row*N+col;
            if(!(measured[a]&&measured[a+N]&&measured[a+N+1])) indices.Append({a,a+N,a+N+1});
            if(!(measured[a]&&measured[a+N+1]&&measured[a+1])) indices.Append({a,a+N+1,a+1});
        }
        auto* mesh=NewObject<UProceduralMeshComponent>(this);
        mesh->SetupAttachment(RootComponent); mesh->SetRelativeLocation(FVector(origin.X,origin.Y,0));
        mesh->bUseAsyncCooking=true; mesh->SetCollisionProfileName(TEXT("BlockAll")); mesh->RegisterComponent();
        mesh->CreateMeshSection_LinearColor(0,vertices,indices,normals,uv,colors,tangents,true);
        mesh->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_Shore.M_Shore")));
        ShoreMeshes.Add(mesh);
        UE_LOG(LogTemp,Display,TEXT("WALENSEE_SHORE_MESH patch=%d vertices=%d triangles=%d"),ShoreMeshes.Num()-1,vertices.Num(),indices.Num()/3);
    }
}
bool ADiveWorld::ValidateShoreline(FString& Error) const
{
    if(ShoreMeshes.Num()!=3||Terrain.ShoreEntries.Num()!=3||Accesses.Num()!=3||AccessRamps.Num()!=3)
    { Error=TEXT("Missing shore patches or accesses"); return false; }
    for(int i=0;i<3;++i)
    {
        auto* mesh=ShoreMeshes[i].Get(); const auto& entry=Terrain.ShoreEntries[i];
        float ground=0;
        if(!mesh||!mesh->GetProcMeshSection(0)||mesh->GetProcMeshSection(0)->ProcIndexBuffer.Num()<300||
           !Terrain.SampleGround(entry.Position.X,entry.Position.Y,ground)||ground<30||
           FMath::Abs(entry.Position.Z-ground-170)>1)
        { Error=TEXT("Invalid shore mesh or dry start"); return false; }
        const auto& access=Accesses[i];
        for(double d=0;d<access.Length;d+=100)
        {
            const FVector p=access.Origin+access.Forward*d;
            const double rampZ=access.Origin.Z-FMath::Max(0.,d-access.Landing)*access.Drop/(access.Length-access.Landing);
            float floor=0,above=0,below=0;
            if(!Terrain.SampleGround(p.X,p.Y,floor)||!SupportZ(p.X,p.Y,above,rampZ+1)||
                !SupportZ(p.X,p.Y,below,rampZ-1)||FMath::Abs(above-rampZ)>.1||FMath::Abs(below-floor)>.1)
            { Error=TEXT("Access support must distinguish above and below the ramp"); return false; }
        }
    }
    Error.Empty(); return true;
}
