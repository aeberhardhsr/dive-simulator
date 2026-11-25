#include "DiveWorld.h"
#include "DivePawn.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"

void ADiveWorld::ResetSediment()
{
    bSiltTestHidden=false;Silt.Reset();SiltBorn=0;SiltCredit[0]=SiltCredit[1]=0;SiltRenderClock=0;
    for(const auto& volume:SiltVolumes) volume->SetVisibility(false);
}

void ADiveWorld::DisturbSediment(int32 Fin,const FVector& Position,double Speed,double Step)
{
    if(Fin<0||Fin>1||Step<=0) return;
    FHitResult hit;FCollisionQueryParams query(SCENE_QUERY_STAT(FinSediment),false,Diver);
    const bool found=GetWorld()->LineTraceSingleByChannel(hit,Position,Position-FVector(0,0,80),ECC_Visibility,query);
    bool soft=found&&(hit.GetComponent()==Ground||ShoreMeshes.Contains(Cast<UProceduralMeshComponent>(hit.GetComponent())));
    if(found&&!soft) for(const auto& chunk:Chunks) if(chunk.Value==hit.GetComponent()) { soft=true;break; }
    const double rate=Dive::Sediment::StirRate(found?hit.Distance/100:1,Speed,
        Position.Z<SurfaceZ(Position.X,Position.Y)-10,soft&&hit.ImpactNormal.Z>.7);
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&Fin==0&&Diver&&FMath::Fmod(Diver->Simulation.Data.ElapsedSeconds,1.)<Step)
        UE_LOG(LogTemp,Display,TEXT("DIVE_SEDIMENT_SOURCE hit=%d soft=%d clearance=%.4f speed=%.4f rate=%.4f credit=%.4f component=%s"),
            found,soft,hit.Distance/100,Speed,rate,SiltCredit[Fin],hit.GetComponent()?*hit.GetComponent()->GetName():TEXT("none"));
    // The speed passes through zero between strokes. Preserve fractional
    // emission across that reversal rather than requiring a whole puff per half-kick.
    if(rate<=0) { SiltCredit[Fin]*=FMath::Exp(-Step);return; }
    SiltCredit[Fin]+=rate*Step;
    if(SiltCredit[Fin]<1) return;
    SiltCredit[Fin]-=1;
    // Drop new visual puffs at saturation; never replace a still-visible cloud.
    if(Silt.Num()>=24) return;
    FDiveSilt puff;puff.Origin=hit.ImpactPoint;puff.Position=puff.Origin+FVector(0,0,12);
    puff.Model.Strength=FMath::Clamp(rate/2,.25,1.5);puff.Side=Fin?1:-1;
    Silt.Add(puff);++SiltBorn;
}

void ADiveWorld::StepSediment(double Step)
{
    for(int i=Silt.Num()-1;i>=0;--i)
    {
        auto& puff=Silt[i];puff.Model.Step(Step);
        const double age=puff.Model.Age;
        const FVector wave=WaveVelocity(puff.Position,Diver?Diver->Simulation.Data.ElapsedSeconds:0);
        puff.Position+=(FVector(0,Flow,0)+wave)*Step*100;
        // Small opposing lobes spread the plume; no random frame-rate dependence.
        puff.Position.X+=puff.Side*12*(FMath::Exp(-(age-Step)/3)-FMath::Exp(-age/3));
        float floor=0;
        if(!Terrain.Sample(puff.Position.X,puff.Position.Y,floor)||!puff.Model.Alive()||
            (Diver&&FVector::DistSquared(puff.Position,Diver->GetActorLocation())>FMath::Square(4000.)))
        { Silt.RemoveAtSwap(i);continue; }
        puff.Position.Z=FMath::Max(puff.Origin.Z+puff.Model.Lift()*100,static_cast<double>(floor)+6);
        // Do not render suspended sediment above the local water surface.
        if(puff.Position.Z+puff.Model.Radius()*100>SurfaceZ(puff.Position.X,puff.Position.Y))
        { Silt.RemoveAtSwap(i);continue; }
    }
}

