#include "DiveWaterScene.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"
#include "RenderingThread.h"
#include "ScenePrivate.h"
#include "SceneExtensions.h"
#include "SceneUniformBuffer.h"
#include "TextureResource.h"
#include "GlobalRenderResources.h"
#include "RHIStaticStates.h"
#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphUtils.h"
#include "RHIGPUReadback.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

BEGIN_SHADER_PARAMETER_STRUCT(FDiveWaterParameters, )
    SHADER_PARAMETER(FVector4f, Wave0)
    SHADER_PARAMETER(FVector4f, Wave1)
    SHADER_PARAMETER(FVector4f, Wave2)
    SHADER_PARAMETER(FVector4f, Wave3)
    SHADER_PARAMETER(FVector4f, Phases)
    SHADER_PARAMETER(FVector4f, Medium)
    SHADER_PARAMETER(float, Enabled)
    SHADER_PARAMETER_TEXTURE(Texture2D, Footprint)
    SHADER_PARAMETER_SAMPLER(SamplerState, FootprintSampler)
    SHADER_PARAMETER(FVector4f, FootprintMap)
    SHADER_PARAMETER(float, FootprintEnabled)
END_SHADER_PARAMETER_STRUCT()

class FDiveLakeFootprintCS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDiveLakeFootprintCS);
    SHADER_USE_PARAMETER_STRUCT(FDiveLakeFootprintCS,FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_UNIFORM_BUFFER(FSceneUniformParameters,Scene)
        SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float4>,ProbePositions)
        SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float2>,ProbeResults)
        SHADER_PARAMETER(uint32,ProbeCount)
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    {return DiveWaveTransportEnabled()&&IsFeatureLevelSupported(Parameters.Platform,ERHIFeatureLevel::SM5);}
};
IMPLEMENT_GLOBAL_SHADER(FDiveLakeFootprintCS,"/DiveRendering/LakeFootprintProbe.usf","MainCS",SF_Compute);

namespace
{
// This registry has process lifetime and no unregister API. Registration is
// opt-in before shader layout creation; the module disallows dynamic reload.
TSceneUniformBufferMemberRegistration<FDiveWaterParameters>* WaterRegistration=nullptr;
struct FWaterInput
{
    FDiveWaterSceneInput Values;
    FTextureRHIRef Footprint;
};
TMap<const FSceneInterface*,FWaterInput> RenderInputs;
void ResetWaterParameters(FDiveWaterParameters& Parameters)
{
    Parameters.Wave0=Parameters.Wave1=Parameters.Wave2=Parameters.Wave3=
        Parameters.Phases=Parameters.Medium=FVector4f(0,0,0,0);
    Parameters.Enabled=0;
    Parameters.Footprint=GWhiteTexture->TextureRHI;
    Parameters.FootprintSampler=TStaticSamplerState<SF_Bilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
    Parameters.FootprintMap=FVector4f(0,0,0,0);
    Parameters.FootprintEnabled=0;
}

class FDiveWaterScene final : public ISceneExtension
{
    bool Reported=false;
    TUniquePtr<FRHIGPUBufferReadback> ProbeReadback;
    bool ProbeDone=false;
    TSharedPtr<const TArray<FVector4f>,ESPMode::ThreadSafe> Probes;
    void Probe(FRDGBuilder& GraphBuilder,FSceneUniformBuffer& Uniforms,const FWaterInput* Input)
    {
        if(ProbeDone||!Input||!Input->Footprint.IsValid()||!Input->Values.FootprintProbes) return;
        if(ProbeReadback)
        {
            if(!ProbeReadback->IsReady()) return;
            const auto* output=static_cast<const FVector2f*>(ProbeReadback->Lock(Probes->Num()*sizeof(FVector2f)));
            float error=0,weightError=0;
            FString csv=TEXT("x_m,y_m,expected_distance_m,gpu_distance_m,expected_wet,gpu_wet\n");
            for(int32 i=0;i<Probes->Num();++i)
            {
                const auto& p=(*Probes)[i];const auto& value=output[i];
                if(!FMath::IsFinite(value.X)||!FMath::IsFinite(value.Y))
                    UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_GPU_FAIL nonfinite sample"));
                error=FMath::Max(error,FMath::Abs(value.X-p.Z));
                weightError=FMath::Max(weightError,FMath::Abs(value.Y-p.W));
                csv+=FString::Printf(TEXT("%.8f,%.8f,%.8f,%.8f,%.8f,%.8f\n"),p.X,p.Y,p.Z,value.X,p.W,value.Y);
            }
            ProbeReadback->Unlock();
            const bool saved=FFileHelper::SaveStringToFile(csv,*(FPaths::ProjectSavedDir()/TEXT("LakeFootprintGPU.csv")));
            if(!saved||error>.025f||weightError>.025f)
                UE_LOG(LogTemp,Fatal,TEXT("DIVE_LAKE_GPU_FAIL count=%d distance_error=%.8f wet_error=%.8f saved=%d"),Probes->Num(),error,weightError,saved);
            UE_LOG(LogTemp,Display,TEXT("DIVE_LAKE_GPU_PASS count=%d max_distance_error_m=%.8f max_wet_error=%.8f"),Probes->Num(),error,weightError);
            ProbeDone=true;return;
        }
        Probes=Input->Values.FootprintProbes;
        auto* points=CreateStructuredBuffer(GraphBuilder,TEXT("DiveLakeProbePositions"),sizeof(FVector4f),Probes->Num(),Probes->GetData(),Probes->Num()*sizeof(FVector4f));
        auto* output=GraphBuilder.CreateBuffer(FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector2f),Probes->Num()),TEXT("DiveLakeProbeResults"));
        auto* parameters=GraphBuilder.AllocParameters<FDiveLakeFootprintCS::FParameters>();
        parameters->Scene=Uniforms.GetBuffer(GraphBuilder);
        parameters->ProbePositions=GraphBuilder.CreateSRV(points);
        parameters->ProbeResults=GraphBuilder.CreateUAV(output);
        parameters->ProbeCount=Probes->Num();
        TShaderMapRef<FDiveLakeFootprintCS> shader(GetGlobalShaderMap(Scene.GetFeatureLevel()));
        FComputeShaderUtils::AddPass(GraphBuilder,RDG_EVENT_NAME("DiveLakeFootprintAudit"),shader,parameters,FIntVector(FMath::DivideAndRoundUp(Probes->Num(),64),1,1));
        ProbeReadback=MakeUnique<FRHIGPUBufferReadback>(TEXT("DiveLakeFootprintAudit"));
        AddEnqueueCopyPass(GraphBuilder,ProbeReadback.Get(),output,Probes->Num()*sizeof(FVector2f));
    }
