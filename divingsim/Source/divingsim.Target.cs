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
        ExtraModuleNames.Add("DiveRendering");
        // Keep a user-opened development executable intact while linking another build.
        if (Target.Arguments != null && Target.Arguments.TryGetValue("-DiveBuildSlot=", out string Slot))
        {
            if (Target.Platform != UnrealTargetPlatform.Win64 || Target.Configuration != UnrealTargetConfiguration.Development ||
                !System.Text.RegularExpressions.Regex.IsMatch(Slot, "^[A-Za-z0-9_]{1,24}$"))
                throw new BuildException("DiveBuildSlot requires Win64 Development and 1-24 letters, digits or underscores.");
            OutputFile = "Binaries/Win64/divingsim-" + Slot + ".exe";
        }
    }
}