void ADiveWorld::RenderSediment(float Dt)
{
    SiltRenderClock+=Dt;if(SiltRenderClock<.05f) return;SiltRenderClock=0;
    while(SiltVolumes.Num()<Silt.Num())
    {
        auto* mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
        auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_FinSediment.M_FinSediment"));
        if(!mesh||!material) { UE_LOG(LogTemp,Error,TEXT("DIVE_SEDIMENT_FAIL missing volume resources"));return; }
        auto* volume=NewObject<UStaticMeshComponent>(this);
        volume->SetMobility(EComponentMobility::Movable);volume->SetStaticMesh(mesh);
        volume->SetCollisionEnabled(ECollisionEnabled::NoCollision);volume->SetCastShadow(false);
        volume->bAffectDynamicIndirectLighting=false;volume->bAffectDistanceFieldLighting=false;volume->bVisibleInRayTracing=false;
        auto* dynamic=UMaterialInstanceDynamic::Create(material,this);volume->SetMaterial(0,dynamic);
        volume->RegisterComponent();SiltVolumes.Add(volume);SiltMaterials.Add(dynamic);
    }
    for(int i=0;i<SiltVolumes.Num();++i)
    {
        auto* volume=SiltVolumes[i].Get();volume->SetVisibility(i<Silt.Num()&&!bSiltTestHidden);
        if(i>=Silt.Num()) continue;
        const auto& puff=Silt[i];auto* material=SiltMaterials[i].Get();
        FVector center=puff.Position;double radius=puff.Model.Radius()*100,optical=puff.Model.OpticalDepth();
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&FParse::Param(FCommandLine::Get(),TEXT("DiveSedimentProbe"))&&Diver)
        {
            volume->SetVisibility(i==0&&!bSiltTestHidden);
            center=Diver->Camera->GetComponentLocation()+Diver->Camera->GetForwardVector()*
                (FParse::Param(FCommandLine::Get(),TEXT("DiveSedimentProbeInside"))?0:250);
            radius=120;optical=10;material->SetVectorParameterValue(TEXT("Probe"),FLinearColor(20000,0,20000));
        }
        volume->SetWorldLocation(center);volume->SetWorldScale3D(FVector(radius/50));
        material->SetVectorParameterValue(TEXT("Center"),FLinearColor(center.X,center.Y,center.Z));
        material->SetScalarParameterValue(TEXT("Radius"),radius);
        material->SetScalarParameterValue(TEXT("Optical"),optical);
        material->SetScalarParameterValue(TEXT("Age"),puff.Model.Age);
        // Same authored ambient proxy as the water medium. This represents
        // scattered illumination; volumes do not emit light into the scene.
        material->SetVectorParameterValue(TEXT("AmbientScattering"),FLinearColor(6000,4500,3000)*WaterAmbientAt(center));
    }
}

void ADiveWorld::HideSedimentForTest(bool Hidden)
{
    if(!FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))) return;
    bSiltTestHidden=Hidden;
    for(int i=0;i<SiltVolumes.Num();++i) SiltVolumes[i]->SetVisibility(i<Silt.Num()&&!Hidden);
}

bool ADiveWorld::ValidateSediment(FString& Detail) const
{
    bool valid=Silt.Num()<=24&&SiltVolumes.Num()<=24&&SiltMaterials.Num()==SiltVolumes.Num();
    double optical=0;
    for(const auto& puff:Silt)
    {
        valid&=!puff.Position.ContainsNaN()&&puff.Model.Age>=0&&puff.Model.Alive();
        optical+=puff.Model.OpticalDepth();
    }
    Detail=FString::Printf(TEXT("born=%d live=%d volumes=%d optical_sum=%.6f"),SiltBorn,Silt.Num(),SiltVolumes.Num(),optical);
    return valid;
}

FVector ADiveWorld::SedimentCenter() const
{
    FVector sum=FVector::ZeroVector;double weight=0;
    for(const auto& puff:Silt) { const double w=puff.Model.OpticalDepth();sum+=puff.Position*w;weight+=w; }
    return weight>0?sum/weight:FVector::ZeroVector;
}
