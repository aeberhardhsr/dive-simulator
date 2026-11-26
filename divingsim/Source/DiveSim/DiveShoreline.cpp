#include "DiveWorld.h"
#include "Simulation/ShoreTransition.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"

void ADiveWorld::CreateShoreline()
{
    for(const auto& grid:Terrain.Shores)
    {
        // Integer-metre vertices align with streamed bathymetry mesh vertices.
        // Full bathymetry triangles remain exclusively in the streamed mesh.
        const Dive::ShoreTransition transition(grid,Terrain.Landscape);
        const int columns=FMath::RoundToInt(transition.East-transition.West)+1;
        const int rows=FMath::RoundToInt(transition.North-transition.South)+1;
        TArray<FVector> vertices,normals; TArray<int32> indices;
        TArray<FVector2D> uv; TArray<FLinearColor> colors; TArray<FProcMeshTangent> tangents;
        TArray<bool> measured;
        auto backgroundNormal=[&](double east,double north)
        {
            const auto& land=Terrain.Landscape;
            const double x=(east-land.West)/land.Spacing-.5,y=(land.North-north)/land.Spacing-.5;
            const int col=FMath::FloorToInt(x),row=FMath::FloorToInt(y);
            auto at=[&](int c,int r)
            {
                auto h=[&](int xx,int yy){return land.Heights[static_cast<size_t>(yy)*land.Columns+xx];};
                const int xn=FMath::Max(0,c-1),xp=FMath::Min(land.Columns-1,c+1),yn=FMath::Max(0,r-1),yp=FMath::Min(land.Rows-1,r+1);
                return FVector(-(h(c,yn)-h(c,yp))/((yp-yn)*land.Spacing),
                    -(h(xp,r)-h(xn,r))/((xp-xn)*land.Spacing),1).GetSafeNormal();
            };
            const double tx=x-col,ty=y-row;const FVector a=at(col,row),b=at(col+1,row),c=at(col,row+1),d=at(col+1,row+1);
            return (tx+ty<=1?a+(b-a)*tx+(c-a)*ty:d+(c-d)*(1-tx)+(b-d)*(1-ty)).GetSafeNormal();
        };
        const FVector origin=Terrain.ToWorld(transition.West,transition.North);
        for(int row=0;row<rows;++row) for(int col=0;col<columns;++col)
        {
            const double east=transition.West+col,north=transition.North-row;
            FVector p=Terrain.ToWorld(east,north); float z=0,bed=0,n=0,s=0,e=0,w=0;
            Terrain.SampleShore(p.X,p.Y,z);
            measured.Add(Terrain.Sample(p.X,p.Y,bed));
            if(!Terrain.SampleShore(p.X+100,p.Y,n)) n=z;
            if(!Terrain.SampleShore(p.X-100,p.Y,s)) s=z;
            if(!Terrain.SampleShore(p.X,p.Y+100,e)) e=z;
            if(!Terrain.SampleShore(p.X,p.Y-100,w)) w=z;
            const double blend=transition.Weight(east,north);
            const FVector geometricNormal=FVector(-(n-s)/200,-(e-w)/200,1).GetSafeNormal();
            const FVector normal=FMath::Lerp(backgroundNormal(east,north),geometricNormal,blend).GetSafeNormal();
            vertices.Add(FVector(p.X-origin.X,p.Y-origin.Y,z)); normals.Add(normal);
            uv.Add(FVector2D((east-grid.West)/(grid.Columns*grid.Spacing),(grid.North-north)/(grid.Rows*grid.Spacing)));
            tangents.Add(FProcMeshTangent(1,0,0));
            const float green=FMath::Clamp((z-100)/400.f,0.f,1.f)*FMath::Clamp(static_cast<float>((normal.Z-.6)*3),0.f,1.f);
            colors.Add(FMath::Lerp(FLinearColor(.24f,.23f,.19f),FLinearColor(.08f,.16f,.055f),green));
            colors.Last().A=blend;
        }
        for(int row=0;row<rows-1;++row) for(int col=0;col<columns-1;++col)
        {
            const int a=row*columns+col;
            if(!(measured[a]&&measured[a+columns]&&measured[a+columns+1])) indices.Append({a,a+columns,a+columns+1});
            if(!(measured[a]&&measured[a+columns+1]&&measured[a+1])) indices.Append({a,a+columns+1,a+1});
        }
        auto* mesh=NewObject<UProceduralMeshComponent>(this);
        mesh->SetupAttachment(RootComponent); mesh->SetRelativeLocation(FVector(origin.X,origin.Y,0));
        mesh->bUseAsyncCooking=true; mesh->SetCollisionProfileName(TEXT("BlockAll")); mesh->RegisterComponent();
        mesh->CreateMeshSection_LinearColor(0,vertices,indices,normals,uv,colors,tangents,true);
        mesh->SetMaterial(0,ImageryMaterial(TEXT("M_ShorePhoto"),Terrain.ShoreImagery[ShoreMeshes.Num()]));
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
        const auto& source=Terrain.Shores[i];const Dive::ShoreTransition boundary(source,Terrain.Landscape);
        const auto* section=mesh->GetProcMeshSection(0);int seamPoints=0,interiorPoints=0,bedPoints=0;double maxError=0,sourceError=0;
        for(const auto& vertex:section->ProcVertexBuffer)
        {
            const FVector p=mesh->GetComponentTransform().TransformPosition(FVector(vertex.Position));
            const auto survey=Terrain.ToSurvey(p);
            const double edge=FMath::Min(FMath::Min(FMath::Abs(survey.X-boundary.West),FMath::Abs(survey.X-boundary.East)),
                FMath::Min(FMath::Abs(survey.Y-boundary.South),FMath::Abs(survey.Y-boundary.North)));
            float bed=0;double background=0,original=0;
            if(Terrain.Sample(p.X,p.Y,bed))
            { sourceError=FMath::Max(sourceError,FMath::Abs(p.Z-bed));++bedPoints;continue; }
            if(edge>=Dive::ShoreTransition::BandMetres)
            {
                if(!source.Sample(survey.X,survey.Y,original)) { Error=TEXT("Missing original shore interior");return false; }
                sourceError=FMath::Max(sourceError,FMath::Abs(p.Z-original*100));++interiorPoints;
            }
            if(edge>.001||p.Z<=100) continue;
            if(!Terrain.Landscape.SampleTriangle(survey.X,survey.Y,background)) { Error=TEXT("Missing terrain seam background");return false; }
            maxError=FMath::Max(maxError,FMath::Abs(p.Z-background*100));++seamPoints;
        }
        if(seamPoints<100||interiorPoints<100||bedPoints<100||maxError>.05||sourceError>.05)
        { Error=TEXT("Shore source boundary or interior preservation failed");return false; }
        UE_LOG(LogTemp,Display,TEXT("DIVE_SHORE_SEAM_PASS entry=%d points=%d max_error_cm=%.6f interior=%d bed=%d source_error_cm=%.6f"),
            i,seamPoints,maxError,interiorPoints,bedPoints,sourceError);
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
