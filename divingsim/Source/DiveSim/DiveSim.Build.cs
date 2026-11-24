using UnrealBuildTool;

public class DiveSim : ModuleRules
{
    public DiveSim(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject", "Engine", "InputCore", "ProceduralMeshComponent"
        });
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "JsonUtilities", "SlateCore" });
    }
}
