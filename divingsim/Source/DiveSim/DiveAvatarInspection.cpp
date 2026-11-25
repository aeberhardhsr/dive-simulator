#include "DiveAvatar.h"
#include "DivePawn.h"
#include "DiveEquipment.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"

void ADiveAvatar::SetInspection(bool Enabled)
{
    if(bInspection==Enabled) return;
    bInspection=Enabled;
    TArray<UPrimitiveComponent*> primitives;GetComponents(primitives);
    if(Diver&&Diver->Equipment)
    { TArray<UPrimitiveComponent*> hands;Diver->Equipment->GetComponents(hands);primitives.Append(hands); }
    for(auto* component:primitives) component->SetLightingChannels(!Enabled,Enabled,false);
    for(auto& light:InspectionLights) light->SetVisibility(Enabled);
    LastInspectionCapture=-1;
}

UTextureRenderTarget2D* ADiveAvatar::InspectionImage()
{
    if(!bInspection||!Diver) return nullptr;
    if(!InspectionCapture)
    {
        InspectionTexture=NewObject<UTextureRenderTarget2D>(this);
        InspectionTexture->RenderTargetFormat=RTF_RGBA8;
        InspectionTexture->ClearColor=FLinearColor(.012f,.032f,.032f,1);
        InspectionTexture->InitAutoFormat(768,512);InspectionTexture->UpdateResourceImmediate(true);
        InspectionCapture=NewObject<USceneCaptureComponent2D>(this);
        InspectionCapture->SetupAttachment(RootComponent);InspectionCapture->RegisterComponent();
        InspectionCapture->TextureTarget=InspectionTexture;
        InspectionCapture->bCaptureEveryFrame=false;InspectionCapture->bCaptureOnMovement=false;
        InspectionCapture->bAlwaysPersistRenderingState=true;
        InspectionCapture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
        InspectionCapture->ProjectionType=ECameraProjectionMode::Perspective;
        InspectionCapture->FOVAngle=35;
        InspectionCapture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        InspectionCapture->ShowOnlyActorComponents(this);
        InspectionCapture->ShowOnlyActorComponents(Diver->Equipment);
        auto& flags=InspectionCapture->ShowFlags;
        flags.SetFog(false);flags.SetAtmosphere(false);flags.SetVolumetricFog(false);flags.SetCloud(false);
        flags.SetSkyLighting(false);flags.SetGlobalIllumination(false);flags.SetMotionBlur(false);
        auto& pp=InspectionCapture->PostProcessSettings;
        pp.bOverride_AutoExposureMethod=true;pp.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
        pp.bOverride_AutoExposureApplyPhysicalCameraExposure=true;pp.AutoExposureApplyPhysicalCameraExposure=true;
        pp.bOverride_CameraISO=true;pp.CameraISO=100;
        pp.bOverride_CameraShutterSpeed=true;pp.CameraShutterSpeed=60;
        pp.bOverride_DepthOfFieldFstop=true;pp.DepthOfFieldFstop=5.6;
        pp.bOverride_AutoExposureBias=true;pp.AutoExposureBias=-1;
        pp.bOverride_FilmGrainIntensity=true;pp.FilmGrainIntensity=0;
        for(int i=0;i<2;++i)
        {
            auto* light=NewObject<UPointLightComponent>(this);
            light->SetupAttachment(RootComponent);light->RegisterComponent();
            light->SetRelativeLocation(i?FVector(-160,130,180):FVector(180,-120,220));
            light->SetIntensityUnits(ELightUnits::Lumens);light->SetIntensity(i?200000:500000);
            light->SetSourceRadius(65);light->SetSoftSourceRadius(90);
            light->SetAttenuationRadius(700);light->SetCastShadows(false);
            light->SetLightingChannels(false,true,false);InspectionLights.Add(light);
        }
        // Components created above must receive their first inspection channel update.
        bInspection=false;SetInspection(true);
    }
    const double now=FPlatformTime::Seconds();
    if(now-LastInspectionCapture>.1)
    {
        LastInspectionCapture=now;
        const FVector target=GetActorTransform().TransformPosition(FVector(0,0,bInspectionClose?137:90));
        const double angle=FMath::DegreesToRadians(InspectionYaw+55*SwimBlend);
        const double distance=bInspectionClose?290:580;
        const FQuat horizontal=FRotator(0,GetActorRotation().Yaw,0).Quaternion();
        const FVector eye=target+horizontal.RotateVector(FVector(distance*FMath::Cos(angle),distance*FMath::Sin(angle),bInspectionClose?25:50));
        InspectionCapture->SetWorldLocation(eye);
        InspectionCapture->SetWorldRotation(FRotationMatrix::MakeFromXZ(target-eye,FVector::UpVector).Rotator());
        InspectionCapture->CaptureScene();
    }
    return InspectionTexture;
}
