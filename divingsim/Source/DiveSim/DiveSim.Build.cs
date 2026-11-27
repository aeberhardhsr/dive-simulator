using UnrealBuildTool;

public class DiveSim : ModuleRules
{
    public DiveSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        // Preserve normal C++ translation-unit scope for private HUD helpers.
        // Adaptive unity must not change build correctness after a git commit.
        bUseUnity = false;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent"
        });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities", "SlateCore", "Slate", "AudioMixer", "CelestialVault", "DaySequence", "RenderCore", "RHI", "DiveRendering" });
        if (Target.bBuildEditor)
            PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "MeshDescription", "StaticMeshDescription", "AssetRegistry" });
    }
}
