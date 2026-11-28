#include "DiveWorld.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Simulation/ShoreMovement.h"

bool ADiveWorld::SupportZ(double X,double Y,float& Z,double MaxAccessZ) const
{
    if(!Terrain.SampleGround(X,Y,Z)) return false;
    for(const auto& access:Accesses)
    {
        const FVector delta=FVector(X,Y,0)-FVector(access.Origin.X,access.Origin.Y,0);
        const double along=FVector::DotProduct(delta,access.Forward);
        const double across=FVector::DotProduct(delta,FVector(-access.Forward.Y,access.Forward.X,0));
        if(along>=-100&&along<=access.Length&&FMath::Abs(across)<=90)
        {
            const double fraction=FMath::Clamp((along-access.Landing)/(access.Length-access.Landing),0.,1.);
            const double rampZ=access.Origin.Z-access.Drop*fraction;
            // A swimmer below the structure must not be snapped onto its top.
            if(rampZ<=MaxAccessZ) Z=FMath::Max(Z,static_cast<float>(rampZ));
        }
    }
    return true;
}
FVector ADiveWorld::ShoreStart(int32 Index) const
{
    return Accesses[Index].Origin+FVector(0,0,Dive::ShoreMovement::FloorClearance*100);
}
void ADiveWorld::CreateAccess()
{
    AccessParts=NewObject<UInstancedStaticMeshComponent>(this);
    AccessParts->SetupAttachment(RootComponent);
    AccessParts->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    AccessParts->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_Equipment.M_Equipment")));
    AccessParts->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    AccessParts->RegisterComponent();
    AccessRails=NewObject<UInstancedStaticMeshComponent>(this);
    AccessRails->SetupAttachment(RootComponent); AccessRails->SetStaticMesh(AccessParts->GetStaticMesh());
    AccessRails->SetMaterial(0,AccessParts->GetMaterial(0)); AccessRails->SetCollisionProfileName(TEXT("BlockAll"));
    AccessRails->RegisterComponent();
    auto beam=[this](FVector A,FVector B,double Thickness)
    {
        const FVector delta=B-A;
        AccessRails->AddInstance(FTransform(delta.Rotation(),(A+B)*.5,FVector(delta.Size()/100,Thickness/100,Thickness/100)));
    };
    for(const auto& entry:Terrain.ShoreEntries)
    {
        FDiveAccess access; access.Forward=FRotator(0,entry.Yaw,0).Vector();
        const FVector right(-access.Forward.Y,access.Forward.X,0);
        access.Origin=entry.Position;
        float top=-30000;
        for(int x=-1;x<=1;++x) for(int y=-1;y<=1;++y)
        {
            const FVector p=entry.Position+access.Forward*(x*50)+right*(y*90);
            float z=0; Terrain.SampleGround(p.X,p.Y,z); top=FMath::Max(top,z);
        }
        access.Origin.Z=top+15;
        access.Steps=FMath::CeilToInt((access.Origin.Z+180)/18);
        access.Drop=access.Steps*18.;
        // Find an elevated, continuous route above the retained survey terrain.
        bool fitted=false;
        for(int landing=1;landing<=12&&!fitted;++landing)
        {
            access.Landing=landing*100.; access.Length=access.Landing+access.Steps*32.;
            fitted=true;
            for(double d=0;d<=access.Length;d+=16) for(double side:{-85.,0.,85.})
            {
                const FVector p=access.Origin+access.Forward*d+right*side;
                float ground=0;
                const double z=access.Origin.Z-FMath::Max(0.,d-access.Landing)*18/32;
                if(!Terrain.SampleGround(p.X,p.Y,ground)||ground>z-5) fitted=false;
            }
        }
        if(!fitted) { UE_LOG(LogTemp,Error,TEXT("DIVE_ACCESS_FAIL cannot fit %s"),*entry.Name); bTerrainReady=false; Terrain.Error=TEXT("Zugang konnte nicht aufgebaut werden."); return; }
        Accesses.Add(access);
        const FRotator rotation=access.Forward.Rotation();
        // Visible treads; a separate continuous ramp is the walk collision.
        AccessParts->AddInstance(FTransform(rotation,access.Origin+access.Forward*((access.Landing-100)*.5)-FVector(0,0,6),
            FVector((access.Landing+100)/100,1.8,.12)));
        for(int step=0;step<access.Steps;++step)
        {
            const FVector p=access.Origin+access.Forward*(access.Landing+(step+.5)*32)-FVector(0,0,(step+.5)*18+4);
            AccessParts->AddInstance(FTransform(rotation,p,FVector(.34,1.8,.08)));
        }
        for(double side:{-90.,90.})
        {
            const FVector a=access.Origin-access.Forward*100+right*side;
            const FVector b=access.Origin+access.Forward*access.Landing+right*side;
            const FVector c=access.Origin+access.Forward*access.Length+right*side-FVector(0,0,access.Drop);
            beam(a+FVector(0,0,100),b+FVector(0,0,100),5);
            beam(b+FVector(0,0,100),c+FVector(0,0,100),5);
            for(double d=-100;d<=access.Length;d+=140)
            {
                const double z=access.Origin.Z-FMath::Max(0.,d-access.Landing)*18/32;
                const FVector p=FVector(access.Origin.X,access.Origin.Y,z)+access.Forward*d+right*side;
                beam(p,p+FVector(0,0,100),5);
            }
        }
        auto* ramp=NewObject<UProceduralMeshComponent>(this); ramp->SetupAttachment(RootComponent);
        ramp->SetRelativeLocation(access.Origin); ramp->bUseAsyncCooking=false;
        ramp->SetCollisionProfileName(TEXT("BlockAll")); ramp->RegisterComponent();
        TArray<FVector> vertices;
        for(const FVector centre:{-access.Forward*100,access.Forward*access.Landing,access.Forward*access.Length-FVector(0,0,access.Drop)})
        { vertices.Add(centre-right*90); vertices.Add(centre+right*90); }
        TArray<int32> indices={0,1,2,1,3,2,2,3,4,3,5,4};
        ramp->CreateMeshSection_LinearColor(0,vertices,indices,TArray<FVector>{},TArray<FVector2D>{},TArray<FLinearColor>{},TArray<FProcMeshTangent>{},true);
        ramp->SetVisibility(false); ramp->SetCastShadow(false); AccessRamps.Add(ramp);
        UE_LOG(LogTemp,Display,TEXT("DIVE_ACCESS_READY name=%s landing=%.2fm length=%.2fm steps=%d"),*entry.Name,access.Landing/100,access.Length/100,access.Steps);
    }
}