public:
    explicit FDiveWaterScene(FScene& InScene):ISceneExtension(InScene) {}
    ~FDiveWaterScene() override {RenderInputs.Remove(&Scene);}
    class FRenderer final : public ISceneExtensionRenderer
    {
        const FSceneInterface* Scene;
        FDiveWaterScene& Owner;
    public:
        FRenderer(FSceneRendererBase& Renderer,const FSceneInterface* InScene,FDiveWaterScene& InOwner):ISceneExtensionRenderer(Renderer),Scene(InScene),Owner(InOwner) {}
        void UpdateSceneUniformBuffer(FRDGBuilder& GraphBuilder,FSceneUniformBuffer& Uniforms) override
        {
            FDiveWaterParameters Parameters{};
            ResetWaterParameters(Parameters);
            if(const auto* Stored=RenderInputs.Find(Scene))
            {
                const auto* Input=&Stored->Values;
                Parameters.Wave0=Input->Waves[0];Parameters.Wave1=Input->Waves[1];
                Parameters.Wave2=Input->Waves[2];Parameters.Wave3=Input->Waves[3];
                Parameters.Phases=Input->Phases;Parameters.Medium=Input->Medium;Parameters.Enabled=1;
                if(Stored->Footprint.IsValid())
                {
                    Parameters.Footprint=Stored->Footprint;
                    Parameters.FootprintMap=Input->FootprintMap;
                    Parameters.FootprintEnabled=1;
                }
                if(!Owner.Reported)
                {
                    UE_LOG(LogTemp,Display,TEXT("DIVE_WATER_SCENE_READY waves=4 clock=%.3f extinction=%.6f strength=%.3f"),
                        Input->Medium.X,Input->Medium.Z,Input->Medium.W);
                    Owner.Reported=true;
                }
            }
            Uniforms.Set(*WaterRegistration,Parameters);
            Owner.Probe(GraphBuilder,Uniforms,RenderInputs.Find(Scene));
        }
    };
    ISceneExtensionRenderer* CreateRenderer(FSceneRendererBase& Renderer,const FEngineShowFlags&) override
    {return new FRenderer(Renderer,&Scene,*this);}
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
    ENQUEUE_RENDER_COMMAND(DivePublishWater)([Scene,Input](FRHICommandListImmediate&)
    {
        auto& Stored=RenderInputs.FindOrAdd(Scene);
        Stored.Values=Input;
        Stored.Footprint=Input.FootprintResource?Input.FootprintResource->TextureRHI:FTextureRHIRef();
        Stored.Values.FootprintResource=nullptr; // The RHI reference retains ownership after this command.
    });
}
