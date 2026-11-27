#include "Modules/ModuleManager.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/CoreDelegates.h"
#include "HAL/FileManager.h"
#include "DiveWaterScene.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "SceneViewExtension.h"
#include "SceneRendering.h"
#include "PostProcess/PostProcessMaterialInputs.h"
#include "GlobalShader.h"
#include "PixelShaderUtils.h"
#include "ShaderParameterStruct.h"
#include "RenderGraphBuilder.h"
#include "ScreenPass.h"

class FDiveSpectralWaterPS : public FGlobalShader
{
public:
    DECLARE_GLOBAL_SHADER(FDiveSpectralWaterPS);
    SHADER_USE_PARAMETER_STRUCT(FDiveSpectralWaterPS, FGlobalShader);
    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputColor)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, InputDepth)
        SHADER_PARAMETER_RDG_TEXTURE(Texture2D, WaterDepth)
        SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture2D<uint2>, WaterStencil)
        SHADER_PARAMETER_RDG_TEXTURE(Texture3D, IntegratedFog)
        SHADER_PARAMETER_SAMPLER(SamplerState, FogSampler)
        SHADER_PARAMETER(FVector4f, InputRect)
        SHADER_PARAMETER(FVector4f, OutputRect)
        SHADER_PARAMETER(float, EyeHeightCm)
        SHADER_PARAMETER(float, AbsorptionGain)
        SHADER_PARAMETER(float, DepthDebug)
        RENDER_TARGET_BINDING_SLOTS()
    END_SHADER_PARAMETER_STRUCT()
    static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
    { return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5); }
};
IMPLEMENT_GLOBAL_SHADER(FDiveSpectralWaterPS, "/DiveRendering/SpectralWater.usf", "MainPS", SF_Pixel);

class FDiveSpectralWaterView final : public FSceneViewExtensionBase
{
public:
    FDiveSpectralWaterView(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister)
    {
        Enabled=FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&
            FParse::Param(FCommandLine::Get(),TEXT("DiveWaterPreview"))&&
            FParse::Param(FCommandLine::Get(),TEXT("DiveSpectralView"));
        FParse::Value(FCommandLine::Get(),TEXT("DiveAbsorptionGain="),Gain);
        Gain=FMath::Clamp(Gain,0.f,2.f);
    }
    void SetupViewFamily(FSceneViewFamily&) override {}
    void SetupView(FSceneViewFamily&,FSceneView&) override {}
    void BeginRenderViewFamily(FSceneViewFamily&) override {}
    void SubscribeToPostProcessingPass(EPostProcessingPass Pass,const FSceneView& View,
        FPostProcessingPassDelegateArray& Callbacks,bool) override
    {
        if(Enabled&&Pass==EPostProcessingPass::BeforeDOF&&View.GetFeatureLevel()>=ERHIFeatureLevel::SM5)
            Callbacks.Add(FPostProcessingPassDelegate::CreateRaw(this,&FDiveSpectralWaterView::Render));
    }
private:
    bool Enabled=false,Reported=false;
    float Gain=1;
    FScreenPassTexture Render(FRDGBuilder& GraphBuilder,const FSceneView& SceneView,const FPostProcessMaterialInputs& Inputs)
    {
        const auto& View=static_cast<const FViewInfo&>(SceneView);
        const auto Fog=View.VolumetricFogResources.IntegratedLightScatteringTexture;
        if(!Fog||!Inputs.SceneTextures.SceneTextures||View.ViewMatrices.GetViewOrigin().Z>=-5)
            return Inputs.ReturnUntouchedSceneColorForPostProcessing(GraphBuilder);
        const auto Input=FScreenPassTexture::CopyFromSlice(GraphBuilder,Inputs.GetInput(EPostProcessMaterialInput::SceneColor));
        auto Output=Inputs.OverrideOutput;
        if(!Output.IsValid()) Output=FScreenPassRenderTarget::CreateFromInput(GraphBuilder,Input,ERenderTargetLoadAction::ENoAction,TEXT("DiveSpectralWater"));
        auto* Parameters=GraphBuilder.AllocParameters<FDiveSpectralWaterPS::FParameters>();
        Parameters->View=View.ViewUniformBuffer;
        Parameters->InputColor=Input.Texture;
        Parameters->InputDepth=Inputs.SceneTextures.SceneTextures->GetParameters()->SceneDepthTexture;
        Parameters->WaterDepth=Inputs.SceneTextures.SceneTextures->GetParameters()->CustomDepthTexture;
        Parameters->WaterStencil=Inputs.SceneTextures.SceneTextures->GetParameters()->CustomStencilTexture;
        Parameters->IntegratedFog=Fog;
        Parameters->FogSampler=TStaticSamplerState<SF_Trilinear,AM_Clamp,AM_Clamp,AM_Clamp>::GetRHI();
        Parameters->InputRect=FVector4f(Input.ViewRect.Min.X,Input.ViewRect.Min.Y,Input.ViewRect.Width(),Input.ViewRect.Height());
        Parameters->OutputRect=FVector4f(Output.ViewRect.Min.X,Output.ViewRect.Min.Y,Output.ViewRect.Width(),Output.ViewRect.Height());
        Parameters->EyeHeightCm=View.ViewMatrices.GetViewOrigin().Z;
        Parameters->AbsorptionGain=Gain;
        Parameters->DepthDebug=FParse::Param(FCommandLine::Get(),TEXT("DiveSpectralDepthDebug"))?1.f:0.f;
        Parameters->RenderTargets[0]=Output.GetRenderTargetBinding();
        TShaderMapRef<FDiveSpectralWaterPS> Shader(View.ShaderMap);
        FPixelShaderUtils::AddFullscreenPass(GraphBuilder,View.ShaderMap,RDG_EVENT_NAME("DiveSpectralWater"),Shader,Parameters,Output.ViewRect);
        if(!Reported)
        {
            UE_LOG(LogTemp,Display,TEXT("DIVE_SPECTRAL_VIEW_READY gain=%.3f samples=16 fog_resource=1 stage=BeforeDOF"),Gain);
            Reported=true;
        }
        return Output;
    }
};

