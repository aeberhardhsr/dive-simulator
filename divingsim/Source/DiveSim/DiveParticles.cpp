#include "DiveWorld.h"
#include "DivePawn.h"
#include "Simulation/Bubbles.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
constexpr int32 GrainCount=640;
FVector NextGrainSeed(FRandomStream& random)
{
    const float x=random.FRand()*800-400,y=random.FRand()*800-400,z=random.FRand()*800-400;
    return FVector(x,y,z);
}
}

void ADiveWorld::InitializeParticles()
{
    if(ParticleMaterial) return;
    auto* sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    Bubbles->SetStaticMesh(sphere);
    Bubbles->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_AirBubble.M_AirBubble")));
    Particles->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Diving/Meshes/SM_SuspendedGrain.SM_SuspendedGrain")));
    Particles->SetMobility(EComponentMobility::Movable);
    Particles->SetEvaluateWorldPositionOffset(true);
    // The dedicated mesh includes conservative bounds for shader displacement.
    // Material WPO limits alone do not expand regular ISM culling bounds.
    auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_SuspendedWater.M_SuspendedWater"));
    if(!material) return;
    ParticleMaterial=UMaterialInstanceDynamic::Create(material,this);
    Particles->SetMaterial(0,ParticleMaterial);
    Particles->SetNumCustomDataFloats(3);
    FRandomStream field(62419);
    for(int i=0;i<GrainCount;++i)
    {
        const FVector seed=NextGrainSeed(field);
        Particles->AddInstance(FTransform(FRotator::ZeroRotator,seed,FVector::OneVector));
        for(int axis=0;axis<3;++axis) Particles->SetCustomDataValue(i,axis,seed[axis],false);
    }
    Particles->MarkRenderStateDirty();
    if(FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest")))
    {
        FParse::Value(FCommandLine::Get(),TEXT("DiveParticleScale="),ParticleScale);
        ParticleScale=FMath::IsFinite(ParticleScale)?FMath::Clamp(ParticleScale,0.f,30.f):1;
        bNoBubbles=FParse::Param(FCommandLine::Get(),TEXT("DiveNoBubbles"));
    }
}

void ADiveWorld::EmitBreath(const FVector& Position,const FVector& Direction,float Pressure)
{
    if(bNoBubbles) return;
    const FVector right=FVector::CrossProduct(FVector::UpVector,Direction).GetSafeNormal();
    for(int i=0;i<28;++i)
    {
        // Keep the prior frame's draw indices valid until UpdateParticles.
        // At saturation, omit new visual bubbles rather than renumbering live ones.
        if(BubbleState.Num()>=512) break;
        FDiveBubble bubble;
        bubble.Side=(i%2)?1:-1;
        bubble.Position=Position+Direction*8+right*(bubble.Side*7);
        bubble.StartPressure=Pressure; bubble.Age=-i*.036f;
        bubble.RadiusMetres=Random.FRandRange(.003f,.012f);bubble.Phase=Random.FRandRange(0.f,2*PI);
        BubbleState.Add(bubble);
    }
}

void ADiveWorld::UpdateParticleField(const FVector& player)
{
    const double seconds=Diver?Diver->Simulation.Data.ElapsedSeconds:0;
    if(ParticleMaterial)
    {
        // Only move the component when entering another 8 m cell. Every grain's
        // drift, wrap, boundary fade and wave-surface rejection run in the GPU WPO.
        const FVector anchor(FMath::FloorToDouble(player.X/800+.5)*800,
            FMath::FloorToDouble(player.Y/800+.5)*800,FMath::FloorToDouble(player.Z/800+.5)*800);
        const FVector previousAnchor=bParticleUniformsReady?ParticleAnchor:anchor;
        const FVector previousViewer=bParticleUniformsReady?LastParticleViewer:player;
        ParticleMaterial->SetVectorParameterValue(TEXT("PreviousAnchor"),FLinearColor(previousAnchor.X,previousAnchor.Y,previousAnchor.Z,0));
        ParticleMaterial->SetVectorParameterValue(TEXT("PreviousViewer"),FLinearColor(previousViewer.X,previousViewer.Y,previousViewer.Z,0));
        ParticleMaterial->SetScalarParameterValue(TEXT("PreviousFieldTime"),bParticleUniformsReady?LastParticleTime:WorldTime);
        ParticleMaterial->SetScalarParameterValue(TEXT("PreviousWaterTime"),bParticleUniformsReady?LastParticleWaterTime:seconds);
        LastParticleViewer=player;LastParticleTime=WorldTime;LastParticleWaterTime=seconds;bParticleUniformsReady=true;
        if(anchor!=ParticleAnchor) { ++ParticleCellMoves;ParticleAnchor=anchor;Particles->SetWorldLocation(anchor); }
        ParticleMaterial->SetVectorParameterValue(TEXT("Anchor"),FLinearColor(anchor.X,anchor.Y,anchor.Z,0));
        ParticleMaterial->SetVectorParameterValue(TEXT("Viewer"),FLinearColor(player.X,player.Y,player.Z,0));
        ParticleMaterial->SetScalarParameterValue(TEXT("FieldTime"),WorldTime);
        ParticleMaterial->SetScalarParameterValue(TEXT("FlowCm"),Flow*100);
        ParticleMaterial->SetScalarParameterValue(TEXT("GrainScale"),ParticleScale);
        ParticleMaterial->SetScalarParameterValue(TEXT("WaterTime"),Diver?Diver->Simulation.Data.ElapsedSeconds:0);
        for(int i=0;i<4;++i)
        {
            const auto& w=Waves.Waves[i];
            ParticleMaterial->SetVectorParameterValue(FName(*FString::Printf(TEXT("Wave%d"),i)),FLinearColor(w.Kx,w.Ky,w.Omega,w.Amplitude));
        }
        ParticleMaterial->SetVectorParameterValue(TEXT("WavePhase"),FLinearColor(Waves.Waves[0].Phase,Waves.Waves[1].Phase,Waves.Waves[2].Phase,Waves.Waves[3].Phase));
    }
}

void ADiveWorld::UpdateParticles(float dt,const FVector& player)
{
    const double seconds=Diver?Diver->Simulation.Data.ElapsedSeconds:0;
    ParticleSimulationSeconds=seconds;
    UpdateParticleField(player);
    Bubbles->ClearInstances();RenderedBubbleIndices.Reset();
    for(int i=BubbleState.Num()-1;i>=0;--i)
    {
        auto& b=BubbleState[i]; const float previousAge=b.Age;b.Age+=dt;
        if(b.Age<0) continue;
        if(previousAge<=0)
        {
            if(Diver&&Diver->Camera)
                b.Position=Diver->Camera->GetComponentLocation()-FVector(0,0,12)+Diver->Camera->GetForwardVector()*8+Diver->Camera->GetRightVector()*(b.Side*7);
            b.StartPressure=WeatherSettings.SurfacePressureBar+PressureDepth(b.Position,seconds)*Dive::BarPerMetre;
            ++BubblesBorn;
        }
        const float step=previousAge<0?b.Age:dt;
        const double beforePressure=WeatherSettings.SurfacePressureBar+PressureDepth(b.Position,seconds)*Dive::BarPerMetre;
        const double beforeRadius=Dive::Bubble::RadiusAt(b.RadiusMetres,b.StartPressure,beforePressure);
        const FVector wave=WaveVelocity(b.Position,Diver?Diver->Simulation.Data.ElapsedSeconds:0);
        b.Position+=(FVector(2*FMath::Cos(b.Age*5+b.Phase),Flow*100+12*b.Side*FMath::Exp(-b.Age*3)+2*FMath::Sin(b.Age*4+b.Phase),100*Dive::Bubble::RiseSpeed(beforeRadius))+wave*100)*step;
        const double pressure=WeatherSettings.SurfacePressureBar+PressureDepth(b.Position,seconds)*Dive::BarPerMetre;
        const double radius=Dive::Bubble::RadiusAt(b.RadiusMetres,b.StartPressure,pressure);
        const bool surfaced=b.Position.Z+radius*100>=SurfaceZ(b.Position.X,b.Position.Y);
        if(surfaced||b.Age>90||FVector::DistSquared(b.Position,player)>FMath::Square(3000.))
        { BubblesSurfaced+=surfaced;BubbleState.RemoveAtSwap(i);continue; }
    }
    // Remove expired entries before building the draw list, so swap removal
    // cannot invalidate the association between a rendered instance and state.
    for(int i=0;i<BubbleState.Num();++i)
    {
        const auto& b=BubbleState[i];if(b.Age<0) continue;
        const double pressure=WeatherSettings.SurfacePressureBar+PressureDepth(b.Position,seconds)*Dive::BarPerMetre;
        const double radius=Dive::Bubble::RadiusAt(b.RadiusMetres,b.StartPressure,pressure);
        const double flatten=.88+.06*FMath::Sin(b.Age*8+b.Phase);
        const double horizontal=1/std::sqrt(flatten);
        // Engine sphere radius is 50 cm. Preserve ellipsoid volume when wobbling.
        Bubbles->AddInstance(FTransform(FRotator(0,b.Phase*180/PI,0),b.Position,FVector(horizontal,horizontal,flatten)*(2*radius)));
        RenderedBubbleIndices.Add(i);
    }
}

bool ADiveWorld::ValidateParticles(FString& detail) const
{
    const double seconds=ParticleSimulationSeconds;
    if(!ParticleMaterial||!Particles->GetStaticMesh()||Particles->GetStaticMesh()->GetBounds().BoxExtent.GetMin()<2200||
        Particles->GetInstanceCount()!=GrainCount||Particles->NumCustomDataFloats!=3||Particles->PerInstanceSMCustomData.Num()!=GrainCount*3||
        !Bubbles->GetMaterial(0)||Bubbles->GetMaterial(0)->GetName()!=TEXT("M_AirBubble"))
    { detail=TEXT("Missing particle resources");return false; }
    float shaderTime=0;
    if(!ParticleMaterial->GetScalarParameterValue(FMaterialParameterInfo(TEXT("FieldTime")),shaderTime)||FMath::Abs(shaderTime-WorldTime)>.001)
    { detail=TEXT("GPU field time is not bound to the particle material");return false; }
    // The instance buffer is immutable after initialization; shader parameters
    // animate it. Check every local transform, including after cell transitions.
    FRandomStream field(62419);
    for(int i=0;i<GrainCount;++i)
    {
        const FVector expected=NextGrainSeed(field);
        FTransform transform;
        if(!Particles->GetInstanceTransform(i,transform,false)||!transform.GetLocation().Equals(expected,.001)||!transform.GetScale3D().Equals(FVector::OneVector,.0001))
        { detail=TEXT("Suspended-particle instance buffer changed");return false; }
        for(int axis=0;axis<3;++axis)
            if(FMath::Abs(Particles->PerInstanceSMCustomData[i*3+axis]-expected[axis])>.001)
            { detail=TEXT("Suspended-particle seed buffer changed");return false; }
    }
    if(RenderedBubbleIndices.Num()!=Bubbles->GetInstanceCount()) { detail=TEXT("Bubble draw count mismatch");return false; }
    if(ParticleScale>1)
    {
        FTransform first;Particles->GetInstanceTransform(0,first,true);
        UE_LOG(LogTemp,Display,TEXT("DIVE_PARTICLE_DEBUG visible=%d registered=%d hidden=%d location=%s bounds_origin=%s bounds_extent=%s first=%s custom_count=%d custom0=%.3f,%.3f,%.3f wpo_max=%.1f"),
            Particles->IsVisible(),Particles->IsRegistered(),Particles->bHiddenInGame,*Particles->GetComponentLocation().ToString(),*Particles->Bounds.Origin.ToString(),*Particles->Bounds.BoxExtent.ToString(),*first.GetLocation().ToString(),
            Particles->PerInstanceSMCustomData.Num(),Particles->PerInstanceSMCustomData[0],Particles->PerInstanceSMCustomData[1],Particles->PerInstanceSMCustomData[2],ParticleMaterial->GetMaterial()->MaxWorldPositionOffsetDisplacement);
    }
    double maxError=0;
    for(int slot=0;slot<RenderedBubbleIndices.Num();++slot)
    {
        const auto& b=BubbleState[RenderedBubbleIndices[slot]];
        FTransform transform;Bubbles->GetInstanceTransform(slot,transform,false);
        const FVector scale=transform.GetScale3D();
        const double pressure=WeatherSettings.SurfacePressureBar+PressureDepth(b.Position,seconds)*Dive::BarPerMetre;
        const double original=b.StartPressure*FMath::Pow(b.RadiusMetres,3);
        const double rendered=pressure*.125*scale.X*scale.Y*scale.Z;
        maxError=FMath::Max(maxError,FMath::Abs(rendered/original-1));
        if(b.Age<0||FMath::Abs(rendered/original-1)>1.e-4)
        { detail=TEXT("Bubble pressure/volume or delayed birth mismatch");return false; }
    }
    detail=FString::Printf(TEXT("gpu_grains=%d scale=%.1f live_bubbles=%d born=%d surfaced=%d immutable_instances=1 cell_moves=%d pv_relative_error=%.9g"),GrainCount,ParticleScale,Bubbles->GetInstanceCount(),BubblesBorn,BubblesSurfaced,ParticleCellMoves,maxError);
    return true;
}
