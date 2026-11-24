using UnrealBuildTool;
using System.Collections.Generic;

public class divingsimTarget : TargetRules
{
    public divingsimTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("DiveSim");
    }
}
