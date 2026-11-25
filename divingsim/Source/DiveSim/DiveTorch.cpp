#include "DivePawn.h"
#include "DiveWorld.h"
#include "Components/SpotLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

void ADivePawn::UpdateTorch()
{
    const auto& model=Dive::Torch::At(PendingConfig.TorchModel);
    const double battery=Simulation.Data.LampBattery;
    Lamp->SetIntensity(model.Output(battery));
    Lamp->SetInnerConeAngle(model.InnerDegrees);
    Lamp->SetOuterConeAngle(model.OuterDegrees);
    Lamp->SetAttenuationRadius(FMath::Clamp(PendingConfig.VisibilityMetres*200,1000.,8000.));
    Lamp->SetVisibility(bLamp&&battery>0);
    if(!TorchFunction)
    {
        if(auto* material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Diving/Materials/M_TorchWater.M_TorchWater")))
        {
            TorchFunction=UMaterialInstanceDynamic::Create(material,this);
            Lamp->SetLightFunctionMaterial(TorchFunction);
        }
        else if(bSmokeTest)
        { UE_LOG(LogTemp,Error,TEXT("DIVE_LIGHT_FAIL Missing torch water material")); FPlatformMisc::RequestExitWithStatus(false,1); return; }
    }
    if(TorchFunction&&Environment)
    {
        const auto position=Lamp->GetComponentLocation();
        TorchFunction->SetVectorParameterValue(TEXT("LampPosition"),FLinearColor(position.X,position.Y,position.Z,0));
        TorchFunction->SetScalarParameterValue(TEXT("WaterSurfaceZ"),Environment->SurfaceZ(position.X,position.Y));
        TorchFunction->SetScalarParameterValue(TEXT("ExtinctionPerCm"),1.45/(PendingConfig.VisibilityMetres*100));
    }
}
