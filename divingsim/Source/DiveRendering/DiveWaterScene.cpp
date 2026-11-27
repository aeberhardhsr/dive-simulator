#include "DiveWaterScene.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "ScenePrivate.h"
#include "SceneExtensions.h"
#include "SceneUniformBuffer.h"

BEGIN_SHADER_PARAMETER_STRUCT(FDiveWaterParameters, )
    SHADER_PARAMETER(FVector4f, Wave0)
    SHADER_PARAMETER(FVector4f, Wave1)
    SHADER_PARAMETER(FVector4f, Wave2)
    SHADER_PARAMETER(FVector4f, Wave3)
    SHADER_PARAMETER(FVector4f, Phases)
    SHADER_PARAMETER(FVector4f, Medium)
    SHADER_PARAMETER(float, Enabled)
END_SHADER_PARAMETER_STRUCT()

namespace
{
// This registry has process lifetime and no unregister API. Registration is
// opt-in before shader layout creation; the module disallows dynamic reload.
TSceneUniformBufferMemberRegistration<FDiveWaterParameters>* WaterRegistration=nullptr;
TMap<const FSceneInterface*,FDiveWaterSceneInput> RenderInputs;
void ResetWaterParameters(FDiveWaterParameters& Parameters)
{
    Parameters.Wave0=Parameters.Wave1=Parameters.Wave2=Parameters.Wave3=
        Parameters.Phases=Parameters.Medium=FVector4f(0,0,0,0);
    Parameters.Enabled=0;
}

class FDiveWaterScene final : public ISceneExtension
{
    bool Reported=false;
public:
    explicit FDiveWaterScene(FScene& InScene):ISceneExtension(InScene) {}
    ~FDiveWaterScene() override {RenderInputs.Remove(&Scene);}
    class FRenderer final : public ISceneExtensionRenderer
    {
        const FSceneInterface* Scene;
        bool& Reported;
    public:
        FRenderer(FSceneRendererBase& Renderer,const FSceneInterface* InScene,bool& InReported):ISceneExtensionRenderer(Renderer),Scene(InScene),Reported(InReported) {}
        void UpdateSceneUniformBuffer(FRDGBuilder&,FSceneUniformBuffer& Uniforms) override
        {
            FDiveWaterParameters Parameters{};
            ResetWaterParameters(Parameters);
            if(const auto* Input=RenderInputs.Find(Scene))
            {
                Parameters.Wave0=Input->Waves[0];Parameters.Wave1=Input->Waves[1];
                Parameters.Wave2=Input->Waves[2];Parameters.Wave3=Input->Waves[3];
                Parameters.Phases=Input->Phases;Parameters.Medium=Input->Medium;Parameters.Enabled=1;
                if(!Reported)
                {
                    UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_SCENE_READY waves=4 clock=%.3f extinction=%.6f strength=%.3f"),
                        Input->Medium.X,Input->Medium.Z,Input->Medium.W);
                    Reported=true;
                }
            }
            Uniforms.Set(*WaterRegistration,Parameters);
        }
    };
    ISceneExtensionRenderer* CreateRenderer(FSceneRendererBase& Renderer,const FEngineShowFlags&) override
    {return new FRenderer(Renderer,&Scene,Reported);}
};
}

bool DiveWaveTransportEnabled()
{
    static const bool Enabled=FParse::Param(FCommandLine::Get(),TEXT("DiveWaveTransportStudy"))&&
        FParse::Param(FCommandLine::Get(),TEXT("DiveSolarShaderStudy"))&&
        FParse::Param(FCommandLine::Get(),TEXT("DiveSolarVolumeStudy"))&&
        FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&FParse::Param(FCommandLine::Get(),TEXT("DiveWaterPreview"));
    return Enabled;
}

void InitializeDiveWaterScene()
{
    if(!DiveWaveTransportEnabled()||WaterRegistration) return;
    const auto* BindingVersion=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DiveWater.SceneBindingVersion"));
    if(!BindingVersion||BindingVersion->GetInt()!=1)
        UE_LOG(LogTemp,Fatal,TEXT("DIVE_WAVE_RENDERER_UNAVAILABLE: wave transport requires the separately rebuilt Renderer scene-binding patch. The installed engine is unsupported; see Documentation/WAVE_TRANSPORT_STUDY.md."));
    WaterRegistration=new TSceneUniformBufferMemberRegistration<FDiveWaterParameters>(TEXT("DiveWater"),
        [](FDiveWaterParameters& Parameters,FRDGBuilder&){ResetWaterParameters(Parameters);});
    static auto* Factory=new TSceneExtensionRegistration<FDiveWaterScene>();
    UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_SCENE_REGISTERED member=DiveWater"));
}

void PublishDiveWaterScene(FSceneInterface* Scene,const FDiveWaterSceneInput& Input)
{
    if(!DiveWaveTransportEnabled()||!Scene) return;
    ENQUEUE_RENDER_COMMAND(DivePublishWater)([Scene,Input](FRHICommandListImmediate&){RenderInputs.Add(Scene,Input);});
}