class FDiveRenderingModule final : public IModuleInterface
{
    TSharedPtr<FDiveSpectralWaterView,ESPMode::ThreadSafe> Extension;
    FDelegateHandle EngineReady;
    void InitializeView()
    {
        if(DiveReflectionTransportEnabled())
            if(auto* CustomDepth=IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"))) CustomDepth->Set(3,ECVF_SetByCode);
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveSpectralView"))&&FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&FParse::Param(FCommandLine::Get(),TEXT("DiveWaterPreview")))
        {
            if(auto* CustomDepth=IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"))) CustomDepth->Set(3,ECVF_SetByCode);
            Extension=FSceneViewExtensions::NewExtension<FDiveSpectralWaterView>();
        }
    }
public:
    void StartupModule() override
    {
        InitializeDiveWaterScene();
        AddShaderSourceDirectoryMapping(TEXT("/DiveRendering"),FPaths::Combine(FPaths::ProjectDir(),TEXT("Shaders")));
        if(DiveReflectionTransportEnabled())
        {
            const bool unified=FParse::Param(FCommandLine::Get(),TEXT("DiveReflectionUnified"));
            if(unified)
            {
                const auto* version=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DiveWater.ReflectionBindingVersion"));
                if(!version||version->GetInt()!=1)
                    UE_LOG(LogTemp,Fatal,TEXT("Unified water reflection requires the separate source-engine custom-depth binding patch."));
            }
            if(FParse::Param(FCommandLine::Get(),TEXT("DiveSolarShaderStudy"))||FParse::Param(FCommandLine::Get(),TEXT("DiveReflectionRayStudy")))
                UE_LOG(LogTemp,Fatal,TEXT("Reflection transport must run separately from other shader experiments."));
            const FString prepared=FPaths::Combine(FPaths::ProjectIntermediateDir(),TEXT("DiveReflectionTransportStudy/Lumen"));
            if(!IFileManager::Get().FileExists(*(prepared/TEXT("LumenReflectionHardwareRayTracing.usf"))))
                UE_LOG(LogTemp,Fatal,TEXT("Reflection transport requires Tools/PrepareReflectionTransport.py."));
            FString routing;
            if(!FFileHelper::LoadFileToString(routing,*(FPaths::GetPath(prepared)/TEXT("RoutingMode.txt")))||routing!=(unified?TEXT("unified"):TEXT("hybrid")))
                UE_LOG(LogTemp,Fatal,TEXT("Prepared reflection shader routing mode does not match the requested mode."));
            const bool control=FParse::Param(FCommandLine::Get(),TEXT("DiveReflectionTransportControl"));
            FString requestedProfile=TEXT("full"),preparedProfile;
            FParse::Value(FCommandLine::Get(),TEXT("DiveReflectionProfile="),requestedProfile);
            if((requestedProfile!=TEXT("full")&&requestedProfile!=TEXT("routing")&&requestedProfile!=TEXT("unshadowed")&&requestedProfile!=TEXT("cached"))||
                !FFileHelper::LoadFileToString(preparedProfile,*(FPaths::GetPath(prepared)/TEXT("ProfileMode.txt")))||preparedProfile!=requestedProfile||
                (control&&requestedProfile!=TEXT("full")&&requestedProfile!=TEXT("cached")))
                UE_LOG(LogTemp,Fatal,TEXT("Prepared reflection diagnostic profile does not match the request."));
            if(!control) AddShaderSourceDirectoryMapping(TEXT("/Engine/Private/Lumen"),prepared);
            UE_LOG(LogTemp,Display,TEXT("DIVE_REFLECTION_TRANSPORT_READY enabled=%d samples=16 light_source=scene scope=raygen_water_frontlayer"),control?0:1);
            UE_LOG(LogTemp,Display,TEXT("DIVE_REFLECTION_ROUTING_READY unified=%d"),unified?1:0);
            UE_LOG(LogTemp,Display,TEXT("DIVE_REFLECTION_PROFILE_READY profile=%s diagnostic=%d"),*preparedProfile,preparedProfile!=TEXT("full")?1:0);
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveReflectionRayStudy"))&&
           FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&FParse::Param(FCommandLine::Get(),TEXT("DiveWaterPreview")))
        {
            if(FParse::Param(FCommandLine::Get(),TEXT("DiveSolarShaderStudy")))
                UE_LOG(LogTemp,Fatal,TEXT("Reflection classification and solar shader studies must run separately."));
            const FString prepared=FPaths::Combine(FPaths::ProjectIntermediateDir(),TEXT("DiveReflectionRayStudy/Lumen"));
            if(!IFileManager::Get().FileExists(*(prepared/TEXT("LumenReflectionHardwareRayTracing.usf"))))
                UE_LOG(LogTemp,Fatal,TEXT("Reflection ray study requires Tools/PrepareReflectionRayStudy.py."));
            AddShaderSourceDirectoryMapping(TEXT("/Engine/Private/Lumen"),prepared);
            UE_LOG(LogTemp,Display,TEXT("DIVE_REFLECTION_RAY_STUDY_READY mapping=project-local legend=Intermediate/DiveReflectionRayStudy/Manifest.json"));
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("DiveSolarShaderStudy"))&&
           FParse::Param(FCommandLine::Get(),TEXT("DiveSmokeTest"))&&FParse::Param(FCommandLine::Get(),TEXT("DiveWaterPreview")))
        {
            const bool volume=FParse::Param(FCommandLine::Get(),TEXT("DiveSolarVolumeStudy"));
            const FString prepared=FPaths::Combine(FPaths::ProjectIntermediateDir(),DiveWaveTransportEnabled()?TEXT("DiveWaveStudy/Private"):
                volume?TEXT("DiveSolarVolumeStudy/Private"):TEXT("DiveSolarStudy/Private"));
            if(!IFileManager::Get().FileExists(*(prepared/TEXT("DeferredLightPixelShaders.usf"))))
                UE_LOG(LogTemp,Fatal,TEXT("Solar shader study requires Tools/PrepareSolarShaderStudy.py for this project."));
            AddShaderSourceDirectoryMapping(TEXT("/Engine/Private"),prepared);
            UE_LOG(LogTemp,Display,TEXT("DIVE_SOLAR_SHADER_STUDY_READY mapping=project-local directional_only=1 mean_plane=%d"),DiveWaveTransportEnabled()?0:1);
            if(volume) UE_LOG(LogTemp,Display,TEXT("DIVE_SOLAR_VOLUME_STUDY_READY receiver=per-froxel local_lights=unchanged"));
        }
        if(GEngine) InitializeView();
        else EngineReady=FCoreDelegates::GetOnPostEngineInit().AddRaw(this,&FDiveRenderingModule::InitializeView);
    }
    bool SupportsDynamicReloading() override {return false;}
    void ShutdownModule() override
    {
        FCoreDelegates::GetOnPostEngineInit().Remove(EngineReady);
        Extension.Reset();
    }
};
IMPLEMENT_MODULE(FDiveRenderingModule,DiveRendering);
