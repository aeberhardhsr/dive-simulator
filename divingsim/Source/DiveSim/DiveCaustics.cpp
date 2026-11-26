#include "DiveWorld.h"
#include "DivePawn.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/Material.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"

void ADiveWorld::UpdateCaustics()
{
    if(!SolarWater||!Sun) return;
    for(int i=0;i<4;++i)
    {
        const auto& w=Waves.Waves[i];
        SolarWater->SetVectorParameterValue(FName(*FString::Printf(TEXT("Wave%d"),i)),FLinearColor(w.Kx,w.Ky,w.Omega,w.Amplitude));
    }
    SolarWater->SetVectorParameterValue(TEXT("WavePhase"),FLinearColor(Waves.Waves[0].Phase,Waves.Waves[1].Phase,Waves.Waves[2].Phase,Waves.Waves[3].Phase));
    const FVector ray=Sun->GetForwardVector();
    SolarWater->SetVectorParameterValue(TEXT("SunRay"),FLinearColor(ray.X,ray.Y,ray.Z,0));
    const float clock=Diver?Diver->Simulation.Data.ElapsedSeconds:0;
    SolarWater->SetScalarParameterValue(TEXT("Clock"),clock);
    if(WaterMaterial) WaterMaterial->SetScalarParameterValue(TEXT("WaveTime"),clock);
    SolarWater->SetScalarParameterValue(TEXT("WindStrength"),FMath::Clamp(WeatherSettings.WindMetresPerSecond/12,0.,1.));
    SolarWater->SetScalarParameterValue(TEXT("Extinction"),1.45/VisibleDistance);
    SolarWater->SetScalarParameterValue(TEXT("CausticStrength"),CausticsStrength);
}

void ADiveWorld::SetCausticsTestStrength(float Value)
{
    if(!bOpticsSmoke) return;
    CausticsStrength=FMath::Clamp(Value,0.f,1.f);UpdateCaustics();
}

bool ADiveWorld::ValidateCaustics(FString& Error) const
{
    if(!SolarWater||!Sun||Sun->LightFunctionMaterial!=SolarWater||SolarWater->GetMaterial()->MaterialDomain!=MD_LightFunction)
    { Error=TEXT("Missing solar underwater light function");return false; }
    const FVector ray=Sun->GetForwardVector();
    const auto shaderRay=SolarWater->K2_GetVectorParameterValue(TEXT("SunRay"));
    if(!shaderRay.Equals(FLinearColor(ray.X,ray.Y,ray.Z,0),1.e-5)||
        !WaterMaterial||SolarWater->K2_GetScalarParameterValue(TEXT("Clock"))!=WaterMaterial->K2_GetScalarParameterValue(TEXT("WaveTime"))||
        FMath::Abs(SolarWater->K2_GetScalarParameterValue(TEXT("Extinction"))-1.45/VisibleDistance)>1.e-6||
        FMath::Abs(SolarWater->K2_GetScalarParameterValue(TEXT("WindStrength"))-FMath::Clamp(WeatherSettings.WindMetresPerSecond/12,0.,1.))>1.e-6)
    { Error=FString::Printf(TEXT("Solar caustics mismatch ray_error=(%.12g,%.12g,%.12g,%.12g) clock_error=%.12g extinction_error=%.12g wind_error=%.12g"),
        shaderRay.R-ray.X,shaderRay.G-ray.Y,shaderRay.B-ray.Z,shaderRay.A,
        SolarWater->K2_GetScalarParameterValue(TEXT("Clock"))-(WaterMaterial?WaterMaterial->K2_GetScalarParameterValue(TEXT("WaveTime")):-1),
        SolarWater->K2_GetScalarParameterValue(TEXT("Extinction"))-1.45/VisibleDistance,
        SolarWater->K2_GetScalarParameterValue(TEXT("WindStrength"))-FMath::Clamp(WeatherSettings.WindMetresPerSecond/12,0.,1.));return false; }
    for(int i=0;i<4;++i)
    {
        const auto& w=Waves.Waves[i];
        if(!SolarWater->K2_GetVectorParameterValue(FName(*FString::Printf(TEXT("Wave%d"),i))).Equals(FLinearColor(w.Kx,w.Ky,w.Omega,w.Amplitude),1.e-5))
        { Error=TEXT("Solar caustics and surface wave coefficients differ");return false; }
    }
    if(!SolarWater->K2_GetVectorParameterValue(TEXT("WavePhase")).Equals(FLinearColor(Waves.Waves[0].Phase,Waves.Waves[1].Phase,Waves.Waves[2].Phase,Waves.Waves[3].Phase),1.e-5)||
        FMath::Abs(SolarWater->K2_GetScalarParameterValue(TEXT("CausticStrength"))-CausticsStrength)>1.e-6)
    { Error=TEXT("Solar caustics phase or strength mismatch");return false; }
    return true;
}

void ADiveWorld::ConfigureCausticsProbe(const Dive::Config& Settings,float Height)
{
    if(!bOpticsSmoke) return;
    Configure(Settings);
    const FVector center=Terrain.Entries[0].Position;
    if(CausticsProbe.IsEmpty())
    {
        CausticsProbe.Add(Prop(TEXT("CausticsReceiver"),TEXT("/Engine/BasicShapes/Cube.Cube"),center,FVector(10,10,.1),TEXT("M_CausticsProbe"),false));
        CausticsProbe.Add(Prop(TEXT("CausticsSphere"),TEXT("/Engine/BasicShapes/Sphere.Sphere"),center,FVector(1.4),TEXT("M_CausticsProbe"),false));
        CausticsProbe.Add(Prop(TEXT("CausticsOccluder"),TEXT("/Engine/BasicShapes/Cube.Cube"),center,FVector(.6,4,.2),TEXT("M_CausticsProbe"),false));
    }
    CausticsProbe[0]->SetWorldLocation(FVector(center.X,center.Y,Height-5));
    CausticsProbe[1]->SetWorldLocation(FVector(center.X+140,center.Y-160,Height+70));
    CausticsProbe[2]->SetWorldLocation(FVector(center.X-70,center.Y+70,Height+140));
    // A deep receiver must not be buried below the surveyed lakebed.
    for(auto& pair:Chunks) pair.Value->SetVisibility(Height>-1000);
}
